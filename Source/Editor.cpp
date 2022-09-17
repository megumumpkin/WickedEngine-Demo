#include "Editor.h"
#include <wiMath_BindLua.h>

Editor::Data* Editor::GetData(){
    static Data data;
    return &data;
}

int Editor_SetGridHelper(lua_State* L){
    int argc = wi::lua::SGetArgCount(L);
    if(argc > 0)
    {
        bool set = wi::lua::SGetBool(L, 1);
        wi::renderer::SetToDrawGridHelper(set);
    }
    else {
        wi::lua::SError(L, "editor_dev_griddraw(bool set) not enough arguments!");
    }
    return 0;
}

int Editor_UIFocused(lua_State* L){
    ImGuiIO& io = ::ImGui::GetIO();
    wi::lua::SSetBool(L,io.WantCaptureMouse);
    return 1;
}

int Editor_IsEntityListUpdated(lua_State* L)
{
    auto& wiscene = Game::Resources::GetScene().wiscene;
    auto& entity_list = wiscene.names.GetEntityArray();

    bool changed = (Editor::GetData()->entitylist_sizecache != entity_list.size());
    if(changed){ 
        Editor::GetData()->entitylist_sizecache = entity_list.size();
        if(Editor::GetData()->transform_translator.selected.size() > 0)
            if(!Game::Resources::GetScene().wiscene.transforms.Contains(Editor::GetData()->transform_translator.selected[0].entity))
                Editor::GetData()->transform_translator.selected.clear();
    }
    wi::lua::SSetBool(L, changed);
    return 1;
}

Editor::GizmoData Editor_SetGizmo(wi::ecs::Entity entity)
{
    auto& wiscene = Game::Resources::GetScene().wiscene;
    Editor::GizmoData gizmoData;
    gizmoData.entity = entity;
    gizmoData.icon = wiscene.decals.Contains(entity) ? ICON_DECAL 
        : wiscene.forces.Contains(entity) ? ICON_FORCE
        : wiscene.cameras.Contains(entity) ? ICON_CAMERA
        : wiscene.armatures.Contains(entity) ? ICON_ARMATURE
        : wiscene.emitters.Contains(entity) ? ICON_EMITTER
        : wiscene.hairs.Contains(entity) ? ICON_HAIR
        : wiscene.sounds.Contains(entity) ? ICON_SOUND
        : "";

    auto lightComponent = wiscene.lights.GetComponent(entity);
    if (lightComponent != nullptr)
    {
        gizmoData.icon = (lightComponent->type == wi::scene::LightComponent::SPOT) ? ICON_POINTLIGHT 
            : (lightComponent->type == wi::scene::LightComponent::DIRECTIONAL) ? ICON_DIRECTIONALLIGHT
            : ICON_POINTLIGHT;
    }

    auto objectComponent = wiscene.objects.GetComponent(entity);
    if (objectComponent != nullptr)
    {
        if (!wiscene.meshes.Contains(objectComponent->meshID))
        {
            gizmoData.icon = ICON_OBJECT;
        }
    }
    return gizmoData;
}

int Editor_UpdateGizmoData(lua_State* L)
{
    auto argc = wi::lua::SGetArgCount(L);
    if (argc > 0)
    {
        wi::ecs::Entity entity = wi::lua::SGetLongLong(L, 1);
        auto gizmo_set = Editor_SetGizmo(entity);
        for (auto& gizmo : Editor::GetData()->gizmo_data)
        {
            if(gizmo.entity == gizmo_set.entity)
            {
                gizmo.icon = gizmo_set.icon;
                break;
            }
        }
    }
    return 0;
}

int Editor_GetObjectList(lua_State* L)
{
    auto& wiscene = Game::Resources::GetScene().wiscene;
    auto& entity_list = wiscene.transforms.GetEntityArray();

    Editor::GetData()->gizmo_data.clear();

    wi::unordered_map<wi::ecs::Entity, wi::vector<wi::ecs::Entity>> reduced_hierarchy_group;
    auto jobCount = (uint32_t)std::ceil(entity_list.size()/128.f);
    
    std::mutex enlistThreadsMutex;
    wi::jobsystem::context enlistThreadGroup;

    wi::vector<wi::unordered_map<wi::ecs::Entity, wi::vector<wi::ecs::Entity>>> thread_hierarchy_group;
    wi::vector<wi::vector<Editor::GizmoData>> thread_gizmo_data;
    thread_hierarchy_group.resize(jobCount);
    thread_gizmo_data.resize(jobCount);

    wi::jobsystem::Dispatch(enlistThreadGroup, jobCount, 1, [&](wi::jobsystem::JobArgs args)
    {
        std::scoped_lock lock (enlistThreadsMutex);

        auto& map_hierarchy_group = thread_hierarchy_group[args.jobIndex];
        auto& map_gizmo_data = thread_gizmo_data[args.jobIndex];

        size_t find_max = std::min(entity_list.size(),(size_t)(args.jobIndex+1)*128);
        for(size_t i = args.jobIndex*128; i < find_max; ++i)
        {
            auto entity = entity_list[i];
            auto has_hierarchy = wiscene.hierarchy.GetComponent(entity);
            if(has_hierarchy != nullptr){
                map_hierarchy_group[has_hierarchy->parentID].push_back(entity);
            } else {
                map_hierarchy_group[0].push_back(entity);
            }

            // Rebuilding Gizmo List
            
            auto gizmoData = Editor_SetGizmo(entity);

            if (gizmoData.icon != "")
            {
                map_gizmo_data.push_back(gizmoData);
            }
        }
    });

    wi::jobsystem::Wait(enlistThreadGroup);

    for(auto& map_hierarchy_group : thread_hierarchy_group)
    {
        for(auto& map_pair : map_hierarchy_group)
        {
            reduced_hierarchy_group[map_pair.first].insert(reduced_hierarchy_group[map_pair.first].end(), map_pair.second.begin(), map_pair.second.end());
        }
    }

    for(auto& map_gizmo_data : thread_gizmo_data)
    {
        Editor::GetData()->gizmo_data.insert(Editor::GetData()->gizmo_data.end(), map_gizmo_data.begin(), map_gizmo_data.end());
    }

    if(reduced_hierarchy_group.size() > 0)
    {
        lua_newtable(L);
        for(auto& map_pair : reduced_hierarchy_group)
        {
            auto& entities = map_pair.second;

            lua_newtable(L);
            for(int i = 0; i < entities.size(); ++i)
            {
                lua_pushnumber(L, entities[i]);
                lua_rawseti(L,-2,i);
            }
            lua_setfield(L, -2, std::to_string(map_pair.first).c_str());
        }
        return 1;
    }
    return 0;
}

