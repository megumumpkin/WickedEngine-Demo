#include "BindLua.h"
#include "Resources_BindLua.h"
#include <filesystem>
#include <string>
#include <wiLua.h>

namespace Game::ScriptBindings::Resources::SceneObject{
    wi::unordered_map<uint32_t, std::string> sco_list;
    wi::unordered_map<std::string, wi::vector<size_t>> sco_filerefs;

    uint32_t sco_inject(std::string& script, std::string filename = "", uint32_t PID = 0){
        // Requirement: have a local table named SCENE_OBJECT
        /* Scene object lua script structure
        D.SCENE_OBJECT = {
            library = {
                main = {"file_name", "sub_entity", "root_entity", loading_strategy, loading_flags, lua_data_skip, "stream_bound", instance_uuid}
                scene_a = {"file_name", "sub_entity", "root_entity", loading_strategy, loading_flags, lua_data, "stream_bound", instance_uuid}
            },
            library_config = {
                scene_a = {"object_name", "config_object_name", config_flags}
            }
        }
        */
        static const std::string sco_functions = R"(
            local function waitAllLoaded()
                waitSignal(sco_id())
            end
            local function waitMainLoaded()
                waitSignal(sco_id() .. "_main")
            end
            local function waitLoaded(instance_name)
                waitSignal(sco_id() .. "_" .. instance_name)
            end
            local function libraryGetInstanceID(instance_name)
                return D.SCENE_OBJECT.library[instance_name][8]
            end
        )";
        
        static const std::string scene_loader_inject = R"(
			runProcess(function()
                -- Stream all listed data, load scene data if it is root and then load .sco scripts when it is not root
                for instance_name, instance_data in pairs(D.SCENE_OBJECT) do
                    local file_name = instance_data[1]
                    local sub_entity = instance_data[2]
                    local root_entity = instance_data[3]
                    local loading_strategy = instance_data[4]
                    local loading_flags = instance_data[5]
                    local lua_data = instance_data[6]
                    local stream_bound = instance_data[7]
                    if instance_name == "main" then
                        if instance_data[8] == 0 then
                            Resources.Library.Load_Async(file_name, function(instance_uuid)
                                instance_data[8] = instance_uuid
                                signal(sco_id() .. "_" .. instance_name)
                            end, sub_entity, root_entity, loading_strategy, loading_flags)
                        end
                    else
                        
                    end
                end
                -- Wait until all has been signaled to signal the waitAllLoaded
                for instance_name, _ in pairs(D.SCENE_OBJECT) do
                    waitSignal(sco_id() .. "_" .. instance_name)
                end
                signal(sco_id())
            end)
		)";

        PID = script_inject(script, filename, PID);
        std::string dynamic_inject_top = "local function sco_id() return \"SCO_"+std::to_string(PID)+"\" end";
        dynamic_inject_top += sco_functions;
        script = dynamic_inject_top + script;
        script += scene_loader_inject;

		return PID;
    }

    void Bind(){
        wi::lua::RunText(R"(
            function AppendScriptData(pid, data)
                PROCESSES_DATA[script_pid()]
            end
        )");
    }
}