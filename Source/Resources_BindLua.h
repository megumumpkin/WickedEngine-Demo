#pragma once
#include <LUA/lua.h>
#include <wiLua.h>
#include <wiLuna.h>
#include "Resources.h"

namespace Game::ScriptBindings::Resources{
    void Update();
    void Bind();
}