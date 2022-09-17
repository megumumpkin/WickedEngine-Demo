#include "Resources.h"

using namespace Game::Resources;

void Library::Instance::Init(wi::jobsystem::context* joblist){
    if(joblist == nullptr) joblist = &scene->job_scenestream;

    auto static_file = file.c_str();
    auto static_entity_name = entity_name.c_str();
    bool has_entity_name = (entity_name != "");

    if(collection_id == wi::ecs::INVALID_ENTITY)
    {
        wi::jobsystem::Execute(*joblist, [=](wi::jobsystem::JobArgs args_job){
            scene->mutex_scenestream.lock();
            auto find_collection = scene->collections.find(wi::helper::string_hash(static_file));
            scene->mutex_scenestream.unlock();
            
            if(find_collection == scene->collections.end())
            {
                if(strategy == LOAD_DIRECT && !has_entity_name && (scene->collections.find(wi::helper::string_hash(static_file)) == scene->collections.end()))
                {
                    scene->collections[wi::helper::string_hash(static_file)] = instance_id;
                    Game::Resources::Scene load_scene;
                    wi::ecs::EntitySerializer seri;

                    auto temp_root = wi::scene::LoadModel(load_scene.wiscene,static_file,XMMatrixIdentity(),true);

                    std::scoped_lock thread_lock (scene->mutex_scenestream);

                    load_scene.wiscene.FindAllEntities(entities);
                    entities.erase(temp_root);
                    scene->wiscene.Merge(load_scene.wiscene);

                    for(auto entity : entities)
                    {
                        if(scene->wiscene.hierarchy.Contains(entity))
                        {
                            auto hierarchyComponent = scene->wiscene.hierarchy.GetComponent(entity);
                            if(hierarchyComponent->parentID == temp_root)
                                scene->wiscene.Component_Attach(entity, instance_id);
                        }

                        if(type == LIBRARY && !scene->disabled.Contains(entity))
                        {
                            scene->Entity_Disable(entity);
                        }
                    }
                    
                    scene->wiscene.Entity_Remove(temp_root, false);

                    collection_id = instance_id;
                }
                else
                {
                    std::scoped_lock thread_lock (scene->mutex_scenestream);

                    auto lib_name = "LIB_"+std::string(static_file);

                    if(scene->wiscene.Entity_FindByName(lib_name.c_str()) == wi::ecs::INVALID_ENTITY)
                    {
                        auto lib_entity = scene->CreateInstance(lib_name);
                        auto lib_instance = scene->instances.GetComponent(lib_entity);
                        
                        lib_instance->file = static_file;
                        lib_instance->type = LIBRARY;
                        lib_instance->instance_id = lib_entity;
                        lib_instance->scene = scene;
                    }
                }
                
            }
            else
            {
                scene->mutex_scenestream.lock();
                auto temp_collection_id = find_collection->second;
                auto collection = scene->instances.GetComponent(temp_collection_id); 
                bool loaded = (collection->collection_id != wi::ecs::INVALID_ENTITY);
                scene->mutex_scenestream.unlock();

                std::scoped_lock thread_lock (scene->mutex_scenestream);

                if(loaded)
                {
                    wi::ecs::EntitySerializer seri;

                    for(auto entity : collection->entities)
                    {
                        bool clone = false;
                        if(std::string(static_entity_name) != "")
                        {
                            if(scene->wiscene.names.Contains(entity))
                            {
                                auto nameComponent = scene->wiscene.names.GetComponent(entity);
                                if(nameComponent->name == std::string(static_entity_name))
                                    clone = true;
                            }
                            
                        }else clone = true;

                        bool clone_rule_allow = scene->wiscene.transforms.Contains(entity);

                        if(clone && clone_rule_allow)
                        {
                            auto clone_entity = scene->Entity_Clone(entity, seri, false);

                            if(scene->wiscene.hierarchy.Contains(clone_entity))
                            {
                                auto hierarchyComponent = scene->wiscene.hierarchy.GetComponent(clone_entity);
                                if(hierarchyComponent->parentID == temp_collection_id)
                                    scene->wiscene.Component_Attach(clone_entity, instance_id);
                            }
                        }
                    }

                    for(auto entity_remap : seri.remap)
                    {
                        entities.insert(entity_remap.second);
                    }

                    collection_id = temp_collection_id;
                }
            }

            std::scoped_lock thread_lock (scene->mutex_scenestream);
        });
    }
}

