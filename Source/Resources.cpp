#include "Resources.h"
#include <Utility/DirectXMath.h>
#include <memory>
#include <mutex>
#include <set>
#include <wiArchive.h>
#include <wiECS.h>
#include <wiHelper.h>
#include <wiJobSystem.h>
#include <wiScene.h>
#include <wiUnorderedMap.h>

using namespace Game::Resources;

void Library::Instance::Init(){
    auto find_collection = scene->collections.find(wi::helper::string_hash(file.c_str()));
    if(find_collection == scene->collections.end())
    {
        wi::jobsystem::Execute(scene->job_scenestream, [=](wi::jobsystem::JobArgs args_job){
            std::scoped_lock lock (scene->mutex_scenestream);

            Scene load_scene;

            auto prototype_root = wi::scene::LoadModel(load_scene.scene, file, XMMatrixIdentity(), true);
            scene->scene.Merge(load_scene.scene);
            
            scene->scene.Component_Attach(prototype_root, instance_id);
            scene->collections[wi::helper::string_hash(file.c_str())] = instance_id;
        });
    }
    else
    {
        collection_id = find_collection->second;

        //Duplicate and get root entity
        wi::ecs::Entity prototype_root;
        wi::ecs::EntitySerializer seri;
        if(entity_name != "") // If it has a name referenced, please iterate through the scene
        {
            for(auto& entity : entities){
                auto nameComponent = scene->scene.names.GetComponent(entity);

                if(nameComponent != nullptr){
                    if(nameComponent->name == entity_name) {
                        prototype_root = scene->Entity_Clone(collection_id,seri);
                        break;
                    }
                }
            }
        }
        else //Just duplicate the entire entity
        {
            prototype_root = scene->Entity_Clone(collection_id,seri);
        }


        auto clone_instance = scene->instances.GetComponent(prototype_root);
        if(clone_instance != nullptr)
        {
            //Remap hierarchy & entity then
            for(auto& entity : entities){
                if(seri.remap.find(entity) != seri.remap.end()) entity = seri.remap.find(entity)->second;
                
                auto hierarchyComponent = scene->scene.hierarchy.GetComponent(entity);

                if(hierarchyComponent != nullptr){
                    if(hierarchyComponent->parentID == prototype_root){
                        scene->scene.Component_Attach(entity, instance_id);
                    }
                }
            }

            //Remove the old root entity
            scene->scene.Entity_Remove(prototype_root);
        }
    }
}

void Library::Instance::Unload(){
    for(auto& entity : entities){
        auto hierarchyComponent = scene->scene.hierarchy.GetComponent(entity);

        if(hierarchyComponent != nullptr){
            if(hierarchyComponent->parentID == instance_id){
                scene->scene.Entity_Remove(entity);
            }
        }
    }
}

void Library::Instance::Serialize(wi::Archive& archive, wi::ecs::EntitySerializer& seri){
    if(archive.IsReadMode())
    {
        archive >> file;
        archive >> entity_name;
    }
    else
    {
        archive << file;
        archive << entity_name;
    }
}



void Library::Disabled::Serialize(wi::Archive& archive, wi::ecs::EntitySerializer& seri){
    wi::ecs::SerializeEntity(archive, entity, seri);
}



void Library::Stream::Serialize(wi::Archive &archive, wi::ecs::EntitySerializer &seri){
    wi::ecs::SerializeEntity(archive, substitute_object, seri);
    stream_zone.Serialize(archive, seri);
}



// Other initializers in Resources_BindLua.cpp

Library::ScriptObject::~ScriptObject(){
    Unload();
}

void Library::ScriptObject::Serialize(wi::Archive &archive, wi::ecs::EntitySerializer &seri){
    if(archive.IsReadMode())
    {
        archive >> file;
        archive >> properties;
    }
    else
    {
        archive << file;
        archive << properties;
    }
}



Scene::Scene(){
    scene.componentLibrary.SetLibraryVersion(1);
}

