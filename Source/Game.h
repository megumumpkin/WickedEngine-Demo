#pragma once
#include "WickedEngine.h"

class Renderer : public wi::RenderPath3D
{
    wi::ecs::Entity testEntity = wi::ecs::INVALID_ENTITY;

public:
    void Load() override;
    void Update(float dt) override;
};

class Game : public wi::Application
{
    Renderer renderer;

public:
    void Initialize() override;
};

