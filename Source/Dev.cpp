#include "Dev.h"
#include "Filesystem.h"
#include "Scene.h"

#include <iostream>
#include <sstream>
#include <iterator>

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

void _DEV_scene_import()
{
    static uint32_t cycle = 0;
    switch (cycle)
    {
        case 0: // PHASE 1 - Import GLTF to Scene
        {
            std::string gltf_file = Game::Filesystem::GetActualPath(Dev::GetCommandData()->input) + "/model.gltf";
            Dev::IO::Import_GLTF(gltf_file, Game::GetScene()->wiscene);

            break;
        }
        case 1: // PHASE 2 - Transform the whole scene to be centered - Update Texture Assets - Extract preview mesh of the scene
        {
            // Extract scene boundary
            {
                std::string bounds_file = Game::Filesystem::GetActualPath(Dev::GetCommandData()->input);
                bounds_file = wi::helper::ReplaceExtension(bounds_file, "bounds");
                wi::ecs::EntitySerializer seri;
                wi::Archive ar_bounds = wi::Archive(bounds_file, false);
                Game::GetScene()->wiscene.bounds.Serialize(ar_bounds, seri);
            }

            // Make texture paths relative to wiscene file
            {
                // Update lens flare and material textures only!
                // Rename extension to ktx2 (conversion is done before this through Blender)
                for(int i = 0; i < Game::GetScene()->wiscene.materials.GetCount(); ++i)
                {
                    wi::scene::MaterialComponent& material = Game::GetScene()->wiscene.materials[i];
                    for(int i = 0; i < wi::scene::MaterialComponent::TEXTURESLOT_COUNT; ++i)
                    {
                        if(material.textures[i].name != "")
                        {
                            // Convert textures here and now, but check first
                            std::string root_path = wi::helper::GetDirectoryFromPath(Game::Filesystem::GetActualPath(Dev::GetCommandData()->input));
                            
                            // std::string texture_file = material.textures[i].name.substr(3,material.textures[i].name.length()-3);
                            std::string texture_file = material.textures[i].name.substr(root_path.length(), material.textures[i].name.length()-root_path.length());
                            std::string actual_texture_file = root_path + texture_file;
                            
                            std::string texture_ktx2 = wi::helper::ReplaceExtension(texture_file, (i == wi::scene::MaterialComponent::NORMALMAP) ? wi::helper::GetExtensionFromFileName(texture_file) : "ktx2");
                            std::string actual_texture_ktx2 = root_path + texture_ktx2;

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
                                auto& resource = material.textures[i].resource;
                                // wi::helper::saveTextureToFile(*std::static_pointer_cast<wi::graphics::Texture>(material.textures[i].GetGPUResource()->internal_state),texture_ktx2);
                                if(resource.IsValid() && resource.GetTexture().IsTexture())
                                {
                                    auto& texture = resource.GetTexture();
                                    wi::vector<uint8_t> filedata;
                                    if(wi::helper::saveTextureToMemory(resource.GetTexture(), filedata))
                                    {
                                        resource.SetFileData(std::move(filedata));

                                        wi::vector<uint8_t> filedata_compressed;
                                        wi::helper::saveTextureToMemoryFile(resource.GetFileData(), resource.GetTexture().desc, "KTX2", filedata_compressed);
                                        
                                        wi::helper::FileWrite(actual_texture_ktx2, filedata_compressed.data(), filedata_compressed.size());
                                    }
                                }
                            }

                            material.textures[i].name = texture_ktx2;
                            // material.textures[i].name = texture_file;
                        }

                    }
                }
            }

            // Extract preview mesh
            {
                wi::unordered_set<wi::ecs::Entity> entity_to_remove;
                // Find name prefix PREVIEW_
                for(int i = 0; i < Game::GetScene()->wiscene.objects.GetCount(); ++i)
                {
                    auto entity = Game::GetScene()->wiscene.objects.GetEntity(i);
                    auto name = Game::GetScene()->wiscene.names.GetComponent(entity);
                    wi::scene::ObjectComponent& object = Game::GetScene()->wiscene.objects[i];
                    if(name != nullptr)
                    {
                        if(name->name.substr(0,8) == "PREVIEW_")
                        {
                            wi::scene::TransformComponent* transform = Game::GetScene()->wiscene.transforms.GetComponent(entity);
                            wi::scene::MeshComponent* mesh = Game::GetScene()->wiscene.meshes.GetComponent(object.meshID);
                            if((transform != nullptr) && (mesh != nullptr))
                            {
                                // Rename the mesh entity name to a format that is known
                                auto mesh_name = Game::GetScene()->wiscene.names.GetComponent(object.meshID);
                                if(mesh_name == nullptr)
                                {
                                    auto& new_mesh_name = Game::GetScene()->wiscene.names.Create(object.meshID);
                                    mesh_name = Game::GetScene()->wiscene.names.GetComponent(object.meshID);
                                }
                                mesh_name->name = "PREVIEW_"+wi::helper::RemoveExtension(wi::helper::GetFileNameFromPath(Dev::GetCommandData()->input));

                                // Transfer material to the same entity as mesh
                                if(mesh->subsets[0].materialID != object.meshID)
                                {
                                    if(Game::GetScene()->wiscene.materials.Contains(mesh->subsets[0].materialID))
                                    {
                                        wi::ecs::EntitySerializer seri;
                                        wi::Archive ar_copy_material;
                                        Game::GetScene()->wiscene.materials.Component_Serialize(mesh->subsets[0].materialID, ar_copy_material, seri);
                                        
                                        ar_copy_material.SetReadModeAndResetPos(true);
                                        Game::GetScene()->wiscene.materials.Component_Serialize(object.meshID, ar_copy_material, seri);
                                        mesh->subsets[0].materialID = object.meshID;
                                        
                                        entity_to_remove.insert(mesh->subsets[0].materialID);
                                    }
                                };

                                // Save this mesh entity to archive
                                {
                                    wi::ecs::EntitySerializer seri;
                                    std::string preview_file = Game::Filesystem::GetActualPath(Dev::GetCommandData()->input);
                                    preview_file = wi::helper::ReplaceExtension(preview_file, "preview");
                                    wi::Archive ar_prev_mesh = wi::Archive(preview_file, false);
                                    // Store object offset position
                                    transform->Serialize(ar_prev_mesh, seri);
                                    // Store mesh
                                    Game::GetScene()->wiscene.Entity_Serialize(
                                            ar_prev_mesh,
                                            seri,
                                            object.meshID,
                                            wi::scene::Scene::EntitySerializeFlags::KEEP_INTERNAL_ENTITY_REFERENCES
                                        );
                                    wi::jobsystem::Wait(seri.ctx);
                                }

                                entity_to_remove.insert(object.meshID);
                            }

                            entity_to_remove.insert(entity);

                            break;
                        }
                    }
                }
                for(auto& entity : entity_to_remove)
                {
                    Game::GetScene()->wiscene.Entity_Remove(entity,false);
                }
            }
            break;
        }
        case 2: // PHASE 3 - Save to Wiscene and Quit
        {
            std::string wiscene_file = Game::Filesystem::GetActualPath(Dev::GetCommandData()->input);
            wiscene_file = wi::helper::ReplaceExtension(wiscene_file, "wiscene");
            
            wi::Archive scene_save = wi::Archive(wiscene_file, false);
            Game::GetScene()->wiscene.Serialize(scene_save);

            break;
        }
        case 3:
        {
            wi::platform::Exit();
            break;
        }
    }

    cycle++;
}

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
    Game::GetScene()->stream_loader_bounds.x = stream_pos.x;
    Game::GetScene()->stream_loader_bounds.y = stream_pos.y;
    Game::GetScene()->stream_loader_bounds.z = stream_pos.z;
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
                // We load our wiscene here
                Game::GetScene()->Load(GetCommandData()->input);
                // wi::scene::LoadModel(Game::GetScene()->wiscene, Game::Filesystem::GetActualPath(GetCommandData()->input));
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
        _internal_updateDevCamera(dt);
}