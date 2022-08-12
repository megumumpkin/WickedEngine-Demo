#pragma once

#include <WickedEngine.h>
#include <mutex>
#include <wiHelper.h>

namespace Game::Resources{
    namespace DataType{
        static inline const std::string SCENE_RESOURCE = ".scres";
        static inline const std::string SCENE_OBJECT = ".sco";
        static inline const std::string SCRIPT_GENERIC = ".lua";
    }
    namespace SourcePath{
        static inline const std::string SHADER = "Data/Shader";
        static inline const std::string INTERFACE = "Data/Interface";
        static inline const std::string LOCALE = "Data/Locale";
        static inline const std::string ASSET = "Data/Asset";
    }
    namespace Library{
        struct Collection{
            std::string name; // Collection name has to be unique, scene file names has to be unique!
            wi::vector<wi::ecs::Entity> entities; // Entities recorded within collection
            wi::vector<wi::Resource> resources; // Resources recorded within collection
            bool keep_alive = false; // Keep collection alive even if it is empty!
            int instance_count; // Counts instances which instance itself from this collection
            float delete_countdown = 60.f; // Delete only if counter reaches zero

            // Keep resources from unloading
            void KeepResources();
            // Free resources, let them unload when there's no object
            void FreeResources(); 
            // Wipe all entities from collection list AND in the scene itself
            void Entities_Wipe(); 
        };
        struct Instance{
            uint32_t instance_uuid; // Unique ID for every instances of a collection
            std::string collection_name; // Reference to the related collection
            wi::unordered_map<uint64_t, wi::ecs::Entity> entities; // List of instanced entities
            bool keep_alive = false; // Keep instance alive event if it is empty!

            // Helper functions for easier access to components

            // Get entity list of an instance
            wi::vector<wi::ecs::Entity> GetEntities();
            // Find entity by name
            wi::ecs::Entity Entity_FindByName(std::string name);
            // Remove an entity from both list and scene itself
            void Entity_Remove(wi::ecs::Entity entity); 
            // Wipe all entities from list AND in the scene itself
            void Entities_Wipe(); 
            // Check if instance is empty
            bool Empty(); 
        };
        struct Data{
            wi::vector<Collection> collections;
            wi::vector<Instance> instances;
            wi::scene::Scene* scene;
            wi::unordered_map<wi::ecs::Entity, std::shared_ptr<wi::Archive>> disabled_entities;

            std::mutex load_protect;
            wi::jobsystem::context jobs;

            // Fast access to collections and instances information
            wi::unordered_map<std::string, size_t> collections_map;
            wi::unordered_map<uint32_t, size_t> instances_map;
        };
        enum LOADING_STRATEGY : uint32_t{
            LOADING_STRATEGY_SHALLOW = 0,
            LOADING_STRATEGY_FULL = 1 << 0,
            LOADING_STRATEGY_ALWAYS_INSTANCE = 1 << 2,
            LOADING_STRATEGY_KEEP_ALIVE = 1 << 3
        };
        enum LOADING_FLAGS : uint32_t{
            LOADING_FLAGS_NONE = 0,
            LOADING_FLAG_RECURSIVE = 1 << 0
        };
        Data* GetLibraryData();
        // Init library system first before using
        void Init();
        // Load scene as collection
        uint32_t Load(std::string file, std::string subresource = "", uint32_t loadstrategy = LOADING_STRATEGY_SHALLOW, uint32_t loadingflags = LOADING_FLAGS_NONE);
        // Load scene as collection asynchronously
        void Load_Async(std::string file, std::function<void(uint32_t)> callback = nullptr, std::string subresource = "", uint32_t loadstrategy = LOADING_STRATEGY_SHALLOW, uint32_t loadingflags = LOADING_FLAGS_NONE);
        // Check if collection exists
        bool Exist(std::string file, std::string subresource = "");
        // Create collection, returns the first instance of that collection
        uint32_t CreateCollection(std::string& name);
        // Get collection from library
        Collection* GetCollection(std::string& name);
        // Get instance from library
        Instance* GetInstance(uint32_t instance_uuid);
        // Sync instance from with collection ?
        void SyncInstance(uint32_t instance_uuid);
        // Remove instance and its entities from scene and library
        bool RemoveInstance(uint32_t instance_uuid);
        // Rebuild access cache, for faster data access
        void RebuildInfoMap();
        // Update library system every frame, checks for collections' and instances' status
        void Update(float dt);

        // Disable component from actively contributing to the scene
        void Entity_Disable(wi::ecs::Entity entity);
        // Enable component to be actively contributing to the scene
        wi::ecs::Entity Entity_Enable(wi::ecs::Entity entity, bool clone = false);
        enum COMPONENT_FILTER : uint32_t{
            COMPONENT_FILTER_USE_BASE = 0,
            COMPONENT_FILTER_REMOVE_CONFIG_AFTER = 1 << 0,
            COMPONENT_FILTER_USE_LAYER_TRANSFORM = 1 << 1,
        };
        // Apply entity configurations from another entity which serves as the configuration data
        // Use COMPONENT_FILTER flags to fine-tune behaviour
        void Entity_ApplyConfig(wi::ecs::Entity target, wi::ecs::Entity data, COMPONENT_FILTER componentfilter);
    }
    namespace LiveUpdate{
        // To check for scene data changes
        void Update(float dt);
    };
}