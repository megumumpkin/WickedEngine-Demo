#pragma once

#include <WickedEngine.h>
#include <mutex>
#include <wiECS.h>
#include <wiHelper.h>
#include <wiScene.h>

namespace Game::Resources{
    namespace DataType{
        static inline const std::string SCENE_DATA = ".sco";
        static inline const std::string SCRIPT = ".lua";
    }
    namespace SourcePath{
        static inline const std::string SHADER = "Data/Shader";
        static inline const std::string INTERFACE = "Data/Interface";
        static inline const std::string LOCALE = "Data/Locale";
        static inline const std::string ASSET = "Data/Asset";
    }
    namespace Library{
        struct Instance{ 
            std::string file;
            wi::ecs::Entity collection;
            wi::vector<wi::ecs::Entity> instances;
        };
    }
    struct Scene{
        wi::scene::Scene scene_internal;
        wi::ecs::ComponentManager<Library::Instance> instances;
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