void Scene::Entity_Disable(wi::ecs::Entity entity){
    auto disable_handle = wi::ecs::CreateEntity();
    auto& disabledComponent = disabled.Create(disable_handle);

    wi::ecs::EntitySerializer seri;
 
    disabledComponent.entity_store.SetReadModeAndResetPos(false);
    scene.Entity_Serialize(disabledComponent.entity_store, seri, entity,
        wi::scene::Scene::EntitySerializeFlags::RECURSIVE | wi::scene::Scene::EntitySerializeFlags::KEEP_INTERNAL_ENTITY_REFERENCES);

    disabled_list[entity] = disable_handle;
    disabledComponent.remap = seri.remap;
}

void Scene::Entity_Enable(wi::ecs::Entity entity){
    auto find_disabled = disabled_list.find(entity);
    if(find_disabled != disabled_list.end())
    {
        auto disabledComponent = disabled.GetComponent(find_disabled->second);

        wi::ecs::EntitySerializer seri;
        seri.remap = disabledComponent->remap;
        disabledComponent->entity_store.SetReadModeAndResetPos(true);
        wi::ecs::Entity root = scene.Entity_Serialize(disabledComponent->entity_store, seri, wi::ecs::INVALID_ENTITY,
            wi::scene::Scene::EntitySerializeFlags::RECURSIVE | wi::scene::Scene::EntitySerializeFlags::KEEP_INTERNAL_ENTITY_REFERENCES);
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
        scene.Entity_Serialize(*archive, seri, entity, wi::scene::Scene::EntitySerializeFlags::RECURSIVE);
    }

    archive->SetReadModeAndResetPos(true);
    auto root = scene.Entity_Serialize(*archive, seri, wi::ecs::INVALID_ENTITY,
        wi::scene::Scene::EntitySerializeFlags::RECURSIVE | wi::scene::Scene::EntitySerializeFlags::KEEP_INTERNAL_ENTITY_REFERENCES);

    return root;
}

void Scene::Library_Update(float dt){
    for(int i = 0; i < instances.GetCount(); ++i){
        auto& instance = instances[i];
        auto instance_entity = instances.GetEntity(i);
        
        if(!streams.Contains(instance_entity) && (instance.scene == nullptr)){
            instance.scene = this;
            instance.instance_id = instance_entity;
            instance.Init();
        }
    }
    for(int i = 0; i < scriptobjects.GetCount(); ++i){
        auto& scriptobject = scriptobjects[i];
        auto scriptobject_entity = scriptobjects.GetEntity(i);
        
        if(!streams.Contains(scriptobject_entity) && (scriptobject.script_pid == 0)){
            scriptobject.Init();
        }
    }
    for(int i = 0; i < streams.GetCount(); ++i){
        auto& stream = streams[i];
        auto stream_entity = streams.GetEntity(i);

        if(stream.stream_zone.intersects(stream_boundary))
        {
            stream.transition += dt*stream_transition_time;
            stream.transition = std::max(stream.transition,0.f);

            if(stream.transition == 1.f){
                auto instance = instances.GetComponent(stream_entity);
                if(instance != nullptr){
                    if(instance->scene == nullptr){
                        instance->scene = this;
                        instance->instance_id = stream_entity;
                        instance->Init();
                    }
                }
                auto scriptobject = scriptobjects.GetComponent(stream_entity);
                if(scriptobject != nullptr) scriptobject->Init();
            }
        }
        else
        {
            stream.transition -= dt*stream_transition_time;
            stream.transition = std::min(stream.transition,1.f);

            if(stream.transition == 1.f){
                auto instance = instances.GetComponent(stream_entity);
                if(instance != nullptr){
                    if(instance->scene != nullptr){
                        instance->Unload();
                        instance->scene = nullptr;
                    }
                }
                auto scriptobject = scriptobjects.GetComponent(stream_entity);
                if(scriptobject != nullptr) scriptobject->Unload();
            }
        }
    }
}

void Scene::Update(float dt){
    Library_Update(dt);
}

void LiveUpdate::Update(float dt){
    
}