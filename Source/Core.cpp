#include "Core.h"
#include "Filesystem.h"
#include "Scene.h"

#ifdef IS_DEV
#include "Dev.h"
#endif

namespace Game{
    void App::Initialize()
    {
        // DO NOT EMBED ANY RESOURCES AT ALL!
        // wi::resourcemanager::SetMode(wi::resourcemanager::Mode::ALLOW_RETAIN_FILEDATA_BUT_DISABLE_EMBEDDING);

        Filesystem::Register_FS("content/", "Data/Content/", false);
        Filesystem::Register_FS("shader/", "Data/Shader/", false);

        wi::renderer::SetShaderSourcePath(Filesystem::GetActualPath("shader/"));
        wi::renderer::SetShaderPath(Filesystem::GetActualPath("shader/"));

        wi::Application::Initialize();

        renderer.init(canvas);
        renderer.Load();
        ActivatePath(&renderer);
    }

    void App::Update(float dt)
    {   
#ifdef IS_DEV
        Dev::Execute();
#endif
        GetScene()->Update(dt);
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