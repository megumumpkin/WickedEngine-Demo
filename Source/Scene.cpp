#include "Scene.h"
#include "Filesystem.h"
#include <mutex>

namespace Game{
    Scene* GetScene(){
        static Scene scene;
        return &scene;
    }

    void Scene::Archive::Block_Table::Serialize(wi::Archive &archive, wi::ecs::EntitySerializer &seri)
    {
        size_t entities_size;
        if(archive.IsReadMode())
        {
            archive >> block_id;
            archive >> block_position.x;
            archive >> block_position.y;
            archive >> block_position.z;
            archive >> no_position;
            archive >> entities_size;
            entities.resize(entities_size);
        }
        else
        {
            archive << block_id;
            archive << block_position.x;
            archive << block_position.y;
            archive << block_position.z;
            archive << no_position;
            archive << entities.size();
        }
        for(auto& entity : entities)
        {
            wi::ecs::SerializeEntity(archive, entity, seri);
        }
    }
    void Scene::Archive::Entity_Table::Serialize(wi::Archive &archive, wi::ecs::EntitySerializer &seri)
    {
        wi::ecs::SerializeEntity(archive, entity, seri);
        size_t prerequisites_size;
        if(archive.IsReadMode())
        {
            archive >> block_id;
            archive >> load_radius;
            archive >> prerequisites_size;
            prerequisites.resize(prerequisites_size);
        }
        else
        {
            archive << block_id;
            archive << load_radius;
            archive << prerequisites.size();
        }
        for(auto& entity : prerequisites)
        {
            wi::ecs::SerializeEntity(archive, entity, seri);
        }
    }
    void Scene::Archive::Save_Header()
    {
        auto header_ar = wi::Archive(fileName + "/headfile", false);
        block_table.Serialize(header_ar, seri);
        entity_table.Serialize(header_ar, seri);
    }
    void Scene::Archive::Load_Header()
    {
        auto header_ar = wi::Archive(fileName + "/headfile", true);
        block_table.Serialize(header_ar, seri);
        entity_table.Serialize(header_ar, seri);
    }
    // Modification of ComponentLibrary.Serialize to work with serializing ONLY the selected entities
    void _internal_scene_partial_serialization(wi::Archive& archive, wi::ecs::EntitySerializer& seri, wi::vector<wi::ecs::Entity> entity_list)
    {
        auto& entries = GetScene()->wiscene.componentLibrary.entries;
        if(archive.IsReadMode())
        {
            bool has_next = false;
            do
            {
                archive >> has_next;
                if(has_next)
                {
                    std::string name;
                    archive >> name;
                    uint64_t jump_size = 0;
                    archive >> jump_size;
                    auto it = entries.find(name);
                    if(it != entries.end())
                    {
                        archive >> seri.version;
                        // it->second.component_manager->Serialize(archive, seri);
                        for(auto& entity : entity_list)
                        {
                            if(it->second.component_manager->Contains(entity))
                            {
                                it->second.component_manager->Component_Serialize(entity, archive, seri);
                            }
                        }
                    }
                    else
                    {
                        // component manager of this name was not registered, skip serialization by jumping over the data
                        archive.Jump(jump_size);
                    }
                }
            }
            while(has_next);
        }
        else
        {
            for(auto& it : entries)
            {
                archive << true;
                archive << it.first; // name
                size_t offset = archive.WriteUnknownJumpPosition(); // we will be able to jump from here...
                archive << it.second.version;
                seri.version = it.second.version;
                for(auto& entity : entity_list)
                {
                    if(it.second.component_manager->Contains(entity))
                    {
                        it.second.component_manager->Component_Serialize(entity, archive, seri);
                    }
                }
                archive.PatchUnknownJumpPosition(offset); // ...to here, if this component manager was not registered
            }
            archive << false;
        }
    }
    void Scene::Archive::Save_Block(wi::ecs::Entity block_id)
    {
        auto block = block_table.GetComponent(block_id);
        block->load_state = Scene::Archive::Block_Table::LoadState::LOADING;

        auto block_ar = wi::Archive(fileName + "/blocks/"+std::to_string(block_id), false);
        _internal_scene_partial_serialization(block_ar, seri, block->entities);
    }
    std::mutex block_stream_sync;
    wi::jobsystem::context block_stream_job;
    void Scene::Archive::Stream_Block(wi::ecs::Entity block_id)
    {
        //Let's notify the streaming system that we have an asset loading
        auto block = block_table.GetComponent(block_id);
        block->load_state = Scene::Archive::Block_Table::LoadState::LOADING;
        
        wi::jobsystem::Execute(block_stream_job, [&](wi::jobsystem::JobArgs args){
            // A copy of our block data and serializer will be here, to avoid memory corruption during job
            block_stream_sync.lock();
            auto block_thread = *block;
            wi::ecs::EntitySerializer seri_thread;
            seri_thread.remap = seri.remap;
            block_stream_sync.unlock();

            // Load archive to memory
            auto block_ar = wi::Archive(fileName + "/blocks/"+std::to_string(block_thread.block_id), true);
            Scene block_scene;
            block_scene.wiscene.componentLibrary.Serialize(block_ar, seri_thread);
            block_thread.load_state = Block_Table::LoadState::LOADED;
            
            // Sync, merge to current scene, leave archive memory deallocation to thread
            block_stream_sync.lock();
            GetScene()->wiscene.Merge(block_scene.wiscene);
            auto block_finish = block_table.GetComponent(block_thread.block_id);
            (*block_finish) = block_thread;
            block_stream_sync.unlock();
        });
    }
    void Scene::Archive::Update_EntityTable()
    {
        // Get all the entities that we know is there on scene
        // This is USEFUL when we need to only update the entity that exists, avoiding crashes and unecessary looping
        wi::unordered_set<wi::ecs::Entity> entity_on_scene;
        GetScene()->wiscene.FindAllEntities(entity_on_scene);

        for(int i = 0; i < entity_table.GetCount(); ++i)
        {
            auto& entity_info = entity_table[i];
            
            // SKIP if entity does not exist on scene
            if(entity_on_scene.find(entity_info.entity) == entity_on_scene.end())
                continue;

            // Calculate radius and dependencies at the same times
            wi::unordered_set<wi::ecs::Entity> upd_deps;
            auto c_transform = GetScene()->wiscene.transforms.GetComponent(entity_info.entity);
            if(c_transform != nullptr)
            {
                XMVECTOR S, R, T;
                XMMatrixDecompose(&S, &R, &T, XMLoadFloat4x4(&c_transform->world));
                XMStoreFloat4(&entity_info.load_radius, T);
                XMFLOAT3 scale;
                XMStoreFloat3(&scale, S);
                entity_info.load_radius.w = (scale.x + scale.y + scale.z)/3.f;
            }
            auto c_hierarchy = GetScene()->wiscene.hierarchy.GetComponent(entity_info.entity);
            if(c_hierarchy != nullptr)
            {
                if(c_hierarchy->parentID != wi::ecs::INVALID_ENTITY)
                    upd_deps.insert(c_hierarchy->parentID);
            }
            auto c_mesh = GetScene()->wiscene.meshes.GetComponent(entity_info.entity);
            if(c_mesh != nullptr)
            {
                for(auto& subset : c_mesh->subsets)
                {
                    if(subset.materialID != wi::ecs::INVALID_ENTITY)
                        upd_deps.insert(subset.materialID);
                }
                if(c_mesh->armatureID != wi::ecs::INVALID_ENTITY)
                    upd_deps.insert(c_mesh->armatureID);
            }
            auto c_object = GetScene()->wiscene.objects.GetComponent(entity_info.entity);
            if(c_object != nullptr)
            {
                if(c_object->meshID != wi::ecs::INVALID_ENTITY)
                    upd_deps.insert(c_object->meshID);
            }
            // Skip animation data, animation data needs special data packing in the scene data, and then load by script
            auto c_ik = GetScene()->wiscene.inverse_kinematics.GetComponent(entity_info.entity);
            if(c_ik != nullptr)
            {
                if(c_ik->target != wi::ecs::INVALID_ENTITY)
                    upd_deps.insert(c_ik->target);
            }
            // Skip expression data, we don't know how to handle this

            entity_info.prerequisites.clear();
            entity_info.prerequisites.insert(entity_info.prerequisites.begin(), upd_deps.begin(), upd_deps.end());
        }
    }

    void Scene::Component_Prefab::Serialize(wi::Archive &archive, wi::ecs::EntitySerializer &seri)
    {
        if(archive.IsReadMode())
        {
            archive >> (uint32_t&)loadMode;
            archive >> targetEntity;
        }
        else 
        {
            archive << (uint32_t)loadMode;
            archive << targetEntity;
        }
    }

    wi::unordered_map<std::string, Scene::Archive> scene_database;

    void Scene::Update(float dt)
    {
        
    }
}