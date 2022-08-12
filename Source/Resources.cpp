#include "Resources.h"
#include <mutex>
#include <set>
#include <wiBacklog.h>
#include <wiECS.h>
#include <wiHelper.h>
#include <wiJobSystem.h>

using namespace Game::Resources;

Library::Data* Library::GetLibraryData(){
    static Library::Data data;
    return &data;
}



uint32_t _resources_internal_generateUUID(){
    uint32_t result; 
    static std::atomic<uint32_t> nextuuid{ 1 };
    return nextuuid.fetch_add(1);
}

void _resources_internal_listLoadedEntities(wi::scene::Scene& scene_staging, Library::Collection& collection, Library::Instance& instance){
    for(int i=0; i<scene_staging.names.GetCount(); ++i){
        auto entity = scene_staging.names.GetEntity(i);
        bool entity_exist = (instance.entities.find(entity) != instance.entities.end());
        if(!entity_exist){
            collection.entities.push_back(entity);
            instance.entities[entity] = entity;
        }
    }
}

wi::ecs::Entity _resources_internal_cloneEntity(wi::ecs::Entity entity, Library::Instance& instance, bool allow_remap){
    auto clone_entity = wi::ecs::INVALID_ENTITY;

    auto found_disabled = Library::GetLibraryData()->disabled_entities.find(entity);
    if(found_disabled != Library::GetLibraryData()->disabled_entities.end()){
        clone_entity = Library::Entity_Enable(entity, allow_remap);
    }else{
        wi::Archive archive;
        wi::ecs::EntitySerializer seri;
        seri.remap = instance.entities;
        seri.allow_remap = allow_remap;
        auto serializeflags = wi::scene::Scene::EntitySerializeFlags::NONE;

        archive.SetReadModeAndResetPos(false);
        Library::GetLibraryData()->scene->Entity_Serialize(archive, seri, entity, serializeflags);

        archive.SetReadModeAndResetPos(true);
        clone_entity = Library::GetLibraryData()->scene->Entity_Serialize(archive, seri, wi::ecs::INVALID_ENTITY, serializeflags);
    }
    instance.entities[entity] = clone_entity;
    return clone_entity;
}

void _resources_internal_cloneObjectRef(wi::scene::ObjectComponent* objectComponent, Library::Instance& instance, wi::vector<wi::ecs::Entity>& ref_mesh){
    auto old_meshID = objectComponent->meshID;
    auto found_meshID = instance.entities.find(old_meshID);
    if(found_meshID == instance.entities.end() && old_meshID != wi::ecs::INVALID_ENTITY){
        objectComponent->meshID = _resources_internal_cloneEntity(old_meshID, instance, true);
        ref_mesh.push_back(objectComponent->meshID);
    }
}

void _resources_internal_cloneMeshRef(wi::scene::MeshComponent* meshComponent, Library::Instance& instance, wi::vector<wi::ecs::Entity>& ref_armature){
    for(auto subset : meshComponent->subsets){
        auto old_subset = subset.materialID;
        auto found_subset = instance.entities.find(old_subset);
        if(found_subset == instance.entities.end() && old_subset != wi::ecs::INVALID_ENTITY){
            subset.materialID = _resources_internal_cloneEntity(old_subset, instance, true);
        }
    }

    auto old_armatureID = meshComponent->armatureID;
    auto found_armatureID = instance.entities.find(old_armatureID);
    if(found_armatureID == instance.entities.end() && old_armatureID != wi::ecs::INVALID_ENTITY){
        meshComponent->armatureID = _resources_internal_cloneEntity(old_armatureID, instance, true);
        ref_armature.push_back(meshComponent->armatureID);
    }
}

void _resources_internal_cloneArmatureRef(wi::scene::ArmatureComponent* armatureComponent, Library::Instance& instance){
    for(auto bone_entity : armatureComponent->boneCollection){
        auto old_bone_entity = bone_entity;
        auto found_bone_entity = instance.entities.find(old_bone_entity);
        if(found_bone_entity == instance.entities.end())
            bone_entity = _resources_internal_cloneEntity(old_bone_entity, instance, true);
    }
}

void _resources_internal_cloneAnimationRef(wi::scene::AnimationComponent* animationComponent, Library::Instance& instance){
    for(auto channel : animationComponent->channels){
        auto old_target = channel.target;
        auto found_target = instance.entities.find(old_target);
        if(found_target == instance.entities.end())
            channel.target = _resources_internal_cloneEntity(old_target, instance, true);
    }
}

