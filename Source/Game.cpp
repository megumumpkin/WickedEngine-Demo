#include "Game.h"
#include "LiveUpdate.h"
#include "Resources.h"
#include "RenderPipeline.h"
#include "BindLua.h"
#include <memory>
#include <wiScene.h>

#ifdef IS_DEV
#include "Editor.h"

#include "ImGui/imgui.h"
#include "ImGui/imgui_internal.h"
#include "ImGui/imgui_BindLua.h"
#include "ImGui/FA6_UI_Icons.h"
#include "ImGui/Widgets/ImGuizmo.h"
#endif

#ifdef _WIN32
#include "ImGui/imgui_impl_win32.h"
#elif defined(SDL2)
#include "ImGui/imgui_impl_sdl.h"
#endif

#include <fstream>
#include <thread>

using namespace wi::ecs;
using namespace wi::scene;
using namespace wi::graphics;
using namespace Game;

wi::graphics::Shader imguiVS;
wi::graphics::Shader imguiPS;
wi::graphics::Texture fontTexture;
wi::graphics::Sampler sampler;
wi::graphics::InputLayout	imguiInputLayout;
wi::graphics::PipelineState imguiPSO;

#ifdef IS_DEV
struct ImGui_Impl_Data
{
};

static ImGui_Impl_Data* ImGui_Impl_GetBackendData()
{
	return ImGui::GetCurrentContext() ? (ImGui_Impl_Data*)ImGui::GetIO().BackendRendererUserData : nullptr;
}

bool ImGui_Impl_CreateDeviceObjects()
{
	auto* backendData = ImGui_Impl_GetBackendData();

	// Build texture atlas
	ImGuiIO& io = ImGui::GetIO();

	std::string fontDir = wi::helper::GetCurrentPath()+"/"+Resources::SourcePath::INTERFACE+"/cantarell.ttf";
	io.Fonts->AddFontFromFileTTF(fontDir.c_str(), 18.0f);

	std::string iconDir = wi::helper::GetCurrentPath()+"/"+Resources::SourcePath::INTERFACE+"/fa-solid-900.ttf";
	static const ImWchar icons_ranges[] = { ICON_MIN_FA, ICON_MAX_16_FA, 0 };
	ImFontConfig icons_config; icons_config.MergeMode = true; icons_config.PixelSnapH = true;
	io.Fonts->AddFontFromFileTTF(iconDir.c_str(), 18.0f, &icons_config, icons_ranges);

	unsigned char* pixels;
	int width, height;
	io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

	// Upload texture to graphics system
	TextureDesc textureDesc;
	textureDesc.width = width;
	textureDesc.height = height;
	textureDesc.mip_levels = 1;
	textureDesc.array_size = 1;
	textureDesc.format = Format::R8G8B8A8_UNORM;
	textureDesc.bind_flags = BindFlag::SHADER_RESOURCE;

	SubresourceData textureData;
	textureData.data_ptr = pixels;
	textureData.row_pitch = width * GetFormatStride(textureDesc.format);
	textureData.slice_pitch = textureData.row_pitch * height;

	wi::graphics::GetDevice()->CreateTexture(&textureDesc, &textureData, &fontTexture);

	SamplerDesc samplerDesc;
	samplerDesc.address_u = TextureAddressMode::WRAP;
	samplerDesc.address_v = TextureAddressMode::WRAP;
	samplerDesc.address_w = TextureAddressMode::WRAP;
	samplerDesc.filter = Filter::MAXIMUM_MIN_MAG_MIP_LINEAR;
	wi::graphics::GetDevice()->CreateSampler(&samplerDesc, &sampler);

	// Store our identifier
	io.Fonts->SetTexID((ImTextureID)&fontTexture);

	imguiInputLayout.elements =
	{
		{ "POSITION", 0, Format::R32G32_FLOAT, 0, (uint32_t)IM_OFFSETOF(ImDrawVert, pos), InputClassification::PER_VERTEX_DATA },
		{ "TEXCOORD", 0, Format::R32G32_FLOAT, 0, (uint32_t)IM_OFFSETOF(ImDrawVert, uv), InputClassification::PER_VERTEX_DATA },
		{ "COLOR", 0, Format::R8G8B8A8_UNORM, 0, (uint32_t)IM_OFFSETOF(ImDrawVert, col), InputClassification::PER_VERTEX_DATA },
	};

	// Create pipeline
	PipelineStateDesc desc;
	desc.vs = &imguiVS;
	desc.ps = &imguiPS;
	desc.il = &imguiInputLayout;
	desc.dss = wi::renderer::GetDepthStencilState(wi::enums::DSSTYPE_DEPTHREAD);
	desc.rs = wi::renderer::GetRasterizerState(wi::enums::RSTYPE_DOUBLESIDED);
	desc.bs = wi::renderer::GetBlendState(wi::enums::BSTYPE_TRANSPARENT);
	desc.pt = PrimitiveTopology::TRIANGLELIST;
	wi::graphics::GetDevice()->CreatePipelineState(&desc, &imguiPSO);

	return true;
}
#endif

