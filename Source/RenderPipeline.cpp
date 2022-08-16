#include "RenderPipeline.h"
#include "Resources.h"
#include <wiRenderPath3D.h>

void Game::RenderPipeline::DefaultPipeline::Update(float dt){
    wi::RenderPath3D::Update(dt);
    Game::Resources::GetScene().Update(dt);
}