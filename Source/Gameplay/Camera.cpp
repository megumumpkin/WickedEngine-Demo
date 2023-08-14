#include "Camera.h"

#include "Hook.h"
#include "GameplayAgencyControl.h"
#include "GBus.h"
#include "../Scene.h"

#include <WickedEngine.h>

namespace Defines
{
    enum class CameraAgencyEnum : uint32_t
    {
        GAMEPLAY,
        EVENT
    };
    static const float CameraLerpSpeed = 4.f;
    static const float PlayerHeightOffset = 1.5f;
    static const float PlayerZLimit = 1.f;

    static const float CameraMinYRotation = -wi::math::PI*0.18f;
    static const float CameraMaxYRotation = wi::math::PI*0.34f; 
}
namespace _internal_
{
    // Camera tracking data
    wi::ecs::Entity core_focus = wi::ecs::INVALID_ENTITY;
    wi::unordered_set<wi::ecs::Entity> sub_focus;

    // Camera agency mode
    Defines::CameraAgencyEnum camera_agency_mode = Defines::CameraAgencyEnum::GAMEPLAY;

    // Camera movement data
    float camera_z_offset = 3.0f;
    wi::scene::TransformComponent camera_transform;
    XMFLOAT3 camera_position_raw, camera_position_lerp;
    XMFLOAT3 player_movement_offset_raw, player_movement_offset_lerp;

    // Camera orientation data
    XMFLOAT2 camera_rotation_raw, camera_rotation_lerp;
}

// Lua hooks
int Camera_SetCoreFocus(lua_State* L)
{
    int argc = wi::lua::SGetArgCount(L);
    if(argc > 0)
    {
        _internal_::core_focus = wi::ecs::Entity(wi::lua::SGetLongLong(L, 1));
    }
    return 0;
}
int Camera_AddSubFocus(lua_State* L)
{
    int argc = wi::lua::SGetArgCount(L);
    if(argc > 0)
    {
        _internal_::sub_focus.insert(wi::ecs::Entity(wi::lua::SGetLongLong(L, 1)));
    }
    return 0;
}
int Camera_RemoveSubFocus(lua_State* L)
{
    int argc = wi::lua::SGetArgCount(L);
    if(argc > 0)
    {
        _internal_::sub_focus.erase(wi::ecs::Entity(wi::lua::SGetLongLong(L, 1)));
    }
    return 0;
}

void Gameplay::Camera::Hook_Init()
{
    // Add lua hook
    wi::lua::RegisterFunc("Camera_SetCoreFocus", Camera_SetCoreFocus);
    wi::lua::RegisterFunc("Camera_AddSubFocus", Camera_AddSubFocus);
    wi::lua::RegisterFunc("Camera_RemoveSubFocus", Camera_RemoveSubFocus);
}