inline void Editor_UpdateSelection()
{
    if((Editor::GetData()->selection.entity != wi::ecs::INVALID_ENTITY) && (Game::Resources::GetScene().wiscene.transforms.Contains(Editor::GetData()->selection.entity)))
    {
        if(Editor::GetData()->transform_translator.selected.empty())
        {
            auto selection = Editor::GetData()->selection;
            Editor::GetData()->transform_translator.selected.push_back(Editor::GetData()->selection);

            auto new_object = Game::Resources::GetScene().wiscene.objects.GetComponent(selection.entity);
            if (new_object)
            {
                new_object->SetUserStencilRef(Editor::EDITOR_STENCILREF_LAST);
            }
        }
        else
        {
            auto& selection = Editor::GetData()->transform_translator.selected.back();
            auto old_entity = selection.entity;
            selection = Editor::GetData()->selection;

            auto old_object = Game::Resources::GetScene().wiscene.objects.GetComponent(old_entity);
            if (old_object)
            {
                old_object->SetUserStencilRef(Editor::EDITOR_STENCILREF_NONE);
            }

            auto new_object = Game::Resources::GetScene().wiscene.objects.GetComponent(selection.entity);
            if (new_object)
            {
                new_object->SetUserStencilRef(Editor::EDITOR_STENCILREF_LAST);
            }
        }
    }
    else
    {
        if(!Editor::GetData()->transform_translator.selected.empty())
        {
            auto& selection = Editor::GetData()->transform_translator.selected.back();
            auto old_entity = selection.entity;
            auto old_object = Game::Resources::GetScene().wiscene.objects.GetComponent(old_entity);
            if (old_object)
            {
                old_object->SetUserStencilRef(Editor::EDITOR_STENCILREF_NONE);
            }


            Editor::GetData()->transform_translator.selected.clear();
        }
    }
}

int Editor_FetchSelection(lua_State* L)
{
    auto argc = wi::lua::SGetArgCount(L);
    if (argc > 0)
    {
        wi::ecs::Entity selected = (wi::ecs::Entity)wi::lua::SGetInt(L, 1);
        Editor::GetData()->selection = wi::scene::PickResult();
        Editor::GetData()->selection.entity = selected;
        Editor_UpdateSelection();
    }
    else
    {
        wi::lua::SError(L, "Editor_FetchSelection(Entity selected) not enough arguments!");
    }

    return 0;
}

void Editor_ProcessHovered()
{
    auto& scene = Game::Resources::GetScene();
    auto& wiscene = scene.wiscene;

    XMFLOAT4 currentMouse = wi::input::GetPointer();
    wi::primitive::Ray pickRay = wi::renderer::GetPickRay((long)currentMouse.x, (long)currentMouse.y, *Editor::GetData()->viewport, Game::Resources::GetScene().wiscene.camera);

    auto hovered = wi::scene::PickResult();

    for (auto& gizmo : Editor::GetData()->gizmo_data)
    {
        if (!wiscene.transforms.Contains(gizmo.entity))
            continue;
        const wi::scene::TransformComponent& transform = *wiscene.transforms.GetComponent(gizmo.entity);

        XMVECTOR disV = XMVector3LinePointDistance(XMLoadFloat3(&pickRay.origin), XMLoadFloat3(&pickRay.origin) + XMLoadFloat3(&pickRay.direction), transform.GetPositionV());
        float dis = XMVectorGetX(disV);
        if (dis > 0.01f && dis < wi::math::Distance(transform.GetPosition(), pickRay.origin) * 0.05f && dis < hovered.distance)
        {
            hovered = wi::scene::PickResult();
            hovered.entity = gizmo.entity;
            hovered.distance = dis;
        }
    }

    Editor::GetData()->hovered = hovered;
}

int Editor_PickEntity(lua_State* L)
{
    XMFLOAT4 currentMouse = wi::input::GetPointer();
    wi::primitive::Ray pickRay = wi::renderer::GetPickRay((long)currentMouse.x, (long)currentMouse.y, *Editor::GetData()->viewport, Game::Resources::GetScene().wiscene.camera);

    auto picked = Editor::GetData()->hovered;
    if (picked.entity == wi::ecs::INVALID_ENTITY)
    {
        picked = wi::scene::Pick(pickRay, ~0u, ~0u, Game::Resources::GetScene().wiscene);
    }

    Editor::GetData()->selection = picked;
    Editor_UpdateSelection();

    wi::lua::SSetLongLong(L, picked.entity);
    return 1;
}

