#pragma once
#include "stdafx.h"

namespace Game
{
    class RenderPipeline : public wi::RenderPath3D
    {
    public:
        void Load() override;
    };
}