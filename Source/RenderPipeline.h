#pragma once
#include "stdafx.h"

namespace Game{
    namespace RenderPipeline{
        class DefaultPipeline : public wi::RenderPath3D{
        private:
        public:
            void Update(float dt);
        };
    }
}