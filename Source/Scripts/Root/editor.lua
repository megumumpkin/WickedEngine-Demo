D("editor_data",{
    elements = {
        resexp = {
            win_visible = false,
            input = "",
        },
        scenegraphview = {
            win_visible = true,
            filter_selected = 0,
            selected_entity = 0,
            wait_update = false,
            list = {
                objects = {},
                meshes = {},
                materials = {},
                animations = {},
                weathers = {},
                lights = {},
                instances = {}
            }
        },
        compinspect = {
            win_visible = true,
            component = {
                name = {},
                transform = {},
                object = {},
                material = {},
                emitter = {},
                light = {},
                rigidbody = {},
                softbody = {},
                force = {},
                weather = {
                    atmosphere = {},
                    cloud = {},
                },
                sound = {},
                collider = {},
                instance = {}
            }
        },
        importwindow = {
            win_visible = false,
            file = "",
            type = "",
            opt_wiscene = {
                import_as_instance = false
            }
        },
        entityselector = {
            win_visible = false,
            filter_type = 0,
            search_string = "",
            selected_entity = 0
        },
        -- FIle menu windows
        fmenu_rnres = {
            win_visible = false,
            init = false,
            input = "",
        },
        -- Extras from Dear Imgui
        helper_demo = {
            win_visible = false,
        },
    },
    actions = {
        -- Command header
        command_head = 0,
        -- Resource menu actions
        resource_new = false,
        resource_rename = false,
        resource_save = false,
        -- Add menu actions
        link_dcc = false,
        import_wiscene = false,
        add_object = false,
        add_light = false,
        add_sound = false,
        add_weather = false,
    },
    navigation = {
        camera = GetCamera(),
        camera_transform = TransformComponent(),
        camera_pos = Vector(0,2,-5),
        camera_rot = Vector(0,0,0),
        camera_speed_mul_select = 0,
        camera_speed_mul = 3.0,
        translatormode = 0,
    },
    core_data = {
        resname = "Untitled Scene",
    }
})

local scene = GetGlobalGameScene()
local wiscene = scene.GetWiScene()

local CAM_MOVE_SPD = 0.3
local CAM_ROT_SPD = 0.03

local compio_name = {
    Name = {"Name", "text"}
}

local compio_transform = {
    Translation_local = {"Local Position", "float3"},
    Rotation_local = {"Local Rotation", "float4"},
    Scale_local = {"Local Scale", "float3"},
}

local compio_object = {
    Color = {"Color", "float4"},
    EmissiveColor = {"EmissiveColor", "float4"},
    CascadeMask = {"CascadeMask", "int"},
    RendertypeMask = {"RendertypeMask", "int"},
}

local compio_material = {
    ShaderType = {"Shader Type", "combo", { choices = "PBR\0PBR - Planar Reflection\0PBR - Anisotropic\0Water\0Cartoon\0Unlit\0PBR - Cloth\0PBR - Clearcoat\0PBR - Cloth Clearcoat\0" }},
    BaseColor = {"Base Color", "float4"},
	EmissiveColor = {"Emissive Color", "float4"},
	EngineStencilRef = {"Engine Stencil Ref", "int"},
	UserStencilRef = {"User Stencil Ref", "int"},
	UserBlendMode = {"User Blend Mode", "int"},
	SpecularColor = {"SpecularColor", "float4"},
	SubsurfaceScattering = {"Subsurface Scattering", "float4"},
	TexMulAdd = {"Texture Color Multiply", "float4"},
	Roughness = {"Roughness", "float"},
	Reflectance = {"Reflectance", "float"},
	Metalness = {"Metalness", "float"},
	NormalMapStrength = {"Normal Map Strength", "float"},
	ParallaxOcclusionMapping = {"Parallax Occlusion Mapping", "float"},
	DisplacementMapping = {"Displacement Mapping", "float"},
	Refraction = {"Refraction", "float"},
	Transmission = {"Transmission", "float"},
	AlphaRef = {"Alpha Ref", "float"},
	SheenColor = {"Sheen Color", "float3"},
	SheenRoughness = {"Sheen Roughness", "float"},
	Clearcoat = {"Clearcoat", "float"},
	ClearcoatRoughness = {"Clearcoat Roughness", "float"},
	TexAnimDirection = {"Texture Anim Direction", "float2"},
	TexAnimFrameRate = {"Texture Anim FrameRate", "float"},
	texAnimElapsedTime = {"Texture Anim Elapsed Time", "float"},
	customShaderID = {"Custom Shader ID", "int"},
}

local compio_emitter = {
    EmitCount = {"Emit Count", "float"},
	Size = {"Size", "float"},
	Life = {"Live", "float"},
	NormalFactor = {"Normal Factor", "float"},
	Randomness = {"Randomness", "float"},
	LifeRandomness = {"Life Randomness", "float"},
	ScaleX = {"Scale X", "float"},
	ScaleY = {"Scale Y", "float"},
	Rotation = {"Rotation", "float"},
	MotionBlurAmount = {"Motion Blur Amount", "float"},
}

local compio_light = {
    Type = {"Type", "combo", { choices = "Directional\0Point\0Spot\0" }},
    Range = {"Range", "float"},
    Intensity = {"Intensity", "float"},
    Color = {"Color", "float3"},
    OuterConeAngle = {"Outer Cone Angle", "float"},
    InnerConeAngle = {"Inner Cone Angle", "float"},
}

local compio_rigidbody = {
    Shape = {"Shape", "combo", { choices = "Box\0Sphere\0Capsule\0Convex Hull\0Triangle Mesh\0" }},
    Mass = {"Mass", "float"},
    Friction = {"Friction", "float"},
    Restitution = {"Restitution", "float"},
    LinearDamping = {"LinearDamping", "float"},
    AngularDamping = {"AngularDamping", "float"},
    BoxParams_HalfExtents = {"BoxParams_HalfExtents", "float3"},
    SphereParams_Radius = {"SphereParams_Radius", "float"},
    CapsuleParams_Radius = {"CapsuleParams_Radius", "float"},
    CapsuleParams_Height = {"CapsuleParams_Height", "float"},
    TargetMeshLOD = {"TargetMeshLOD", "int"},
}

local compio_softbody = {
    Mass = {"Mass", "float"},
    Friction = {"Friction", "float"},
    Restitution = {"Restitution", "float"},
}

local compio_forcefield = {
    Type = {"Type", "combo", { choices = "Point\0Plane\0" }},
    Gravity = {"Gravity", "float"},
    Range = {"Range", "float"},
}

local compio_weather_atmos = {
    bottomRadius = {"Bottom Radius", "float"},
	topRadius = {"Top Radius", "float"},
	planetCenter = {"Planet Center", "float3"},
	rayleighDensityExpScale = {"Rayleigh Density Exp Scale", "float"},
	rayleighScattering = {"Rayleigh Scattering", "float3"},
	mieDensityExpScale = {"Mie Density Exp Scale", "float"},
	mieScattering = {"Mie Scattering", "float3"},
	mieExtinction = {"Mie Extinction", "float3"},
	mieAbsorption = {"Mie Absorption", "float3"},
	miePhaseG = {"Mie G Phase", "float"},
	absorptionDensity0LayerWidth = {"Absorption Density Layer 0 - Width", "float"},
	absorptionDensity0ConstantTerm = {"Absorption Density Layer 0 - Constant Term", "float"},
	absorptionDensity0LinearTerm = {"Absorption Density Layer 0 - Linear Term", "float"},
	absorptionDensity1ConstantTerm = {"Absorption Density Layer 1 - Constant Term", "float"},
	absorptionDensity1LinearTerm = {"Absorption Density Layer 1 - Linear Term", "float"},
	absorptionExtinction = {"Absorption Extinction", "float3"},
	groundAlbedo = {"Ground Albedo", "float3"},
}

