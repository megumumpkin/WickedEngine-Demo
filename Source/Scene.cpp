#include "Scene.h"
#include "Filesystem.h"
#include "Gameplay.h"

#include <mutex>
#include <chrono>
#include <algorithm>

namespace Game{
    wi::jobsystem::context stream_job;
    std::mutex stream_mutex;

    wi::ecs::Entity Scene::Prefab::FindEntityByName(std::string name)
    {
        wi::ecs::Entity get_entity = wi::ecs::INVALID_ENTITY;
        auto find_entity = entity_name_map.find(name);
        if(find_entity != entity_name_map.end())
            get_entity = find_entity->second;
        return get_entity;
    }
    void Scene::Prefab::Enable()
    {
        for(auto& map_pair : remap)
        {
            auto& target_entity = map_pair.second;
            GetScene().Entity_Enable(target_entity);
        }
        disabled = false;
    }
    void Scene::Prefab::Disable()
    {
        for(auto& map_pair : remap)
        {
            auto& target_entity = map_pair.second;
            GetScene().Entity_Disable(target_entity);
        }
        disabled = true;
    }
    void Scene::Prefab::Unload()
    {
        for(auto& map_pair : remap)
        {
            auto& target_entity = map_pair.second;
            GetScene().wiscene.Entity_Remove(target_entity, false);
        }
        loaded = false;
    }
    Scene::Prefab::~Prefab()
    {
        // // Stash remap data away
        // GetScene().prefab_remap_stash[tostash_prefabID] = remap;
    }

    void Scene::Component_Prefab::Serialize(wi::Archive &archive, wi::ecs::EntitySerializer &seri)
    {
        if(archive.IsReadMode())
        {
            archive >> file;
            archive >> (uint32_t&)copy_mode;
            archive >> (uint32_t&)stream_mode;
            archive >> stream_distance_multiplier;
        }
        else
        {
            archive << file;
            archive << uint32_t(copy_mode);
            archive << uint32_t(stream_mode);
            archive << stream_distance_multiplier;
        }
    }
    void Scene::Component_Script::Serialize(wi::Archive &archive, wi::ecs::EntitySerializer &seri)
    {
        if(archive.IsReadMode())
        {
            archive >> file;
            archive >> params;
        }
        else
        {
            archive << file;
            archive << params;
        }
    }

    std::shared_ptr<Scene::StreamJob> GetStreamJobData() // Pointer to stream job
    {
        static std::shared_ptr<Scene::StreamJob> stream_data = std::make_shared<Scene::StreamJob>();
        return stream_data;
    }

    struct _internal_Scene_Serialize_DEPRECATED_PreviousFrameTransformComponent
	{
		void Serialize(wi::Archive& archive, wi::ecs::EntitySerializer& seri) { /*this never serialize any data*/ }
	};
    void _internal_Scene_Serialize(wi::scene::Scene& scene, wi::Archive& archive, wi::ecs::EntitySerializer& seri, wi::ecs::Entity root = wi::ecs::INVALID_ENTITY)
    {
        wi::Timer timer;
        // auto archive = wi::Archive(fileName, readMode);
		if (archive.IsReadMode())
		{
			uint32_t reserved;
			archive >> reserved;
		}
		else
		{
			uint32_t reserved = 0;
			archive << reserved;
		}

		// Keeping this alive to keep serialized resources alive until entity serialization ends:
		wi::resourcemanager::ResourceSerializer resource_seri;
		if (archive.GetVersion() >= 63)
		{
			wi::resourcemanager::Serialize(archive, resource_seri);
		}

		if(archive.GetVersion() >= 84)
		{
			// New scene serialization path with component library:
			scene.componentLibrary.Serialize(archive, seri);
		}

		// Additional data serializations:
		if (archive.GetVersion() >= 85)
		{
			scene.ddgi.Serialize(archive);
		}

        // Parent all unparented transforms to new root entity
        if(root != wi::ecs::INVALID_ENTITY)
        {
            for (size_t i = 0; i < scene.transforms.GetCount() - 1; ++i) // GetCount() - 1 because the last added was the "root"
            {
                wi::ecs::Entity entity = scene.transforms.GetEntity(i);
                if (!scene.hierarchy.Contains(entity))
                {
                    scene.Component_Attach(entity, root);
                }
            }
        }
		// wi::backlog::post("Scene serialize took " + std::to_string(timer.elapsed_seconds()) + " sec");
    }
    void _internal_Scene_Merge(wi::scene::Scene& target, wi::scene::Scene& other)
    {
        wi::vector<std::string> entry_str;
        for (auto& entry : target.componentLibrary.entries)
		{
			// entry.second.component_manager->Merge(*other.componentLibrary.entries[entry.first].component_manager);
            entry_str.push_back(entry.first);
        }
        wi::jobsystem::context merge_ctx;
        wi::jobsystem::Dispatch(merge_ctx, entry_str.size(), 1, [&](wi::jobsystem::JobArgs jobArgs){
            target.componentLibrary.entries[entry_str[jobArgs.jobIndex]]
                .component_manager->Merge(*other.componentLibrary.entries[entry_str[jobArgs.jobIndex]].component_manager);
        });
        wi::jobsystem::Wait(merge_ctx);
    }