const char* Game::GetApplicationName(){
	return "Indefinite Point";
}

void Application::Initialize(){
	wi::resourcemanager::SetMode(wi::resourcemanager::Mode::ALLOW_RETAIN_FILEDATA);

	wi::renderer::SetShaderSourcePath(wi::helper::GetCurrentPath()+"/"+Resources::SourcePath::SHADER+"/");
	wi::renderer::SetShaderPath(wi::helper::GetCurrentPath()+"/"+Resources::SourcePath::SHADER+"/");
#ifdef IS_DEV
    Initialize_ImGUI();
#endif
    wi::Application::Initialize();
	ScriptBindings::Init();
	LiveUpdate::Init();
#ifdef IS_DEV
	Editor::Init();
#endif

	view.app = this;
    view.init(canvas);
	view.Load();

	ActivatePath(&view);
}

#ifdef IS_DEV
void Application::Initialize_ImGUI(){
    {
        wi::renderer::LoadShader(ShaderStage::VS, imguiVS, "ImGuiVS.cso");
		wi::renderer::LoadShader(ShaderStage::PS, imguiPS, "ImGuiPS.cso");
    }

    IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();

    ImGui_ImplSDL2_InitForVulkan(window);

    IM_ASSERT(io.BackendRendererUserData == NULL && "Already initialized a renderer backend!");

    ImGui_Impl_Data* bd = IM_NEW(ImGui_Impl_Data)();
	io.BackendRendererUserData = (void*)bd;
	io.BackendRendererName = "Wicked";
	io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;
}
#else
	void Application::Initialize_ImGUI(){}
#endif

void Application::Compose(wi::graphics::CommandList cmd){
    wi::Application::Compose(cmd);
    Compose_ImGUI(cmd);
}

