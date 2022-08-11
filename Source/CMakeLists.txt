set(WickedEngine_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../WickedEngine/build/cmake")
find_package(WickedEngine REQUIRED)
add_subdirectory(efsw)

set (SOURCE_FILES
    main_${PLATFORM}.cpp
	Resources.h
	Resources.cpp
	Resources_BindLua.h
	Resources_BindLua.cpp
	RenderPipeline.h
	RenderPipeline.cpp
	BindLua.h
	BindLua_Globals.h
	BindLua.cpp
	LiveUpdate.h
	LiveUpdate.cpp
    Application.h
    Application.cpp
)

set (DEV_FILES
	${SOURCE_FILES}
    ImGui/imconfig.h
	ImGui/imgui.cpp
	ImGui/imgui.h
	ImGui/imgui_demo.cpp
	ImGui/imgui_draw.cpp
	ImGui/imgui_internal.h
	ImGui/imgui_tables.cpp
	ImGui/imgui_widgets.cpp
	ImGui/imstb_rectpack.h
	ImGui/imstb_textedit.h
	ImGui/imstb_truetype.h
	ImGui/imgui_impl_sdl.cpp
	ImGui/imgui_impl_sdl.h

	ImGui/FA6_UI_Icons.h

	ImGui/imgui_iterator.h
	ImGui/imgui_BindLua.h
	ImGui/imgui_BindLua.cpp

	ImGui/Widgets/ImGuizmo.h
	ImGui/Widgets/ImGuizmo.cpp

	Editor.h
	Editor.cpp
)

add_executable(Game ${SOURCE_FILES})
target_link_libraries(Game PUBLIC
    WickedEngine::WickedEngine
	efsw
)

add_executable(GameDevMode ${DEV_FILES})
target_compile_definitions(GameDevMode PUBLIC IS_DEV=1)
target_link_libraries(GameDevMode PUBLIC
	WickedEngine::WickedEngine
	efsw
)

# Copy content to build folder:
add_custom_command(
	TARGET Game POST_BUILD
	# Copy the shader library next to the executable
	COMMAND ${CMAKE_COMMAND} -E copy_if_different "${WickedEngine_DIR}/../../WickedEngine/libdxcompiler.so" "${CMAKE_CURRENT_BINARY_DIR}"
	COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/Shader/*.hlsl" "${CMAKE_CURRENT_BINARY_DIR}/data/shader/"
	COMMAND ${CMAKE_COMMAND} -E copy_if_different "${WickedEngine_DIR}/../../WickedEngine/shaders/*" "${CMAKE_CURRENT_BINARY_DIR}/data/shader/"
	COMMAND ${CMAKE_COMMAND} -E copy_if_different "${WickedEngine_DIR}/../../WickedEngine/shaders/ffx-fsr/*" "${CMAKE_CURRENT_BINARY_DIR}/data/shader/ffx-fsr"
	COMMAND ${CMAKE_COMMAND} -E copy_if_different "${WickedEngine_DIR}/../../WickedEngine/shaders/ffx-shadows-dnsr/*" "${CMAKE_CURRENT_BINARY_DIR}/data/shader/ffx-shadows-dnsr"
)