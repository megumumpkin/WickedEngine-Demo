cmake_minimum_required(VERSION 3.8)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(CMAKE_INSTALL_PREFIX "${CMAKE_CURRENT_SOURCE_DIR}/build/Install")
set(CMAKE_PREFIX_PATH "${CMAKE_CURRENT_SOURCE_DIR}/build/Install")

project(Game)

add_subdirectory(Library)

set(WickedEngine_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Library/WickedEngine/build/cmake")
find_package(WickedEngine REQUIRED)
find_package(reproc++)

set (SOURCE_FILES
	Source/stdafx.h
	Source/Config.h
	Source/Config.cpp
	Source/Filesystem.h
	Source/Filesystem.cpp
	Source/Scripting_Globals.h
	Source/Scripting.h
	Source/Scripting.cpp
	Source/Scene.h
	Source/Scene.cpp
	Source/Scene_BindScript.h
	Source/Scene_BindScript.cpp
	Source/RenderPipeline.h
	Source/RenderPipeline.cpp
	Source/Gameplay.h
	Source/Gameplay.cpp
	Source/Core.h
	Source/Core.cpp
)

set (DEV_FILES
	${SOURCE_FILES}
	Source/json.hpp
	Source/tiny_gltf.h
	Source/Dev.h
	Source/Dev_LiveUpdate.cpp
	Source/Dev_IO_GLTF.cpp
	Source/Dev.cpp
)

set (GAMEPLAY_FILES
	Source/Gameplay/Hook.cpp
	Source/Gameplay/GameplayAgencyControl.h
	Source/Gameplay/GameplayAgencyControl.cpp
	Source/Gameplay/Helpers.h
	Source/Gameplay/Helpers.cpp
	Source/Gameplay/Components.h
	Source/Gameplay/Components.cpp
	Source/Gameplay/Components_BindScript.h
	Source/Gameplay/Components_BindScript.cpp
	Source/Gameplay/Player.h
	Source/Gameplay/Player.cpp
	Source/Gameplay/Camera.h
	Source/Gameplay/Camera.cpp
	Source/Gameplay/Projectile.h
	Source/Gameplay/Projectile.cpp
	Source/Gameplay/HitProcessor.h
	Source/Gameplay/HitProcessor.cpp
)

if (WIN32)
	list (APPEND SOURCE_FILES
		Source/main_Windows.cpp
		Source/main_Windows.h
		Source/TemplateWindows.rc
	)
	add_executable(Game WIN32 ${SOURCE_FILES})
	set_target_properties(Game PROPERTIES
		ENABLE_EXPORTS 1
  		WINDOWS_EXPORT_ALL_SYMBOLS 1
	)
	target_link_libraries(Game PUBLIC
	 	WickedEngine::WickedEngine_Windows
	)

	add_library(Gameplay SHARED ${GAMEPLAY_FILES})
 	set_target_properties(Gameplay PROPERTIES
		ENABLE_EXPORTS 1
  		WINDOWS_EXPORT_ALL_SYMBOLS 1
	)
	target_link_libraries(Gameplay PUBLIC
	 	WickedEngine::WickedEngine_Windows
		Game
	)

	list (APPEND DEV_FILES
		Source/main_Windows.cpp
		Source/main_Windows.h
		Source/TemplateWindows.rc
	)
	add_executable(Dev WIN32 ${DEV_FILES})
	set_target_properties(Dev PROPERTIES
		ENABLE_EXPORTS 1
  		WINDOWS_EXPORT_ALL_SYMBOLS 1
	)
	target_compile_definitions(Dev PUBLIC IS_DEV=1)
	target_link_libraries(Dev PUBLIC
	 	WickedEngine::WickedEngine_Windows
		efsw
		reproc++
	)

	add_library(Gameplay_DEV SHARED ${GAMEPLAY_FILES})
 	set_target_properties(Gameplay_DEV PROPERTIES
		ENABLE_EXPORTS 1
  		WINDOWS_EXPORT_ALL_SYMBOLS 1
	)
	target_link_libraries(Gameplay_DEV PUBLIC
	 	WickedEngine::WickedEngine_Windows
		Dev
	)
else()
	list (APPEND SOURCE_FILES Source/main_Linux.cpp)
	add_executable(Game ${SOURCE_FILES})
	set_target_properties(Game PROPERTIES
		ENABLE_EXPORTS 1
	)
	target_link_libraries(Game PUBLIC
	 	WickedEngine::WickedEngine_Linux
	)

	add_library(Gameplay SHARED ${GAMEPLAY_FILES})
	target_link_libraries(Gameplay PUBLIC
	 	WickedEngine::WickedEngine_Linux
		Game
	)

	list (APPEND DEV_FILES Source/main_Linux.cpp)
	add_executable(Dev ${DEV_FILES})
	set_target_properties(Dev PROPERTIES
		ENABLE_EXPORTS 1
	)
	target_compile_definitions(Dev PUBLIC IS_DEV=1)
	target_link_libraries(Dev PUBLIC
	 	WickedEngine::WickedEngine_Linux
		efsw
		reproc++
	)

	add_library(Gameplay_DEV SHARED ${GAMEPLAY_FILES})
	target_link_libraries(Gameplay_DEV PUBLIC
	 	WickedEngine::WickedEngine_Linux
		Dev
	)
endif ()



if (MSVC)
	set_property(TARGET Game PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
endif ()

get_target_property(LIB_DXCOMPILER_LOCATION WickedEngine::dxcompiler IMPORTED_LOCATION)

# Copy content to build folder:
if(NOT WIN32)
	add_custom_command(
		TARGET Game POST_BUILD
		# Create symlink of data
		COMMAND ${CMAKE_COMMAND} -E create_symlink "${CMAKE_CURRENT_SOURCE_DIR}/Data" "${CMAKE_CURRENT_BINARY_DIR}/Data"

		# Copy the shader library next to the executable
		COMMAND ${CMAKE_COMMAND} -E copy_if_different "${LIB_DXCOMPILER_LOCATION}" "${CMAKE_CURRENT_BINARY_DIR}"
		# COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/Source/Shader/*.hlsl" "${CMAKE_CURRENT_BINARY_DIR}/Data/Shader/"
		COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/Library/WickedEngine/build/WickedEngine/libWickedEngine_Linux.so" "${CMAKE_CURRENT_BINARY_DIR}"
  		COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/Library/WickedEngine/WickedEngine/shaders/*" "${CMAKE_CURRENT_SOURCE_DIR}/Data/Shader/"
		COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/Library/WickedEngine/WickedEngine/shaders/compressonator/*" "${CMAKE_CURRENT_SOURCE_DIR}/Data/Shader/compressonator/"
		COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/Library/WickedEngine/WickedEngine/shaders/ffx-fsr/*" "${CMAKE_CURRENT_SOURCE_DIR}/Data/Shader/ffx-fsr/"
		COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/Library/WickedEngine/WickedEngine/shaders/ffx-fsr2/*" "${CMAKE_CURRENT_SOURCE_DIR}/Data/Shader/ffx-fsr2/"
		COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/Library/WickedEngine/WickedEngine/shaders/ffx-shadows-dnsr/*" "${CMAKE_CURRENT_SOURCE_DIR}/Data/Shader/ffx-shadows-dnsr/"
	)
endif()
