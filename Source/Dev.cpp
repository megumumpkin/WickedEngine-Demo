#include "Dev.h"
#include "Filesystem.h"
#include "Scene.h"

#include <iostream>
#include <map>
#include <sstream>
#include <iterator>
#include <filesystem>

#include <reproc++/run.hpp>

Dev::CommandData* Dev::GetCommandData()
{
    static Dev::CommandData command_data;
    return &command_data;
}

Dev::ProcessData* Dev::GetProcessData()
{
    static Dev::ProcessData process_data;
    return &process_data;
}

static const wi::unordered_map<std::string, Dev::CommandData::CommandType> CommandTypeLookup = {
    {"SCENE_IMPORT", Dev::CommandData::CommandType::SCENE_IMPORT},
    {"SCENE_PREVIEW", Dev::CommandData::CommandType::SCENE_PREVIEW},
    {"SCENE_EXTRACT", Dev::CommandData::CommandType::SCENE_EXTRACT}
};

static const std::string HelpMenuStr = R"([ Game Devtool Command Help ]
Usage: Dev [OPTIONS]
Run a Dev game runtime with commands, if no commands is input, the Dev program will run the program as a finished game with debug menus.

To Note:
THE DEV TOOL WILL START THE PATH READING FROM Data/Content FOLDER, ACTING AS ROOT PATH.
ANY FILE OUTSIDE THIS FOLDER WON'T BE PROCESSED OR WILL HAVE ERRORS ON THE OUTPUT.

Options:
  -h  Show this help
  -t  Set which type of execution will the Dev program do
  -i  File to be input, either a wiscene file or assetsmith, depending on the -t command
  -o  File to be output, depends on the commands that are used

-t Available Inputs:
  SCENE_IMPORT    Imports the .assetsmith scene into engine type scene
                  Usage:   Dev -t SCENE_IMPORT -i my_scene.assetsmith

  SCENE_PREVIEW   Preview the desired scene
                  Usage:   Dev -t SCENE_PREVIEW -i my_scene.wiscene

  SCENE_EXTRACT   Extract the scene into a GLTF file (.glb)
                  Usage:   Dev -t SCENE_EXTRACT -i my_scene.wiscene -o my_scene.glb
)";

bool _internal_ReadCMD(wi::vector<std::string>& args)
{
    for(size_t i = 0; i < args.size(); ++i)
    {
        std::string arg_str = std::string(args[i]);
        if(arg_str[0] == '-')
        {
            switch(arg_str[1])
            {
                case 'h':
                {
                    std::cout << HelpMenuStr << std::endl;
                    return false;
                    break;
                }
                case 't':
                {
                    if ((i+1) < args.size())
                    {
                        Dev::GetCommandData()->type = CommandTypeLookup.find(std::string(args[i+1]))->second;
                    }
                    break;
                }
                case 'i':
                {
                    if ((i+1) < args.size())
                    {
                        Dev::GetCommandData()->input = "content/" + std::string(args[i+1]);
                    }
                    break;
                }
                case 'o':
                {
                    if ((i+1) < args.size())
                    {
                        Dev::GetCommandData()->output = "content/" + std::string(args[i+1]);
                    }
                    break;
                }

                default:
                    std::cout << "Running Dev as full game with debug menu" << std::endl;
            }
            ++i;
        }
    }
    return true;
}

bool Dev::ReadCMD(int argc, char *argv[])
{
    wi::vector<std::string> args;
    for (int i = 1; i < argc; i++)
    {
        args.push_back(std::string(argv[i]));
    }
    return _internal_ReadCMD(args);
}

bool Dev::ReadCMD(const wchar_t* win_args)
{
    wi::vector<std::string> args;

    std::wstring from = win_args;
    std::string to;
    wi::helper::StringConvert(from, to);

    std::istringstream iss(to);

    args =
    {
        std::istream_iterator<std::string>{iss},
        std::istream_iterator<std::string>{}
    };
    return _internal_ReadCMD(args);
}

wi::vector<std::string> Dev::IO::importdata_lodtier;

