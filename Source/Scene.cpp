#include "Scene.h"
#include "Filesystem.h"
#include <chrono>

namespace Game{
    Scene* GetScene(){
        static Scene scene;
        return &scene;
    }

    void Scene::Component_Prefab::Serialize(wi::Archive &archive, wi::ecs::EntitySerializer &seri)
    {
        if(archive.IsReadMode())
        {
            archive >> file;
            archive >> (uint32_t&)copy_mode;
            archive >> (uint32_t&)stream_mode;
        }
        else
        {
            archive << file;
            archive << uint32_t(copy_mode);
            archive << uint32_t(stream_mode);
        }
    }

    Scene::StreamJob* GetStreamJobData() // Pointer to stream job
    {
        static Scene::StreamJob stream_data;
        return &stream_data;
    }

    struct _internal_Scene_Serialize_DEPRECATED_PreviousFrameTransformComponent
	{
		void Serialize(wi::Archive& archive, wi::ecs::EntitySerializer& seri) { /*this never serialized any data*/ }
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
		else
		{
			// Old serialization path with hard coded component types:
			scene.names.Serialize(archive, seri);
			scene.layers.Serialize(archive, seri);
			scene.transforms.Serialize(archive, seri);
			if (archive.GetVersion() < 75)
			{
				wi::ecs::ComponentManager<_internal_Scene_Serialize_DEPRECATED_PreviousFrameTransformComponent> prev_transforms;
				prev_transforms.Serialize(archive, seri);
			}
			scene.hierarchy.Serialize(archive, seri);
			scene.materials.Serialize(archive, seri);
			scene.meshes.Serialize(archive, seri);
			scene.impostors.Serialize(archive, seri);
			scene.objects.Serialize(archive, seri);
			wi::ecs::ComponentManager<wi::primitive::AABB> aabbs_tmp; // no longer needed from serializer
			aabbs_tmp.Serialize(archive, seri);
			scene.rigidbodies.Serialize(archive, seri);
			scene.softbodies.Serialize(archive, seri);
			scene.armatures.Serialize(archive, seri);
			scene.lights.Serialize(archive, seri);
			aabbs_tmp.Serialize(archive, seri);
			scene.cameras.Serialize(archive, seri);
			scene.probes.Serialize(archive, seri);
			aabbs_tmp.Serialize(archive, seri);
			scene.forces.Serialize(archive, seri);
			scene.decals.Serialize(archive, seri);
			aabbs_tmp.Serialize(archive, seri);
			scene.animations.Serialize(archive, seri);
			scene.emitters.Serialize(archive, seri);
			scene.hairs.Serialize(archive, seri);
			scene.weathers.Serialize(archive, seri);
			if (archive.GetVersion() >= 30)
			{
				scene.sounds.Serialize(archive, seri);
			}
			if (archive.GetVersion() >= 37)
			{
				scene.inverse_kinematics.Serialize(archive, seri);
			}
			if (archive.GetVersion() >= 38)
			{
				scene.springs.Serialize(archive, seri);
			}
			if (archive.GetVersion() >= 46)
			{
				scene.animation_datas.Serialize(archive, seri);
			}

			if (archive.GetVersion() < 46)
			{
				// Fixing the animation import from archive that didn't have separate animation data components:
				for (size_t i = 0; i < scene.animations.GetCount(); ++i)
				{
					wi::scene::AnimationComponent& animation = scene.animations[i];
					for (const wi::scene::AnimationComponent::AnimationChannel& channel : animation.channels)
					{
						assert(channel.samplerIndex < (int)animation.samplers.size());
						wi::scene::AnimationComponent::AnimationSampler& sampler = animation.samplers[channel.samplerIndex];
						if (sampler.data == wi::ecs::INVALID_ENTITY)
						{
							// backwards-compatibility mode
							sampler.data = wi::ecs::CreateEntity();
							scene.animation_datas.Create(sampler.data) = sampler.backwards_compatibility_data;
							sampler.backwards_compatibility_data.keyframe_times.clear();
							sampler.backwards_compatibility_data.keyframe_data.clear();
						}
					}
				}
			}
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

		wi::backlog::post("Scene serialize took " + std::to_string(timer.elapsed_seconds()) + " sec");
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

    wi::jobsystem::context stream_job;
    std::mutex stream_mutex;

    void _internal_Clone_Prefab(Scene::Archive& archive, wi::ecs::Entity clone_prefabID)
    {
        Scene::Prefab* find_clone_prefab = GetScene()->prefabs.GetComponent(clone_prefabID);
        if(find_clone_prefab != nullptr)
        {
            wi::ecs::EntitySerializer seri;
            seri.remap = find_clone_prefab->remap;
            wi::unordered_map<wi::ecs::Entity, wi::ecs::Entity> clone_remap;
            for (auto& map_pair : archive.remap)
            {
                // Clone entity first
                auto& origin_entity = map_pair.second;
                auto clone_entity = GetScene()->Entity_Clone(origin_entity, seri, (find_clone_prefab->copy_mode == Scene::Prefab::CopyMode::SHALLOW_COPY) ? false : true);
                clone_remap[origin_entity] = clone_entity;
            }
            wi::jobsystem::Wait(seri.ctx);
            find_clone_prefab->remap = seri.remap;

            // Remap parenting for shallow copy
            if(find_clone_prefab->copy_mode == Scene::Prefab::CopyMode::SHALLOW_COPY){
                for(auto& map_pair : clone_remap)
                {
                    wi::backlog::post(std::to_string(map_pair.second));

                    // If it has no parent then we attach them to prefab
                    wi::scene::HierarchyComponent* original_hierarchy = GetScene()->wiscene.hierarchy.GetComponent(map_pair.first);
                    wi::scene::HierarchyComponent* hierarchy = GetScene()->wiscene.hierarchy.GetComponent(map_pair.second);
                    if((original_hierarchy != nullptr) && (hierarchy != nullptr))
                    {
                        if(original_hierarchy->parentID == archive.prefabID)
                            GetScene()->wiscene.Component_Attach(map_pair.second, clone_prefabID, true);
                        else
                            GetScene()->wiscene.Component_Attach(map_pair.second, clone_remap[original_hierarchy->parentID], true);
                    }
                    else
                        GetScene()->wiscene.Component_Attach(map_pair.second, clone_prefabID, true);
                }
            }

            find_clone_prefab->loaded = true;
        }
    }

    std::vector<Scene::StreamData*> stream_callbacks;
    void _internal_Finish_Stream(Scene::StreamData* stream_callback)
    {
        Scene::Archive& archive = GetScene()->scene_db[stream_callback->file];
        archive.remap = stream_callback->remap;

        GetScene()->wiscene.Merge(stream_callback->block->wiscene);
        delete(stream_callback->block);

        // Work on the prefab too if it exists
        Scene::Prefab* find_prefab = GetScene()->prefabs.GetComponent(archive.prefabID);
        if(find_prefab != nullptr)
        {
            // Need to parent components to the scene
            for(auto& map_pair : archive.remap)
            {
                // If it has no parent then we attach them to prefab
                bool set_parent = false;
                wi::scene::HierarchyComponent* hierarchy = GetScene()->wiscene.hierarchy.GetComponent(map_pair.second);
                if(hierarchy != nullptr)
                {
                    if(hierarchy->parentID == wi::ecs::INVALID_ENTITY)
                        set_parent = true;
                }
                else
                    set_parent = true;
                if(set_parent)
                    GetScene()->wiscene.Component_Attach(map_pair.second, archive.prefabID, true);

                // If prefab is a library then we need to disable the entity
                if(find_prefab->copy_mode == Scene::Prefab::CopyMode::LIBRARY)
                {
                    auto& target_entity = map_pair.second;
                    GetScene()->Entity_Disable(target_entity);
                }
            }
            
            find_prefab->loaded = true;
        }

        if(stream_callback->clone_prefabID != wi::ecs::INVALID_ENTITY)
        {
            _internal_Clone_Prefab(archive, stream_callback->clone_prefabID);
        }

        delete(stream_callback);
        archive.load_state = Scene::Archive::LoadState::LOADED;
    }
    void Scene::Archive::Load(wi::ecs::Entity clone_prefabID)
    {
        if(load_state == LoadState::UNLOADED)
        {
            load_state = LoadState::LOADING;

            // Add data to stream job
            StreamData* stream_data_init = new StreamData();
            stream_data_init->file = file;
            stream_data_init->actual_file = Filesystem::GetActualPath(file);
            stream_data_init->remap = remap;
            stream_data_init->clone_prefabID = clone_prefabID;
            GetStreamJobData()->stream_queue.push_back(stream_data_init);

            auto stream_job_data = GetStreamJobData();

            //re-run stream job if the job does not exist
            if(!wi::jobsystem::IsBusy(stream_job))
            {
                wi::jobsystem::Execute(stream_job, [stream_job_data](wi::jobsystem::JobArgs jobargs){
                    size_t it = 0;
                    size_t it_end = 0;
                    it_end = stream_job_data->stream_queue.size();
                    while(it < it_end)
                    {
                        StreamData* stream_data_ptr = stream_job_data->stream_queue[it];
                        wi::ecs::EntitySerializer seri;
                        seri.remap = stream_data_ptr->remap;

                        stream_data_ptr->block = new Scene();

                        auto ar_stream = wi::Archive(stream_data_ptr->actual_file);
                        _internal_Scene_Serialize(stream_data_ptr->block->wiscene, ar_stream, seri);
                        wi::jobsystem::Wait(seri.ctx);

                        stream_data_ptr->remap = seri.remap;

                        std::scoped_lock scene_sync(stream_mutex);

                        stream_callbacks.push_back(stream_data_ptr);

                        it++;
                        it_end = stream_job_data->stream_queue.size();
                    }
                    stream_job_data->stream_queue.clear();
                });
            }
        }

        if((clone_prefabID != wi::ecs::INVALID_ENTITY) && (load_state == LoadState::LOADED))
        {
            _internal_Clone_Prefab(*this, clone_prefabID);
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
        "Game::Scene::Prefab"
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
        "Game::Scene::Prefab"
    };
    void Scene::Entity_Disable(wi::ecs::Entity entity)
    {
        auto& inactive = inactives.Create(entity);
        wi::ecs::EntitySerializer seri;
        seri.allow_remap = false;
        auto ar_disable = wi::Archive();
        for(auto& componentID : disable_filter_list)
        {
            auto compmgr = wiscene.componentLibrary.entries[componentID].component_manager.get();
            auto& compver = wiscene.componentLibrary.entries[componentID].version;
            seri.version = compver;
            compmgr->Component_Serialize(entity, ar_disable, seri);
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
            for(auto& componentID : disable_filter_list)
            {
                auto compmgr = wiscene.componentLibrary.entries[componentID].component_manager.get();
                auto& compver = wiscene.componentLibrary.entries[componentID].version;
                seri.version = compver;
                compmgr->Component_Serialize(entity, ar_enable, seri);
            }
        }
    }
    wi::ecs::Entity Scene::Entity_Clone(wi::ecs::Entity entity, wi::ecs::EntitySerializer& seri, bool deep_copy)
    {
        // Clone filter'll cut out inactive and prefab from being cloned, as it poses some risk
        wi::Archive ar_copy;

        bool temp_remap = seri.allow_remap;
        if(!deep_copy)
            seri.allow_remap = false;

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

        wi::ecs::Entity clone_entity = wi::ecs::CreateEntity();

        ar_copy.SetReadModeAndResetPos(true);
        for(auto& componentID : *clone_filter_list)
        {
            auto compmgr = wiscene.componentLibrary.entries[componentID].component_manager.get();
            auto& compver = wiscene.componentLibrary.entries[componentID].version;
            seri.version = compver;
            compmgr->Component_Serialize(clone_entity, ar_copy, seri);
        }
        wi::jobsystem::Wait(seri.ctx);

        // Only copy inactives if the cloning's deep_copy value to true
        auto inactive = inactives.GetComponent(entity);
        if(inactive != nullptr)
        {
            if(deep_copy)
            {
                auto ar_enable = wi::Archive(inactive->inactive_storage.data());
                for(auto& componentID : disable_filter_list)
                {
                    auto compmgr = wiscene.componentLibrary.entries[componentID].component_manager.get();
                    auto& compver = wiscene.componentLibrary.entries[componentID].version;
                    seri.version = compver;
                    compmgr->Component_Serialize(clone_entity, ar_enable, seri);
                }
            }
        }
        seri.allow_remap = temp_remap;
        return clone_entity;
    }

    void Scene::Load(std::string file)
    {
        // Create an archive to work with
        scene_db[file] = {};

        // Set the archive data
        current_scene = file;
        scene_db[file].file = file;
        scene_db[file].Load();
    }

    void Scene::Update(float dt)
    {
        // Finish load callback
        std::scoped_lock stream_lock(stream_mutex);
        for(auto stream_callback : stream_callbacks)
        {
            _internal_Finish_Stream(stream_callback);
        }
        stream_callbacks.clear();

        // Stream prefabs
        wi::unordered_map<wi::ecs::Entity,std::string> deferred_lib_stream;
        for(int i = 0; i < prefabs.GetCount(); ++i)
        {
            auto prefabID = prefabs.GetEntity(i);
            Prefab& prefab = prefabs[i];

            if(!prefab.loaded)
            {
                auto find_archive = scene_db.find(prefab.file);
                if(find_archive == scene_db.end()) // Create archive
                {
                    scene_db[prefab.file] = {};
                    Archive& archive = scene_db[prefab.file];
                    archive.file = prefab.file;
                    if(prefab.copy_mode == Prefab::CopyMode::DEEP_COPY)
                    {
                        deferred_lib_stream[prefabID] = prefab.file;
                    }
                    else
                    {
                        archive.prefabID = prefabID;
                        archive.Load();
                    }

                    find_archive = scene_db.find(prefab.file);
                }

                Archive& archive = find_archive->second;
                if((archive.prefabID != prefabID) && (archive.load_state == Archive::LoadState::LOADED)) // Clone, but only after streaming
                {
                    archive.Load(prefabID);
                }
            }
        }

        // Deferred library loading
        for(auto& deferred_pair : deferred_lib_stream)
        {
            wi::ecs::EntitySerializer seri;
            seri.allow_remap = false;
             
            Archive& archive = scene_db[deferred_pair.second];
            auto lib_prefabID = Entity_Clone(deferred_pair.first,seri);
            
            Prefab* prefab = prefabs.GetComponent(lib_prefabID);
            prefab->copy_mode = Prefab::CopyMode::LIBRARY;
            
            archive.prefabID = lib_prefabID;
            archive.Load(deferred_pair.first);
        }
    }
}