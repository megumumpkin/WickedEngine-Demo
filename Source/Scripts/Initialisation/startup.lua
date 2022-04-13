#!/usr/bin/lua

SetProfilerEnabled(false)

if pcall(getfenv, 4) then
    error("Running the startup script as a library!")
else
    print("Running the startup script as a program.")
end

LoadModel('Assets/Scenes/World.wiscene')