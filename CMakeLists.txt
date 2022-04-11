cmake_minimum_required(VERSION 3.8)
project(Game)

find_package(WickedEngine REQUIRED)

set (SOURCE_FILES
	Source/stdafx.cpp
	Source/Game.cpp
	Source/Game.h
	Source/stdafx.h
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
	)
else()
	list (APPEND SOURCE_FILES
		Source/main_SDL2.cpp
	)

	add_executable(Game ${SOURCE_FILES})

	target_link_libraries(Game PUBLIC
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
	# Copy the shader library next to the executable
	COMMAND ${CMAKE_COMMAND} -E copy_if_different "${LIB_DXCOMPILER_LOCATION}" "${CMAKE_CURRENT_BINARY_DIR}"
)
