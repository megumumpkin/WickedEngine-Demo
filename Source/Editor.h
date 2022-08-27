#pragma once

#include "stdafx.h"
#include "Editor_Translator.h"

#include "ImGui/imgui.h"
#include "ImGui/imgui_internal.h"
#include "ImGui/imgui_BindLua.h"
#include "ImGui/FA6_UI_Icons.h"
#include "ImGui/Widgets/ImGuizmo.h"

#ifdef _WIN32
#include "ImGui/imgui_impl_win32.h"
#elif defined(SDL2)
#include "ImGui/imgui_impl_sdl.h"
#endif

namespace Editor{
    struct Data
    {
        wi::RenderPath* viewport;
        wi::graphics::RenderPass renderpass_editor;
        wi::graphics::RenderPass renderpass_selection_editor;
        wi::graphics::Texture rt_depthbuffer_editor;
        wi::graphics::Texture rt_selection_editor;

        const XMFLOAT4 editor_selection_color = XMFLOAT4(1.f,0.6f,0.3f,0.8f);

        wi::scene::PickResult selection;
        Translator transform_translator;
    };
    enum EDITOR_STENCILREF : uint8_t
    {
        EDITOR_STENCILREF_NONE = 0x00,
        EDITOR_STENCILREF_LAST = 0x0F,
    };
    Data* GetData();
    void Init();
    void Update(float dt, wi::RenderPath2D& viewport);

    //Render pipeline processes
    void ResizeBuffers(wi::graphics::GraphicsDevice* device, wi::RenderPath3D* renderPath);
    void Render(wi::graphics::GraphicsDevice* device, wi::graphics::CommandList& cmd);
    void Compose(wi::graphics::GraphicsDevice* device, wi::graphics::CommandList& cmd);
}