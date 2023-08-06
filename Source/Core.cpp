#include "Core.h"
#include "Filesystem.h"
#include "Scripting.h"
#include "Scene.h"
#include "Gameplay.h"

#ifdef IS_DEV
#include "Dev.h"
#endif

namespace Game{
    RenderPipeline* renderPipeline_ptr;
    RenderPipeline* GetRenderPipeline()
    {
        return renderPipeline_ptr;
    }

    void App::Initialize()
    {
        wi::Application::Initialize();

        renderer.init(canvas);
        renderer.Load();
        // renderer.setSceneUpdateEnabled(false);
        ActivatePath(&renderer);
        renderPipeline_ptr = &renderer;

#ifdef IS_DEV
        Dev::LiveUpdate::Init();
#endif

        Scripting::Init(dynamic_cast<wi::Application*>(this));
        Gameplay::Init();
    }

    void App::Update(float dt)
    {   
#ifdef IS_DEV
        Dev::Execute(dt);
        if(Dev::GetCommandData()->type != Dev::CommandData::CommandType::SCENE_PREVIEW)
        {
            wi::Application::Update(0);
        }
        else
        {
#endif
        GetScene().PreUpdate(dt);
        Gameplay::PreUpdate(dt*wi::renderer::GetGameSpeed());
        wi::Application::Update(dt);
        GetScene().Update(dt);
        Gameplay::Update(dt*wi::renderer::GetGameSpeed());
#ifdef IS_DEV
        Dev::LiveUpdate::Update();
        }
#endif
    }

    void App::FixedUpdate()
    {
        Gameplay::FixedUpdate();
    }

    // void App::Render()
    // {
    //     renderer.
    // }

    // void App::Compose(wi::graphics::CommandList cmd)
    // {

    // }
}