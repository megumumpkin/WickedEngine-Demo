#pragma once
#include "stdafx.h"

#include "Scripting.h"

namespace Game{
    struct Scene
    {
        struct FadeData
        {
            float object = 0.f;
            float light = 0.f;
            float material = 0.f;
        };
        // Scene streaming structures
        struct Archive
        {
            // Scene file structure
            // |- scene.wiscene  -> main scene file
            // |- scene.preview  -> a single entity that stores the preview data
            // |- scene.area  -> a file that stores a float, about a scene boundary

            bool is_root = false;
            
            wi::ecs::Entity archiveID = wi::ecs::INVALID_ENTITY; // Also has preview of the scene
            std::string file; // File name of the wiscene file
            wi::unordered_set<wi::ecs::Entity> entities;
            wi::unordered_map<uint64_t, wi::ecs::Entity> remap; // Remap data for the serialized file
            wi::unordered_map<wi::ecs::Entity, FadeData> fade_data; // Fade data
            wi::unordered_map<std::string, wi::ecs::Entity> entity_name_map; // For fast entity searching
            uint32_t dependency_count = 0; // Prefab dependency counter, useful for determining wheter to load or unload from disk

            // Scene streaming extradata
            // Boundary data
            wi::primitive::AABB bounds;
            // Preview data
            wi::scene::TransformComponent preview_transform;

            // Streaming data
            enum class LoadState
            {
                UNINITIALIZED,
                UNLOADED,
                LOADING,
                LOADED
            };
            LoadState load_state = LoadState::UNINITIALIZED; // Check loading progress of streaming

            void Init(); // Initialize archive before anything - for prefab only
            void Load(wi::ecs::Entity prefabID = wi::ecs::INVALID_ENTITY);
            void Unload(wi::ecs::Entity prefabID = wi::ecs::INVALID_ENTITY);
        };
        struct StreamData
        {
            enum class StreamType
            {
                INIT,
                ROOT,
                FULL
            };
            StreamType stream_type;
            std::string file; // File mapping for archive
            std::string actual_file; // Actual file path for loading the scene file
            wi::ecs::Entity archiveID;
            wi::unordered_set<wi::ecs::Entity> entities;
            wi::unordered_map<uint64_t, wi::ecs::Entity> remap;
            wi::unordered_map<std::string, wi::ecs::Entity> entity_name_map;
            std::shared_ptr<Scene> block; // Scene file where the serialization happens

            // Prefab specific data
            wi::scene::TransformComponent preview_transform;
            wi::unordered_map<wi::ecs::Entity, FadeData> fade_data;
        };
        struct StreamJob
        {
            wi::vector<std::shared_ptr<StreamData>> stream_queue;
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
            wi::unordered_set<wi::ecs::Entity> entities;
            wi::unordered_map<uint64_t, wi::ecs::Entity> remap; // Remap data for the serialized file, also used for entity listing
            wi::unordered_map<std::string, wi::ecs::Entity> entity_name_map; // For fast entity searching
            uint32_t safe_delete_counter = 0;
            bool loaded = false; // Has the prefab been loaded or not?
            bool disabled = false;

            // Fade management
            wi::ecs::Entity preview_object; // Object for preview
            float fade_factor = 0.f; // Fade factor - 1 is fully loaded - 0 is unloaded
            wi::vector<std::pair<wi::ecs::Entity, FadeData>> fade_data; // Original object's transparency are stored here

            wi::ecs::Entity FindEntityByName(std::string name); // Since prefabs get duplicate names, we have to have a function to just search INSIDE the prefab data
            void Enable(); // Enable all components from the prefab
            void Disable(); // Disable all components from the prefab
            void Unload(); // Remove all components from the prefab
            wi::ecs::Entity tostash_prefabID = wi::ecs::INVALID_ENTITY; // Just to stash remap data to scene database
            ~Prefab(); // Custom destructor to handle things
        };
        struct Inactive
        {
            wi::Archive inactive_storage;
        };

        // Component data attached to scene
        struct Component_Prefab : public Prefab
        {
            void Serialize(wi::Archive& archive, wi::ecs::EntitySerializer& seri);
        };
        struct Component_Inactive : public Inactive
        {
            void Serialize(wi::Archive& archive, wi::ecs::EntitySerializer& seri);
        };
        struct Component_Script : public Scripting::Script
        {
            void Serialize(wi::Archive& archive, wi::ecs::EntitySerializer& seri);
        };

        wi::scene::Scene wiscene;
        wi::ecs::ComponentManager<Component_Prefab>& prefabs = wiscene.componentLibrary.Register<Component_Prefab>("Game::Scene::Prefab");
        wi::ecs::ComponentManager<Component_Inactive>& inactives = wiscene.componentLibrary.Register<Component_Inactive>("Game::Scene::Inactive");
        wi::ecs::ComponentManager<Component_Script>& scripts = wiscene.componentLibrary.Register<Component_Script>("Game::Scene::Script");

        std::string current_scene; // Current filename that is used to point the root scene from scene_db
        wi::unordered_map<std::string, Archive> scene_db; // Lists all scenes referenced in this current game session
        wi::unordered_map<wi::ecs::Entity,wi::unordered_map<uint64_t, wi::ecs::Entity>> prefab_remap_stash;// Stashed serializers list

        // Streaming operation functions
        float stream_transition_speed = 3.f; // speed*framepseed
        XMFLOAT4 stream_loader_bounds = XMFLOAT4(0,0,0,10.f); // level streaming object
        float stream_loader_screen_estate = 0.5f; // until it is 10% of screen estate we do unload

        // Scene operation functions
        bool Entity_Exists(wi::ecs::Entity entity);
        void Entity_Disable(wi::ecs::Entity entity);
        void Entity_Enable(wi::ecs::Entity entity);
        wi::ecs::Entity Entity_Clone(
            wi::ecs::Entity entity, 
            wi::ecs::EntitySerializer& seri, 
            bool deep_copy = false, 
            wi::unordered_map<uint64_t, wi::ecs::Entity>* clone_seri = nullptr);

        // Load the scene file
        void Load(std::string file);
        void CreatePrefab(wi::ecs::Entity entity);

        void RunScriptUpdateSystem(wi::jobsystem::context& ctx);
        void RunPrefabUpdateSystem(float dt, wi::jobsystem::context& ctx);

        void PreUpdate(float dt);
        void Update(float dt);
    };

    inline Scene& GetScene()
    {
        static Scene scene;
        return scene;
    }
}