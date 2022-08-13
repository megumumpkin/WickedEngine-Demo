#include "BindLua.h"
#include "Resources.h"
#include "Resources_BindLua.h"
#include <memory>
#include <wiArchive.h>
#include <wiLua.h>
#include <wiScene_BindLua.h>
#include <set>
#include <filesystem>

namespace Game::ScriptBindings::Resources{

    std::set<std::string> async_callbacks;
    
    void Bind(){
        Library_BindLua::Bind();

        wi::lua::RunText("Resources = Resources or {}");
        wi::lua::RunText("Resources.SourcePath = SourcePath or {}");
        wi::lua::RunText("Resources.SourcePath.SHADER = \""+Game::Resources::SourcePath::SHADER+"\"");
        wi::lua::RunText("Resources.SourcePath.INTERFACE = \""+Game::Resources::SourcePath::INTERFACE+"\"");
        wi::lua::RunText("Resources.SourcePath.LOCALE = \""+Game::Resources::SourcePath::LOCALE+"\"");
        wi::lua::RunText("Resources.SourcePath.ASSET = \""+Game::Resources::SourcePath::ASSET+"\"");
        wi::lua::RunText("Resources.Library = Library()");
        wi::lua::RunText("Resources.LibraryFlags = LibraryFlags or {}");
        wi::lua::RunText("Resources.LibraryFlags.LOADING_STRATEGY_SHALLOW = "+std::to_string(Game::Resources::Library::LOADING_STRATEGY_SHALLOW));
        wi::lua::RunText("Resources.LibraryFlags.LOADING_FULL = "+std::to_string(Game::Resources::Library::LOADING_STRATEGY_FULL));
        wi::lua::RunText("Resources.LibraryFlags.LOADING_STRATEGY_ALWAYS_INSTANCE = "+std::to_string(Game::Resources::Library::LOADING_STRATEGY_ALWAYS_INSTANCE));
        wi::lua::RunText("Resources.LibraryFlags.LOADING_FLAGS_NONE = "+std::to_string(Game::Resources::Library::LOADING_FLAGS_NONE));
        wi::lua::RunText("Resources.LibraryFlags.LOADING_FLAG_RECURSIVE = "+std::to_string(Game::Resources::Library::LOADING_FLAG_RECURSIVE));
        wi::lua::RunText("Resources.LibraryFlags.COMPONENT_FILTER_USE_BASE = "+std::to_string(Game::Resources::Library::COMPONENT_FILTER_USE_BASE));
        wi::lua::RunText("Resources.LibraryFlags.COMPONENT_FILTER_USE_LAYER_TRANSFORM = "+std::to_string(Game::Resources::Library::COMPONENT_FILTER_USE_LAYER_TRANSFORM));
        wi::lua::RunText("Resources.LibraryFlags.COMPONENT_FILTER_REMOVE_CONFIG_AFTER = "+std::to_string(Game::Resources::Library::COMPONENT_FILTER_REMOVE_CONFIG_AFTER));

        Game::ScriptBindings::Register_AsyncCallback("asyncload",[=](std::string tid, std::shared_ptr<wi::Archive> archive_ptr){
		    auto& archive = *archive_ptr;
            uint32_t instance_uuid;
			archive >> instance_uuid;
			auto L = wi::lua::GetLuaState();
			lua_getglobal(L, "asyncload_setresult");
			lua_pushstring(L, tid.c_str());
			lua_pushnumber(L, instance_uuid);
			lua_call(L,3,0);
		});
    }

