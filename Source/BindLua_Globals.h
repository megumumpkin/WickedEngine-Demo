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

-- To check if the script has already been initialized
function Script_Initialized(pid)
    local result = true
    if PROCESSES_DATA[pid]._INITIALIZED == 1 then
        result = false
    end
    return result
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

-- Helper functions

-- Deep Copy with Metatable Support
-- From: https://gist.github.com/tylerneylon/81333721109155b2d244
function deepcopy(obj, seen)
    -- Handle non-tables and previously-seen tables.
    if type(obj) ~= 'table' then return obj end
    if seen and seen[obj] then return seen[obj] end
  
    -- New table; mark it as seen and copy recursively.
    local s = seen or {}
    local res = {}
    s[obj] = res
    for k, v in pairs(obj) do res[deepcopy(k, s)] = deepcopy(v, s) end
    return setmetatable(res, getmetatable(obj))
end
)";

