#pragma once
#include <LUA/lua.h>
#include <wiLua.h>
#include <wiLuna.h>
#include "Resources.h"

namespace Game::ScriptBindings::Resources
{
    void Update();
    void Bind();

    class Library_Instance_BindLua
    {
    public:
        bool owning = false;
		Game::Resources::Library::Instance* component = nullptr;

		static const char className[];
		static Luna<Library_Instance_BindLua>::FunctionType methods[];
		static Luna<Library_Instance_BindLua>::PropertyType properties[];

		Library_Instance_BindLua(Game::Resources::Library::Instance* component) :component(component) {}
		Library_Instance_BindLua(lua_State *L);
		~Library_Instance_BindLua();

        int SetFile(lua_State* L);
		int GetFile(lua_State* L);
        int SetEntityName(lua_State* L);
		int GetEntityName(lua_State* L);
        int SetStrategy(lua_State* L);
		int GetStrategy(lua_State* L);    
        int SetType(lua_State* L);
		int GetType(lua_State* L);
    };

    class Library_Disabled_BindLua
    {
    public:
        bool owning = false;
		Game::Resources::Library::Disabled* component = nullptr;

		static const char className[];
		static Luna<Library_Disabled_BindLua>::FunctionType methods[];
		static Luna<Library_Disabled_BindLua>::PropertyType properties[];

		Library_Disabled_BindLua(Game::Resources::Library::Disabled* component) :component(component) {}
		Library_Disabled_BindLua(lua_State *L);
		~Library_Disabled_BindLua();

        int SetEntity(lua_State* L);
		int GetEntity(lua_State* L);
    };

    class Library_Stream_BindLua
    {
    public:
        bool owning = false;
		Game::Resources::Library::Stream* component = nullptr;

		static const char className[];
		static Luna<Library_Stream_BindLua>::FunctionType methods[];
		static Luna<Library_Stream_BindLua>::PropertyType properties[];

		Library_Stream_BindLua(Game::Resources::Library::Stream* component) :component(component) {}
		Library_Stream_BindLua(lua_State *L);
		~Library_Stream_BindLua();

        int SetSubstitute(lua_State* L);
		int GetSubstitute(lua_State* L);
        int SetZone(lua_State* L);
		int GetZone(lua_State* L);
    };

    class Scene_BindLua
    {
    public:
        bool owning = false;
		Game::Resources::Scene* scene = nullptr;

		static const char className[];
		static Luna<Scene_BindLua>::FunctionType methods[];
		static Luna<Scene_BindLua>::PropertyType properties[];

		Scene_BindLua(Game::Resources::Scene* scene) :scene(scene) {}
		Scene_BindLua(lua_State *L);
		~Scene_BindLua();

		int GetWiScene(lua_State* L);

		int Component_GetInstance(lua_State* L);
		int Component_GetDisabled(lua_State* L);
		int Component_GetStream(lua_State* L);

		int Component_GetInstanceArray(lua_State* L);
		int Component_GetDisabledArray(lua_State* L);
		int Component_GetStreamArray(lua_State* L);

		int Entity_Create(lua_State* L);
        int Component_CreateInstance(lua_State* L);
        int Entity_SetStreamable(lua_State* L);
        
        int Entity_Disable(lua_State* L);
        int Entity_Enable(lua_State* L);

		int Entity_GetInstanceArray(lua_State* L);
		int Entity_GetDisabledArray(lua_State* L);
		int Entity_GetStreamArray(lua_State* L);

		int Entity_GetMeshArray(lua_State* L);

		int LoadScene(lua_State* L);
    };
}