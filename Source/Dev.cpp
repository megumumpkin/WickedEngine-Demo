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
            // Used by scene translation and preview extraction
            Dev::GetProcessData()->scene_offset = Game::GetScene()->wiscene.bounds.getCenter();
            XMVECTOR offset_inv_vector = XMVectorMultiply(XMLoadFloat3(&Dev::GetProcessData()->scene_offset), XMLoadFloat3(new XMFLOAT3(-1.f,-1.f,-1.f)));
            XMFLOAT3 offset_inv;
            XMStoreFloat3(&offset_inv, offset_inv_vector);

            // Translate scene to center
            {
                for(int i = 0; i < Game::GetScene()->wiscene.transforms.GetCount(); ++i)
                {
                    auto entity = Game::GetScene()->wiscene.transforms.GetEntity(i);
                    auto hierarchy = Game::GetScene()->wiscene.hierarchy.GetComponent(entity);
                    if(hierarchy != nullptr)
                    {
                        if(hierarchy->parentID != wi::ecs::INVALID_ENTITY)
                            continue;
                    }

                    wi::scene::TransformComponent& transform = Game::GetScene()->wiscene.transforms[i];
                    transform.Translate(offset_inv);
                }
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
                            
                            std::string texture_file = material.textures[i].name.substr(3,material.textures[i].name.length()-3);
                            std::string actual_texture_file = root_path + texture_file;
                            
                            std::string texture_ktx2 = wi::helper::ReplaceExtension(texture_file, "ktx2");
                            std::string actual_texture_ktx2 = root_path + texture_ktx2;

                            bool update = false;
                            if(wi::helper::FileExists(actual_texture_ktx2))
                            {
                                if(std::filesystem::last_write_time(actual_texture_ktx2) < std::filesystem::last_write_time(actual_texture_file))
                                    update = true;
                            }
                            else
                                update = true;

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
                            wi::scene::MeshComponent* mesh = Game::GetScene()->wiscene.meshes.GetComponent(object.meshID);
                            if(mesh != nullptr)
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
                                        
                                        entity_to_remove.insert(mesh->subsets[0].materialID);
                                    }
                                };

                                // Translate mesh's vertices
                                for(auto& v_pos : mesh->vertex_positions)
                                {
                                    XMVECTOR v_offset = XMVectorAdd(XMLoadFloat3(&v_pos), XMLoadFloat3(&(Dev::GetProcessData()->scene_offset)));
                                    XMStoreFloat3(&v_pos, v_offset);
                                }

                                // Save this mesh entity to archive
                                {
                                    wi::ecs::EntitySerializer seri;
                                    std::string preview_file = Game::Filesystem::GetActualPath(Dev::GetCommandData()->input);
                                    preview_file = wi::helper::ReplaceExtension(preview_file, "preview");
                                    wi::Archive ar_prev_mesh = wi::Archive(preview_file, false);
                                    Game::GetScene()->wiscene.Entity_Serialize(
                                            ar_prev_mesh,
                                            seri,
                                            object.meshID,
                                            wi::scene::Scene::EntitySerializeFlags::KEEP_INTERNAL_ENTITY_REFERENCES
                                        );
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

            std::string offset_file = wi::helper::ReplaceExtension(wiscene_file, "offset");
            wi::scene::TransformComponent offset_data;
            offset_data.translation_local = Dev::GetProcessData()->scene_offset;
            wi::ecs::EntitySerializer seri;
            wi::Archive ar_offset = wi::Archive(offset_file, false);
            offset_data.Serialize(ar_offset, seri);
            
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

void Dev::Execute()
{
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
                // Game::GetScene()->Load(GetCommandData()->input);
                wi::scene::LoadModel(Game::GetScene()->wiscene, Game::Filesystem::GetActualPath(GetCommandData()->input));
                execution_done = true;
                break;
            }
            case CommandData::CommandType::SCENE_EXTRACT:
            {
                break;
            }
        }
    }
}