void Library::Instance::Unload(){
    if(instance_id == collection_id)
    {
        scene->collections.erase(wi::helper::string_hash(file.c_str()));
    }

    for(auto& entity : entities)
    {
        scene->wiscene.Entity_Remove(entity);
    }
    entities.clear();

    collection_id = wi::ecs::INVALID_ENTITY;
}

void Library::Instance::Serialize(wi::Archive& archive, wi::ecs::EntitySerializer& seri){
    if(archive.IsReadMode())
    {
        archive >> file;
        archive >> entity_name;
        archive >> (uint32_t&)strategy;
        archive >> (uint32_t&)type;
    }
    else
    {
        archive << file;
        archive << entity_name;
        archive << strategy;
        archive << type;
    }
}



void Library::Disabled::Serialize(wi::Archive& archive, wi::ecs::EntitySerializer& seri){}



void Library::Stream::Serialize(wi::Archive &archive, wi::ecs::EntitySerializer &seri){
    if(archive.IsReadMode())
    {
        archive >> external_substitute_object;
    }
    else 
    {
        archive << external_substitute_object;
    }
    stream_zone.Serialize(archive, seri);
}



wi::ecs::Entity Scene::CreateInstance(std::string name){
    auto entity = wi::ecs::CreateEntity();

    auto& namecomponent = wiscene.names.Create(entity);
    namecomponent.name = name;
    wiscene.layers.Create(entity);
    wiscene.transforms.Create(entity);

    auto instance = instances.Create(entity);

    return entity;
}

void Scene::SetStreamable(wi::ecs::Entity entity, bool set, wi::primitive::AABB bound){
    if(set)
    {
        auto& streamcomponent = streams.Create(entity);
        streamcomponent.stream_zone = bound;
    }
    else
    {
        if(streams.Contains(entity))
        {
            streams.Remove(entity);
        }
    }
}

static std::vector<std::string> disable_list = {
    "wi::scene::Scene::objects",
    "wi::scene::Scene::impostors",
    "wi::scene::Scene::rigidbodies",
    "wi::scene::Scene::softbodies",
    "wi::scene::Scene::armatures",
    "wi::scene::Scene::lights",
    "wi::scene::Scene::cameras",
    "wi::scene::Scene::probes",
    "wi::scene::Scene::forces",
    "wi::scene::Scene::decals",
    "wi::scene::Scene::animations",
    "wi::scene::Scene::animation_datas",
    "wi::scene::Scene::emitters",
    "wi::scene::Scene::hairs",
    "wi::scene::Scene::weathers",
    "wi::scene::Scene::sounds",
    "wi::scene::Scene::inverse_kinematics",
    "wi::scene::Scene::springs",
    "wi::scene::Scene::colliders",
    "wi::scene::Scene::scripts",
    "wi::scene::Scene::expressions",
    "wi::scene::Scene::terrains",
    "game::component::streams"
};

void Scene::Entity_Disable(wi::ecs::Entity entity){
    auto& disable_store = disabled.Create(entity);

    auto& store = disable_store.entity_store;
    store.SetReadModeAndResetPos(false);

    wi::ecs::EntitySerializer seri;
    seri.allow_remap = false;
    
    for(auto& to_disable : disable_list)
    {
        auto compmgr = wiscene.componentLibrary.entries[to_disable].component_manager.get();
        auto& compver = wiscene.componentLibrary.entries[to_disable].version;
        seri.version = compver;
        compmgr->Component_Serialize(entity, store, seri);
        compmgr->Remove(entity);
    }
    store << false;
    disable_store.remap.insert(seri.remap.begin(), seri.remap.end());
}

void Scene::Entity_Enable(wi::ecs::Entity entity){
    auto disable_store = disabled.GetComponent(entity);
    if(disable_store != nullptr)
    {
        auto& read = disable_store->entity_store;
        read.SetReadModeAndResetPos(true);

        wi::ecs::EntitySerializer seri;
        seri.allow_remap = false;
        seri.remap.insert(disable_store->remap.begin(), disable_store->remap.end());

        bool has_next = false;
        std::string compmgr_name;
        
        for(auto& to_disable : disable_list)
        {
            auto compmgr = wiscene.componentLibrary.entries[to_disable].component_manager.get();
            auto& compver = wiscene.componentLibrary.entries[to_disable].version;
            seri.version = compver;
            compmgr->Component_Serialize(entity, read, seri);
            compmgr->Remove(entity);
        }

        disabled.Remove(entity);
    }
}