local compio_weather_cloud = {
    Albedo = {"Albedo Color", "float3"},
	CloudAmbientGroundMultiplier = {"Cloud Ambient Ground Multiplier", "float"},
	ExtinctionCoefficient = {"Extinction Coefficient", "float3"},
	HorizonBlendAmount = {"Horizon Blend Amount", "float"},
	HorizonBlendPower = {"Horizon Blend Power", "float"},
	WeatherDensityAmount = {"Weather Density Amount", "float"},
	CloudStartHeight = {"Cloud Start Height", "float"},
	CloudThickness = {"Cloud Thickness", "float"},
	SkewAlongWindDirection = {"Skew Along Wind Direction", "float"},
	TotalNoiseScale = {"Total Noise Scale", "float"},
	DetailScale = {"Detail Scale", "float"},
	WeatherScale = {"Weather Scale", "float"},
	CurlScale = {"Curl Scale", "float"},
	DetailNoiseModifier = {"Detail Noise Modifier", "float"},
	TypeAmount = {"Type Amount", "float"},
	TypeMinimum = {"Type Minimum", "float"},
	AnvilAmount = {"Anvil Amount", "float"},
	AnvilOverhangHeight = {"Anvil Overhang Height", "float"},
	AnimationMultiplier = {"Animation Multiplier", "float"},
	WindSpeed = {"Wind Speed", "float"},
	WindAngle = {"Wind Angle", "float"},
	WindUpAmount = {"Wind Up Amount", "float"},
	CoverageWindSpeed = {"Coverage Wind Speed", "float"},
	CoverageWindAngle = {"Coverage Wind Angle", "float"},
	CloudGradientSmall = {"Cloud Gradient Small", "float3"},
	CloudGradientMedium = {"Cloud Gradient Medium", "float3"},
	CloudGradientLarge = {"Cloud Gradient Large", "float3"},
}

local compio_weather = {
    sunColor = {"Sun Color" , "float3"},
    sunDirection = {"Sun Direction"  , "float3"},
    skyExposure = {"Sky Exposure" , "float"},
    horizon = {"Horizon Color" , "float3"},
    zenith = {"Zenith Color" , "float3"},
    ambient = {"Ambient Color" , "float3"},
    fogStart = {"Fog Start" , "float"},
    fogEnd = {"Fog End" , "float"},
    fogHeightStart = {"Fog Height Start" , "float"},
    fogHeightEnd = {"Fog Height End" , "float"},
    windDirection = {"Wind Direction" , "float3"},
    windRandomness = {"Wind Randomness" , "float"},
    windWaveSize = {"Wind Wave Size" , "float"},
    windSpeed = {"Wind Speed" , "float"},
    stars = {"Star Density" , "float"}
}

local compio_sound = {
    Filename = {"Filename", "text"},
    Volume = {"Volume", "float"}
}

local compio_collider = {
    Shape = {"Shape", "combo", { choices = "Sphere\0Capsule\0Plane\0" }},
    Radius = {"Radius", "float"},
    Offset = {"Offset", "float3"},
    Tail = {"Tail", "float3"},
}

local compio_instance = {
    File = {"File", "text"},
    EntityName = {"Subtarget Entity Name", "text"},
    Strategy = {"Loading Strategy", "combo", { choices = "Direct\0Instance\0Preload\0" }},
    Type = {"Type", "combo", { choices = "Default\0Library\0" }},
}

local compio_stream = {
    ExternalSubstitute = {"External Substitute Model", "text"},
    Substitute = {"Substitute", "int"}
}

local component_set_generic = function(component, editdata)
    for key, data in pairs(editdata) do
        component[key] = data
    end
end

local component_set_transform = function(component, editdata)
    component_set_generic(component, editdata)
    component.SetDirty()
end

local component_set_light = function(component, editdata)
    component.Type = editdata.Type
    component.Range = editdata.Range
    component.Intensity = editdata.Intensity
    component.Color = editdata.Color
    component.OuterConeAngle = editdata.OuterConeAngle
    component.InnerConeAngle = editdata.InnerConeAngle
    component.SetCastShadow(editdata.set_shadow)
    component.SetVolumetricsEnabled(editdata.set_volumetric)
end

local component_set_sound = function(component, editdata)
    component.Filename = editdata.Filename
    component.Volume = editdata.Volume
    component.SetLooped(editdata.set_loop)
    component.SetDisable3D(editdata.set_3d)
    if editdata.play then
        component.Play()
    else
        component.Stop()
    end
end

local component_set_weather = function(component, editdata)
    component_set_generic(component.AtmosphereParameters, editdata.atmosphere)
    component_set_generic(component.VolumetricCloudParameters, editdata.cloud)
    for key, _ in pairs(compio_weather) do
        component[key] = editdata[key]
    end
end

local component_set_instance = function(component, editdata)
    for key, _ in pairs(compio_instance) do
        component[key] = editdata[key]
    end
end

local edit_execcmd = function(command, extradata, holdout)
    if command == "add_obj" then
        if type(extradata) == "table" then
            local entity = 0
            if holdout == nil then
                entity = scene.Entity_Create()
            else
                entity = extradata.entity
            end
            
            extradata.entity = entity
            local name = wiscene.Component_CreateName(entity)
            name.SetName(extradata.name)
            if extradata.type == "object" then
                local transform = wiscene.Component_CreateTransform(entity)
                local layer = wiscene.Component_CreateLayer(entity)
                local object = wiscene.Component_CreateObject(entity)
            end
            if extradata.type == "light" then
                local transform = wiscene.Component_CreateTransform(entity)
                local layer = wiscene.Component_CreateLayer(entity)
                local light = wiscene.Component_CreateLight(entity)
            end
            if extradata.type == "sound" then
                local transform = wiscene.Component_CreateTransform(entity)
                local layer = wiscene.Component_CreateLayer(entity)
                local sound = wiscene.Component_CreateSound(entity)
            end
            if extradata.type == "weather" then
                local weather = wiscene.Component_CreateWeather(entity)
                weather.SetRealisticSky(true)
                weather.SetVolumetricClouds(true)
            end
        end
    end

    if command == "mod_comp" then
        if type(extradata) == "table" then
            if extradata.type == "name" then
                local namecomponent = wiscene.Component_GetName(extradata.entity)
                if namecomponent then component_set_generic(namecomponent, extradata.post) end  
            end
            if extradata.type == "layer" then
                local layercomponent = wiscene.Component_GetLayer(extradata.entity)
                if layercomponent then component_set_generic(layercomponent, extradata.post) end
            end
            if extradata.type == "transform" then
                local transformcomponent = wiscene.Component_GetTransform(extradata.entity)
                if transformcomponent then component_set_transform(transformcomponent, extradata.post) end
            end
            if extradata.type == "object" then
                local objectcomponent = wiscene.Component_GetObject(extradata.entity)
                if objectcomponent then component_set_generic(objectcomponent, extradata.post) end
                if extradata.pre.meshID ~= extradata.post.meshID then Editor_UpdateGizmoData(extradata.entity) end
            end
            if extradata.type == "material" then
                local materialcomponent = wiscene.Component_GetMaterial(extradata.entity)
                if materialcomponent then component_set_generic(materialcomponent, extradata.post) end
            end
            if extradata.type == "emitter" then
                local emittercomponent = wiscene.Component_GetEmitter(extradata.entity)
                if emittercomponent then component_set_generic(emittercomponent, extradata.post) end
            end
            if extradata.type == "light" then
                local lightcomponent = wiscene.Component_GetLight(extradata.entity)
                if lightcomponent then component_set_light(lightcomponent, extradata.post) end
                if extradata.pre.type ~= extradata.post.type then Editor_UpdateGizmoData(extradata.entity) end
            end
            if extradata.type == "rigidbody" then
                local rigidbodycomponent = wiscene.Component_GetRigidBodyPhysics(extradata.entity)
                if rigidbodycomponent then component_set_generic(rigidbodycomponent, extradata.post) end
            end
            if extradata.type == "softbody" then
                local softbodycomponent = wiscene.Component_GetSoftBodyPhysics(extradata.entity)
                if softbodycomponent then component_set_generic(softbodycomponent, extradata.post) end
            end
            if extradata.type == "force" then
                local forcecomponent = wiscene.Component_GetForceField(extradata.entity)
                if forcecomponent then component_set_generic(forcecomponent, extradata.post) end
            end
            if extradata.type == "weather" then
                local weathercomponent = wiscene.Component_GetWeather(extradata.entity)
                if weathercomponent then component_set_weather(weathercomponent, extradata.post) end
            end
            if extradata.type == "sound" then
                local soundcomponent = wiscene.Component_GetSound(extradata.entity)
                if soundcomponent then component_set_sound(soundcomponent, extradata.post) end
            end
            if extradata.type == "collider" then
                local collidercomponent = wiscene.Component_GetCollider(extradata.entity)
                if collidercomponent then component_set_generic(collidercomponent, extradata.post) end
            end
            if extradata.type == "instance" then
                local instancecomponent = scene.Component_GetInstance(extradata.entity)
                if instancecomponent then component_set_instance(instancecomponent, extradata.post) end
            end
        end
    end

    if command == "del_obj" then
        if holdout == nil then
            extradata.index = #D.editor_data.actions.command_list
            Editor_StashDeletedEntity(extradata.entity, extradata.index)
        end
        wiscene.Entity_Remove(extradata.entity)
    end

    -- To run new command or just redo previous commands
    if holdout == nil then
        if D.editor_data.actions.command_head < #D.editor_data.actions.command_list then
            for idx = #D.editor_data.actions.command_list, D.editor_data.actions.command_head, -1 do
                if idx > 1 then 
                    table.remove(D.editor_data.actions.command_list, idx) 
                    Editor_DeletedEntityDrop(idx)
                end
            end
        end
        table.insert(D.editor_data.actions.command_list, {command,extradata})
        D.editor_data.actions.command_head = #D.editor_data.actions.command_list
    end

    D.editor_data.elements.scenegraphview.wait_update = true
