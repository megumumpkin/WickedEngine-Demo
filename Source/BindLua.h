#pragma once
#include <WickedEngine.h>
#include <wiArchive.h>
#include "Resources_BindLua.h"

namespace Game::ScriptBindings{
    // For scripting to work you have to initialize them first
    void Init();
    // Updates stuff which needs synchronization from Lua
    void Update(float dt);
    // To add new callbacks for any async processes that communicate with the scripting system
    void Register_AsyncCallback(std::string callback_type, std::function<void(std::string, std::shared_ptr<wi::Archive>)>);
    // To push a callback event for any async processes that calls c function from lua and wants results back
    void Push_AsyncCallback(std::string callback_UID, std::shared_ptr<wi::Archive> async_data);

    // Generate script pid if you want to fetch PID first before launching or other stuff
    uint32_t script_genPID();

    // For other script system to adapt the base injection
    uint32_t script_inject(std::string& script, std::string filename = "", uint32_t PID = 0);

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