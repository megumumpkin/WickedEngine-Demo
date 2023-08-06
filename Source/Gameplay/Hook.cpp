#include "../Gameplay.h"

#include "Hook.h"
#include "Components.h"
#include "Player.h"
#include "Camera.h"
#include "Projectile.h"
#include "HitProcessor.h"

#include <iostream>

Game::Scene* scene;

extern "C" void Gameplay_Hook()
{
    Gameplay::gameplay_hooks["Gameplay::Component"] = std::make_unique<Gameplay::ComponentHook>();
    Gameplay::gameplay_hooks["Gameplay::Player"] = std::make_unique<Gameplay::Player>();
    Gameplay::gameplay_hooks["Gameplay::Camera"] = std::make_unique<Gameplay::Camera>();
    Gameplay::gameplay_hooks["Gameplay::Projectiles"] = std::make_unique<Gameplay::Projectile>();
    Gameplay::gameplay_hooks["Gameplay::HitProcessor"] = std::make_unique<Gameplay::HitProcessor>();
}