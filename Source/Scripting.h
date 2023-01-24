#pragma once
#include "stdafx.h"

namespace Game::Scripting
{
    // For scripting to work you have to initialize them first
    void Init(wi::Application* app);
    // Updates stuff which needs synchronization from Lua
    void Update(float dt);
    // Attach this game framework's scripting parameters
    void AppendFrameworkScriptingParameters(std::string& script, std::string filename, uint32_t PID, const std::string& customparameters_prepend = "", const std::string& customparameters_append = "");

    // Callback system
    // To add new callbacks for any async processes that communicate with the scripting system
    void Register_AsyncCallback(std::string callback_type, std::function<void(std::string, std::shared_ptr<wi::Archive>)> callback_solver);
    // To push a callback event for any async processes that calls c function from lua and wants results back
    void Push_AsyncCallback(std::string callback_UID, std::shared_ptr<wi::Archive> async_data);

    struct Script
    {
        // Persistent data
        std::string file; // File path of the script to be launched
        std::string params; // Parameters of the script to be attached to

        // Runtime data
        // PID uses entityID!
        bool done_init = false; // Check if the script has been initialized or not
    };
}