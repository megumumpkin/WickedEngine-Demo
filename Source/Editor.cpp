#include "Editor.h"

Editor::Data* Editor::GetData(){
    static Data data;
    return &data;
}

int Editor_GetObjectList(lua_State* L)
{
    auto& wiscene = Game::Resources::GetScene().wiscene;
    auto& entity_list = wiscene.transforms.GetEntityArray();
    wi::unordered_map<wi::ecs::Entity, wi::vector<wi::ecs::Entity>> reduced_hierarchy_group;
    auto jobCount = (uint32_t)std::ceil(entity_list.size()/128.f);
    
    std::mutex enlistThreadsMutex;
    wi::jobsystem::context enlistThreadGroup;

    wi::vector<wi::unordered_map<wi::ecs::Entity, wi::vector<wi::ecs::Entity>>> thread_hierarchy_group;
    thread_hierarchy_group.resize(jobCount);

    wi::jobsystem::Dispatch(enlistThreadGroup, jobCount, 1, [&](wi::jobsystem::JobArgs args)
    {
        std::scoped_lock lock (enlistThreadsMutex);

        auto& map_hierarchy_group = thread_hierarchy_group[args.jobIndex];

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
    if(Editor::GetData()->selection.entity != wi::ecs::INVALID_ENTITY)
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

int Editor_PickEntity(lua_State* L)
{

    XMFLOAT4 currentMouse = wi::input::GetPointer();
    wi::primitive::Ray pickRay = wi::renderer::GetPickRay((long)currentMouse.x, (long)currentMouse.y, *Editor::GetData()->viewport, Game::Resources::GetScene().wiscene.camera);
    
    auto hovered = wi::scene::PickResult();
    
    if (hovered.entity == wi::ecs::INVALID_ENTITY)
    {
        hovered = wi::scene::Pick(pickRay, ~0u, ~0u, Game::Resources::GetScene().wiscene);
    }

    Editor::GetData()->selection = hovered;
    Editor_UpdateSelection();

    wi::lua::SSetLongLong(L, hovered.entity);
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

void Editor::Init()
{
    wi::lua::RunText("EditorAPI = true");
    wi::lua::RegisterFunc("Editor_GetObjectList", Editor_GetObjectList);
    wi::lua::RegisterFunc("Editor_FetchSelection", Editor_FetchSelection);
    wi::lua::RegisterFunc("Editor_PickEntity", Editor_PickEntity);
    wi::lua::RegisterFunc("Editor_SetTranslatorMode", Editor_SetTranslatorMode);
    wi::lua::RegisterFunc("Editor_GetTranslatorMode", Editor_GetTranslatorMode);

    Editor::GetData()->transform_translator.SetEnabled(true);
    Editor::GetData()->transform_translator.scene = &Game::Resources::GetScene().wiscene;
}

void Editor::Update(float dt, wi::RenderPath2D& viewport)
{
    GetData()->transform_translator.Update(Game::Resources::GetScene().wiscene.camera, viewport);
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
			desc.attachments.push_back(wi::graphics::RenderPassAttachment::RenderTarget(&GetData()->rt_selection_editor, wi::graphics::RenderPassAttachment::LoadOp::CLEAR));
			desc.attachments.push_back(
				wi::graphics::RenderPassAttachment::DepthStencil(
					renderPath->GetDepthStencil(),
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
                    &GetData()->rt_depthbuffer_editor,
                    wi::graphics::RenderPassAttachment::LoadOp::CLEAR,
                    wi::graphics::RenderPassAttachment::StoreOp::DONTCARE
                )
            );
            desc.attachments.push_back(
                wi::graphics::RenderPassAttachment::RenderTarget(
                    &renderPath->GetRenderResult(),
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