end

local edit_redocmd = function()
    local current = D.editor_data.actions.command_head
    local next = D.editor_data.actions.command_head + 1
    if current < #D.editor_data.actions.command_list then
        D.editor_data.actions.command_head = next
        edit_execcmd(D.editor_data.actions.command_list[next][1],D.editor_data.actions.command_list[next][2], true)
    end
end

local edit_undocmd = function()
    local command = D.editor_data.actions.command_list[D.editor_data.actions.command_head][1]
    local extradata = D.editor_data.actions.command_list[D.editor_data.actions.command_head][2]

    if command == "add_obj" then
        wiscene.Entity_Remove(extradata.entity)
    end
    if command == "mod_comp" then
        if extradata.type == "name" then
            local namecomponent = wiscene.Component_GetName(extradata.entity)
            if namecomponent then component_set_generic(namecomponent, extradata.pre) end  
        end
        if extradata.type == "layer" then
            local layercomponent = wiscene.Component_GetLayer(extradata.entity)
            if layercomponent then component_set_generic(layercomponent, extradata.pre) end
        end
        if extradata.type == "transform" then
            local transformcomponent = wiscene.Component_GetTransform(extradata.entity)
            if transformcomponent then component_set_transform(transformcomponent, extradata.pre) end
        end
        if extradata.type == "object" then
            local objectcomponent = wiscene.Component_GetObject(extradata.entity)
            if objectcomponent then component_set_generic(objectcomponent, extradata.pre) end
            if extradata.pre.meshID ~= extradata.post.meshID then Editor_UpdateGizmoData(extradata.entity) end
        end
        if extradata.type == "material" then
            local materialcomponent = wiscene.Component_GetMaterial(extradata.entity)
            if materialcomponent then component_set_generic(materialcomponent, extradata.pre) end
        end
        if extradata.type == "emitter" then
            local emittercomponent = wiscene.Component_GetEmitter(extradata.entity)
            if emittercomponent then component_set_generic(emittercomponent, extradata.pre) end
        end
        if extradata.type == "light" then
            local lightcomponent = wiscene.Component_GetLight(extradata.entity)
            if lightcomponent then component_set_light(lightcomponent, extradata.pre) end
            if extradata.pre.type ~= extradata.post.type then Editor_UpdateGizmoData(extradata.entity) end
        end
        if extradata.type == "rigidbody" then
            local rigidbodycomponent = wiscene.Component_GetRigidBodyPhysics(extradata.entity)
            if rigidbodycomponent then component_set_generic(rigidbodycomponent, extradata.pre) end
        end
        if extradata.type == "softbody" then
            local softbodycomponent = wiscene.Component_GetSoftBodyPhysics(extradata.entity)
            if softbodycomponent then component_set_generic(softbodycomponent, extradata.pre) end
        end
        if extradata.type == "weather" then
            local weathercomponent = wiscene.Component_GetWeather(extradata.entity)
            if weathercomponent then component_set_weather(weathercomponent, extradata.pre) end
        end
        if extradata.type == "sound" then
            local soundcomponent = wiscene.Component_GetSound(extradata.entity)
            if soundcomponent then component_set_sound(soundcomponent, extradata.pre) end
        end
        if extradata.type == "collider" then
            local collidercomponent = wiscene.Component_GetCollider(extradata.entity)
            if collidercomponent then component_set_generic(collidercomponent, extradata.pre) end
        end
        if extradata.type == "instance" then
            local instancecomponent = scene.Component_GetInstance(extradata.entity)
            if instancecomponent then component_set_instance(instancecomponent, extradata.post) end
        end
    end
    if command == "del_obj" then
        Editor_RestoreDeletedEntity(extradata.index)
    end

    D.editor_data.actions.command_head = math.max(D.editor_data.actions.command_head - 1, 1)
    
    D.editor_data.elements.scenegraphview.wait_update = true
end

