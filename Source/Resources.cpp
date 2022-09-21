#include "Resources.h"

using namespace Game::Resources;

void Library::Instance::Init(){
    if(load_state == UNLOADED)
    {
        load_state = LOADING;

        wi::jobsystem::Execute(scene->job_scenestream, [=](wi::jobsystem::JobArgs args)
        {
            bool has_lib = false;
            Library::Instance lib_data;

            bool instance_clone = false;
            
            scene->mutex_scenestream.lock();
            
            auto job_data = *this;
            
            auto hashed_file = wi::helper::string_hash(job_data.file.c_str());
            auto find_collection = job_data.scene->collections.find(hashed_file);

            wi::ecs::Entity collection_entity = job_data.instance_id;
            if(find_collection == job_data.scene->collections.end())
            {
                job_data.scene->mutex_scenestream.unlock();
                auto load_data = &job_data;
                if(job_data.strategy != LOAD_DIRECT || job_data.entity_name != "")
                {
                    has_lib = true;
                    instance_clone = true;

                    load_data = &lib_data;
                    load_data->scene = job_data.scene;
                    load_data->file = job_data.file;
                    load_data->type = LIBRARY;
                    load_data->load_state = LOADED;

                    std::scoped_lock thread_lock (job_data.scene->mutex_scenestream);
                    collection_entity = wi::ecs::CreateEntity();

                    load_data->instance_id = collection_entity;

                    auto& namecomponent = job_data.scene->wiscene.names.Create(collection_entity);
                    namecomponent.name = "LIB_"+job_data.file;
                    job_data.scene->wiscene.layers.Create(collection_entity);
                    job_data.scene->wiscene.transforms.Create(collection_entity);
                    job_data.collection_id = collection_entity;
                }
                load_data->collection_id = collection_entity;

                Scene load_scene;
                wi::ecs::EntitySerializer seri;

                auto temp_root = wi::scene::LoadModel(load_scene.wiscene, load_data->file, XMMatrixIdentity(), true);
                load_scene.wiscene.FindAllEntities(load_data->entities);
                load_data->entities.erase(temp_root);

                std::scoped_lock thread_lock (job_data.scene->mutex_scenestream);
                job_data.scene->collections[hashed_file] = collection_entity;
                job_data.scene->wiscene.Merge(load_scene.wiscene);

                for(auto& entity : load_data->entities)
                {
                    auto hierarchyComponent = job_data.scene->wiscene.hierarchy.GetComponent(entity);
                    if(hierarchyComponent != nullptr)
                        if(hierarchyComponent->parentID == temp_root)
                            job_data.scene->wiscene.Component_Attach(entity, load_data->instance_id);

                    if(load_data->type == LIBRARY)
                        job_data.scene->Entity_Disable(entity);
                }
                
                job_data.scene->wiscene.Entity_Remove(temp_root,false);
            }
            else 
            {
                job_data.scene->mutex_scenestream.unlock();
                collection_entity = find_collection->second;
                instance_clone = true;
            }

            if(instance_clone)
            {
                bool done_loading = false;
                bool cancel = false;
                Library::Instance* collection = nullptr;

                if(has_lib)
                {
                    collection = &lib_data;
                }
                else 
                {
                    do
                    {
                        std::scoped_lock thread_lock (job_data.scene->mutex_scenestream);
                        collection = job_data.scene->instances.GetComponent(collection_entity);
                        if(collection != nullptr)
                        {
                            if(collection->load_state == LOADED)
                                done_loading = true;
                        }
                        else 
                            cancel = true;
                    }
                    while(!done_loading && !cancel);
                }

                if(!cancel)
                {
                    wi::ecs::EntitySerializer seri;
                    std::scoped_lock thread_lock (job_data.scene->mutex_scenestream);
                    for(auto& entity : collection->entities)
                    {
                        bool clone = false;
                        if(job_data.entity_name != "")
                        {
                            if(scene->wiscene.names.Contains(entity))
                            {
                                auto nameComponent = job_data.scene->wiscene.names.GetComponent(entity);
                                if(nameComponent->name == job_data.entity_name)
                                    clone = true;
                            }
                            
                        }
                        else 
                            clone = true;
                        
                        bool clone_allowed = job_data.scene->wiscene.transforms.Contains(entity);

                        if(clone && clone_allowed)
                        {
                            auto clone_entity = job_data.scene->Entity_Clone(entity, seri, false);

                            auto hierarchyComponent = job_data.scene->wiscene.hierarchy.GetComponent(clone_entity);
                            if(hierarchyComponent != nullptr)
                                if(hierarchyComponent->parentID == collection_entity)
                                    job_data.scene->wiscene.Component_Attach(clone_entity, job_data.instance_id);

                            job_data.entities.insert(clone_entity);
                        }
                    }
                }
            }

            std::scoped_lock thread_lock (scene->mutex_scenestream);

            auto set_data = scene->instances.GetComponent(job_data.instance_id);
            set_data->entities = job_data.entities;
            set_data->load_state = LOADED;

            if(has_lib)
            {
                auto& lib_finish = job_data.scene->instances.Create(collection_entity);
                lib_finish = lib_data;
            }
        });
    }
}

