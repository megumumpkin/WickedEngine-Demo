#include "Scene.h"
#include "Filesystem.h"
#include <mutex>

namespace Game{
    Scene* GetScene(){
        static Scene scene;
        return &scene;
    }

    wi::jobsystem::context stream_jobs;
    std::mutex header_mutex;
    std::mutex scene_mutex;

    void Scene::Archive::Head_Load()
    {
        wi::jobsystem::Execute(stream_jobs, [&](wi::jobsystem::JobArgs jobArgs){
            std::string actual_file = Filesystem::GetActualPath(file) + "/head";
            wi::helper::FileExists(actual_file);
            auto ar_head = wi::Archive(actual_file);
            
            std::scoped_lock header_sync(header_mutex);
            
            // Read stream radius list
            wi::ecs::EntitySerializer seri;
            size_t radius_size, meshes_size, animations_size, materials_size, sounds_size;
            ar_head >> radius_size;
            for(size_t i = 0; i < radius_size; ++i)
            {
                wi::ecs::Entity key;
                float value;
                // ar_head >> key;
                wi::ecs::SerializeEntity(ar_head, key, seri);
                ar_head >> value;
                radius[key] = value;
            }

            // Read mesh list
            ar_head >> meshes_size;
            for(size_t i = 0; i < meshes_size; ++i)
            {
                wi::ecs::Entity key;
                wi::ecs::Entity value;
                // ar_head >> key;
                wi::ecs::SerializeEntity(ar_head, key, seri);
                // ar_head >> value;
                wi::ecs::SerializeEntity(ar_head, value, seri);
                meshes[key] = value;
            }

            // Read animation list
            ar_head >> animations_size;
            for(size_t i = 0; i < animations_size; ++i)
            {
                std::string key;
                wi::ecs::Entity value;
                ar_head >> key;
                // ar_head >> value;
                wi::ecs::SerializeEntity(ar_head, value, seri);
                animations[key] = value;
            }

            // Read material blobs
            ar_head >> materials_size;
            for(size_t i = 0; i < materials_size; ++i)
            {
                wi::ecs::Entity key;
                // ar_head >> key;
                wi::ecs::SerializeEntity(ar_head, key, seri);
                materials[key] = {};
                ar_head >> materials[key];
            }

            // Read sound blobs
            ar_head >> sounds_size;
            for(size_t i = 0; i < sounds_size; ++i)
            {
                wi::ecs::Entity key;
                // ar_head >> key;
                wi::ecs::SerializeEntity(ar_head, key, seri);
                sounds[key] = {};
                ar_head >> sounds[key];
            }
        });
    }

    void Scene::Archive::Head_Save()
    {
        wi::helper::DirectoryCreate(Filesystem::GetActualPath(file) + "/");

        std::string actual_file = Filesystem::GetActualPath(file) + "/head";
        auto ar_head = wi::Archive(actual_file, false);

        wi::ecs::EntitySerializer seri;
        
        // Write stream radius list
        ar_head << radius.size();
        for(auto& radius_pair : radius)
        {
            // ar_head << radius_pair.first;
            wi::ecs::SerializeEntity(ar_head, radius_pair.first, seri);
            ar_head << radius_pair.second;
        }

        // Write mesh list
        ar_head << meshes.size();
        for(auto& mesh_pair : meshes)
        {
            // ar_head << mesh_pair.first;
            wi::ecs::SerializeEntity(ar_head, mesh_pair.first, seri);
            // ar_head << mesh_pair.second;
            wi::ecs::SerializeEntity(ar_head, mesh_pair.second, seri);
        }

        // Write animation list
        ar_head << animations.size();
        for(auto& animation_pair : animations)
        {
            ar_head << animation_pair.first;
            // ar_head << animation_pair.second;
            wi::ecs::SerializeEntity(ar_head, animation_pair.second, seri);
        }

        // Write material blobs
        ar_head << materials.size();
        for(auto& material_pair : materials)
        {
            // ar_head << material_pair.first;
            wi::ecs::SerializeEntity(ar_head, material_pair.first, seri);
            ar_head << material_pair.second;
        }

        // Write sound blobs
        ar_head << sounds.size();
        for(auto& sound_pair : sounds)
        {
            // ar_head << sound_pair.first;
            wi::ecs::SerializeEntity(ar_head, sound_pair.first, seri);
            ar_head << sound_pair.second;
        }
    }