local drawtopbar = function()
    local viewport = imgui.GetMainViewport()
    imgui.SetNextWindowPos(viewport.Pos.x,viewport.Pos.y)
    imgui.SetNextWindowSize(viewport.Size.x,20.0)
    local mainbarflags = 0 | imgui.constant.WindowFlags.NoBackground | imgui.constant.WindowFlags.NoTitleBar | imgui.constant.WindowFlags.NoResize | imgui.constant.WindowFlags.NoMove | imgui.constant.WindowFlags.NoScrollbar | imgui.constant.WindowFlags.NoSavedSettings
    imgui.PushStyleVar(imgui.constant.StyleVar.WindowBorderSize, 0)
    if imgui.Begin("##MainBar", true, mainbarflags) then
        imgui.PopStyleVar()
        if imgui.Button("\xef\x86\xb2 " .. D.editor_data.core_data.resname .. "   \xef\x83\x97") then
            D.editor_data.elements.resexp.win_visible = not D.editor_data.elements.resexp.win_visible
        end
        imgui.SameLine()
        if imgui.Button("\xef\x85\x9b File") then
            imgui.OpenPopup("MBFM")
        end
        imgui.SameLine()
        if imgui.Button("\xef\x83\xbe Add") then
            imgui.OpenPopup("MBIM")
        end
        imgui.SameLine()
        if imgui.Button("\xef\x8b\x90 Window") then
            imgui.OpenPopup("MBWM")
        end
        imgui.SameLine()
        imgui.SetNextItemWidth(60.0)
        local changed_translatormode = false
        changed_translatormode, D.editor_data.navigation.translatormode = imgui.Combo("\xef\x81\x87##translatormode",D.editor_data.navigation.translatormode,"\xef\x81\x87\0\xef\x80\x9e\0\xef\x90\xa4\0")
        if changed_translatormode then Editor_SetTranslatorMode(D.editor_data.navigation.translatormode+1) end

        imgui.SameLine()
        imgui.SetNextItemWidth(60.0)
        local changed_camspdmode = false
        changed_camspdmode, D.editor_data.navigation.camera_speed_mul_select = imgui.Combo("\xef\x80\xb0SPD##camspdmul",D.editor_data.navigation.camera_speed_mul_select,"x2\0x3\0x4\0x5\0")
        if changed_camspdmode then D.editor_data.navigation.camera_speed_mul = 3.0 * (D.editor_data.navigation.camera_speed_mul_select+1) end

        if imgui.BeginPopupContextWindow("MBFM") then
            D.editor_data.actions.resource_new = imgui.MenuItem("\xee\x93\xae New Resource",nil,D.editor_data.actions.resource_new)
            D.editor_data.actions.resource_rename = imgui.MenuItem("\xef\x81\x84 Rename Resource",nil,D.editor_data.actions.resource_rename)
            D.editor_data.actions.resource_save = imgui.MenuItem("\xef\x83\x87 Save Resource",nil,D.editor_data.actions.resource_save)
            imgui.EndPopup()
        end

        if imgui.BeginPopupContextWindow("MBIM") then
            D.editor_data.actions.link_dcc = imgui.MenuItem("\xef\x83\x81 Create DCC Link",nil,D.editor_data.actions.link_dcc)
            D.editor_data.actions.import_wiscene = imgui.MenuItem("\xee\x92\xb8 Import WiScene",nil,D.editor_data.actions.import_wiscene)
            D.editor_data.actions.add_object = imgui.MenuItem("\xef\x86\xb2 Add Object",nil,D.editor_data.actions.add_object)
            D.editor_data.actions.add_light = imgui.MenuItem("\xef\x83\xab Add Light",nil,D.editor_data.actions.add_light)
            D.editor_data.actions.add_sound = imgui.MenuItem("\xef\x80\xa8 Add Sound",nil,D.editor_data.actions.add_sound)
            D.editor_data.actions.add_weather = imgui.MenuItem("\xef\x9b\x84 Add Weather",nil,D.editor_data.actions.add_weather)
            imgui.EndPopup()
        end

        if imgui.BeginPopupContextWindow("MBWM") then
            ret, D.editor_data.elements.scenegraphview.win_visible = imgui.MenuItem_4("\xef\xa0\x82 Scene Graph Viewer","",D.editor_data.elements.scenegraphview.win_visible)
            ret, D.editor_data.elements.compinspect.win_visible = imgui.MenuItem_4("\xef\x82\x85 Component Inspector","",D.editor_data.elements.compinspect.win_visible)
            ret, D.editor_data.elements.helper_demo.win_visible = imgui.MenuItem_4("\xef\x8b\x90 IMGUI Demo Window","",D.editor_data.elements.helper_demo.win_visible)
            imgui.EndPopup()
        end

        imgui.End() 
    end
end

local drawmenubardialogs = function()
    if D.editor_data.elements.fmenu_rnres.win_visible then
        local fmenu_rnres = D.editor_data.elements.fmenu_rnres

        if fmenu_rnres.init == false then
            fmenu_rnres.input = D.editor_data.core_data.resname
            fmenu_rnres.init = true
        end

        local sub_visible = false
        sub_visible, fmenu_rnres.win_visible = imgui.Begin("\xef\x81\x84 Rename Scene", fmenu_rnres.win_visible)
        if sub_visible then
            ret, fmenu_rnres.input = imgui.InputText("##fmenu_rnres_input", fmenu_rnres.input, 255)
            imgui.SameLine()
            if imgui.Button("\xef\x81\x84 ") then
                D.editor_data.core_data.resname = fmenu_rnres.input
                fmenu_rnres.input = ""
            end
            imgui.End()
        end
    end
end

local drawsceneexp = function()
    local resexp = D.editor_data.elements.resexp

    if resexp.win_visible then
        local sub_visible = false
        sub_visible, resexp.win_visible = imgui.Begin("\xef\x86\xb2 Scene Explorer", resexp.win_visible)
        if sub_visible then
            _, resexp.input = imgui.InputText("##resexp_sin", resexp.input, 255)
            imgui.SameLine() imgui.Button("\xef\x85\x8e Search")

            imgui.PushStyleVar(imgui.constant.StyleVar.ChildRounding, 5.0)
            local childflags = 0 | imgui.constant.WindowFlags.NoTitleBar
            local view_oblist = imgui.BeginChild("##listview", 0, 0, true, childflags)
            imgui.PopStyleVar()

            -- Display previews of scenes here!

            imgui.EndChild()
        end
        imgui.End()
    end
end

function Editor_DisplayEntityList(entities_list, selector, holdout)
    for _, entity in pairs(entities_list) do
        local name = "entity-" .. entity

        local nameComponent = wiscene.Component_GetName(entity)
        if nameComponent then name = nameComponent.Name end

        local flag = imgui.constant.TreeNodeFlags.Leaf

        if selector == entity then
            flag = flag | imgui.constant.TreeNodeFlags.Selected
        end

        local ret_tree = imgui.TreeNodeEx(name .. "##" .. entity, flag)
        if imgui.IsItemClicked() then
            selector = entity
            if holdout == nil then Editor_FetchSelection(selector) end
        end

        if ret_tree then
            imgui.TreePop()
        end
    end
    return selector
end

function Editor_DisplayTreeList(entities_list, selector, holdout)
    local scenegraphview = D.editor_data.elements.scenegraphview
    for _, entity in pairs(entities_list) do
        local name = "entity-" .. entity

        local nameComponent = wiscene.Component_GetName(entity)
        if nameComponent then name = nameComponent.Name end

        local flag = 0
        local has_children = true
        if type(scenegraphview.list.objects[string.format("%i" , entity)]) ~= "table" then 
            flag = flag | imgui.constant.TreeNodeFlags.Leaf
            has_children = false
        end
        if selector == entity then
            flag = flag | imgui.constant.TreeNodeFlags.Selected
        end

        local ret_tree = imgui.TreeNodeEx(name .. "##" .. entity, flag)
        if imgui.IsItemClicked() then
            selector = entity
            if holdout == nil then Editor_FetchSelection(selector) end
        end
        if ret_tree then
            if has_children then selector = Editor_DisplayTreeList(scenegraphview.list.objects[string.format("%i" , entity)], selector) end
            imgui.TreePop()
        end
    end
    return selector
end

local drawscenegraphview = function()
    local scenegraphview = D.editor_data.elements.scenegraphview

    if scenegraphview.win_visible then
        local sub_visible = false
        sub_visible, scenegraphview.win_visible = imgui.Begin("\xef\xa0\x82 Scene Graph Viewer", scenegraphview.win_visible)
        if sub_visible then
            ret_filter, scenegraphview.filter_selected = imgui.Combo("\xef\x82\xb0##filter",scenegraphview.filter_selected,"Objects\0Meshes\0Materials\0Animation\0Lights\0Weathers\0")
            
            imgui.PushStyleVar(imgui.constant.StyleVar.ChildRounding, 5.0)
            local childflags = 0 | imgui.constant.WindowFlags.NoTitleBar
            local view_oblist = imgui.BeginChild("##listview", 0, 0, true, childflags)
            imgui.PopStyleVar()
            
            if view_oblist then
                if not scenegraphview.wait_update then
                    local entities_list = {}
                    if scenegraphview.filter_selected == 0 then 
                        if scenegraphview.list.objects then entities_list = scenegraphview.list.objects["0"] end
                    end
                    if scenegraphview.filter_selected == 1 then entities_list = scenegraphview.list.meshes end
                    if scenegraphview.filter_selected == 2 then entities_list = scenegraphview.list.materials end
                    if scenegraphview.filter_selected == 3 then entities_list = scenegraphview.list.animations end
                    if scenegraphview.filter_selected == 4 then entities_list = scenegraphview.list.lights end
                    if scenegraphview.filter_selected == 5 then entities_list = scenegraphview.list.weathers end
                    if type(entities_list) ~= "nil" then
                        if scenegraphview.filter_selected > 0 then
                            scenegraphview.selected_entity = Editor_DisplayEntityList(entities_list, scenegraphview.selected_entity)
                        else
                            scenegraphview.selected_entity = Editor_DisplayTreeList(entities_list, scenegraphview.selected_entity)
                        end
                    end
                end    
            end
            imgui.EndChild()
        end
        imgui.End()
    end