void Library::Instance::Unload()
{
    if((instance_id == collection_id) && collection_id > wi::ecs::INVALID_ENTITY)
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

void Library::Instance::Serialize(wi::Archive& archive, wi::ecs::EntitySerializer& seri)
{
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



void Library::Stream::Serialize(wi::Archive &archive, wi::ecs::EntitySerializer &seri)
{
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



wi::ecs::Entity Scene::CreateInstance(std::string name)
{
    auto entity = wi::ecs::CreateEntity();

    auto& namecomponent = wiscene.names.Create(entity);
    namecomponent.name = name;
    wiscene.layers.Create(entity);
    wiscene.transforms.Create(entity);

    auto instance = instances.Create(entity);

    return entity;
}

void Scene::SetStreamable(wi::ecs::Entity entity, bool set, wi::primitive::AABB bound)
{
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

void Scene::Entity_Disable(wi::ecs::Entity entity)
{
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

void Scene::Entity_Enable(wi::ecs::Entity entity)
{
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

wi::ecs::Entity Scene::Entity_Clone(wi::ecs::Entity entity, wi::ecs::EntitySerializer& seri, bool recursive)
{
    wi::Archive archive;

    auto flag_recursive = (recursive) ? wi::scene::Scene::EntitySerializeFlags::RECURSIVE : wi::scene::Scene::EntitySerializeFlags::NONE;

    archive.SetReadModeAndResetPos(false);
    wiscene.Entity_Serialize(archive, seri, entity, flag_recursive);

    archive.SetReadModeAndResetPos(true);
    auto root = wiscene.Entity_Serialize(archive, seri, wi::ecs::INVALID_ENTITY,
        flag_recursive | wi::scene::Scene::EntitySerializeFlags::KEEP_INTERNAL_ENTITY_REFERENCES);

    if(disabled.Contains(root))
    {
        disabled.Remove(root);
        auto disable_store = disabled.GetComponent(entity);
        if(disable_store != nullptr)
        {
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

void Scene::Library_Update(float dt)
{
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
        if(instance.load_state == Library::Instance::UNLOADED && !instance.lock && !streams.Contains(instance_entity)) { instance.Init(); }
    }
    // Else work with streaming
    for(int i = 0; i < streams.GetCount(); ++i)
    {
        auto& stream = streams[i];
        auto stream_entity = streams.GetEntity(i);

        if(stream.stream_zone.intersects(stream_boundary))
        {
            if(instances.Contains(stream_entity)){
                // Init instances first if the data is not loaded
                auto instance = instances.GetComponent(stream_entity);
                if((instance->load_state == Library::Instance::UNLOADED) && !instance->lock){
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
                if(instances.Contains(stream_entity))
                {
                    auto instance = instances.GetComponent(stream_entity);
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
                if(wiscene.objects.Contains(object_it.first))
                {
                    auto objectComponent = wiscene.objects.GetComponent(object_it.first);
                    objectComponent->color.w = object_it.second * stream.transition;
                }
            }
            // Render previews too if available
            if(wiscene.objects.Contains(stream_entity))
            {
                auto streampreview = wiscene.objects.GetComponent(stream_entity);
                streampreview->color.w = 1.0 - stream.transition;
            }
        }
    }
}

void Scene::Update(float dt)
{
    Library_Update(dt);
}

void Scene::Clear()
{
    wiscene.Clear();
    collections.clear();
    stream_boundary = wi::primitive::AABB();
}

void LiveUpdate::Update(float dt)
{
    
}