#pragma once
#include "stdafx.h"

namespace Game::ScriptBindings::Resources
{
    void Update();
    void Bind();

    class Library_Instance_BindLua
    {
	private:
		std::unique_ptr<Game::Resources::Library::Instance> owning;
    public:
		Game::Resources::Library::Instance* component = nullptr;

		static const char className[];
		static Luna<Library_Instance_BindLua>::FunctionType methods[];
		static Luna<Library_Instance_BindLua>::PropertyType properties[];

		inline void BuildBindings()
		{
			File = wi::lua::StringProperty(&component->file);
			EntityName = wi::lua::StringProperty(&component->entity_name);
			Strategy = wi::lua::IntProperty(reinterpret_cast<int*>(&component->strategy));
			Type = wi::lua::IntProperty(reinterpret_cast<int*>(&component->type));
			Lock = wi::lua::BoolProperty(&component->lock);
		}

		Library_Instance_BindLua(Game::Resources::Library::Instance* component) :component(component)
		{ 
			BuildBindings(); 
		}
		Library_Instance_BindLua(lua_State *L)
		{
			owning = std::make_unique<Game::Resources::Library::Instance>();
			component = owning.get();
			BuildBindings();
		}

		wi::lua::StringProperty File;
		wi::lua::StringProperty EntityName;
		wi::lua::IntProperty Strategy;
		wi::lua::IntProperty Type;
		wi::lua::BoolProperty Lock;

		PropertyFunction(File)
		PropertyFunction(EntityName)
		PropertyFunction(Strategy)
		PropertyFunction(Type)
		PropertyFunction(Lock)

		int Init(lua_State* L);
		int Unload(lua_State* L);
    };

    class Library_Disabled_BindLua
    {
	private:
		std::unique_ptr<Game::Resources::Library::Disabled> owning;
    public:
		Game::Resources::Library::Disabled* component = nullptr;

		static const char className[];
		static Luna<Library_Disabled_BindLua>::FunctionType methods[];
		static Luna<Library_Disabled_BindLua>::PropertyType properties[];

		inline void BuildBindings(){}

		Library_Disabled_BindLua(Game::Resources::Library::Disabled* component) :component(component)
		{ 
			BuildBindings(); 
		}
		Library_Disabled_BindLua(lua_State *L)
		{
			owning = std::make_unique<Game::Resources::Library::Disabled>();
			component = owning.get();
			BuildBindings();
		}
    };

    class Library_Stream_BindLua
    {
	private:
		std::unique_ptr<Game::Resources::Library::Stream> owning;
    public:
		Game::Resources::Library::Stream* component = nullptr;

		static const char className[];
		static Luna<Library_Stream_BindLua>::FunctionType methods[];
		static Luna<Library_Stream_BindLua>::PropertyType properties[];

		inline void BuildBindings()
		{
			ExternalSubstitute = wi::lua::StringProperty(&component->external_substitute_object);
		}

		Library_Stream_BindLua(Game::Resources::Library::Stream* component) :component(component)
		{
			BuildBindings();
		}
		Library_Stream_BindLua(lua_State *L)
		{
			owning = std::make_unique<Game::Resources::Library::Stream>();
			component = owning.get();
			BuildBindings();
		}

		wi::lua::StringProperty ExternalSubstitute;

		PropertyFunction(ExternalSubstitute)

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

		int Component_RemoveInstance(lua_State* L);

		int Entity_GetInstanceArray(lua_State* L);
		int Entity_GetDisabledArray(lua_State* L);
		int Entity_GetStreamArray(lua_State* L);

		int Entity_GetMeshArray(lua_State* L);

		int GetStreamBoundary(lua_State* L);
		int SetStreamBoundary(lua_State* L);

		int LoadScene(lua_State* L);

		int Clear(lua_State* L);
    };
}