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
    void Scene::Component_Inactive::Serialize(wi::Archive &archive, wi::ecs::EntitySerializer &seri)
    {
        wi::vector<uint8_t> data;
        if(archive.IsReadMode())
        {
            archive >> data;
            inactive_storage = wi::Archive();
            inactive_storage << data;
        }
        else
        {
            inactive_storage.WriteData(data);
            archive << data;
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
    // void _internal_Scene_Merge(wi::scene::Scene& target, wi::scene::Scene& other)
    // {
    //     wi::vector<std::string> entry_str;
    //     for (auto& entry : target.componentLibrary.entries)
	// 	{
	// 		// entry.second.component_manager->Merge(*other.componentLibrary.entries[entry.first].component_manager);
    //         entry_str.push_back(entry.first);
    //     }
    //     wi::jobsystem::context merge_ctx;
    //     wi::jobsystem::Dispatch(merge_ctx, entry_str.size(), 1, [&](wi::jobsystem::JobArgs jobArgs){
    //         target.componentLibrary.entries[entry_str[jobArgs.jobIndex]]
    //             .component_manager->Merge(*other.componentLibrary.entries[entry_str[jobArgs.jobIndex]].component_manager);
    //     });
    //     wi::jobsystem::Wait(merge_ctx);
    // }

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

        bool do_clone = false;
        if(find_clone_prefab != nullptr)
            do_clone = true;

        if(do_clone)
        {
            auto find_archive = GetScene().scene_db.find(find_clone_prefab->file);
            
            // Check if there is a stashed remap data
            // TODO

            wi::ecs::EntitySerializer seri;

            // Deep Copy Init
            if(find_clone_prefab->copy_mode == Scene::Prefab::CopyMode::DEEP_COPY)
            {
                for(auto origin_entity : archive.entities)
                {
                    if(find_clone_prefab->remap.find(origin_entity) == find_clone_prefab->remap.end())
                        find_clone_prefab->remap[origin_entity] = wi::ecs::CreateEntity();
                }
            }

            // Clone
            Scene::Prefab::CopyMode copy_mode = find_clone_prefab->copy_mode;
            wi::unordered_set<wi::ecs::Entity> clone_entities;
            wi::unordered_map<uint64_t, wi::ecs::Entity> clone_remap;
            clone_remap.insert(find_clone_prefab->remap.begin(), find_clone_prefab->remap.end());
            for(auto origin_entity : archive.entities)
            {
                auto clone_entity = GetScene().Entity_Clone(origin_entity, seri, (copy_mode == Scene::Prefab::CopyMode::DEEP_COPY) ? true : false, &clone_remap);
                clone_entities.insert(clone_entity);
            }
            wi::jobsystem::Wait(seri.ctx);
            find_clone_prefab = GetScene().prefabs.GetComponent(clone_prefabID);
            find_clone_prefab->entities.insert(clone_entities.begin(),clone_entities.end());
            find_clone_prefab->remap.insert(clone_remap.begin(),clone_remap.end());

            wi::unordered_set<wi::ecs::Entity> springs_root_list;
		    wi::unordered_map<wi::ecs::Entity,wi::vector<wi::ecs::Entity>> springs_hierarchy_list;
            for(auto origin_entity : archive.entities)
            {
                // If it has no parent then we attach them to prefab
                wi::scene::HierarchyComponent* original_hierarchy = GetScene().wiscene.hierarchy.GetComponent(origin_entity);
                wi::scene::HierarchyComponent* hierarchy = GetScene().wiscene.hierarchy.GetComponent(clone_remap[origin_entity]);
                if((original_hierarchy != nullptr) && (hierarchy != nullptr))
                {
                    if(original_hierarchy->parentID == archive.archiveID)
                        GetScene().wiscene.Component_Attach(clone_remap[origin_entity], clone_prefabID, true);
                    else
                        GetScene().wiscene.Component_Attach(clone_remap[origin_entity], clone_remap[original_hierarchy->parentID], true);
                }

                //Springs
                if(GetScene().wiscene.springs.Contains(clone_remap[origin_entity]))
                {
                    bool has_parent = false;
                    if(hierarchy != nullptr)
                    {
                        if(GetScene().wiscene.springs.Contains(hierarchy->parentID))
                        {
                            springs_hierarchy_list[hierarchy->parentID].push_back(clone_remap[origin_entity]);
                            has_parent = true;
                        }
                    }

                    if(!has_parent)
                    {
                        springs_root_list.insert(clone_remap[origin_entity]);
                    }
                }
            }

            // Spring reprocess
            std::atomic<size_t> r_idx = 0;
            wi::unordered_set<wi::ecs::Entity> reorder_done;
            for(auto& root : springs_root_list)
            {
                std::function<void(wi::ecs::Entity)> reorder_fn;
                reorder_fn = [&](wi::ecs::Entity target)
                    {
                        if (reorder_done.find(target) == reorder_done.end()){
                            size_t idx = GetScene().wiscene.springs.GetIndex(target);
                            GetScene().wiscene.springs[idx].Reset(true);
                            GetScene().wiscene.springs.MoveItem(idx, r_idx);
                            reorder_done.insert(target);
                            r_idx.fetch_add(1);
                        }

                        auto find_hierarchy = springs_hierarchy_list.find(target);
                        if(find_hierarchy != springs_hierarchy_list.end())
                        {
                            for(auto& hierarchy_child : find_hierarchy->second)
                            {
                                reorder_fn(hierarchy_child);
                                // wi::backlog::post("[MURAMASA] child of "+ std::to_string(target) +" > "+std::to_string(state.scene->springs.GetIndex(target)));
                            }
                        }
                    };

                reorder_fn(root);
            }

            // Transfer data from archive
            if(find_archive != GetScene().scene_db.end())
            {
                for(auto& fade_pair : find_archive->second.fade_data)
                {
                    find_clone_prefab->fade_data.push_back({clone_remap[fade_pair.first], fade_pair.second});
                }
                for(auto& name_pair : find_archive->second.entity_name_map)
                {
                    find_clone_prefab->entity_name_map.insert({name_pair.first, clone_remap[name_pair.second]});
                }
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

                    if(stream_data_ptr->stream_type == Scene::StreamData::StreamType::INIT)
                    {
                        stream_data_ptr->preview_transform.Serialize(ar_stream, seri);
                        stream_data_ptr->block->wiscene.componentLibrary.Entity_Serialize(stream_data_ptr->archiveID, ar_stream, seri);
                        wi::jobsystem::Wait(seri.ctx);
                    }
                    else
                    {
                        _internal_Scene_Serialize(stream_data_ptr->block->wiscene, ar_stream, seri, wi::ecs::INVALID_ENTITY);
                        wi::jobsystem::Wait(seri.ctx);

                        if(stream_data_ptr->stream_type == Scene::StreamData::StreamType::FULL)
                        {
                            stream_data_ptr->block->wiscene.FindAllEntities(stream_data_ptr->entities);

                            for(auto& entity : stream_data_ptr->entities)
                            {
                                wi::scene::NameComponent* name = stream_data_ptr->block->wiscene.names.GetComponent(entity);
                                if(name != nullptr)
                                    stream_data_ptr->entity_name_map.insert({name->name, entity});

                                Scene::FadeData fade_data;
                                wi::scene::ObjectComponent* object = stream_data_ptr->block->wiscene.objects.GetComponent(entity);
                                if(object != nullptr)
                                {
                                    fade_data.object = object->color.w;
                                }
                                wi::scene::LightComponent* light = stream_data_ptr->block->wiscene.lights.GetComponent(entity);
                                if(light != nullptr)
                                {
                                    fade_data.light = light->intensity;
                                }
                                wi::scene::MaterialComponent* material = stream_data_ptr->block->wiscene.materials.GetComponent(entity);
                                if(material != nullptr)
                                {
                                    fade_data.material = material->baseColor.w;
                                }
                                stream_data_ptr->fade_data.insert({entity, fade_data});

                                bool do_parent = false;
                                wi::scene::HierarchyComponent* hier = stream_data_ptr->block->wiscene.hierarchy.GetComponent(entity);
                                if(hier != nullptr)
                                {
                                    if(hier->parentID == wi::ecs::INVALID_ENTITY)
                                        do_parent = true;
                                }
                                else
                                    do_parent = true;
                                if(do_parent)
                                    stream_data_ptr->block->wiscene.Component_Attach(entity, stream_data_ptr->archiveID);

                                // Store datas
                                stream_data_ptr->block->Entity_Disable(entity);
                            }
                        }
                    }

                    stream_data_ptr->remap = seri.remap;
                    return_callback = true;
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
        if(stream_callback->stream_type == Scene::StreamData::StreamType::INIT)
        {
            if(stream_callback->block != nullptr)
            {
                GetScene().wiscene.Merge(stream_callback->block->wiscene);
                archive.preview_transform = stream_callback->preview_transform;
            }
            archive.load_state = Scene::Archive::LoadState::UNLOADED;
        }
        else
        {
            GetScene().wiscene.Merge(stream_callback->block->wiscene);
            archive.remap = stream_callback->remap;
            if(stream_callback->stream_type == Scene::StreamData::StreamType::FULL)
            {
                archive.fade_data.insert(stream_callback->fade_data.begin(),stream_callback->fade_data.end());
                archive.entity_name_map.insert(stream_callback->entity_name_map.begin(),stream_callback->entity_name_map.end());
                archive.entities.insert(stream_callback->entities.begin(), stream_callback->entities.end());
            }
            archive.load_state = Scene::Archive::LoadState::LOADED;
        }
    }
    void Scene::Archive::Init()
    {
        archiveID = wi::ecs::CreateEntity();

        // Add data to stream job
        GetStreamJobData()->stream_queue.push_back(std::make_shared<StreamData>());
        StreamData* stream_data_init = GetStreamJobData()->stream_queue.back().get();
        stream_data_init->stream_type = StreamData::StreamType::INIT;
        stream_data_init->file = file;
        stream_data_init->actual_file = Filesystem::GetActualPath(wi::helper::ReplaceExtension(file, "preview"));
        stream_data_init->archiveID = archiveID;

        wi::ecs::EntitySerializer seri;
        wi::Archive ar_bounds = wi::Archive(wi::helper::ReplaceExtension(stream_data_init->actual_file, "bounds"));
        bounds.Serialize(ar_bounds, seri);
    }
    void Scene::Archive::Load(wi::ecs::Entity prefabID)
    {   
        if( (prefabID != wi::ecs::INVALID_ENTITY) && 
            (load_state == LoadState::LOADED) )
        {
            dependency_count++;
            _internal_Clone_Prefab(*this, prefabID);
        }

        if(load_state == LoadState::UNLOADED)
        {
            // Check if there is a stashed remap data

            // Add data to stream job
            GetStreamJobData()->stream_queue.push_back(std::make_shared<StreamData>());
            StreamData* stream_data_init = GetStreamJobData()->stream_queue.back().get();
            stream_data_init->stream_type = (is_root) ? StreamData::StreamType::ROOT : StreamData::StreamType::FULL;
            stream_data_init->file = file;
            stream_data_init->actual_file = Filesystem::GetActualPath(file);
            stream_data_init->archiveID = archiveID;
            stream_data_init->remap = remap;

            load_state = LoadState::LOADING;
        }

        if(load_state == LoadState::UNINITIALIZED)
        {
            Init();
        }
    }
    void Scene::Archive::Unload(wi::ecs::Entity clone_prefabID)
    {
        // if(clone_prefabID != wi::ecs::INVALID_ENTITY)
        // {
        //     Prefab* clone_prefab = GetScene().prefabs.GetComponent(clone_prefabID);
        //     Prefab* prefab = GetScene().prefabs.GetComponent(prefabID);
        //     if(clone_prefab != nullptr)
        //     {
        //         clone_prefab->Unload();
        //         dependency_count = std::max(dependency_count-1, uint32_t(0));
        //     }
        //     if((prefab->copy_mode == Prefab::CopyMode::LIBRARY) && (dependency_count == 0))
        //     {
        //         prefab->Unload();
        //         load_state = LoadState::UNLOADED;   
        //     }
        // }
        // if(clone_prefabID == wi::ecs::INVALID_ENTITY)
        // {
        //     Prefab* prefab = GetScene().prefabs.GetComponent(prefabID);
        //     if(prefab != nullptr)
        //     {
        //         if(dependency_count > 0)
        //         {
        //             if(!prefab->disabled) 
        //                 prefab->Disable();
        //         }
        //         else
        //         {
        //             prefab->Unload();
        //             load_state = LoadState::UNLOADED;
        //         }
                    
        //     }
        // }
    }

    wi::vector<std::string> storage_filter_list = { // Any that is commented out will not be cloned over!
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
        "wi::scene::Scene::animations",
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
        if(!inactives.Contains(entity))
        {
            auto& inactive = inactives.Create(entity);
            wi::ecs::EntitySerializer seri;
            seri.allow_remap = false;

            auto& ar_disable = inactive.inactive_storage;
            ar_disable.SetReadModeAndResetPos(false);

            bool script_done = false;
            Scripting::Script* script = scripts.GetComponent(entity);
            if(script != nullptr)
                script_done = script->done_init;

            for(auto& componentID : storage_filter_list)
            {
                auto compmgr = wiscene.componentLibrary.entries[componentID].component_manager.get();
                auto& compver = wiscene.componentLibrary.entries[componentID].version;
                seri.version = compver;
                compmgr->Component_Serialize(entity, ar_disable, seri);
                wi::jobsystem::Wait(seri.ctx);
                compmgr->Remove(entity);
            }

            ar_disable << script_done;
        }
    }
    void Scene::Entity_Enable(wi::ecs::Entity entity)
    {
        auto inactive = inactives.GetComponent(entity);
        if(inactive != nullptr)
        {
            wi::ecs::EntitySerializer seri;
            seri.allow_remap = false;

            auto& ar_enable = inactive->inactive_storage;
            ar_enable.SetReadModeAndResetPos(true);

            // Write actual entity data
            for(auto& componentID : storage_filter_list)
            {
                auto compmgr = wiscene.componentLibrary.entries[componentID].component_manager.get();
                auto& compver = wiscene.componentLibrary.entries[componentID].version;
                seri.version = compver;
                compmgr->Component_Serialize(entity, ar_enable, seri);
            }
            wi::jobsystem::Wait(seri.ctx);

            bool script_done = false;
            ar_enable >> script_done;
            Scripting::Script* script = scripts.GetComponent(entity);
            if(script != nullptr)
                script->done_init = script_done;
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
            seri.remap.insert(clone_seri->begin(), clone_seri->end());
        bool temp_remap = seri.allow_remap;
        if(!deep_copy)
            seri.allow_remap = false;
        
        ar_copy.SetReadModeAndResetPos(false);
        GetScene().wiscene.componentLibrary.Entity_Serialize(entity, ar_copy, seri);
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
        GetScene().wiscene.componentLibrary.Entity_Serialize(clone_entity, ar_copy, seri);
        wi::jobsystem::Wait(seri.ctx);

        if(clone_seri != nullptr)
        {
            clone_seri->insert({entity,clone_entity});
            if(deep_copy)
            {
                clone_seri->insert(seri.remap.begin(), seri.remap.end());
            }
        }

        auto inactive = inactives.GetComponent(entity);
        if(inactive != nullptr)
        {
            wi::Archive& ar_enable = inactive->inactive_storage;
            ar_enable.SetReadModeAndResetPos(true);
            for(auto& componentID : storage_filter_list)
            {
                auto compmgr = wiscene.componentLibrary.entries[componentID].component_manager.get();
                auto& compver = wiscene.componentLibrary.entries[componentID].version;
                seri.version = compver;
                compmgr->Component_Serialize(clone_entity, ar_enable, seri);
            }
            wi::jobsystem::Wait(seri.ctx);
        }
        if(inactives.Contains(clone_entity))
            inactives.Remove(clone_entity);

        return clone_entity;
    }

    void Scene::Load(std::string file)
    {
        // Create an archive to work with
        scene_db[file] = {};

        // Set the archive data
        current_scene = file;
        scene_db[file].file = file;
        scene_db[file].is_root = true;
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
        wi::vector<std::pair<wi::ecs::Entity,Scene::FadeData>> fade_update_list;
        wi::vector<std::pair<wi::ecs::Entity,std::pair<wi::ecs::Entity, wi::scene::TransformComponent>>> preview_create_list;
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

            bool archive_ready = true;
            if(find_archive == scene_db.end()) // Create archive first
            {
                scene_db[prefab.file] = {};
                Archive& archive = scene_db[prefab.file];
                archive.file = prefab.file;
                stream_enlist_job.load_list[prefabID] = prefab.file;
                archive_ready = false;
            }
            if(archive_ready)
            {
                if(find_archive->second.load_state == Archive::LoadState::UNLOADED)
                {
                    stream_enlist_job.load_list[wi::ecs::INVALID_ENTITY] = prefab.file;
                    archive_ready = false;
                }
            }

            // Determine whether to load or not
            if(archive_ready)
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

                // Create preview object
                if((wiscene.meshes.Contains(archive.archiveID)) && (prefab.preview_object == wi::ecs::INVALID_ENTITY))
                {
                    std::scoped_lock stream_list_sync (stream_enlist_job.stream_list_mutex);
                    stream_enlist_job.preview_create_list.push_back({prefabID,{archive.archiveID, archive.preview_transform}});
                } 

                prefab.fade_factor = wi::math::Clamp(
                    prefab.fade_factor+((is_loadable) ? 1.0:-1.0)*stream_enlist_job.dt*GetScene().stream_transition_speed,
                    0.f, 1.f);

                if(prefab.fade_factor > 0.f)
                {
                    if( (!prefab.loaded) &&
                        (archive.load_state == Archive::LoadState::LOADED) ) // Clone, but only after streaming
                    {
                        std::scoped_lock stream_list_sync (stream_enlist_job.stream_list_mutex);
                        stream_enlist_job.load_list[prefabID] = archive.file;
                    }

                    std::scoped_lock stream_list_sync(stream_enlist_job.stream_list_mutex);
                    
                    if(prefab.preview_object != wi::ecs::INVALID_ENTITY)
                    {
                        FadeData result;
                        result.object = 1.f-prefab.fade_factor;
                        result.light = 1.f;
                        result.material = 1.f;
                        stream_enlist_job.fade_update_list.push_back({prefab.preview_object, result});
                    }

                    for(auto& fade_object : prefab.fade_data)
                    {
                        FadeData result;
                        result.object = fade_object.second.object * prefab.fade_factor;
                        result.light = fade_object.second.light * prefab.fade_factor;
                        result.material = fade_object.second.material * prefab.fade_factor;
                        stream_enlist_job.fade_update_list.push_back({fade_object.first, result});
                    }
                }
                else
                {
                    if( (prefab.loaded) &&
                        (archive.load_state == Archive::LoadState::LOADED) )
                    {
                        std::scoped_lock stream_list_sync (stream_enlist_job.stream_list_mutex);
                        stream_enlist_job.unload_list[prefabID] = archive.file;
                    }
                }
            }
        });
        wi::jobsystem::Wait(ctx);

        // Preview object creation
        for(auto& preview_create_pair : stream_enlist_job.preview_create_list)
        {
            Prefab* prefab = prefabs.GetComponent(preview_create_pair.first);
            if(prefab != nullptr)
            {
                prefab->preview_object = wiscene.Entity_CreateObject("PREVIEW_OBJECT");

                wi::scene::TransformComponent* transform = wiscene.transforms.GetComponent(prefab->preview_object);
                transform->translation_local = preview_create_pair.second.second.translation_local;
                transform->rotation_local = preview_create_pair.second.second.rotation_local;
                transform->scale_local = preview_create_pair.second.second.scale_local;
                transform->UpdateTransform();
                wiscene.Component_Attach(prefab->preview_object, preview_create_pair.first, true);
                
                wi::scene::ObjectComponent* object = wiscene.objects.GetComponent(prefab->preview_object);
                object->meshID = preview_create_pair.second.first;
            }
        }

        // Fade management
        wi::jobsystem::Dispatch(ctx, uint32_t(stream_enlist_job.fade_update_list.size()), 255, [this, &stream_enlist_job](wi::jobsystem::JobArgs jobArgs){
            auto& fade_pair = stream_enlist_job.fade_update_list[jobArgs.jobIndex];
            wi::scene::ObjectComponent* object = wiscene.objects.GetComponent(fade_pair.first);
            if(object != nullptr)
                object->color.w = fade_pair.second.object;

            wi::scene::LightComponent* light = wiscene.lights.GetComponent(fade_pair.first);
            if(light != nullptr)
                light->intensity = fade_pair.second.light;
            
            wi::scene::MaterialComponent* material = wiscene.materials.GetComponent(fade_pair.first);
            if(material != nullptr)
                material->baseColor.w = fade_pair.second.material;
        });
        wi::jobsystem::Wait(ctx);

        // Process loads
        for(auto& load_pair : stream_enlist_job.load_list)
        {
            Archive& archive = scene_db[load_pair.second];
            archive.Load(load_pair.first);
        }
        stream_enlist_job.load_list.clear();

        // Process unloads
        for(auto& unload_pair : stream_enlist_job.unload_list)
        {
            Archive& archive = scene_db[unload_pair.second];
            archive.Unload(unload_pair.first);
        }
        stream_enlist_job.unload_list.clear();
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