    const char Library_BindLua::className[] = "Library";
    Luna<Library_BindLua>::FunctionType Library_BindLua::methods[] = {
        lunamethod(Library_BindLua, Load),
        lunamethod(Library_BindLua, Load_Async),
        lunamethod(Library_BindLua, Exist),
        lunamethod(Library_BindLua, GetInstance),
        lunamethod(Library_BindLua, RebuildInfoMap),
        lunamethod(Library_BindLua, GetScene),
        lunamethod(Library_BindLua, Entity_Disable),
        lunamethod(Library_BindLua, Entity_Enable),
        lunamethod(Library_BindLua, Entity_ApplyConfig),
        { NULL, NULL }
    };
    Luna<Library_BindLua>::PropertyType Library_BindLua::properties[] = {
        { NULL, NULL }
    };
    void Library_BindLua::Bind(){
		static bool initialized = false;
		if (!initialized)
		{
			initialized = true;
			Luna<Library_BindLua>::Register(wi::lua::GetLuaState());

            Collection_BindLua::Bind();
            Instance_BindLua::Bind();
		}
    }
    int Library_BindLua::Load(lua_State *L){
        int argc = wi::lua::SGetArgCount(L);
        if(argc > 0){
            std::string filepath = wi::lua::SGetString(L, 1);
            std::string subresource = "";
            if(argc > 1) subresource = wi::lua::SGetString(L, 2);
            wi::ecs::Entity root = wi::ecs::INVALID_ENTITY;
            if(argc > 2) root = wi::lua::SGetInt(L, 3);
            uint32_t loadingstrategy = 0;
            if(argc > 3) loadingstrategy = wi::lua::SGetInt(L, 4);
            uint32_t loadingflags = 0;
            if(argc > 4) loadingflags = wi::lua::SGetInt(L, 5);
            uint32_t result = Game::Resources::Library::Load(filepath, subresource, root, loadingstrategy, loadingflags);
            wi::lua::SSetInt(L, result);

            return 1;
        }else{
            wi::lua::SError(L, "Load(string filepath) not enough arguments!");
        }
        return 0;
    }
    uint32_t _internal_LoadAsync_genPID(){
        static std::atomic<uint32_t> LoadAsyncTID_next{ 0 + 1 };
		return LoadAsyncTID_next.fetch_add(1);
    };
    int Library_BindLua::Load_Async(lua_State *L){
        int argc = wi::lua::SGetArgCount(L);
        if(argc > 0){
            std::string filepath = wi::lua::SGetString(L, 1);
            std::string TID = "ASYNCLOAD_"+std::to_string(_internal_LoadAsync_genPID());
            std::string subresource = "";
            if(argc >= 3) subresource = wi::lua::SGetString(L, 3);
            wi::ecs::Entity root = wi::ecs::INVALID_ENTITY;
            if(argc >= 4) root = wi::lua::SGetInt(L, 4);
            uint32_t loadingstrategy = 0;
            if(argc >= 5) loadingstrategy = wi::lua::SGetInt(L, 5);
            uint32_t loadingflags = 0;
            if(argc >= 6) loadingflags = wi::lua::SGetInt(L, 6);
            Game::Resources::Library::Load_Async(filepath, [=](uint32_t instance_uuid){
                std::shared_ptr<wi::Archive> callback_data_ptr = std::make_shared<wi::Archive>();
                auto& callback_data = *callback_data_ptr;
                callback_data.SetReadModeAndResetPos(false);
                callback_data << "asyncload";
                callback_data << instance_uuid;
                Push_AsyncCallback(TID, callback_data_ptr);
            }, subresource, root, loadingstrategy, loadingflags);
            return 1;
        }else{
            wi::lua::SError(L, "Load_Async(string filepath, func callback) not enough arguments!");
        }
        return 0;
    }
    int Library_BindLua::Exist(lua_State *L){
        int argc = wi::lua::SGetArgCount(L);
        if(argc > 0){
            std::string filepath = wi::lua::SGetString(L, 1);
            std::string subresource = "";
            if(argc > 1) subresource = wi::lua::SGetString(L, 2);
            bool result = Game::Resources::Library::Exist(filepath, subresource);
            wi::lua::SSetBool(L, result);

            return 1;
        }else{
            wi::lua::SError(L, "Exist(string filepath) not enough arguments!");
        }
        return 0;
    }
    int Library_BindLua::GetInstance(lua_State* L){
        int argc = wi::lua::SGetArgCount(L);
        if(argc > 0){
            uint32_t uuid = wi::lua::SGetInt(L, 1);
            auto result = Game::Resources::Library::GetInstance(uuid);
            if(result == nullptr) return 0;
            Luna<Instance_BindLua>::push(L,new Instance_BindLua(result));

            return 1;
        }else{
            wi::lua::SError(L, "GetInstance(int uuid) not enough arguments!");
        }
        return 0;
    }
    int Library_BindLua::RebuildInfoMap(lua_State* L){
        Game::Resources::Library::RebuildInfoMap();
        return 1;
    }
    int Library_BindLua::GetScene(lua_State* L){
        Luna<wi::lua::scene::Scene_BindLua>::push(L, new wi::lua::scene::Scene_BindLua(Game::Resources::Library::GetLibraryData()->scene));
        return 1;
    }
    int Library_BindLua::Entity_Disable(lua_State* L){
        int argc = wi::lua::SGetArgCount(L);
        if(argc > 0){
            uint32_t entity = wi::lua::SGetInt(L, 1);
            Game::Resources::Library::Entity_Disable(entity);

            return 1;
        }else{
            wi::lua::SError(L, "Entity_Disable(int entity) not enough arguments!");
        }
        return 0;
    }
    int Library_BindLua::Entity_Enable(lua_State* L){
        int argc = wi::lua::SGetArgCount(L);
        if(argc > 0){
            uint32_t entity = wi::lua::SGetInt(L, 1);
            Game::Resources::Library::Entity_Enable(entity);

            return 1;
        }else{
            wi::lua::SError(L, "Entity_Enable(int entity) not enough arguments!");
        }
        return 0;
    }
    int Library_BindLua::Entity_ApplyConfig(lua_State *L){
        int argc = wi::lua::SGetArgCount(L);
        if(argc > 2){
            uint32_t target = wi::lua::SGetInt(L, 1);
            uint32_t config = wi::lua::SGetInt(L, 2);
            uint32_t componentfilter = wi::lua::SGetInt(L, 3);
            Game::Resources::Library::Entity_ApplyConfig(target, config, (Game::Resources::Library::COMPONENT_FILTER)componentfilter);

            return 1;
        }else{
            wi::lua::SError(L, "Entity_ApplyConfig(int target, int config, int componentfilter) not enough arguments!");
        }
        return 0;
    }