void _resources_internal_loadChildren(wi::ecs::Entity entity, wi::vector<wi::ecs::Entity>& loadList){
    for (size_t i = 0; i < Library::GetLibraryData()->scene->hierarchy.GetCount(); ++i)
    {
        auto hier = Library::GetLibraryData()->scene->hierarchy[i];
        if (hier.parentID == entity)
        {
            wi::ecs::Entity child = Library::GetLibraryData()->scene->hierarchy.GetEntity(i);
            loadList.push_back(child);
            _resources_internal_loadChildren(child, loadList);
        }
    }
}



void Library::Init(){
    GetLibraryData()->scene = &wi::scene::GetScene();
}

uint32_t Library::Load(std::string file, std::string subresource, uint32_t loadingstrategy, uint32_t loadingflags){
    std::scoped_lock lock (GetLibraryData()->load_protect);

    bool ld_full = (loadingstrategy & LOADING_STRATEGY_FULL);
    bool ld_always_instance = (loadingstrategy & LOADING_STRATEGY_ALWAYS_INSTANCE);
    bool opt_recursive = (loadingflags & LOADING_FLAG_RECURSIVE);

    uint32_t instance_uuid = 0;

    if(Exist(file, subresource)){ // Clone data from scene instead
        Collection* _collection = nullptr; // Need to track all the instances inside the collection

        // Prepare entites that are needed to be loaded
        wi::vector<wi::ecs::Entity> loadList;
        for(auto& collection : GetLibraryData()->collections){
            if(collection.name == file){
                _collection = &collection;
                if(!subresource.empty()){
                    auto entity = GetLibraryData()->scene->Entity_FindByName(subresource);
                    if(entity != wi::ecs::INVALID_ENTITY){
                        loadList.push_back(entity);
                        if(opt_recursive){
                            _resources_internal_loadChildren(entity, loadList);
                        }
                    }
                }else{
                    for(auto entity : collection.entities){
                        loadList.push_back(entity);
                    }
                }
                break;
            }
        }

        // Prepare instance database
        GetLibraryData()->instances.emplace_back();
        auto& instance = GetLibraryData()->instances.back();
        instance.instance_uuid = _resources_internal_generateUUID();
        instance.collection_name = _collection->name;
        if(_collection->keep_alive) instance.keep_alive = true;

        instance_uuid = instance.instance_uuid;

        // Load using different strategies
        // First is to copy the entity first
        wi::vector<wi::ecs::Entity> ref_mesh;
        wi::vector<wi::ecs::Entity> ref_armature;
        for(auto entity : loadList){ 
            auto new_entity = _resources_internal_cloneEntity(entity, instance, ld_full);
        }
        
        // After everything is copied, check remap the entity
        if(ld_full){
            for(auto new_entity_kval : instance.entities){
                auto new_entity = new_entity_kval.second;

                auto objectComponent = GetLibraryData()->scene->objects.GetComponent(new_entity);
                if(objectComponent != nullptr) _resources_internal_cloneObjectRef(objectComponent, instance, ref_mesh);

                auto meshComponent = GetLibraryData()->scene->meshes.GetComponent(new_entity);
                if(meshComponent != nullptr) _resources_internal_cloneMeshRef(meshComponent, instance, ref_armature);

                auto armatureComponent = GetLibraryData()->scene->armatures.GetComponent(new_entity);
                if(armatureComponent != nullptr) _resources_internal_cloneArmatureRef(armatureComponent, instance);
            }
            for(auto mesh_entity : ref_mesh){
                auto meshComponent = GetLibraryData()->scene->meshes.GetComponent(mesh_entity);
                if(meshComponent != nullptr) _resources_internal_cloneMeshRef(meshComponent, instance, ref_armature);
            }
            for(auto armature_entity : ref_armature){
                auto armatureComponent = GetLibraryData()->scene->armatures.GetComponent(armature_entity);
                if(armatureComponent != nullptr) _resources_internal_cloneArmatureRef(armatureComponent, instance);
            }
        }
        for(auto entity : _collection->entities){
            // If there are animations within the collection, we have to be careful to actually also clone it!
            auto AnimationComponent = GetLibraryData()->scene->animations.GetComponent(entity);
            if(AnimationComponent != nullptr){
                bool clone_animation;
                
                for(auto channel : AnimationComponent->channels){
                    auto instance_get = instance.entities.find(channel.target);
                    if(instance_get != instance.entities.end()){
                        clone_animation = true;
                        break;
                    }
                }

                if(clone_animation){
                    auto new_animation = _resources_internal_cloneEntity(entity, instance, ld_full);
                    auto new_animationComponent = GetLibraryData()->scene->animations.GetComponent(entity);
                    _resources_internal_cloneAnimationRef(new_animationComponent, instance);
                }
            }
        }
    }else{ // There's no collection exist for the request, we need to load it first!
        // Force to use always instance if the object needed is the subresource
        if(!subresource.empty()) ld_always_instance = true;

        wi::scene::Scene scene_staging;
        wi::scene::LoadModel(scene_staging, file);

        GetLibraryData()->collections.emplace_back();
        auto& collection = GetLibraryData()->collections.back();
        collection.name = file;

        // A Collection must have 1 Instance at least!
        GetLibraryData()->instances.emplace_back();
        auto& instance = GetLibraryData()->instances.back();
        instance.instance_uuid = _resources_internal_generateUUID();
        instance.collection_name = file;
        
        _resources_internal_listLoadedEntities(scene_staging, collection, instance);

        instance_uuid = instance.instance_uuid;

        GetLibraryData()->scene->Merge(scene_staging);

        // To keep resource references, a collection has to keep hold of resource's data (avoid deletion when disabled)
        collection.KeepResources();

        // The always instance strategy are used for assets that'll have its state changed over the lifespan of the object
        // If we clone an object that is not in its original state for another component it'll be disastrous (e.g. characters)
        // For static objects, since it's state won't change over time we can just disable the always instance flag.
        if(ld_always_instance){
            // Disable all entities within the current collection!
            Library::RebuildInfoMap();

            for(auto entity : collection.entities){
                Entity_Disable(entity);
            }

            instance_uuid = Library::Load(file, subresource, loadingstrategy, loadingflags);
        }
    }

    Library::RebuildInfoMap();

    return instance_uuid;
}