int Editor_SetTranslatorMode(lua_State* L)
{
    auto argc = wi::lua::SGetArgCount(L);
    if (argc > 0)
    {
        bool s_pos, s_rot, s_sca;
        int set = wi::lua::SGetInt(L, 1);
        switch (set)
        {
            case 1:
            {
                s_pos = true;
                s_rot = false;
                s_sca = false;
                break;
            }
            case 2:
            {
                s_pos = false;
                s_rot = true;
                s_sca = false;
                break;
            }
            case 3:
            {
                s_pos = false;
                s_rot = false;
                s_sca = true;
                break;
            }
            default:
                break;
        }

        Editor::GetData()->transform_translator.isTranslator = s_pos;
        Editor::GetData()->transform_translator.isRotator = s_rot;
        Editor::GetData()->transform_translator.isScalator = s_sca;
    }
    return 0;
}

int Editor_GetTranslatorMode(lua_State* L)
{
    // Just pack it like this, it's going to be alright (?)
    int option = (Editor::GetData()->transform_translator.isTranslator*1);
    option += (Editor::GetData()->transform_translator.isRotator*2);
    option += (Editor::GetData()->transform_translator.isScalator*3);
    wi::lua::SSetInt(L, option);
    return 1;
}

int Editor_StashDeletedEntity(lua_State* L)
{
    auto argc = wi::lua::SGetArgCount(L);
    if(argc >= 2)
    {
        wi::ecs::Entity target_entity = wi::lua::SGetLongLong(L, 1);
        uint32_t target_store = wi::lua::SGetLongLong(L, 2);

        auto& remap = Editor::GetData()->clips_deleted[target_store].remap;
        auto& archive = Editor::GetData()->clips_deleted[target_store].archive;

        archive.SetReadModeAndResetPos(false);

        wi::ecs::EntitySerializer seri;
        seri.allow_remap = false;
        wi::scene::Scene::EntitySerializeFlags flags = wi::scene::Scene::EntitySerializeFlags::RECURSIVE;
        flags |= wi::scene::Scene::EntitySerializeFlags::KEEP_INTERNAL_ENTITY_REFERENCES;
        Game::Resources::GetScene().wiscene.Entity_Serialize(archive, seri, target_entity, flags);

        remap = seri.remap;
    }
    else
    {
        wi::lua::SError(L,"Editor_StashDeletedEntity(Entity entity, int history_index) can't have zero arguments!");
    }
    return 0;
}

int Editor_WipeDeletedEntityList(lua_State* L)
{
    Editor::GetData()->clips_deleted.clear();
    return 0;
}

int Editor_RestoreDeletedEntity(lua_State* L)
{
    auto argc = wi::lua::SGetArgCount(L);
    if(argc > 0)
    {
        uint32_t target_store = wi::lua::SGetLongLong(L, 1);
        auto& remap = Editor::GetData()->clips_deleted[target_store].remap;
        auto& archive = Editor::GetData()->clips_deleted[target_store].archive;

        archive.SetReadModeAndResetPos(true);

        wi::ecs::EntitySerializer seri;
        seri.allow_remap = false;
        seri.remap = remap;

        wi::scene::Scene::EntitySerializeFlags flags = wi::scene::Scene::EntitySerializeFlags::RECURSIVE;
        flags |= wi::scene::Scene::EntitySerializeFlags::KEEP_INTERNAL_ENTITY_REFERENCES;
        Game::Resources::GetScene().wiscene.Entity_Serialize(archive, seri, wi::ecs::INVALID_ENTITY, flags);
    }
    else
    {
        wi::lua::SError(L,"Editor_RestoreDeletedEntity(Entity entity) can't have zero arguments!");
    }
    return 0;
}

int Editor_DeletedEntityDrop(lua_State* L)
{
    auto argc = wi::lua::SGetArgCount(L);
    if(argc > 0)
    {
        uint32_t target_head = wi::lua::SGetLongLong(L, 1);
        auto index_exist = Editor::GetData()->clips_deleted.find(target_head);
        if (index_exist != Editor::GetData()->clips_deleted.end())
        {
            Editor::GetData()->clips_deleted.erase(target_head);
        }
    }
    else
    {
        wi::lua::SError(L,"Editor_RestoreDeletedEntity(int target_head) can't have zero arguments!");
    }
    return 0;
}

int Editor_ImportGLTF(lua_State* L)
{
    auto argc = wi::lua::SGetArgCount(L);
    if(argc > 0)
    {
        std::string filepath = wi::lua::SGetString(L, 1);
        Editor::IO::ImportModel_GLTF(filepath, Game::Resources::GetScene());
    }
    else
    {
        wi::lua::SError(L,"Editor_ImportGLTF(string filepath) not neight arguments");
    }
    return 0;
}

int Editor_LoadWiScene(lua_State* L)
{
    auto& scene = Game::Resources::GetScene();
    auto& wiscene = scene.wiscene;
    auto argc = wi::lua::SGetArgCount(L);
    if(argc >= 2)
    {
        std::string file = wi::lua::SGetString(L, 1);
        bool as_instance = wi::lua::SGetBool(L, 2);
        Game::Resources::Scene dscene;
        wi::scene::LoadModel(dscene.wiscene, file);
        if(as_instance)
        {
            auto newfile = Game::Resources::SourcePath::SCENE+"/"+wi::helper::GetFileNameFromPath(file)+Game::Resources::DataType::SCENE_DATA;
            auto saveto = wi::Archive(newfile,false);
            dscene.wiscene.Serialize(saveto);
            wi::ecs::Entity instance_entity = scene.CreateInstance(wi::helper::GetFileNameFromPath(file));
            auto instanceComponent = scene.instances.GetComponent(instance_entity);
            if(instanceComponent != nullptr)
            {
                instanceComponent->file = newfile;
            }
        }
        else
        {
            scene.wiscene.Merge(dscene.wiscene);
        }
    }
    return 0;
}

