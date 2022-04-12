SetProfilerEnabled(false)

if pcall(getfenv, 4) then
    print("Running the startup script as a library.")
else
    print("Running the startup script as a program.")
end