#pragma once
#include <WickedEngine.h>
#include <wiArchive.h>
#include "Resources_BindLua.h"

namespace Game::ScriptBindings{
    void Init();
    void Update(float dt);
    void Register_AsyncCallback(std::string callback_type, std::function<void(std::string, wi::Archive)>);
    void Push_AsyncCallback(std::string callback_UID, wi::Archive& async_data);
    namespace LiveUpdate{
        struct ScriptReloadEvent{
            enum MODE{
                RELOAD,
                UNLOAD
            };
            MODE mode;
            enum TYPE{
                LUA,
                SCENE_OBJECT
            };
            TYPE type;
            std::string file;
        };
        void PushEvent(ScriptReloadEvent event);
        void Update(float dt);
    }
}