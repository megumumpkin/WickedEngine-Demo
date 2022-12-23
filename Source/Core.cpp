#include "Core.h"

namespace Game{
    void App::Initialize()
    {
        wi::Application::Initialize();
        renderer.init(canvas);
        renderer.Load();
        ActivatePath(&renderer);
    }

    // void App::Update(float dt)
    // {

    // }

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