void Library::Load_Async(std::string file, std::function<void(uint32_t)> callback, std::string subresource, uint32_t loadingstrategy, uint32_t loadingflags){
    wi::jobsystem::Execute(GetLibraryData()->jobs, [=](wi::jobsystem::JobArgs args){
        uint32_t instance_uuid = Load(file, subresource, loadingstrategy, loadingflags);
        if(callback != nullptr) wi::eventhandler::Subscribe_Once(wi::eventhandler::EVENT_THREAD_SAFE_POINT, [=](uint64_t userdata){
            callback(instance_uuid);
        });
    });
}

bool Library::Exist(std::string file, std::string subresource){
    auto found_collection = GetLibraryData()->collections_map.find(file);
    if(found_collection != GetLibraryData()->collections_map.end()){
        auto index = found_collection->second;
        auto collection = GetLibraryData()->collections.at(index);

        if(!subresource.empty()){
            for(auto entity : collection.entities){
                auto nameComponent = GetLibraryData()->scene->names.GetComponent(entity);
                if(nameComponent->name == subresource){
                    return true;
                }
            }
        }else return true;
    }

    return false;
}

void Library::RebuildInfoMap(){
    // Rebuild the collection array map
    GetLibraryData()->collections_map.clear();
    for(size_t index=0; index<GetLibraryData()->collections.size(); ++index){
        GetLibraryData()->collections[index].instance_count = 0;
        GetLibraryData()->collections_map[GetLibraryData()->collections[index].name] = index;
    }

    // Rebuild the instance array map
    GetLibraryData()->instances_map.clear();
    for(size_t index=0; index<GetLibraryData()->instances.size(); ++index){
        GetLibraryData()->collections[GetLibraryData()->collections_map[GetLibraryData()->instances[index].collection_name]].instance_count = 1;
        GetLibraryData()->instances_map[GetLibraryData()->instances[index].instance_uuid] = index;
    }
}

void Library::Update(float dt){
    for(int instance_index = 0; instance_index<GetLibraryData()->instances.size(); ++instance_index){
        if(GetLibraryData()->instances[instance_index].Empty()){
            GetLibraryData()->instances[instance_index] = std::move(GetLibraryData()->instances.back());
            GetLibraryData()->instances.pop_back();
            RebuildInfoMap();
        }
    }
    for(auto& collection : GetLibraryData()->collections){
        if(collection.instance_count == 0){
            if(collection.delete_countdown <= 0.f){
                collection.FreeResources();
                collection.Entities_Wipe();
                collection = std::move(GetLibraryData()->collections.back());
                GetLibraryData()->collections.pop_back();
                RebuildInfoMap();
            }else{
                collection.delete_countdown -= dt;
            }
        }else{
            collection.delete_countdown = 60.f;
        }
    }
}



