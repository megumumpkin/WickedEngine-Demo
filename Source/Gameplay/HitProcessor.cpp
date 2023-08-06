#include "HitProcessor.h"

#include "Components.h"
#include "../Scene.h"

#include <WickedEngine.h>
#include <mutex>

void Gameplay::HitProcessor::Hook_Init()
{
}

struct HitDataCollider
{
    bool is_capsule = false;
    wi::primitive::Sphere collider_sphere;
    wi::primitive::Capsule collider_capsule;
};
HitDataCollider _internal_GenerateColliderData(wi::ecs::Entity hitdataID)
{
    HitDataCollider hitdatacollider;

    Gameplay::Component::HitData* hitdata = Gameplay::Component::hitdatas->GetComponent(hitdataID);
    wi::scene::TransformComponent* transform = Game::GetScene().wiscene.transforms.GetComponent(hitdataID);

    switch(hitdata->colliderType)
    {
        case Gameplay::Component::HitData::ColliderType::SPHERE:
        {
            XMFLOAT3 extent_vec3 = transform->GetScale();
            float radius = std::max(extent_vec3.x, std::max(extent_vec3.y, extent_vec3.z));
            
            hitdatacollider.collider_sphere = wi::primitive::Sphere(transform->GetPosition(), radius);
            break;
        };
        case Gameplay::Component::HitData::ColliderType::CAPSULE:
        {
            XMFLOAT3 extent = XMFLOAT3(0.f,1.f,0.f);
            XMVECTOR stub_vector, remapped_extent_V, positive_position_V, opposite_position_V;
            XMMatrixDecompose(&stub_vector, &stub_vector, &remapped_extent_V, 
                XMMatrixTranslationFromVector(XMLoadFloat3(&extent)))
                *XMLoadFloat4x4(&transform->world);
            XMFLOAT3 temp, positive_position, opposite_position;
            temp = transform->GetPosition();
            positive_position_V = XMLoadFloat3(&temp)+remapped_extent_V;
            opposite_position_V = XMLoadFloat3(&temp)-remapped_extent_V;
            XMStoreFloat3(&positive_position, positive_position_V);
            XMStoreFloat3(&opposite_position, opposite_position_V);

            extent = transform->GetScale();
            float radius = std::max(extent.x, extent.y);
            
            hitdatacollider.collider_capsule = wi::primitive::Capsule(opposite_position, positive_position, radius);
            hitdatacollider.is_capsule = true;
            break;
        };
    }

    return hitdatacollider;
}

wi::jobsystem::context hp_ctx;
std::mutex sync_hitpoint;
void Gameplay::HitProcessor::Hook_PreUpdate(float dt)
{
    
}

void Gameplay::HitProcessor::Hook_Update(float dt)
{
    // Calculate hit and accumulate value changes
    wi::jobsystem::Dispatch(hp_ctx, Gameplay::Component::hitdatas->GetCount(), 255, [](wi::jobsystem::JobArgs jobArgs){
        uint32_t hitdata_idx = jobArgs.jobIndex;
        wi::ecs::Entity hitdataID = Gameplay::Component::hitdatas->GetEntity(hitdata_idx);
        Gameplay::Component::HitData* hitdata = Gameplay::Component::hitdatas->GetComponent(hitdataID);

        if(hitdata->communicationType == Gameplay::Component::HitData::CommunicationType::SENDER)
        {
            HitDataCollider this_collider = _internal_GenerateColliderData(hitdataID);

            // Now do collision check!
            // Check all hitdatas first!
            for(size_t i = 0; i < Gameplay::Component::hitdatas->GetCount(); ++i)
            {
                wi::ecs::Entity other_hitdataID = Gameplay::Component::hitdatas->GetEntity(i);
                Gameplay::Component::HitData* other_hitdata = Gameplay::Component::hitdatas->GetComponent(hitdataID);
                HitDataCollider other_collider = _internal_GenerateColliderData(other_hitdataID);

                if(other_hitdata->communicationType != Gameplay::Component::HitData::CommunicationType::RECEIVER)
                    continue;

                bool collide = false;
                if(other_collider.is_capsule)
                {
                    if(this_collider.is_capsule)
                    {
                        XMFLOAT3 position, incident_normal;
                        float penetration_depth;
                        if(other_collider.collider_capsule.intersects(this_collider.collider_capsule, position, incident_normal, penetration_depth))
                            collide = true;
                    }
                    else if(other_collider.collider_capsule.intersects(this_collider.collider_sphere))
                        collide = true;
                }
                else
                {
                    if(this_collider.is_capsule)
                    {
                        if(other_collider.collider_sphere.intersects(this_collider.collider_capsule))
                            collide = true;
                    }
                    else if(other_collider.collider_sphere.intersects(this_collider.collider_sphere))
                        collide = true;
                }
                
                if(collide)
                {
                    std::scoped_lock sync (sync_hitpoint);
                    other_hitdata->hitpoint += hitdata->hitpoint;
                    hitdata->hit = true;
                    other_hitdata->hit = true;
                }
            }

            // Check all Navmesh for hit
            if(this_collider.is_capsule)
            {
                wi::scene::SceneIntersectCapsuleResult res = Game::GetScene().wiscene.Intersects(this_collider.collider_capsule, wi::enums::FILTER_NAVIGATION_MESH | wi::enums::FILTER_COLLIDER, ~0u);
                if(res.entity != wi::ecs::INVALID_ENTITY)
                    hitdata->hit = true;
            }
            else
            {
                wi::scene::SceneIntersectSphereResult res = Game::GetScene().wiscene.Intersects(this_collider.collider_sphere, wi::enums::FILTER_NAVIGATION_MESH | wi::enums::FILTER_COLLIDER, ~0u);
                if(res.entity != wi::ecs::INVALID_ENTITY)
                    hitdata->hit = true;
            }
        }
    });
    wi::jobsystem::Wait(hp_ctx);
}

void Gameplay::HitProcessor::Hook_FixedUpdate()
{
}

void Gameplay::HitProcessor::Hook_Migrate(bool store, wi::vector<uint8_t>& storage)
{
}