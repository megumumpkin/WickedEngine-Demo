#include "Scene.h"
#include "Filesystem.h"
#include "Gameplay.h"

#include <mutex>
#include <chrono>
#include <algorithm>

namespace Game{
    wi::jobsystem::context stream_job;
    std::mutex stream_mutex;
    struct _internal_Stream_Task
    {
        enum class STREAM_STATE
        {
            FREE,
            PENDING,
            INPROGRESS,
            DONE
        };
        STREAM_STATE stream_state = STREAM_STATE::FREE;
        enum class STREAM_TYPE
        {
            ROOT,
            INIT,
            FULL
        };
        STREAM_TYPE stream_type = STREAM_TYPE::INIT;

        bool return_callback = false;

        wi::ecs::Entity archiveID;
        std::string file;
        std::string actual_file;
        Scene scene;
        wi::unordered_set<wi::ecs::Entity> prefabs;
        wi::unordered_set<wi::ecs::Entity> entities;
        wi::unordered_map<uint64_t, wi::ecs::Entity> remap;
        wi::unordered_map<wi::ecs::Entity, Scene::FadeData> fade_data;
        wi::unordered_map<std::string, wi::ecs::Entity> entity_name_map;
        wi::unordered_map<size_t, wi::ecs::Entity> lod_tiers;