#ifdef IS_DEV
void Application::Compose_ImGUI(wi::graphics::CommandList cmd){
    ImGui::Render();

	auto drawData = ImGui::GetDrawData();

	if (!drawData || drawData->TotalVtxCount == 0)
	{
		return;
	}

	// Avoid rendering when minimized, scale coordinates for retina displays (screen coordinates != framebuffer coordinates)
	int fb_width = (int)(drawData->DisplaySize.x * drawData->FramebufferScale.x);
	int fb_height = (int)(drawData->DisplaySize.y * drawData->FramebufferScale.y);
	if (fb_width <= 0 || fb_height <= 0)
		return;

	auto* bd = ImGui_Impl_GetBackendData();

	GraphicsDevice* device = wi::graphics::GetDevice();

	// Get memory for vertex and index buffers
	const uint64_t vbSize = sizeof(ImDrawVert) * drawData->TotalVtxCount;
	const uint64_t ibSize = sizeof(ImDrawIdx) * drawData->TotalIdxCount;
	auto vertexBufferAllocation = device->AllocateGPU(vbSize, cmd);
	auto indexBufferAllocation = device->AllocateGPU(ibSize, cmd);

	// Copy and convert all vertices into a single contiguous buffer
	ImDrawVert* vertexCPUMem = reinterpret_cast<ImDrawVert*>(vertexBufferAllocation.data);
	ImDrawIdx* indexCPUMem = reinterpret_cast<ImDrawIdx*>(indexBufferAllocation.data);
	for (int cmdListIdx = 0; cmdListIdx < drawData->CmdListsCount; cmdListIdx++)
	{
		const ImDrawList* drawList = drawData->CmdLists[cmdListIdx];
		memcpy(vertexCPUMem, &drawList->VtxBuffer[0], drawList->VtxBuffer.Size * sizeof(ImDrawVert));
		memcpy(indexCPUMem, &drawList->IdxBuffer[0], drawList->IdxBuffer.Size * sizeof(ImDrawIdx));
		vertexCPUMem += drawList->VtxBuffer.Size;
		indexCPUMem += drawList->IdxBuffer.Size;
	}

	// Setup orthographic projection matrix into our constant buffer
	struct ImGuiConstants
	{
		float mvp[4][4];
	};

	{
		const float L = drawData->DisplayPos.x;
		const float R = drawData->DisplayPos.x + drawData->DisplaySize.x;
		const float T = drawData->DisplayPos.y;
		const float B = drawData->DisplayPos.y + drawData->DisplaySize.y;

		//Matrix4x4::CreateOrthographicOffCenter(0.0f, drawData->DisplaySize.x, drawData->DisplaySize.y, 0.0f, 0.0f, 1.0f, &constants.projectionMatrix);

		ImGuiConstants constants;

		float mvp[4][4] =
		{
			{ 2.0f / (R - L),   0.0f,           0.0f,       0.0f },
			{ 0.0f,         2.0f / (T - B),     0.0f,       0.0f },
			{ 0.0f,         0.0f,           0.5f,       0.0f },
			{ (R + L) / (L - R),  (T + B) / (B - T),    0.5f,       1.0f },
		};
		memcpy(&constants.mvp, mvp, sizeof(mvp));

		device->BindDynamicConstantBuffer(constants, 0, cmd);
	}

	const GPUBuffer* vbs[] = {
		&vertexBufferAllocation.buffer,
	};
	const uint32_t strides[] = {
		sizeof(ImDrawVert),
	};
	const uint64_t offsets[] = {
		vertexBufferAllocation.offset,
	};

	device->BindVertexBuffers(vbs, 0, 1, strides, offsets, cmd);
	device->BindIndexBuffer(&indexBufferAllocation.buffer, IndexBufferFormat::UINT16, indexBufferAllocation.offset, cmd);

	Viewport viewport;
	viewport.width = (float)fb_width;
	viewport.height = (float)fb_height;
	device->BindViewports(1, &viewport, cmd);

	device->BindPipelineState(&imguiPSO, cmd);

	device->BindSampler(&sampler, 0, cmd);

	// Will project scissor/clipping rectangles into framebuffer space
	ImVec2 clip_off = drawData->DisplayPos;         // (0,0) unless using multi-viewports
	ImVec2 clip_scale = drawData->FramebufferScale; // (1,1) unless using retina display which are often (2,2)

	//passEncoder->SetSampler(0, Sampler::LinearWrap());

	// Render command lists
	int32_t vertexOffset = 0;
	uint32_t indexOffset = 0;
	for (uint32_t cmdListIdx = 0; cmdListIdx < (uint32_t)drawData->CmdListsCount; ++cmdListIdx)
	{
		const ImDrawList* drawList = drawData->CmdLists[cmdListIdx];
		for (uint32_t cmdIndex = 0; cmdIndex < (uint32_t)drawList->CmdBuffer.size(); ++cmdIndex)
		{
			const ImDrawCmd* drawCmd = &drawList->CmdBuffer[cmdIndex];
			if (drawCmd->UserCallback)
			{
				// User callback, registered via ImDrawList::AddCallback()
				// (ImDrawCallback_ResetRenderState is a special callback value used by the user to request the renderer to reset render state.)
				if (drawCmd->UserCallback == ImDrawCallback_ResetRenderState)
				{
				}
				else
				{
					drawCmd->UserCallback(drawList, drawCmd);
				}
			}
			else
			{
				// Project scissor/clipping rectangles into framebuffer space
				ImVec2 clip_min(drawCmd->ClipRect.x - clip_off.x, drawCmd->ClipRect.y - clip_off.y);
				ImVec2 clip_max(drawCmd->ClipRect.z - clip_off.x, drawCmd->ClipRect.w - clip_off.y);
				if (clip_max.x < clip_min.x || clip_max.y < clip_min.y)
					continue;

				// Apply scissor/clipping rectangle
				Rect scissor;
				scissor.left = (int32_t)(clip_min.x);
				scissor.top = (int32_t)(clip_min.y);
				scissor.right = (int32_t)(clip_max.x);
				scissor.bottom = (int32_t)(clip_max.y);
				device->BindScissorRects(1, &scissor, cmd);

				const Texture* texture = (const Texture*)drawCmd->TextureId;
				device->BindResource(texture, 0, cmd);
				device->DrawIndexed(drawCmd->ElemCount, indexOffset, vertexOffset, cmd);
			}
			indexOffset += drawCmd->ElemCount;
		}
		vertexOffset += drawList->VtxBuffer.size();
	}
}
#else
void Application::Compose_ImGUI(wi::graphics::CommandList cmd){}
#endif