end

local display_edit_parameters = function(component, parameter_list, edit_store)
    local changed = false

    for key, data in pairs(parameter_list) do
        -- Init
        if edit_store[key] == nil then edit_store[key] = component[key] end
        -- 

        -- TODO: draw specific ui types by condition
        local label = data[1]
        local type = data[2]
        local extradata = data[3]

        if type == "int" then
            local ret = false
            ret, edit_store[key] = imgui.InputInt(label, edit_store[key])
            if ret then changed = true end
        end

        if type == "float" then
            _, edit_store[key] = imgui.InputFloat(label, edit_store[key], 0, 0, "%.12f")
        end

        if type == "float2" then
            _, edit_store[key].X, edit_store[key].Y = imgui.InputFloat2(label, edit_store[key].X, edit_store[key].Y, "%.12f")
        end

        if type == "float3" then
            _, edit_store[key].X, edit_store[key].Y, edit_store[key].Z = imgui.InputFloat3(label, edit_store[key].X, edit_store[key].Y, edit_store[key].Z, "%.12f")
        end

        if type == "float4" then
            _, edit_store[key].X, edit_store[key].Y, edit_store[key].Z, edit_store[key].W = imgui.InputFloat4(label, edit_store[key].X, edit_store[key].Y, edit_store[key].Z, edit_store[key].W, "%.12f")
        end

        if type == "text" then
            _, edit_store[key] = imgui.InputText(label, edit_store[key], 255)
        end

        if type == "check" then
            local ret = false
            ret, edit_store[key] = imgui.InputText(label, edit_store[key], 255)
            if ret then changed = true end
        end

        if (type == "combo") and (extradata ~= nil) then
            local ret = false
            ret, edit_store[key] = imgui.Combo(label, edit_store[key], extradata.choices)
            if ret then changed = true end
        end
    end

    return changed
end

local build_edit_prestate = function(component, parameter_list, pre_storage)
    for key, _ in pairs(parameter_list) do
        pre_storage[key] = component[key]
    end
end

