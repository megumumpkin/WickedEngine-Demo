#include "RenderPipeline.h"

void Game::RenderPipeline::DefaultPipeline::Update(float dt){
    wi::RenderPath3D::Update(dt);
    Game::Resources::GetScene().Update(dt);
}