#pragma once

// GBus - Gameplay Bus
// Interact with other components through this bus

#include <WickedEngine.h>

namespace Gameplay::GBus
{
    namespace Camera
    {
        void SetPlayerMovementOffset(XMFLOAT3 offset);
        void AddPlayerCameraRotation(XMFLOAT2 rotation);
    }
    namespace Projectile
    {
        void CreateProgram(wi::primitive::Ray spawn, wi::ecs::Entity programID, std::string programfile);
        void SetSpawn(wi::primitive::Ray spawn, wi::ecs::Entity programID);
        void SetShoot(bool value, wi::ecs::Entity programID);
    }
}