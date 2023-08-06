#include "RenderPipeline.h"
#include "Scene.h"

namespace Game{
    void RenderPipeline::Load()
    {
        scene = &(GetScene().wiscene);

        wi::renderer::SetVXGIEnabled(true);
        // this->setVXGIResolveFullResolutionEnabled(true);
        // wi::renderer::SetGIBoost(1.3f);
        for(int i = 0; i < 6; ++i)
        {
            float size = (i < 2) ? 0.75f :
                (i < 4) ? 0.5f : 0.25f;
            scene->vxgi.clipmaps[0].voxelsize = size;
        }

        wi::RenderPath3D::Load();
    }
    void RenderPipeline::PreUpdate()
    {
        wi::RenderPath3D::PreUpdate();
    }
    void RenderPipeline::FixedUpdate()
    {
        wi::RenderPath3D::FixedUpdate();
    }
    void RenderPipeline::Update(float dt)
    {
        wi::RenderPath3D::Update(dt);
    }
    void RenderPipeline::Render() const
    {
        wi::RenderPath3D::Render();
    }
}