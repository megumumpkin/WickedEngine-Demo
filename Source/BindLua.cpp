#include "BindLua.h"
#include "BindLua_Globals.h"
#include "Resources_BindLua.h"
#include "WickedEngine.h"
#include <LUA/lua.h>
#include <mutex>
#include <string>
#include <wiBacklog.h>
#include <wiHelper.h>
#include <wiJobSystem.h>
#include <wiLua.h>

#include <filesystem>
#include <wiRenderer.h>
#include <wiUnorderedMap.h>

#if IS_DEV
#include "ImGui/imgui_BindLua.h"
#endif

namespace Game::ScriptBindings{
	static const char* WILUA_ERROR_PREFIX = "[Lua Error] ";

	wi::unordered_map<uint32_t, std::string> scripts;
	wi::unordered_map<std::string, wi::vector<size_t>> scripts_filerefs;

	uint32_t script_genPID(){
		static std::atomic<uint32_t> scriptpid_next{ 0 + 1 };
		return scriptpid_next.fetch_add(1);
	}
	void scripts_genDict(){
		scripts_filerefs.clear();
		for(auto& kval : scripts){
			auto abspath = std::filesystem::canonical(kval.second).string();
			scripts_filerefs[abspath].push_back(kval.first);
		}
	}

	uint32_t script_inject(std::string& script, std::string filename = "", uint32_t PID = 0){
		static const std::string persistent_inject = R"(
			local runProcess = function(func) 
				success, co = Internal_runProcess(script_file(), script_pid(), func)
				return success, co
			end
			if _ENV.PROCESSES_DATA[script_pid()] == nil then
				_ENV.PROCESSES_DATA[script_pid()] = { _INITIALIZED = -1 }
			end
			if _ENV.PROCESSES_DATA[script_pid()]._INITIALIZED < 1 then
				_ENV.PROCESSES_DATA[script_pid()]._INITIALIZED = _ENV.PROCESSES_DATA[script_pid()]._INITIALIZED + 1
			end
			local D = _ENV.PROCESSES_DATA[script_pid()]
			setmetatable(D, {
				__call = function(self, key, value)
					if self[key] == nil then
						self[key] = value
					else
						Internal_SyncSubTable(self[key],value)
					end
				end
			})
		)";

		if(PID == 0){
			PID = script_genPID();
		}

		if(filename != ""){
			scripts[PID] = filename;
			scripts_genDict();
		}

		std::string dynamic_inject = "local function script_file() return \""+ filename +"\" end\n";
		dynamic_inject += "local function script_pid() return \""+std::to_string(PID)+"\" end\n";
		dynamic_inject += "local function script_dir() return \""+wi::helper::GetDirectoryFromPath(filename)+"\" end\n";
		dynamic_inject += persistent_inject;
		script = dynamic_inject + script;

		return PID;
	}

	// Script's system side async jobs
	wi::jobsystem::context script_sys_jobs;
	std::mutex script_sys_mutex;

	wi::unordered_map<std::string, std::string> filedialog_msg;

	int Internal_DoFile(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);

		if (argc > 0)
		{
			bool fixedpath = false;
			uint32_t PID = 0;

			std::string filename = wi::lua::SGetString(L, 1);
			if(argc >= 2) PID = wi::lua::SGetInt(L, 2);

			wi::vector<uint8_t> filedata;

			if (wi::helper::FileRead(filename, filedata))
			{
				std::string command = std::string(filedata.begin(), filedata.end());
				PID = script_inject(command, filename, PID);

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

#if IS_DEV
	int Internal_FileDialog(lua_State *L){
		int argc = wi::lua::SGetArgCount(L);

		if (argc >= 4)
		{
			auto mode = wi::lua::SGetInt(L, 1);
			auto desc = wi::lua::SGetString(L, 2);
			auto types = wi::lua::SGetString(L, 3);
			wi::vector<std::string> type_list;
			if(!types.empty()){
				std::string token;
				std::stringstream ss(types);
				while(std::getline(ss, token, ';')){
					type_list.push_back(token);
					wi::backlog::post(token);
				}
			}
			auto UID = wi::lua::SGetString(L, 4);

			wi::jobsystem::Execute(script_sys_jobs, [=](wi::jobsystem::JobArgs){
				wi::helper::FileDialog({(wi::helper::FileDialogParams::TYPE) mode, desc, type_list}, [&](std::string fileName) {
					wi::eventhandler::Subscribe_Once(wi::eventhandler::EVENT_THREAD_SAFE_POINT, [=](uint64_t userdata){
						std::scoped_lock lock (script_sys_mutex);
						filedialog_msg[UID] = fileName;
					});
				});
			});
			return 0;
		}
		else {
			wi::lua::SError(L, "filedialog(int mode, string desc, table extensions, function(filename)) not enough arguments!");
		}
		return 0;
	}

	int Internal_EditorDevGridDraw(lua_State* L){
		int argc = wi::lua::SGetArgCount(L);
		if(argc > 0)
		{
			bool set = wi::lua::SGetBool(L, 1);
			wi::renderer::SetToDrawGridHelper(set);
		}
		else {
			wi::lua::SError(L, "editor_dev_griddraw(bool set) not enough arguments!");
		}
		return 0;
	}
#endif

    void Init(){
        wi::lua::RunText(ScriptBindings_Globals);
		wi::lua::RegisterFunc("dofile", Internal_DoFile);
#if IS_DEV
		wi::lua::RegisterFunc("Internal_filedialog", Internal_FileDialog);
		wi::lua::RegisterFunc("editor_dev_griddraw", Internal_EditorDevGridDraw);
        ImGui::Bind();
#endif
        Resources::Bind();
    }

	// 
	void Update(float dt){
		// For sending any messaging events
		// filedialog messages
		for(auto& fd_msg_kval : filedialog_msg){
			auto L = wi::lua::GetLuaState();
			lua_getglobal(L, "fd_setresult");
			lua_pushstring(L, fd_msg_kval.first.c_str());
			lua_pushstring(L, fd_msg_kval.second.c_str());
			lua_pushstring(L, wi::helper::toUpper(wi::helper::GetExtensionFromFileName(fd_msg_kval.second)).c_str());
			lua_call(L,3,0);
		}
		filedialog_msg.clear();
	}
}

namespace Game::ScriptBindings::LiveUpdate{
    std::vector<ScriptReloadEvent> events;
    void PushEvent(ScriptReloadEvent event){
        events.push_back(event);
		scripts_genDict();
    }
    void Update(float dt){
        for(auto& event : events){
			if(event.mode == ScriptReloadEvent::RELOAD){
				auto path = event.file;
				if(scripts_filerefs.find(path) != scripts_filerefs.end()){
					auto L = wi::lua::GetLuaState();
					lua_getglobal(L, "killProcessFile");
					lua_pushstring(L, event.file.c_str());
					lua_pushboolean(L, true);
					lua_call(L, 2, 0);

					auto list = scripts_filerefs[path];
					for (auto pid : list){
						lua_getglobal(L, "dofile");
						lua_pushstring(L, event.file.c_str());
						lua_pushinteger(L, pid);
						lua_call(L, 2, 0);
					}
				}
			}
        }
        events.clear();
    }
}