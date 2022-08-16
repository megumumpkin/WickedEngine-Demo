#include "BindLua.h"
#include "Resources.h"
#include "Resources_BindLua.h"
#include <LUA/lua.h>
#include <string>
#include <wiLua.h>

namespace Game::ScriptBindings::Resources{
    void Update(){

    }
    void Bind(){

    }
}

void Game::Resources::Library::ScriptObject::Init(){
    auto L = wi::lua::GetLuaState();
    lua_getglobal(L, "dofile");
    lua_pushstring(L, file.c_str());
    lua_call(L,1,1);
    script_pid = lua_tointeger(L, 1);

    lua_getglobal(L, "uploadScriptData");
    lua_pushstring(L, std::to_string(script_pid).c_str());
    lua_newtable(L);
    for(int i = 0; i < properties.size(); ++i){
        bool is_header = (i%2 == 1);
        if(is_header)
        {
            lua_setfield(L, -2, properties[i].c_str());
        }
        else
        {
            switch(properties[i][0]){
                case 'B':
                {
                    lua_pushboolean(L, (std::stoi(properties[i].substr(1)) > 0) ? true : false);
                    break;
                }
                case 'I':
                {
                    lua_pushinteger(L, std::stoi(properties[i].substr(1)));
                    break;
                }
                case 'F':
                {
                    lua_pushnumber(L, std::stof(properties[i].substr(1)));
                    break;
                }
                case 'S':
                {
                    lua_pushstring(L, properties[i].substr(1).c_str());
                    break;
                }
                default:
                    break;
            }
        }
    }
    lua_call(L,2,0);
}

void Game::Resources::Library::ScriptObject::Unload(){
    auto L = wi::lua::GetLuaState();
    lua_getglobal(L, "killProcessPID");
    lua_pushstring(L, std::to_string(script_pid).c_str());
    lua_call(L,1,0);
}