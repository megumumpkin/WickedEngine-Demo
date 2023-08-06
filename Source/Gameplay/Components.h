#pragma once
#include "../Gameplay.h"
#include <WickedEngine.h>

namespace Gameplay
{
    namespace Component
    {
        struct Health
        {
            float health = 0.f;
            void Serialize(wi::Archive& archive, wi::ecs::EntitySerializer& seri){}
        };
        struct HitData
        {
            bool hit = false;
            float hitpoint = 0.f;
            enum class CommunicationType
            {
                SENDER,
                RECEIVER,
                DISABLED
            };
            CommunicationType communicationType = CommunicationType::SENDER;
            enum class ColliderType
            {
                SPHERE,
                CAPSULE
            };
            ColliderType colliderType = ColliderType::SPHERE;
            void Serialize(wi::Archive& archive, wi::ecs::EntitySerializer& seri){}
        };
        struct Projectile
        {
            XMFLOAT3 normal;
            wi::ecs::Entity hitbox = wi::ecs::INVALID_ENTITY;
            wi::ecs::Entity program = wi::ecs::INVALID_ENTITY;
            void Serialize(wi::Archive& archive, wi::ecs::EntitySerializer& seri){}
        }; 

        extern wi::ecs::ComponentManager<Health>* healths;
        extern wi::ecs::ComponentManager<HitData>* hitdatas;
        extern wi::ecs::ComponentManager<Projectile>* projectiles;
    }
    class ComponentHook final : public GameplayHook
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