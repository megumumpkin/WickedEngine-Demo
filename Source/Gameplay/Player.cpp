#include "Player.h"

#include "Hook.h"
#include "GameplayAgencyControl.h"
#include "GBus.h"
#include "Helpers.h"
#include "Components.h"
#include "../Scene.h"
#include "../Core.h"

#include <WickedEngine.h>

// local constants
namespace Defines
{
    // Input defines
    enum class InputSrcEnum : uint32_t
    {
        MOUSE_KEYBOARD,
        MOUSE_MOVE,
        MOUSE_SCROLL,
        GAMEPAD_BUTTON,
        GAMEPAD_AXIS
    };
    enum class InputEnum : uint32_t
    {
        AXES_MOVE,
        AXES_CAMERA,
        ACT_JUMP,
        ACT_ATTACK1,
        ACT_ATTACK2,
        MOD_RUN,
        MOD_DASH,
        TGL_WALK,
        AXES_CAMERA_MOUSE_SCROLL,
        ACT_SWITCH_WEAPON
    };

    // Animation defines
    enum class AnimationEnum : uint32_t
    {
        Player_Anim_Idle,
        Player_Anim_Walk,
        Player_Anim_Jog,
        Player_Anim_Run,
        Player_Anim_Jump_Still,
        Player_Anim_Jump_Forward,
        Player_Anim_Jump_Roll,
        Player_Anim_Fall_Still,
        Player_Anim_Fall_Forward,
        Player_Anim_Dash,
        Player_Anim_GunShoot,
        Player_Anim_Override_Hip,
        Player_Anim_Override_Head,
        Player_Anim_SwordSwingNormal1,
        Player_Anim_SwordSwingNormal2,
        Player_Anim_SwordThrust1,
        Player_Anim_SwordUpSwing,
        Player_Anim_SwordDownThrust,
        Player_Anim_SwordSpinVertical,
        Player_Anim_SwordDownStrikeStuck,
        Player_Anim_SwordUnstuckActionLead,
        Player_Anim_SwordUnstuckActionToGun,
    };
    wi::unordered_map<AnimationEnum, std::string> AnimationStrings = {
        {AnimationEnum::Player_Anim_Idle, "Player_Anim_Idle"},
        {AnimationEnum::Player_Anim_Walk, "Player_Anim_Walk"},
        {AnimationEnum::Player_Anim_Jog, "Player_Anim_Jog"},
        {AnimationEnum::Player_Anim_Run, "Player_Anim_Run"},
        {AnimationEnum::Player_Anim_Jump_Still, "Player_Anim_Jump_Still"},
        {AnimationEnum::Player_Anim_Jump_Forward, "Player_Anim_Jump_Forward"},
        {AnimationEnum::Player_Anim_Jump_Roll, "Player_Anim_Jump_Roll"},
        {AnimationEnum::Player_Anim_Fall_Still, "Player_Anim_Fall_Still"},
        {AnimationEnum::Player_Anim_Fall_Forward, "Player_Anim_Fall_Forward"},
        {AnimationEnum::Player_Anim_Dash, "Player_Anim_Dash"},
        {AnimationEnum::Player_Anim_GunShoot, "Player_Anim_GunShoot"},
        {AnimationEnum::Player_Anim_Override_Hip, "Player_Anim_Override_Hip"},
        {AnimationEnum::Player_Anim_Override_Head, "Player_Anim_Override_Head"},
        {AnimationEnum::Player_Anim_SwordSwingNormal1, "Player_Anim_SwordSwingNormal1"},
        {AnimationEnum::Player_Anim_SwordSwingNormal2, "Player_Anim_SwordSwingNormal2"},
        {AnimationEnum::Player_Anim_SwordThrust1, "Player_Anim_SwordThrust1"},
        {AnimationEnum::Player_Anim_SwordUpSwing, "Player_Anim_SwordUpSwing"},
        {AnimationEnum::Player_Anim_SwordDownThrust, "Player_Anim_SwordDownThrust"},
        {AnimationEnum::Player_Anim_SwordSpinVertical, "Player_Anim_SwordSpinVertical"},
        {AnimationEnum::Player_Anim_SwordDownStrikeStuck, "Player_Anim_SwordDownStrikeStuck"},
        {AnimationEnum::Player_Anim_SwordUnstuckActionLead, "Player_Anim_SwordUnstuckActionLead"},
        {AnimationEnum::Player_Anim_SwordUnstuckActionToGun, "Player_Anim_SwordUnstuckActionToGun"},
    };
    wi::unordered_map<AnimationEnum, float> AnimationWCycleSpeeds = {
        {AnimationEnum::Player_Anim_Walk, 1.f},
        {AnimationEnum::Player_Anim_Jog, 30.f/60.f},
        {AnimationEnum::Player_Anim_Run, 30.f/60.f},
    };

    namespace FilesystemPaths
    {
        std::string Resource_Guns = "content/Props/Weapons/Guns/";
        std::string Resource_Swords = "content/Props/Weapons/Swords/";
    }
    enum class GunEnum : uint32_t
    {
        Prototyping_Gun
    };
    wi::unordered_map<GunEnum, std::string> GunStrings = {
        {GunEnum::Prototyping_Gun, "Prototyping_Gun"},
    };
    enum class SwordEnum : uint32_t
    {
        Prototyping_Sword
    };
    wi::unordered_map<SwordEnum, std::string> SwordStrings = {
        {SwordEnum::Prototyping_Sword, "Prototyping_Sword"},
    };
    enum class WeaponUseEnum : uint32_t
    {
        GUN_ACTIVE,
        SWORD_ACTIVE
    };

    // Runtime defines
    namespace Runtime
    {
        // Input
        static const float KeyAxeTransition = 7.f;
        static const float AxeDeadZone = 0.1f;
        // Camera Control
        static const float CamZoomMin = 2.4f;
        static const float CamZoomMax = -5.f;
        // Speed caps
        static const float WalkSpeedCap = 2.3f;
        static const float JogSpeedCap = 9.f;
        static const float RunSpeedCap = 14.f;
        static const float DashSpeed = 17.f;
        static const float SlashSpeed = 12.f;
        // Anim transitions
        static const float AnimTransitionWalk = 2.3f;
        static const float AnimTransitionJog = 7.8f;
        static const float AnimTransitionRun = 14.f;
        static const float AnimSpeedWalk = 0.9f;
        static const float AnimSpeedJog = 1.8f;
        static const float AnimSpeedRun = 2.5f;
        static const float AnimSpeedRunEXTransition = 18.f;
        static const float AnimSpeedRunEX = 3.f;

        // Gravity
        static const float BaseTerminalVelocity = -20.f;
        static const float JumpVelocity = 9.f;
        // Default vectors
        static const XMFLOAT3 UpVector = XMFLOAT3(0.f,1.f,0.f);
        static const XMFLOAT3 ForwardVector = XMFLOAT3(0.f,0.f,1.f);
        static const XMFLOAT3 SideVector = XMFLOAT3(1.f,0.f,0.f);

    }

    // Action Defines
    enum class ActionEnum
    {
        IDLE,
        MOVE,
        JUMP,
        DASH,
        SHOOT,
        SLASH
    };
}
namespace _internal_
{
    namespace Init_State
    {
        wi::ecs::Entity playerID = wi::ecs::INVALID_ENTITY;
    }
    namespace Anim_State
    {
        namespace EventMSG
        {
            // int Jump = false;
            namespace Move
            {
                float transition_lerp = 0.f;
                float move_transition = 0.f; // 0.f = Walking, 1.f = Jogging, 2.f = Running
                float move_speed = 0.f;
                float dash_lerp = 0.f;
            }
            namespace Jump
            {
                bool jump_init = false;
                float transition_lerp = 0.f;
                float air_forwardness = 0.f; // 0.f = Still, 1.f = Forward
                float air_forwardness_start = 0.f;
                float air_fall_transition = 0.f; // 0.f = Jumping, 1.f = Falling
                float air_fall_transition_start = 0.f;
                float air_fall_transition_lerp = 0.f; // Smoothed
                float jump_timer = 0.f;
                float roll_timer = 0.f; // Jump roll status
                float roll_lerp = 0.f;
            }
            namespace Dash
            {
                wi::scene::ObjectComponent* plasma_object;
                wi::EmittedParticleSystem* spark_emitter;
                wi::EmittedParticleSystem* ripple_emitter;

                float vfx_lerp = 0.f;
                float object_plasma_original_alpha = 0.f;
                float object_plasma_original_emitalpha = 0.f;
                int particle_spark_original_count = 0;
                int particle_ripple_original_count = 0;
            }
            namespace Shoot
            {
                float transition_lerp = 0.f;
            }
            namespace Sword
            {
                float transition_lerp = 0.f;

                bool cancellable = true;
                bool enable_hitbox = false;
                bool slash_reset = false;
                Defines::AnimationEnum current_animation = Defines::AnimationEnum::Player_Anim_SwordSwingNormal1;
                wi::unordered_map<Defines::AnimationEnum, float> slash_lerps = {
                    {Defines::AnimationEnum::Player_Anim_SwordSwingNormal1, 0.f},
                    {Defines::AnimationEnum::Player_Anim_SwordSwingNormal2, 0.f},
                    {Defines::AnimationEnum::Player_Anim_SwordThrust1, 0.f},
                    {Defines::AnimationEnum::Player_Anim_SwordUpSwing, 0.f},
                    {Defines::AnimationEnum::Player_Anim_SwordDownThrust, 0.f},
                    {Defines::AnimationEnum::Player_Anim_SwordSpinVertical, 0.f},
                    {Defines::AnimationEnum::Player_Anim_SwordDownStrikeStuck, 0.f},
                    {Defines::AnimationEnum::Player_Anim_SwordUnstuckActionLead, 0.f},
                    {Defines::AnimationEnum::Player_Anim_SwordUnstuckActionToGun, 0.f},
                };
                wi::unordered_map<Defines::AnimationEnum, Gameplay::Helpers::GradientMap> slash_hitcancel_timing = {};
            }
        }
    }
    namespace Input_State
    {
        // Uses internal predefined input IDs to start
        wi::unordered_map<Defines::InputEnum, std::vector<std::pair<Defines::InputSrcEnum,uint32_t>>> InputMap = {
            {Defines::InputEnum::AXES_MOVE, {
                {Defines::InputSrcEnum::MOUSE_KEYBOARD, wi::input::BUTTON('A')},
                {Defines::InputSrcEnum::MOUSE_KEYBOARD, wi::input::BUTTON('D')},
                {Defines::InputSrcEnum::MOUSE_KEYBOARD, wi::input::BUTTON('W')},
                {Defines::InputSrcEnum::MOUSE_KEYBOARD, wi::input::BUTTON('S')},
                {Defines::InputSrcEnum::GAMEPAD_AXIS, wi::input::GAMEPAD_ANALOG_THUMBSTICK_L}
            }},
            {Defines::InputEnum::ACT_JUMP, {
                {Defines::InputSrcEnum::MOUSE_KEYBOARD, wi::input::KEYBOARD_BUTTON_SPACE},
                {Defines::InputSrcEnum::GAMEPAD_BUTTON, wi::input::GAMEPAD_BUTTON_2}
            }},
            {Defines::InputEnum::ACT_ATTACK1, {
                {Defines::InputSrcEnum::MOUSE_KEYBOARD, wi::input::MOUSE_BUTTON_LEFT},
                {Defines::InputSrcEnum::GAMEPAD_BUTTON, wi::input::GAMEPAD_BUTTON_1}
            }},
            {Defines::InputEnum::ACT_ATTACK2, {
                {Defines::InputSrcEnum::MOUSE_KEYBOARD, wi::input::MOUSE_BUTTON_LEFT},
                {Defines::InputSrcEnum::GAMEPAD_BUTTON, wi::input::GAMEPAD_BUTTON_4}
            }},
            {Defines::InputEnum::MOD_RUN, {
                {Defines::InputSrcEnum::MOUSE_KEYBOARD, wi::input::KEYBOARD_BUTTON_LSHIFT},
                {Defines::InputSrcEnum::GAMEPAD_AXIS, wi::input::GAMEPAD_ANALOG_TRIGGER_L}
            }},
            {Defines::InputEnum::MOD_DASH, {
                {Defines::InputSrcEnum::MOUSE_KEYBOARD, wi::input::BUTTON('F')},
                {Defines::InputSrcEnum::GAMEPAD_AXIS, wi::input::GAMEPAD_ANALOG_TRIGGER_R}
            }},
            {Defines::InputEnum::TGL_WALK, {
                {Defines::InputSrcEnum::MOUSE_KEYBOARD, wi::input::KEYBOARD_BUTTON_LCONTROL}
            }},
            {Defines::InputEnum::AXES_CAMERA, {
                {Defines::InputSrcEnum::MOUSE_KEYBOARD, wi::input::BUTTON(wi::input::KEYBOARD_BUTTON_LEFT)},
                {Defines::InputSrcEnum::MOUSE_KEYBOARD, wi::input::BUTTON(wi::input::KEYBOARD_BUTTON_RIGHT)},
                {Defines::InputSrcEnum::MOUSE_KEYBOARD, wi::input::BUTTON(wi::input::KEYBOARD_BUTTON_UP)},
                {Defines::InputSrcEnum::MOUSE_KEYBOARD, wi::input::BUTTON(wi::input::KEYBOARD_BUTTON_DOWN)},
                {Defines::InputSrcEnum::MOUSE_MOVE, wi::input::BUTTON_NONE},
                {Defines::InputSrcEnum::GAMEPAD_AXIS, wi::input::GAMEPAD_ANALOG_THUMBSTICK_R}
            }},
            {Defines::InputEnum::AXES_CAMERA_MOUSE_SCROLL, {
                {Defines::InputSrcEnum::MOUSE_SCROLL, wi::input::BUTTON_NONE}
            }},
            {Defines::InputEnum::ACT_SWITCH_WEAPON, {
                {Defines::InputSrcEnum::GAMEPAD_BUTTON, wi::input::GAMEPAD_BUTTON_DOWN},
                {Defines::InputSrcEnum::MOUSE_KEYBOARD, wi::input::BUTTON('Q')}
            }}
        };
    }
    namespace Runtime_State
    {
        bool scenedata_ready = false;
        float TimeDelta = 0.f;

