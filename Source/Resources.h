#pragma once

#include <WickedEngine.h>
#include <mutex>
#include <any>

namespace Game::Resources{
    namespace DataType{
        static inline const std::string SCENE_DATA = ".bscn";
        static inline const std::string SCRIPT = ".lua";
    }
    namespace SourcePath{
        static inline const std::string SHADER = "Data/Shader";
        static inline const std::string INTERFACE = "Data/Interface";
        static inline const std::string LOCALE = "Data/Locale";
        static inline const std::string ASSET = "Data/Asset";
    }
    struct Scene;
    namespace Library{
        /*  RULES for instance:
            You cannot serialize child entities that are not part of the original data!
            If you put a child entity there, it will be removed without caution!
        */
        struct Instance{
            std::string file; // Path to file
            std::string entity_name; // For cloning a specific entity
            
            // Non serialized attributes
            Scene* scene;
            wi::ecs::Entity instance_id;
            wi::ecs::Entity collection_id;
            wi::vector<wi::ecs::Entity> entities;

            void Init();
            void Unload();

            void Serialize(wi::Archive& archive, wi::ecs::EntitySerializer& seri);
        };

        // Stores entity disablement
        struct Disabled{
            wi::ecs::Entity entity;
            
            // Non serialized attributes
            wi::unordered_map<uint64_t, wi::ecs::Entity> remap;
            wi::Archive entity_store;

            void Serialize(wi::Archive& archive, wi::ecs::EntitySerializer& seri);
        };

        // For scene streaming please use this!
        struct Stream{
            wi::ecs::Entity substitute_object;
            wi::primitive::AABB stream_zone;

            // Non serialized attributes
            float transition;

            void Serialize(wi::Archive& archive, wi::ecs::EntitySerializer& seri);
        };

        struct ScriptObject{
            std::string file; // Lua script file to load and attach
            wi::vector<std::string> properties;

            // Non serialized attributes
            uint32_t script_pid;

            void Init();
            void Unload();
            
            ~ScriptObject();

            void Serialize(wi::Archive& archive, wi::ecs::EntitySerializer& seri);
        };
    }
    struct Scene{
        wi::scene::Scene scene;
        wi::ecs::ComponentManager<Library::Instance>& instances = scene.componentLibrary.Register<Library::Instance>(1);
        wi::ecs::ComponentManager<Library::Disabled>& disabled = scene.componentLibrary.Register<Library::Disabled>(1);
        wi::ecs::ComponentManager<Library::Stream>& streams = scene.componentLibrary.Register<Library::Stream>(1);
        wi::ecs::ComponentManager<Library::ScriptObject>& scriptobjects = scene.componentLibrary.Register<Library::ScriptObject>(1);

        // Library system data
        wi::unordered_map<uint32_t, wi::ecs::Entity> collections;
        wi::unordered_map<wi::ecs::Entity, wi::ecs::Entity> disabled_list;
        wi::primitive::AABB stream_boundary;
        float stream_transition_time = 0.5f;

        // Scene jobsystem
        std::mutex mutex_scenestream;
        wi::jobsystem::context job_scenestream;

        // Scene processing functions
        Scene();
        void Init();

        void Entity_Disable(wi::ecs::Entity entity);
        void Entity_Enable(wi::ecs::Entity entity);
        wi::ecs::Entity Entity_Clone(wi::ecs::Entity entity, wi::ecs::EntitySerializer& seri);

        void Library_Update(float dt);
        void Update(float dt);
    };
    inline Scene& GetScene(){
        static Scene scene;
		return scene;
    }
    namespace LiveUpdate{
        // To check for scene data changes
        void Update(float dt);
    };
}