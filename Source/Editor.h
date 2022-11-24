#pragma once

#include "stdafx.h"
#include "Editor_Translator.h"

#include "ImGui/imgui.h"
#include "ImGui/imgui_internal.h"
#include "ImGui/imgui_BindLua.h"
#include "ImGui/FA6_UI_Icons.h"
#include "ImGui/Widgets/ImGuizmo.h"

#ifdef _WIN32
#include "ImGui/imgui_impl_win32.h"
#elif defined(SDL2)
#include "ImGui/imgui_impl_sdl.h"
#endif

#include "Editor_FontAwesomeV6.h"

// ICON LIST ------------------------------------------
#define ICON_LAYER				ICON_FA_LAYER_GROUP
#define ICON_TRANSFORM			ICON_FA_LOCATION_DOT
#define ICON_MESH				ICON_FA_CUBE
#define ICON_OBJECT				ICON_FA_CUBES
#define ICON_RIGIDBODY			ICON_FA_CUBES_STACKED
#define ICON_SOFTBODY			ICON_FA_FLAG
#define ICON_EMITTER			ICON_FA_FIRE
#define ICON_HAIR				ICON_FA_SEEDLING
#define ICON_FORCE				ICON_FA_WIND
#define ICON_SOUND				ICON_FA_VOLUME_HIGH
#define ICON_DECAL				ICON_FA_NOTE_STICKY
#define ICON_CAMERA				ICON_FA_VIDEO
#define ICON_ENVIRONMENTPROBE	ICON_FA_EARTH_ASIA
#define ICON_ANIMATION			ICON_FA_PERSON_RUNNING
#define ICON_ARMATURE			ICON_FA_PERSON
#define ICON_POINTLIGHT			ICON_FA_LIGHTBULB
#define ICON_SPOTLIGHT			ICON_FA_LIGHTBULB // todo: find better one for spotlight
#define	ICON_DIRECTIONALLIGHT	ICON_FA_SUN
#define ICON_MATERIAL			ICON_FA_FILL_DRIP
#define ICON_WEATHER			ICON_FA_CLOUD
#define ICON_BONE				ICON_FA_BONE
#define ICON_IK					ICON_FA_HAND_FIST
#define ICON_NAME				ICON_FA_COMMENT_DOTS
#define ICON_COLLIDER			ICON_FA_CAPSULES
#define ICON_SCRIPT				ICON_FA_SCROLL
#define ICON_HIERARCHY			ICON_FA_ARROWS_DOWN_TO_PEOPLE
#define ICON_EXPRESSION			ICON_FA_MASKS_THEATER

#define ICON_TERRAIN			ICON_FA_MOUNTAIN_SUN

#define ICON_SAVE				ICON_FA_FLOPPY_DISK
#define ICON_OPEN				ICON_FA_FOLDER_OPEN
#define ICON_CLOSE				ICON_FA_TRASH
#define ICON_BACKLOG			ICON_FA_BOOK
#define ICON_HELP				ICON_FA_CIRCLE_QUESTION
#define ICON_EXIT				ICON_FA_CIRCLE_XMARK
#define ICON_SCALE				ICON_FA_UP_RIGHT_AND_DOWN_LEFT_FROM_CENTER
#define ICON_ROTATE				ICON_FA_ROTATE_RIGHT
#define ICON_TRANSLATE			ICON_FA_UP_DOWN_LEFT_RIGHT
#define ICON_CHECK				ICON_FA_CHECK
#define ICON_DISABLED			ICON_FA_BAN

#define ICON_LEFT_RIGHT			ICON_FA_LEFT_RIGHT
#define ICON_UP_DOWN			ICON_FA_UP_DOWN
#define ICON_UPRIGHT_DOWNLEFT	ICON_FA_UP_RIGHT_AND_DOWN_LEFT_FROM_CENTER
#define ICON_CIRCLE				ICON_FA_CIRCLE
#define ICON_SQUARE				ICON_FA_SQUARE_FULL
#define ICON_CUBE				ICON_FA_CUBE
#define ICON_LOOP				ICON_FA_REPEAT
#define ICON_PLAY				ICON_FA_PLAY
#define ICON_PAUSE				ICON_FA_PAUSE
#define ICON_STOP				ICON_FA_STOP
#define ICON_PEN				ICON_FA_PEN
#define ICON_FILTER				ICON_FA_FILTER

#define ICON_DARK				ICON_FA_MOON
#define ICON_BRIGHT				ICON_FA_SUN
#define ICON_SOFT				ICON_FA_LEAF
#define ICON_HACKING			ICON_FA_COMPUTER

// ----------------------------------------------------

namespace Editor{
    namespace IO
    {
        void ImportModel_GLTF(const std::string& fileName, Game::Resources::Scene& scene);
        void ExportModel_GLTF(const std::string& fileName, Game::Resources::Scene& scene);
    }
    struct ClipData
    {
        wi::unordered_map<uint64_t, wi::ecs::Entity> remap;
        wi::Archive archive;
    };
    struct GizmoData
    {
        wi::ecs::Entity entity = wi::ecs::INVALID_ENTITY;
        std::string icon;
    };
    struct Data
    {
        wi::RenderPath* viewport;
        wi::graphics::RenderPass renderpass_editor;
        wi::graphics::RenderPass renderpass_selection_editor;
        wi::graphics::Texture rt_depthbuffer_editor;
        wi::graphics::Texture rt_selection_editor;

        std::string current_loaded_scene;

        size_t entitylist_sizecache = 0;
        wi::vector<GizmoData> gizmo_data;

        const XMFLOAT4 editor_selection_color = XMFLOAT4(1.f,0.6f,0.3f,0.8f);
        const wi::Color inactiveEntityColor = wi::Color::fromFloat4(XMFLOAT4(1, 1, 1, 0.5f));
        const wi::Color hoveredEntityColor = wi::Color::fromFloat4(XMFLOAT4(1, 1, 1, 1));
        const wi::Color selectedEntityColor = wi::Color::fromFloat4(XMFLOAT4(1, 0.7f, 0.5f, 1));

        wi::scene::PickResult hovered;
        wi::scene::PickResult selection;
        Translator transform_translator;
        wi::scene::TransformComponent transform_start;

        wi::unordered_map<uint32_t, ClipData> clips_deleted;
        wi::unordered_map<wi::ecs::Entity, ClipData> clips_copy;

        wi::unordered_map<std::string, wi::Resource> resourcemap;
        
        wi::unordered_set<wi::ecs::Entity> unsaved_instances;

        // Preview Renderer Data
        Game::RenderPipeline::DefaultPipeline preview_render;
        wi::scene::CameraComponent preview_camera;
        Game::Resources::Scene preview_scene;
    };
    enum EDITOR_STENCILREF : uint8_t
    {
        EDITOR_STENCILREF_NONE = 0x00,
        EDITOR_STENCILREF_LAST = 0x0F,
    };
    Data* GetData();
    void Init();
    void Update(float dt, wi::RenderPath2D& viewport);

    //Render pipeline processes
    void ResizeBuffers(wi::graphics::GraphicsDevice* device, wi::RenderPath3D* renderPath);
    void Render(wi::graphics::GraphicsDevice* device, wi::graphics::CommandList& cmd);
    void Compose(wi::graphics::GraphicsDevice* device, wi::graphics::CommandList& cmd);
}