local drawcompinspect = function()
    local compinspect = D.editor_data.elements.compinspect
    if compinspect.win_visible then
        local sub_visible = false
        sub_visible, compinspect.win_visible = imgui.Begin("\xef\x82\x85 Component Inspector", compinspect.win_visible)
        if sub_visible then
            local entity = D.editor_data.elements.scenegraphview.selected_entity
            if entity > 0 then
                
                -- NameComponent
                local namecomponent = wiscene.Component_GetName(entity)
                if namecomponent then
                    local editor_name = compinspect.component.name
                    local ret_tree = imgui.TreeNode("Name Component")
                    if ret_tree then
                        display_edit_parameters(namecomponent, compio_name, editor_name)
                        
                        if input.Press(KEYBOARD_BUTTON_ENTER) then
                            local editdata = {
                                entity = entity,
                                type = "name",
                                pre = {},
                                post = deepcopy(editor_name)
                            }
                            build_edit_prestate(namecomponent, compio_name, editdata.pre)
                            edit_execcmd("mod_comp", editdata)
                        end

                        imgui.TreePop()
                    end
                    if not imgui.IsItemFocused() then build_edit_prestate(namecomponent, compio_name, editor_name) end
                end
                --

                -- LayerComponent
                local layercomponent = wiscene.Component_GetLayer(entity)
                if layercomponent then
                    local ret_tree = imgui.TreeNode("Layer Component")
                    if ret_tree then
                        local changed = false
                        local layers = layercomponent.LayerMask
                        local set = 0
                        
                        for flag_id = 0, 31, 1 do
                            local block = 1 << flag_id
                            local get_check = (layers & block) > 0
                            local set_changed = false
                            
                            if (flag_id%6 > 0) and (flag_id ~= 0) then imgui.SameLine() end
                            set_changed, get_check = imgui.Checkbox(flag_id .. "##" .. flag_id, get_check)
                            if set_changed then changed = true end

                            local get = 0
                            if get_check == true then
                                set = set | block
                            end
                        end
                        if imgui.Button("Select ALL") then
                            set = 0xffffffff
                            changed = true
                        end
                        imgui.SameLine()
                        if imgui.Button("Select NONE") then 
                            set = 0 
                            changed = true
                        end
                        imgui.TreePop()
                        if changed then
                            local editdata = {
                                entity = entity,
                                type = "layer",
                                pre = {
                                    LayerMask = layercomponent.LayerMask
                                },
                                post = {
                                    LayerMask = set
                                }
                            }
                            edit_execcmd("mod_comp", editdata)
                        end
                    end
                end
                --

                -- TransformComponent
                local transformcomponent = wiscene.Component_GetTransform(entity)
                if transformcomponent then
                    local editor_transform = compinspect.component.transform
                    local ret_tree = imgui.TreeNode("Transform Component")

                    if ret_tree then
                        local changed = false

                        local changed_params = display_edit_parameters(transformcomponent, compio_transform, editor_transform)
                        if changed_params then changed = true end

                        if input.Press(KEYBOARD_BUTTON_ENTER) then changed = true end
                        if changed then
                            local editdata = {
                                entity = entity,
                                type = "transform",
                                pre = {},
                                post = deepcopy(editor_transform)
                            }
                            build_edit_prestate(transformcomponent, compio_transform, editdata.pre)
                            edit_execcmd("mod_comp", editdata)
                        end

                        imgui.TreePop()
                    end
                    if not imgui.IsItemFocused() then 
                        build_edit_prestate(transformcomponent, compio_transform, editor_transform)
                    end
                end
                --

                -- ObjectComponent
                local objectcomponent = wiscene.Component_GetObject(entity)
                if objectcomponent then
                    local editor_object = compinspect.component.transform
                    -- Init
                    if editor_object.MeshID == nil then editor_object.MeshID = objectcomponent.MeshID end
                    --
                    local ret_tree = imgui.TreeNode("Object Component")
                    if ret_tree then
                        local changed = false

                        local mesh_name = editor_object.MeshID .. " - NO MESH"
                        if editor_object.MeshID > 0 then
                            local mesh_namecomponent = wiscene.Component_GetName(editor_object.MeshID)
                            if mesh_namecomponent then mesh_name = editor_object.MeshID .. " - " .. mesh_namecomponent.GetName() end
                        end

                        local changed_params = display_edit_parameters(objectcomponent, compio_object, editor_object)
                        if changed_params then changed = true end

                        imgui.InputText("Mesh ID##meshid", mesh_name, 255, imgui.constant.InputTextFlags.ReadOnly)
                        imgui.SameLine()
                        if imgui.Button("\xef\x86\xb2 Set Mesh") then 
                            D.editor_data.elements.entityselector.filter_type = 0 -- mesh
                            D.editor_data.elements.entityselector.win_visible = true
                            runProcess(function()
                                waitSignal("Editor_EntitySelect_Finish")
                                editor_object.meshID = D.editor_data.elements.entityselector.selected_entity
                                local editdata = {
                                    entity = entity,
                                    type = "object",
                                    pre = {
                                        meshID = objectcomponent.MeshID,
                                    },
                                    post = deepcopy(editor_object)
                                }
                                build_edit_prestate(objectcomponent, compio_object, editdata.pre)
                                edit_execcmd("mod_comp", editdata)
                            end)
                        end

                        if input.Press(KEYBOARD_BUTTON_ENTER) then changed = true end

                        if changed then
                            local editdata = {
                                entity = entity,
                                type = "object",
                                pre = {
                                    meshID = objectcomponent.MeshID,
                                },
                                post = deepcopy(editor_object)
                            }
                            build_edit_prestate(objectcomponent, compio_object, editdata.pre)
                            edit_execcmd("mod_comp", editdata)
                        end

                        imgui.TreePop()
                    end
                    if not imgui.IsItemFocused() then 
                        editor_object.meshID = objectcomponent.GetMeshID()
                        build_edit_prestate(objectcomponent, compio_object, editor_object)
                    end
                end
                --

                -- EmitterComponent
                local emittercomponent = wiscene.Component_GetEmitter(entity)
                if emittercomponent then
                    local editor_emitter = compinspect.component.emitter

                    local ret_tree = imgui.TreeNode("Emitter Component")
                    if ret_tree then
                        local changed = false

                        local changed_params = display_edit_parameters(emittercomponent, compio_emitter, editor_emitter)
                        if changed_params then changed = true end

                        if input.Press(KEYBOARD_BUTTON_ENTER) then changed = true end

                        if changed then
                            local editdata = {
                                entity = entity,
                                type = "emitter",
                                pre = {},
                                post = deepcopy(editor_emitter)
                            }
                            build_edit_prestate(emittercomponent, compio_emitter, editdata.pre)
                            edit_execcmd("mod_comp", editdata)
                        end
                        imgui.TreePop()
                    end
                    if not imgui.IsItemFocused() then 
                        build_edit_prestate(emittercomponent, compio_emitter, editor_emitter) 
                    end
                end
                --

                -- LightComponent
                local lightcomponent = wiscene.Component_GetLight(entity)
                if lightcomponent then
                    local editor_light = compinspect.component.light
                    local ret_tree = imgui.TreeNode("Light Component")
                    --Init
                    if editor_light.Type == nil then editor_light.Type = lightcomponent.Type end
                    --
                    if ret_tree then
                        local changed = false
                        
                        local changed_params = display_edit_parameters(lightcomponent, compio_light, editor_light)
                        if changed_params then changed = true end
                        
                        local changed_set_shadow = false
                        editor_light.set_shadow = lightcomponent.IsCastShadow()
                        changed_set_shadow, editor_light.set_shadow = imgui.Checkbox("Cast Shadow##shadow", editor_light.set_shadow)
                        if changed_set_shadow then changed = true end

                        local changed_set_volumetric = false
                        editor_light.set_volumetric = lightcomponent.IsVolumetricsEnabled()
                        changed_set_volumetric, editor_light.set_volumetric = imgui.Checkbox("Contribute Volumetric##volumetric", editor_light.set_volumetric)
                        if changed_set_volumetric then changed = true end

                        if input.Press(KEYBOARD_BUTTON_ENTER) then changed = true end

                        if changed then
                            local editdata = {
                                entity = entity,
                                type = "light",
                                pre = {
                                    type = lightcomponent.Type,
                                    set_shadow = lightcomponent.IsCastShadow(),
                                    set_volumetric = lightcomponent.IsVolumetricsEnabled()
                                },
                                post = deepcopy(editor_light)
                            }
                            build_edit_prestate(lightcomponent, compio_light, editdata.pre)
                            edit_execcmd("mod_comp", editdata)
                        end
                        
                        imgui.TreePop()
                    end
                    if not imgui.IsItemFocused() then 
                        editor_light.Type = lightcomponent.Type,
                        build_edit_prestate(lightcomponent, compio_light, editor_light)

                    end
                end
                --

                -- MaterialComponent
                local materialcomponent = wiscene.Component_GetMaterial(entity)
                if materialcomponent then
                    local editor_material = compinspect.component.material

                    local ret_tree = imgui.TreeNode("Material Component")
                    if ret_tree then
                        local changed = false

                        local changed_params = display_edit_parameters(materialcomponent, compio_material, editor_material)
                        if changed_params then changed = true end

                        if input.Press(KEYBOARD_BUTTON_ENTER) then changed = true end

                        if changed then
                            local editdata = {
                                entity = entity,
                                type = "material",
                                pre = {},
                                post = deepcopy(editor_material)
                            }
                            build_edit_prestate(materialcomponent, compio_material, editdata.pre)
                            edit_execcmd("mod_comp", editdata)
                        end
                        imgui.TreePop()
                    end
                    if not imgui.IsItemFocused() then 
                        build_edit_prestate(materialcomponent, compio_material, editor_material) 
                    end
                end
                --

                -- RigidBodyComponent
                local rigidbodycomponent = wiscene.Component_GetRigidBodyPhysics(entity)
                if rigidbodycomponent then
                    local editor_rbody = compinspect.component.rigidbody

                    local ret_tree = imgui.TreeNode("Rigid Body Component")
                    if ret_tree then
                        local changed = false

                        local changed_params = display_edit_parameters(rigidbodycomponent, compio_rigidbody, editor_rbody)
                        if changed_params then changed = true end

                        if input.Press(KEYBOARD_BUTTON_ENTER) then changed = true end

                        if changed then
                            local editdata = {
                                entity = entity,
                                type = "rigidbody",
                                pre = {},
                                post = deepcopy(editor_rbody)
                            }
                            build_edit_prestate(rigidbodycomponent, compio_rigidbody, editdata.pre)
                            edit_execcmd("mod_comp", editdata)
                        end
                        imgui.TreePop()
                    end
                    if not imgui.IsItemFocused() then 
                        build_edit_prestate(rigidbodycomponent, compio_rigidbody, editor_rbody) 
                    end
                end
                --

                -- SoftBodyComponent
                local softbodycomponent = wiscene.Component_GetSoftBodyPhysics(entity)
                if softbodycomponent then
                    local editor_sbody = compinspect.component.softbody

                    local ret_tree = imgui.TreeNode("Soft Body Component")
                    if ret_tree then
                        local changed = false

                        local changed_params = display_edit_parameters(softbodycomponent, compio_softbody, editor_sbody)
                        if changed_params then changed = true end

                        if input.Press(KEYBOARD_BUTTON_ENTER) then changed = true end

                        if changed then
                            local editdata = {
                                entity = entity,
                                type = "softbody",
                                pre = {},
                                post = deepcopy(editor_rbody)
                            }
                            build_edit_prestate(softbodycomponent, compio_softbody, editdata.pre)
                            edit_execcmd("mod_comp", editdata)
                        end
                        imgui.TreePop()
                    end
                    if not imgui.IsItemFocused() then 
                        build_edit_prestate(softbodycomponent, compio_softbody, editor_sbody) 
                    end
                end
                --

                -- ForceFieldComponent
                local forcecomponent = wiscene.Component_GetForceField(entity)
                if forcecomponent then
                    local editor_force = compinspect.component.force

                    local ret_tree = imgui.TreeNode("Force Field Component")
                    if ret_tree then
                        local changed = false

                        local changed_params = display_edit_parameters(forcecomponent, compio_force, editor_force)
                        if changed_params then changed = true end

                        if input.Press(KEYBOARD_BUTTON_ENTER) then changed = true end

                        if changed then
                            local editdata = {
                                entity = entity,
                                type = "force",
                                pre = {},
                                post = deepcopy(editor_rbody)
                            }
                            build_edit_prestate(forcecomponent, compio_force, editdata.pre)
                            edit_execcmd("mod_comp", editdata)
                        end
                        imgui.TreePop()
                    end
                    if not imgui.IsItemFocused() then 
                        build_edit_prestate(forcecomponent, compio_force, editor_force) 
                    end
                end
                --

                -- WeatherComponent
                local weathercomponent = wiscene.Component_GetWeather(entity)
                if weathercomponent then
                    local editor_weather = compinspect.component.weather

                    local params_atmos = weathercomponent.AtmosphereParameters
                    local params_cloud = weathercomponent.VolumetricCloudParameters

                    local ret_tree = imgui.TreeNode("Weather Component")
                    if ret_tree then

                        local changed = false

                        display_edit_parameters(weathercomponent, compio_weather, editor_weather)

                        local ret_tree_atmos = imgui.TreeNode("Atmosphere Parameters")
                        if ret_tree_atmos then
                            display_edit_parameters(params_atmos, compio_weather_atmos, editor_weather.atmosphere)
                            imgui.TreePop()
                        end

                        local ret_tree_cloud = imgui.TreeNode("Cloud Parameters")
                        if ret_tree_cloud then
                            display_edit_parameters(params_cloud, compio_weather_cloud, editor_weather.cloud)
                            imgui.TreePop()
                        end
                        
                        if input.Press(KEYBOARD_BUTTON_ENTER) then changed = true end

                        if changed then
                            local editdata = {
                                entity = entity,
                                type = "weather",
                                pre = {
                                    atmosphere = {},
                                    cloud = {}
                                },
                                post = deepcopy(editor_weather)
                            }
                            build_edit_prestate(params_atmos, compio_weather_atmos, editdata.pre.atmosphere)
                            build_edit_prestate(params_cloud, compio_weather_cloud, editdata.pre.cloud)
                            build_edit_prestate(weathercomponent, compio_weather, editdata.pre)
                            edit_execcmd("mod_comp", editdata)
                        end
                        imgui.TreePop()
                    end
                    if not imgui.IsItemFocused() then 
                        build_edit_prestate(params_atmos, compio_weather_atmos, params_atmos)
                        build_edit_prestate(params_cloud, compio_weather_cloud, params_cloud)
                        build_edit_prestate(weathercomponent, compio_weather, editor_weather) 
                    end
                end
                --

                -- SoundComponent
                local soundcomponent = wiscene.Component_GetSound(entity)
                if soundcomponent then
                    local editor_sound = compinspect.component.sound

                    local ret_tree = imgui.TreeNode("Sound Component")
                    if ret_tree then
                        local changed = false

                        local changed_params = display_edit_parameters(soundcomponent, compio_sound, editor_sound)
                        if changed_params then changed = true end

                        local changed_set_loop = false
                        editor_sound.set_loop = soundcomponent.IsLooped()
                        changed_set_loop, editor_sound.set_loop = imgui.Checkbox("Loop Sound", editor_sound.set_loop)
                        if changed_set_loop then changed = true end

                        local changed_set_2d = false
                        editor_sound.set_2d = soundcomponent.IsDisable3D()
                        changed_set_2d, editor_sound.set_2d = imgui.Checkbox("2D sound", editor_sound.set_2d)
                        if changed_set_2d then changed = true end

                        if soundcomponent.IsPlaying() then
                            if imgui.Button("Stop") then 
                                editor_sound.play = false
                                changed = true
                            end
                        else
                            if imgui.Button("Play") then 
                                editor_sound.play = true
                                changed = true
                            end
                        end
                        
                        if input.Press(KEYBOARD_BUTTON_ENTER) then changed = true end

                        if changed then
                            local editdata = {
                                entity = entity,
                                type = "sound",
                                pre = {
                                    set_loop = soundcomponent.IsLooped(),
                                    set_2d = soundcomponent.IsDisable3D(),
                                    play = soundcomponent.IsPlaying()
                                },
                                post = deepcopy(editor_sound)
                            }
                            build_edit_prestate(soundcomponent, compio_sound, editdata.pre)
                            edit_execcmd("mod_comp", editdata)
                        end
                        imgui.TreePop()
                    end
                    if not imgui.IsItemFocused() then 
                        build_edit_prestate(soundcomponent, compio_sound, editor_sound)
                    end
                end
                --

                -- ColliderComponent
                local collidercomponent = wiscene.Component_GetCollider(entity)
                if collidercomponent then
                    local editor_collider = compinspect.component.collider

                    local ret_tree = imgui.TreeNode("Collider Component")
                    if ret_tree then
                        local changed = false

                        local changed_params = display_edit_parameters(collidercomponent, compio_collider, editor_collider)
                        if changed_params then changed = true end

                        if input.Press(KEYBOARD_BUTTON_ENTER) then changed = true end

                        if changed then
                            local editdata = {
                                entity = entity,
                                type = "collider",
                                pre = {},
                                post = deepcopy(editor_rbody)
                            }
                            build_edit_prestate(collidercomponent, compio_collider, editdata.pre)
                            edit_execcmd("mod_comp", editdata)
                        end
                        imgui.TreePop()
                    end
                    if not imgui.IsItemFocused() then 
                        build_edit_prestate(collidercomponent, compio_collider, editor_collider) 
                    end
                end
                --

                -- InstanceComponent
                local instancecomponent = scene.Component_GetInstance(entity)
                if instancecomponent then
                    local editor_instance = compinspect.component.instance

                    local ret_tree = imgui.TreeNode("Instance Component")
                    if ret_tree then
                        local changed = false

                        local changed_params = display_edit_parameters(instancecomponent, compio_instance, editor_instance)
                        if changed_params then changed = true end

                        if input.Press(KEYBOARD_BUTTON_ENTER) then changed = true end

                        if changed then
                            local editdata = {
                                entity = entity,
                                type = "instance",
                                pre = {
                                    Strategy = instancecomponent.Strategy,
                                    Type = instancecomponent.Type,
                                },
                                post = deepcopy(editor_instance)
                            }
                            build_edit_prestate(instancecomponent, compio_instance, editdata.pre)
                            edit_execcmd("mod_comp", editdata)
                        end
                        imgui.TreePop()
                    end
                    if not imgui.IsItemFocused() then 
                        build_edit_prestate(instancecomponent, compio_instance, editor_instance)
                    end
                end
                --

                if imgui.Button("               Add Component               ") then end -- TODO
            end
        end

        imgui.End()
    end
