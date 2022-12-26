#include "RenderPipeline.h"
#include "Scene.h"

namespace Game{
    void RenderPipeline::Load()
    {
        scene = &(GetScene()->wiscene);
    }
}