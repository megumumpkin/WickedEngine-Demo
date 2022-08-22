#include "BindLua.h"
#include "Resources.h"
#include "Resources_BindLua.h"
#include <LUA/lauxlib.h>
#include <LUA/lua.h>
#include <LUA/luaconf.h>
#include <string>
#include <wiArchive.h>
#include <wiBacklog.h>
#include <wiECS.h>
#include <wiLua.h>
#include <wiLuna.h>
#include <wiPrimitive.h>
#include <wiPrimitive_BindLua.h>
#include <wiScene.h>
#include <wiScene_BindLua.h>
#include <wiVector.h>

void Internal_ScriptObjectData_LuaBuildTable(lua_State *L, wi::vector<uint8_t>& properties)
{
    lua_newtable(L);
    auto parc = wi::Archive(properties.data());
    parc.SetReadModeAndResetPos(true);
    bool has_next;
    std::string header;
    uint32_t type;
    do
    {
        parc >> has_next;
        if(has_next)
        {
            parc >> header;
            parc >> type;
            switch(type){
                case LUA_TBOOLEAN:
                {
                    bool temp;
                    parc >> temp;
                    lua_pushboolean(L, temp);
                    break;
                }
                case LUA_TNUMBER:
                {
                    float temp;
                    parc >> temp;
                    lua_pushnumber(L, temp);
                    break;
                }
                case LUA_TSTRING:
                {
                    std::string temp;
                    parc >> temp;
                    lua_pushstring(L, temp.c_str());
                    break;
                }
                default:
                    break;
            }
            lua_setfield(L, -2, header.c_str());
        }
    }
    while(has_next);
}

void Internal_ScriptObjectData_CStoreTable(lua_State *L, wi::vector<uint8_t>& properties)
{
    auto parc = wi::Archive();
    parc.SetReadModeAndResetPos(false);

    lua_pushnil(L);
    while(lua_next(L, 1)){
        parc << true;

        std::string header = wi::lua::SGetString(L, -2);
        parc << header;

        auto type = lua_type(L, -1);
        parc << type;

        switch(type){
            case LUA_TBOOLEAN:
            {
                parc << wi::lua::SGetBool(L, -1);
                break;
            }
            case LUA_TNUMBER:
            {
                parc << wi::lua::SGetDouble(L, -1);
                break;
            }
            case LUA_TSTRING:
            {
                parc << wi::lua::SGetString(L, -1);
                break;
            }
            default:
                break;
        }
        lua_pop(L, 1);
    }

    auto offset = parc.WriteUnknownJumpPosition();
    properties = wi::vector<uint8_t>(parc.GetData(), parc.GetData() + offset);
}

int GetGlobalGameScene(lua_State* L){
    Luna<Game::ScriptBindings::Resources::Scene_BindLua>::push(L, new Game::ScriptBindings::Resources::Scene_BindLua(&Game::Resources::GetScene()));
    return 1;
}



namespace Game::ScriptBindings::Resources{
    void Update()
    {
        // TODO: idk
    }
    void Bind()
    {
        static bool initialized = false;
        if(!initialized)
        {
            initialized = true;

            auto L = wi::lua::GetLuaState();

            wi::lua::RunText(R"(
                INSTANCE_LOAD_DIRECT = 0
                INSTANCE_LOAD_INSTANTIATE = 1
                INSTANCE_LOAD_PRELOAD = 2

                INSTANCE_DEFAULT = 0
                INSTANCE_LIBRARY = 1
            )");

            Luna<Library_Instance_BindLua>::Register(L);
            Luna<Library_Disabled_BindLua>::Register(L);
            Luna<Library_Stream_BindLua>::Register(L);
            Luna<Library_ScriptObject_BindLua>::Register(L);
            Luna<Scene_BindLua>::Register(L);

            wi::lua::RegisterFunc("GetGlobalGameScene", GetGlobalGameScene);
        }
    }

    