end

local drawimportwindow = function()
    local importwindow = D.editor_data.elements.importwindow

    if importwindow.win_visible then
        local sub_visible = false
        sub_visible, importwindow.win_visible = imgui.Begin("\xef\x82\x85 Import", importwindow.win_visible)
        if sub_visible then
            imgui.InputText("File##file", importwindow.file, 255, imgui.constant.InputTextFlags.ReadOnly)
            if importwindow.type == "WISCENE" then
                _, importwindow.opt_wiscene.import_as_instance = imgui.Checkbox("Import As Instance##wiscene_inst", importwindow.opt_wiscene.import_as_instance)
            end
            if imgui.Button("Import##proceed") then
                if importwindow.type == "WISCENE" then
                    Editor_LoadWiScene(importwindow.file, importwindow.opt_wiscene.import_as_instance)
                end
                importwindow.win_visible = false
            end
            imgui.End()
        end
    end
end

local drawentityselector = function()
    local entityselector = D.editor_data.elements.entityselector
    local scenegraphview = D.editor_data.elements.scenegraphview

    if entityselector.win_visible then
        local sub_visible = false
        sub_visible, entityselector.win_visible = imgui.Begin("\xef\x86\xb2 Entity Selector", entityselector.win_visible)
        if sub_visible then
            _, entityselector.search_string = imgui.InputText("##enttsel_searchstr", entityselector.search_string, 255)
            imgui.SameLine() imgui.Button("\xef\x85\x8e Search")
            
            if imgui.Button("Select") then 
                signal("Editor_EntitySelect_Finish")
                entityselector.win_visible = false
            end
            
            imgui.PushStyleVar(imgui.constant.StyleVar.ChildRounding, 5.0)
            local childflags = 0 | imgui.constant.WindowFlags.NoTitleBar
            local view_oblist = imgui.BeginChild("##listview", 0, 0, true, childflags)
            imgui.PopStyleVar()
            if view_oblist then
                if not scenegraphview.wait_update then
                    local entities_list = {}
                    if entityselector.filter_type == 0 then entities_list = scenegraphview.list.meshes end
                    if scenegraphview.filter_type == 1 then entities_list = scenegraphview.list.materials end
                    if scenegraphview.filter_type == 2 then entities_list = scenegraphview.list.animations end
                    if type(entities_list) ~= "nil" then
                        entityselector.selected_entity = Editor_DisplayEntityList(entities_list, entityselector.selected_entity, true)
                    end
                end 
            end
            imgui.EndChild()
        end
        imgui.End()
    end
