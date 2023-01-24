#include "Scene_BindScript.h"
#include <wiScene_BindLua.h>

namespace Game::Scripting
{
    namespace Scene
    {
        // PREFAB SECTION START
        const char Prefab_Bind::className[] = "PrefabComponent";
        Luna<Prefab_Bind>::FunctionType Prefab_Bind::methods[] = {
            lunamethod(Prefab_Bind, FindEntityByName),
            lunamethod(Prefab_Bind, Enable),
            lunamethod(Prefab_Bind, Disable),
            lunamethod(Prefab_Bind, Unload),
            lunamethod(Prefab_Bind, IsLoaded),
            lunamethod(Prefab_Bind, IsDisabled),
            {NULL, NULL}
        };
        Luna<Prefab_Bind>::PropertyType Prefab_Bind::properties[] = {
            lunaproperty(Prefab_Bind, file),
            lunaproperty(Prefab_Bind, copy_mode),
            lunaproperty(Prefab_Bind, stream_mode),
            lunaproperty(Prefab_Bind, stream_distance_multiplier),
            {NULL, NULL}
        };
        int Prefab_Bind::FindEntityByName(lua_State *L)
        {
            int argc = wi::lua::SGetArgCount(L);
            if(argc > 0)
            {
                std::string name = wi::lua::SGetString(L, 1);
                wi::ecs::Entity entity = component->FindEntityByName(name);
                wi::lua::SSetLongLong(L, entity);
                return 1;
            }
            else
            {
                wi::lua::SError(L, "PrefabComponent.FindEntityByName(string name) not enough arguments!");
            }
            return 0;
        }
        int Prefab_Bind::Enable(lua_State *L)
        {
            component->Enable();
            return 0;
        }
        int Prefab_Bind::Disable(lua_State *L)
        {
            component->Disable();
            return 0;
        }
        int Prefab_Bind::Unload(lua_State *L)
        {
            component->Unload();
            return 0;
        }
        int Prefab_Bind::IsLoaded(lua_State *L)
        {
            wi::lua::SSetBool(L, component->loaded);
            return 1;
        }
        int Prefab_Bind::IsDisabled(lua_State *L)
        {
            wi::lua::SSetBool(L, component->disabled);
            return 1;
        }
        // PREFAB SECTION END

        // SCRIPT SECTION START
        const char Script_Bind::className[] = "FrameworkScriptComponent";
        Luna<Script_Bind>::FunctionType Script_Bind::methods[] = {
            {NULL, NULL}
        };
        Luna<Script_Bind>::PropertyType Script_Bind::properties[] = {
            lunaproperty(Script_Bind, file),
            {NULL, NULL}
        };
        // SCRIPT SECTION END