        void Clear()
        {
            stream_state = STREAM_STATE::FREE;
            stream_type = STREAM_TYPE::INIT;
            file.clear();
            actual_file.clear();
            entities.clear();
            remap.clear();
            fade_data.clear();
            entity_name_map.clear();
            lod_tiers.clear();
            prefabs.clear();
        }
    };
    _internal_Stream_Task stream_task;

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
        for(auto& target_entity : entities)
        {
            GetScene().Entity_Enable(target_entity);
        }
        disabled = false;
    }
    void Scene::Prefab::Disable()
    {
        for(auto& target_entity : entities)
        {
            GetScene().Entity_Disable(target_entity);
        }
        disabled = true;
    }
    void Scene::Prefab::Unload()
    {
        for(auto& target_entity : entities)
        {
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
        data.clear();
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

            Scene::Prefab::CopyMode copy_mode = find_clone_prefab->copy_mode;
            // // DEBUG
            // copy_mode = Scene::Prefab::CopyMode::DEEP_COPY;

            wi::ecs::EntitySerializer seri;

            // Clone            
            wi::unordered_set<wi::ecs::Entity> clone_entities;
            wi::unordered_map<uint64_t, wi::ecs::Entity> clone_remap;

            clone_remap[archive.archiveID] = clone_prefabID; 
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
            for(auto clone_entity : clone_entities)
            {
                //Springs
                if(GetScene().wiscene.springs.Contains(clone_entity))
                {
                    auto hierarchy = GetScene().wiscene.hierarchy.GetComponent(clone_entity);
                    bool has_parent = false;
                    if(hierarchy != nullptr)
                    {
                        if(GetScene().wiscene.springs.Contains(hierarchy->parentID))
                        {
                            springs_hierarchy_list[hierarchy->parentID].push_back(clone_entity);
                            has_parent = true;
                        }
                    }

                    if(!has_parent)
                    {
                        springs_root_list.insert(clone_entity);
                    }
                }
            }

            // Spring reprocess
            wi::unordered_set<wi::ecs::Entity> reorder_done;
            for(auto& root : springs_root_list)
            {
                std::function<void(wi::ecs::Entity)> reorder_fn;
                reorder_fn = [&](wi::ecs::Entity target)
                    {
                        if (reorder_done.find(target) == reorder_done.end()){
                            size_t idx = GetScene().wiscene.springs.GetIndex(target);
                            GetScene().wiscene.springs[idx].Reset(true);
                            GetScene().wiscene.springs.MoveItem(idx, GetScene().wiscene.springs.GetCount()-1);
                            reorder_done.insert(target);
                        }

                        auto find_hierarchy = springs_hierarchy_list.find(target);
                        if(find_hierarchy != springs_hierarchy_list.end())
                        {
                            for(auto& hierarchy_child : find_hierarchy->second)
                            {
                                reorder_fn(hierarchy_child);
                            }
                        }
                    };

                reorder_fn(root);
            }

            // Transfer data from archive
            if(find_archive != GetScene().scene_db.end())
            {
                for(auto& prefabID : find_archive->second.prefabs)
                {
                    find_clone_prefab->prefabs.insert(clone_remap[prefabID]);
                }
                for(auto& fade_pair : find_archive->second.fade_data)
                {
                    find_clone_prefab->fade_data.push_back({clone_remap[fade_pair.first], fade_pair.second});
                
                    wi::ecs::Entity fade_init_entity = clone_remap[fade_pair.first];
                    wi::scene::ObjectComponent* object = GetScene().wiscene.objects.GetComponent(fade_init_entity);
                    if(object != nullptr)
                    {
                        object->color.w = 0.f;
                    }
                    wi::scene::LightComponent* light = GetScene().wiscene.lights.GetComponent(fade_init_entity);
                    if(light != nullptr)
                    {
                        light->intensity = 0.f;
                    }
                    wi::scene::MaterialComponent* material = GetScene().wiscene.materials.GetComponent(fade_init_entity);
                    if(material != nullptr)
                    {
                        material->baseColor.w = 0.f;
                    }
                }
                for(auto& name_pair : find_archive->second.entity_name_map)
                {
                    find_clone_prefab->entity_name_map.insert({name_pair.first, clone_remap[name_pair.second]});
                }
            }

            find_clone_prefab->loaded = true;
        }
    }
    void _internal_Run_Stream_Job()
    {
        wi::jobsystem::Execute(stream_job, [](wi::jobsystem::JobArgs jobargs){
            {
                std::scoped_lock init_sync (stream_mutex);
                stream_task.stream_state = _internal_Stream_Task::STREAM_STATE::INPROGRESS;
            }

            bool return_callback = false;

            if(stream_task.stream_type == _internal_Stream_Task::STREAM_TYPE::INIT)
                return_callback = true;

            if(wi::helper::FileExists(stream_task.actual_file))
            {
                wi::ecs::EntitySerializer seri;
                seri.remap = stream_task.remap;

                Gameplay::SceneInit(&(stream_task.scene.wiscene));
                auto ar_stream = wi::Archive(stream_task.actual_file,true);

                if(stream_task.stream_type == _internal_Stream_Task::STREAM_TYPE::INIT)
                {
                    size_t count = 0;
                    ar_stream >> count;
                    for(int it = 0; it < count; ++it)
                    {
                        wi::ecs::Entity tier_prefabID = wi::ecs::INVALID_ENTITY;
                        if(it < stream_task.lod_tiers.size())
                            tier_prefabID = stream_task.lod_tiers[it];
                        else
                            tier_prefabID = wi::ecs::CreateEntity();

                        stream_task.scene.wiscene.componentLibrary.Entity_Serialize(tier_prefabID, ar_stream, seri);
                        wi::jobsystem::Wait(seri.ctx);

                        // Store datas
                        stream_task.lod_tiers.insert({it, tier_prefabID});
                        stream_task.scene.Entity_Disable(tier_prefabID);
                    }
                }
                else
                {
                    _internal_Scene_Serialize(stream_task.scene.wiscene, ar_stream, seri, wi::ecs::INVALID_ENTITY);
                    wi::jobsystem::Wait(seri.ctx);

                    if(stream_task.stream_type == _internal_Stream_Task::STREAM_TYPE::FULL)
                    {
                        stream_task.scene.wiscene.FindAllEntities(stream_task.entities);

                        for(auto& entity : stream_task.entities)
                        {
                            if(stream_task.scene.prefabs.Contains(entity))
                                stream_task.prefabs.insert(entity);

                            wi::scene::NameComponent* name = stream_task.scene.wiscene.names.GetComponent(entity);
                            if(name != nullptr)
                                stream_task.entity_name_map.insert({name->name, entity});

                            Scene::FadeData fade_data;
                            wi::scene::ObjectComponent* object = stream_task.scene.wiscene.objects.GetComponent(entity);
                            if(object != nullptr)
                            {
                                fade_data.object = object->color.w;
                            }
                            wi::scene::LightComponent* light = stream_task.scene.wiscene.lights.GetComponent(entity);
                            if(light != nullptr)
                            {
                                fade_data.light = light->intensity;
                            }
                            wi::scene::MaterialComponent* material = stream_task.scene.wiscene.materials.GetComponent(entity);
                            if(material != nullptr)
                            {
                                fade_data.material = material->baseColor.w;
                            }
                            stream_task.fade_data.insert({entity, fade_data});

                            bool do_parent = false;
                            wi::scene::HierarchyComponent* hier = stream_task.scene.wiscene.hierarchy.GetComponent(entity);
                            if(hier != nullptr)
                            {
                                if(hier->parentID == wi::ecs::INVALID_ENTITY)
                                    do_parent = true;
                            }
                            else
                                do_parent = true;
                            if(do_parent)
                                stream_task.scene.wiscene.Component_Attach(entity, stream_task.archiveID);

                            // Store datas
                            stream_task.scene.Entity_Disable(entity);
                        }
                    }
                }

                stream_task.remap = seri.remap;
                return_callback = true;
            }

            std::scoped_lock done_sync (stream_mutex);
            stream_task.return_callback = return_callback;
            stream_task.stream_state = _internal_Stream_Task::STREAM_STATE::DONE;
        });
    }
    void _internal_Finish_Stream()
    {
        Scene::Archive& archive = GetScene().scene_db[stream_task.file];
        if(stream_task.stream_type == _internal_Stream_Task::STREAM_TYPE::INIT)
        {
            if(stream_task.lod_tiers.size() > 0)
            {
                GetScene().wiscene.Merge(stream_task.scene.wiscene);
                for(size_t it = 0; it < stream_task.lod_tiers.size(); ++it)
                {
                    archive.lod_tiers[it] = stream_task.lod_tiers[it];
                }
            }
            archive.load_state = Scene::Archive::LoadState::UNLOADED;
        }
        else
        {
            GetScene().wiscene.Merge(stream_task.scene.wiscene);
            archive.remap = stream_task.remap;
            if(stream_task.stream_type == _internal_Stream_Task::STREAM_TYPE::FULL)
            {
                archive.fade_data.insert(stream_task.fade_data.begin(),stream_task.fade_data.end());
                archive.entity_name_map.insert(stream_task.entity_name_map.begin(),stream_task.entity_name_map.end());
                archive.entities.insert(stream_task.entities.begin(), stream_task.entities.end());
                archive.prefabs.insert(stream_task.prefabs.begin(), stream_task.prefabs.end());
            }
            archive.load_state = Scene::Archive::LoadState::LOADED;
        }
        stream_task.Clear();
    }
    void Scene::Archive::Init()
    {
        archiveID = wi::ecs::CreateEntity();

        std::string preview_actual_file = Filesystem::GetActualPath(wi::helper::ReplaceExtension(file, "preview"));

        wi::ecs::EntitySerializer seri;
        wi::Archive ar_bounds = wi::Archive(wi::helper::ReplaceExtension(preview_actual_file, "bounds"));
        bounds.Serialize(ar_bounds, seri);

        stream_task.stream_type = _internal_Stream_Task::STREAM_TYPE::INIT;
        stream_task.archiveID = archiveID;
        stream_task.file = file;
        stream_task.actual_file = preview_actual_file;
        stream_task.remap.insert(remap.begin(), remap.end());

        load_state = LoadState::INITIALIZING;
    }
    void Scene::Archive::Load(wi::ecs::Entity prefabID)
    {   
        if( (prefabID != wi::ecs::INVALID_ENTITY) && 
            (load_state == LoadState::LOADED) )
        {
            dependency_count++;
            _internal_Clone_Prefab(*this, prefabID);
        }

        if(stream_task.stream_state == _internal_Stream_Task::STREAM_STATE::FREE)
        {
            if(load_state == LoadState::UNLOADED)
            {
                // Check if there is a stashed remap data
                Prefab* prefab = GetScene().prefabs.GetComponent(prefabID);

                // Add data to stream task
                stream_task.stream_type = (is_root) ? _internal_Stream_Task::STREAM_TYPE::ROOT : _internal_Stream_Task::STREAM_TYPE::FULL;
                stream_task.archiveID = archiveID;
                stream_task.file = file;
                stream_task.actual_file = Filesystem::GetActualPath(file);
                stream_task.remap.insert(remap.begin(), remap.end());

                load_state = LoadState::LOADING;
            }

            if(load_state == LoadState::UNINITIALIZED)
            {
                Init();
            }
            stream_task.stream_state = _internal_Stream_Task::STREAM_STATE::PENDING;
        }
    }
    void Scene::Archive::Unload(wi::ecs::Entity prefabID)
    {
        if(prefabID != wi::ecs::INVALID_ENTITY)
        {
            Prefab* prefab = GetScene().prefabs.GetComponent(prefabID);
            if(prefab != nullptr)
            {
                // Delete subtier data first!
                for(auto sub_prefabID : prefab->prefabs)
                {
                    Prefab* sub_prefab = GetScene().prefabs.GetComponent(sub_prefabID);

                    if(sub_prefab->loaded)
                        sub_prefab->Unload();

                    for(auto& sub_tier_pair : sub_prefab->lod_tiers)
                    {
                        wi::ecs::Entity sub_tier_prefabID = sub_tier_pair.second;
                        Prefab* sub_tier_prefab = GetScene().prefabs.GetComponent(sub_tier_prefabID);

                        sub_tier_prefab->Unload();
                        GetScene().wiscene.Entity_Remove(sub_tier_prefabID, false);;
                    }
                }

                prefab->Unload();
                dependency_count = std::max(dependency_count-1, uint32_t(0));
            }
        }
    }
    void Scene::Archive::CreateLODTier(wi::ecs::Entity prefabID)
    {
        if(prefabID != wi::ecs::INVALID_ENTITY)
        {
            wi::unordered_map<uint64_t, wi::ecs::Entity> remap;

            Prefab* prefab = GetScene().prefabs.GetComponent(prefabID);
            if(prefab != nullptr)
            {
                remap.insert(prefab->remap.begin(), prefab->remap.end());
            }

            wi::ecs::EntitySerializer seri;
            
            wi::unordered_map<size_t, wi::ecs::Entity> cp_lod_tiers;
            wi::unordered_map<size_t, Scene::Prefab::StreamMode> cp_lod_tier_stream_modes;
            for(auto tier_pair : lod_tiers)
            {
                wi::ecs::Entity clone_prefabID = GetScene().Entity_Clone(tier_pair.second, seri, true, &remap);
                cp_lod_tiers.insert({tier_pair.first, clone_prefabID});
                
                Prefab* clone_prefab = GetScene().prefabs.GetComponent(clone_prefabID);
                cp_lod_tier_stream_modes.insert({tier_pair.first, clone_prefab->stream_mode});
                clone_prefab->stream_mode = Prefab::StreamMode::MANUAL;

                GetScene().wiscene.Component_Attach(clone_prefabID, prefabID, true);
            }

            prefab = GetScene().prefabs.GetComponent(prefabID);
            if(prefab != nullptr)
            {
                prefab->lod_tiers.insert(cp_lod_tiers.begin(), cp_lod_tiers.end());
                prefab->lod_tier_stream_modes.insert(cp_lod_tier_stream_modes.begin(), cp_lod_tier_stream_modes.end());
            }
        }
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
    wi::vector<std::string> shallow_filter_list = { // Any that is commented out will not be cloned over!
        "wi::scene::Scene::names",
        "wi::scene::Scene::layers",
        "wi::scene::Scene::transforms",
        "wi::scene::Scene::hierarchy",
        "wi::scene::Scene::materials"
        // "wi::scene::Scene::meshes",
        // "wi::scene::Scene::impostors",
        // "wi::scene::Scene::objects",
        // "wi::scene::Scene::rigidbodies",
        // "wi::scene::Scene::softbodies",
        // "wi::scene::Scene::armatures",
        // "wi::scene::Scene::lights",
        // "wi::scene::Scene::cameras",
        // "wi::scene::Scene::probes",
        // "wi::scene::Scene::forces",
        // "wi::scene::Scene::decals",
        // "wi::scene::Scene::animations",
        // "wi::scene::Scene::animation_datas",
        // "wi::scene::Scene::emitters",
        // "wi::scene::Scene::hairs",
        // "wi::scene::Scene::weathers",
        // "wi::scene::Scene::sounds",
        // "wi::scene::Scene::inverse_kinematics",
        // "wi::scene::Scene::springs",
        // "wi::scene::Scene::colliders",
        // "wi::scene::Scene::scripts",
        // "wi::scene::Scene::expressions",
        // "wi::scene::Scene::humanoids",
        // "wi::scene::Scene::terrains",
        // "Game::Scene::Prefab",
        // "Game::Scene::Script"
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
        seri.allow_remap = deep_copy;
        
        ar_copy.SetReadModeAndResetPos(false);
        if(deep_copy)
        {
            GetScene().wiscene.componentLibrary.Entity_Serialize(entity, ar_copy, seri);
            wi::jobsystem::Wait(seri.ctx);
        }
        else
        {
            seri.allow_remap = true;
            for(auto& componentID : shallow_filter_list)
            {
                auto compmgr = wiscene.componentLibrary.entries[componentID].component_manager.get();
                auto& compver = wiscene.componentLibrary.entries[componentID].version;
                seri.version = compver;
                compmgr->Component_Serialize(entity, ar_copy, seri);
            }
            wi::jobsystem::Wait(seri.ctx);
            seri.allow_remap = false;
        }

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
        if(deep_copy)
        {
            GetScene().wiscene.componentLibrary.Entity_Serialize(clone_entity, ar_copy, seri);
            wi::jobsystem::Wait(seri.ctx);
        }
        else
        {
            seri.allow_remap = true;
            for(auto& componentID : shallow_filter_list)
            {
                auto compmgr = wiscene.componentLibrary.entries[componentID].component_manager.get();
                auto& compver = wiscene.componentLibrary.entries[componentID].version;
                seri.version = compver;
                compmgr->Component_Serialize(clone_entity, ar_copy, seri);
            }
            wi::jobsystem::Wait(seri.ctx);
            seri.allow_remap = false;
        }

        if(clone_seri != nullptr)
        {
            clone_seri->insert({entity,clone_entity});
            clone_seri->insert(seri.remap.begin(), seri.remap.end());
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
        wi::jobsystem::Dispatch(ctx, scripts.GetCount(), 1, [this, &script_enlist_job](wi::jobsystem::JobArgs jobArgs){
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

    struct _internal_PrefabUpdateSystem_enlist_job
    {
        bool prealloc_done = false;
        std::mutex stream_list_mutex;
        
        bool load_occupied = false;
        wi::ecs::Entity loadID = wi::ecs::INVALID_ENTITY;
        std::string load_file;

        wi::unordered_map<wi::ecs::Entity, std::string> unload_queue;
        wi::unordered_map<wi::ecs::Entity, std::string> clone_queue;

        wi::unordered_map<wi::ecs::Entity, std::string> lodtier_queue;

        void Clear()
        {
            load_occupied = false;
            loadID = wi::ecs::INVALID_ENTITY;
            clone_queue.clear();
            unload_queue.clear();
            lodtier_queue.clear();
        }
    };
    _internal_PrefabUpdateSystem_enlist_job prefab_enlist_job;
    bool _internal_CheckArchiveReady(Scene& scene, wi::ecs::Entity prefabID, Scene::Prefab& prefab)
    {
        auto find_archive = scene.scene_db.find(prefab.file);

        bool archive_ready = true;
        if(find_archive == scene.scene_db.end()) // Create archive first
        {
            std::scoped_lock stream_list_sync (prefab_enlist_job.stream_list_mutex);
            
            if(!prefab_enlist_job.load_occupied)
            {
                scene.scene_db[prefab.file] = Scene::Archive();
                Scene::Archive& archive = scene.scene_db[prefab.file];
                archive.file = prefab.file;

                prefab_enlist_job.loadID = prefabID;
                prefab_enlist_job.load_file = prefab.file;
                prefab_enlist_job.load_occupied = true;
            }

            archive_ready = false;
        }
        if(archive_ready)
        {
            if(find_archive->second.load_state == Scene::Archive::LoadState::UNLOADED)
            {
                std::scoped_lock stream_list_sync (prefab_enlist_job.stream_list_mutex);
                
                if(!prefab_enlist_job.load_occupied)
                {
                    prefab_enlist_job.loadID = prefabID;
                    prefab_enlist_job.load_file = prefab.file;
                    prefab_enlist_job.load_occupied = true;
                }

                archive_ready = false;
            }
        }

        return archive_ready;
    }
    bool _internal_CheckPrefabLoadable(Scene::Prefab::StreamMode stream_mode, wi::ecs::Entity prefabID, Scene::Prefab& prefab, Scene& scene, Scene::Archive& archive)
    {
        bool is_loadable = false;
        switch(stream_mode)
        {
            case Game::Scene::Prefab::StreamMode::DIRECT:
            {
                is_loadable = true;
                break;
            }
            case Game::Scene::Prefab::StreamMode::DISTANCE:
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
                
                wi::scene::TransformComponent* prefab_transform = scene.wiscene.transforms.GetComponent(prefabID);
                if(prefab_transform != nullptr)
                    transformator = *prefab_transform;
                zone_check = zone_check.transform(transformator.world);

                is_loadable = zone_check.intersects(
                    wi::primitive::Sphere(
                        XMFLOAT3(scene.stream_loader_bounds.x, scene.stream_loader_bounds.y, scene.stream_loader_bounds.z),
                        scene.stream_loader_bounds.w)
                );
                break;
            }
            case Game::Scene::Prefab::StreamMode::SCREEN_ESTATE:
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
                
                wi::scene::TransformComponent* prefab_transform = scene.wiscene.transforms.GetComponent(prefabID);
                if(prefab_transform != nullptr)
                    transformator = *prefab_transform;
                zone_check = zone_check.transform(transformator.world);

                is_loadable = ((zone_check.getRadius()/wi::math::Distance(zone_check.getCenter(),wi::scene::GetCamera().Eye)) > (scene.stream_loader_screen_estate));
                break;
            }
            case Game::Scene::Prefab::StreamMode::MANUAL:
            {
                is_loadable = prefab.fade_manual_active;
                break;
            }
            default:
                break;
        }

        return is_loadable;
    }
    void Scene::RunPrefabUpdateSystem(float dt, wi::jobsystem::context& ctx)
    {
        if(stream_task.stream_state == _internal_Stream_Task::STREAM_STATE::DONE)
            _internal_Finish_Stream();

        wi::jobsystem::Dispatch(ctx, prefabs.GetCount(), 1, [this, dt](wi::jobsystem::JobArgs jobArgs){
            size_t prefab_idx = jobArgs.jobIndex;
            wi::ecs::Entity prefabID = prefabs.GetEntity(prefab_idx);

            Prefab* prefab = prefabs.GetComponent(prefabID);
            bool archive_ready = _internal_CheckArchiveReady(*this, prefabID, *prefab);

            if(archive_ready)
            {
                Archive& archive = scene_db.find(prefab->file)->second;
                bool is_loadable = _internal_CheckPrefabLoadable(prefab->stream_mode, prefabID, *prefab, *this, archive);

                if(prefab->lod_tiers.size() > 0)
                {
                    bool tier_do_load = false;
                    size_t target_idx = 0;
                    for(size_t idx = 0; idx < prefab->lod_tiers.size(); ++idx)
                    {
                        size_t r_idx = prefab->lod_tiers.size()-idx-1;

                        wi::ecs::Entity tier_prefabID = prefab->lod_tiers[r_idx];
                        Prefab* tier_prefab = prefabs.GetComponent(tier_prefabID);
                        tier_prefab->fade_manual_active = false;

                        bool is_tier_loadable = false;
                        bool tier_archive_ready = _internal_CheckArchiveReady(*this, tier_prefabID, *tier_prefab);
                        if(tier_archive_ready)
                        {
                            Archive& tier_archive = scene_db.find(tier_prefab->file)->second;
                            is_tier_loadable = _internal_CheckPrefabLoadable(prefab->lod_tier_stream_modes[r_idx], tier_prefabID, *tier_prefab, *this, tier_archive);
                        }
                        if(!is_tier_loadable)
                            break;

                        tier_do_load = is_tier_loadable;
                        target_idx = r_idx;
                    }

                    Prefab* tier_prefab = prefabs.GetComponent(prefab->lod_tiers[target_idx]);
                    tier_prefab->fade_manual_active = (target_idx == 0) ? !is_loadable : tier_do_load;
                    is_loadable = (target_idx == 0) ? is_loadable : false;
                }

                if( (prefab->lod_tiers.size() == 0)
                    &&(archive.lod_tiers.size() > 0)
                    &&(prefab_enlist_job.lodtier_queue.size() < 32))
                {
                    std::scoped_lock stream_list_sync (prefab_enlist_job.stream_list_mutex);
                    prefab_enlist_job.lodtier_queue.insert({prefabID, prefab->file});
                }
                
                prefab->fade_factor = wi::math::Clamp(prefab->fade_factor + (((is_loadable) ? +dt : -dt)*stream_transition_speed),0.f,1.f);

                if( (prefab->loaded)
                    &&(prefab->fade_factor_diff != prefab->fade_factor))
                {
                    for(auto& fade_pair : prefab->fade_data)
                    {
                        wi::ecs::Entity fade_entity = fade_pair.first;
                        FadeData& fade_data = fade_pair.second;

                        wi::scene::ObjectComponent* object = wiscene.objects.GetComponent(fade_entity);
                        if(object != nullptr)
                        {
                            object->color.w = fade_data.object*prefab->fade_factor;
                        }

                        wi::scene::MaterialComponent* material = wiscene.materials.GetComponent(fade_entity);
                        if(material != nullptr)
                        {
                            material->baseColor.w = fade_data.material*prefab->fade_factor;
                        }

                        wi::scene::LightComponent* light = wiscene.lights.GetComponent(fade_entity);
                        if(light != nullptr)
                        {
                            light->intensity = fade_data.light*prefab->fade_factor;
                        }
                    }
                    prefab->fade_factor_diff = prefab->fade_factor;
                }

                if( (!prefab->loaded)
                    &&(prefab->fade_factor > 0.f) 
                    &&(prefab_enlist_job.clone_queue.size() < 255) )
                {                    
                    std::scoped_lock stream_list_sync (prefab_enlist_job.stream_list_mutex);
                    prefab_enlist_job.clone_queue.insert({prefabID, prefab->file});
                    prefab->fade_factor = 0.f;
                }
                if( (prefab->loaded)
                    &&(prefab->fade_factor == 0.f)
                    &&(prefab_enlist_job.unload_queue.size() < 255) )
                {                    
                    std::scoped_lock stream_list_sync (prefab_enlist_job.stream_list_mutex);
                    prefab_enlist_job.unload_queue.insert({prefabID, prefab->file});
                }
            }
        });
        wi::jobsystem::Wait(ctx);

        for(auto& queue_pair : prefab_enlist_job.unload_queue)
        {
            Archive& archive = scene_db.find(queue_pair.second)->second;
            archive.Unload(queue_pair.first);
        }

        if(prefab_enlist_job.load_occupied)
        {
            Archive& archive = scene_db.find(prefab_enlist_job.load_file)->second;
            archive.Load(prefab_enlist_job.loadID);
        }

        for(auto& queue_pair : prefab_enlist_job.clone_queue)
        {
            Archive& archive = scene_db.find(queue_pair.second)->second;
            archive.Load(queue_pair.first);
        }

        for(auto& queue_pair : prefab_enlist_job.lodtier_queue)
        {
            Archive& archive = scene_db.find(queue_pair.second)->second;
            archive.CreateLODTier(queue_pair.first);
        }

        prefab_enlist_job.Clear();
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
            if(stream_task.stream_state == _internal_Stream_Task::STREAM_STATE::PENDING)
            {
                stream_task.stream_state == _internal_Stream_Task::STREAM_STATE::INPROGRESS;
                _internal_Run_Stream_Job();
            }
        }
    }
}