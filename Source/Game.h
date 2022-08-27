#pragma once
#include "stdafx.h"

namespace Game{
    const char* GetApplicationName();

    class LoadingScreen : public wi::LoadingScreen {
    public:
        void Load() override;
        void Update(float dt) override;
    };

    class ApplicationView : public wi::RenderPath2D
    {
    private:
        std::unique_ptr<wi::RenderPath3D> renderPath;
        uint32_t renderpath_current;

        wi::graphics::Texture rt_selectionOutline_MSAA;
        wi::graphics::Texture rt_selectionOutline[2];
	    wi::graphics::RenderPass renderpass_selectionOutline[2];
        const XMFLOAT4 selectionColor = XMFLOAT4(1, 0.6f, 0, 1);
        const XMFLOAT4 selectionColor2 = XMFLOAT4(0, 1, 0.6f, 0.35f);
    public:
        wi::Application* app;
        enum RENDERPATH{
            RENDERPATH_DEFAULT,
        };
        enum STENCILFX{
            NONE,
            SELECTION_OUTLINE
        };
        void SetRenderPath(RENDERPATH path);

        void ResizeBuffers() override;
        void ResizeLayout() override;
        void Load() override;
        void Start() override;
        void PreUpdate() override;
        void FixedUpdate() override;
        void Update(float dt) override;
        void PostUpdate() override;
        void Render() const override;
        void Compose(wi::graphics::CommandList cmd) const override;

        void Update_ImGUI();
    };

    class Application : public wi::Application {
    private:
        ApplicationView view;
    public:
        ~Application() override;
        void Initialize() override;
        void Initialize_ImGUI();
        void Compose(wi::graphics::CommandList cmd) override;
        void Compose_ImGUI(wi::graphics::CommandList cmd);
    };
}