Application::~Application(){
#if IS_DEV
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
#endif
}

void ApplicationView::SetRenderPath(RENDERPATH path){
	switch(path){
		case RENDERPATH_DEFAULT:
			renderPath = std::make_unique<RenderPipeline::DefaultPipeline>();
			break;
		default:
			assert(0);
			break;
	}
	renderPath->scene = &Resources::GetScene().wiscene;
	renderPath->resolutionScale = resolutionScale;
	renderPath->Load();
#ifdef IS_DEV
	Editor::GetData()->viewport = renderPath.get();
#endif
}

void ApplicationView::ResizeBuffers(){
	init(app->canvas);
	RenderPath2D::ResizeBuffers();

	GraphicsDevice* device = wi::graphics::GetDevice();

	renderPath->init(*this);
	renderPath->ResizeBuffers();

	if(renderPath->GetDepthStencil() != nullptr)
	{
		bool success = false;

		XMUINT2 internalResolution = GetInternalResolution();

		TextureDesc desc;
		desc.width = internalResolution.x;
		desc.height = internalResolution.y;

		desc.format = Format::R8_UNORM;
		desc.bind_flags = BindFlag::RENDER_TARGET | BindFlag::SHADER_RESOURCE;
		if (renderPath->getMSAASampleCount() > 1)
		{
			desc.sample_count = renderPath->getMSAASampleCount();
			success = device->CreateTexture(&desc, nullptr, &rt_selectionOutline_MSAA);
			assert(success);
			desc.sample_count = 1;
		}
		success = device->CreateTexture(&desc, nullptr, &rt_selectionOutline[0]);
		assert(success);
		success = device->CreateTexture(&desc, nullptr, &rt_selectionOutline[1]);
		assert(success);

		{
			RenderPassDesc desc;
			desc.attachments.push_back(RenderPassAttachment::RenderTarget(&rt_selectionOutline[0], RenderPassAttachment::LoadOp::CLEAR));
			if (renderPath->getMSAASampleCount() > 1)
			{
				desc.attachments[0].texture = &rt_selectionOutline_MSAA;
				desc.attachments.push_back(RenderPassAttachment::Resolve(&rt_selectionOutline[0]));
			}
			desc.attachments.push_back(
				RenderPassAttachment::DepthStencil(
					renderPath->GetDepthStencil(),
					RenderPassAttachment::LoadOp::LOAD,
					RenderPassAttachment::StoreOp::STORE,
					ResourceState::DEPTHSTENCIL_READONLY,
					ResourceState::DEPTHSTENCIL_READONLY,
					ResourceState::DEPTHSTENCIL_READONLY
				)
			);
			success = device->CreateRenderPass(&desc, &renderpass_selectionOutline[0]);
			assert(success);

			if (renderPath->getMSAASampleCount() == 1)
			{
				desc.attachments[0].texture = &rt_selectionOutline[1]; // rendertarget
			}
			else
			{
				desc.attachments[1].texture = &rt_selectionOutline[1]; // resolve
			}
			success = device->CreateRenderPass(&desc, &renderpass_selectionOutline[1]);
			assert(success);
		}
	}
}

void ApplicationView::ResizeLayout(){
    RenderPath2D::ResizeLayout();
}

