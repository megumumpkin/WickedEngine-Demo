#include "Scene.h"

namespace Game{
    Scene* GetScene(){
        static Scene scene;
        return &scene;
    }
    void Scene::Archive::Load(std::string file, Scene &scene)
    {
        
    }
    void Scene::Archive::Save(std::string file, Scene &scene, bool Direct)
    {
        // TODO save data to zip archive
    }
    void Scene::Component_Prefab::Serialize(wi::Archive &archive, wi::ecs::EntitySerializer &seri)
    {
        if(archive.IsReadMode())
        {
            archive >> (uint32_t&)loadMode;
            archive >> targetEntity;
            archive >> streamDistance;
        }
        else 
        {
            archive << (uint32_t)loadMode;
            archive << targetEntity;
            archive << streamDistance;
        }
    }
    void Scene::Component_Preview::Serialize(wi::Archive &archive, wi::ecs::EntitySerializer &seri)
    {
        if(archive.IsReadMode())
        {
            archive >> prefabs_before_hidden;
        }
        else
        {
            archive << prefabs_before_hidden;
        }
    }
}