    const char Library_Instance_BindLua::className[] = "InstanceComponent";
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
            wi::lua::SError(L, "InstanceComponent::SetFile(string file) not enough arguments!");
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
            wi::lua::SError(L, "InstanceComponent::SetEntityName(string entity_name) not enough arguments!");
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
            wi::lua::SError(L, "InstanceComponent::SetStrategy(enum LOADING_STRATEGY) not enough arguments!");
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
            wi::lua::SError(L, "InstanceComponent::SetType(enum INSTANCE_TYPE) not enough arguments!");
        }
        return 0;
    };
    int Library_Instance_BindLua::GetType(lua_State* L)
    {
        wi::lua::SSetInt(L, component->type);
        return 1;
    };



    const char Library_Disabled_BindLua::className[] = "DisabledComponent";
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
            wi::ecs::Entity entity = (wi::ecs::Entity)wi::lua::SGetLongLong(L, 1);
            component->entity = entity;
        }
        else
        {
            wi::lua::SError(L, "DisabledComponent::SetEntity(Entity entity) not enough arguments!");
        }
        return 0;
    };
    int Library_Disabled_BindLua::GetEntity(lua_State* L)
    {
        wi::lua::SSetLongLong(L, component->entity);
        return 1;
    };



    const char Library_Stream_BindLua::className[] = "StreamComponent";
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
            wi::lua::SError(L, "StreamComponent::SetSubstitute(Entity entity) not enough arguments!");
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
            if(stream_zone)
                component->stream_zone = stream_zone->aabb;
            else
                wi::lua::SError(L, "StreamComponent::SetZone(AABB zone) argument must be AABB type!");
        }
        else
        {
            wi::lua::SError(L, "StreamComponent::SetZone(AABB zone) not enough arguments!");
        }
        return 0;
    };
    int Library_Stream_BindLua::GetZone(lua_State* L)
    {
        Luna<wi::lua::primitive::AABB_BindLua>::push(L, new wi::lua::primitive::AABB_BindLua(component->stream_zone));
        return 1;
    };



    const char Library_ScriptObjectData_BindLua::className[] = "ScriptObjectData";
    Luna<Library_ScriptObjectData_BindLua>::FunctionType Library_ScriptObjectData_BindLua::methods[] = {
        lunamethod(Library_ScriptObjectData_BindLua, SetFile),
        lunamethod(Library_ScriptObjectData_BindLua, GetFile),
        lunamethod(Library_ScriptObjectData_BindLua, SetProperties),
        lunamethod(Library_ScriptObjectData_BindLua, GetProperties),
        { NULL, NULL }
    };
    Luna<Library_ScriptObjectData_BindLua>::PropertyType Library_ScriptObjectData_BindLua::properties[] = {
        { NULL, NULL }
    };
    Library_ScriptObjectData_BindLua::Library_ScriptObjectData_BindLua(lua_State *L)
    {
        owning = true;
        component = new Game::Resources::Library::ScriptObjectData;
    }
    Library_ScriptObjectData_BindLua::~Library_ScriptObjectData_BindLua()
    {
        if(owning){
            delete component;
        }
    }
    int Library_ScriptObjectData_BindLua::SetFile(lua_State* L)
    {
        int argc = wi::lua::SGetArgCount(L);
        if (argc > 0)
        {
            std::string file = wi::lua::SGetString(L, 1);
            component->file = file;
        }
        else
        {
            wi::lua::SError(L, "ScriptObjectData::SetFile(String file) not enough arguments!");
        }
        return 0;
    };
    int Library_ScriptObjectData_BindLua::GetFile(lua_State* L)
    {
        wi::lua::SSetString(L, component->file);
        return 1;
    };
    int Library_ScriptObjectData_BindLua::SetProperties(lua_State* L)
    {
        int argc = wi::lua::SGetArgCount(L);
        if (argc > 0)
        {
            Internal_ScriptObjectData_CStoreTable(L, component->properties);
        }
        else
        {
            wi::lua::SError(L, "ScriptObjectData::SetProperties(table properties) not enough arguments!");
        }
        return 0;
    };
    int Library_ScriptObjectData_BindLua::GetProperties(lua_State* L)
    {
        Internal_ScriptObjectData_LuaBuildTable(L, component->properties);
        
        return 1;
    };



    const char Library_ScriptObject_BindLua::className[] = "ScriptObjectComponent";
    Luna<Library_ScriptObject_BindLua>::FunctionType Library_ScriptObject_BindLua::methods[] = {
        lunamethod(Library_ScriptObject_BindLua, SetScriptData),
        lunamethod(Library_ScriptObject_BindLua, GetScriptData),
        { NULL, NULL }
    };
    Luna<Library_ScriptObject_BindLua>::PropertyType Library_ScriptObject_BindLua::properties[] = {
        { NULL, NULL }
    };
    Library_ScriptObject_BindLua::Library_ScriptObject_BindLua(lua_State *L)
    {
        owning = true;
        component = new Game::Resources::Library::ScriptObject;
    }
    Library_ScriptObject_BindLua::~Library_ScriptObject_BindLua()
    {
        if(owning){
            delete component;
        }
    }
    int Library_ScriptObject_BindLua::SetScriptData(lua_State* L)
    {
        int argc = wi::lua::SGetArgCount(L);
        if (argc > 0)
        {
            bool is_reset = false;

            lua_pushnil(L);
            while(lua_next(L, 1)){
                auto bindlua_data = Luna<Library_ScriptObjectData_BindLua>::lightcheck(L,-1);
                if(bindlua_data)
                {
                    if(!is_reset)
                        component->scripts.clear();
                    component->scripts.push_back(*bindlua_data->component);
                }
                else
                {
                    wi::lua::SError(L, "ScriptObjectComponent::SetScriptData(Table(ScriptObjectComponent) scriptdata) table member must be ScriptObjectComponent type!");
                }
                lua_pop(L,1);
            }
        }
        else
        {
            wi::lua::SError(L, "ScriptObjectComponent::SetScriptData(Table(ScriptObjectComponent) scriptdata) not enough arguments!");
        }
        return 0;
    };
    int Library_ScriptObject_BindLua::GetScriptData(lua_State* L)
    {
        lua_createtable(L, (int)component->scripts.size(), 0);
        int newTable = lua_gettop(L);
        for (size_t i = 0; i < component->scripts.size(); ++i)
        {
            Luna<Library_ScriptObjectData_BindLua>::push(L, new Library_ScriptObjectData_BindLua(&component->scripts[i]));
            lua_rawseti(L, newTable, lua_Integer(i + 1));
        }
        return 1;
    };



    // TODO: Scene_Bindlua
    const char Scene_BindLua::className[] = "GameScene";
    Luna<Scene_BindLua>::FunctionType Scene_BindLua::methods[] = {
        lunamethod(Scene_BindLua, GetWiScene),
        lunamethod(Scene_BindLua, Component_GetInstance),
        lunamethod(Scene_BindLua, Component_GetDisabled),
        lunamethod(Scene_BindLua, Component_GetStream),
        lunamethod(Scene_BindLua, Component_GetScriptObject),
        lunamethod(Scene_BindLua, Entity_Create),
        lunamethod(Scene_BindLua, Component_CreateInstance),
        lunamethod(Scene_BindLua, Entity_SetStreamable),
        lunamethod(Scene_BindLua, Entity_SetScript),
        lunamethod(Scene_BindLua, Entity_Disable),
        lunamethod(Scene_BindLua, Entity_Enable),
        lunamethod(Scene_BindLua, Entity_GetInstanceArray),
        lunamethod(Scene_BindLua, Entity_GetDisabledArray),
        lunamethod(Scene_BindLua, Entity_GetStreamArray),
        lunamethod(Scene_BindLua, Entity_GetScriptObjectArray),
        lunamethod(Scene_BindLua, LoadScene),
        { NULL, NULL }
    };
    Luna<Scene_BindLua>::PropertyType Scene_BindLua::properties[] = {
        { NULL, NULL }
    };
    Scene_BindLua::Scene_BindLua(lua_State *L)
    {
        owning = true;
        scene = new Game::Resources::Scene;
    }
    Scene_BindLua::~Scene_BindLua()
    {
        if(owning){
            delete scene;
        }
    }
    int Scene_BindLua::GetWiScene(lua_State* L)
    {
        Luna<wi::lua::scene::Scene_BindLua>::push(L, new wi::lua::scene::Scene_BindLua(&scene->wiscene));
        return 1;
    };
    int Scene_BindLua::Component_GetInstance(lua_State* L)
    {
        int argc = wi::lua::SGetArgCount(L);
        if (argc > 0)
        {
            wi::ecs::Entity entity = (wi::ecs::Entity)wi::lua::SGetLongLong(L, 1);

            auto component = scene->instances.GetComponent(entity);
            if (component == nullptr)
            {
                return 0;
            }

            Luna<Library_Instance_BindLua>::push(L, new Library_Instance_BindLua(component));
            return 1;
        }
        else
        {
            wi::lua::SError(L, "GameScene::Component_GetInstance(Entity entity) not enough arguments!");
        }
        return 0;
    };
    int Scene_BindLua::Component_GetDisabled(lua_State* L)
    {
        int argc = wi::lua::SGetArgCount(L);
        if (argc > 0)
        {
            wi::ecs::Entity entity = (wi::ecs::Entity)wi::lua::SGetLongLong(L, 1);

            auto component = scene->disabled.GetComponent(entity);
            if (component == nullptr)
            {
                return 0;
            }

            Luna<Library_Disabled_BindLua>::push(L, new Library_Disabled_BindLua(component));
            return 1;
        }
        else
        {
            wi::lua::SError(L, "GameScene::Component_GetDisabled(Entity entity) not enough arguments!");
        }
        return 0;
    };
    int Scene_BindLua::Component_GetStream(lua_State* L)
    {
        int argc = wi::lua::SGetArgCount(L);
        if (argc > 0)
        {
            wi::ecs::Entity entity = (wi::ecs::Entity)wi::lua::SGetLongLong(L, 1);

            auto component = scene->streams.GetComponent(entity);
            if (component == nullptr)
            {
                return 0;
            }

            Luna<Library_Stream_BindLua>::push(L, new Library_Stream_BindLua(component));
            return 1;
        }
        else
        {
            wi::lua::SError(L, "GameScene::Component_GetStream(Entity entity) not enough arguments!");
        }
        return 0;
    };
    int Scene_BindLua::Component_GetScriptObject(lua_State* L)
    {
        int argc = wi::lua::SGetArgCount(L);
        if (argc > 0)
        {
            wi::ecs::Entity entity = (wi::ecs::Entity)wi::lua::SGetLongLong(L, 1);

            auto component = scene->scriptobjects.GetComponent(entity);
            if (component == nullptr)
            {
                return 0;
            }

            Luna<Library_ScriptObject_BindLua>::push(L, new Library_ScriptObject_BindLua(component));
            return 1;
        }
        else
        {
            wi::lua::SError(L, "GameScene::Component_GetScriptObject(Entity entity) not enough arguments!");
        }
        return 0;
    };
    int Scene_BindLua::Entity_Create(lua_State *L){
        wi::lua::SSetInt(L, wi::ecs::CreateEntity());
        return 1;
    }
    int Scene_BindLua::Component_CreateInstance(lua_State *L){
        int argc = wi::lua::SGetArgCount(L);
        if (argc > 0)
        {
            wi::ecs::Entity entity = (wi::ecs::Entity)wi::lua::SGetLongLong(L, 1);

            scene->instances.Create(entity);

            auto& component = scene->instances.Create(entity);
            Luna<Library_Instance_BindLua>::push(L, new Library_Instance_BindLua(&component));
            return 1;
        }
        else
        {
            wi::lua::SError(L, "GameScene::Component_CreateInstance(Entity entity) not enough arguments!");
        }
        return 0;
    }
    int Scene_BindLua::Entity_SetStreamable(lua_State *L){
        int argc = wi::lua::SGetArgCount(L);
        if (argc > 0)
        {
            wi::ecs::Entity entity = (wi::ecs::Entity)wi::lua::SGetLongLong(L, 1);
            bool set = true;
            if(argc >= 2) set = wi::lua::SGetBool(L, 2);
            wi::primitive::AABB bound = wi::primitive::AABB();
            if(argc >= 3){
                auto bound_get = Luna<wi::lua::primitive::AABB_BindLua>::check(L, 3);
                if(bound_get)
                    bound = bound_get->aabb;
            }
            
            scene->SetStreamable(entity, true, bound);
        }
        else
        {
            wi::lua::SError(L, "GameScene::Entity_SetStreamable(Entity entity) not enough arguments!");
        }
        return 0;
    }
    int Scene_BindLua::Entity_SetScript(lua_State *L){
        int argc = wi::lua::SGetArgCount(L);
        if (argc > 0)
        {
            wi::ecs::Entity entity = (wi::ecs::Entity)wi::lua::SGetLongLong(L, 1);
            bool set = true;
            if(argc >= 2) set = wi::lua::SGetBool(L, 2);
            std::string file = "";
            if(argc >= 3) file = wi::lua::SGetString(L, 3);

            scene->SetScript(entity, set, file);
        }
        else
        {
            wi::lua::SError(L, "GameScene::Entity_SetScript(Entity entity) not enough arguments!");
        }
        return 0;
    }
    int Scene_BindLua::Entity_Disable(lua_State *L){
        int argc = wi::lua::SGetArgCount(L);
        if (argc > 0)
        {
            wi::ecs::Entity entity = (wi::ecs::Entity)wi::lua::SGetLongLong(L, 1);
            scene->Entity_Disable(entity);
        }
        else
        {
            wi::lua::SError(L, "GameScene::Entity_SetScript(Entity entity) not enough arguments!");
        }
        return 0;
    }
    int Scene_BindLua::Entity_Enable(lua_State *L){
        int argc = wi::lua::SGetArgCount(L);
        if (argc > 0)
        {
            wi::ecs::Entity entity = (wi::ecs::Entity)wi::lua::SGetLongLong(L, 1);
            scene->Entity_Enable(entity);
        }
        else
        {
            wi::lua::SError(L, "GameScene::Entity_SetScript(Entity entity) not enough arguments!");
        }
        return 0;
    }
    int Scene_BindLua::Entity_GetInstanceArray(lua_State *L){
        lua_createtable(L, (int)scene->instances.GetCount(), 0);
        int newTable = lua_gettop(L);
        for (size_t i = 0; i < scene->instances.GetCount(); ++i)
        {
            Luna<Library_Instance_BindLua>::push(L, new Library_Instance_BindLua(&scene->instances[i]));
            lua_rawseti(L, newTable, lua_Integer(i + 1));
        }
        return 1;
    }
    int Scene_BindLua::Entity_GetDisabledArray(lua_State *L){
        lua_createtable(L, (int)scene->disabled.GetCount(), 0);
        int newTable = lua_gettop(L);
        for (size_t i = 0; i < scene->disabled.GetCount(); ++i)
        {
            Luna<Library_Disabled_BindLua>::push(L, new Library_Disabled_BindLua(&scene->disabled[i]));
            lua_rawseti(L, newTable, lua_Integer(i + 1));
        }
        return 1;
    }
    int Scene_BindLua::Entity_GetStreamArray(lua_State *L){
        lua_createtable(L, (int)scene->streams.GetCount(), 0);
        int newTable = lua_gettop(L);
        for (size_t i = 0; i < scene->streams.GetCount(); ++i)
        {
            Luna<Library_Stream_BindLua>::push(L, new Library_Stream_BindLua(&scene->streams[i]));
            lua_rawseti(L, newTable, lua_Integer(i + 1));
        }
        return 1;
    }
    int Scene_BindLua::Entity_GetScriptObjectArray(lua_State *L){
        lua_createtable(L, (int)scene->scriptobjects.GetCount(), 0);
        int newTable = lua_gettop(L);
        for (size_t i = 0; i < scene->scriptobjects.GetCount(); ++i)
        {
            Luna<Library_ScriptObject_BindLua>::push(L, new Library_ScriptObject_BindLua(&scene->scriptobjects[i]));
            lua_rawseti(L, newTable, lua_Integer(i + 1));
        }
        return 1;
    }
    int Scene_BindLua::LoadScene(lua_State *L){
        int argc = wi::lua::SGetArgCount(L);
        if (argc > 0)
        {
            std::string file = wi::lua::SGetString(L, 1);
            wi::scene::LoadModel(scene->wiscene,file);
        }
        else
        {
            wi::lua::SError(L, "GameScene::LoadScene(String file) not enough arguments!");
        }
        return 0;
        return 0;
    }
}



void Game::Resources::Library::ScriptObjectData::Init(){
    auto L = wi::lua::GetLuaState();
    auto PID = Game::ScriptBindings::script_genPID();

    lua_getglobal(L, "uploadScriptData");
    lua_pushstring(L, std::to_string(PID).c_str());
    Internal_ScriptObjectData_LuaBuildTable(L, properties);
    lua_call(L,2,0);
    
    lua_getglobal(L, "dofile");
    lua_pushstring(L, file.c_str());
    lua_call(L,1,0);
}

void Game::Resources::Library::ScriptObjectData::Unload(){
    auto L = wi::lua::GetLuaState();
    lua_getglobal(L, "killProcessPID");
    lua_pushstring(L, std::to_string(script_pid).c_str());
    lua_call(L,1,0);
}