int Editor_SaveScene(lua_State* L)
{
    auto argc = wi::lua::SGetArgCount(L);
    if(argc > 0)
    {
        auto filepath = wi::lua::SGetString(L, 1);
        auto& scene = Game::Resources::GetScene();

        // Step 1: Check if there are unsaved instances and then save them!
        for(auto& instance_entity : Editor::GetData()->unsaved_instances)
        {
            auto instance = scene.instances.GetComponent(instance_entity);
            // TODO
        }

        wi::vector<wi::ecs::Entity> instance_library_removelist;

        // Step 2: Unload all instances
        for(int i = 0; i < scene.instances.GetCount(); ++i)
        {
            scene.instances[i].Unload();
            auto instance_entity = scene.instances.GetEntity(i);
            if(scene.wiscene.names.Contains(instance_entity))
            {
                auto nameComponent = scene.wiscene.names.GetComponent(instance_entity);
                if(nameComponent->name.substr(0,4) == "LIB_") instance_library_removelist.push_back(instance_entity);
            }
        }

        for(auto& entity : instance_library_removelist)
        {
            scene.wiscene.Entity_Remove(entity);
        }
        
        // Step 4: Save the scene
        auto archive = wi::Archive(filepath,false);
        scene.wiscene.Serialize(archive);

        // Step 5: Restore the scene instances
        for(int i = 0; i < scene.instances.GetCount(); ++i)
        {
            if(!scene.streams.Contains(scene.instances.GetEntity(i))) { scene.instances[i].Init(); }
        }
    }
    else 
    {
        wi::lua::SError(L, "Editor_SaveScene(string filepath) not enough arguments!");
    }
    return 0;
}

int Editor_LoadScene(lua_State* L)
{
    auto argc = wi::lua::SGetArgCount(L);
    if(argc > 0)
    {
        auto filepath = wi::lua::SGetString(L, 1);
        auto& scene = Game::Resources::GetScene();
        scene.Clear();
        wi::scene::LoadModel(scene.wiscene, filepath);
    }
    else 
    {
        wi::lua::SError(L, "Editor_SaveScene(string filepath) not enough arguments!");
    }
    return 0;
}

int Editor_ImguiImage(lua_State* L)
{
    auto argc = wi::lua::SGetArgCount(L);
    if(argc >= 3)
    {
        std::string imagefile = wi::lua::SGetString(L, 1);
        float width = wi::lua::SGetFloat(L, 2);
        float height = wi::lua::SGetFloat(L, 3);

        // Allocate resource first if it does not exist
        auto find_image = Editor::GetData()->resourcemap.find(imagefile);
        if(find_image == Editor::GetData()->resourcemap.end())
        {
            Editor::GetData()->resourcemap[imagefile] = wi::resourcemanager::Load(imagefile);
        }

        auto& resource = Editor::GetData()->resourcemap[imagefile];
        if (resource.IsValid() && resource.GetTexture().IsValid())
        {
            wi::graphics::Texture* image = (wi::graphics::Texture*)&resource.GetTexture();
            float iwidth = image->desc.width;
            float iheight = image->desc.height;

            float ratio = iwidth / iheight;
            float hratio = iheight / iwidth;
            float pratio = width / height;

            float image_width = (ratio < pratio)? height * ratio : width;
            float image_height = (ratio < pratio)? height : width * hratio;

            ImGui::Image(image, ImVec2(image_width, image_height));
        }
    }
    return 0;
}

int Editor_ImguiImageButton(lua_State* L)
{
    auto argc = wi::lua::SGetArgCount(L);
    if(argc >= 3)
    {
        std::string imagefile = wi::lua::SGetString(L, 1);
        float width = wi::lua::SGetFloat(L, 2);
        float height = wi::lua::SGetFloat(L, 3);

        // Allocate resource first if it does not exist
        auto find_image = Editor::GetData()->resourcemap.find(imagefile);
        if(find_image == Editor::GetData()->resourcemap.end())
        {
            Editor::GetData()->resourcemap[imagefile] = wi::resourcemanager::Load(imagefile);
        }

        auto& resource = Editor::GetData()->resourcemap[imagefile];
        if (resource.IsValid() && resource.GetTexture().IsValid())
        {
            wi::graphics::Texture* image = (wi::graphics::Texture*)&resource.GetTexture();
            float iwidth = image->desc.width;
            float iheight = image->desc.height;

            float ratio = iwidth / iheight;
            float hratio = iheight / iwidth;
            float pratio = width / height;

            float image_width = (ratio < pratio)? height * ratio : width;
            float image_height = (ratio < pratio)? height : width * hratio;

            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(3, 3));
            auto result = ImGui::ImageButton(image, ImVec2(image_width, image_height));
            ImGui::PopStyleVar();

            wi::lua::SSetBool(L, result);

            return 1;
        }
    }
    return 0;
}

int Editor_ListDirectory(lua_State* L)
{
    wi::unordered_map<std::string, wi::vector<std::string>> dir_files;
    
    auto scene_root = std::filesystem::path(wi::helper::GetCurrentPath() + "/Data/Scene");
    for (auto& filenode : std::filesystem::recursive_directory_iterator(scene_root,std::filesystem::directory_options::follow_directory_symlink))
    {
        std::string path = filenode.path();
        wi::helper::MakePathRelative(wi::helper::GetCurrentPath() + "/Data", path);
        std::string file, dir;
        wi::helper::SplitPath(path, dir, file);
        dir_files[dir].push_back(file);
    }
    auto texture_root = std::filesystem::path(wi::helper::GetCurrentPath() + "/Data/Texture");
    for (auto& filenode : std::filesystem::recursive_directory_iterator(texture_root,std::filesystem::directory_options::follow_directory_symlink))
    {
        std::string path = filenode.path();
        wi::helper::MakePathRelative(wi::helper::GetCurrentPath() + "/Data", path);
        std::string file, dir;
        wi::helper::SplitPath(path, dir, file);
        dir_files[dir].push_back(file);
    }
    auto sound_root = std::filesystem::path(wi::helper::GetCurrentPath() + "/Data/Sound");
    for (auto& filenode : std::filesystem::recursive_directory_iterator(sound_root,std::filesystem::directory_options::follow_directory_symlink))
    {
        std::string path = filenode.path();
        wi::helper::MakePathRelative(wi::helper::GetCurrentPath() + "/Data", path);
        std::string file, dir;
        wi::helper::SplitPath(path, dir, file);
        dir_files[dir].push_back(file);
    }

    lua_newtable(L);

    for(auto& dir : dir_files)
    {
        lua_newtable(L);
        for(int i = 0; i < dir.second.size(); ++i)
        {
            lua_pushstring(L, dir.second[i].c_str());
            lua_rawseti(L,-2,i+1);
        }
        lua_setfield(L, -2, dir.first.c_str());
    }

    return 1;
}