    wi::ecs::Entity _internal_ecs_clone_entity(wi::ecs::Entity entity, wi::ecs::EntitySerializer& seri)
    {
        wi::ecs::Entity clone_entity = wi::ecs::INVALID_ENTITY;
        if(seri.remap.find(entity) != seri.remap.end())
        {
            clone_entity = seri.remap[entity];
        }
        else
        {
            clone_entity = wi::ecs::CreateEntity();
            seri.remap[entity] = clone_entity;
        }
        return clone_entity;
    }

    void _internal_Clone_Prefab(Scene::Archive& archive, wi::ecs::Entity clone_prefabID)
    {
        Scene::Prefab* find_clone_prefab = GetScene().prefabs.GetComponent(clone_prefabID);
        Scene::Prefab* find_prefab = GetScene().prefabs.GetComponent(archive.prefabID);
        if(find_clone_prefab != nullptr)
        {
            // Check if there is a stashed remap data

            wi::ecs::EntitySerializer seri;
            if(find_clone_prefab->copy_mode == Scene::Prefab::CopyMode::DEEP_COPY)
            {
                for(auto& map_pair : archive.remap)
                {
                    auto& origin_entity = map_pair.second;
                    if(find_clone_prefab->remap.find(origin_entity) == find_clone_prefab->remap.end())
                        find_clone_prefab->remap[origin_entity] = wi::ecs::CreateEntity();
                }
            }
            for(auto& map_pair : archive.remap)
            {
                auto& origin_entity = map_pair.second;
                auto clone_entity = GetScene().Entity_Clone(origin_entity, seri, (find_clone_prefab->copy_mode == Scene::Prefab::CopyMode::DEEP_COPY) ? true : false, &(find_clone_prefab->remap));
            }
            wi::jobsystem::Wait(seri.ctx);

            // Remap parenting for shallow copy
            for(auto& map_pair : find_clone_prefab->remap)
            {
                // If it has no parent then we attach them to prefab
                wi::scene::HierarchyComponent* original_hierarchy = GetScene().wiscene.hierarchy.GetComponent(map_pair.first);
                wi::scene::HierarchyComponent* hierarchy = GetScene().wiscene.hierarchy.GetComponent(map_pair.second);
                if((original_hierarchy != nullptr) && (hierarchy != nullptr))
                {
                    if(original_hierarchy->parentID == archive.prefabID)
                        GetScene().wiscene.Component_Attach(map_pair.second, clone_prefabID, true);
                    else
                        GetScene().wiscene.Component_Attach(map_pair.second, find_clone_prefab->remap[original_hierarchy->parentID], true);
                }
                else
                    GetScene().wiscene.Component_Attach(map_pair.second, clone_prefabID, true);
            }

            // Clone object fade data
            if(find_prefab != nullptr)
            {
                wi::jobsystem::context fade_data_ctx;
                if(find_clone_prefab->fade_data.size() < find_prefab->fade_data.size())
                    find_clone_prefab->fade_data.resize(find_prefab->fade_data.size());
                wi::jobsystem::Dispatch(fade_data_ctx, uint32_t(find_prefab->fade_data.size()), 255, [find_clone_prefab, find_prefab](wi::jobsystem::JobArgs jobArgs){
                    auto& fade_pair = find_prefab->fade_data[jobArgs.jobIndex];
                    wi::ecs::Entity object_remapped = find_clone_prefab->remap[fade_pair.first];
                    find_clone_prefab->fade_data[jobArgs.jobIndex] = {object_remapped, fade_pair.second};
                    // Set clone object's initial fade
                    wi::scene::ObjectComponent* object = GetScene().wiscene.objects.GetComponent(object_remapped);
                    if(object != nullptr)
                        object->color.w = 0.f;
                    
                    // Also do other stuff
                    Game::Scripting::Script* script = GetScene().scripts.GetComponent(object_remapped);
                    if(script != nullptr)
                    {
                        script->done_init = false;
                    }
                });
                wi::jobsystem::Wait(fade_data_ctx);
            }

            find_clone_prefab->loaded = true;
        }
    }
    wi::vector<std::shared_ptr<Scene::StreamData>> stream_callbacks;
    void _internal_Run_Stream_Job(std::shared_ptr<Scene::StreamJob> stream_job_data)
    {
        wi::jobsystem::Execute(stream_job, [stream_job_data](wi::jobsystem::JobArgs jobargs){
            size_t it = 0;
            size_t it_end = 0;
            it_end = stream_job_data->stream_queue.size();
            while(it < it_end)
            {
                bool return_callback = false;

                auto stream_data_ptr = stream_job_data->stream_queue.at(it);

                if(stream_data_ptr->stream_type == Scene::StreamData::StreamType::INIT)
                    return_callback = true;

                if(wi::helper::FileExists(stream_data_ptr->actual_file))
                {
                    wi::ecs::EntitySerializer seri;
                    seri.remap = stream_data_ptr->remap;

                    stream_data_ptr->block = std::make_shared<Scene>();
                    Gameplay::SceneInit(&stream_data_ptr->block->wiscene);
                    auto ar_stream = wi::Archive(stream_data_ptr->actual_file);

                    switch(stream_data_ptr->stream_type)
                    {
                        case Scene::StreamData::StreamType::INIT:
                        {
                            stream_data_ptr->preview_transform.Serialize(ar_stream, seri);
                            stream_data_ptr->clone_prefabID = stream_data_ptr->block->wiscene.Entity_Serialize(
                                ar_stream,
                                seri,
                                wi::ecs::INVALID_ENTITY,
                                wi::scene::Scene::EntitySerializeFlags::NONE
                            );
                            wi::jobsystem::Wait(seri.ctx);
                            break;
                        }
                        case Scene::StreamData::StreamType::FULL:
                        {
                            _internal_Scene_Serialize(stream_data_ptr->block->wiscene, ar_stream, seri, wi::ecs::INVALID_ENTITY);
                            wi::jobsystem::Wait(seri.ctx);

                            // Check object for listing - prefab only
                            if (stream_data_ptr->is_prefab)
                            {
                                wi::jobsystem::context enlist_ctx;
                                wi::jobsystem::Dispatch(enlist_ctx, stream_data_ptr->block->wiscene.objects.GetCount(), 255, [stream_data_ptr](wi::jobsystem::JobArgs jobArgs){
                                    auto objectID = stream_data_ptr->block->wiscene.objects.GetEntity(jobArgs.jobIndex);
                                    wi::scene::ObjectComponent& object = stream_data_ptr->block->wiscene.objects[jobArgs.jobIndex];

                                    stream_data_ptr->fade_data.push_back({objectID,object.color.w});
                                    object.color.w = 0.f;
                                });
                                wi::jobsystem::Wait(enlist_ctx);
                            }

                            return_callback = true;
                            break;
                        }
                    }

                    stream_data_ptr->remap = seri.remap;
                }

                std::scoped_lock scene_sync(stream_mutex);

                if(return_callback)
                    stream_callbacks.push_back(stream_data_ptr);

                it++;
                it_end = stream_job_data->stream_queue.size();
            }
            stream_job_data->stream_queue.clear();
        });
    }
    void _internal_Finish_Stream(std::shared_ptr<Scene::StreamData> stream_callback)
    {
        Scene::Archive& archive = GetScene().scene_db[stream_callback->file];
        switch(stream_callback->stream_type)
        {
            case Scene::StreamData::StreamType::INIT:
            {
                if(stream_callback->block != nullptr)
                {
                    // GetScene().wiscene.Merge(stream_callback->block->wiscene);
                    _internal_Scene_Merge(Game::GetScene().wiscene, stream_callback->block->wiscene);
                    archive.previewID = stream_callback->clone_prefabID;
                    archive.preview_transform = stream_callback->preview_transform;
                }
                archive.load_state = Scene::Archive::LoadState::UNLOADED;
                break;
            }
            case Scene::StreamData::StreamType::FULL:
            {
                archive.remap = stream_callback->remap;

                // GetScene().wiscene.Merge(stream_callback->block->wiscene);
                _internal_Scene_Merge(Game::GetScene().wiscene, stream_callback->block->wiscene);

                // Work on the prefab too if it exists
                Scene::Prefab* find_prefab = GetScene().prefabs.GetComponent(archive.prefabID);
                if(find_prefab != nullptr)
                {
                    find_prefab->remap = archive.remap;
                    // Need to parent components to the scene
                    for(auto& map_pair : find_prefab->remap)
                    {
                        // If it has no parent then we attach them to prefab
                        bool set_parent = false;
                        wi::scene::HierarchyComponent* hierarchy = GetScene().wiscene.hierarchy.GetComponent(map_pair.second);
                        if(hierarchy != nullptr)
                        {
                            if(hierarchy->parentID == wi::ecs::INVALID_ENTITY)
                                set_parent = true;
                        }
                        else
                            set_parent = true;
                        if(set_parent)
                            GetScene().wiscene.Component_Attach(map_pair.second, archive.prefabID, true);

                        // If prefab is a library then we need to disable the entity
                        if(find_prefab->copy_mode == Scene::Prefab::CopyMode::LIBRARY)
                        {
                            auto& target_entity = map_pair.second;
                            GetScene().Entity_Disable(target_entity);
                            find_prefab->disabled = true;
                        }
                        for(auto& map_pair : find_prefab->remap)
                        {
                            auto& target_entity = map_pair.second;
                            wi::scene::NameComponent* name_get = GetScene().wiscene.names.GetComponent(target_entity);
                            if(name_get != nullptr)
                                find_prefab->entity_name_map[name_get->name] = target_entity;
                        }
                    }
                    // Store fade data
                    find_prefab->fade_data = stream_callback->fade_data;
                    
                    find_prefab->loaded = true;
                }

                archive.load_state = Scene::Archive::LoadState::LOADED;
                break;
            }
        }
    }
    void Scene::Archive::Init()
    {
        // Add data to stream job
        GetStreamJobData()->stream_queue.push_back(std::make_shared<StreamData>());
        StreamData* stream_data_init = GetStreamJobData()->stream_queue.back().get();
        stream_data_init->stream_type = StreamData::StreamType::INIT;
        stream_data_init->file = file;
        stream_data_init->actual_file = Filesystem::GetActualPath(wi::helper::ReplaceExtension(file, "preview"));
        stream_data_init->clone_prefabID = previewID;

        // Load boundary first, since it is small and predictable
        {
            wi::ecs::EntitySerializer seri;
            wi::Archive ar_bounds = wi::Archive(wi::helper::ReplaceExtension(stream_data_init->actual_file, "bounds"));
            bounds.Serialize(ar_bounds, seri);
        }
    }
    void Scene::Archive::Load(wi::ecs::Entity clone_prefabID)
    {
        if(load_state == LoadState::UNLOADED)
        {
            load_state = LoadState::LOADING;

            // Check if there is a stashed remap data

            // Add data to stream job
            GetStreamJobData()->stream_queue.push_back(std::make_shared<StreamData>());
            StreamData* stream_data_init = GetStreamJobData()->stream_queue.back().get();
            stream_data_init->stream_type = StreamData::StreamType::FULL;
            stream_data_init->file = file;
            stream_data_init->actual_file = Filesystem::GetActualPath(file);
            wi::backlog::post(stream_data_init->actual_file);
            stream_data_init->remap = remap;
            stream_data_init->is_prefab = (prefabID != wi::ecs::INVALID_ENTITY);
        }

        if((clone_prefabID != wi::ecs::INVALID_ENTITY) && (load_state == LoadState::LOADED))
        {
            dependency_count++;
            _internal_Clone_Prefab(*this, clone_prefabID);
        }

        if((clone_prefabID == wi::ecs::INVALID_ENTITY) && (load_state == LoadState::LOADED)) // Re-enable prefab
        {
            Prefab* prefab = GetScene().prefabs.GetComponent(prefabID);
            if(prefab != nullptr)
            {
                if(prefab->disabled)
                    prefab->Enable();
            }
        }
    }
    void Scene::Archive::Unload(wi::ecs::Entity clone_prefabID)
    {
        if(clone_prefabID != wi::ecs::INVALID_ENTITY)
        {
            Prefab* clone_prefab = GetScene().prefabs.GetComponent(clone_prefabID);
            Prefab* prefab = GetScene().prefabs.GetComponent(prefabID);
            if(clone_prefab != nullptr)
            {
                clone_prefab->Unload();
                dependency_count = std::max(dependency_count-1, uint32_t(0));
            }
            if((prefab->copy_mode == Prefab::CopyMode::LIBRARY) && (dependency_count == 0))
            {
                prefab->Unload();
                load_state = LoadState::UNLOADED;   
            }
        }
        if(clone_prefabID == wi::ecs::INVALID_ENTITY)
        {
            Prefab* prefab = GetScene().prefabs.GetComponent(prefabID);
            if(prefab != nullptr)
            {
                if(dependency_count > 0)
                {
                    if(!prefab->disabled) 
                        prefab->Disable();
                }
                else
                {
                    prefab->Unload();
                    load_state = LoadState::UNLOADED;
                }
                    
            }
        }
    }

