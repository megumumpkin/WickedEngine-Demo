cmake_minimum_required(VERSION 3.8)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

project(Game)

add_subdirectory(Library)

set(WickedEngine_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Library/WickedEngine/build/cmake")
find_package(WickedEngine REQUIRED)

set (SOURCE_FILES
	Source/stdafx.h
	Source/Config.h
	Source/Config.cpp
	Source/Filesystem.h
	Source/Filesystem.cpp
	Source/Scene.h
	Source/Scene.cpp
	Source/RenderPipeline.h
	Source/RenderPipeline.cpp
	Source/Core.h
	Source/Core.cpp
)

set (DEV_FILES
	${SOURCE_FILES}
	Source/json.hpp
	Source/tiny_gltf.h
	Source/Dev.h
	Source/Dev_IO_GLTF.cpp
	Source/Dev.cpp
)

if (WIN32)
	list (APPEND SOURCE_FILES
		Source/main_Windows.cpp
		Source/main_Windows.h
		Source/TemplateWindows.rc
	)

	add_executable(Game WIN32 ${SOURCE_FILES})

	target_link_libraries(Game PUBLIC
	 	WickedEngine::WickedEngine
	)
else()
	list (APPEND SOURCE_FILES Source/main_Linux.cpp)
	add_executable(Game ${SOURCE_FILES})
	target_link_libraries(Game PUBLIC
	 	WickedEngine::WickedEngine
	)

	list (APPEND DEV_FILES Source/main_Linux.cpp)
	add_executable(Dev ${DEV_FILES})
	target_compile_definitions(Dev PUBLIC IS_DEV=1)
	target_link_libraries(Dev PUBLIC
	 	WickedEngine::WickedEngine
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
	# COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/Source/Shader/*.hlsl" "${CMAKE_CURRENT_BINARY_DIR}/Data/Shader/"
	COMMAND ${CMAKE_COMMAND} -E copy_if_different "${WickedEngine_DIR}/../../WickedEngine/shaders/*" "${CMAKE_CURRENT_SOURCE_DIR}/Data/Shader/"
	COMMAND ${CMAKE_COMMAND} -E copy_if_different "${WickedEngine_DIR}/../../WickedEngine/shaders/compressonator/*" "${CMAKE_CURRENT_SOURCE_DIR}/Data/Shader/compressonator/"
	COMMAND ${CMAKE_COMMAND} -E copy_if_different "${WickedEngine_DIR}/../../WickedEngine/shaders/ffx-fsr/*" "${CMAKE_CURRENT_SOURCE_DIR}/Data/Shader/ffx-fsr/"
	COMMAND ${CMAKE_COMMAND} -E copy_if_different "${WickedEngine_DIR}/../../WickedEngine/shaders/ffx-fsr2/*" "${CMAKE_CURRENT_SOURCE_DIR}/Data/Shader/ffx-fsr2/"
	COMMAND ${CMAKE_COMMAND} -E copy_if_different "${WickedEngine_DIR}/../../WickedEngine/shaders/ffx-shadows-dnsr/*" "${CMAKE_CURRENT_SOURCE_DIR}/Data/Shader/ffx-shadows-dnsr/"
)