int Editor_RenderScenePreview(lua_State* L)
{
    auto argc = wi::lua::SGetArgCount(L);
    if(argc > 0)
    {
        auto resourcename = wi::lua::SGetString(L, 1);

        auto& renderer = Editor::GetData()->preview_render;
        auto& camera = Editor::GetData()->preview_camera;

        auto& gamescene = Game::Resources::GetScene();
        renderer.scene = &gamescene.wiscene;

        wi::vector<uint32_t> original_layers;

        wi::primitive::AABB bounds;
        wi::ecs::Entity entity;
        if(argc >= 2)
        {
            entity = (wi::ecs::Entity)wi::lua::SGetLongLong(L, 2);
        }
        for(int i = 0; i<renderer.scene->layers.GetCount(); ++i)
        {
            if(entity != wi::ecs::INVALID_ENTITY)
            {
                original_layers.push_back(renderer.scene->layers[i].layerMask);
                renderer.scene->layers[i].layerMask = 0;
            }

            auto layer_entity = renderer.scene->layers.GetEntity(i);
            if(renderer.scene->objects.Contains(layer_entity))
            { 
                auto object = renderer.scene->objects.GetComponent(layer_entity);
                bounds = wi::primitive::AABB::Merge(
                    bounds, 
                    wi::primitive::AABB(
                        XMFLOAT3(object->center.x - object->radius, object->center.y - object->radius, object->center.z - object->radius),
                        XMFLOAT3(object->center.x + object->radius, object->center.y + object->radius, object->center.z + object->radius)
                    ));
            }
        }
        auto instanceComponent = Game::Resources::GetScene().instances.GetComponent(entity);
        if(instanceComponent != nullptr)
        {
            for(auto& entity : instanceComponent->entities)
            {
                auto layerMask = renderer.scene->layers.GetComponent(entity);
                layerMask->layerMask = 1;
            }
        }
    
        auto distance = std::min(bounds.getRadius()*2.3f,10000000.f);
        wi::backlog::post(std::to_string(distance));

        wi::scene::TransformComponent transform, transform_origin;

        auto angular = 30.f / 180.f * XM_PI;
        transform_origin.RotateRollPitchYaw(XMFLOAT3(angular, angular, 0)); 

        transform.Translate(XMFLOAT3(0,0,-distance));
        transform_origin.Translate(bounds.getCenter());
        transform_origin.UpdateTransform();
        transform.UpdateTransform_Parented(transform_origin);

        camera.SetDirty();
        camera.TransformCamera(transform);
        
        for(int count = 0; count < 3; ++count)
        {
            bool is_cull_enabled = wi::renderer::GetOcclusionCullingEnabled();
            bool is_grid_set = wi::renderer::GetToDrawGridHelper();
            wi::renderer::SetToDrawGridHelper(false);
            renderer.Update(0.16f);
            renderer.PostUpdate();
            renderer.Render();
            wi::renderer::SetToDrawGridHelper(is_grid_set);
            wi::renderer::SetOcclusionCullingEnabled(is_cull_enabled);
        }

        Editor::GetData()->resourcemap[resourcename].SetTexture(*renderer.GetLastPostprocessRT());

        if(original_layers.size() > 0)
        {
            for(int i = 0; i<renderer.scene->layers.GetCount(); ++i)
            {
                renderer.scene->layers[i].layerMask = original_layers[i];
            }
        }
    }
    else
    {
        wi::lua::SError(L, "Editor_RenderScenePreview(string imagename)");
    }
    return 0;
}

int Editor_RenderMaterialPreview(lua_State* L)
{
    auto argc = wi::lua::SGetArgCount(L);
    if(argc >= 2)
    {
        auto resourcename = wi::lua::SGetString(L, 1);
        wi::ecs::Entity entity = (wi::ecs::Entity)wi::lua::SGetLongLong(L, 2);

        auto& renderer = Editor::GetData()->preview_render;
        auto& camera = Editor::GetData()->preview_camera;

        auto& gamescene = Editor::GetData()->preview_scene;
        renderer.scene = &gamescene.wiscene;

        wi::primitive::AABB bounds;
        for(int i = 0; i<renderer.scene->layers.GetCount(); ++i)
        {
            auto layer_entity = renderer.scene->layers.GetEntity(i);
            if(renderer.scene->objects.Contains(layer_entity)){ 
                auto object = renderer.scene->objects.GetComponent(layer_entity);
                bounds = wi::primitive::AABB::Merge(
                    bounds, 
                    wi::primitive::AABB(
                        XMFLOAT3(object->center.x - object->radius, object->center.y - object->radius, object->center.z - object->radius),
                        XMFLOAT3(object->center.x + object->radius, object->center.y + object->radius, object->center.z + object->radius)
                    ));
                bounds = wi::primitive::AABB::Merge(bounds, wi::primitive::AABB()); 
            }
        }
    
        auto distance = std::min(bounds.getRadius()*2.3f,10000000.f);
        wi::backlog::post(std::to_string(distance));

        wi::scene::TransformComponent transform;

        transform.Translate(XMFLOAT3(0,0,-distance));
        transform.UpdateTransform();

        camera.SetDirty();
        camera.TransformCamera(transform);
        
        for(int count = 0; count < 3; ++count)
        {
            bool is_cull_enabled = wi::renderer::GetOcclusionCullingEnabled();
            bool is_grid_set = wi::renderer::GetToDrawGridHelper();
            wi::renderer::SetToDrawGridHelper(false);
            renderer.Update(0.16f);
            renderer.PostUpdate();
            renderer.Render();
            wi::renderer::SetToDrawGridHelper(is_grid_set);
            wi::renderer::SetOcclusionCullingEnabled(is_cull_enabled);
        }

        Editor::GetData()->resourcemap[resourcename].SetTexture(*renderer.GetLastPostprocessRT());
    }
    else
    {
        wi::lua::SError(L, "Editor_RenderMaterialPreview(string imagename, Entity material_entity)");
    }
    return 0;
}

