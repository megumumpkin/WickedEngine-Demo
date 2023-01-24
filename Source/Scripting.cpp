#include "Scripting.h"
#include "Scripting_Globals.h"
#include "Scene_BindScript.h"

#include <wiApplication_BindLua.h>

static const char* WILUA_ERROR_PREFIX = "[Lua Error] ";

// Add custom c bind functions here
int Bind_DoFile(lua_State* L)
{
    int argc = wi::lua::SGetArgCount(L);

    if (argc > 0)
    {
        uint32_t PID = 0;

        std::string filename = wi::lua::SGetString(L, 1);
        if(argc >= 2) PID = wi::lua::SGetInt(L, 2);
        std::string customparameters_prepend;
        if(argc >= 3) customparameters_prepend = wi::lua::SGetString(L, 3);
        std::string customparameters_append;
        if(argc >= 4) customparameters_prepend = wi::lua::SGetString(L, 4);

        wi::vector<uint8_t> filedata;

        if (wi::helper::FileRead(filename, filedata))
        {
            if(PID == wi::ecs::INVALID_ENTITY)
                PID = wi::ecs::CreateEntity();
            std::string command = std::string(filedata.begin(), filedata.end());
            Game::Scripting::AppendFrameworkScriptingParameters(command, filename, PID, customparameters_prepend, customparameters_append);

            int status = luaL_loadstring(L, command.c_str());
            if (status == 0)
            {
                status = lua_pcall(L, 0, LUA_MULTRET, 0);
                auto return_PID = std::to_string(PID);
                wi::lua::SSetString(L, return_PID);
            }
            else
            {
                const char* str = lua_tostring(L, -1);

                if (str == nullptr)
                    return 0;

                std::string ss;
                ss += WILUA_ERROR_PREFIX;
                ss += str;
                wi::backlog::post(ss, wi::backlog::LogLevel::Error);
                lua_pop(L, 1); // remove error message
            }
        }
    }
    else
    {
        wi::lua::SError(L, "dofile(string filename) not enough arguments!");
    }

    return 1;
}
wi::Application* app_get = nullptr;
int Bind_GetAppRuntime(lua_State* L)
{
    Luna<wi::lua::Application_BindLua>::push(L, new wi::lua::Application_BindLua(app_get));
    return 1;
}

void Game::Scripting::Init(wi::Application* app)
{
    app_get = app;

    wi::lua::RunText(Scripting_Globals);
    wi::lua::RegisterFunc("dofile", Bind_DoFile);

    Scene::Bind();

    wi::lua::RegisterFunc("GetAppRuntime", Bind_GetAppRuntime);
}

// Script tracking
wi::unordered_map<uint32_t, std::string> scripts;
wi::unordered_map<std::string, wi::vector<size_t>> scripts_filerefs;

// Insert framework specific scripting parameters
void Game::Scripting::AppendFrameworkScriptingParameters(std::string& script, std::string filename, uint32_t PID, const std::string& customparameters_prepend, const std::string& customparameters_append)
{
    static const std::string persistent_prepend = 
        "local D = _ENV.PROCESSES_DATA[script_pid()];"
        "setmetatable(D, {"
        "   __call = function(self, key, value)"
        "       if self[key] == nil then"
        "           self[key] = value;"
        "       else"
        "           Internal_SyncSubTable(self[key],value);"
        "       end;"
        "   end;"
        "});";
    wi::lua::AttachScriptParameters(script, filename, PID, persistent_prepend+customparameters_prepend, customparameters_append);
}

// Scripting callback system
wi::unordered_map<std::string, std::function<void(std::string,std::shared_ptr<wi::Archive>)>> async_callback_solvers;
wi::unordered_map<std::string, std::shared_ptr<wi::Archive>> async_callbacks;

void Game::Scripting::Register_AsyncCallback(std::string callback_type, std::function<void (std::string, std::shared_ptr<wi::Archive>)> callback_solver)
{
    async_callback_solvers[callback_type] = callback_solver;
}

void Game::Scripting::Push_AsyncCallback(std::string callback_UID, std::shared_ptr<wi::Archive> async_data)
{
    async_callbacks[callback_UID] = async_data;
}