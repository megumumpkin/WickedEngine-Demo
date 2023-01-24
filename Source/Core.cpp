#include "Core.h"
#include "Filesystem.h"
#include "Scripting.h"
#include "Scene.h"

#ifdef IS_DEV
#include "Dev.h"
#endif

namespace Game{
    void App::Initialize()
    {
        wi::Application::Initialize();

        renderer.init(canvas);
        renderer.Load();
        ActivatePath(&renderer);

        Scripting::Init(dynamic_cast<wi::Application*>(this));
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
        GetScene()->PreUpdate(dt);
        wi::Application::Update(dt);
        GetScene()->Update(dt);
#ifdef IS_DEV
        }
#endif
    }

    // void App::FixedUpdate()
    // {

    // }

    // void App::Render()
    // {
    //     renderer.
    // }

    // void App::Compose(wi::graphics::CommandList cmd)
    // {

    // }
}