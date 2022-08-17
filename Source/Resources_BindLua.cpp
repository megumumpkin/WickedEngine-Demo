#include "BindLua.h"
#include "Resources.h"
#include "Resources_BindLua.h"
#include <LUA/lua.h>
#include <string>
#include <wiBacklog.h>
#include <wiECS.h>
#include <wiLua.h>
#include <wiLuna.h>
#include <wiPrimitive_BindLua.h>

namespace Game::ScriptBindings::Resources{
    void Update()
    {

    }
    void Bind()
    {

    }

    

    const char Library_Instance_BindLua::className[] = "Instance";
    Luna<Library_Instance_BindLua>::FunctionType Library_Instance_BindLua::methods[] = {
        lunamethod(Library_Instance_BindLua, SetFile),
        lunamethod(Library_Instance_BindLua, GetFile),
        lunamethod(Library_Instance_BindLua, SetEntityName),
        lunamethod(Library_Instance_BindLua, GetEntityName),
        lunamethod(Library_Instance_BindLua, SetStrategy),
        lunamethod(Library_Instance_BindLua, GetStrategy),
        lunamethod(Library_Instance_BindLua, SetType),
        lunamethod(Library_Instance_BindLua, SetType),
        { NULL, NULL }
    };
    Luna<Library_Instance_BindLua>::PropertyType Library_Instance_BindLua::properties[] = {
        { NULL, NULL }
    };
    Library_Instance_BindLua::Library_Instance_BindLua(lua_State *L)
    {
        owning = true;
        component = new Game::Resources::Library::Instance;
    }
    Library_Instance_BindLua::~Library_Instance_BindLua()
    {
        if(owning){
            delete component;
        }
    }
    int Library_Instance_BindLua::SetFile(lua_State* L)
    {
        int argc = wi::lua::SGetArgCount(L);
        if (argc > 0)
        {
            std::string file = wi::lua::SGetString(L, 1);
            component->file = file;
        }
        else
        {
            wi::lua::SError(L, "Instance.SetFile(string file) not enough arguments!");
        }
        return 0;
    };
    int Library_Instance_BindLua::GetFile(lua_State* L)
    {
        wi::lua::SSetString(L, component->file);
        return 1;
    };
    int Library_Instance_BindLua::SetEntityName(lua_State* L)
    {
        int argc = wi::lua::SGetArgCount(L);
        if (argc > 0)
        {
            std::string entity_name = wi::lua::SGetString(L, 1);
            component->entity_name = entity_name;
        }
        else
        {
            wi::lua::SError(L, "Instance.SetEntityName(string entity_name) not enough arguments!");
        }
        return 0;
    };
    int Library_Instance_BindLua::GetEntityName(lua_State* L)
    {
        wi::lua::SSetString(L, component->entity_name);
        return 1;
    };
    int Library_Instance_BindLua::SetStrategy(lua_State* L)
    {
        int argc = wi::lua::SGetArgCount(L);
        if (argc > 0)
        {
            Game::Resources::Library::Instance::LOADING_STRATEGY strategy = (Game::Resources::Library::Instance::LOADING_STRATEGY) wi::lua::SGetInt(L, 1);
            component->strategy = strategy;
        }
        else
        {
            wi::lua::SError(L, "Instance.SetStrategy(enum LOADING_STRATEGY) not enough arguments!");
        }
        return 0;
    };
    int Library_Instance_BindLua::GetStrategy(lua_State* L)
    {
        wi::lua::SSetInt(L, component->strategy);
        return 1;
    };
    int Library_Instance_BindLua::SetType(lua_State* L)
    {
        int argc = wi::lua::SGetArgCount(L);
        if (argc > 0)
        {
            Game::Resources::Library::Instance::INSTANCE_TYPE type = (Game::Resources::Library::Instance::INSTANCE_TYPE) wi::lua::SGetInt(L, 1);
            component->type = type;
        }
        else
        {
            wi::lua::SError(L, "Instance.SetType(enum INSTANCE_TYPE) not enough arguments!");
        }
        return 0;
    };
    int Library_Instance_BindLua::GetType(lua_State* L)
    {
        wi::lua::SSetInt(L, component->type);
        return 1;
    };



