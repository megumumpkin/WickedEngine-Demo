#pragma once

#include <wiRenderPath.h>

namespace Editor{
    struct Data{
        wi::RenderPath* viewport;
    };
    Data* GetData();
    void Init();
    void Update(float dt);
}