#include "Scene.h"
#include "Filesystem.h"
#include <chrono>

namespace Game{
    Scene* GetScene(){
        static Scene scene;
        return &scene;
    }

    Scene::StreamJob* stream_job_data; // Pointer to stream job

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

    void Scene::Archive::Load(wi::ecs::Entity clone_prefabID)
    {
        if(load_state == LoadState::UNLOADED)
        {
            load_state = LoadState::LOADING;

            // Make sure we renew the pointer if the stream job is done
            if(!wi::jobsystem::IsBusy(stream_job))
                stream_job_data = new StreamJob();

            // Add data to stream job
            StreamData* stream_data_init = new StreamData();
            stream_data_init->file = file;
            stream_data_init->remap = remap;
            stream_job_data->stream_queue.push_back(stream_data_init);

            // Add pending cloning to archive
            temp_clone_prefabID = clone_prefabID;

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

                        std::string actual_file = Filesystem::GetActualPath(stream_data_ptr->file);
                        auto ar_scene = wi::Archive(actual_file);

                        wi::ecs::EntitySerializer seri;
                        seri.remap = stream_data_ptr->remap;

                        stream_data_ptr->block = new Scene();
                        _internal_Scene_Serialize(stream_data_ptr->block->wiscene, ar_scene, seri);
                        stream_data_ptr->remap = seri.remap;

                        std::scoped_lock scene_sync(stream_mutex);

                        GetScene()->scene_db[stream_data_ptr->file].stream_done = true;
                        GetScene()->scene_db[stream_data_ptr->file].remap = stream_data_ptr->remap;
                        GetScene()->scene_db[stream_data_ptr->file].stream_block = stream_data_ptr->block;

                        delete(stream_data_ptr);

                        it++;
                        it_end = stream_job_data->stream_queue.size();
                    }
                    delete(stream_job_data);
                });
            }
        }

        if(stream_done && (load_state == LoadState::LOADING))
        {
            // Start merging
            GetScene()->wiscene.Merge(stream_block->wiscene);
            delete(stream_block);

            // Work on the prefab too if it exists
            Prefab* find_prefab = GetScene()->prefabs.GetComponent(prefabID);
            if(find_prefab != nullptr)
            {
                if(find_prefab->copy_mode == Prefab::CopyMode::LIBRARY)
                {
                    for(auto& map_pair : remap)
                    {
                        auto& target_entity = map_pair.second;
                        GetScene()->Entity_Disable(target_entity);
                    }
                }
                
                find_prefab->loaded = true;
            }

            stream_done = false;
            temp_clone_prefabID = wi::ecs::INVALID_ENTITY;
            load_state = LoadState::LOADED;
        }

        if((clone_prefabID == wi::ecs::INVALID_ENTITY) && (load_state == LoadState::LOADED))
        {
            Prefab* find_clone_prefab = GetScene()->prefabs.GetComponent(clone_prefabID);
            if(find_clone_prefab != nullptr)
            {
                wi::ecs::EntitySerializer seri;
                seri.remap = find_clone_prefab->remap;
                for (auto& map_pair : remap)
                {
                    auto& origin_entity = map_pair.second;
                    GetScene()->Entity_Clone(origin_entity, &seri, (find_clone_prefab->copy_mode == Prefab::CopyMode::SHALLOW_COPY) ? false : true);
                }
            }
        }
    }

    const wi::vector<std::string> disable_filter_list = { // Any element that is not commented out will be disabled by the component
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
    const wi::vector<std::string> clone_filter_list = { // Any that is commented out will not be cloned over!
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
    void Scene::Entity_Disable(wi::ecs::Entity entity)
    {
        auto& inactive = inactives.Create(entity);
        wi::ecs::EntitySerializer seri;
        seri.allow_remap = false;
        auto ar_disable = wi::Archive();
        for(auto& componentID : disable_filter_list)
        {
            wiscene.componentLibrary.entries[componentID].component_manager->Component_Serialize(entity, ar_disable, seri);
            wiscene.componentLibrary.entries[componentID].component_manager->Remove(entity);
        }
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
                wiscene.componentLibrary.entries[componentID].component_manager->Component_Serialize(entity, ar_enable, seri);
            }
        }
    }
    void Scene::Entity_Clone(wi::ecs::Entity entity, wi::ecs::EntitySerializer *seri, bool deep_copy)
    {
        // Clone filter'll cut out inactive and prefab from being cloned, as it poses some risk
        wi::Archive ar_copy;
        for(auto& componentID : clone_filter_list)
        {
            wiscene.componentLibrary.entries[componentID].component_manager->Component_Serialize(entity, ar_copy, *seri);
        }

        wi::ecs::Entity clone_entity = wi::ecs::INVALID_ENTITY;
        if(seri->remap.find(entity) != seri->remap.end())
        {
            clone_entity = seri->remap[entity];
        }
        else
        {
            clone_entity = wi::ecs::CreateEntity();
            seri->remap[entity] = clone_entity;
        }

        for(auto& componentID : clone_filter_list)
        {
            wiscene.componentLibrary.entries[componentID].component_manager->Component_Serialize(clone_entity, ar_copy, *seri);
        }

        // Only copy inactives if the cloning's deep_copy value to true
        if(deep_copy)
        {
            auto inactive = inactives.GetComponent(entity);
            if(inactive != nullptr)
            {
                auto ar_enable = wi::Archive(inactive->inactive_storage.data());
                for(auto& componentID : disable_filter_list)
                {
                    wiscene.componentLibrary.entries[componentID].component_manager->Component_Serialize(clone_entity, ar_enable, *seri);
                }
            }
        }
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

    void Scene::Save(std::string file)
    {
        // Get actual file path here
        auto actual_file = Filesystem::GetActualPath(file);
    }

    void Scene::Update(float dt)
    {
        // Update all loading states of archive
        std::scoped_lock stream_sync(stream_mutex);
        for(auto& archive_pair : scene_db) // Merge scene 1 per frame
        {
            Archive& archive = archive_pair.second;
            if(archive.load_state == Archive::LoadState::LOADING)
            {
                archive.Load(archive.temp_clone_prefabID);
                break;
            }
        }

        // Check prefabs for loading
        wi::unordered_map<wi::ecs::Entity, Prefab> new_prefabs;
        for(int i = 0; i < GetScene()->prefabs.GetCount(); ++i)
        {
            wi::ecs::Entity prefabID = GetScene()->prefabs.GetEntity(i);
            Prefab& prefab = GetScene()->prefabs[i];

            auto find_archive = GetScene()->scene_db.find(prefab.file);
            if(find_archive != GetScene()->scene_db.end()) // Clone
            {
                Archive& archive = find_archive->second;
                archive.Load(prefabID);
            }
            else // Load from disk
            {
                GetScene()->scene_db[prefab.file] = {}; // Init
                Archive& archive_new = GetScene()->scene_db[prefab.file];
                archive_new.file = prefab.file;
                if(prefab.copy_mode == Prefab::CopyMode::DEEP_COPY) // Do clone first before deep copy
                {
                    wi::ecs::Entity new_prefabID = wi::ecs::CreateEntity();
                    // Push back new prefab data
                    new_prefabs[new_prefabID] = {};
                    new_prefabs[new_prefabID].copy_mode = Prefab::CopyMode::LIBRARY;
                    archive_new.prefabID = new_prefabID;
                    archive_new.Load();
                }
                else
                {
                    archive_new.prefabID = prefabID;
                    archive_new.Load(prefabID);
                }
            }
        }

        for(auto& new_pair : new_prefabs)
        {
            Prefab& new_prefab = GetScene()->prefabs.Create(new_pair.first);
            new_prefab = new_pair.second;
        }
    }
}