    const char Library_Disabled_BindLua::className[] = "Instance";
    Luna<Library_Disabled_BindLua>::FunctionType Library_Disabled_BindLua::methods[] = {
        lunamethod(Library_Disabled_BindLua, SetEntity),
        lunamethod(Library_Disabled_BindLua, GetEntity),
        { NULL, NULL }
    };
    Luna<Library_Disabled_BindLua>::PropertyType Library_Disabled_BindLua::properties[] = {
        { NULL, NULL }
    };
    Library_Disabled_BindLua::Library_Disabled_BindLua(lua_State *L)
    {
        owning = true;
        component = new Game::Resources::Library::Disabled;
    }
    Library_Disabled_BindLua::~Library_Disabled_BindLua()
    {
        if(owning){
            delete component;
        }
    }
    int Library_Disabled_BindLua::SetEntity(lua_State* L)
    {
        int argc = wi::lua::SGetArgCount(L);
        if (argc > 0)
        {
            wi::ecs::Entity entity = (wi::ecs::Entity)wi::lua::SGetInt(L, 1);
            component->entity = entity;
        }
        else
        {
            wi::lua::SError(L, "Instance.SetEntity(Entity entity) not enough arguments!");
        }
        return 0;
    };
    int Library_Disabled_BindLua::GetEntity(lua_State* L)
    {
        wi::lua::SSetInt(L, component->entity);
        return 1;
    };



    const char Library_Stream_BindLua::className[] = "Instance";
    Luna<Library_Stream_BindLua>::FunctionType Library_Stream_BindLua::methods[] = {
        lunamethod(Library_Stream_BindLua, SetSubstitute),
        lunamethod(Library_Stream_BindLua, GetSubstitute),
        lunamethod(Library_Stream_BindLua, SetZone),
        lunamethod(Library_Stream_BindLua, GetZone),
        { NULL, NULL }
    };
    Luna<Library_Stream_BindLua>::PropertyType Library_Stream_BindLua::properties[] = {
        { NULL, NULL }
    };
    Library_Stream_BindLua::Library_Stream_BindLua(lua_State *L)
    {
        owning = true;
        component = new Game::Resources::Library::Stream;
    }
    Library_Stream_BindLua::~Library_Stream_BindLua()
    {
        if(owning){
            delete component;
        }
    }
    int Library_Stream_BindLua::SetSubstitute(lua_State* L)
    {
        int argc = wi::lua::SGetArgCount(L);
        if (argc > 0)
        {
            wi::ecs::Entity substitute_object = (wi::ecs::Entity)wi::lua::SGetInt(L, 1);
            component->substitute_object = substitute_object;
        }
        else
        {
            wi::lua::SError(L, "Instance.SetSubstitute(Entity entity) not enough arguments!");
        }
        return 0;
    };
    int Library_Stream_BindLua::GetSubstitute(lua_State* L)
    {
        wi::lua::SSetInt(L, component->substitute_object);
        return 1;
    };
    int Library_Stream_BindLua::SetZone(lua_State* L)
    {
        int argc = wi::lua::SGetArgCount(L);
        if (argc > 0)
        {
            auto stream_zone = Luna<wi::lua::primitive::AABB_BindLua>::check(L, 1);
            component->stream_zone = stream_zone->aabb;
        }
        else
        {
            wi::lua::SError(L, "Instance.SetZone(AABB zone) not enough arguments!");
        }
        return 0;
    };
    int Library_Stream_BindLua::GetZone(lua_State* L)
    {
        Luna<wi::lua::primitive::AABB_BindLua>::push(L, new wi::lua::primitive::AABB_BindLua(component->stream_zone));
        return 1;
    };
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