end

local update_scenegraph = function()
    -- Update data only when the entity list changes
    if Editor_IsEntityListUpdated() then
        D.editor_data.elements.scenegraphview.list.objects = Editor_GetObjectList()
        D.editor_data.elements.scenegraphview.list.meshes = scene.Entity_GetMeshArray()
        D.editor_data.elements.scenegraphview.list.materials = wiscene.Entity_GetMaterialArray()
        D.editor_data.elements.scenegraphview.list.animations = wiscene.Entity_GetAnimationArray()
        D.editor_data.elements.scenegraphview.list.lights = wiscene.Entity_GetLightArray()
        D.editor_data.elements.scenegraphview.list.weathers = wiscene.Entity_GetWeatherArray()
        D.editor_data.elements.scenegraphview.list.instances = scene.Entity_GetInstanceArray()
    end

    D.editor_data.elements.scenegraphview.wait_update = false
end

local update_sysmenu_actions = function()
    local actions = D.editor_data.actions
    -- File menu actions
    if actions.resource_new then
        backlog_post("ACT_STUB: new resource not working")
        actions.resource_new = false
    end
    if actions.resource_rename then
        D.editor_data.elements.fmenu_rnres.win_visible = true
        actions.resource_rename = false
    end
    if actions.resource_save then
        backlog_post("ACT_STUB: save resource not working")
        actions.resource_save = false
    end
    --
    
    -- Add menu actions
    if actions.import_wiscene then
        filedialog(0,"Wicked Engine Scene","wiscene",function(data)
            local import_opt = D.editor_data.elements.importwindow
            import_opt.file = data.filepath
            import_opt.type = data.type
            import_opt.win_visible = true
        end)
        actions.import_wiscene = false
    end
    if actions.add_object then
        edit_execcmd("add_obj", {type = "object", name = "New Object"})
        actions.add_object = false
    end
    if actions.add_light then
        edit_execcmd("add_obj", {type = "light", name = "New Light"})
        actions.add_light = false
    end
    if actions.add_sound then
        edit_execcmd("add_obj", {type = "sound", name = "New Sound"})
        actions.add_sound = false
    end
    if actions.add_weather then
        edit_execcmd("add_obj", {type = "weather", name = "New Weather"})
        actions.add_weather = false
    end
    --
end

local update_navigation = function()
    local navigation = D.editor_data.navigation

    if Editor_UIFocused() == false then

        local camera_pos_delta = Vector()
        local camera_rot_delta = Vector()

        local move_spd = CAM_MOVE_SPD + 0
        if(input.Down(KEYBOARD_BUTTON_LSHIFT)) then move_spd = CAM_MOVE_SPD * D.editor_data.navigation.camera_speed_mul end
        
        -- Camera movement WASDQE
        if(input.Down(string.byte('W'))) then camera_pos_delta.Z = 1.0*move_spd end
        if(input.Down(string.byte('S'))) then camera_pos_delta.Z = -1.0*move_spd end
        if(input.Down(string.byte('A'))) then camera_pos_delta.X = -1.0*move_spd end
        if(input.Down(string.byte('D'))) then camera_pos_delta.X = 1.0*move_spd end
        if(input.Down(string.byte('Q'))) then camera_pos_delta.Y = -1.0*move_spd end
        if(input.Down(string.byte('E'))) then camera_pos_delta.Y = 1.0*move_spd end
        -- Camera rotation keyboard
        if(input.Down(KEYBOARD_BUTTON_UP)) then camera_rot_delta.X = -1.0*CAM_ROT_SPD end
        if(input.Down(KEYBOARD_BUTTON_DOWN)) then camera_rot_delta.X = 1.0*CAM_ROT_SPD end
        if(input.Down(KEYBOARD_BUTTON_LEFT)) then camera_rot_delta.Y = -1.0*CAM_ROT_SPD end
        if(input.Down(KEYBOARD_BUTTON_RIGHT)) then camera_rot_delta.Y = 1.0*CAM_ROT_SPD end
        -- Get rotated movement
        local camera_rot_matrix = matrix.Rotation(navigation.camera_rot)
        camera_pos_delta = vector.Transform(camera_pos_delta, camera_rot_matrix)

        -- Apply rotation delta
        navigation.camera_rot.X = navigation.camera_rot.X + camera_rot_delta.X
        navigation.camera_rot.Y = navigation.camera_rot.Y + camera_rot_delta.Y
        -- Apply movement delta
        navigation.camera_pos.Z = navigation.camera_pos.Z + camera_pos_delta.Z
        navigation.camera_pos.X = navigation.camera_pos.X + camera_pos_delta.X
        navigation.camera_pos.Y = navigation.camera_pos.Y + camera_pos_delta.Y

        -- Camera transform update
        navigation.camera_transform.ClearTransform()
        navigation.camera_transform.Translate(navigation.camera_pos)
        navigation.camera_transform.Rotate(navigation.camera_rot)
        navigation.camera_transform.UpdateTransform()
        navigation.camera.TransformCamera(navigation.camera_transform)
        navigation.camera.UpdateCamera()

        if(input.Press(MOUSE_BUTTON_RIGHT)) then
            local picked = Editor_PickEntity()
            D.editor_data.elements.scenegraphview.selected_entity = picked
        end

    end
end

local update_editaction = function()
    if (D.editor_data.elements.scenegraphview.selected_entity > 0) and (input.Press(KEYBOARD_BUTTON_DELETE)) then
        edit_execcmd("del_obj", {entity = D.editor_data.elements.scenegraphview.selected_entity + 0})
    end
    if (input.Down(KEYBOARD_BUTTON_LCONTROL) and (input.Press(string.byte('Z')))) then
        if(input.Down(KEYBOARD_BUTTON_LSHIFT)) then
            edit_redocmd()
        else
            edit_undocmd()
        end
    end
end

runProcess(function()
    if not Script_Initialized(script_pid()) then
        if D.editor_data.actions.command_list == nil then
            D.editor_data.actions.command_list = {}
        end
        edit_execcmd("init")
        Editor_SetGridHelper(true)
    end

    while true do
        update_scenegraph()
        update_sysmenu_actions()
        update_navigation()
        update_editaction()
        update()
    end
end)

runProcess(function()
    while true do
        imgui_draw()
        drawtopbar()
        drawmenubardialogs()
        drawsceneexp()
        drawcompinspect()
        drawscenegraphview()
        drawimportwindow()
        drawentityselector()
        if D.editor_data.elements.helper_demo.win_visible then
            imgui.ShowDemoWindow()
        end
    end
end)