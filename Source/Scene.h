#pragma once
#include "stdafx.h"
#include <mutex>

namespace Game{
    struct Scene
    {
        // Scene streaming structures
        struct Archive
        {
            // Scene file structure
            // |- scene.wiscene  -> main scene file
            // |- scene.preview  -> a single entity that stores the preview data
            // |- scene.area  -> a file that stores a float, about a scene boundary

            std::string file; // File name of the wiscene file
            wi::ecs::Entity prefabID = wi::ecs::INVALID_ENTITY; // Target prefab if exists
            wi::unordered_map<uint64_t, wi::ecs::Entity> remap; // Remap data for the serialized file

            // Scene streaming extradata
            float area = 0.f; // Total area of the scene
            wi::unordered_map<uint64_t, wi::ecs::Entity> preview; // Preview of the scene

            // Streaming data
            enum class LoadState
            {
                UNLOADED,
                LOADING,
                LOADED
            };
            LoadState load_state = LoadState::UNLOADED; // Check loading progress of streaming
            bool stream_done = false; // Check if the thread has done streaming
            Scene* stream_block; // Get the block of the stream
            wi::ecs::Entity temp_clone_prefabID = wi::ecs::INVALID_ENTITY; // Temp storage for pending cloning

            void Load(wi::ecs::Entity clone_prefabID = wi::ecs::INVALID_ENTITY);
        };
        struct StreamData
        {
            std::string file; // Archive file where streaming is done
            wi::unordered_map<uint64_t, wi::ecs::Entity> remap; // Remap data for serialization in stream
            Scene* block; // Scene file where the serialization happens
        };
        struct StreamJob
        {
            wi::vector<StreamData*> stream_queue;
        };

        struct Prefab
        {
            std::string file; // A key to the archive file
            enum class CopyMode
            {
                SHALLOW_COPY,
                DEEP_COPY,
                LIBRARY
            };
            CopyMode copy_mode = CopyMode::SHALLOW_COPY;
            enum class StreamMode
            {
                DIRECT,
                DISTANCE,
                SCREEN_ESTATE,
                MANUAL,
            };
            StreamMode stream_mode = StreamMode::DIRECT;
            float stream_distance_multiplier = 1.f;

            // Runtime data
            wi::unordered_map<uint64_t, wi::ecs::Entity> remap; // Remap data for the serialized file, also used for entity listing
            bool loaded = false; // Is the prefab has been loaded or not?
        };
        struct Inactive
        {
            wi::vector<uint8_t> inactive_storage;
        };

        // Component data attached to scene
        struct Component_Prefab : public Prefab
        {
            void Serialize(wi::Archive& archive, wi::ecs::EntitySerializer& seri){};
        };
        struct Component_Inactive : public Inactive
        {
            void Serialize(wi::Archive& archive, wi::ecs::EntitySerializer& seri){};
        };

        wi::scene::Scene wiscene;
        wi::ecs::ComponentManager<Component_Prefab>& prefabs = wiscene.componentLibrary.Register<Component_Prefab>("Game::Scene::Prefab");
        wi::ecs::ComponentManager<Component_Inactive>& inactives = wiscene.componentLibrary.Register<Component_Inactive>("Game::Scene::Inactive");

        std::string current_scene; // Current filename that is used to point the root scene from scene_db
        wi::unordered_map<std::string, Archive> scene_db; // Lists all scenes referenced in this current game session

        // Scene operation functions
        void Entity_Disable(wi::ecs::Entity entity);
        void Entity_Enable(wi::ecs::Entity entity);
        void Entity_Clone(wi::ecs::Entity entity, wi::ecs::EntitySerializer* seri, bool deep_copy = false);

        // Load the scene file
        void Load(std::string file);

        void Update(float dt);
    };

    Scene* GetScene();
}