void Library::Entity_Disable(wi::ecs::Entity entity){
    auto found_disabled = Library::GetLibraryData()->disabled_entities.find(entity);
    if(found_disabled == Library::GetLibraryData()->disabled_entities.end()){
        GetLibraryData()->disabled_entities[entity] = std::make_shared<wi::Archive>();
        auto& archive = *GetLibraryData()->disabled_entities[entity];
        archive.SetReadModeAndResetPos(false);

        wi::ecs::EntitySerializer seri;
        seri.allow_remap = false;
        auto serializeflags = wi::scene::Scene::EntitySerializeFlags::KEEP_INTERNAL_ENTITY_REFERENCES;
        GetLibraryData()->scene->Entity_Serialize(archive, seri, entity, serializeflags);
        GetLibraryData()->scene->Entity_Remove(entity,false);
    }
}

wi::ecs::Entity Library::Entity_Enable(wi::ecs::Entity entity, bool clone){
    auto found_disabled = Library::GetLibraryData()->disabled_entities.find(entity);
    if(found_disabled != Library::GetLibraryData()->disabled_entities.end()){
        auto& archive = *found_disabled->second.get();
        archive.SetReadModeAndResetPos(true);

        wi::ecs::EntitySerializer seri;
        auto serializeflags = wi::scene::Scene::EntitySerializeFlags::KEEP_INTERNAL_ENTITY_REFERENCES;
        if(!clone){
            seri.allow_remap = false;
        }
        
        auto restore_entity = GetLibraryData()->scene->Entity_Serialize(archive, seri, entity, serializeflags);
        return restore_entity;
    }
    return wi::ecs::INVALID_ENTITY;
}