void ApplicationView::Load(){
	SetRenderPath(RENDERPATH_DEFAULT);
    RenderPath2D::Load();
}

void ApplicationView::Start(){
	RenderPath2D::Start();
}

void ApplicationView::PreUpdate(){
	RenderPath2D::PreUpdate();
	renderPath->PreUpdate();
}

void ApplicationView::FixedUpdate(){
	RenderPath2D::FixedUpdate();
	renderPath->FixedUpdate();
}

void ApplicationView::Update(float dt){
    ApplicationView::Update_ImGUI();
	Game::ScriptBindings::Update(dt);
	LiveUpdate::Update(dt);

#ifdef IS_DEV
	Editor::Update(dt);
#endif
	
    RenderPath2D::Update(dt);
	renderPath->Update(dt);
}

#ifdef IS_DEV
void ApplicationView::Update_ImGUI(){
	auto* backendData = ImGui_Impl_GetBackendData();
	IM_ASSERT(backendData != NULL);

	if (!fontTexture.IsValid())
	{
		ImGui_Impl_CreateDeviceObjects();
	}

    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();
	ImGuizmo::BeginFrame();

	ScriptBindings::ImGui::BeginFrame();
}
#else
void ApplicationView::Update_ImGUI(){}
#endif

void ApplicationView::PostUpdate(){
	RenderPath2D::PostUpdate();
	renderPath->PostUpdate();
}

void ApplicationView::Render() const{
	renderPath->Render();

	// Selection outline:
	if(renderPath->GetDepthStencil() != nullptr)
	{
		GraphicsDevice* device = wi::graphics::GetDevice();
		CommandList cmd = device->BeginCommandList();

		device->EventBegin("Editor - Selection Outline Mask", cmd);

		Viewport vp;
		vp.width = (float)rt_selectionOutline[0].GetDesc().width;
		vp.height = (float)rt_selectionOutline[0].GetDesc().height;
		device->BindViewports(1, &vp, cmd);

		wi::image::Params fx;
		fx.enableFullScreen();
		fx.stencilComp = wi::image::STENCILMODE::STENCILMODE_EQUAL;

		// We will specify the stencil ref in user-space, don't care about engine stencil refs here:
		//	Otherwise would need to take into account engine ref and draw multiple permutations of stencil refs.
		fx.stencilRefMode = wi::image::STENCILREFMODE_USER;

		// Materials outline:
		{
			device->RenderPassBegin(&renderpass_selectionOutline[0], cmd);

			// Draw solid blocks of selected materials
			fx.stencilRef = STENCILFX::SELECTION_OUTLINE;
			wi::image::Draw(wi::texturehelper::getWhite(), fx, cmd);

			device->RenderPassEnd(cmd);
		}

		// Objects outline:
		{
			device->RenderPassBegin(&renderpass_selectionOutline[1], cmd);

			// Draw solid blocks of selected objects
			fx.stencilRef = STENCILFX::SELECTION_OUTLINE;
			wi::image::Draw(wi::texturehelper::getWhite(), fx, cmd);

			device->RenderPassEnd(cmd);
		}

		device->EventEnd(cmd);
	}

    RenderPath2D::Render();
}

void ApplicationView::Compose(wi::graphics::CommandList cmd) const{
	renderPath->Compose(cmd);

	CameraComponent cam = *renderPath->camera;
	cam.jitter = XMFLOAT2(0, 0);
	cam.UpdateCamera();

	if (renderPath->GetDepthStencil() != nullptr)
	{
		GraphicsDevice* device = wi::graphics::GetDevice();
		device->EventBegin("Editor - Selection Outline", cmd);
		wi::renderer::BindCommonResources(cmd);
		float opacity = 1.f;
		XMFLOAT4 col = selectionColor2;
		col.w *= opacity;
		wi::renderer::Postprocess_Outline(rt_selectionOutline[0], cmd, 0.1f, 1, col);
		col = selectionColor;
		col.w *= opacity;
		wi::renderer::Postprocess_Outline(rt_selectionOutline[1], cmd, 0.1f, 1, col);
		device->EventEnd(cmd);
	}

	RenderPath2D::Compose(cmd);
}