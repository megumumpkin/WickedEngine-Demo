#include "Resources.h"

using namespace Game::Resources;

void Library::Instance::Init(wi::jobsystem::context* joblist){
    if(joblist == nullptr) joblist = &scene->job_scenestream;

    auto static_filepath = file;
    auto static_entityname = entity_name;
    auto static_scene = scene;
    
    wi::jobsystem::Execute(*joblist, [=](wi::jobsystem::JobArgs args_job){
        loading = true;
        bool do_clone = false;

        scene->mutex_scenestream.lock();
        auto find_collection = scene->collections.find(wi::helper::string_hash(file.c_str()));
        if(find_collection == scene->collections.end()) // Load from disk if collection does not exist!
        {
            scene->mutex_scenestream.unlock();
            // Check loading strategy first
            if(strategy == LOAD_DIRECT && entity_name == ""){
                // Store newly made collection to the list
                {
                    std::scoped_lock thread_lock (scene->mutex_scenestream);
                    scene->collections[wi::helper::string_hash(file.c_str())] = instance_id;
                }

                collection_id = instance_id;

                Scene load_scene;
                auto prototype_root = wi::scene::LoadModel(load_scene.wiscene, file, XMMatrixIdentity(), true);

                // Scan the loaded scene and enlist entities
                load_scene.wiscene.FindAllEntities(entities);
                entities.erase(prototype_root);

                std::scoped_lock thread_lock (scene->mutex_scenestream);

                // Merge and set parent to collection
                scene->wiscene.Merge(load_scene.wiscene);
                for(auto entity : entities)
                {
                    auto hierarchyComponent = scene->wiscene.hierarchy.GetComponent(entity);
                    if(hierarchyComponent != nullptr)
                    {
                        if(hierarchyComponent->parentID == prototype_root)
                        {
                            scene->wiscene.Component_Attach(entity, instance_id, true);
                        }
                    }
                }
                scene->wiscene.Entity_Remove(prototype_root, false);

                // Store newly made collection to the list
                scene->collections[wi::helper::string_hash(file.c_str())] = instance_id;

                // If we're loading as library we have to stash non-data objects!
                if(type == LIBRARY){
                    for(auto& entity : entities){
                        bool ignore = (
                            (scene->wiscene.materials.Contains(entity) == true) ||
                            (scene->wiscene.meshes.Contains(entity) == true)
                        );

                        if(!ignore){ // Stash away non data objects by disabling them!
                            scene->Entity_Disable(entity);
                        }
                    }
                }
            } else {
                // Spawn another entity as library and then ask them to load it as library
                auto lib_entity = scene->CreateInstance("LIB_"+file);
                auto lib_instance = scene->instances.GetComponent(lib_entity);

                lib_instance->scene = scene;
                lib_instance->instance_id = lib_entity;
                lib_instance->strategy = LOAD_DIRECT;
                lib_instance->type = LIBRARY;
                lib_instance->file = static_filepath;

                scene->mutex_scenestream.unlock();

                wi::jobsystem::context preload_list;

                lib_instance->Init(&preload_list);
                wi::jobsystem::Wait(preload_list);

                scene->mutex_scenestream.lock();

                find_collection = scene->collections.find(wi::helper::string_hash(static_filepath.c_str()));

                do_clone = true;
            }
        }
        else do_clone = true;
        
        // We can just clone if the collection exists
        if(do_clone)
        { 
            auto collection_entity = find_collection->second;
            auto collection_instance = scene->instances.GetComponent(collection_entity);

            scene->mutex_scenestream.unlock();

            if(collection_instance != nullptr)
            {
                while(collection_instance->loading){}

                wi::ecs::EntitySerializer seri;
                collection_id = collection_entity;
                
                std::scoped_lock thread_lock (scene->mutex_scenestream);
                for(auto& origin_entity :  collection_instance->entities)
                {
                    bool clone = false;
                    if(static_entityname != "")
                    {
                        auto find_entity = origin_entity;
                        auto find_disabled = scene->disabled_list.find(find_entity);
                        if(find_disabled != scene->disabled_list.end())
                        {
                            find_entity = find_disabled->second;
                        }

                        if(scene->wiscene.names.Contains(find_entity))
                        {
                            auto origin_name = scene->wiscene.names.GetComponent(find_entity);
                            if(static_entityname == origin_name->name) 
                                clone = true;
                        }
                    }
                    else clone = true;

                    auto cloned = (seri.remap.find(origin_entity) != seri.remap.end());

                    if(clone && !cloned)
                    {
                        auto new_entity = scene->Entity_Clone(origin_entity, seri);
                        
                        if(static_scene->wiscene.hierarchy.Contains(new_entity))
                        {
                            auto hierarchyComponent = static_scene->wiscene.hierarchy.GetComponent(new_entity);
                            if(hierarchyComponent->parentID == collection_entity)
                                static_scene->wiscene.Component_Attach(new_entity, instance_id, true);
                        }
                    }
                }

                for(auto& entity_remap : seri.remap)
                {
                    entities.insert(entity_remap.second);
                }
            }
        }

        // For streaming objects, we need to track transition fx,
        // so first we need to register objectComponents of the instance
        // And then start the transition from zero!
        auto streamComponent = scene->streams.GetComponent(instance_id);
        if(streamComponent != nullptr)
        {
            for (auto& entity : entities){
                auto objectComponent = scene->wiscene.objects.GetComponent(entity);
                if(objectComponent != nullptr) streamComponent->instance_original_transparency[entity] = objectComponent->GetTransparency();
            }
        }

        loading = false;
    });
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



void Library::Disabled::Serialize(wi::Archive& archive, wi::ecs::EntitySerializer& seri){
    wi::ecs::SerializeEntity(archive, entity, seri);
}



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

void Scene::Entity_Disable(wi::ecs::Entity entity){
    auto disable_handle = wi::ecs::CreateEntity();

    wi::ecs::EntitySerializer seri;
 
    if(wiscene.names.Contains(entity))
    {
        auto nameComponent = wiscene.names.GetComponent(entity);
        auto& disabledNameStore = wiscene.names.Create(disable_handle); 
        disabledNameStore.name = nameComponent->name;
    }

    auto& disabledComponent = disabled.Create(disable_handle);

    disabledComponent.entity_store.SetReadModeAndResetPos(false);
    wiscene.Entity_Serialize(disabledComponent.entity_store, seri, entity,
        wi::scene::Scene::EntitySerializeFlags::RECURSIVE | wi::scene::Scene::EntitySerializeFlags::KEEP_INTERNAL_ENTITY_REFERENCES);

    disabled_list[entity] = disable_handle;
    disabledComponent.remap = seri.remap;

    wiscene.Entity_Remove(entity);
}

void Scene::Entity_Enable(wi::ecs::Entity entity){
    auto find_disabled = disabled_list.find(entity);
    if(find_disabled != disabled_list.end())
    {
        auto disabledComponent = disabled.GetComponent(find_disabled->second);

        wi::ecs::EntitySerializer seri;
        seri.remap = disabledComponent->remap;
        disabledComponent->entity_store.SetReadModeAndResetPos(true);
        wiscene.Entity_Serialize(disabledComponent->entity_store, seri, wi::ecs::INVALID_ENTITY,
            wi::scene::Scene::EntitySerializeFlags::RECURSIVE | wi::scene::Scene::EntitySerializeFlags::KEEP_INTERNAL_ENTITY_REFERENCES);

        wiscene.Entity_Remove(find_disabled->second);
    }
}

wi::ecs::Entity Scene::Entity_Clone(wi::ecs::Entity entity, wi::ecs::EntitySerializer& seri){
    wi::Archive* archive;

    wi::Archive new_archive;
    auto find_disabled = disabled_list.find(entity);
    if(find_disabled != disabled_list.end()) // If disabled then just clone the disabled data
    {
        auto disabledComponent = disabled.GetComponent(find_disabled->second);
        archive = &disabledComponent->entity_store;
        seri.remap = disabledComponent->remap;
    }
    else // If not then just copy it as usual
    {
        archive = &new_archive;
        archive->SetReadModeAndResetPos(false);
        wiscene.Entity_Serialize(*archive, seri, entity, wi::scene::Scene::EntitySerializeFlags::RECURSIVE);
    }

    archive->SetReadModeAndResetPos(true);
    auto root = wiscene.Entity_Serialize(*archive, seri, wi::ecs::INVALID_ENTITY,
        wi::scene::Scene::EntitySerializeFlags::RECURSIVE | wi::scene::Scene::EntitySerializeFlags::KEEP_INTERNAL_ENTITY_REFERENCES);

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
    disabled_list.clear();
    stream_boundary = wi::primitive::AABB();
}

void LiveUpdate::Update(float dt){
    
}