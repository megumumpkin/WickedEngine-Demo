#pragma once
#include "../Gameplay.h"

namespace Gameplay
{
    class Player final : public GameplayHook
    {
    public:
        // Hook functions
        void Hook_Init();
        void Hook_PreUpdate(float dt);
        void Hook_Update(float dt);
        void Hook_FixedUpdate();
        void Hook_Migrate(bool store, wi::vector<uint8_t>& storage);
    };
}