        // Camera Control
        float zoom = 0.f;
        XMFLOAT3 sidle_lerp = XMFLOAT3();

        // Ground movement states
        bool toggled_walk = false;
        float dynamic_speed_cap_raw = 10.f;
        float dynamic_speed_cap = 10.f;
        XMFLOAT4 direction_quaternion = XMFLOAT4(0.f,0.f,0.f,1.f);
        XMFLOAT4 direction_quaternion_lerp = direction_quaternion;
        XMFLOAT4 tilt_quaternion = XMFLOAT4(0.f,0.f,0.f,1.f);

        // Air control states
        bool jump = false;
        float dynamic_terminal_velocity = Defines::Runtime::BaseTerminalVelocity;
        XMFLOAT2 static_h_velocity = XMFLOAT2();
        XMFLOAT2 h_velocity_offset = XMFLOAT2();

        // Dashing control states
        bool dash = false;
        float dash_timer = 0.f;
        float dash_timer_cooldown = 0.f;
        bool air_dash_done = false;

        // Physics data
        float h_velocity;
        float jump_potential = 0.f;
        XMFLOAT3 velocity;// = XMFLOAT3(0.f, Defines::Runtime::BaseTerminalVelocity, 0.f);
        wi::primitive::Capsule collider_shape;
        wi::scene::TransformComponent collider_position;
        // Ground constraint physics data
        wi::ecs::Entity ground_entity = wi::ecs::INVALID_ENTITY;
        wi::scene::TransformComponent prev_ground_transform = wi::scene::TransformComponent();
        // --

        // Player camera states
        bool mouse_state_lock = false;
        XMFLOAT4 locked_mouse_pointer;
        XMFLOAT2 camera_orient_vec;
        
        // Player Weaponry
        wi::ecs::Entity weapon_hold_l_entity;
        wi::ecs::Entity weapon_hold_r_entity;
        wi::ecs::Entity weapon_grip_l_entity;
        wi::ecs::Entity weapon_grip_r_entity;
        Defines::GunEnum current_gun = Defines::GunEnum::Prototyping_Gun;
        Defines::SwordEnum current_sword = Defines::SwordEnum::Prototyping_Sword;
        Defines::WeaponUseEnum current_use = Defines::WeaponUseEnum::GUN_ACTIVE;
        bool weapon_active = false;

        // Player Attacks
        bool shoot = false;
        float shoot_timer = 30.f;
        XMFLOAT4 hip_aim_rotation;
        bool slash = false;
        float slash_timer = 0.f;
        uint32_t air_slash_counter = 0;
        float upslash_window = 0.f;

        // Player Extras
        XMFLOAT4 head_look_rotation;
    }
    wi::unordered_map<Defines::AnimationEnum, wi::scene::AnimationComponent*> AnimationPtrs;
    wi::unordered_map<Defines::ActionEnum, std::function<void(float)>> Actions;
    wi::unordered_map<Defines::ActionEnum, float> ActionBlends;
    wi::unordered_map<Defines::GunEnum, wi::ecs::Entity> Guns;
    wi::unordered_map<Defines::SwordEnum, wi::ecs::Entity> Swords;
    wi::ecs::Entity projectile_program_ID = wi::ecs::INVALID_ENTITY;
}

int CreatePlayer(lua_State* L)
{
    // CreatePlayer(int playerID)
    int argc = wi::lua::SGetArgCount(L);
    if(argc > 0)
    {
        _internal_::Init_State::playerID = (wi::ecs::Entity) wi::lua::SGetLongLong(L, 1);
    }
    return 0;
}

// Input system
XMFLOAT2 InputGetAxeValue(Defines::InputEnum InputID)
{
    static wi::unordered_map<Defines::InputEnum, XMFLOAT2> cache_raw;
    static wi::unordered_map<Defines::InputEnum, XMFLOAT4> cache_lerp;

    bool button_pressed = false;
    uint32_t button_axe_id = 0;

    XMFLOAT2 result = XMFLOAT2(0.f,0.f);
    for(auto& inputvariant : _internal_::Input_State::InputMap[InputID])
    {
        switch(inputvariant.first)
        {
            case(Defines::InputSrcEnum::MOUSE_KEYBOARD):
            {
                if(wi::input::Down(wi::input::BUTTON(inputvariant.second)))
                {
                    switch(button_axe_id)
                    {
                        case 0:
                        {
                            cache_raw[InputID].x = -1.f;
                            break;
                        }
                        case 1:
                        {
                            cache_raw[InputID].x = 1.f;
                            break;
                        }
                        case 2:
                        {
                            cache_raw[InputID].y = 1.f;
                            break;
                        }
                        case 3:
                        {
                            cache_raw[InputID].y = -1.f;
                            break;
                        }
                    }
                }

                button_axe_id++;
                if(button_axe_id > 3)
                {
                    if(wi::math::Length(cache_raw[InputID]) > 0.05f)
                    {
                        auto cache_raw_vec3 = XMFLOAT3(cache_raw[InputID].x, 0.f, cache_raw[InputID].y);

                        auto transform_matrix = XMMatrixLookAtLH(XMVECTOR(), XMLoadFloat3(&cache_raw_vec3), XMLoadFloat3(&Defines::Runtime::UpVector));
                        XMVECTOR stub_vector, move_vector, rotation_vector;
                        XMMatrixDecompose(&stub_vector, &rotation_vector, &stub_vector, transform_matrix);
                        XMFLOAT4 cache_lerp_raw;
                        XMStoreFloat4(&cache_lerp_raw, rotation_vector);

                        if(cache_lerp.find(InputID) != cache_lerp.end())
                        {
                            auto angle_diff = Gameplay::Helpers::GetAngleDiff(cache_lerp[InputID], cache_lerp_raw);
                            if(abs(angle_diff) > wi::math::PI/1.7f)
                                cache_lerp.erase(InputID);
                        }
                        else
                            cache_lerp[InputID] = cache_lerp_raw;
                        cache_lerp[InputID] = wi::math::Slerp(cache_lerp[InputID], cache_lerp_raw, _internal_::Runtime_State::TimeDelta*Defines::Runtime::KeyAxeTransition);
                        
                        transform_matrix = 
                            XMMatrixTranslationFromVector(XMLoadFloat3(&Defines::Runtime::ForwardVector)) *
                            XMMatrixRotationQuaternion(XMLoadFloat4(&cache_lerp[InputID]));
                        XMMatrixDecompose(&stub_vector, &stub_vector, &move_vector, transform_matrix);
                        XMStoreFloat3(&cache_raw_vec3, move_vector);

                        result.x += cache_raw_vec3.x;
                        result.y += cache_raw_vec3.z;
                    }
                    else
                    {
                        if(cache_lerp.find(InputID) != cache_lerp.end())
                            cache_lerp.erase(InputID);
                    }
                    cache_raw[InputID] = XMFLOAT2(0.f,0.f);
                    button_axe_id = 0;
                }
                break;
            }
            case(Defines::InputSrcEnum::MOUSE_MOVE):
            {
                auto mouse_state = wi::input::GetMouseState();
                if(_internal_::Runtime_State::mouse_state_lock)
                {
                    static XMFLOAT2 last_position = XMFLOAT2(0.f,0.f);
                    result.x += mouse_state.position.x - _internal_::Runtime_State::locked_mouse_pointer.x;
                    result.y += mouse_state.position.y - _internal_::Runtime_State::locked_mouse_pointer.y;
                    // last_position = mouse_state.position;
                    wi::input::SetPointer(_internal_::Runtime_State::locked_mouse_pointer);
                    wi::input::HidePointer(true);
                }
                break;
            }
            case(Defines::InputSrcEnum::MOUSE_SCROLL):
            {
                auto mouse_state = wi::input::GetMouseState();
                if(_internal_::Runtime_State::mouse_state_lock)
                {
                    result.y += mouse_state.delta_wheel;
                }
                break;
            }
            case(Defines::InputSrcEnum::GAMEPAD_AXIS):
            {
                // Check if stick exceeds dead zone
                XMFLOAT4 axedata = wi::input::GetAnalog(wi::input::GAMEPAD_ANALOG(inputvariant.second));
                if(wi::math::Distance(XMVECTOR(),XMLoadFloat4(&axedata)) > 0.1f)
                {
                    result.x += axedata.x;
                    result.y += axedata.y;
                }
                break;
            }
            default:
                break;
        }
    }

    return result;
}
bool InputGetButtonDown(Defines::InputEnum InputID)
{
    for(auto& inputvariant : _internal_::Input_State::InputMap[InputID])
    {
        if(wi::input::Down(wi::input::BUTTON(inputvariant.second)))
            return true;
    }
    return false;
}
bool InputGetButtonPressed(Defines::InputEnum InputID)
{
    for(auto& inputvariant : _internal_::Input_State::InputMap[InputID])
    {
        if(wi::input::Press(wi::input::BUTTON(inputvariant.second)))
            return true;
    }
    return false;
}

