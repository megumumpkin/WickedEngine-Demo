#pragma once
#include "stdafx.h"
#include <WickedEngine.h>

namespace Gameplay
{
    class GameplayHook
    {
    public:
        virtual void Hook_Init() = 0;
        virtual void Hook_PreUpdate(float dt) = 0;
        virtual void Hook_Update(float dt) = 0;
        virtual void Hook_FixedUpdate() = 0;
        virtual void Hook_Migrate(bool store, wi::vector<uint8_t>& storage) = 0;
    };

    extern wi::unordered_map<std::string, std::unique_ptr<GameplayHook>> gameplay_hooks;
    extern wi::vector<std::function<void(wi::scene::Scene*)>> scene_init_hooks;

    void Init();
    void SceneInit(wi::scene::Scene* scene);
    void PreUpdate(float dt);
    void Update(float dt);
    void FixedUpdate();
    void Deinit();
}

extern "C" void Add_Hook_Gameplay(std::string key, std::unique_ptr<Gameplay::GameplayHook>& hook);
extern "C" void Add_Hook_Scene(std::function<void(wi::scene::Scene*)> hook);