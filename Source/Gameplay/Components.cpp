#include "Components.h"
#include "Components_BindScript.h"

#include "../Scene.h"

#include <WickedEngine.h>

namespace Gameplay::Component
{
    wi::ecs::ComponentManager<Health>* healths;
    wi::ecs::ComponentManager<HitData>* hitdatas;
    wi::ecs::ComponentManager<Projectile>* projectiles;
}

void Init_Scene(wi::scene::Scene* scene, bool set_root = false)
{
    auto healths = &scene->componentLibrary.Register<Gameplay::Component::Health>("Game::Gameplay::Health");
    auto hitdatas = &scene->componentLibrary.Register<Gameplay::Component::HitData>("Game::Gameplay::HitData");
    auto projectiles = &scene->componentLibrary.Register<Gameplay::Component::Projectile>("Game::Gameplay::Projectile");

    if(set_root)
    {
        Gameplay::Component::healths = healths;
        Gameplay::Component::hitdatas = hitdatas;
        Gameplay::Component::projectiles = projectiles;
    }
}

void Gameplay::ComponentHook::Hook_Init()
{
    Init_Scene(&Game::GetScene().wiscene, true);
    // Gameplay::scene_init_hooks.push_back([](wi::scene::Scene* scene){
    //     Init_Scene(scene);
    // });
    Add_Hook_Scene([](wi::scene::Scene* scene){
        Init_Scene(scene);
    });
    Gameplay::Scripting::Component::Bind();
}

void Gameplay::ComponentHook::Hook_PreUpdate(float dt)
{

}

void Gameplay::ComponentHook::Hook_Update(float dt)
{

}

void Gameplay::ComponentHook::Hook_FixedUpdate()
{

}

void Gameplay::ComponentHook::Hook_Migrate(bool store, wi::vector<uint8_t>& storage)
{
    
}