    wi::vector<std::string> disable_filter_list = { // Any element that is not commented out will be disabled by the component
        // "wi::scene::Scene::names",
        // "wi::scene::Scene::layers",
        // "wi::scene::Scene::transforms",
        // "wi::scene::Scene::hierarchy",
        // "wi::scene::Scene::materials",
        // "wi::scene::Scene::meshes",
        "wi::scene::Scene::impostors",
        "wi::scene::Scene::objects",
        "wi::scene::Scene::rigidbodies",
        "wi::scene::Scene::softbodies",
        // "wi::scene::Scene::armatures",
        "wi::scene::Scene::lights",
        "wi::scene::Scene::cameras",
        "wi::scene::Scene::probes",
        "wi::scene::Scene::forces",
        "wi::scene::Scene::decals",
        // "wi::scene::Scene::animations",
        // "wi::scene::Scene::animation_datas",
        "wi::scene::Scene::emitters",
        "wi::scene::Scene::hairs",
        "wi::scene::Scene::weathers",
        "wi::scene::Scene::sounds",
        "wi::scene::Scene::inverse_kinematics",
        "wi::scene::Scene::springs",
        "wi::scene::Scene::colliders",
        "wi::scene::Scene::scripts",
        "wi::scene::Scene::expressions",
        "wi::scene::Scene::humanoids",
        "wi::scene::Scene::terrains",
    };
    wi::vector<std::string> full_clone_filter_list = { // Any that is commented out will not be cloned over!
        "wi::scene::Scene::names",
        "wi::scene::Scene::layers",
        "wi::scene::Scene::transforms",
        "wi::scene::Scene::hierarchy",
        "wi::scene::Scene::materials",
        "wi::scene::Scene::meshes",
        "wi::scene::Scene::impostors",
        "wi::scene::Scene::objects",
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
        "wi::scene::Scene::humanoids",
        "wi::scene::Scene::terrains",
        "Game::Scene::Prefab",
        "Game::Scene::Script"
    };
    wi::vector<std::string> shallow_clone_filter_list = { // Any that is commented out will not be cloned over!
        "wi::scene::Scene::names",
        "wi::scene::Scene::layers",
        "wi::scene::Scene::transforms",
        "wi::scene::Scene::hierarchy",
        // "wi::scene::Scene::materials",
        // "wi::scene::Scene::meshes",
        "wi::scene::Scene::impostors",
        "wi::scene::Scene::objects",
        "wi::scene::Scene::rigidbodies",
        "wi::scene::Scene::softbodies",
        // "wi::scene::Scene::armatures",
        "wi::scene::Scene::lights",
        "wi::scene::Scene::cameras",
        "wi::scene::Scene::probes",
        "wi::scene::Scene::forces",
        "wi::scene::Scene::decals",
        // "wi::scene::Scene::animations",
        // "wi::scene::Scene::animation_datas",
        "wi::scene::Scene::emitters",
        "wi::scene::Scene::hairs",
        "wi::scene::Scene::weathers",
        "wi::scene::Scene::sounds",
        "wi::scene::Scene::inverse_kinematics",
        "wi::scene::Scene::springs",
        "wi::scene::Scene::colliders",
        "wi::scene::Scene::scripts",
        "wi::scene::Scene::expressions",
        "wi::scene::Scene::humanoids",
        "wi::scene::Scene::terrains",
        "Game::Scene::Prefab",
        "Game::Scene::Script"
    };
    bool Scene::Entity_Exists(wi::ecs::Entity entity)
    {
        for(auto& component_pair : wiscene.componentLibrary.entries)
        {
            auto compmgr = component_pair.second.component_manager.get();
            if(compmgr->Contains(entity))
                return true;
        }
        return false;
    }
    void Scene::Entity_Disable(wi::ecs::Entity entity)
    {
        auto& inactive = inactives.Create(entity);
        wi::ecs::EntitySerializer seri;
        seri.allow_remap = false;
        auto ar_disable = wi::Archive();
        ar_disable.SetReadModeAndResetPos(false);
        // Create jump headers
        wi::unordered_map<std::string, size_t> component_jump_list;
        for(auto& componentID : disable_filter_list)
        {
            component_jump_list[componentID] = ar_disable.WriteUnknownJumpPosition();
        }
        // Write actual entity data
        for(auto& componentID : disable_filter_list)
        {
            ar_disable.PatchUnknownJumpPosition(component_jump_list[componentID]);
            auto compmgr = wiscene.componentLibrary.entries[componentID].component_manager.get();
            auto& compver = wiscene.componentLibrary.entries[componentID].version;
            seri.version = compver;
            compmgr->Component_Serialize(entity, ar_disable, seri);
            wi::jobsystem::Wait(seri.ctx);
            compmgr->Remove(entity);
        }
        ar_disable.WriteData(inactive.inactive_storage);
    }
    void Scene::Entity_Enable(wi::ecs::Entity entity)
    {
        auto inactive = inactives.GetComponent(entity);
        if(inactive != nullptr)
        {
            wi::ecs::EntitySerializer seri;
            seri.allow_remap = false;
            auto ar_enable = wi::Archive(inactive->inactive_storage.data());
            ar_enable.SetReadModeAndResetPos(true);
            // Read jump headers
            wi::unordered_map<std::string, size_t> component_jump_list;
            for(auto& componentID : disable_filter_list)
            {
                size_t jump;
                ar_enable >> jump;
                component_jump_list[componentID] = jump;
            }
            // Write actual entity data
            for(auto& componentID : disable_filter_list)
            {
                auto compmgr = wiscene.componentLibrary.entries[componentID].component_manager.get();
                auto& compver = wiscene.componentLibrary.entries[componentID].version;
                seri.version = compver;
                compmgr->Component_Serialize(entity, ar_enable, seri);
            }
            wi::jobsystem::Wait(seri.ctx);
        }
        inactives.Remove(entity);
    }
    wi::ecs::Entity Scene::Entity_Clone(
        wi::ecs::Entity entity,
        wi::ecs::EntitySerializer& seri,
        bool deep_copy, 
        wi::unordered_map<uint64_t, wi::ecs::Entity>* clone_seri)
    {
        // Clone filter'll cut out inactive and prefab from being cloned, as it poses some risk
        wi::Archive ar_copy;

        if(clone_seri != nullptr)
            seri.remap = *clone_seri;
        bool temp_remap = seri.allow_remap;
        if(!deep_copy)
            seri.allow_remap = false;
        // seri.allow_remap = false;

        wi::vector<std::string>* clone_filter_list = &shallow_clone_filter_list;
        if(deep_copy)
            clone_filter_list = &full_clone_filter_list;
        
        ar_copy.SetReadModeAndResetPos(false);
        for(auto& componentID : *clone_filter_list)
        {
            auto compmgr = wiscene.componentLibrary.entries[componentID].component_manager.get();
            auto& compver = wiscene.componentLibrary.entries[componentID].version;
            seri.version = compver;
            compmgr->Component_Serialize(entity, ar_copy, seri);
        }
        wi::jobsystem::Wait(seri.ctx);

        wi::ecs::Entity clone_entity = wi::ecs::INVALID_ENTITY;
        if(clone_seri != nullptr)
        {
            auto find_clone = clone_seri->find(entity);
            if(find_clone != clone_seri->end())
                clone_entity = find_clone->second;
        }
        if(clone_entity == wi::ecs::INVALID_ENTITY)
            clone_entity = wi::ecs::CreateEntity();
        

        ar_copy.SetReadModeAndResetPos(true);
        for(auto& componentID : *clone_filter_list)
        {
            auto compmgr = wiscene.componentLibrary.entries[componentID].component_manager.get();
            auto& compver = wiscene.componentLibrary.entries[componentID].version;
            seri.version = compver;
            compmgr->Component_Serialize(clone_entity, ar_copy, seri);
        }
        wi::jobsystem::Wait(seri.ctx);

        if(clone_seri != nullptr)
        {
            (*clone_seri)[entity] = clone_entity;
            if(deep_copy)
            {
                clone_seri->insert(seri.remap.begin(), seri.remap.end());
            }
        }

        auto inactive = inactives.GetComponent(entity);
        if(inactive != nullptr)
        {
            wi::Archive ar_enable = wi::Archive(inactive->inactive_storage.data());
            ar_enable.SetReadModeAndResetPos(true);
            // Read jump headers
            wi::unordered_map<std::string, size_t> component_jump_list;
            for(auto& componentID : disable_filter_list)
            {
                size_t jump;
                ar_enable >> jump;
                component_jump_list[componentID] = jump;
            }
            for(auto& componentID : (*clone_filter_list))
            {
                auto find_jump = component_jump_list.find(componentID);
                if(find_jump != component_jump_list.end())
                {
                    ar_enable.Jump(find_jump->second);
                    auto compmgr = wiscene.componentLibrary.entries[componentID].component_manager.get();
                    auto& compver = wiscene.componentLibrary.entries[componentID].version;
                    seri.version = compver;
                    compmgr->Component_Serialize(clone_entity, ar_enable, seri);
                }
            }
            wi::jobsystem::Wait(seri.ctx);
        }

        return clone_entity;
    }