void Library::Entity_ApplyConfig(wi::ecs::Entity target, wi::ecs::Entity config, COMPONENT_FILTER componentfilter){
    bool remove_config_after = (componentfilter & COMPONENT_FILTER_REMOVE_CONFIG_AFTER);
    bool use_layer_transform = (componentfilter & COMPONENT_FILTER_USE_LAYER_TRANSFORM);

    if(use_layer_transform){
        auto layerComponent_tgt = GetLibraryData()->scene->layers.GetComponent(target);
        auto layerComponent_cfg = GetLibraryData()->scene->layers.GetComponent(config);
        if(layerComponent_tgt != nullptr && layerComponent_cfg != nullptr){
            layerComponent_tgt->layerMask = layerComponent_cfg->layerMask;
        }
        auto transformComponent_tgt = GetLibraryData()->scene->transforms.GetComponent(target);
        auto transformComponent_cfg = GetLibraryData()->scene->transforms.GetComponent(config);
        if(transformComponent_tgt != nullptr && transformComponent_cfg != nullptr){
            transformComponent_tgt->SetDirty();
            transformComponent_tgt->translation_local = transformComponent_cfg->translation_local;
            transformComponent_tgt->rotation_local = transformComponent_cfg->rotation_local;
            transformComponent_tgt->scale_local = transformComponent_cfg->scale_local;
            transformComponent_tgt->UpdateTransform();
        }
    }

    auto objectComponent_tgt = GetLibraryData()->scene->objects.GetComponent(target);
    auto objectComponent_cfg = GetLibraryData()->scene->objects.GetComponent(config);
    if(objectComponent_tgt != nullptr && objectComponent_cfg != nullptr){
        objectComponent_tgt->_flags = objectComponent_cfg->_flags;
        objectComponent_tgt->center = objectComponent_cfg->center;
        objectComponent_tgt->cascadeMask = objectComponent_cfg->cascadeMask;
        objectComponent_tgt->color = objectComponent_cfg->color;
        objectComponent_tgt->emissiveColor = objectComponent_cfg->emissiveColor;
        objectComponent_tgt->userStencilRef = objectComponent_cfg->userStencilRef;
        objectComponent_tgt->lod_distance_multiplier = objectComponent_cfg->lod_distance_multiplier;
    }
    auto meshComponent_tgt = GetLibraryData()->scene->meshes.GetComponent(target);
    auto meshComponent_cfg = GetLibraryData()->scene->meshes.GetComponent(config);
    if(meshComponent_tgt != nullptr && meshComponent_cfg != nullptr){
        meshComponent_tgt->_flags = meshComponent_cfg->_flags;
        meshComponent_tgt->subsets = meshComponent_cfg->subsets;
        meshComponent_tgt->subsets_per_lod = meshComponent_cfg->subsets_per_lod;
        meshComponent_tgt->tessellationFactor = meshComponent_cfg->tessellationFactor;
    }
    auto rigidbodyComponent_tgt = GetLibraryData()->scene->rigidbodies.GetComponent(target);
    auto rigidbodyComponent_cfg = GetLibraryData()->scene->rigidbodies.GetComponent(config);
    if(rigidbodyComponent_tgt != nullptr && rigidbodyComponent_cfg != nullptr){
        rigidbodyComponent_tgt->_flags = rigidbodyComponent_cfg->_flags;
        rigidbodyComponent_tgt->mass = rigidbodyComponent_cfg->mass;
        rigidbodyComponent_tgt->friction = rigidbodyComponent_cfg->friction;
        rigidbodyComponent_tgt->restitution = rigidbodyComponent_cfg->restitution;
        rigidbodyComponent_tgt->damping_linear = rigidbodyComponent_cfg->damping_linear;
        rigidbodyComponent_tgt->damping_angular = rigidbodyComponent_cfg->damping_angular;

        rigidbodyComponent_tgt->shape = rigidbodyComponent_cfg->shape;
        rigidbodyComponent_tgt->box = rigidbodyComponent_cfg->box;
        rigidbodyComponent_tgt->sphere = rigidbodyComponent_cfg->sphere;
        rigidbodyComponent_tgt->capsule = rigidbodyComponent_cfg->capsule;
    }
    auto softbodyComponent_tgt = GetLibraryData()->scene->softbodies.GetComponent(target);
    auto softbodyComponent_cfg = GetLibraryData()->scene->softbodies.GetComponent(config);
    if(softbodyComponent_tgt != nullptr && softbodyComponent_cfg != nullptr){
        softbodyComponent_tgt->_flags = softbodyComponent_cfg->_flags;
        softbodyComponent_tgt->mass = softbodyComponent_cfg->mass;
        softbodyComponent_tgt->friction = softbodyComponent_cfg->friction;
        softbodyComponent_tgt->restitution = softbodyComponent_cfg->restitution;
    }
    auto lightComponent_tgt = GetLibraryData()->scene->lights.GetComponent(target);
    auto lightComponent_cfg = GetLibraryData()->scene->lights.GetComponent(config);
    if(lightComponent_tgt != nullptr && lightComponent_cfg != nullptr){
        lightComponent_tgt->_flags = lightComponent_cfg->_flags;
        lightComponent_tgt->color = lightComponent_cfg->color;
        lightComponent_tgt->type = lightComponent_cfg->type;
        lightComponent_tgt->intensity = lightComponent_cfg->intensity;
        lightComponent_tgt->range = lightComponent_cfg->range;
        lightComponent_tgt->outerConeAngle = lightComponent_tgt->outerConeAngle;
        lightComponent_tgt->innerConeAngle = lightComponent_tgt->innerConeAngle;
    }

    if(remove_config_after) GetLibraryData()->scene->Entity_Remove(config,false);
}




uint32_t Library::CreateCollection(std::string& name){
    bool name_avail = (GetLibraryData()->collections_map.find(name) != GetLibraryData()->collections_map.end());
    if(name_avail) {
        auto new_collection = GetLibraryData()->collections.emplace_back();
        new_collection.keep_alive = true;
        auto new_instance = GetLibraryData()->instances.emplace_back();
        new_instance.instance_uuid = _resources_internal_generateUUID();
        new_instance.collection_name = name;
        new_instance.keep_alive = true;
        new_collection.instance_count++;
        RebuildInfoMap();
        return new_instance.instance_uuid;
    }
    return 0;
}

Library::Collection* Library::GetCollection(std::string& name){
    auto collection_find = GetLibraryData()->collections_map.find(name);
    if(collection_find != GetLibraryData()->collections_map.end()){
        return &GetLibraryData()->collections[collection_find->second];
    }
    return nullptr;
}

Library::Instance* Library::GetInstance(uint32_t instance_uuid){
    auto instance_find = GetLibraryData()->instances_map.find(instance_uuid);
    if(instance_find != GetLibraryData()->instances_map.end()){
        return &GetLibraryData()->instances[instance_find->second];
    }
    return nullptr;
}

