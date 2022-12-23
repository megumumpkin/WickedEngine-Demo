#pragma once
#include "stdafx.h"

namespace Game{
    class RenderPipeline : public wi::RenderPath3D
    {};
    class App : public wi::Application
    {
        RenderPipeline renderer;
    public:
        // This is where the critical initializations happen (before any rendering or anything else)
		void Initialize() override;
		// // This is where application-wide updates get executed once per frame. 
		// //  RenderPath::Update is also called from here for the active component
		// void Update(float dt) override;
		// // This is where application-wide updates get executed in a fixed timestep based manner. 
		// //  RenderPath::FixedUpdate is also called from here for the active component
		// void FixedUpdate() override;
		// // This is where application-wide rendering happens to offscreen buffers. 
		// //  RenderPath::Render is also called from here for the active component
		// void Render() override;
		// // This is where the application will render to the screen (backbuffer). It must render to the provided command list.
		// void Compose(wi::graphics::CommandList cmd) override;
    };
}