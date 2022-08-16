#pragma once

static const char* ScriptBindings_Globals = R"(
-----------------------------------------------
-- Framework Lua Globals
-----------------------------------------------

-- User data sync handling
function Internal_SyncSubTable(storage,source)
    if type(source) == type(storage) then
        if type(source) == "table" then
            for key, value in pairs(source) do
                if storage[key] ~= nil then
                    Internal_SyncSubTable(storage[key],source[key])
                else
                    storage[key] = source[key]
                end
            end
        end
    else
        storage = source
    end
end

Async_Callback_Data = {}
function async_callback_listen(tid, func)
    runProcess(function()
        waitSignal(tid)
        func(Async_Callback_Data[tid])
        Async_Callback_Data[tid] = nil
    end)
end
function async_callback_setdata(tid, data)
    Async_Callback_Data[tid] = data
    signal(tid)
end

function uploadScriptData(pid, data)
    Internal_SyncSubTable(PROCESSES_DATA[pid],data_table)
end
)";

