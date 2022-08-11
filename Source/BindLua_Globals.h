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

-- filedialog functions
local fd_tid_counter = 0
fd_tid_result = {}
function fd_createTID()
    fd_tid_counter = fd_tid_counter + 1
    return fd_tid_counter
end
function fd_setresult(tid, path, type)
    fd_tid_result[tid] = { path, type }
    signal(tid)
end
function filedialog(mode, desc, ext, func)
    local tid = "FD_"..fd_createTID()
    Internal_filedialog(mode,desc,ext,tid)
    runProcess(function()
        waitSignal(tid)
        func(fd_tid_result[tid][1],fd_tid_result[tid][2])
        fd_tid_result[tid] = nil
    end)
end
)";