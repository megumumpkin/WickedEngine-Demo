#include "Core.h"
#include "Filesystem.h"
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
    }

    void App::Update(float dt)
    {   
#ifdef IS_DEV
        Dev::Execute(dt);
#else
        GetScene()->Update(dt);
#endif
        wi::Application::Update(dt);
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