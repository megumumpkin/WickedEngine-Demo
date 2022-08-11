#pragma once
#include <WickedEngine.h>
#include "Resources_BindLua.h"

namespace Game::ScriptBindings{
    void Init();
    void Update(float dt);
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