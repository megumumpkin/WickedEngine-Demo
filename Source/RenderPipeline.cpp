#include "RenderPipeline.h"
#include "Scene.h"

namespace Game{
    void RenderPipeline::Load()
    {
        scene = &(GetScene()->wiscene);
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