    const char Collection_BindLua::className[] = "Collection";
    Luna<Collection_BindLua>::FunctionType Collection_BindLua::methods[] = {
        lunamethod(Collection_BindLua, KeepResources),
        lunamethod(Collection_BindLua, FreeResources),
        lunamethod(Collection_BindLua, Entities_Wipe),
        { NULL, NULL }
    };
    Luna<Collection_BindLua>::PropertyType Collection_BindLua::properties[] = {
        { NULL, NULL }
    };
    Collection_BindLua::~Collection_BindLua(){
        if(owning) delete data;
    }
    void Collection_BindLua::Bind(){
		static bool initialized = false;
		if (!initialized)
		{
			initialized = true;
			Luna<Collection_BindLua>::Register(wi::lua::GetLuaState());
		}
    }
    int Collection_BindLua::KeepResources(lua_State *L){
        data->KeepResources();
        return 1;
    }
    int Collection_BindLua::FreeResources(lua_State *L){
        data->FreeResources();
        return 1;
    }
    int Collection_BindLua::Entities_Wipe(lua_State *L){
        data->Entities_Wipe();
        return 1;
    }

    const char Instance_BindLua::className[] = "Instance";
    Luna<Instance_BindLua>::FunctionType Instance_BindLua::methods[] = {
        lunamethod(Instance_BindLua, GetEntities),
        lunamethod(Instance_BindLua, Entity_FindByName),
        lunamethod(Instance_BindLua, Entity_Remove),
        lunamethod(Instance_BindLua, Entities_Wipe),
        lunamethod(Instance_BindLua, Empty),
        { NULL, NULL }
    };
    Luna<Instance_BindLua>::PropertyType Instance_BindLua::properties[] = {
        { NULL, NULL }
    };
    Instance_BindLua::~Instance_BindLua(){
        if(owning) delete data;
    }
    void Instance_BindLua::Bind(){
		static bool initialized = false;
		if (!initialized)
		{
			initialized = true;
			Luna<Instance_BindLua>::Register(wi::lua::GetLuaState());
		}
    }
    int Instance_BindLua::GetEntities(lua_State *L){
        lua_createtable(L, (int)data->entities.size(), 0);
        int newTable = lua_gettop(L);
        auto entities = data->GetEntities();
        for(int i = 0; i < entities.size(); ++i){
            wi::lua::SSetInt(L, entities[i]);
		    lua_rawseti(L, newTable, lua_Integer(i + 1));
        }
        return 1;
    }
    int Instance_BindLua::Entity_FindByName(lua_State *L){
        int argc = wi::lua::SGetArgCount(L);
        if(argc > 0){
            std::string name = wi::lua::SGetString(L, 1);
            auto result = data->Entity_FindByName(name);
            wi::lua::SSetInt(L, result);

            return 1;
        }else{
            wi::lua::SError(L, "Entity_FindByName(string name) not enough arguments!");
        }
        
        return 0;
    }
    int Instance_BindLua::Entity_Remove(lua_State *L){
        int argc = wi::lua::SGetArgCount(L);
        if(argc > 0){
            wi::ecs::Entity entity = wi::lua::SGetInt(L, 1);
            data->Entity_Remove(entity);
            return 1;
        }else{
            wi::lua::SError(L, "Entity_Remove(int entity) not enough arguments!");
        }
        return 1;
    }
    int Instance_BindLua::Entities_Wipe(lua_State *L){
        data->Entities_Wipe();
        return 1;
    }
    int Instance_BindLua::Empty(lua_State *L){
        bool result = data->Empty();
        wi::lua::SSetBool(L, result);
        return 1;
    }
}