int Editor_SaveImage(lua_State* L)
{
    auto argc = wi::lua::SGetArgCount(L);
    if (argc > 0)
    {
        auto resource_name = wi::lua::SGetString(L, 1);
        auto save_alias = resource_name;
        if(argc >= 2)
        {
            save_alias = wi::lua::SGetString(L, 2);
        }
        auto find_image = Editor::GetData()->resourcemap.find(resource_name);
        if(find_image != Editor::GetData()->resourcemap.end())
        {
            auto& resource = find_image->second;
            if(resource.IsValid() && resource.GetTexture().IsTexture())
            {
                auto& texture = resource.GetTexture();
                wi::helper::saveTextureToFile(texture, save_alias);
            }
        }
    }
    else
    {
        wi::lua::SError(L, "Editor_SaveImage(string resource_name)");
    }
    return 0;
}

int Editor_ExtractSubInstanceNames(lua_State* L)
{
    auto argc = wi::lua::SGetArgCount(L);
    if(argc > 0)
    {
        auto filepath = wi::lua::SGetString(L, 1);

        wi::ecs::Entity instance_entity = wi::ecs::INVALID_ENTITY;
        if(argc >= 2)
        {
            instance_entity = (wi::ecs::Entity)wi::lua::SGetInt(L, 2);
        }

        wi::unordered_set<std::string> names;

        auto& scene = Game::Resources::GetScene();

        auto instanceComponent = scene.instances.GetComponent(instance_entity);
        if(instanceComponent != nullptr)
        {
            for(auto& entity : instanceComponent->entities)
            {
                auto nameComponent = scene.wiscene.names.GetComponent(entity);
                names.insert(nameComponent->name);
            }
        }
        else
        {
            for(int i = 0; i < scene.wiscene.names.GetCount(); ++i)
            {
                names.insert(scene.wiscene.names[i].name);
            }
        }

        wi::vector<std::string> names_export;
        names_export.insert(names_export.begin(), names.begin(), names.end());

        auto save = wi::Archive(filepath, false);
        save << names_export;
    }
    else
    {
        wi::lua::SError(L, "Editor_ExtractSubInstanceNames(string filepath) not enough arguments!");
    }

    return 0;
}

int Editor_FetchSubInstanceNames(lua_State* L)
{
    auto argc = wi::lua::SGetArgCount(L);
    if(argc > 0)
    {
        auto filepath = wi::lua::SGetString(L, 1);

        wi::vector<std::string> names_import;
        
        auto load = wi::Archive(filepath, true);
        load >> names_import;

        lua_newtable(L);
        for(int i = 0; i < names_import.size(); ++i)
        {
            wi::lua::SSetString(L, names_import[i]);
            lua_rawseti(L, -2, i);
        }

        return 1;
    }
    else
    {
        wi::lua::SError(L, "Editor_FetchSubInstanceNames(string filepath) not enough arguments!");
    }
    return 0;
}

void Editor::Init()
{
    wi::lua::RunText("EditorAPI = true");

    wi::lua::RegisterFunc("Editor_SetGridHelper", Editor_SetGridHelper);
    wi::lua::RegisterFunc("Editor_UIFocused", Editor_UIFocused);

    wi::lua::RegisterFunc("Editor_IsEntityListUpdated", Editor_IsEntityListUpdated);
    wi::lua::RegisterFunc("Editor_GetObjectList", Editor_GetObjectList);
    wi::lua::RegisterFunc("Editor_FetchSelection", Editor_FetchSelection);

    wi::lua::RegisterFunc("Editor_PickEntity", Editor_PickEntity);

    wi::lua::RegisterFunc("Editor_SetTranslatorMode", Editor_SetTranslatorMode);
    wi::lua::RegisterFunc("Editor_GetTranslatorMode", Editor_GetTranslatorMode);

    wi::lua::RegisterFunc("Editor_StashDeletedEntity", Editor_StashDeletedEntity);
    wi::lua::RegisterFunc("Editor_RestoreDeletedEntity", Editor_RestoreDeletedEntity);
    wi::lua::RegisterFunc("Editor_DeletedEntityDrop", Editor_DeletedEntityDrop);
    wi::lua::RegisterFunc("Editor_WipeDeletedEntityList", Editor_WipeDeletedEntityList);

    wi::lua::RegisterFunc("Editor_UpdateGizmoData", Editor_UpdateGizmoData);

    wi::lua::RegisterFunc("Editor_ImportGLTF", Editor_ImportGLTF);

    wi::lua::RegisterFunc("Editor_LoadWiScene", Editor_LoadWiScene);
    wi::lua::RegisterFunc("Editor_LoadScene", Editor_LoadScene);
    wi::lua::RegisterFunc("Editor_SaveScene", Editor_SaveScene);

    wi::lua::RegisterFunc("Editor_ImguiImage", Editor_ImguiImage);
    wi::lua::RegisterFunc("Editor_ImguiImageButton", Editor_ImguiImageButton);

    wi::lua::RegisterFunc("Editor_ListDirectory", Editor_ListDirectory);
    wi::lua::RegisterFunc("Editor_RenderScenePreview", Editor_RenderScenePreview);
    wi::lua::RegisterFunc("Editor_RenderMaterialPreview", Editor_RenderMaterialPreview);
    wi::lua::RegisterFunc("Editor_SaveImage", Editor_SaveImage);

    wi::lua::RegisterFunc("Editor_ExtractSubInstanceNames", Editor_ExtractSubInstanceNames);
    wi::lua::RegisterFunc("Editor_FetchSubInstanceNames", Editor_FetchSubInstanceNames);

    Editor::GetData()->transform_translator.SetEnabled(true);
    Editor::GetData()->transform_translator.scene = &Game::Resources::GetScene().wiscene;

    wi::font::AddFontStyle("FontAwesomeV6", font_awesome_v6, sizeof(font_awesome_v6));

    Editor::GetData()->preview_render.scene = &Editor::GetData()->preview_scene.wiscene;
    Editor::GetData()->preview_render.camera = &Editor::GetData()->preview_camera;
    // Editor::GetData()->preview_render.setSceneUpdateEnabled(false);
    Editor::GetData()->preview_render.init(512,512);
}