    void _internal_Scene_LoadModel(wi::scene::Scene& scene, const std::string& fileName, wi::ecs::EntitySerializer& seri, wi::ecs::Entity root = wi::ecs::INVALID_ENTITY)
    {
        wi::Archive archive(fileName, true);
		if (archive.IsOpen())
		{
			// Serialize it from file (archive version 85 as base):
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
            // With this we will ensure that serialized entities are unique and persistent across the scene:
            scene.componentLibrary.Serialize(archive, seri);
            scene.ddgi.Serialize(archive);
			// scene.Serialize(archive);

            if (root != wi::ecs::INVALID_ENTITY)
			{
				// Parent all unparented transforms to new root entity
				for (size_t i = 0; i < scene.transforms.GetCount(); ++i)
				{
					wi::ecs::Entity entity = scene.transforms.GetEntity(i);
					if (!scene.hierarchy.Contains(entity))
					{
						scene.Component_Attach(entity, root, true);
					}
				}
			}
		}
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

    void Scene::Archive::Init(wi::ecs::Entity clone_prefabID)
    {
        wi::jobsystem::Execute(stream_jobs, [&](wi::jobsystem::JobArgs jobArgs){
            // Thread init needs to load all necessary variables before running
            header_mutex.lock();

            // Get the actual file that is needed to be loaded
            auto actual_file = Filesystem::GetActualPath(file) + "/main";

            // Temporary copy of the archive variables for undisturbed processing
            auto temp_archive_name = file;
            auto temp_prefabID = prefabID;
            auto temp_load_state = load_state;
            auto temp_remap = remap;

            // For prefabs, we need to copy the prefab data first before merging
            Component_Prefab temp_prefab, temp_clone_prefab;
            if(temp_prefabID != wi::ecs::INVALID_ENTITY)
                temp_prefab = *GetScene()->prefabs.GetComponent(prefabID);
            if(clone_prefabID != wi::ecs::INVALID_ENTITY)
                temp_clone_prefab = *GetScene()->prefabs.GetComponent(clone_prefabID);

            header_mutex.unlock();
            // Exit initialization and start running independently

            // Load from disk if the archive hasn't been loaded yet
            if(temp_load_state == LoadState::UNLOADED)
            {
                wi::ecs::EntitySerializer seri;
                seri.remap = temp_remap;
                Scene temp_scene;
                _internal_Scene_LoadModel(temp_scene.wiscene, actual_file, seri, temp_prefabID);
                temp_remap = seri.remap;
                temp_load_state = LoadState::LOADED;

                // If it is a prefab, we need to process the prefab
                if(temp_prefabID != wi::ecs::INVALID_ENTITY)
                {
                    temp_scene.wiscene.FindAllEntities(temp_prefab.entities);
                    temp_prefab.remap = temp_remap;

                    if(temp_prefab.copy_mode == Prefab::CopyMode::LIBRARY)
                    {
                        for(auto& entity : temp_prefab.entities)
                        {
                            temp_scene.Entity_Disable(entity);
                        }
                    }

                    temp_prefab.loaded = true;
                }

                std::scoped_lock scene_sync(scene_mutex);
                GetScene()->wiscene.Merge(temp_scene.wiscene);

                Archive& done_archive = GetScene()->scene_db[temp_archive_name];
                done_archive.remap = temp_remap;
                done_archive.load_state = temp_load_state;

                if(temp_prefabID != wi::ecs::INVALID_ENTITY)
                {
                    auto done_prefab = GetScene()->prefabs.GetComponent(temp_prefabID);
                    (*done_prefab) = temp_prefab;
                }
            }

            // Can only go and proceed to clone the prefab if it is already loaded from disk, ready to copy in memory
            if(temp_load_state == LoadState::LOADED)
            {
                std::scoped_lock scene_sync(scene_mutex);
                // If a clone prefab demands a prefab load, then we need to clone the original prefab components to this one too
                if(clone_prefabID != wi::ecs::INVALID_ENTITY)
                {
                    wi::ecs::EntitySerializer seri;

                    // Change cloning mode depending on whether it has target entity or not
                    if(temp_clone_prefab.target_entity != "")
                    {
                        for(auto& entity : temp_prefab.entities)
                        {
                            auto name = GetScene()->wiscene.Entity_FindByName(temp_clone_prefab.target_entity);
                            if(name != wi::ecs::INVALID_ENTITY)
                            {
                                GetScene()->Entity_Clone(entity, &seri);
                                break;
                            }
                        }
                    }
                    else
                    {
                        for(auto& entity : temp_prefab.entities)
                        {
                            GetScene()->Entity_Clone(entity, &seri);
                        }
                    }

                    // Census all newly cloned entity to prefab
                    temp_clone_prefab.remap = seri.remap;
                    for(auto& remap_pair : temp_clone_prefab.remap)
                    {
                        temp_clone_prefab.entities.insert(remap_pair.second);
                    }

                    temp_clone_prefab.loaded = true;

                    auto done_clone_prefab = GetScene()->prefabs.GetComponent(clone_prefabID);
                    (*done_clone_prefab) = temp_clone_prefab;
                }
            }
        });
        
        // After we launched the stream job, our next job is to set the load state to loading to block other prefabs to also execute the loading process
        if(load_state == LoadState::UNLOADED)
            load_state = LoadState::LOADING;
    }
    void Scene::Archive::Unload(wi::ecs::Entity clone_prefabID)
    {
        if(prefabID != wi::ecs::INVALID_ENTITY)
        {
            if(clone_prefabID != wi::ecs::INVALID_ENTITY)
            {
                auto prefab = GetScene()->prefabs.GetComponent(clone_prefabID);
                for(auto& entity : prefab->entities)
                {
                    GetScene()->wiscene.Entity_Remove(entity, false);
                }
                prefab->entities.clear();
            }
            else
            {
                auto prefab = GetScene()->prefabs.GetComponent(prefabID);
                for(auto& entity : prefab->entities)
                {
                    GetScene()->wiscene.Entity_Remove(entity, false);
                }
                prefab->entities.clear();
            }
        }
        else // Clear all since it is THE root scene
        {
            GetScene()->wiscene.Clear();
        }
    }
    void Scene::Archive::Stream_Mesh(wi::ecs::Entity meshID, bool unload, wi::ecs::Entity clone_prefabID)
    {
        if(!unload)
        {
            // Only load when it is not in progress, please
            if(mesh_streaming_inprogress.find(meshID) == mesh_streaming_inprogress.end())
            {
                mesh_streaming_inprogress.insert(meshID); // Put this into load progress list to prevent others from loading at the same time
                wi::jobsystem::Execute(stream_jobs, [&](wi::jobsystem::JobArgs jobArgs){
                    // Thread init needs to load all necessary variables before running
                    header_mutex.lock();

                    auto actual_file = Filesystem::GetActualPath(file) + "/meshes/" + std::to_string(meshID);

                    // Load only needed header data to memory
                    auto temp_archive_name = file;
                    auto temp_prefabID = prefabID;
                    auto temp_remap = remap;

                    // For prefabs, we need to copy the prefab data first before merging
                    Component_Prefab temp_prefab, temp_clone_prefab;
                    if(temp_prefabID != wi::ecs::INVALID_ENTITY)
                        temp_prefab = *GetScene()->prefabs.GetComponent(prefabID);
                    if(clone_prefabID != wi::ecs::INVALID_ENTITY)
                        temp_clone_prefab = *GetScene()->prefabs.GetComponent(clone_prefabID);

                    header_mutex.unlock();
                    // Exit initialization and start running independently

                    if(!GetScene()->wiscene.meshes.Contains(meshID))
                    {
                        wi::ecs::EntitySerializer seri;
                        seri.remap = temp_remap;

                        // Load the mesh archive to memory
                        auto ar_mesh = wi::Archive(actual_file);

                        // Sync and serialize them to scene
                        std::scoped_lock scene_sync(scene_mutex);

                        // Serialize mesh into scene
                        GetScene()->wiscene.meshes.Component_Serialize(meshID, ar_mesh, seri);

                        // Get the material dependency and load them if it is not loaded
                        auto mesh = GetScene()->wiscene.meshes.GetComponent(meshID);
                        wi::unordered_set<wi::ecs::Entity> material_count_up; // For count census
                        for(auto& subset : mesh->subsets)
                        {
                            if(subset.materialID != wi::ecs::INVALID_ENTITY)
                            {
                                if(material_dependency_counts.find(subset.materialID) == material_dependency_counts.end())
                                {
                                    auto find_material = materials.find(subset.materialID);
                                    if(find_material != materials.end())
                                    {
                                        auto ar_material = wi::Archive(find_material->second.data());
                                        GetScene()->wiscene.materials.Component_Serialize(subset.materialID, ar_material, seri);
                                        material_count_up.insert(subset.materialID);
                                    }
                                }
                            }
                        }

                        // Update the count, but not to the prefab since it is one and the same
                        Archive& done_archive = GetScene()->scene_db[temp_archive_name];
                        for(auto& materialID : material_count_up)
                        {
                            done_archive.material_dependency_counts[materialID] += 1;
                        }
                        done_archive.mesh_dependency_counts[meshID] += 1;

                        auto done_clone_prefab = GetScene()->prefabs.GetComponent(clone_prefabID);
                        (*done_clone_prefab) = temp_clone_prefab;
                    }

                    if(clone_prefabID != wi::ecs::INVALID_ENTITY)
                    {
                        std::scoped_lock scene_sync(scene_mutex);

                        Archive& done_archive = GetScene()->scene_db[temp_archive_name];

                        // Check copy mode if it is shallow copy or deep copy, which will determine on how will mesh be also loaded
                        wi::ecs::EntitySerializer seri;
                        seri.allow_remap = (temp_clone_prefab.copy_mode == Component_Prefab::CopyMode::SHALLOW_COPY) ? false : true; // SHALLOW_COPY won't copy the materials, DEEP_COPY will copy the materials
                        seri.remap = temp_clone_prefab.remap;

                        auto ar_clone_mesh = wi::Archive();
                        GetScene()->wiscene.meshes.Component_Serialize(meshID, ar_clone_mesh, seri);

                        wi::ecs::Entity clone_entity = _internal_ecs_clone_entity(meshID, seri);
                        
                        ar_clone_mesh.SetReadModeAndResetPos(true);
                        GetScene()->wiscene.meshes.Component_Serialize(clone_entity, ar_clone_mesh, seri);

                        wi::unordered_set<wi::ecs::Entity> material_count_up; // For count census
                        if(seri.allow_remap) // If DEEP_COPY is desired, we need to clone the material
                        {
                            auto mesh = GetScene()->wiscene.meshes.GetComponent(meshID);
                            for(auto& subset : mesh->subsets)
                            {
                                wi::ecs::Entity material_clone_entity = _internal_ecs_clone_entity(subset.materialID, seri);
                                auto ar_clone_material = wi::Archive(done_archive.materials[subset.materialID].data());
                                GetScene()->wiscene.materials.Component_Serialize(material_clone_entity, ar_clone_material, seri);
                                material_count_up.insert(subset.materialID);
                            }
                        }

                        // Update the count
                        for(auto& materialID : material_count_up)
                        {
                            done_archive.material_dependency_counts[materialID] += 1;
                            temp_clone_prefab.material_dependency_counts[materialID] += 1;
                        }
                        done_archive.mesh_dependency_counts[meshID] += 1;
                        temp_clone_prefab.mesh_dependency_counts[meshID] += 1;

                        auto done_clone_prefab = GetScene()->prefabs.GetComponent(clone_prefabID);
                        (*done_clone_prefab) = temp_clone_prefab;
                    }

                    std::scoped_lock scene_sync(scene_mutex);
                    Archive& done_archive = GetScene()->scene_db[temp_archive_name];
                    done_archive.mesh_streaming_inprogress.erase(meshID);
                });
            }
        }
        else
        {
            if(clone_prefabID != wi::ecs::INVALID_ENTITY)
            {
                auto clone_prefab = GetScene()->prefabs.GetComponent(clone_prefabID);
                if(clone_prefab->mesh_dependency_counts.find(meshID) != clone_prefab->mesh_dependency_counts.end())
                {
                    // Decrease count if it has count bigger than 1 and remove the component if it is the last one left
                    if(clone_prefab->mesh_dependency_counts[meshID] > 1)
                    {
                        clone_prefab->mesh_dependency_counts[meshID] -= 1;
                    }
                    else // Check whether to remove the material data too or not, and then finally remove the mesh
                    {
                        auto mesh = GetScene()->wiscene.meshes.GetComponent(clone_prefab->remap[meshID]);
                        for(auto& subset : mesh->subsets)
                        {
                            if(clone_prefab->material_dependency_counts.find(subset.materialID) != clone_prefab->material_dependency_counts.end())
                            {
                                if(clone_prefab->material_dependency_counts[subset.materialID] > 1)
                                {
                                    clone_prefab->material_dependency_counts[subset.materialID] -= 1;
                                }
                                else
                                {
                                    // Remove the material from the clone object
                                    GetScene()->wiscene.meshes.Remove(clone_prefab->remap[subset.materialID]);
                                    clone_prefab->material_dependency_counts.erase(subset.materialID);
                                }
                            }
                        }

                        GetScene()->wiscene.meshes.Remove(clone_prefab->remap[meshID]);
                        clone_prefab->mesh_dependency_counts.erase(meshID);
                    }
                }
            }

            if(mesh_dependency_counts.find(meshID) != mesh_dependency_counts.end())
            {
                if(mesh_dependency_counts[meshID] > 1)
                {
                    mesh_dependency_counts[meshID] -= 1;
                }
                else // Check whether to remove the material data too or not, and then finally remove the mesh
                {
                    auto mesh = GetScene()->wiscene.meshes.GetComponent(meshID);
                    for(auto& subset : mesh->subsets)
                    {
                        if(material_dependency_counts.find(subset.materialID) != material_dependency_counts.end())
                        {
                            if(material_dependency_counts[subset.materialID] > 1)
                            {
                                material_dependency_counts[subset.materialID] -= 1;
                            }
                            else
                            {
                                // Remove the material from the clone object
                                GetScene()->wiscene.meshes.Remove(subset.materialID);
                                material_dependency_counts.erase(subset.materialID);
                            }
                        }
                    }

                    GetScene()->wiscene.meshes.Remove(meshID);
                    mesh_dependency_counts.erase(meshID);
                }
            }
        }
    }
    void Scene::Archive::Stream_Animation(std::string animationID, bool unload, wi::ecs::Entity clone_prefabID)
    {
        if(!unload)
        {
            
        }
        else
        {

        }
    }
    void Scene::Archive::Stream_Sounds(wi::ecs::Entity soundID, bool unload, wi::ecs::Entity clone_prefabID)
    {
        if(!unload)
        {
            
        }
        else
        {

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

    void Scene::Update(float dt)
    {
        
    }
}