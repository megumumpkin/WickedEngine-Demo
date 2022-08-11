cmake_minimum_required(VERSION 3.8)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

project(Game)

add_subdirectory(Library)

set(WickedEngine_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Library/WickedEngine/build/cmake")
find_package(WickedEngine REQUIRED)

set (SOURCE_FILES
	Source/Resources.h
	Source/Resources.cpp
	Source/Resources_BindLua.h
	Source/Resources_BindLua.cpp
	Source/RenderPipeline.h
	Source/RenderPipeline.cpp
	Source/BindLua.h
	Source/BindLua_Globals.h
	Source/BindLua.cpp
	Source/LiveUpdate.h
	Source/LiveUpdate.cpp
    Source/Game.h
    Source/Game.cpp
)

set (DEV_FILES
	${SOURCE_FILES}
    Source/ImGui/imconfig.h
	Source/ImGui/imgui.cpp
	Source/ImGui/imgui.h
	Source/ImGui/imgui_demo.cpp
	Source/ImGui/imgui_draw.cpp
	Source/ImGui/imgui_internal.h
	Source/ImGui/imgui_tables.cpp
	Source/ImGui/imgui_widgets.cpp
	Source/ImGui/imstb_rectpack.h
	Source/ImGui/imstb_textedit.h
	Source/ImGui/imstb_truetype.h
	Source/ImGui/imgui_impl_sdl.cpp
	Source/ImGui/imgui_impl_sdl.h

	Source/ImGui/FA6_UI_Icons.h

	Source/ImGui/imgui_iterator.h
	Source/ImGui/imgui_BindLua.h
	Source/ImGui/imgui_BindLua.cpp

	Source/ImGui/Widgets/ImGuizmo.h
	Source/ImGui/Widgets/ImGuizmo.cpp

	Source/Editor.h
	Source/Editor.cpp
)

if (WIN32)
	list (APPEND SOURCE_FILES
		Source/main_Windows.cpp
		Source/main_Windows.h
		Source/Game.rc
	)

	add_executable(Game WIN32 ${SOURCE_FILES})

	target_link_libraries(Game PUBLIC
	 	WickedEngine_Windows
		efsw
	)
else()
	list (APPEND SOURCE_FILES Source/main_SDL2.cpp)
	add_executable(Game ${SOURCE_FILES})
	target_link_libraries(Game PUBLIC
	 	WickedEngine::WickedEngine
		efsw
	)

	list (APPEND DEV_FILES Source/main_SDL2.cpp)
	add_executable(Dev ${DEV_FILES})
	target_compile_definitions(Dev PUBLIC IS_DEV=1)
	target_link_libraries(Dev PUBLIC
	 	WickedEngine::WickedEngine
		efsw
	)
endif ()



if (MSVC)
	set_property(TARGET Game PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
endif ()

get_target_property(LIB_DXCOMPILER_LOCATION WickedEngine::dxcompiler IMPORTED_LOCATION)

# Copy content to build folder:
add_custom_command(
	TARGET Game POST_BUILD
	# Create symlink of data
	COMMAND ${CMAKE_COMMAND} -E create_symlink "${CMAKE_CURRENT_SOURCE_DIR}/Data" "${CMAKE_CURRENT_BINARY_DIR}/Data"

	# Copy the shader library next to the executable
	COMMAND ${CMAKE_COMMAND} -E copy_if_different "${LIB_DXCOMPILER_LOCATION}" "${CMAKE_CURRENT_BINARY_DIR}"
	COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/Source/Shader/*.hlsl" "${CMAKE_CURRENT_BINARY_DIR}/Data/Shader/"
	COMMAND ${CMAKE_COMMAND} -E copy_if_different "${WickedEngine_DIR}/../../WickedEngine/shaders/*" "${CMAKE_CURRENT_BINARY_DIR}/Data/Shader/"
	COMMAND ${CMAKE_COMMAND} -E copy_if_different "${WickedEngine_DIR}/../../WickedEngine/shaders/ffx-fsr/*" "${CMAKE_CURRENT_BINARY_DIR}/Data/Shader/ffx-fsr"
	COMMAND ${CMAKE_COMMAND} -E copy_if_different "${WickedEngine_DIR}/../../WickedEngine/shaders/ffx-shadows-dnsr/*" "${CMAKE_CURRENT_BINARY_DIR}/Data/Shader/ffx-shadows-dnsr"

	# Create symlink of root scripts
	COMMAND ${CMAKE_COMMAND} -E create_symlink "${CMAKE_CURRENT_SOURCE_DIR}/Source/Scripts/Root/startup.lua" "${CMAKE_CURRENT_BINARY_DIR}/startup.lua"
	COMMAND ${CMAKE_COMMAND} -E create_symlink "${CMAKE_CURRENT_SOURCE_DIR}/Source/Scripts/Root/editor.lua" "${CMAKE_CURRENT_BINARY_DIR}/editor.lua"
)