void Library::SyncInstance(uint32_t instance_uuid){
    auto instance_find = GetLibraryData()->instances_map.find(instance_uuid);
    if(instance_find != GetLibraryData()->instances_map.end()){
        auto& instance = GetLibraryData()->instances[instance_find->second];
        auto collection_find = GetLibraryData()->collections_map.find(instance.collection_name);
        auto& collection = GetLibraryData()->collections[collection_find->second];
        // Phase 1 : Verification
        bool sync_original = false;
        std::set<wi::ecs::Entity> collection_ent;
        for(auto& entity : collection.entities){
            collection_ent.insert(entity);
        }
        for(auto& entity_kval : instance.entities){
            auto entity = entity_kval.second;
            if(!sync_original){
                if(collection_ent.find(entity) != collection_ent.end()){
                    sync_original = true;
                    break;
                }
            }
        }
        // Phase 2 : Syncing
        for(auto& entity : collection_ent){
            if(instance.entities.find(entity) == instance.entities.end()){
                if(sync_original){
                    instance.entities[entity] = entity;
                }else{
                    _resources_internal_cloneEntity(entity, instance, false);
                }
            }
        }
    }
}



void Library::Collection::KeepResources(){
    for(auto entity : entities){
        auto materialComponent = Library::GetLibraryData()->scene->materials.GetComponent(entity);
        if(materialComponent != nullptr){
            for(int i=wi::scene::MaterialComponent::BASECOLORMAP; i<wi::scene::MaterialComponent::TEXTURESLOT_COUNT; ++i){
                resources.push_back(materialComponent->textures[i].resource);
            }
        }
        auto lightComponent = Library::GetLibraryData()->scene->lights.GetComponent(entity);
        if(lightComponent != nullptr){
            for(auto lensFlareRimTexture : lightComponent->lensFlareRimTextures){
                resources.push_back(lensFlareRimTexture);
            }
        }
        auto decalComponent = Library::GetLibraryData()->scene->decals.GetComponent(entity);
        if(decalComponent != nullptr){
            resources.push_back(decalComponent->texture);
            resources.push_back(decalComponent->normal);
        }
        auto weatherComponent = Library::GetLibraryData()->scene->weathers.GetComponent(entity);
        if(weatherComponent != nullptr){
            resources.push_back(weatherComponent->skyMap);
            resources.push_back(weatherComponent->colorGradingMap);
        }
        auto soundComponent = Library::GetLibraryData()->scene->sounds.GetComponent(entity);
        if(soundComponent != nullptr){
            resources.push_back(soundComponent->soundResource);
        }
    }
}

void Library::Collection::FreeResources(){
    resources.clear();
}

void Library::Collection::Entities_Wipe(){
    for(auto entity: entities){
        bool found_disabled = (GetLibraryData()->disabled_entities.find(entity) != GetLibraryData()->disabled_entities.end());
        if(found_disabled) Library::Entity_Enable(entity);
        GetLibraryData()->scene->Entity_Remove(entity,false);
    }
    entities.clear();
}


std::vector<wi::ecs::Entity> Library::Instance::GetEntities(){
    std::vector<wi::ecs::Entity> get;
    for(auto entity_kval : entities){
        auto entity = entity_kval.second;
        get.push_back(entity);
    }
    return get;
}

wi::ecs::Entity Library::Instance::Entity_FindByName(std::string name){
    for(auto entity_kval : entities){
        auto entity = entity_kval.second;
        auto name_find = GetLibraryData()->scene->names.GetComponent(entity);
        if(name_find != nullptr){
            if(name == name_find->name) return entity;
        }
    }
    return wi::ecs::INVALID_ENTITY;
}

void Library::Instance::Entity_Remove(wi::ecs::Entity entity){
    bool is_first = (GetLibraryData()->collections[GetLibraryData()->collections_map[collection_name]].entities[0] == entities[0]);
    for(auto entity_kval : entities){
        auto index = entity_kval.first;
        auto _entity = entity_kval.second;

        if(_entity == entity){
            if(is_first) Library::Entity_Disable(entity);
            else GetLibraryData()->scene->Entity_Remove(entity,false);
            entities.erase(index);
            break;
        }
    }
}

void Library::Instance::Entities_Wipe(){
    auto entities_to_wipe = GetEntities();
    for(auto entity : entities_to_wipe){
        Entity_Remove(entity);
    }
}

bool Library::Instance::Empty(){
    return (entities.size() == 0);
}

void LiveUpdate::Update(float dt){
    
}