#pragma once
#include <LUA/lua.h>
#include <wiLua.h>
#include <wiLuna.h>
#include "Resources.h"

namespace Game::ScriptBindings::Resources{
    void Update();
    void Bind();
    
    //Library
    class Library_BindLua{
    public:
        static const char className[];
		static Luna<Library_BindLua>::FunctionType methods[];
		static Luna<Library_BindLua>::PropertyType properties[];
        Library_BindLua(lua_State* L) {}
        static void Bind();

        int Load(lua_State* L);
        int Load_Async(lua_State *L);
        int Exist(lua_State* L);
        int GetInstance(lua_State* L);
        int RebuildInfoMap(lua_State* L);

        int GetScene(lua_State* L);

        int Entity_Disable(lua_State* L);
        int Entity_Enable(lua_State* L);
        int Entity_ApplyConfig(lua_State* L);
    };

    class Collection_BindLua{
    public:
        bool owning = false;
        Game::Resources::Library::Collection* data = nullptr;

        static const char className[];
		static Luna<Collection_BindLua>::FunctionType methods[];
		static Luna<Collection_BindLua>::PropertyType properties[];
        
        Collection_BindLua(lua_State* L) {}
        Collection_BindLua(Game::Resources::Library::Collection* data) :data(data) {}
        ~Collection_BindLua();
        
        static void Bind();

        int KeepResources(lua_State* L);
        int FreeResources(lua_State* L);
        int Entities_Wipe(lua_State* L);
    };

    class Instance_BindLua{
    public:
        bool owning = false;
        Game::Resources::Library::Instance* data = nullptr;

        static const char className[];
		static Luna<Instance_BindLua>::FunctionType methods[];
		static Luna<Instance_BindLua>::PropertyType properties[];
        
        Instance_BindLua(lua_State* L) {}
        Instance_BindLua(Game::Resources::Library::Instance* data) :data(data) {}
        ~Instance_BindLua();

        int GetEntities(lua_State *L);
        int Entity_FindByName(lua_State *L);
        int Entity_Remove(lua_State *L);
        int Entities_Wipe(lua_State *L);
        int Empty(lua_State *L);

        static void Bind();
    };
}