// Adapted from wicked's sample character controller
void RepositionPlayerGround(XMFLOAT3& current_position, XMFLOAT4& direction)
{
    // Get positional data
    wi::scene::TransformComponent previous_ground_transform = _internal_::Runtime_State::prev_ground_transform;
    wi::scene::TransformComponent current_ground_transform = *Game::GetScene().wiscene.transforms.GetComponent(_internal_::Runtime_State::ground_entity);

    // Get delta difference between previous and current ground transform
    XMMATRIX ground_transform_delta = XMMatrixMultiply(
        XMLoadFloat4x4(&current_ground_transform.world), 
        XMMatrixInverse(nullptr, XMLoadFloat4x4(&previous_ground_transform.world)));
    
    // Translate new direction vector through matrix multiplication
    XMVECTOR stub_vector, new_current_position, new_direction;
    XMMatrixDecompose(&stub_vector, &new_direction, &stub_vector, 
        XMMatrixMultiply(
            XMMatrixRotationQuaternion(XMLoadFloat4(&direction)), 
            ground_transform_delta));
    XMStoreFloat4(&direction, new_direction);

    // Store new previous ground transform
    _internal_::Runtime_State::prev_ground_transform = current_ground_transform;
}
void CalculatePlayerCollision(XMFLOAT3& current_position, XMFLOAT3& velocity, float dt)
{
    XMFLOAT3 original_position = current_position;
    XMFLOAT3 original_velocity = velocity;

    int num_steps = 16;
    float substep = dt/num_steps;

    wi::primitive::Capsule capsule;
    capsule.radius = _internal_::Runtime_State::collider_shape.radius;
    wi::renderer::DrawCapsule(capsule);

    wi::primitive::Ray ray_ground;

    bool ground_intersect = false;

    while (num_steps > 0)
    {
        num_steps -= 1;

        XMFLOAT3 step;
        XMStoreFloat3(&step, XMLoadFloat3(&velocity)*substep);
        XMStoreFloat3(&current_position, XMVectorAdd(XMLoadFloat3(&current_position),XMLoadFloat3(&step)));

        capsule.base = current_position;
        XMStoreFloat3(&capsule.tip, XMVectorAdd(XMLoadFloat3(&current_position),XMLoadFloat3(&_internal_::Runtime_State::collider_shape.tip)));
        
        wi::scene::Scene::CapsuleIntersectionResult res = Game::GetScene().wiscene.Intersects(capsule, wi::enums::FILTER_NAVIGATION_MESH | wi::enums::FILTER_COLLIDER, ~0u);
        if(res.entity != wi::ecs::INVALID_ENTITY)
        {
            XMFLOAT3 up_vec = XMFLOAT3(0.f,1.f,0.f);
            auto ground_slope_vector = XMVector3Dot(XMLoadFloat3(&res.normal), XMLoadFloat3(&up_vec));
            XMFLOAT3 ground_slope_vec;
            XMStoreFloat3(&ground_slope_vec, ground_slope_vector);
            float ground_slope = (ground_slope_vec.x + ground_slope_vec.y + ground_slope_vec.z)/3.f;
            float slope_threshold = 0.5f;

            // If ground then we stop the Y velocity to zero
            if((res.normal.y > 0.f) && (ground_slope > slope_threshold))
            {
                velocity.y = -2.4f;
                current_position.y += res.depth;
                ground_intersect = true;

                // Get latest ground data
                if(_internal_::Runtime_State::ground_entity != res.entity)
                {
                    _internal_::Runtime_State::ground_entity = res.entity;
                    _internal_::Runtime_State::prev_ground_transform = *Game::GetScene().wiscene.transforms.GetComponent(res.entity);
                }

                // Manage jump state
                if(_internal_::Runtime_State::jump)
                {
                    _internal_::Runtime_State::tilt_quaternion = _internal_::Runtime_State::direction_quaternion_lerp;
                    _internal_::Runtime_State::jump = false;
                    _internal_::Runtime_State::air_dash_done = false;
                }
            }
            else if(ground_slope <= slope_threshold)
            {
                // Slide on contact surface
                XMFLOAT3 static_h_velocity_vec3 = XMFLOAT3(_internal_::Runtime_State::static_h_velocity.x, 0.f, _internal_::Runtime_State::static_h_velocity.y);
                XMFLOAT3 h_velocity_offset_vec3 = XMFLOAT3(_internal_::Runtime_State::h_velocity_offset.x, 0.f, _internal_::Runtime_State::h_velocity_offset.y);
                
                XMFLOAT3* velocity_ptr = nullptr;
                for(int i = 0; i < 3; ++i)
                {
                    switch(i)
                    {
                        case 0: velocity_ptr = &velocity; break;
                        case 1: velocity_ptr = &static_h_velocity_vec3; break;
                        case 2: velocity_ptr = &h_velocity_offset_vec3; break;
                        default: velocity_ptr = &velocity; break;
                    }

                    float velocity_length = wi::math::Length(*velocity_ptr);
                    auto velocity_normalized = XMVector3Normalize(XMLoadFloat3(velocity_ptr));
                    auto undesired_motion = XMVector3Dot(velocity_normalized, XMLoadFloat3(&res.normal));
                    undesired_motion = XMVectorMultiply(XMLoadFloat3(&res.normal), undesired_motion);
                    auto desired_motion = XMVectorSubtract(velocity_normalized, undesired_motion);
                    XMFLOAT3 desired_motion_vec;
                    XMStoreFloat3(&desired_motion_vec, desired_motion);
                    if(ground_intersect)
                        desired_motion_vec.y = 0;
                    auto velocity_vector = XMLoadFloat3(&desired_motion_vec)*velocity_length;
                    XMStoreFloat3(velocity_ptr, velocity_vector);
                    if(i == 0)
                    {
                        auto current_position_vector = XMVectorAdd(XMLoadFloat3(&current_position), XMLoadFloat3(&res.normal)*res.depth);
                        XMStoreFloat3(&current_position, current_position_vector);
                    }
                }

                velocity.y = original_velocity.y;

                _internal_::Runtime_State::static_h_velocity.x = static_h_velocity_vec3.x;
                _internal_::Runtime_State::static_h_velocity.y = static_h_velocity_vec3.z;

                _internal_::Runtime_State::h_velocity_offset.x = h_velocity_offset_vec3.x;
                _internal_::Runtime_State::h_velocity_offset.y = h_velocity_offset_vec3.z;

                // Cut y velocity if the contact point is a ceiling
                if(res.normal.y < 0.f)
                    velocity.y = 0.f;
            }
        }

        if(ground_intersect)
        {
            // If ground then we find a way to keep the object snap to ground every iteration
            auto rg_p_a = current_position;
            rg_p_a.y += 0.3f;
            auto rg_p_b = current_position;
            rg_p_b.y -= 0.3f;
            ray_ground.CreateFromPoints(rg_p_a, rg_p_b);
            wi::scene::Scene::RayIntersectionResult res2 = Game::GetScene().wiscene.Intersects(ray_ground, wi::enums::FILTER_NAVIGATION_MESH | wi::enums::FILTER_COLLIDER, ~0u);
            if(res2.entity != wi::ecs::INVALID_ENTITY)
                current_position.y = res2.position.y;
        }
    }

    float y_slope_diff = current_position.y - original_position.y;
    float h_speed = wi::math::Length(XMFLOAT2(velocity.x, velocity.z));
    Gameplay::Helpers::GradientMap h_speed_gradient({{0.f,0.f},{3.f,1.f}});
    if(y_slope_diff >= 0.f)
        _internal_::Runtime_State::jump_potential = y_slope_diff*20.f*h_speed_gradient.Get(h_speed);

    if(!ground_intersect && !_internal_::Runtime_State::jump)
    {
        _internal_::Runtime_State::velocity.y = 0.f;
        _internal_::Runtime_State::jump = true;
        _internal_::Runtime_State::static_h_velocity.x = velocity.x;
        _internal_::Runtime_State::static_h_velocity.y = velocity.z;
    }

    if(!ground_intersect)
        _internal_::Runtime_State::ground_entity = wi::ecs::INVALID_ENTITY;
}

