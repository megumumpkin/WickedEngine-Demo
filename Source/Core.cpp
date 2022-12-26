#include "Core.h"
#include "Filesystem.h"

namespace Game{
    void App::Initialize()
    {
        Filesystem::Register_FS("content/", "Data/Content/", false);
        Filesystem::Register_FS("shader/", "Data/Shader/", false);

        wi::renderer::SetShaderSourcePath(Filesystem::GetActualPath("shader/"));
        wi::renderer::SetShaderPath(Filesystem::GetActualPath("shader/"));

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