        // SCENE SECTION START
        const char Scene_Bind::className[] = "FrameworkScene";
        Luna<Scene_Bind>::FunctionType Scene_Bind::methods[] = {
            lunamethod(Scene_Bind, GetWiScene),
            lunamethod(Scene_Bind, Component_CreatePrefab),
            lunamethod(Scene_Bind, Component_CreateScript),
            lunamethod(Scene_Bind, Component_GetPrefab),
            lunamethod(Scene_Bind, Component_GetScript),
            lunamethod(Scene_Bind, Component_Remove),
            lunamethod(Scene_Bind, Entity_Exists),
            lunamethod(Scene_Bind, Entity_Disable),
            lunamethod(Scene_Bind, Entity_Enable),
            lunamethod(Scene_Bind, Entity_Clone),
            lunamethod(Scene_Bind, Load),
            {NULL, NULL}
        };
        Luna<Scene_Bind>::PropertyType Scene_Bind::properties[] = {
            lunaproperty(Scene_Bind, stream_transition_speed),
            lunaproperty(Scene_Bind, stream_loader_bounds),
            lunaproperty(Scene_Bind, stream_loader_screen_estate),
            {NULL, NULL}
        };
        int Scene_Bind::GetWiScene(lua_State* L)
        {
            Luna<wi::lua::scene::Scene_BindLua>::push(L, new wi::lua::scene::Scene_BindLua(&(scene->wiscene)));
            return 1;
        }
        int Scene_Bind::Component_CreatePrefab(lua_State *L)
        {
            int argc = wi::lua::SGetArgCount(L);
            if(argc > 0)
            {
                wi::ecs::Entity entity = (wi::ecs::Entity)wi::lua::SGetLongLong(L, 1);
                Game::Scene::Component_Prefab& component = scene->prefabs.Create(entity);
                Luna<Prefab_Bind>::push(L, new Prefab_Bind(&component));
                return 1;
            }
            else
            {
                wi::lua::SError(L, "Scene.Component_CreatePrefab(int entity) not enough arguments!");
            }
            return 0;
        }
        int Scene_Bind::Component_CreateScript(lua_State *L)
        {
            int argc = wi::lua::SGetArgCount(L);
            if(argc > 0)
            {
                wi::ecs::Entity entity = (wi::ecs::Entity)wi::lua::SGetLongLong(L, 1);
                Game::Scene::Component_Script& component = scene->scripts.Create(entity);
                Luna<Script_Bind>::push(L, new Script_Bind(&component));
                return 1;
            }
            else
            {
                wi::lua::SError(L, "Scene.Component_CreateScript(int entity) not enough arguments!");
            }
            return 0;
        }
        int Scene_Bind::Component_GetPrefab(lua_State *L)
        {
            int argc = wi::lua::SGetArgCount(L);
            if(argc > 0)
            {
                wi::ecs::Entity entity = (wi::ecs::Entity)wi::lua::SGetLongLong(L, 1);
                Game::Scene::Component_Prefab* component = scene->prefabs.GetComponent(entity);
                if(component != nullptr)
                {
                    Luna<Prefab_Bind>::push(L, new Prefab_Bind(component));
                    return 1;
                }
            }
            else
            {
                wi::lua::SError(L, "Scene.Component_GetPrefab(int entity) not enough arguments!");
            }
            return 0;
        }
        int Scene_Bind::Component_GetScript(lua_State *L)
        {
            int argc = wi::lua::SGetArgCount(L);
            if(argc > 0)
            {
                wi::ecs::Entity entity = (wi::ecs::Entity)wi::lua::SGetLongLong(L, 1);
                Game::Scene::Component_Script* component = scene->scripts.GetComponent(entity);
                if(component != nullptr)
                {
                    Luna<Script_Bind>::push(L, new Script_Bind(component));
                    return 1;
                }
                
            }
            else
            {
                wi::lua::SError(L, "Scene.Component_GetScript(int entity) not enough arguments!");
            }
            return 0;
        }
        int Scene_Bind::Component_Remove(lua_State *L)
        {
            int argc = wi::lua::SGetArgCount(L);
            if(argc >= 2)
            {
                wi::ecs::Entity entity = (wi::ecs::Entity)wi::lua::SGetLongLong(L, 1);
                std::string componentID = wi::lua::SGetString(L, 2);
                auto find_componentmgr = scene->wiscene.componentLibrary.entries.find(componentID);
                if(find_componentmgr != scene->wiscene.componentLibrary.entries.end())
                {
                    find_componentmgr->second.component_manager->Remove(entity);
                }
            }
            else
            {
                wi::lua::SError(L, "Scene.Component_Remove(int entity, string componentID) not enough arguments!");
            }
            return 0;
        }
        int Scene_Bind::Entity_Exists(lua_State *L)
        {
            int argc = wi::lua::SGetArgCount(L);
            if(argc > 0)
            {
                wi::ecs::Entity entity = (wi::ecs::Entity)wi::lua::SGetLongLong(L, 1);
                wi::lua::SSetBool(L, scene->Entity_Exists(entity));
                return 1;
            }
            else
            {
                wi::lua::SError(L, "Scene.Entity_Exists(int entity) not enough arguments!");
            }
            return 0;
        }
        int Scene_Bind::Entity_Disable(lua_State *L)
        {
            int argc = wi::lua::SGetArgCount(L);
            if(argc > 0)
            {
                wi::ecs::Entity entity = (wi::ecs::Entity)wi::lua::SGetLongLong(L, 1);
                scene->Entity_Disable(entity);
            }
            else
            {
                wi::lua::SError(L, "Scene.Entity_Disable(int entity) not enough arguments!");
            }
            return 0;
        }
        int Scene_Bind::Entity_Enable(lua_State *L)
        {
            int argc = wi::lua::SGetArgCount(L);
            if(argc > 0)
            {
                wi::ecs::Entity entity = (wi::ecs::Entity)wi::lua::SGetLongLong(L, 1);
                scene->Entity_Enable(entity);
            }
            else
            {
                wi::lua::SError(L, "Scene.Entity_Disable(int entity) not enough arguments!");
            }
            return 0;
        }
        int Scene_Bind::Entity_Clone(lua_State *L)
        {
            int argc = wi::lua::SGetArgCount(L);
            if(argc > 0)
            {
                wi::ecs::Entity entity = (wi::ecs::Entity)wi::lua::SGetLongLong(L, 1);
                bool deep_copy = false;
                if(argc > 1)
                    deep_copy = wi::lua::SGetBool(L, 2);
                wi::ecs::EntitySerializer seri;
                wi::ecs::Entity clone_entity = scene->Entity_Clone(entity, seri, deep_copy);
                wi::lua::SSetLongLong(L, clone_entity);
                return 1;
            }
            else
            {
                wi::lua::SError(L, "Scene.Entity_Clone(int entity) not enough arguments!");
            }
            return 0;
        }
        int Scene_Bind::Load(lua_State *L)
        {
            int argc = wi::lua::SGetArgCount(L);
            if(argc > 0)
            {
                std::string scenefile = wi::lua::SGetString(L, 1);
                scene->Load(scenefile);
            }
            else
            {
                wi::lua::SError(L, "Scene.Load(string scenefile) not enough arguments!");
            }
            return 0;
        }
        // SCENE SECTION END

        int GetScene(lua_State* L)
        {
            Luna<Scene_Bind>::push(L, new Scene_Bind(Game::GetScene()));
            return 1;
        }

        static const std::string value_bindings = R"(
            PrefabComponent_CopyMode = {
                SHALLOW_COPY = 0,
                DEEP_COPY = 1,
                LIBRARY = 2,
            }
            PrefabComponent_StreamMode = {
                DIRECT = 0,
                DISTANCE = 1,
                SCREEN_ESTATE = 2,
                MANUAL = 3,
            }
        )";

        void Bind()
        {
            static bool initialized = false;
            if(!initialized)
            {
                lua_State* L = wi::lua::GetLuaState();

                Luna<Prefab_Bind>::Register(L);
                Luna<Script_Bind>::Register(L);
                Luna<Scene_Bind>::Register(L);
                
                wi::lua::RegisterFunc("GetScene", GetScene);
                wi::lua::RunText(value_bindings);
            }
        }
    }
}