// Hook stuff here
void Gameplay::Player::Hook_Init()
{
    // Add lua hook
    wi::lua::RegisterFunc("CreatePlayer", CreatePlayer);

    // Add actions
    _internal_::Actions[Defines::ActionEnum::IDLE] = [=](float transition)
    {
        if(transition > 0.f)
        {
            _internal_::AnimationPtrs[Defines::AnimationEnum::Player_Anim_Idle]->Play();
            _internal_::AnimationPtrs[Defines::AnimationEnum::Player_Anim_Idle]->amount = transition;
        }
        else
        {
            _internal_::AnimationPtrs[Defines::AnimationEnum::Player_Anim_Idle]->Stop();
        }
    };
    _internal_::Actions[Defines::ActionEnum::MOVE] = [=](float transition)
    {
        if(transition > 0.f)
        {
            float anim_transition = _internal_::Anim_State::EventMSG::Move::move_transition;
            float anim_speed = _internal_::Anim_State::EventMSG::Move::move_speed;
            _internal_::AnimationPtrs[Defines::AnimationEnum::Player_Anim_Walk]->Play();
            _internal_::AnimationPtrs[Defines::AnimationEnum::Player_Anim_Jog]->Play();
            _internal_::AnimationPtrs[Defines::AnimationEnum::Player_Anim_Run]->Play();
            _internal_::AnimationPtrs[Defines::AnimationEnum::Player_Anim_Walk]->speed = Defines::AnimationWCycleSpeeds[Defines::AnimationEnum::Player_Anim_Walk]*anim_speed;
            _internal_::AnimationPtrs[Defines::AnimationEnum::Player_Anim_Jog]->speed = Defines::AnimationWCycleSpeeds[Defines::AnimationEnum::Player_Anim_Jog]*anim_speed;
            _internal_::AnimationPtrs[Defines::AnimationEnum::Player_Anim_Run]->speed = Defines::AnimationWCycleSpeeds[Defines::AnimationEnum::Player_Anim_Run]*anim_speed;

            
            // Fuzzy transitions
            static Gameplay::Helpers::GradientMap walk_gradient({{0.f,1.f},{1.f,0.f}});
            _internal_::AnimationPtrs[Defines::AnimationEnum::Player_Anim_Walk]->amount = //transition;
                walk_gradient.Get(anim_transition)*transition;

            static Gameplay::Helpers::GradientMap jog_gradient({{0.f,0.f},{1.f,1.f},{2.f,0.f}});
            _internal_::AnimationPtrs[Defines::AnimationEnum::Player_Anim_Jog]->amount = 
                jog_gradient.Get(anim_transition)*transition;

            static Gameplay::Helpers::GradientMap run_gradient({{1.f,0.f},{2.f,1.f}});
            _internal_::AnimationPtrs[Defines::AnimationEnum::Player_Anim_Run]->amount = 
                run_gradient.Get(anim_transition)*transition;
        }
        else
        {
            _internal_::AnimationPtrs[Defines::AnimationEnum::Player_Anim_Walk]->Stop();
            _internal_::AnimationPtrs[Defines::AnimationEnum::Player_Anim_Jog]->Stop();
            _internal_::AnimationPtrs[Defines::AnimationEnum::Player_Anim_Run]->Stop();
        }
    };
    _internal_::Actions[Defines::ActionEnum::JUMP] = [=](float transition)
    {
        if(transition > 0.f)
        {
            // Jump roll conditions
            if(!_internal_::Anim_State::EventMSG::Jump::jump_init)
            {
                _internal_::Anim_State::EventMSG::Jump::air_forwardness_start = _internal_::Anim_State::EventMSG::Jump::air_forwardness;
                _internal_::Anim_State::EventMSG::Jump::air_fall_transition_start = _internal_::Anim_State::EventMSG::Jump::air_fall_transition;
                _internal_::Anim_State::EventMSG::Jump::jump_init = true;
            }
            _internal_::Anim_State::EventMSG::Jump::jump_timer += _internal_::Runtime_State::TimeDelta;

            // Make sure the animation is non looping
            _internal_::AnimationPtrs[Defines::AnimationEnum::Player_Anim_Jump_Still]->SetLooped(false);
            _internal_::AnimationPtrs[Defines::AnimationEnum::Player_Anim_Jump_Forward]->SetLooped(false);
            _internal_::AnimationPtrs[Defines::AnimationEnum::Player_Anim_Fall_Still]->SetLooped(false);
            _internal_::AnimationPtrs[Defines::AnimationEnum::Player_Anim_Fall_Forward]->SetLooped(false);
            _internal_::AnimationPtrs[Defines::AnimationEnum::Player_Anim_Jump_Roll]->SetLooped(false);

            _internal_::AnimationPtrs[Defines::AnimationEnum::Player_Anim_Jump_Still]->Play();
            _internal_::AnimationPtrs[Defines::AnimationEnum::Player_Anim_Jump_Forward]->Play();
            _internal_::AnimationPtrs[Defines::AnimationEnum::Player_Anim_Fall_Still]->Play();
            _internal_::AnimationPtrs[Defines::AnimationEnum::Player_Anim_Fall_Forward]->Play();

            // Jump roll check
            bool do_roll = (_internal_::Anim_State::EventMSG::Jump::jump_timer > 0.2f
                && !_internal_::Runtime_State::dash
                && !_internal_::Runtime_State::air_dash_done
                && _internal_::Anim_State::EventMSG::Jump::air_forwardness_start > 0.9f
                && _internal_::Anim_State::EventMSG::Jump::air_fall_transition_start > 0.6f
                && _internal_::Anim_State::EventMSG::Jump::roll_timer < 25.f/60.f);
            if(do_roll)
            {
                _internal_::AnimationPtrs[Defines::AnimationEnum::Player_Anim_Jump_Roll]->Play();
                _internal_::Anim_State::EventMSG::Jump::roll_timer += _internal_::Runtime_State::TimeDelta;
                if(_internal_::Anim_State::EventMSG::Jump::roll_lerp < 0.07f)
                {
                    _internal_::AnimationPtrs[Defines::AnimationEnum::Player_Anim_Jump_Roll]->timer = 0.f;
                }
            }
            _internal_::Anim_State::EventMSG::Jump::roll_lerp = wi::math::Lerp(
                _internal_::Anim_State::EventMSG::Jump::roll_lerp,
                float(do_roll),
                std::min(_internal_::Runtime_State::TimeDelta*7.f,1.f)
            );
        
            _internal_::AnimationPtrs[Defines::AnimationEnum::Player_Anim_Jump_Still]->amount = 
                transition*(1.f - _internal_::Anim_State::EventMSG::Jump::roll_lerp)*(1.f - _internal_::Anim_State::EventMSG::Jump::air_forwardness)*_internal_::Anim_State::EventMSG::Jump::air_fall_transition_lerp;
            _internal_::AnimationPtrs[Defines::AnimationEnum::Player_Anim_Jump_Forward]->amount = 
                transition*(1.f - _internal_::Anim_State::EventMSG::Jump::roll_lerp)*_internal_::Anim_State::EventMSG::Jump::air_forwardness*_internal_::Anim_State::EventMSG::Jump::air_fall_transition_lerp;
            _internal_::AnimationPtrs[Defines::AnimationEnum::Player_Anim_Fall_Still]->amount = 
                transition*(1.f - _internal_::Anim_State::EventMSG::Jump::roll_lerp)*(1.f - _internal_::Anim_State::EventMSG::Jump::air_forwardness)*(1.f -_internal_::Anim_State::EventMSG::Jump::air_fall_transition_lerp);
            _internal_::AnimationPtrs[Defines::AnimationEnum::Player_Anim_Fall_Forward]->amount = 
                transition*(1.f - _internal_::Anim_State::EventMSG::Jump::roll_lerp)*_internal_::Anim_State::EventMSG::Jump::air_forwardness*(1.f -_internal_::Anim_State::EventMSG::Jump::air_fall_transition_lerp);
        
            _internal_::AnimationPtrs[Defines::AnimationEnum::Player_Anim_Jump_Roll]->amount = transition*_internal_::Anim_State::EventMSG::Jump::roll_lerp;
        }
        else
        {
            // When the animations end then we stop them
            _internal_::AnimationPtrs[Defines::AnimationEnum::Player_Anim_Jump_Still]->Stop();
            _internal_::AnimationPtrs[Defines::AnimationEnum::Player_Anim_Jump_Forward]->Stop();
            _internal_::AnimationPtrs[Defines::AnimationEnum::Player_Anim_Fall_Still]->Stop();
            _internal_::AnimationPtrs[Defines::AnimationEnum::Player_Anim_Fall_Forward]->Stop();
            _internal_::AnimationPtrs[Defines::AnimationEnum::Player_Anim_Jump_Roll]->Stop();
        }
        if(!_internal_::Runtime_State::jump)
        {
            // Reset anim init state
            _internal_::Anim_State::EventMSG::Jump::jump_init = false;
            _internal_::Anim_State::EventMSG::Jump::jump_timer = 0.f;
            _internal_::Anim_State::EventMSG::Jump::roll_timer = 0.f;
            _internal_::AnimationPtrs[Defines::AnimationEnum::Player_Anim_Jump_Roll]->Stop();

        }
    };
    _internal_::Actions[Defines::ActionEnum::DASH] = [=](float transition)
    {
        if(transition > 0.f)
        {
            // Dash VFX animation control
            _internal_::Anim_State::EventMSG::Dash::vfx_lerp = transition > 0.5f ? 
                _internal_::Anim_State::EventMSG::Dash::vfx_lerp > 0.99f ? 1.f :
                wi::math::Lerp(
                    _internal_::Anim_State::EventMSG::Dash::vfx_lerp,
                    1.f,
                    _internal_::Runtime_State::TimeDelta*30.f
                ) :
                0.f;

            // Dash core animation
            _internal_::AnimationPtrs[Defines::AnimationEnum::Player_Anim_Dash]->SetLooped(false);
            _internal_::AnimationPtrs[Defines::AnimationEnum::Player_Anim_Dash]->Play();
            _internal_::AnimationPtrs[Defines::AnimationEnum::Player_Anim_Dash]->amount = transition;
        }
        else
        {
            // Dash VFX animation control
            _internal_::Anim_State::EventMSG::Dash::vfx_lerp = 0.f;

            // Dash core animation
            _internal_::AnimationPtrs[Defines::AnimationEnum::Player_Anim_Dash]->Stop();
        }
        // Dash vfx animation
        Gameplay::Helpers::GradientMap object_plasma_alpha_gradient({{0.f,0.f},{1.f,_internal_::Anim_State::EventMSG::Dash::object_plasma_original_alpha*5.f}});
        _internal_::Anim_State::EventMSG::Dash::plasma_object->color.w = object_plasma_alpha_gradient.Get(_internal_::Anim_State::EventMSG::Dash::vfx_lerp);

        Gameplay::Helpers::GradientMap object_plasma_emitalpha_gradient({{0.f,0.f},{1.f,_internal_::Anim_State::EventMSG::Dash::object_plasma_original_emitalpha*5.f}});
        _internal_::Anim_State::EventMSG::Dash::plasma_object->emissiveColor.w = object_plasma_emitalpha_gradient.Get(_internal_::Anim_State::EventMSG::Dash::vfx_lerp);

        Gameplay::Helpers::GradientMap particle_spark_emitter_gradient({{0.f,0.f},{1.f,_internal_::Anim_State::EventMSG::Dash::particle_spark_original_count*10.f}});
        _internal_::Anim_State::EventMSG::Dash::spark_emitter->count = particle_spark_emitter_gradient.Get(_internal_::Anim_State::EventMSG::Dash::vfx_lerp);
    
        Gameplay::Helpers::GradientMap particle_ripple_emitter_gradient({{0.f,0.f},{1.f,_internal_::Anim_State::EventMSG::Dash::particle_ripple_original_count*10.f}});
        _internal_::Anim_State::EventMSG::Dash::ripple_emitter->count = particle_ripple_emitter_gradient.Get(_internal_::Anim_State::EventMSG::Dash::vfx_lerp);
    };
    _internal_::Actions[Defines::ActionEnum::SHOOT] = [=](float transition)
    {
        // Gun animation
        _internal_::AnimationPtrs[Defines::AnimationEnum::Player_Anim_GunShoot]->Play();
        _internal_::AnimationPtrs[Defines::AnimationEnum::Player_Anim_GunShoot]->speed = 0.f;
        _internal_::AnimationPtrs[Defines::AnimationEnum::Player_Anim_GunShoot]->amount = transition;
    
        _internal_::AnimationPtrs[Defines::AnimationEnum::Player_Anim_Override_Hip]->Play();
        _internal_::AnimationPtrs[Defines::AnimationEnum::Player_Anim_Override_Hip]->speed = 0.f;
        _internal_::AnimationPtrs[Defines::AnimationEnum::Player_Anim_Override_Hip]->amount = transition;

        // Gun aim get
        wi::primitive::Ray screen_ray = wi::renderer::GetPickRay(int(Game::GetRenderPipeline()->width/2), int(Game::GetRenderPipeline()->height/2), *Game::GetRenderPipeline());
        wi::scene::Scene::RayIntersectionResult aim_res = Game::GetScene().wiscene.Intersects(screen_ray, wi::enums::FILTER_NAVIGATION_MESH | wi::enums::FILTER_COLLIDER, ~0u);

        if(aim_res.entity == wi::ecs::INVALID_ENTITY)
        {
            XMVECTOR far_position = XMLoadFloat3(&screen_ray.origin) + XMLoadFloat3(&screen_ray.direction)*100.f;
            XMStoreFloat3(&aim_res.position, far_position);
        }

        wi::ecs::Entity hip_entity = _internal_::AnimationPtrs[Defines::AnimationEnum::Player_Anim_Override_Hip]->channels[0].target;
        wi::ecs::Entity hip_parent_entity = Game::GetScene().wiscene.hierarchy.GetComponent(hip_entity)->parentID;

        wi::ecs::Entity hip_override_data_id = _internal_::AnimationPtrs[Defines::AnimationEnum::Player_Anim_Override_Hip]->samplers[0].data;
        wi::scene::AnimationDataComponent* hip_override_data = Game::GetScene().wiscene.animation_datas.GetComponent(hip_override_data_id);
        
        wi::scene::TransformComponent* hip_transform = Game::GetScene().wiscene.transforms.GetComponent(hip_entity);
        wi::scene::TransformComponent* hip_parent_transform = Game::GetScene().wiscene.transforms.GetComponent(hip_parent_entity);

        XMFLOAT3 hip_world_position = hip_transform->GetPosition();
        XMVECTOR position_diff = XMLoadFloat3(&aim_res.position) - XMLoadFloat3(&hip_world_position) - XMLoadFloat3(&Defines::Runtime::UpVector)*0.35f;
        XMFLOAT3 position_diff_vec3;
        XMStoreFloat3(&position_diff_vec3, position_diff);
        
        XMVECTOR angle_hori_normal = XMVector3Normalize(position_diff*XMVectorSet(1.f, 0.f, 1.f, 1.f));
        XMVECTOR angle_vert_normal = XMVector3Normalize(position_diff);
        XMFLOAT3 angle_vert_normal_vec3;
        XMStoreFloat3(&angle_vert_normal_vec3, angle_vert_normal);
        angle_vert_normal_vec3.x = 0.f;
        angle_vert_normal_vec3.z = 1.f - angle_vert_normal_vec3.y;
        // angle_vert_normal_vec3.y *= -1.f;
        angle_vert_normal = XMLoadFloat3(&angle_vert_normal_vec3);

        float horizontal_angle = wi::math::GetAngle(XMLoadFloat3(&Defines::Runtime::ForwardVector)*-1.f, angle_hori_normal, XMLoadFloat3(&Defines::Runtime::UpVector));
        float vertical_angle = wi::math::GetAngle(XMLoadFloat3(&Defines::Runtime::ForwardVector)*1.f, angle_vert_normal, XMLoadFloat3(&Defines::Runtime::SideVector));

        float aim_distance = wi::math::Length(position_diff_vec3);
        static Gameplay::Helpers::GradientMap aim_distance_correction ({{0.f,XM_PI/2.7f},{3.5f,0.f}});
        float vertical_angle_abs = (vertical_angle) > XM_PI/2*3 ? 0.f : vertical_angle;
        static Gameplay::Helpers::GradientMap vertical_angle_abs_remap ({{0.f,0.f},{XM_PI/6.f,1.f}});
        float corrective_angle = vertical_angle_abs_remap.Get(vertical_angle_abs);

        XMVECTOR stub_vector, rotation_vector;
        XMMatrixDecompose(&stub_vector, &rotation_vector, &stub_vector, 
            XMMatrixRotationRollPitchYaw(-vertical_angle-corrective_angle, horizontal_angle, 0.f)
            *XMMatrixInverse(nullptr, XMLoadFloat4x4(&hip_parent_transform->world))
            );

        XMFLOAT4 hip_rotation_target;
        XMStoreFloat4(&hip_rotation_target, rotation_vector);
        *(XMFLOAT4*)(hip_override_data->keyframe_data.data()) = hip_rotation_target;


        wi::primitive::Ray shoot_ray;
        Game::Scene::Prefab* current_gun_prefab = Game::GetScene().prefabs.GetComponent(_internal_::Guns[_internal_::Runtime_State::current_gun]);
        wi::ecs::Entity nozzle_entity = current_gun_prefab->FindEntityByName("PROJECTILE_NOZZLE");
        wi::scene::TransformComponent* nozzle_transform = Game::GetScene().wiscene.transforms.GetComponent(nozzle_entity);

        shoot_ray.CreateFromPoints(nozzle_transform->GetPosition(), aim_res.position);
        GBus::Projectile::SetSpawn(shoot_ray, _internal_::projectile_program_ID);
    };

    // Initialize sword's action timings
    _internal_::Anim_State::EventMSG::Sword::slash_hitcancel_timing[Defines::AnimationEnum::Player_Anim_SwordSwingNormal1] = Gameplay::Helpers::GradientMap({{0.f,0.f},{(6.f/60.f),1.f},{(12.f/60.f),2.f}});
    _internal_::Anim_State::EventMSG::Sword::slash_hitcancel_timing[Defines::AnimationEnum::Player_Anim_SwordSwingNormal2] = Gameplay::Helpers::GradientMap({{0.f,0.f},{(6.f/60.f),1.f},{(12.f/60.f),2.f}});
    _internal_::Anim_State::EventMSG::Sword::slash_hitcancel_timing[Defines::AnimationEnum::Player_Anim_SwordThrust1] = Gameplay::Helpers::GradientMap({{0.f,0.f},{(8.f/60.f),1.f},{(13.f/60.f),2.f}});
    _internal_::Anim_State::EventMSG::Sword::slash_hitcancel_timing[Defines::AnimationEnum::Player_Anim_SwordUpSwing] = Gameplay::Helpers::GradientMap({{0.f,0.f},{(8.f/60.f),1.f},{(13.f/60.f),2.f}});
    _internal_::Anim_State::EventMSG::Sword::slash_hitcancel_timing[Defines::AnimationEnum::Player_Anim_SwordDownThrust] = Gameplay::Helpers::GradientMap({{0.f,0.f},{(1.f/60.f),1.f},{(2.f/60.f),2.f}});
    _internal_::Anim_State::EventMSG::Sword::slash_hitcancel_timing[Defines::AnimationEnum::Player_Anim_SwordSpinVertical] = Gameplay::Helpers::GradientMap({{0.f,0.f},{(1.f/60.f),1.f},{(2.f/60.f),2.f}});
    _internal_::Anim_State::EventMSG::Sword::slash_hitcancel_timing[Defines::AnimationEnum::Player_Anim_SwordDownStrikeStuck] = Gameplay::Helpers::GradientMap({{0.f,0.f},{(1.f/60.f),1.f},{(2.f/60.f),2.f}});
    _internal_::Anim_State::EventMSG::Sword::slash_hitcancel_timing[Defines::AnimationEnum::Player_Anim_SwordUnstuckActionLead] = Gameplay::Helpers::GradientMap({{0.f,0.f},{(1.f/60.f),1.f},{(2.f/60.f),2.f}});
    _internal_::Anim_State::EventMSG::Sword::slash_hitcancel_timing[Defines::AnimationEnum::Player_Anim_SwordUnstuckActionToGun] = Gameplay::Helpers::GradientMap({{0.f,0.f},{(1.f/60.f),1.f},{(2.f/60.f),2.f}});
    
    _internal_::Actions[Defines::ActionEnum::SLASH] = [=](float transition)
    {
        for(auto slash_lerps_pair : _internal_::Anim_State::EventMSG::Sword::slash_lerps)
        {
            bool do_animate = 
                (_internal_::Anim_State::EventMSG::Sword::current_animation == slash_lerps_pair.first);
            
            if(do_animate)
            {
                float hitslash_value_get = 
                    _internal_::Anim_State::EventMSG::Sword::slash_hitcancel_timing[slash_lerps_pair.first]
                        .Get(_internal_::AnimationPtrs[slash_lerps_pair.first]->timer);

                _internal_::Anim_State::EventMSG::Sword::cancellable = (hitslash_value_get == 2.f);
                _internal_::Anim_State::EventMSG::Sword::enable_hitbox = ((hitslash_value_get > 1.f) && ((hitslash_value_get < 2.f)));

                if(_internal_::Anim_State::EventMSG::Sword::slash_reset)
                    _internal_::AnimationPtrs[slash_lerps_pair.first]->Stop();
            }

            slash_lerps_pair.second = wi::math::Lerp(
                slash_lerps_pair.second,
                do_animate,
                std::min(_internal_::Runtime_State::TimeDelta*30.f, 1.f)
            );

            if(slash_lerps_pair.second > 0.f)
            {
                _internal_::AnimationPtrs[slash_lerps_pair.first]->Play();
                _internal_::AnimationPtrs[slash_lerps_pair.first]->SetLooped(false);
                _internal_::AnimationPtrs[slash_lerps_pair.first]->amount = transition*slash_lerps_pair.second;
            }
            else
            {
                _internal_::AnimationPtrs[slash_lerps_pair.first]->Stop();
            }
        }
        if(_internal_::Anim_State::EventMSG::Sword::slash_reset)
            _internal_::Anim_State::EventMSG::Sword::slash_reset = false;

        Game::Scene::Prefab* current_sword_prefab = Game::GetScene().prefabs.GetComponent(_internal_::Swords[_internal_::Runtime_State::current_sword]);
        wi::ecs::Entity hitbox_entity = current_sword_prefab->FindEntityByName("HITBOX");
        Gameplay::Component::HitData* hitdata = Gameplay::Component::hitdatas->GetComponent(hitbox_entity);
        if(hitdata != nullptr)
        {
            hitdata->communicationType = (_internal_::Anim_State::EventMSG::Sword::enable_hitbox) ? 
                Gameplay::Component::HitData::CommunicationType::SENDER : 
                Gameplay::Component::HitData::CommunicationType::DISABLED;
        }
        
        if(transition == 0.0f)
        {
            for(auto slash_lerps_pair : _internal_::Anim_State::EventMSG::Sword::slash_lerps)
            {
                _internal_::AnimationPtrs[slash_lerps_pair.first]->Stop();
            }
        }
    };

    // Set up collision data
    _internal_::Runtime_State::collider_shape.tip = XMFLOAT3(0.f,1.55f,0.f);
    _internal_::Runtime_State::collider_shape.radius = 0.2f;
}

