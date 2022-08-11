#pragma once
#include "imgui.h"

namespace Game::ScriptBindings::ImGui{
    static const char* Bindings_Globals = R"(
        function imgui_draw()
            waitSignal("game_debug_ui")
        end
    )";
    void BeginFrame();
    void Bind();
}