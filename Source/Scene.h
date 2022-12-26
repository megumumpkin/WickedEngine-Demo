#pragma once
#include "stdafx.h"

namespace Game{
    struct Scene
    {
        // Scene Internal Data
        struct Prefab
        {
            enum class LoadMode
            {
                SHALLOW_COPY,
                DEEP_COPY,
                FULL,
                LIBRARY
            };
            LoadMode loadMode = LoadMode::SHALLOW_COPY;
            wi::ecs::Entity targetEntity; // Entity to copy
            float streamDistance = 0.f; // Enable streaming if streaming distance is bigger than 0
        
            // Runtime Variables
            enum class LoadState
            {
                UNLOADED,
                LOADING,
                LOADED
            };
            LoadState loadState = LoadState::UNLOADED;
            wi::vector<wi::ecs::Entity> entity_list; // Precompiled entity list for ease of work
            float transitionFade = 0.f; // 1 is visible 0 is invisible
        };
        struct Preview
        {
            wi::vector<wi::ecs::Entity> prefabs_before_hidden;
        
            // Runtime Variables
            float transitionFade = 0.f;
        };
        struct Inactive
        {
            wi::Archive inactive_storage;
        };

        struct Component_Prefab : public Prefab
        {
            void Serialize(wi::Archive& archive, wi::ecs::EntitySerializer& seri);
        };
        struct Component_Preview : public Preview
        {
            void Serialize(wi::Archive& archive, wi::ecs::EntitySerializer& seri);
        };
        struct Component_Inactive : public Inactive
        {
            void Serialize(wi::Archive& archive, wi::ecs::EntitySerializer& seri){};
        };

        struct Archive
        {
            void Load(std::string file, Scene& scene);
            void Save(std::string file, Scene& scene, bool Direct = false); // For Game Editing
        };

        wi::scene::Scene wiscene;
        wi::ecs::ComponentManager<Component_Prefab>& prefabs = wiscene.componentLibrary.Register<Component_Prefab>("Game::Scene::Prefab");
        wi::ecs::ComponentManager<Component_Preview>& previews = wiscene.componentLibrary.Register<Component_Preview>("Game::Scene::Preview");
        wi::ecs::ComponentManager<Component_Inactive>& inactive = wiscene.componentLibrary.Register<Component_Inactive>("Game::Scene::Inactive");

        void Update(float dt);
    };
    Scene* GetScene();
}