void _DEV_scene_import()
{
    static uint32_t cycle = 0;
    switch (cycle)
    {
        case 0: // PHASE 1 - Import GLTF to Scene
        {
            wi::resourcemanager::SetMode(wi::resourcemanager::Mode::ALLOW_RETAIN_FILEDATA_BUT_DISABLE_EMBEDDING);

            std::string gltf_file = Game::Filesystem::GetActualPath(Dev::GetCommandData()->input) + "/model.gltf";
            Dev::IO::Import_GLTF(gltf_file, Game::GetScene().wiscene);

            wi::backlog::post("GLTF Conversion :"+gltf_file);

            cycle++;
            break;
        }
        case 1: // PHASE 2 - Transform the whole scene to be centered - Update Texture Assets - Extract preview mesh of the scene
        {
            std::string bounds_file = Game::Filesystem::GetActualPath(Dev::GetCommandData()->input);
            bounds_file = wi::helper::ReplaceExtension(bounds_file, "bounds");
            wi::ecs::EntitySerializer seri;
            wi::Archive ar_bounds = wi::Archive(bounds_file, false);
            Game::GetScene().wiscene.bounds.Serialize(ar_bounds, seri);

            wi::backlog::post("Streaming Bounds File Generated: "+bounds_file);
            wi::backlog::post("Now Processing Textures");

            cycle++;
            break;
        }
        case 2:
        // Make texture paths relative to wiscene file
        {
            // Update lens flare and material textures only!
            // Rename extension to ktx2 (conversion is done before this through Blender)

            std::string root_path = wi::helper::GetDirectoryFromPath(Game::Filesystem::GetActualPath(Dev::GetCommandData()->input));
            
            wi::jobsystem::context convert_ctx;
            for(int i = 0; i < Game::GetScene().wiscene.materials.GetCount(); ++i)
            {
                wi::scene::MaterialComponent& material = Game::GetScene().wiscene.materials[i];
                for(int j = 0; j < wi::scene::MaterialComponent::TEXTURESLOT_COUNT; ++j)
                {
                    if(material.textures[j].name != "")
                    {
                        // wi::backlog::post("Processing Texture: "+material.textures[j].name);
                        // Convert textures here and now, but check first
                        std::string texture_file = material.textures[j].name.substr(3,material.textures[j].name.length()-3);
                        texture_file = material.textures[j].name.substr(root_path.length(), material.textures[j].name.length()-root_path.length());
                        std::string actual_texture_file = std::filesystem::absolute(root_path).append(texture_file).lexically_normal().generic_string();
                        
                        std::string texture_ktx2 = wi::helper::ReplaceExtension(texture_file, "ktx2");
                        std::string actual_texture_ktx2 = wi::helper::ReplaceExtension(actual_texture_file, "ktx2");

                        bool update = false;
                        if(wi::helper::FileExists(actual_texture_ktx2))
                        {
                            if(std::filesystem::last_write_time(actual_texture_ktx2) < std::filesystem::last_write_time(actual_texture_file))
                                update = true;
                        }
                        else
                            update = true;

                        if(texture_file == texture_ktx2)
                            update = false;

                        if(update)
                        {
                            wi::vector<std::string> args = {
#ifdef _WIN32
                                wi::helper::GetCurrentPath()+"/Tools/KTX-Software/win32/bin/toktx.exe",
#else
                                wi::helper::GetCurrentPath()+"/Tools/KTX-Software/linux/bin/toktx",
#endif // _WIN32
                                "--genmipmap",
                                "--assign_oetf",(j == wi::scene::MaterialComponent::BASECOLORMAP) ? "srgb" : "linear",
                                "--encode","uastc",
                                "--uastc_quality","0",
                                "--zcmp","15",
                                "--t2",
                                actual_texture_ktx2,
                                actual_texture_file};
                            auto process = reproc::process();
                            reproc::options shexec_options;
                            reproc::stop_actions shexec_options_stop = {
                                { reproc::stop::terminate, reproc::milliseconds(5000) },
                                { reproc::stop::kill, reproc::milliseconds(2000) },
                                {}
                            };
                            shexec_options.stop = shexec_options_stop;
#ifndef _WIN32
                            static std::map<std::string, std::string> shexec_env_map = {{"LD_LIBRARY_PATH",wi::helper::GetCurrentPath()+"/Tools/KTX-Software/linux/lib"}};
                            shexec_options.env.extra = reproc::env(shexec_env_map);
                            shexec_options.env.behavior = reproc::env::extend;
#endif // _WIN32
                            process.start(args, shexec_options);
                            wi::jobsystem::Execute(convert_ctx, [&](wi::jobsystem::JobArgs jobArgs){
                                reproc::drain(process, reproc::sink::null, reproc::sink::null);
                                process.stop(shexec_options.stop);
                            });
                            wi::jobsystem::Wait(convert_ctx);
                        }

                        material.textures[j].name = texture_ktx2;
                        // wi::backlog::post("Processed Name: "+material.textures[j].name);
                    }

                }
            }
            cycle++;
            break;
        }
        case 3:
        // Extract preview mesh
        {
            wi::backlog::post("Processing Textures Done");

            std::string preview_file = Game::Filesystem::GetActualPath(Dev::GetCommandData()->input);
            preview_file = wi::helper::ReplaceExtension(preview_file, "preview");
            wi::Archive ar_preview = wi::Archive(preview_file, false);

            wi::ecs::EntitySerializer seri;
            seri.allow_remap = false;

            ar_preview << Dev::IO::importdata_lodtier.size();
            for(auto lodtier_str : Dev::IO::importdata_lodtier)
            {
                wi::ecs::Entity prefabID = Game::GetScene().wiscene.Entity_FindByName(lodtier_str);

                Game::GetScene().wiscene.componentLibrary.Entity_Serialize(prefabID, ar_preview, seri);
                wi::jobsystem::Wait(seri.ctx);
                
                Game::GetScene().wiscene.Entity_Remove(prefabID);
            }

            wi::backlog::post("Processing LOD Tier Data Done!");

            cycle++;
            break;
        }
        
        case 4: // PHASE 3 - Save to Wiscene and Quit
        {
            std::string wiscene_file = Game::Filesystem::GetActualPath(Dev::GetCommandData()->input);
            wiscene_file = wi::helper::ReplaceExtension(wiscene_file, "wiscene");
            
            wi::Archive scene_save = wi::Archive(wiscene_file, false);
            Game::GetScene().wiscene.Serialize(scene_save);

            wi::backlog::post("Wiscene Conversion Done: "+wiscene_file);
            wi::backlog::post("[<-- ! -->] You can close this window now [<-- ! -->]");

            cycle++;
            break;
        }
        // case 5:
        // {
        //     wi::platform::Exit();
        //     break;
        // }
    }
}

