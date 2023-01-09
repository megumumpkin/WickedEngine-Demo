#include "Core.h"
#include "Filesystem.h"
#include "Scene.h"

namespace Game{
    void App::Initialize()
    {
        // DO NOT EMBED ANY RESOURCES AT ALL!
        wi::resourcemanager::SetMode(wi::resourcemanager::Mode::DISCARD_FILEDATA_AFTER_LOAD);

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
        static bool test_done = false;
        if(!test_done)
        {
            auto p_a_ent = wi::ecs::CreateEntity();
            Scene::Prefab& p_a = GetScene()->prefabs.Create(p_a_ent);
            p_a.file = "content/DEVTEST/hologram_test.wiscene";

            auto p_b_ent = wi::ecs::CreateEntity();
            Scene::Prefab& p_b = GetScene()->prefabs.Create(p_b_ent);
            p_b.file = "content/DEVTEST/hairparticle_torus.wiscene";

            GetScene()->Load("content/DEVTEST/sponza.wiscene");

            test_done = true;
        }
        
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