// static bool scenedata_ready = false;
void Gameplay::Player::Hook_PreUpdate(float dt)
{
    auto prefab = Game::GetScene().prefabs.GetComponent(_internal_::Init_State::playerID);

    dt = wi::math::Clamp(dt, 0.f, 1.f); // Cap dt for gameplay
    _internal_::Runtime_State::TimeDelta = dt;

    wi::scene::CameraComponent& camera = wi::scene::GetCamera();

    if(_internal_::Runtime_State::scenedata_ready)
    {
        // Detach ground parent
        {
            auto hierarchy = Game::GetScene().wiscene.hierarchy.GetComponent(_internal_::Init_State::playerID);
            if(hierarchy != nullptr)
                Game::GetScene().wiscene.Component_Detach(_internal_::Init_State::playerID);
        }

        wi::scene::TransformComponent* transform = Game::GetScene().wiscene.transforms.GetComponent(_internal_::Init_State::playerID);

        // If the program is developer's binary, manage agency between devlook and player
#ifdef IS_DEV
        if(wi::input::Press(wi::input::KEYBOARD_BUTTON_F5))
            SetGameplayAgencyControl(!GetGameplayAgencyControl());
#endif

        // Player Input
        XMFLOAT2 move_vec = (Gameplay::GetGameplayAgencyControl()) ? InputGetAxeValue(Defines::InputEnum::AXES_MOVE) : XMFLOAT2();
        XMFLOAT3 move_vec3 = XMFLOAT3(move_vec.x, 0.f, move_vec.y);
        auto move_vec_dist = wi::math::Length(move_vec);
        
        XMFLOAT2 cam_orient_vec = (Gameplay::GetGameplayAgencyControl()) ? InputGetAxeValue(Defines::InputEnum::AXES_CAMERA) : XMFLOAT2();
        auto cam_orient_dist = wi::math::Length(cam_orient_vec);

        _internal_::Runtime_State::zoom += InputGetAxeValue(Defines::InputEnum::AXES_CAMERA_MOUSE_SCROLL).y*dt*20.f;
        _internal_::Runtime_State::zoom = wi::math::Clamp(_internal_::Runtime_State::zoom, Defines::Runtime::CamZoomMax, Defines::Runtime::CamZoomMin);

        if(InputGetButtonPressed(Defines::InputEnum::TGL_WALK))
        {
            _internal_::Runtime_State::toggled_walk = !_internal_::Runtime_State::toggled_walk;
        }
        if(InputGetButtonDown(Defines::InputEnum::MOD_RUN))
        {
            if(InputGetButtonPressed(Defines::InputEnum::MOD_DASH))
            {
                bool initiate_dash = false;
                if(!_internal_::Runtime_State::dash && (_internal_::Runtime_State::dash_timer_cooldown == 0.f))
                {
                    if(!_internal_::Runtime_State::jump)
                        initiate_dash = true;
                    else if(!_internal_::Runtime_State::air_dash_done)
                        initiate_dash = true;
                }
                if(initiate_dash)
                {
                    _internal_::Runtime_State::dash_timer = 0.f;
                    _internal_::Runtime_State::dash = true;
                }
            }

            _internal_::Runtime_State::dynamic_speed_cap_raw = _internal_::Runtime_State::toggled_walk ? Defines::Runtime::JogSpeedCap : Defines::Runtime::RunSpeedCap;
            if(_internal_::Runtime_State::dash && (_internal_::Runtime_State::dash_timer_cooldown == 0.f))
                _internal_::Runtime_State::dash_timer = std::min(_internal_::Runtime_State::dash_timer + dt,0.5f);
        }
        else
        {
            _internal_::Runtime_State::dynamic_speed_cap_raw =_internal_::Runtime_State::toggled_walk ? Defines::Runtime::WalkSpeedCap :  Defines::Runtime::JogSpeedCap;
            if(_internal_::Runtime_State::dash && (_internal_::Runtime_State::dash_timer_cooldown == 0.f))
                _internal_::Runtime_State::dash_timer = 0.5f;
        }

        // Player dash handling
        if(_internal_::Runtime_State::dash && (_internal_::Runtime_State::dash_timer_cooldown == 0.f) && (_internal_::Runtime_State::dash_timer == 0.5f))
        {
            _internal_::Runtime_State::dash_timer_cooldown = 0.4f;
            if(_internal_::Runtime_State::jump && !_internal_::Runtime_State::air_dash_done)
                _internal_::Runtime_State::air_dash_done = true;
        }
        if(_internal_::Runtime_State::dash && _internal_::Runtime_State::dash_timer_cooldown > 0.f)
        {
            _internal_::Runtime_State::dash_timer_cooldown = std::max(_internal_::Runtime_State::dash_timer_cooldown - dt, 0.f);
            if(_internal_::Runtime_State::dash_timer_cooldown == 0)
                _internal_::Runtime_State::dash = false;
        }
        bool do_dash = (_internal_::Runtime_State::dash_timer > 0.f) && (_internal_::Runtime_State::dash_timer < 0.5f);
        _internal_::Anim_State::EventMSG::Move::dash_lerp = wi::math::Lerp(
            _internal_::Anim_State::EventMSG::Move::dash_lerp,
            wi::math::Lerp(
                _internal_::Anim_State::EventMSG::Move::dash_lerp,
                do_dash,
                wi::math::Clamp(40.f*dt,0.f,1.f)
            ),
            wi::math::Clamp(40.f*dt,0.f,1.f)
        );

        if(!_internal_::Runtime_State::jump && InputGetButtonDown(Defines::InputEnum::ACT_JUMP))
        {
            transform->translation_local.y += 0.4f; // Easy detach on moving platforms
            _internal_::Runtime_State::velocity.y = Defines::Runtime::JumpVelocity;
            _internal_::Runtime_State::velocity.y += _internal_::Runtime_State::jump_potential;
            _internal_::Runtime_State::static_h_velocity = XMFLOAT2(
                _internal_::Runtime_State::velocity.x,
                _internal_::Runtime_State::velocity.z
            );
            _internal_::Runtime_State::h_velocity_offset = XMFLOAT2();
            _internal_::Runtime_State::jump = true;
            _internal_::Runtime_State::upslash_window = 8.f/60.f;
        }

        if(InputGetButtonPressed(Defines::InputEnum::ACT_SWITCH_WEAPON))
        {
            _internal_::Runtime_State::current_use = (_internal_::Runtime_State::current_use == Defines::WeaponUseEnum::GUN_ACTIVE) ? 
                Defines::WeaponUseEnum::SWORD_ACTIVE :
                Defines::WeaponUseEnum::GUN_ACTIVE;
        }

        // Attack actions
        if(_internal_::Runtime_State::current_use == Defines::WeaponUseEnum::GUN_ACTIVE)
        {
            if(InputGetButtonDown(Defines::InputEnum::ACT_ATTACK1))
                _internal_::Runtime_State::shoot = true;
            if(InputGetButtonDown(Defines::InputEnum::ACT_ATTACK2))
                _internal_::Runtime_State::shoot = true;
        }
        bool attack_up_slash = false;
        if( (_internal_::Anim_State::EventMSG::Sword::cancellable || _internal_::Runtime_State::slash_timer == 0.f)
            && (_internal_::Runtime_State::current_use == Defines::WeaponUseEnum::SWORD_ACTIVE))
        {
            if(InputGetButtonPressed(Defines::InputEnum::ACT_ATTACK1))
                _internal_::Runtime_State::slash = true;
            if(InputGetButtonPressed(Defines::InputEnum::ACT_ATTACK2))
                _internal_::Runtime_State::slash = true;
            if((_internal_::Runtime_State::upslash_window > 0.f)
                &&(InputGetButtonPressed(Defines::InputEnum::ACT_ATTACK1)
                    ||InputGetButtonPressed(Defines::InputEnum::ACT_ATTACK2)))
            {
                _internal_::Runtime_State::upslash_window = 0.f;
                attack_up_slash = true;
            }
        }
        _internal_::Runtime_State::upslash_window = (_internal_::Runtime_State::upslash_window > 0.f) ?
            _internal_::Runtime_State::upslash_window - dt : 0.f;
        
        if(Gameplay::GetGameplayAgencyControl())
        {
            // Get hold of mouse cursor
            if(!_internal_::Runtime_State::mouse_state_lock)
            {
                _internal_::Runtime_State::locked_mouse_pointer = wi::input::GetPointer();
                _internal_::Runtime_State::mouse_state_lock = true;   
            }
        }
        else
        {
            // Release hold of mouse cursor
            if(_internal_::Runtime_State::mouse_state_lock)
            {
                wi::input::SetPointer(_internal_::Runtime_State::locked_mouse_pointer);
                wi::input::HidePointer(false);
                _internal_::Runtime_State::mouse_state_lock = false;
            }
        }
        // ---

        // Player Movement
        // Dynamic speed cap lerping
        _internal_::Runtime_State::dynamic_speed_cap = (std::abs(_internal_::Runtime_State::dynamic_speed_cap - _internal_::Runtime_State::dynamic_speed_cap_raw) < 0.01f) ? 
            _internal_::Runtime_State::dynamic_speed_cap_raw 
            : wi::math::Lerp(_internal_::Runtime_State::dynamic_speed_cap, _internal_::Runtime_State::dynamic_speed_cap_raw, wi::math::Clamp(7.f*dt,0.f,1.f));
        // Ground State
        if (!_internal_::Runtime_State::jump)
        {
            if(_internal_::Runtime_State::air_slash_counter > 0)
                _internal_::Runtime_State::air_slash_counter = 0;

            // Re-attach player to ground
            if(_internal_::Runtime_State::ground_entity != wi::ecs::INVALID_ENTITY)
                RepositionPlayerGround(transform->translation_local, _internal_::Runtime_State::direction_quaternion);

            // H-Velocity control
            if(move_vec_dist > 0.1f)
            {
                // Get forward velocity
                _internal_::Runtime_State::h_velocity = std::min(
                    _internal_::Runtime_State::h_velocity + move_vec_dist*16.f*dt,
                    _internal_::Runtime_State::dynamic_speed_cap
                );
            }
            else if(!do_dash) // Only reduce forward velocity AFTER dash is enacted
            {
                // Reduce forward velocity, keep the changed player direction
                _internal_::Runtime_State::h_velocity = std::max(
                    _internal_::Runtime_State::h_velocity - 30.f*dt,
                    0.f
                );
            }

            float dynamic_slash_speed = (_internal_::Runtime_State::h_velocity > 5.f) ? Defines::Runtime::SlashSpeed : std::max(_internal_::Runtime_State::h_velocity, 2.f);

            // Forward velocity control
            XMVECTOR stub_vector, rotation_vector, forward_velocity_vector;
            if(do_dash 
                || (_internal_::Runtime_State::slash_timer > 0.f))
            {
                XMFLOAT3 forward_velocity = XMFLOAT3(0.f,0.f,
                    (_internal_::Runtime_State::slash_timer > 0.f) ? dynamic_slash_speed : Defines::Runtime::DashSpeed);
                auto vector_matrix = 
                    XMMatrixTranslationFromVector(XMLoadFloat3(&forward_velocity))
                    * XMMatrixRotationQuaternion(XMLoadFloat4(&_internal_::Runtime_State::direction_quaternion));
                XMMatrixDecompose(&stub_vector, &stub_vector, &forward_velocity_vector, vector_matrix);
            }
            else if(move_vec_dist > 0.1f)
            {
                XMFLOAT3 forward_velocity = XMFLOAT3(0.f,0.f,_internal_::Runtime_State::h_velocity);

                // Multiply rotation matrix with camera direction
                auto camera_at_vector = camera.At;
                camera_at_vector.y = 0.f;
                camera_at_vector.x = -camera_at_vector.x; // Inverse X at vector
                auto camera_rotation_matrix = XMMatrixLookAtLH(XMVECTOR(), XMLoadFloat3(&camera_at_vector), XMLoadFloat3(&Defines::Runtime::UpVector));
                
                auto vector_matrix = 
                    XMMatrixTranslationFromVector(XMLoadFloat3(&forward_velocity))
                    * XMMatrixLookAtLH(XMVECTOR(), XMLoadFloat3(&move_vec3), XMLoadFloat3(&Defines::Runtime::UpVector))
                    * camera_rotation_matrix;
                XMMatrixDecompose(&stub_vector, &rotation_vector, &forward_velocity_vector, vector_matrix);
                XMStoreFloat4(&_internal_::Runtime_State::direction_quaternion, rotation_vector);
            }
            else
            {
                XMFLOAT3 forward_velocity = XMFLOAT3(0.f,0.f,_internal_::Runtime_State::h_velocity);
                auto vector_matrix = 
                    XMMatrixTranslationFromVector(XMLoadFloat3(&forward_velocity))
                    * XMMatrixRotationQuaternion(XMLoadFloat4(&_internal_::Runtime_State::direction_quaternion));
                XMMatrixDecompose(&stub_vector, &stub_vector, &forward_velocity_vector, vector_matrix);
            }
            // Player direction
            // Player direction lerp
            _internal_::Runtime_State::direction_quaternion_lerp = wi::math::Slerp(
                _internal_::Runtime_State::direction_quaternion_lerp, 
                wi::math::Slerp(
                    _internal_::Runtime_State::direction_quaternion_lerp,
                    _internal_::Runtime_State::direction_quaternion,
                    28.f*dt
                ), 
                28.f*dt);

            // Tilt calculation
            auto player_tilt_quaternion_slerp = XMQuaternionSlerp(
                XMLoadFloat4(&_internal_::Runtime_State::tilt_quaternion), 
                XMLoadFloat4(&_internal_::Runtime_State::direction_quaternion_lerp), 
                30.f*dt);
            XMStoreFloat4(&_internal_::Runtime_State::tilt_quaternion, player_tilt_quaternion_slerp);
            float tilt_direction = Gameplay::Helpers::GetAngleDiff(_internal_::Runtime_State::tilt_quaternion, _internal_::Runtime_State::direction_quaternion);
            tilt_direction = wi::math::Clamp(tilt_direction, -wi::math::PI/3.5f, wi::math::PI/3.5f);
            Gameplay::Helpers::GradientMap tilt_move_factor({{3.f,0.f},{12.f,1.f}});
            tilt_direction *= tilt_move_factor.Get(wi::math::Length(XMFLOAT2(_internal_::Runtime_State::velocity.x, _internal_::Runtime_State::velocity.z)));

            // Set player orientation
            XMFLOAT4 player_orientation_quat;
            auto player_orientation_matrix = 
                XMMatrixRotationRollPitchYaw(0.f, 0.f, -tilt_direction)
                * XMMatrixRotationQuaternion(XMLoadFloat4(&_internal_::Runtime_State::direction_quaternion_lerp))
                * XMMatrixRotationRollPitchYaw(0.f, wi::math::PI, 0.f);
            XMMatrixDecompose(&stub_vector, &rotation_vector, &stub_vector, player_orientation_matrix);
            XMStoreFloat4(&player_orientation_quat, rotation_vector);
            transform->rotation_local = player_orientation_quat;
            // --
            
            float v_velocity = _internal_::Runtime_State::velocity.y;
            XMStoreFloat3(&_internal_::Runtime_State::velocity, forward_velocity_vector);
            _internal_::Runtime_State::velocity.y = v_velocity;
        }
        // Air State
        else
        {
            if(_internal_::Runtime_State::slash && _internal_::Anim_State::EventMSG::Sword::cancellable)
                _internal_::Runtime_State::air_slash_counter += 1;
            // // Add offset
            XMVECTOR stub_vector, rotation_vector, forward_velocity_vector;
            if(do_dash 
                || (_internal_::Runtime_State::slash_timer > 0.f))
            {
                XMFLOAT3 forward_velocity = XMFLOAT3(0.f,0.f,
                    (_internal_::Runtime_State::slash_timer > 0.f) ? Defines::Runtime::SlashSpeed : Defines::Runtime::DashSpeed);
                auto vector_matrix = 
                    XMMatrixTranslationFromVector(XMLoadFloat3(&forward_velocity))
                    * XMMatrixRotationQuaternion(XMLoadFloat4(&_internal_::Runtime_State::direction_quaternion));
                XMMatrixDecompose(&stub_vector, &stub_vector, &forward_velocity_vector, vector_matrix);

                XMFLOAT3 static_h_velocity_forward = XMFLOAT3(0.f,0.f,wi::math::Length(_internal_::Runtime_State::static_h_velocity));
                XMVECTOR static_h_velocity_vector = XMVECTOR();
                auto static_h_velocity_matrix = 
                    XMMatrixTranslationFromVector(XMLoadFloat3(&static_h_velocity_forward))
                    * XMMatrixRotationQuaternion(XMLoadFloat4(&_internal_::Runtime_State::direction_quaternion));
                XMMatrixDecompose(&stub_vector, &stub_vector, &static_h_velocity_vector, static_h_velocity_matrix);
                XMStoreFloat3(&static_h_velocity_forward, static_h_velocity_vector);
                _internal_::Runtime_State::static_h_velocity.x = static_h_velocity_forward.x;
                _internal_::Runtime_State::static_h_velocity.y = static_h_velocity_forward.z;
                _internal_::Runtime_State::h_velocity_offset = XMFLOAT2();
            }
            else if(move_vec_dist > 0.1f)
            {
                // Get forward velocity
                XMFLOAT3 forward_velocity = XMFLOAT3(0.f,0.f,move_vec_dist*13.f*dt);

                // Multiply rotation matrix with camera direction
                auto camera_at_vector = camera.At;
                camera_at_vector.y = 0.f;
                camera_at_vector.x = -camera_at_vector.x; // Inverse X at vector
                auto camera_rotation_matrix = XMMatrixLookAtLH(XMVECTOR(), XMLoadFloat3(&camera_at_vector), XMLoadFloat3(&Defines::Runtime::UpVector));
                
                auto vector_matrix = 
                    XMMatrixTranslationFromVector(XMLoadFloat3(&forward_velocity))
                    * XMMatrixLookAtLH(XMVECTOR(), XMLoadFloat3(&move_vec3), XMLoadFloat3(&Defines::Runtime::UpVector))
                    * camera_rotation_matrix;
                XMMatrixDecompose(&stub_vector, &rotation_vector, &forward_velocity_vector, vector_matrix);
                XMStoreFloat4(&_internal_::Runtime_State::direction_quaternion, rotation_vector);
                XMStoreFloat3(&forward_velocity, forward_velocity_vector);
                
                // Store final offset
                XMFLOAT2 old_h_offset = _internal_::Runtime_State::h_velocity_offset;
                _internal_::Runtime_State::h_velocity_offset.x += forward_velocity.x;
                _internal_::Runtime_State::h_velocity_offset.y += forward_velocity.z;
                float h_offset_length = wi::math::Length(_internal_::Runtime_State::h_velocity_offset);
                if(h_offset_length > 5.f)
                {
                    auto h_velocity_offset_vector = XMVector2Normalize(XMLoadFloat2(&_internal_::Runtime_State::h_velocity_offset))*5.f;
                    XMStoreFloat2(&_internal_::Runtime_State::h_velocity_offset, h_velocity_offset_vector);
                }
            }
            // Player direction
            // Player direction lerp
            _internal_::Runtime_State::direction_quaternion_lerp = wi::math::Slerp(
                _internal_::Runtime_State::direction_quaternion_lerp, 
                wi::math::Slerp(
                    _internal_::Runtime_State::direction_quaternion_lerp,
                    _internal_::Runtime_State::direction_quaternion,
                    22.f*dt
                ), 
                22.f*dt);
            // Set player orientation
            XMFLOAT4 player_orientation_quat;
            auto player_orientation_matrix = 
                XMMatrixRotationQuaternion(XMLoadFloat4(&_internal_::Runtime_State::direction_quaternion_lerp))
                * XMMatrixRotationRollPitchYaw(0.f, wi::math::PI, 0.f);
            XMMatrixDecompose(&stub_vector, &rotation_vector, &stub_vector, player_orientation_matrix);
            XMStoreFloat4(&player_orientation_quat, rotation_vector);
            transform->rotation_local = player_orientation_quat;
            // --

            // Get h velocity adjustment
            if((wi::math::Length(_internal_::Runtime_State::static_h_velocity) > 0.001f) &&
                (wi::math::Length(_internal_::Runtime_State::h_velocity_offset) > 0.001f))
            {
                float directional_reduction_modifier = wi::math::Clamp(
                    wi::math::Length(_internal_::Runtime_State::static_h_velocity)*10.f, 
                    0.f, 1.f);
                XMFLOAT3 static_h_velocity_vec3 = XMFLOAT3(
                    _internal_::Runtime_State::static_h_velocity.x,
                    0.f,
                    _internal_::Runtime_State::static_h_velocity.y
                );
                XMFLOAT3 h_velocity_offset_vec3 = XMFLOAT3(
                    _internal_::Runtime_State::h_velocity_offset.x,
                    0.f,
                    _internal_::Runtime_State::h_velocity_offset.y
                );
                auto static_h_velocity_orientation_matrix = 
                    XMMatrixLookAtLH(XMVECTOR(), 
                        XMLoadFloat3(&static_h_velocity_vec3), 
                        XMLoadFloat3(&Defines::Runtime::UpVector));
                auto h_velocity_offset_orientation_matrix = 
                    XMMatrixLookAtLH(XMVECTOR(), 
                        XMLoadFloat3(&h_velocity_offset_vec3), 
                        XMLoadFloat3(&Defines::Runtime::UpVector));
                XMVECTOR orient_a, orient_b;
                XMMatrixDecompose(&stub_vector, &orient_a, &stub_vector, static_h_velocity_orientation_matrix);
                XMMatrixDecompose(&stub_vector, &orient_b, &stub_vector, h_velocity_offset_orientation_matrix);
                XMFLOAT4 quat_a, quat_b;
                XMStoreFloat4(&quat_a, orient_a);
                XMStoreFloat4(&quat_b, orient_b);
                float directional_reduction_value = Gameplay::Helpers::GetAngleDiff(quat_a, quat_b);
                directional_reduction_value = std::abs(wi::math::Clamp(directional_reduction_value, -wi::math::PI/3.f, wi::math::PI/3.f))/(wi::math::PI/3.f);
                directional_reduction_value = wi::math::Lerp(1.f, directional_reduction_value, directional_reduction_modifier);
                _internal_::Runtime_State::h_velocity_offset.x *= directional_reduction_value;
                _internal_::Runtime_State::h_velocity_offset.y *= directional_reduction_value;
            }

            XMFLOAT3 dash_velocity = XMFLOAT3();
            XMStoreFloat3(&dash_velocity, forward_velocity_vector);

            _internal_::Runtime_State::velocity.x = (do_dash) ? dash_velocity.x :
                _internal_::Runtime_State::static_h_velocity.x + _internal_::Runtime_State::h_velocity_offset.x;
            _internal_::Runtime_State::velocity.z = (do_dash) ? dash_velocity.z :
                _internal_::Runtime_State::static_h_velocity.y + _internal_::Runtime_State::h_velocity_offset.y;
            if(do_dash
                || ((_internal_::Runtime_State::air_slash_counter < 3) && (_internal_::Runtime_State::slash_timer > 0.f)
                    && !(
                        _internal_::Anim_State::EventMSG::Sword::current_animation == Defines::AnimationEnum::Player_Anim_SwordUpSwing
                        || _internal_::Anim_State::EventMSG::Sword::current_animation == Defines::AnimationEnum::Player_Anim_SwordDownThrust
                        || _internal_::Anim_State::EventMSG::Sword::current_animation == Defines::AnimationEnum::Player_Anim_SwordSpinVertical
                        || (_internal_::Runtime_State::jump && _internal_::Runtime_State::slash)
                        )
                    )
                )
                _internal_::Runtime_State::velocity.y = 0.f;
        }

        // Gravity
        _internal_::Runtime_State::velocity.y = std::max(
            _internal_::Runtime_State::velocity.y - 16.f*dt, 
            _internal_::Runtime_State::dynamic_terminal_velocity);

        // Collision Calculation
        CalculatePlayerCollision(transform->translation_local, _internal_::Runtime_State::velocity, dt);
        _internal_::Runtime_State::h_velocity = wi::math::Length(XMFLOAT2(_internal_::Runtime_State::velocity.x, _internal_::Runtime_State::velocity.z));
        transform->SetDirty();
        // ---

        // Weapon Display
        {
            for(auto& gun_map_pair : _internal_::Guns)
            {
                wi::scene::HierarchyComponent* hierarchy = Game::GetScene().wiscene.hierarchy.GetComponent(gun_map_pair.second);
                if( (_internal_::Runtime_State::current_use == Defines::WeaponUseEnum::GUN_ACTIVE)
                    && (_internal_::Runtime_State::current_gun == gun_map_pair.first))
                {
                    if(hierarchy->parentID != _internal_::Runtime_State::weapon_grip_r_entity)
                        hierarchy->parentID = _internal_::Runtime_State::weapon_grip_r_entity;
                }
                else
                {
                    if(hierarchy->parentID != _internal_::Runtime_State::weapon_hold_r_entity)
                        hierarchy->parentID = _internal_::Runtime_State::weapon_hold_r_entity;
                }
            }
            for(auto& sword_map_pair : _internal_::Swords)
            {
                wi::scene::HierarchyComponent* hierarchy = Game::GetScene().wiscene.hierarchy.GetComponent(sword_map_pair.second);
                if( (_internal_::Runtime_State::current_use == Defines::WeaponUseEnum::SWORD_ACTIVE)
                    && (_internal_::Runtime_State::current_sword == sword_map_pair.first))
                {
                    if(hierarchy->parentID != _internal_::Runtime_State::weapon_grip_r_entity)
                        hierarchy->parentID = _internal_::Runtime_State::weapon_grip_r_entity;
                }
                else
                {
                    if(hierarchy->parentID != _internal_::Runtime_State::weapon_hold_l_entity)
                        hierarchy->parentID = _internal_::Runtime_State::weapon_hold_l_entity;
                }
            }
        }

        // Animation control
        {
            // Move animation control
            XMFLOAT2 h_velocity_vec2 = XMFLOAT2(_internal_::Runtime_State::velocity.x,_internal_::Runtime_State::velocity.z);
            float h_velocity = wi::math::Length(h_velocity_vec2);

            _internal_::Anim_State::EventMSG::Move::transition_lerp = wi::math::Lerp(_internal_::Anim_State::EventMSG::Move::transition_lerp, h_velocity > 0.f ? 1.f : 0.f, dt*10.f);

            static Gameplay::Helpers::GradientMap move_transition_gradient({
                {Defines::Runtime::AnimTransitionWalk,0.f},
                {Defines::Runtime::AnimTransitionJog,1.f},
                {Defines::Runtime::AnimTransitionRun,2.f}});
            _internal_::Anim_State::EventMSG::Move::move_transition = move_transition_gradient.Get(h_velocity);

            static Gameplay::Helpers::GradientMap move_speed_gradient({
                {0.f,0.f},
                {Defines::Runtime::AnimTransitionWalk,Defines::Runtime::AnimSpeedWalk},
                {Defines::Runtime::AnimTransitionJog,Defines::Runtime::AnimSpeedJog},
                {Defines::Runtime::AnimTransitionRun,Defines::Runtime::AnimSpeedRun},
                {Defines::Runtime::AnimSpeedRunEXTransition,Defines::Runtime::AnimSpeedRunEX}});

            _internal_::Anim_State::EventMSG::Move::move_speed = move_speed_gradient.Get(h_velocity);

            // Jump animation control
            _internal_::Anim_State::EventMSG::Jump::transition_lerp = wi::math::Lerp(
                _internal_::Anim_State::EventMSG::Jump::transition_lerp,
                wi::math::Lerp(
                    _internal_::Anim_State::EventMSG::Jump::transition_lerp,
                    _internal_::Runtime_State::jump,
                    wi::math::Clamp(30.f*dt,0.f,1.f)
                ),
                wi::math::Clamp(30.f*dt,0.f,1.f)
            );
            
            static Gameplay::Helpers::GradientMap jump_fall_gradient({{-6.f,0.f},{6.f,1.f}});
            _internal_::Anim_State::EventMSG::Jump::air_fall_transition = jump_fall_gradient.Get(_internal_::Runtime_State::velocity.y);
            _internal_::Anim_State::EventMSG::Jump::air_fall_transition_lerp = wi::math::Lerp(
                _internal_::Anim_State::EventMSG::Jump::air_fall_transition_lerp,
                wi::math::Lerp(
                    _internal_::Anim_State::EventMSG::Jump::air_fall_transition_lerp,
                    _internal_::Anim_State::EventMSG::Jump::air_fall_transition,
                    wi::math::Clamp(30.f*dt,0.f,1.f)
                ),
                wi::math::Clamp(30.f*dt,0.f,1.f)
            );

            // Shooting controls
            if(_internal_::Runtime_State::shoot)
            {
                _internal_::Runtime_State::shoot_timer = 0.f;
                _internal_::Runtime_State::shoot = false;
                _internal_::Anim_State::EventMSG::Shoot::transition_lerp = 1.f;
            }
            _internal_::Runtime_State::shoot_timer = _internal_::Runtime_State::shoot_timer < (20.f/60.f) ? _internal_::Runtime_State::shoot_timer+dt : _internal_::Runtime_State::shoot_timer;
            bool do_shoot = _internal_::Runtime_State::shoot_timer < (20.f/60.f);
            _internal_::Anim_State::EventMSG::Shoot::transition_lerp = wi::math::Lerp(
                _internal_::Anim_State::EventMSG::Shoot::transition_lerp,
                wi::math::Lerp(
                    _internal_::Anim_State::EventMSG::Shoot::transition_lerp,
                    float(do_shoot),
                    wi::math::Clamp(20.f*dt,0.f,1.f)
                ),
                wi::math::Clamp(20.f*dt,0.f,1.f)
            );
            GBus::Projectile::SetShoot(do_shoot, _internal_::projectile_program_ID);

            // Slashing controls
            if(_internal_::Runtime_State::slash)
            {
                _internal_::Runtime_State::slash = false;
                _internal_::Anim_State::EventMSG::Sword::slash_reset = true;
                // Action trees
                // Combo action
                if(_internal_::Runtime_State::slash_timer > 0.f)
                {
                    // Set next current animation (for no other button presses)
                    switch(_internal_::Anim_State::EventMSG::Sword::current_animation)
                    {
                        case Defines::AnimationEnum::Player_Anim_SwordSwingNormal1:
                        {
                            _internal_::Anim_State::EventMSG::Sword::current_animation = Defines::AnimationEnum::Player_Anim_SwordSwingNormal2;
                            break;
                        }
                        case Defines::AnimationEnum::Player_Anim_SwordSwingNormal2:
                        {
                            _internal_::Anim_State::EventMSG::Sword::current_animation = Defines::AnimationEnum::Player_Anim_SwordThrust1;
                            break;
                        }
                        case Defines::AnimationEnum::Player_Anim_SwordThrust1:
                        {
                            _internal_::Anim_State::EventMSG::Sword::current_animation = Defines::AnimationEnum::Player_Anim_SwordSwingNormal1;
                            break;
                        }
                        default:
                        {
                            _internal_::Anim_State::EventMSG::Sword::current_animation = Defines::AnimationEnum::Player_Anim_SwordSwingNormal1;
                            break;
                        }
                    }
                }
                else
                    _internal_::Anim_State::EventMSG::Sword::current_animation = Defines::AnimationEnum::Player_Anim_SwordSwingNormal1;

                if(attack_up_slash)
                    _internal_::Anim_State::EventMSG::Sword::current_animation = Defines::AnimationEnum::Player_Anim_SwordUpSwing;

                // Set animation timer
                switch(_internal_::Anim_State::EventMSG::Sword::current_animation)
                {
                    case Defines::AnimationEnum::Player_Anim_SwordSwingNormal1:
                    {
                        _internal_::Runtime_State::slash_timer = 25.f/60.f;
                        break;
                    }
                    case Defines::AnimationEnum::Player_Anim_SwordSwingNormal2:
                    {
                        _internal_::Runtime_State::slash_timer = 25.f/60.f;
                        break;
                    }
                    case Defines::AnimationEnum::Player_Anim_SwordThrust1:
                    {
                        _internal_::Runtime_State::slash_timer = 40.f/60.f;
                        break;
                    }
                    case Defines::AnimationEnum::Player_Anim_SwordUpSwing:
                    {
                        _internal_::Runtime_State::slash_timer = 60.f/60.f;
                        break;
                    }
                    default:
                        break;
                }
            }
            _internal_::Runtime_State::slash_timer = (_internal_::Runtime_State::slash_timer > 0.f) ?
                _internal_::Runtime_State::slash_timer - dt
                : 0.f;
            bool do_slash = _internal_::Runtime_State::slash_timer > 0.f;
            _internal_::Anim_State::EventMSG::Sword::transition_lerp = wi::math::Lerp(
                _internal_::Anim_State::EventMSG::Sword::transition_lerp,
                wi::math::Lerp(
                    _internal_::Anim_State::EventMSG::Sword::transition_lerp,
                    float(do_slash),
                    wi::math::Clamp(30.f*dt,0.f,1.f)
                ),
                wi::math::Clamp(30.f*dt,0.f,1.f)
            );

            static Gameplay::Helpers::GradientMap jump_forwardness_gradient({{0.f,0.f},{7.f,1.f}});
            _internal_::Anim_State::EventMSG::Jump::air_forwardness = jump_forwardness_gradient.Get(h_velocity);

            // Transition application
            _internal_::ActionBlends[Defines::ActionEnum::IDLE] = 
                (1.f - _internal_::Anim_State::EventMSG::Move::transition_lerp)
                *(1.f-_internal_::Anim_State::EventMSG::Jump::transition_lerp)
                *(1.f-_internal_::Anim_State::EventMSG::Move::dash_lerp)
                *(1.f-_internal_::Anim_State::EventMSG::Sword::transition_lerp);
            _internal_::ActionBlends[Defines::ActionEnum::MOVE] = 
                (_internal_::Anim_State::EventMSG::Move::transition_lerp)
                *(1.f-_internal_::Anim_State::EventMSG::Jump::transition_lerp)
                *(1.f-_internal_::Anim_State::EventMSG::Move::dash_lerp)
                *(1.f-_internal_::Anim_State::EventMSG::Sword::transition_lerp);
            _internal_::ActionBlends[Defines::ActionEnum::JUMP] = 
            _internal_::Anim_State::EventMSG::Jump::transition_lerp
                *(1.f-_internal_::Anim_State::EventMSG::Move::dash_lerp)
                *(1.f-_internal_::Anim_State::EventMSG::Sword::transition_lerp);
            _internal_::ActionBlends[Defines::ActionEnum::DASH] = 
                _internal_::Anim_State::EventMSG::Move::dash_lerp;
            _internal_::ActionBlends[Defines::ActionEnum::SLASH] = 
                (_internal_::Anim_State::EventMSG::Sword::transition_lerp)
                *(1.f-_internal_::Anim_State::EventMSG::Move::dash_lerp);
            _internal_::ActionBlends[Defines::ActionEnum::SHOOT] = 
                (_internal_::Anim_State::EventMSG::Shoot::transition_lerp)
                *(1.f-_internal_::Anim_State::EventMSG::Move::dash_lerp);
        }

        // Animation updates
        for(auto& action_pair : _internal_::Actions)
        {
            action_pair.second(_internal_::ActionBlends[action_pair.first]);
        }
        // ---

        // Camera updates
        // Set Camera Orientation
        if(cam_orient_dist > 0.1f)
        {
            cam_orient_vec.x *= 0.02f;//dt*0.8f;
            cam_orient_vec.y *= 0.02f;//dt*0.8f;

            Gameplay::GBus::Camera::AddPlayerCameraRotation(cam_orient_vec);
        }
        {
            static float l_r_sidle = 1.f;
            if(abs(move_vec.x) > 0.1f)
            {
                l_r_sidle = move_vec.x > 0.f ? -1.3f : 1.3f;
            }
            float l_r_sidle_lerp = (_internal_::Runtime_State::shoot_timer < (20.f/60.f)) ? l_r_sidle : -move_vec.x;

            XMFLOAT3 sidle = XMFLOAT3(l_r_sidle_lerp,0.f,_internal_::Runtime_State::zoom);
            _internal_::Runtime_State::sidle_lerp = wi::math::Lerp(
                _internal_::Runtime_State::sidle_lerp,
                wi::math::Lerp(
                    _internal_::Runtime_State::sidle_lerp,
                    sidle,
                    dt*10.f
                ),
                dt*10.f
            );
            Gameplay::GBus::Camera::SetPlayerMovementOffset(_internal_::Runtime_State::sidle_lerp);
        }
        // ---

        // Reattach ground parent (if player is on a ground)
        if(_internal_::Runtime_State::ground_entity != wi::ecs::INVALID_ENTITY)
            Game::GetScene().wiscene.Component_Attach(_internal_::Init_State::playerID, _internal_::Runtime_State::ground_entity);
    }
}

