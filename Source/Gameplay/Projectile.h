#pragma once
#include "../Gameplay.h"
#include <WickedEngine.h>

static const char* Projectile_Scripting_Globals = R"(
-----------------------------------------------
-- Gameplay Projectile Globals
-----------------------------------------------
GAMEPLAY_BULLET_PROGRAM = {}

function Gameplay_Projectile_StoreResetData(programID)
    _ENV.GAMEPLAY_BULLET_PROGRAM[programID].Reset = {}
    _ENV.GAMEPLAY_BULLET_PROGRAM[programID].Reset.Rate = deepcopy(_ENV.GAMEPLAY_BULLET_PROGRAM[programID].Rate)
    _ENV.GAMEPLAY_BULLET_PROGRAM[programID].Reset.Spawners = deepcopy(_ENV.GAMEPLAY_BULLET_PROGRAM[programID].Spawners)
    _ENV.GAMEPLAY_BULLET_PROGRAM[programID].Reset.Velocity = deepcopy(_ENV.GAMEPLAY_BULLET_PROGRAM[programID].Velocity)
end
function Gameplay_Projectile_RestoreResetData(programID)
    _ENV.GAMEPLAY_BULLET_PROGRAM[programID].Rate = deepcopy(_ENV.GAMEPLAY_BULLET_PROGRAM[programID].Reset.Rate)
    _ENV.GAMEPLAY_BULLET_PROGRAM[programID].Spawners = deepcopy(_ENV.GAMEPLAY_BULLET_PROGRAM[programID].Reset.Spawners)
    _ENV.GAMEPLAY_BULLET_PROGRAM[programID].Velocity = deepcopy(_ENV.GAMEPLAY_BULLET_PROGRAM[programID].Reset.Velocity)
end
function Gameplay_Projectile_GetPrefabFile(programID) 
    return _ENV.GAMEPLAY_BULLET_PROGRAM[programID].Prefab
end
function Gameplay_Projectile_GetSpawnRate(programID) 
    return _ENV.GAMEPLAY_BULLET_PROGRAM[programID].Rate
end
function Gameplay_Projectile_GetSpawners(programID) 
    return _ENV.GAMEPLAY_BULLET_PROGRAM[programID].Spawners
end
function Gameplay_Projectile_GetVelocity(programID) 
    return _ENV.GAMEPLAY_BULLET_PROGRAM[programID].Velocity
end
)";

namespace Gameplay
{
    class Projectile final : public GameplayHook
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