wi::ecs::Entity Scene::Entity_Clone(wi::ecs::Entity entity, wi::ecs::EntitySerializer& seri, bool recursive){
    wi::Archive archive;

    archive.SetReadModeAndResetPos(false);
    wiscene.Entity_Serialize(archive, seri, entity, wi::scene::Scene::EntitySerializeFlags::RECURSIVE);

    archive.SetReadModeAndResetPos(true);
    auto root = wiscene.Entity_Serialize(archive, seri, wi::ecs::INVALID_ENTITY,
        ((recursive) ? wi::scene::Scene::EntitySerializeFlags::RECURSIVE : wi::scene::Scene::EntitySerializeFlags::NONE) | wi::scene::Scene::EntitySerializeFlags::KEEP_INTERNAL_ENTITY_REFERENCES);

    if(disabled.Contains(root))
    {
        disabled.Remove(root);
        if(disabled.Contains(entity))
        {
            auto disable_store = disabled.GetComponent(entity);
            auto& read = disable_store->entity_store;
            read.SetReadModeAndResetPos(true);

            wi::ecs::EntitySerializer d_seri;
            d_seri.allow_remap = false;
            d_seri.remap.insert(disable_store->remap.begin(), disable_store->remap.end());

            bool has_next = false;
            std::string compmgr_name;
            for(auto& to_disable : disable_list)
            {
                auto compmgr = wiscene.componentLibrary.entries[to_disable].component_manager.get();
                auto& compver = wiscene.componentLibrary.entries[to_disable].version;
                d_seri.version = compver;
                compmgr->Component_Serialize(root, read, d_seri);
            }
        }
    }
    
    return root;
}

void Scene::Library_Update(float dt){
    // Directly load instance if not set to stream
    for(int i = 0; i < instances.GetCount(); ++i)
    {
        auto& instance = instances[i];
        auto instance_entity = instances.GetEntity(i);
        
        if(instance.scene == nullptr)
        {
            instance.scene = this;
            instance.instance_id = instance_entity;
        }
        if(instance.collection_id == wi::ecs::INVALID_ENTITY && !instance.lock && !streams.Contains(instance_entity)) { instance.Init(); }
    }
    // Else work with streaming
    for(int i = 0; i < streams.GetCount(); ++i)
    {
        auto& stream = streams[i];
        auto stream_entity = streams.GetEntity(i);

        if(stream.stream_zone.intersects(stream_boundary))
        {
            auto instance = instances.GetComponent(stream_entity);
            if(instance != nullptr){
                // Init instances first if the data is not loaded
                if((instance->collection_id == wi::ecs::INVALID_ENTITY) && !instance->lock){
                    instance->Init();
                    for(auto& entity : instance->entities)
                    {
                        auto objectComponent = wiscene.objects.GetComponent(entity);
                        if(objectComponent != nullptr)
                        {
                            stream.instance_original_transparency[entity] = objectComponent->color.w;
                        }
                    }
                }

                // Once loading is done we then can transition this to the loaded data
                if(instance->collection_id != wi::ecs::INVALID_ENTITY){
                    stream.transition += dt*stream_transition_time;
                    stream.transition = std::min(stream.transition,1.f);

                    if(stream.transition > 0.f) // Preload script after it is done!
                    {
                        
                    }
                }
            }
        }
        else
        {
            stream.transition -= dt*stream_transition_time;
            stream.transition = std::max(stream.transition,0.f);

            // Removes instance after transition finishes
            if(stream.transition == 0.f){
                auto instance = instances.GetComponent(stream_entity);
                if(instance != nullptr){
                    if((instance->collection_id != wi::ecs::INVALID_ENTITY) && !instance->lock){
                        instance->Unload();
                        instance->collection_id = wi::ecs::INVALID_ENTITY;
                        stream.instance_original_transparency.clear();
                    }
                }
            }
        }

        // If transition is ongoing, we're going to manipulate the transition of tracked objects!
        if(stream.transition > 0.f && stream.transition < 1.f){
            for(auto& object_it : stream.instance_original_transparency){
                auto objectComponent = wiscene.objects.GetComponent(object_it.first);
                if(objectComponent != nullptr)
                {
                    objectComponent->color.w = object_it.second * stream.transition;
                }
            }
            // Render previews too if available
            auto streampreview = wiscene.objects.GetComponent(stream_entity);
            if(streampreview != nullptr)
            {
                streampreview->color.w = 1.0 - stream.transition;
            }
        }
    }
}

void Scene::Update(float dt){
    Library_Update(dt);
}

void Scene::Clear()
{
    wiscene.Clear();
    collections.clear();
    stream_boundary = wi::primitive::AABB();
}

void LiveUpdate::Update(float dt){
    
}