void Editor::Update(float dt, wi::RenderPath2D& viewport)
{
    GetData()->transform_translator.Update(Game::Resources::GetScene().wiscene.camera, viewport);
    if(GetData()->transform_translator.IsDragStarted())
    {
        auto& translator = GetData()->transform_translator;
        if(translator.selected.size() > 0)
        {
            auto transformComponent = Game::Resources::GetScene().wiscene.transforms.GetComponent(translator.selected[0].entity);
            Editor::GetData()->transform_start = *transformComponent;
        }
    }
    if(GetData()->transform_translator.IsDragEnded())
    {
        auto& translator = GetData()->transform_translator;

        if(translator.selected.size() > 0)
        {
            auto L = wi::lua::GetLuaState();
            lua_getglobal(L, "editor_hook_execcmd");

            wi::lua::SSetString(L, "mod_comp");
            
            lua_newtable(L);
            wi::lua::SSetLongLong(L, translator.selected[0].entity);
            lua_setfield(L, -2, "entity");
            wi::lua::SSetString(L, "transform");
            lua_setfield(L, -2, "type");
            lua_newtable(L);

            Luna<wi::lua::Vector_BindLua>::push(L, new wi::lua::Vector_BindLua(XMLoadFloat3(&Editor::GetData()->transform_start.translation_local)));
            lua_setfield(L, -2, "Translation_local");
            Luna<wi::lua::Vector_BindLua>::push(L, new wi::lua::Vector_BindLua(XMLoadFloat4(&Editor::GetData()->transform_start.rotation_local)));
            lua_setfield(L, -2, "Rotation_local");
            Luna<wi::lua::Vector_BindLua>::push(L, new wi::lua::Vector_BindLua(XMLoadFloat3(&Editor::GetData()->transform_start.scale_local)));
            lua_setfield(L, -2, "Scale_local");
            lua_setfield(L, -2, "pre");
            lua_newtable(L);
            auto transformComponent = Game::Resources::GetScene().wiscene.transforms.GetComponent(translator.selected[0].entity);
            Luna<wi::lua::Vector_BindLua>::push(L, new wi::lua::Vector_BindLua(XMLoadFloat3(&transformComponent->translation_local)));
            lua_setfield(L, -2, "Translation_local");
            Luna<wi::lua::Vector_BindLua>::push(L, new wi::lua::Vector_BindLua(XMLoadFloat4(&transformComponent->rotation_local)));
            lua_setfield(L, -2, "Rotation_local");
            Luna<wi::lua::Vector_BindLua>::push(L, new wi::lua::Vector_BindLua(XMLoadFloat3(&transformComponent->scale_local)));
            lua_setfield(L, -2, "Scale_local");
            lua_setfield(L, -2, "post");

            wi::lua::SSetBool(L,false);
            
            lua_call(L, 3, 0);
        }
    }
    Editor_ProcessHovered();
}