    void Scene::Load(std::string file)
    {
        // Create an archive to work with
        scene_db[file] = {};

        // Set the archive data
        current_scene = file;
        scene_db[file].file = file;
        scene_db[file].load_state = Archive::LoadState::UNLOADED;
        scene_db[file].Load();
    }

    struct _internal_ScriptUpdateSystem_enlist_job
    {
        wi::unordered_set<wi::ecs::Entity> script_load_list;
        std::mutex script_update_mutex;
    };
    void Scene::RunScriptUpdateSystem(wi::jobsystem::context &ctx)
    {
        _internal_ScriptUpdateSystem_enlist_job script_enlist_job;
        wi::jobsystem::Dispatch(ctx, scripts.GetCount(), 255, [this, &script_enlist_job](wi::jobsystem::JobArgs jobArgs){
            bool do_load = false;

            auto scriptID = scripts.GetEntity(jobArgs.jobIndex);
            Scripting::Script& script = scripts[jobArgs.jobIndex];

            if(!script.done_init)
            {
                std::scoped_lock script_list_sync(script_enlist_job.script_update_mutex);
                script_enlist_job.script_load_list.insert(scriptID);
            }
        });
        wi::jobsystem::Wait(ctx);
        for(auto& scriptID : script_enlist_job.script_load_list)
        {
            Scripting::Script* script = scripts.GetComponent(scriptID);
            // wi::lua::RunText("dofile(\""+Filesystem::GetActualPath(script->file)+"\","+std::to_string(scriptID)+")");
            lua_State* L = wi::lua::GetLuaState();
            lua_getglobal(L, "dofile");
            lua_pushstring(L, Filesystem::GetActualPath(script->file).c_str());
            lua_pushinteger(L, scriptID);
            std::string params_temp = "local function GetEntity() return " + std::to_string(scriptID) + "; end;"+script->params;
            lua_pushstring(L, params_temp.c_str());
            lua_call(L, 3, 1);
            wi::ecs::Entity stub_PID = (wi::ecs::Entity)wi::lua::SGetLongLong(L, -1);
            lua_pop(L, 1);
            script->done_init = true;
        }
    }
    struct _internal_PrefabUpdateSystem_stream_enlist_job
    {
        float dt;
        wi::unordered_map<wi::ecs::Entity,std::string> deferred_lib_stream;
        wi::vector<std::pair<wi::ecs::Entity,float>> fade_update_list;
        wi::vector<std::pair<wi::ecs::Entity,std::pair<wi::ecs::Entity, wi::scene::TransformComponent*>>> preview_create_list;
        wi::unordered_map<wi::ecs::Entity, std::string> load_list;
        wi::unordered_map<wi::ecs::Entity, std::string> unload_list;
        wi::unordered_map<std::string, wi::ecs::Entity> library_create_list;
        std::mutex stream_list_mutex;
    };
    void Scene::RunPrefabUpdateSystem(float dt, wi::jobsystem::context& ctx)
    {
        // Finish load callback
        for(auto stream_callback : stream_callbacks)
        {
            _internal_Finish_Stream(stream_callback);
        }
        stream_callbacks.clear();

        // Stream prefabs
        _internal_PrefabUpdateSystem_stream_enlist_job stream_enlist_job;
        stream_enlist_job.dt = dt;
        wi::jobsystem::Dispatch(ctx, prefabs.GetCount(), 255, [this, &stream_enlist_job](wi::jobsystem::JobArgs jobArgs){
            bool do_init = false;
            bool do_stream = false;
            
            auto prefabID = prefabs.GetEntity(jobArgs.jobIndex);
            Prefab& prefab = prefabs[jobArgs.jobIndex];

            auto find_archive = scene_db.find(prefab.file);

            if(find_archive == scene_db.end()) // Create archive first
            {
                scene_db[prefab.file] = {};
                Archive& archive = scene_db[prefab.file];
                archive.file = prefab.file;
                archive.Init();
            }

            // Determine whether to load or not
            if(find_archive != (scene_db.end()))
            {
                Archive& archive = find_archive->second;

                // Is loadable check START
                bool is_loadable = false;
                switch(prefab.stream_mode)
                {
                    case Prefab::StreamMode::DIRECT:
                    {
                        is_loadable = true;
                        break;
                    }
                    case Prefab::StreamMode::DISTANCE:
                    {
                        // Calculate by zone distance
                        wi::primitive::AABB zone_check = archive.bounds;
                        wi::scene::TransformComponent transformator;
                        
                        XMFLOAT3 zone_center = zone_check.getCenter();
                        transformator.Translate(XMFLOAT3(-zone_center.x, -zone_center.y, -zone_center.z));
                        transformator.UpdateTransform();
                        zone_check = zone_check.transform(transformator.world);
                        
                        transformator = wi::scene::TransformComponent();
                        transformator.Scale(XMFLOAT3(prefab.stream_distance_multiplier, prefab.stream_distance_multiplier, prefab.stream_distance_multiplier));
                        transformator.UpdateTransform();
                        zone_check = zone_check.transform(transformator.world);

                        transformator = wi::scene::TransformComponent();
                        transformator.Translate(XMFLOAT3(zone_center.x, zone_center.y, zone_center.z));
                        transformator.UpdateTransform();
                        zone_check = zone_check.transform(transformator.world);
                        
                        wi::scene::TransformComponent* prefab_transform = wiscene.transforms.GetComponent(prefabID);
                        if(prefab_transform != nullptr)
                            transformator = *prefab_transform;
                        zone_check = zone_check.transform(transformator.world);

                        is_loadable = zone_check.intersects(
                            wi::primitive::Sphere(
                                XMFLOAT3(stream_loader_bounds.x, stream_loader_bounds.y, stream_loader_bounds.z),
                                stream_loader_bounds.w)
                        );
                        break;
                    }
                    case Prefab::StreamMode::SCREEN_ESTATE:
                    {
                        // Calculate by zone estate
                        wi::primitive::AABB zone_check = archive.bounds;
                        wi::scene::TransformComponent transformator;
                        
                        XMFLOAT3 zone_center = zone_check.getCenter();
                        transformator.Translate(XMFLOAT3(-zone_center.x, -zone_center.y, -zone_center.z));
                        transformator.UpdateTransform();
                        zone_check = zone_check.transform(transformator.world);
                        
                        transformator = wi::scene::TransformComponent();
                        transformator.Scale(XMFLOAT3(prefab.stream_distance_multiplier, prefab.stream_distance_multiplier, prefab.stream_distance_multiplier));
                        transformator.UpdateTransform();
                        zone_check = zone_check.transform(transformator.world);

                        transformator = wi::scene::TransformComponent();
                        transformator.Translate(XMFLOAT3(zone_center.x, zone_center.y, zone_center.z));
                        transformator.UpdateTransform();
                        zone_check = zone_check.transform(transformator.world);
                        
                        wi::scene::TransformComponent* prefab_transform = wiscene.transforms.GetComponent(prefabID);
                        if(prefab_transform != nullptr)
                            transformator = *prefab_transform;
                        zone_check = zone_check.transform(transformator.world);

                        is_loadable = ((zone_check.getRadius()/wi::math::Distance(zone_check.getCenter(),wi::scene::GetCamera().Eye)) > (stream_loader_screen_estate));
                        break;
                    }
                    default:
                        break;
                }
                // Is loadable check END

                if((wiscene.meshes.Contains(archive.previewID)) && (prefab.preview_object == wi::ecs::INVALID_ENTITY)) // Create preview object
                    stream_enlist_job.preview_create_list.push_back({prefabID,{archive.previewID, &archive.preview_transform}});

                if(is_loadable && (!prefab.loaded) /*&& (prefab.fade_factor <= 0.f)*/) // Load
                {
                    prefab.tostash_prefabID = prefabID;
                    if(archive.load_state == Archive::LoadState::UNLOADED)
                    {
                        std::scoped_lock stream_list_sync(stream_enlist_job.stream_list_mutex);

                        if(prefab.copy_mode == Prefab::CopyMode::DEEP_COPY)
                        {
                            if(archive.prefabID == wi::ecs::INVALID_ENTITY)
                            {
                                stream_enlist_job.library_create_list[archive.file] = prefabID;
                            }
                            else
                                stream_enlist_job.load_list[wi::ecs::INVALID_ENTITY] = archive.file; // Reload
                        }
                        else
                        {
                            archive.prefabID = prefabID;
                            archive.remap = prefab.remap;
                            stream_enlist_job.load_list[wi::ecs::INVALID_ENTITY] = archive.file;
                        }
                    }

                    if(archive.load_state == Archive::LoadState::LOADED) // Clone, but only after streaming
                    {
                        std::scoped_lock stream_list_sync(stream_enlist_job.stream_list_mutex);
                        stream_enlist_job.load_list[((archive.prefabID == prefabID) ? wi::ecs::INVALID_ENTITY : prefabID)] = archive.file;
                    }    
                }

                
                if(is_loadable && prefab.loaded && (prefab.fade_factor < 1.f)) // Fade in
                {
                    prefab.fade_factor = std::min(prefab.fade_factor + (stream_transition_speed*stream_enlist_job.dt), 1.f);
                    
                    std::scoped_lock stream_list_sync(stream_enlist_job.stream_list_mutex);
                    
                    if(prefab.preview_object != wi::ecs::INVALID_ENTITY)
                        stream_enlist_job.fade_update_list.push_back({prefab.preview_object, 1.f-prefab.fade_factor});

                    for(auto& fade_object : prefab.fade_data)
                    {
                        stream_enlist_job.fade_update_list.push_back({fade_object.first, fade_object.second * prefab.fade_factor});
                    }
                }

                if(!is_loadable && prefab.loaded && (prefab.fade_factor > 0.f)) // Fade out
                {
                    prefab.fade_factor = std::max(prefab.fade_factor - (stream_transition_speed*stream_enlist_job.dt), 0.f);

                    std::scoped_lock stream_list_sync(stream_enlist_job.stream_list_mutex);

                    if(prefab.preview_object != wi::ecs::INVALID_ENTITY)
                        stream_enlist_job.fade_update_list.push_back({prefab.preview_object, 1.f-prefab.fade_factor});

                    for(auto& fade_object : prefab.fade_data)
                    {
                        stream_enlist_job.fade_update_list.push_back({fade_object.first, fade_object.second * prefab.fade_factor});
                    }
                }

                if(!is_loadable && prefab.loaded && (prefab.fade_factor == 0.f)) // Unload
                {
                    std::scoped_lock stream_list_sync(stream_enlist_job.stream_list_mutex);
                    stream_enlist_job.unload_list[((archive.prefabID == prefabID) ? wi::ecs::INVALID_ENTITY : prefabID)] = archive.file;
                }
            }
        });
        wi::jobsystem::Wait(ctx);

        // Library creation
        for(auto& library_create_pair : stream_enlist_job.library_create_list)
        {
            Archive& archive = scene_db[library_create_pair.first];
            wi::ecs::EntitySerializer seri;
            wi::ecs::Entity library_prefabID = Entity_Clone(library_create_pair.second, seri, true);
            wi::jobsystem::Wait(ctx);
            Prefab* library_prefab = prefabs.GetComponent(library_prefabID);
            library_prefab->copy_mode = Prefab::CopyMode::LIBRARY;
            library_prefab->stream_mode = Prefab::StreamMode::DIRECT;
            
            archive.prefabID = library_prefabID;
            archive.remap = library_prefab->remap;
            archive.Load();
        }

        // Preview object creation
        for(auto& preview_create_pair : stream_enlist_job.preview_create_list)
        {
            Prefab* prefab = prefabs.GetComponent(preview_create_pair.first);
            prefab->preview_object = wiscene.Entity_CreateObject("PREVIEW_OBJECT");

            wi::scene::TransformComponent* transform = wiscene.transforms.GetComponent(prefab->preview_object);
            transform->translation_local = preview_create_pair.second.second->translation_local;
            transform->rotation_local = preview_create_pair.second.second->rotation_local;
            transform->scale_local = preview_create_pair.second.second->scale_local;
            transform->UpdateTransform();
            wiscene.Component_Attach(prefab->preview_object, preview_create_pair.first, true);
            
            wi::scene::ObjectComponent* object = wiscene.objects.GetComponent(prefab->preview_object);
            object->meshID = preview_create_pair.second.first;
        }

        // Fade management
        wi::jobsystem::Dispatch(ctx, uint32_t(stream_enlist_job.fade_update_list.size()), 255, [this, &stream_enlist_job](wi::jobsystem::JobArgs jobArgs){
            auto& fade_pair = stream_enlist_job.fade_update_list[jobArgs.jobIndex];
            wi::scene::ObjectComponent* object = wiscene.objects.GetComponent(fade_pair.first);
            if(object != nullptr)
                object->color.w = fade_pair.second;
        });
        wi::jobsystem::Wait(ctx);

        // Process loads
        for(auto& load_pair : stream_enlist_job.load_list)
        {
            Archive& archive = scene_db[load_pair.second];
            archive.Load(load_pair.first);
        }

        // Process unloads
        for(auto& unload_pair : stream_enlist_job.unload_list)
        {
            Archive& archive = scene_db[unload_pair.second];
            archive.Unload(unload_pair.first);
        }
    }

    void Scene::PreUpdate(float dt)
    {
        wi::jobsystem::context update_ctx;
        std::scoped_lock stream_lock(stream_mutex);

        // Run scripting update
        RunScriptUpdateSystem(update_ctx);
        // Run prefab updates
        RunPrefabUpdateSystem(dt, update_ctx);
    }

    void Scene::Update(float dt)
    {
        // wi::jobsystem::context update_ctx;
        // std::scoped_lock stream_lock(stream_mutex);
        if(!wi::jobsystem::IsBusy(stream_job))
        {
            if((GetStreamJobData()->stream_queue.size() > 0))
                _internal_Run_Stream_Job(GetStreamJobData());
        }
    }
}