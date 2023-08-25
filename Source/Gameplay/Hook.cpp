#include "../Gameplay.h"

#include "Hook.h"
#include "Components.h"
#include "Player.h"
#include "Camera.h"
#include "Projectile.h"
#include "HitProcessor.h"

#include <iostream>

Game::Scene* scene;
#ifdef _WIN32
void Gameplay_Hook()
#else
extern "C" void Gameplay_Hook()
#endif
{
    // Gameplay::gameplay_hooks["Gameplay::Component"] = std::make_unique<Gameplay::ComponentHook>();
    // Gameplay::gameplay_hooks["Gameplay::Player"] = std::make_unique<Gameplay::Player>();
    // Gameplay::gameplay_hooks["Gameplay::Camera"] = std::make_unique<Gameplay::Camera>();
    // Gameplay::gameplay_hooks["Gameplay::Projectiles"] = std::make_unique<Gameplay::Projectile>();
    // Gameplay::gameplay_hooks["Gameplay::HitProcessor"] = std::make_unique<Gameplay::HitProcessor>();
    std::unique_ptr<Gameplay::GameplayHook> hook_component = std::make_unique<Gameplay::ComponentHook>();
    Add_Hook_Gameplay("Gameplay::Component", hook_component);
    std::unique_ptr<Gameplay::GameplayHook> hook_player = std::make_unique<Gameplay::Player>();
    Add_Hook_Gameplay("Gameplay::Player", hook_player);
    std::unique_ptr<Gameplay::GameplayHook> hook_camera = std::make_unique<Gameplay::Camera>();
    Add_Hook_Gameplay("Gameplay::Camera", hook_camera);
    std::unique_ptr<Gameplay::GameplayHook> hook_projectile = std::make_unique<Gameplay::Projectile>();
    Add_Hook_Gameplay("Gameplay::Projectiles", hook_projectile);
    std::unique_ptr<Gameplay::GameplayHook> hook_hitprocessor = std::make_unique<Gameplay::HitProcessor>();
    Add_Hook_Gameplay("Gameplay::HitProcessor", hook_hitprocessor);
}