bool USE_DEV_CAMERA = false;
void _internal_updateDevCamera(float dt)
{
    static wi::scene::TransformComponent devCameraTransform;
    static XMFLOAT3 devCameraMove;
    float devCamMoveSPD = 0.25f;
    wi::scene::CameraComponent& camera = wi::scene::GetCamera();
    // float devCamRotSPD;

    XMFLOAT4 currentMouse = wi::input::GetPointer();
    static XMFLOAT4 originalMouse = XMFLOAT4(0, 0, 0, 0);
    static bool camControlStart = true;
    if (camControlStart)
    {
        originalMouse = wi::input::GetPointer();
    }

    float xDif = 0, yDif = 0;

    if (wi::input::Down(wi::input::MOUSE_BUTTON_MIDDLE))
    {
        camControlStart = false;
#if 0
        // Mouse delta from previous frame:
        xDif = currentMouse.x - originalMouse.x;
        yDif = currentMouse.y - originalMouse.y;
#else
        // Mouse delta from hardware read:
        xDif = wi::input::GetMouseState().delta_position.x;
        yDif = wi::input::GetMouseState().delta_position.y;
#endif
        xDif = 0.1f * xDif * (1.0f / 60.0f);
        yDif = 0.1f * yDif * (1.0f / 60.0f);
        wi::input::SetPointer(originalMouse);
        wi::input::HidePointer(true);
}
    else
    {
        camControlStart = true;
        wi::input::HidePointer(false);
    }

    const float buttonrotSpeed = 16.0f * dt;
    if (wi::input::Down(wi::input::KEYBOARD_BUTTON_LEFT))
    {
        xDif -= buttonrotSpeed;
    }
    if (wi::input::Down(wi::input::KEYBOARD_BUTTON_RIGHT))
    {
        xDif += buttonrotSpeed;
    }
    if (wi::input::Down(wi::input::KEYBOARD_BUTTON_UP))
    {
        yDif -= buttonrotSpeed;
    }
    if (wi::input::Down(wi::input::KEYBOARD_BUTTON_DOWN))
    {
        yDif += buttonrotSpeed;
    }

    const XMFLOAT4 leftStick = wi::input::GetAnalog(wi::input::GAMEPAD_ANALOG_THUMBSTICK_L, 0);
    const XMFLOAT4 rightStick = wi::input::GetAnalog(wi::input::GAMEPAD_ANALOG_THUMBSTICK_R, 0);
    const XMFLOAT4 rightTrigger = wi::input::GetAnalog(wi::input::GAMEPAD_ANALOG_TRIGGER_R, 0);

    const float jostickrotspeed = 0.05f;
    xDif += rightStick.x * jostickrotspeed;
    yDif += rightStick.y * jostickrotspeed;

    xDif *= 0.18f;
    yDif *= 0.18f;

    // FPS Camera
    const float clampedDT = std::min(dt, 0.1f); // if dt > 100 millisec, don't allow the camera to jump too far...

    const float speed = ((wi::input::Down(wi::input::KEYBOARD_BUTTON_LSHIFT) ? 10.0f : 1.0f) + rightTrigger.x * 10.0f) * devCamMoveSPD * clampedDT;
    XMVECTOR move = XMLoadFloat3(&devCameraMove);
    XMVECTOR moveNew = XMVectorSet(leftStick.x, 0, leftStick.y, 0);

    if (!wi::input::Down(wi::input::KEYBOARD_BUTTON_LCONTROL))
    {
        // Only move camera if control not pressed
        if (wi::input::Down((wi::input::BUTTON)'A') || wi::input::Down(wi::input::GAMEPAD_BUTTON_LEFT)) { moveNew += XMVectorSet(-1, 0, 0, 0); }
        if (wi::input::Down((wi::input::BUTTON)'D') || wi::input::Down(wi::input::GAMEPAD_BUTTON_RIGHT)) { moveNew += XMVectorSet(1, 0, 0, 0); }
        if (wi::input::Down((wi::input::BUTTON)'W') || wi::input::Down(wi::input::GAMEPAD_BUTTON_UP)) { moveNew += XMVectorSet(0, 0, 1, 0); }
        if (wi::input::Down((wi::input::BUTTON)'S') || wi::input::Down(wi::input::GAMEPAD_BUTTON_DOWN)) { moveNew += XMVectorSet(0, 0, -1, 0); }
        if (wi::input::Down((wi::input::BUTTON)'E') || wi::input::Down(wi::input::GAMEPAD_BUTTON_2)) { moveNew += XMVectorSet(0, 1, 0, 0); }
        if (wi::input::Down((wi::input::BUTTON)'Q') || wi::input::Down(wi::input::GAMEPAD_BUTTON_1)) { moveNew += XMVectorSet(0, -1, 0, 0); }
        moveNew += XMVector3Normalize(moveNew);
    }
    moveNew *= speed;

    move = XMVectorLerp(move, moveNew, devCamMoveSPD * clampedDT / 0.0166f); // smooth the movement a bit
    float moveLength = XMVectorGetX(XMVector3Length(move));

    if (moveLength < 0.0001f)
    {
        move = XMVectorSet(0, 0, 0, 0);
    }

    if (abs(xDif) + abs(yDif) > 0 || moveLength > 0.0001f)
    {
        XMMATRIX camRot = XMMatrixRotationQuaternion(XMLoadFloat4(&devCameraTransform.rotation_local));
        XMVECTOR move_rot = XMVector3TransformNormal(move, camRot);
        XMFLOAT3 _move;
        XMStoreFloat3(&_move, move_rot);
        devCameraTransform.Translate(_move);
        devCameraTransform.RotateRollPitchYaw(XMFLOAT3(yDif, xDif, 0));
        camera.SetDirty();
    }

    devCameraTransform.UpdateTransform();
    XMStoreFloat3(&devCameraMove, move);

    camera.TransformCamera(devCameraTransform);
    camera.UpdateCamera();
    auto stream_pos = devCameraTransform.GetPosition();
    Game::GetScene().stream_loader_bounds.x = stream_pos.x;
    Game::GetScene().stream_loader_bounds.y = stream_pos.y;
    Game::GetScene().stream_loader_bounds.z = stream_pos.z;
}

