#pragma once

#include "stdafx.h"

namespace Game::Resources
{
    namespace DataType
    {
        static inline const std::string SCENE = ".scene";
        static inline const std::string SCRIPT = ".lua";
    }
    namespace SourcePath
    {
        static inline const std::string SHADER = "Data/Shader";
        static inline const std::string LOCALE = "Data/Locale";
        static inline const std::string CONTENT = "Data/Content";
    }
    struct Scene;
    namespace Library
    {
        /*  RULES for instance:
            You cannot serialize child entities that are not part of the original data!
            If you put a child entity there, it will be removed without caution!
        */
        struct Instance{
            std::string file; // Path to file
            std::string entity_name; // For cloning a specific entity
            enum LOADING_STRATEGY{
                LOAD_DIRECT,
                LOAD_INSTANTIATE,
                LOAD_PRELOAD
            }strategy;
            enum INSTANCE_TYPE{
                DEFAULT,
                LIBRARY
            }type;
            
            // Non serialized attributes
            Scene* scene;
            wi::ecs::Entity instance_id;
            wi::ecs::Entity collection_id;
            wi::unordered_set<wi::ecs::Entity> entities;
            enum LOADING_STATE
            {
                UNLOADED,
                LOADING,
                LOADED
            }load_state;

            bool lock = false; // Lock resource from automatically loading (Good for editing)

            void Init();
            void Unload();

            void Serialize(wi::Archive& archive, wi::ecs::EntitySerializer& seri);
        };

        // Stores entity disablement
        struct Disabled{            
            // Non serialized attributes
            wi::Archive entity_store;
            wi::unordered_map<uint64_t, wi::ecs::Entity> remap;

            void Serialize(wi::Archive& archive, wi::ecs::EntitySerializer& seri);
        };

        // For scene streaming please use this!
        struct Stream{
            std::string external_substitute_object; // If the substitute model is outside then we may need to load it
            wi::primitive::AABB stream_zone;

            // Non serialized attributes
            float transition;
            wi::unordered_map<wi::ecs::Entity, float> instance_original_transparency;

            void Serialize(wi::Archive& archive, wi::ecs::EntitySerializer& seri);
        };

        // AssetSmith metadata, for direct import and export of data
        struct AssetSmith{
            std::string file;

            void Serialize(wi::Archive& archive, wi::ecs::EntitySerializer& seri);
        }
    }
    struct Scene
    {
        wi::scene::Scene wiscene;
        wi::ecs::ComponentManager<Library::Instance>& instances = wiscene.componentLibrary.Register<Library::Instance>("game::component::instances");
        wi::ecs::ComponentManager<Library::Disabled>& disabled = wiscene.componentLibrary.Register<Library::Disabled>("game::component::disabled");
        wi::ecs::ComponentManager<Library::Stream>& streams = wiscene.componentLibrary.Register<Library::Stream>("game::component::streams");
        wi::ecs::ComponentManager<Library::AssetSmith>& assetmsith = wiscene.componentLibrary.Register<Library::AssetSmith>("game::component::assetsmith");

        // Library system data
        wi::unordered_map<uint32_t, wi::ecs::Entity> collections;

        wi::primitive::AABB stream_boundary;
        float stream_transition_time = 4.f;

        // Scene jobsystem
        std::mutex mutex_scenestream;
        wi::jobsystem::context job_scenestream;

        // Entity Creators
        wi::ecs::Entity CreateInstance(std::string name);
        
        // Append Data
        void SetStreamable(wi::ecs::Entity entity, bool set = true, wi::primitive::AABB bound = wi::primitive::AABB()); // True to enable, false to disable

        // Entity Management
        void Entity_Disable(wi::ecs::Entity entity);
        void Entity_Enable(wi::ecs::Entity entity);
        wi::ecs::Entity Entity_Clone(wi::ecs::Entity entity, wi::ecs::EntitySerializer& seri, bool recursive = true);

        // Scene processing functions
        void Init();
        void Library_Update(float dt);
        void Update(float dt);
        void Clear();
    };
    inline Scene& GetScene()
    {
        static Scene scene;
		return scene;
    }
    namespace LiveUpdate
    {
        // To check for scene data changes
        void Update(float dt);
    };
}