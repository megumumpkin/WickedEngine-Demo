#pragma once
#include "stdafx.h"

namespace Game
{
    class RenderPipeline : public wi::RenderPath3D
    {
    public:
        void Load() override;
        void PreUpdate() override;
        void FixedUpdate() override;
        void Update(float dt) override;
        void Render() const override;
    };
}