void Dev::Execute(float dt)
{
    static bool run_gamescene_update = false;
    static bool execution_done = false;
    if(!execution_done)
    {
        switch(GetCommandData()->type)
        {
            case CommandData::CommandType::SCENE_IMPORT:
            {
                _DEV_scene_import();
                break;
            }
            case CommandData::CommandType::SCENE_PREVIEW:
            {
                // Register editor filesystem here
                Game::Filesystem::Register_FS("editor/", "Data/Editor/", false);

                // We load our wiscene here
                Game::GetScene().Load(GetCommandData()->input);
                wi::lua::RunFile(Game::Filesystem::GetActualPath("editor/editorsky.lua"));
                // // We load our skymap here
                // std::string skymap_file = Game::Filesystem::GetActualPath("editor/kloofendal_43d_clear_4k.hdr");
                // Game::GetScene().wiscene.weather.skyMapName = skymap_file;
                // Game::GetScene().wiscene.weather.skyMap = wi::resourcemanager::Load(skymap_file, wi::resourcemanager::Flags::IMPORT_RETAIN_FILEDATA);
                // Game::GetScene().wiscene.weather.sky_rotation = wi::math::PI*1.2f;
                // wi::scene::LoadModel(Game::GetScene().wiscene, Game::Filesystem::GetActualPath(GetCommandData()->input));
                run_gamescene_update = true;
                execution_done = true;
                break;
            }
            case CommandData::CommandType::SCENE_EXTRACT:
            {
                break;
            }
        }
    }

    if(run_gamescene_update)
    {
        if(USE_DEV_CAMERA)
            _internal_updateDevCamera(dt);
        if(wi::input::Press(wi::input::KEYBOARD_BUTTON_F5))
        {
            USE_DEV_CAMERA = !USE_DEV_CAMERA;
        }
    }
}