void Gameplay::Player::Hook_Update(float dt)
{
    auto prefab = Game::GetScene().prefabs.GetComponent(_internal_::Init_State::playerID);
    if(!_internal_::Runtime_State::scenedata_ready)
    {
        _internal_::Runtime_State::scenedata_ready = true;
        if(prefab != nullptr)
        {
            if(!prefab->loaded)
                _internal_::Runtime_State::scenedata_ready = false;
        }
        else
            _internal_::Runtime_State::scenedata_ready = false;
        
        if(!Game::GetScene().wiscene.transforms.Contains(_internal_::Init_State::playerID))
            _internal_::Runtime_State::scenedata_ready = false;
        
        // Animation check
        if(_internal_::Runtime_State::scenedata_ready)
        {
            for(auto& player_animationstr_pair : Defines::AnimationStrings)
            {
                wi::ecs::Entity animationID = prefab->FindEntityByName(player_animationstr_pair.second);
                _internal_::AnimationPtrs[player_animationstr_pair.first] = Game::GetScene().wiscene.animations.GetComponent(animationID);
                if(_internal_::AnimationPtrs[player_animationstr_pair.first] == nullptr)
                {
                    wi::backlog::post("[Player][ERROR] Animation \""+player_animationstr_pair.second+"\" is not in the prefab, please add animation data to Player with this name.");
                    _internal_::Runtime_State::scenedata_ready = false;
                    break;
                }
            }
        }

        // VFX reqs check
        if(_internal_::Runtime_State::scenedata_ready)
        {
            wi::ecs::Entity vfx_plasma_id = prefab->FindEntityByName("Player_Armor_Build_Turbine_VFX");
            if (vfx_plasma_id == wi::ecs::INVALID_ENTITY) _internal_::Runtime_State::scenedata_ready = false;
            wi::ecs::Entity vfx_spark_id = prefab->FindEntityByName("Player_Armor_Build_Turbine_VFX_ParticleSpawner");
            if (vfx_spark_id == wi::ecs::INVALID_ENTITY) _internal_::Runtime_State::scenedata_ready = false;
            wi::ecs::Entity vfx_ripple_id = prefab->FindEntityByName("Player_Armor_Build_Turbine_VFX_Ripple");
            if (vfx_ripple_id == wi::ecs::INVALID_ENTITY) _internal_::Runtime_State::scenedata_ready = false;

            wi::ecs::Entity weapon_grip_l = prefab->FindEntityByName("weapongrip_l");
            if (weapon_grip_l == wi::ecs::INVALID_ENTITY) _internal_::Runtime_State::scenedata_ready = false;
            wi::ecs::Entity weapon_grip_r = prefab->FindEntityByName("weapongrip_r");
            if (weapon_grip_r == wi::ecs::INVALID_ENTITY) _internal_::Runtime_State::scenedata_ready = false;
            wi::ecs::Entity weapon_hold_l = prefab->FindEntityByName("weaponhold_l");
            if (weapon_hold_l == wi::ecs::INVALID_ENTITY) _internal_::Runtime_State::scenedata_ready = false;
            wi::ecs::Entity weapon_hold_r = prefab->FindEntityByName("weaponhold_r");
            if (weapon_hold_r == wi::ecs::INVALID_ENTITY) _internal_::Runtime_State::scenedata_ready = false;

            if(_internal_::Runtime_State::scenedata_ready)
            {
                _internal_::Anim_State::EventMSG::Dash::plasma_object = Game::GetScene().wiscene.objects.GetComponent(vfx_plasma_id);
                _internal_::Anim_State::EventMSG::Dash::object_plasma_original_alpha = _internal_::Anim_State::EventMSG::Dash::plasma_object->color.w;
                _internal_::Anim_State::EventMSG::Dash::object_plasma_original_emitalpha = _internal_::Anim_State::EventMSG::Dash::plasma_object->emissiveColor.w;
                
                _internal_::Anim_State::EventMSG::Dash::spark_emitter = Game::GetScene().wiscene.emitters.GetComponent(vfx_spark_id);
                _internal_::Anim_State::EventMSG::Dash::particle_spark_original_count = _internal_::Anim_State::EventMSG::Dash::spark_emitter->count;
                // _internal_::Anim_State::EventMSG::Dash::spark_emitter->normal_factor = -0.1f;
                
                _internal_::Anim_State::EventMSG::Dash::ripple_emitter = Game::GetScene().wiscene.emitters.GetComponent(vfx_ripple_id);
                _internal_::Anim_State::EventMSG::Dash::particle_ripple_original_count = _internal_::Anim_State::EventMSG::Dash::ripple_emitter->count;
                _internal_::Anim_State::EventMSG::Dash::ripple_emitter->normal_factor = -0.01f;
            
                _internal_::Runtime_State::weapon_grip_l_entity = weapon_grip_l;
                _internal_::Runtime_State::weapon_grip_r_entity = weapon_grip_r;
                _internal_::Runtime_State::weapon_hold_l_entity = weapon_hold_l;
                _internal_::Runtime_State::weapon_hold_r_entity = weapon_hold_r;
            }
        }

        // Weapons check
        if(_internal_::Runtime_State::scenedata_ready)
        {
            for(auto& player_swordstr_pair : Defines::SwordStrings)
            {
                auto find_sword = _internal_::Swords.find(player_swordstr_pair.first);
                if(find_sword == _internal_::Swords.end())
                {
                    wi::ecs::Entity prefabID = wi::ecs::CreateEntity();
                    
                    wi::scene::TransformComponent& transform = Game::GetScene().wiscene.transforms.Create(prefabID);
                    transform.RotateRollPitchYaw(XMFLOAT3(-XM_PI/2.f,XM_PI,XM_PI));
                    Game::GetScene().wiscene.Component_Attach(prefabID,_internal_::Runtime_State::weapon_hold_l_entity,true);

                    Game::Scene::Prefab& prefab = Game::GetScene().prefabs.Create(prefabID);
                    prefab.file = Defines::FilesystemPaths::Resource_Swords+player_swordstr_pair.second+"/"+player_swordstr_pair.second+".wiscene";
                    prefab.stream_mode = Game::Scene::Prefab::StreamMode::DIRECT;
                    // prefab.fade_factor = 0.9f;

                    _internal_::Swords[player_swordstr_pair.first] = prefabID;

                    _internal_::Runtime_State::scenedata_ready = false;
                }
                else
                {
                    Game::Scene::Prefab* prefab = Game::GetScene().prefabs.GetComponent(find_sword->second);
                    if(!prefab->loaded)
                        _internal_::Runtime_State::scenedata_ready = false;
                }
            }

            for(auto& player_gunstr_pair : Defines::GunStrings)
            {
                auto find_gun = _internal_::Guns.find(player_gunstr_pair.first);
                if(find_gun == _internal_::Guns.end())
                {
                    wi::ecs::Entity prefabID = wi::ecs::CreateEntity();
                    
                    wi::scene::TransformComponent& transform = Game::GetScene().wiscene.transforms.Create(prefabID);
                    transform.RotateRollPitchYaw(XMFLOAT3(-XM_PI/2.f,XM_PI,XM_PI));
                    Game::GetScene().wiscene.Component_Attach(prefabID,_internal_::Runtime_State::weapon_hold_l_entity,true);

                    Game::Scene::Prefab& prefab = Game::GetScene().prefabs.Create(prefabID);
                    prefab.file = Defines::FilesystemPaths::Resource_Guns+player_gunstr_pair.second+"/"+player_gunstr_pair.second+".wiscene";
                    prefab.stream_mode = Game::Scene::Prefab::StreamMode::DIRECT;
                    // prefab.fade_factor = 0.9f;

                    _internal_::Guns[player_gunstr_pair.first] = prefabID;

                    _internal_::Runtime_State::scenedata_ready = false;
                }
                else
                {
                    Game::Scene::Prefab* prefab = Game::GetScene().prefabs.GetComponent(find_gun->second);
                    if(!prefab->loaded)
                        _internal_::Runtime_State::scenedata_ready = false;
                }
            }
        }

        // Final prep
        if(_internal_::Runtime_State::scenedata_ready)
        {
            wi::ecs::Entity hip_bone = prefab->FindEntityByName("spine_02");
            wi::scene::TransformComponent* hip_bone_transform = Game::GetScene().wiscene.transforms.GetComponent(hip_bone);

            wi::ecs::Entity head_bone = prefab->FindEntityByName("head");
            wi::scene::TransformComponent* head_bone_transform = Game::GetScene().wiscene.transforms.GetComponent(head_bone);

            wi::ecs::Entity animplayer_hip_entity = prefab->FindEntityByName(Defines::AnimationStrings[Defines::AnimationEnum::Player_Anim_Override_Hip]);
            uint32_t animplayer_hip_index = Game::GetScene().wiscene.animations.GetIndex(animplayer_hip_entity);
            Game::GetScene().wiscene.animations.MoveItem(animplayer_hip_index, Game::GetScene().wiscene.animations.GetCount()-1);

            wi::ecs::Entity animplayer_head_entity = prefab->FindEntityByName(Defines::AnimationStrings[Defines::AnimationEnum::Player_Anim_Override_Head]);
            uint32_t animplayer_head_index = Game::GetScene().wiscene.animations.GetIndex(animplayer_head_entity);
            Game::GetScene().wiscene.animations.MoveItem(animplayer_head_index, Game::GetScene().wiscene.animations.GetCount()-1);

            wi::ecs::Entity animplayer_gunshoot_entity = prefab->FindEntityByName(Defines::AnimationStrings[Defines::AnimationEnum::Player_Anim_GunShoot]);
            uint32_t animplayer_gunshoot_index = Game::GetScene().wiscene.animations.GetIndex(animplayer_gunshoot_entity);
            Game::GetScene().wiscene.animations.MoveItem(animplayer_gunshoot_index, Game::GetScene().wiscene.animations.GetCount()-1);

            // Recheck again
            for(auto& player_animationstr_pair : Defines::AnimationStrings)
            {
                wi::ecs::Entity animationID = prefab->FindEntityByName(player_animationstr_pair.second);
                _internal_::AnimationPtrs[player_animationstr_pair.first] = Game::GetScene().wiscene.animations.GetComponent(animationID);
            }

            wi::primitive::Ray spawn;
            spawn.direction.z = 10.f;
            _internal_::projectile_program_ID = wi::ecs::CreateEntity();
            GBus::Projectile::CreateProgram(spawn, _internal_::projectile_program_ID, "content/Scripts/ProjectilePrograms/basicvslice.lua");
            GBus::Projectile::SetShoot(false, _internal_::projectile_program_ID);
        }
    }
}

void Gameplay::Player::Hook_FixedUpdate()
{
}

void Gameplay::Player::Hook_Migrate(bool store, wi::vector<uint8_t>& storage)
{
    if(store)
    {
        auto ar_store = wi::Archive();
        ar_store << _internal_::Init_State::playerID;
        // ar_store << player_rotation_euler;
        ar_store.WriteData(storage);
    }
    else
    {
        auto ar_load = wi::Archive(storage.data());
        ar_load.SetReadModeAndResetPos(true);
        ar_load >> _internal_::Init_State::playerID;
        // ar_load >> player_rotation_euler;
    }
}