void Editor::ResizeBuffers(wi::graphics::GraphicsDevice* device, wi::RenderPath3D* renderPath)
{
    if(renderPath->GetDepthStencil() != nullptr)
	{
		bool success = false;

		XMUINT2 internalResolution = renderPath->GetInternalResolution();

		wi::graphics::TextureDesc desc;
		desc.width = internalResolution.x;
		desc.height = internalResolution.y;

		desc.format = wi::graphics::Format::R8_UNORM;
		desc.bind_flags = wi::graphics::BindFlag::RENDER_TARGET | wi::graphics::BindFlag::SHADER_RESOURCE;
		success = device->CreateTexture(&desc, nullptr, &GetData()->rt_selection_editor);
		assert(success);
		{
			wi::graphics::RenderPassDesc desc;
			desc.attachments.push_back(wi::graphics::RenderPassAttachment::RenderTarget(GetData()->rt_selection_editor, wi::graphics::RenderPassAttachment::LoadOp::CLEAR));
			desc.attachments.push_back(
				wi::graphics::RenderPassAttachment::DepthStencil(
					*renderPath->GetDepthStencil(),
					wi::graphics::RenderPassAttachment::LoadOp::LOAD,
					wi::graphics::RenderPassAttachment::StoreOp::STORE,
					wi::graphics::ResourceState::DEPTHSTENCIL_READONLY,
					wi::graphics::ResourceState::DEPTHSTENCIL_READONLY,
					wi::graphics::ResourceState::DEPTHSTENCIL_READONLY
				)
			);
			success = device->CreateRenderPass(&desc, &GetData()->renderpass_selection_editor);
			assert(success);
		}
	}

    {
        wi::graphics::TextureDesc desc;
        desc.width = renderPath->GetRenderResult().GetDesc().width;
        desc.height = renderPath->GetRenderResult().GetDesc().height;
        desc.format = wi::graphics::Format::D32_FLOAT;
        desc.bind_flags = wi::graphics::BindFlag::DEPTH_STENCIL;
        desc.layout = wi::graphics::ResourceState::DEPTHSTENCIL;
        desc.misc_flags = wi::graphics::ResourceMiscFlag::TRANSIENT_ATTACHMENT;
        device->CreateTexture(&desc, nullptr, &GetData()->rt_depthbuffer_editor);
        device->SetName(&GetData()->rt_depthbuffer_editor, "rt_depthbuffer_editor");

        {
            wi::graphics::RenderPassDesc desc;
            desc.attachments.push_back(
                wi::graphics::RenderPassAttachment::DepthStencil(
                    GetData()->rt_depthbuffer_editor,
                    wi::graphics::RenderPassAttachment::LoadOp::CLEAR,
                    wi::graphics::RenderPassAttachment::StoreOp::DONTCARE
                )
            );
            desc.attachments.push_back(
                wi::graphics::RenderPassAttachment::RenderTarget(
                    renderPath->GetRenderResult(),
                    wi::graphics::RenderPassAttachment::LoadOp::CLEAR,
                    wi::graphics::RenderPassAttachment::StoreOp::STORE
                )
            );
            device->CreateRenderPass(&desc, &GetData()->renderpass_editor);
        }
    }
}

void Editor::Render(wi::graphics::GraphicsDevice* device, wi::graphics::CommandList& cmd)
{
    auto& scene = Game::Resources::GetScene();
    auto& wiscene = scene.wiscene;

    device->EventBegin("Editor", cmd);

    //Editor's rendarpass
    device->RenderPassBegin(&GetData()->renderpass_editor,cmd);

    wi::graphics::Viewport vp;
    vp.width = (float)GetData()->rt_depthbuffer_editor.GetDesc().width;
    vp.height = (float)GetData()->rt_depthbuffer_editor.GetDesc().height;
    device->BindViewports(1, &vp, cmd);

    //Translator
    {
        device->EventBegin("Editor - Translator", cmd);
        GetData()->transform_translator.Draw(Game::Resources::GetScene().wiscene.camera, cmd);
        device->EventEnd(cmd);
    }

    //Gizmos
    {
        // remove camera jittering
        wi::scene::CameraComponent cam = Game::Resources::GetScene().wiscene.camera;
        cam.jitter = XMFLOAT2(0, 0);
        cam.UpdateCamera();
        const XMMATRIX VP = cam.GetViewProjection();

        const XMMATRIX R = XMLoadFloat3x3(&cam.rotationMatrix);

        wi::font::Params fp;
        fp.customRotation = &R;
        fp.customProjection = &VP;
        fp.size = 32; // icon font render quality
        const float scaling = 0.0025f;
        fp.h_align = wi::font::WIFALIGN_CENTER;
        fp.v_align = wi::font::WIFALIGN_CENTER;
        fp.shadowColor = wi::Color::Shadow();
        fp.shadow_softness = 1;

        for(auto& gizmo : GetData()->gizmo_data)
        {
            if (!wiscene.transforms.Contains(gizmo.entity))
                continue;
            const wi::scene::TransformComponent& transform = *wiscene.transforms.GetComponent(gizmo.entity);

            fp.position = transform.GetPosition();
            fp.scaling = scaling * wi::math::Distance(transform.GetPosition(), cam.Eye);
            fp.color = GetData()->inactiveEntityColor;

            if(GetData()->hovered.entity == gizmo.entity)
            {
                fp.color = GetData()->hoveredEntityColor;
            }

            if(GetData()->selection.entity == gizmo.entity)
            {
                fp.color = GetData()->selectedEntityColor;
            }

            wi::font::Draw(gizmo.icon, fp, cmd);
        }
    }
    
    device->RenderPassEnd(cmd);

    //Editor's selection
    {
        device->EventBegin("Editor - Selection", cmd);

		wi::graphics::Viewport vp_sel;
		vp_sel.width = (float)GetData()->rt_selection_editor.GetDesc().width;
		vp_sel.height = (float)GetData()->rt_selection_editor.GetDesc().height;
		device->BindViewports(1, &vp, cmd);

		wi::image::Params fx;
		fx.enableFullScreen();
		fx.stencilComp = wi::image::STENCILMODE::STENCILMODE_EQUAL;

		// We will specify the stencil ref in user-space, don't care about engine stencil refs here:
		//	Otherwise would need to take into account engine ref and draw multiple permutations of stencil refs.
		fx.stencilRefMode = wi::image::STENCILREFMODE_USER;

		// Objects outline:
		{
			device->RenderPassBegin(&GetData()->renderpass_selection_editor, cmd);

			// Draw solid blocks of selected objects
			fx.stencilRef = EDITOR_STENCILREF_LAST;
			wi::image::Draw(wi::texturehelper::getWhite(), fx, cmd);

			device->RenderPassEnd(cmd);
		}
    }

    device->EventEnd(cmd);
}

void Editor::Compose(wi::graphics::GraphicsDevice *device, wi::graphics::CommandList &cmd)
{
    device->EventBegin("Editor - Selection", cmd);
    wi::renderer::BindCommonResources(cmd);
    XMFLOAT4 col = GetData()->editor_selection_color;
    wi::renderer::Postprocess_Outline(GetData()->rt_selection_editor, cmd, 0.1f, 1, col);
    device->EventEnd(cmd);
}