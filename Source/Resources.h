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
            int instance_count;
            float delete_countdown = 60.f; // Delete only if counter reaches zero

            void KeepResources();
            void FreeResources();
            void Entities_Wipe();
        };
        struct Instance{
            uint32_t instance_uuid;
            std::string collection_name;
            wi::unordered_map<uint64_t, wi::ecs::Entity> entities;

            //Helper functions for easier access to components
            wi::vector<wi::ecs::Entity> GetEntities();
            wi::ecs::Entity Entity_FindByName(std::string name);
            void Entity_Remove(wi::ecs::Entity entity);
            void Entities_Wipe();
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
            LOADING_STRATEGY_ALWAYS_INSTANCE = 1 << 2
        };
        enum LOADING_FLAGS : uint32_t{
            LOADING_FLAGS_NONE = 0,
            LOADING_FLAG_RECURSIVE = 1 << 0
        };
        Data* GetLibraryData();
        void Init();
        uint32_t Load(std::string file, std::string subresource = "", uint32_t loadstrategy = LOADING_STRATEGY_SHALLOW, uint32_t loadingflags = LOADING_FLAGS_NONE);
        void Load_Async(std::string file, std::function<void(uint32_t)> callback = nullptr, std::string subresource = "", uint32_t loadstrategy = LOADING_STRATEGY_SHALLOW, uint32_t loadingflags = LOADING_FLAGS_NONE);
        bool Exist(std::string file, std::string subresource = "");
        Instance* GetInstance(uint32_t instance_uuid);
        void RebuildInfoMap();
        void Update(float dt);

        void Entity_Disable(wi::ecs::Entity entity);
        wi::ecs::Entity Entity_Enable(wi::ecs::Entity entity, bool clone = false);
        enum COMPONENT_FILTER : uint32_t{
            COMPONENT_FILTER_USE_BASE = 0,
            COMPONENT_FILTER_REMOVE_CONFIG_AFTER = 1 << 0,
            COMPONENT_FILTER_USE_LAYER_TRANSFORM = 1 << 1,
        };
        void Entity_ApplyConfig(wi::ecs::Entity target, wi::ecs::Entity data, COMPONENT_FILTER componentfilter);
    }
    namespace LiveUpdate{
        void Update(float dt);
    };
}