void Gameplay::Camera::Hook_Update(float dt)
{
    if(Game::GetScene().wiscene.transforms.Contains(_internal_::core_focus) && Gameplay::GetGameplayAgencyControl())
    {
        wi::scene::CameraComponent& camera = wi::scene::GetCamera();
        _internal_::camera_transform = wi::scene::TransformComponent();

        // Check camera agency
        switch(_internal_::camera_agency_mode)
        {
            case Defines::CameraAgencyEnum::GAMEPLAY:
            {
                // Find center
                wi::scene::TransformComponent* core_focus_transform = Game::GetScene().wiscene.transforms.GetComponent(_internal_::core_focus);

                _internal_::camera_position_raw = core_focus_transform->GetPosition();
                // _internal_::camera_position_lerp = wi::math::Lerp(_internal_::camera_position_lerp, _internal_::camera_position_raw, Defines::CameraLerpSpeed*7.f*dt);
                _internal_::camera_position_lerp = _internal_::camera_position_raw;

                // Calculate player movement offset
                XMFLOAT3 camera_up_vector = XMFLOAT3(0.f,1.f,0.f); // Y-Up
                auto camera_at_vector = camera.At;
                camera_at_vector.y = 0.f;
                camera_at_vector.x = -camera_at_vector.x; // Inverse X at vector
                auto camera_rotation_matrix = XMMatrixLookAtLH(XMVECTOR(), XMLoadFloat3(&camera_at_vector), XMLoadFloat3(&camera_up_vector));
                auto camera_rotation_matrix_inv = XMMatrixInverse(nullptr, camera_rotation_matrix);

                auto player_direction_matrix = XMMatrixTranslationFromVector(XMLoadFloat3(&_internal_::player_movement_offset_lerp));
                player_direction_matrix *= camera_rotation_matrix_inv;

                // XMVECTOR decompose_temp, player_movement_offset_vec;
                // XMMatrixDecompose(&decompose_temp, &decompose_temp, &player_movement_offset_vec, player_direction_matrix);
                // XMStoreFloat3(&final_player_movement_offset, player_movement_offset_vec);

                // player_direction_matrix = XMMatrixTranslationFromVector(XMLoadFloat3(&final_player_movement_offset));
                // player_direction_matrix *= camera_rotation_matrix;
                // XMMatrixDecompose(&decompose_temp, &decompose_temp, &player_movement_offset_vec, player_direction_matrix);
                // XMStoreFloat3(&final_player_movement_offset, player_movement_offset_vec);
                // ---

                // Calculate camera orientation offset
                _internal_::camera_rotation_lerp = wi::math::Lerp(
                    _internal_::camera_rotation_lerp, 
                    wi::math::Lerp(
                        _internal_::camera_rotation_lerp, 
                        _internal_::camera_rotation_raw, 
                        wi::math::Clamp(30.f*dt, 0.f, 1.f)),
                    wi::math::Clamp(30.f*dt, 0.f, 1.f));
                // ---
                
                _internal_::camera_transform.Translate(
                    XMFLOAT3(_internal_::player_movement_offset_raw.x,_internal_::player_movement_offset_raw.y,-_internal_::camera_z_offset+_internal_::player_movement_offset_raw.z));
                _internal_::camera_transform.UpdateTransform();
                _internal_::camera_transform.MatrixTransform(
                    XMMatrixRotationQuaternion(XMQuaternionRotationRollPitchYaw(
                        _internal_::camera_rotation_lerp.y, 
                        _internal_::camera_rotation_lerp.x, 
                        0.f)));
                _internal_::camera_transform.UpdateTransform();
                _internal_::camera_transform.Translate(_internal_::camera_position_lerp);
                _internal_::camera_transform.Translate(XMFLOAT3(0.f,Defines::PlayerHeightOffset,0.f));
                _internal_::camera_transform.UpdateTransform();

                // Finish camera transformation
                camera.SetDirty();
                camera.TransformCamera(_internal_::camera_transform);
                camera.UpdateCamera();
                break;
            }
            case Defines::CameraAgencyEnum::EVENT:
            {
                break;
            }
            default:
                break;
        }
    }
}
void Gameplay::Camera::Hook_PreUpdate(float dt)
{

}

void Gameplay::Camera::Hook_FixedUpdate()
{

}

void Gameplay::Camera::Hook_Migrate(bool store, wi::vector<uint8_t>& storage)
{
    if(store)
    {
        auto ar_store = wi::Archive();
        ar_store << _internal_::core_focus;
        ar_store.WriteData(storage);
    }
    else
    {
        auto ar_load = wi::Archive(storage.data());
        ar_load.SetReadModeAndResetPos(true);
        ar_load >> _internal_::core_focus;
    }
}

// GBus Implementation
void Gameplay::GBus::Camera::SetPlayerMovementOffset(XMFLOAT3 offset)
{
    _internal_::player_movement_offset_raw = offset;
}
void Gameplay::GBus::Camera::AddPlayerCameraRotation(XMFLOAT2 rotation)
{
    _internal_::camera_rotation_raw.x += rotation.x;
    _internal_::camera_rotation_raw.y += rotation.y;
    _internal_::camera_rotation_raw.y = wi::math::Clamp(_internal_::camera_rotation_raw.y, Defines::CameraMinYRotation, Defines::CameraMaxYRotation);
}