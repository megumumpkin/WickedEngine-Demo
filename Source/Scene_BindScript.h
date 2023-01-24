#pragma once
#include "Scene.h"
#include <wiMath_BindLua.h>

namespace Game::Scripting
{
    namespace Scene
    {
        class Prefab_Bind
        {
        private:
            std::unique_ptr<Game::Scene::Component_Prefab> owning;
        public:
            Game::Scene::Component_Prefab* component;

            static const char className[];
            static Luna<Prefab_Bind>::FunctionType methods[];
            static Luna<Prefab_Bind>::PropertyType properties[];

            inline void BuildBindings()
            {
                file = wi::lua::StringProperty(&component->file);
                copy_mode = wi::lua::IntProperty((int*)&component->copy_mode);
                stream_mode = wi::lua::IntProperty((int*)&component->stream_mode);
                stream_distance_multiplier = wi::lua::FloatProperty(&component->stream_distance_multiplier);
            }

            Prefab_Bind(Game::Scene::Component_Prefab* component) :component(component) { BuildBindings(); }
            Prefab_Bind(lua_State* L)
            {
                owning = std::make_unique<Game::Scene::Component_Prefab>();
                this->component = owning.get();
                BuildBindings();
            }

            wi::lua::StringProperty file;
            PropertyFunction(file)
            wi::lua::IntProperty copy_mode;
            PropertyFunction(copy_mode)
            wi::lua::IntProperty stream_mode;
            PropertyFunction(stream_mode)
            wi::lua::FloatProperty stream_distance_multiplier;
            PropertyFunction(stream_distance_multiplier)

            int FindEntityByName(lua_State *L);
            int Enable(lua_State *L);
            int Disable(lua_State *L);
            int Unload(lua_State *L);
            int IsLoaded(lua_State *L);
            int IsDisabled(lua_State *L);
        };

        class Script_Bind
        {
        private:
            std::unique_ptr<Game::Scene::Component_Script> owning;
        public:
            Game::Scene::Component_Script* component;

            static const char className[];
            static Luna<Script_Bind>::FunctionType methods[];
            static Luna<Script_Bind>::PropertyType properties[];

            inline void BuildBindings()
            {
                file = wi::lua::StringProperty(&component->file);
            }

            Script_Bind(Game::Scene::Component_Script* component) :component(component) { BuildBindings(); }
            Script_Bind(lua_State* L)
            {
                owning = std::make_unique<Game::Scene::Component_Script>();
                this->component = owning.get();
                BuildBindings();
            }

            wi::lua::StringProperty file;
            PropertyFunction(file)
        };

        class Scene_Bind
        {
        private:
            std::unique_ptr<Game::Scene> owning;
        public:
            Game::Scene* scene;

            static const char className[];
            static Luna<Scene_Bind>::FunctionType methods[];
            static Luna<Scene_Bind>::PropertyType properties[];

            inline void BuildBindings()
            {
                stream_transition_speed = wi::lua::FloatProperty(&scene->stream_transition_speed);
                stream_loader_bounds = wi::lua::VectorProperty(&scene->stream_loader_bounds);
                stream_loader_screen_estate = wi::lua::FloatProperty(&scene->stream_loader_screen_estate);
            }

            Scene_Bind(Game::Scene* scene) :scene(scene) { BuildBindings(); }
            Scene_Bind(lua_State* L)
            {
                owning = std::make_unique<Game::Scene>();
                this->scene = owning.get();
                BuildBindings();
            }

            wi::lua::FloatProperty stream_transition_speed;
            PropertyFunction(stream_transition_speed)
            wi::lua::VectorProperty stream_loader_bounds;
            PropertyFunction(stream_loader_bounds)
            wi::lua::FloatProperty stream_loader_screen_estate;
            PropertyFunction(stream_loader_screen_estate)

            int GetWiScene(lua_State* L);

            int Component_CreatePrefab(lua_State* L);
            int Component_CreateScript(lua_State* L);

            int Component_GetPrefab(lua_State* L);
            int Component_GetScript(lua_State* L);

            int Component_Remove(lua_State* L);

            int Entity_Exists(lua_State* L);
            int Entity_Disable(lua_State* L);
            int Entity_Enable(lua_State* L);
            int Entity_Clone(lua_State* L);

            int Load(lua_State* L);
        };

        int GetScene(lua_State*L);
        
        void Bind();
    }
}