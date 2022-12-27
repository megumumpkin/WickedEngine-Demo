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
            archive >> entities_size;
            entities.resize(entities_size);
        }
        else
        {
            archive << block_id;
            archive << block_position.x;
            archive << block_position.y;
            archive << block_position.z;
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
    void Scene::Archive::Save_Block(wi::ecs::Entity block_id)
    {
        //TODO
        // auto block_ar = wi::Archive(fileName + "/blocks/"+std::to_string(block_id), false);
        //How do we approach this?
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
}