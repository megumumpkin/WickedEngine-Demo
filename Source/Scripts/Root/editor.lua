D("editor_data",{
    elements = {
        resexp = {
            win_visible = false,
            updated = false,
            input = "",
            selected_file = "",
            selected_resname = "",
            selected_subinstance = "",
            directory_stack = {},
            directory_list = {},
            instance_popup = false,
            subinstance_list = {}
        },
        scenegraphview = {
            win_visible = true,
            filter_selected = 0,
            selected_entity = 0,
            wait_update = false,
            force_refresh = false,
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
                layers = {},
                object = {},
                emitter = {},
                hairparticle = {},
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
                instance = {},
                stream = {},
                mesh = {},
                material = {},
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
            target_entity = 0,
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
        resource_open = false,
        resource_instance = false,
        -- Add menu actions
        link_dcc = false,
        import_wiscene = false,
        import_gltf = false,
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
    {"Name", "Name", "text"}
}

local compio_transform = {
    {"Translation_local", "Local Position", "float3"},
    {"Rotation_local", "Local Rotation", "float4"},
    {"Scale_local", "Local Scale", "float3"},
}

local compio_object = {
    {"MeshID", "Mesh ID", "entity", { type = "object", setmode = 1 }},
    {"Color", "Color", "float4"},
    {"EmissiveColor", "EmissiveColor", "float4"},
    {"CascadeMask", "CascadeMask", "int"},
    {"RendertypeMask", "RendertypeMask", "int"},
}

local compio_material_texturenames = {
    "Base Color Map",
    "Normal Map",
    "Surface Map",
    "Emissive Map",
    "Displacement Map",
    "Occlusion Map",
    "Transmission Map",
    "Sheen Color Map",
    "Sheen Roughness Map",
    "Clearcoat Map",
    "Clearcoat Roughness Map",
    "Clearcoat Normal Map",
    "Specular Map",
}

local compio_material = {
    {"ShaderType", "Shader Type", "combo", { choices = "PBR\0PBR - Planar Reflection\0PBR - Anisotropic\0Water\0Cartoon\0Unlit\0PBR - Cloth\0PBR - Clearcoat\0PBR - Cloth Clearcoat\0" }},
    {"BaseColor", "Base Color", "float4"},
	{"EmissiveColor", "Emissive Color", "float4"},
	{"EngineStencilRef", "Engine Stencil Ref", "int"},
	{"UserStencilRef", "User Stencil Ref", "int"},
	{"UserBlendMode", "User Blend Mode", "int"},
	{"SpecularColor", "SpecularColor", "float4"},
	{"SubsurfaceScattering", "Subsurface Scattering", "float4"},
	{"TexMulAdd", "Texture Color Multiply", "float4"},
	{"Roughness", "Roughness", "float"},
	{"Reflectance", "Reflectance", "float"},
	{"Metalness", "Metalness", "float"},
	{"NormalMapStrength", "Normal Map Strength", "float"},
	{"ParallaxOcclusionMapping", "Parallax Occlusion Mapping", "float"},
	{"DisplacementMapping", "Displacement Mapping", "float"},
	{"Refraction", "Refraction", "float"},
	{"Transmission", "Transmission", "float"},
	{"AlphaRef", "Alpha Ref", "float"},
	{"SheenColor", "Sheen Color", "float3"},
	{"SheenRoughness", "Sheen Roughness", "float"},
	{"Clearcoat", "Clearcoat", "float"},
	{"ClearcoatRoughness", "Clearcoat Roughness", "float"},
	{"TexAnimDirection", "Texture Anim Direction", "float2"},
	{"TexAnimFrameRate", "Texture Anim FrameRate", "float"},
	{"texAnimElapsedTime", "Texture Anim Elapsed Time", "float"},
	{"customShaderID", "Custom Shader ID", "int"},
}

local compio_mesh = {
    {"TessellationFactor", "Tesselation Factor", "float"},
    {"ArmatureID", "Armature ID", "int"},
    {"SubsetsPerLOD", "Subsets Per LOD", "int"},
}

local compio_emitter = {
    {"Mass", "Mass", "float"},
    {"Velocity", "Velocity", "float3"},
    {"Gravity", "Gravity", "float3"},
    {"Drag", "Drag", "float"},
    {"Restitution", "Restitution", "float"},

    {"EmitCount", "Emit Count", "float"},
	{"Size", "Size", "float"},
	{"Life", "Live", "float"},
	{"NormalFactor", "Normal Factor", "float"},
	{"Randomness", "Randomness", "float"},
	{"LifeRandomness", "Life Randomness", "float"},
	{"ScaleX", "Scale X", "float"},
	{"ScaleY", "Scale Y", "float"},
	{"Rotation", "Rotation", "float"},
	{"MotionBlurAmount", "Motion Blur Amount", "float"},

    {"SPH_h", "SPH_h", "float"},
    {"SPH_K", "SPH_K", "float"},
    {"SPH_p0", "SPH_p0", "float"},
    {"SPH_e", "SPH_e", "float"},

    {"SpriteSheet_Frames_X", "SpriteSheet Frames X Count", "int"},
    {"SpriteSheet_Frames_Y", "SpriteSheet Frames Y Count", "int"},
    {"SpriteSheet_Frame_Count", "SpriteSheet Frame Count", "int"},
    {"SpriteSheet_Frame_Start", "SpriteSheet Frame Start", "int"},
    {"SpriteSheet_Framerate", "SpriteSheet Framerate", "float"},
}

local compio_hairparticle = {
    {"StrandCount", "Strand Count", "int"},
    {"SegmentCount", "Segment Count", "int"},
    {"RandomSeed", "Random Seed", "int"},
    {"Length", "Length", "float"},
    {"Stiffness", "Stiffness", "float"},
    {"Randomness", "Randomess", "float"},
    {"ViewDistance", "ViewDistance", "float"},
    {"SpriteSheet_Frames_X", "SpriteSheet Frames X Count", "int"},
    {"SpriteSheet_Frames_Y", "SpriteSheet Frames Y Count", "int"},
    {"SpriteSheet_Frame_Count", "SpriteSheet Frame Count", "int"},
    {"SpriteSheet_Frame_Start", "SpriteSheet Frame Start", "int"},
}

local compio_light = {
    {"Type", "Type", "combo", { choices = "Directional\0Point\0Spot\0" }},
    {"Range", "Range", "float"},
    {"Intensity", "Intensity", "float"},
    {"Color", "Color", "float3"},
    {"OuterConeAngle", "Outer Cone Angle", "float"},
    {"InnerConeAngle", "Inner Cone Angle", "float"},
}

local compio_rigidbody = {
    {"Shape", "Shape", "combo", { choices = "Box\0Sphere\0Capsule\0Convex Hull\0Triangle Mesh\0" }},
    {"Mass", "Mass", "float"},
    {"Friction", "Friction", "float"},
    {"Restitution", "Restitution", "float"},
    {"LinearDamping", "LinearDamping", "float"},
    {"AngularDamping", "AngularDamping", "float"},
    {"BoxParams_HalfExtents", "BoxParams_HalfExtents", "float3"},
    {"SphereParams_Radius", "SphereParams_Radius", "float"},
    {"CapsuleParams_Radius", "CapsuleParams_Radius", "float"},
    {"CapsuleParams_Height", "CapsuleParams_Height", "float"},
    {"TargetMeshLOD", "TargetMeshLOD", "int"},
}

local compio_softbody = {
    {"Mass", "Mass", "float"},
    {"Friction", "Friction", "float"},
    {"Restitution", "Restitution", "float"},
}

local compio_forcefield = {
    {"Type", "Type", "combo", { choices = "Point\0Plane\0" }},
    {"Gravity", "Gravity", "float"},
    {"Range", "Range", "float"},
}

local compio_weather_atmos = {
    {"bottomRadius", "Bottom Radius", "float"},
	{"topRadius", "Top Radius", "float"},
	{"planetCenter", "Planet Center", "float3"},
	{"rayleighDensityExpScale", "Rayleigh Density Exp Scale", "float"},
	{"rayleighScattering", "Rayleigh Scattering", "float3"},
	{"mieDensityExpScale", "Mie Density Exp Scale", "float"},
	{"mieScattering", "Mie Scattering", "float3"},
	{"mieExtinction", "Mie Extinction", "float3"},
	{"mieAbsorption", "Mie Absorption", "float3"},
	{"miePhaseG", "Mie G Phase", "float"},
	{"absorptionDensity0LayerWidth", "Absorption Density Layer 0 - Width", "float"},
	{"absorptionDensity0ConstantTerm", "Absorption Density Layer 0 - Constant Term", "float"},
	{"absorptionDensity0LinearTerm", "Absorption Density Layer 0 - Linear Term", "float"},
	{"absorptionDensity1ConstantTerm", "Absorption Density Layer 1 - Constant Term", "float"},
	{"absorptionDensity1LinearTerm", "Absorption Density Layer 1 - Linear Term", "float"},
	{"absorptionExtinction", "Absorption Extinction", "float3"},
	{"groundAlbedo", "Ground Albedo", "float3"},
}

local compio_weather_cloud = {
    {"Albedo", "Albedo Color", "float3"},
	{"CloudAmbientGroundMultiplier", "Cloud Ambient Ground Multiplier", "float"},
	{"ExtinctionCoefficient", "Extinction Coefficient", "float3"},
	{"HorizonBlendAmount", "Horizon Blend Amount", "float"},
	{"HorizonBlendPower", "Horizon Blend Power", "float"},
	{"WeatherDensityAmount", "Weather Density Amount", "float"},
	{"CloudStartHeight", "Cloud Start Height", "float"},
	{"CloudThickness", "Cloud Thickness", "float"},
	{"SkewAlongWindDirection", "Skew Along Wind Direction", "float"},
	{"TotalNoiseScale", "Total Noise Scale", "float"},
	{"DetailScale", "Detail Scale", "float"},
	{"WeatherScale", "Weather Scale", "float"},
	{"CurlScale", "Curl Scale", "float"},
	{"DetailNoiseModifier", "Detail Noise Modifier", "float"},
	{"TypeAmount", "Type Amount", "float"},
	{"TypeMinimum", "Type Minimum", "float"},
	{"AnvilAmount", "Anvil Amount", "float"},
	{"AnvilOverhangHeight", "Anvil Overhang Height", "float"},
	{"AnimationMultiplier", "Animation Multiplier", "float"},
	{"WindSpeed", "Wind Speed", "float"},
	{"WindAngle", "Wind Angle", "float"},
	{"WindUpAmount", "Wind Up Amount", "float"},
	{"CoverageWindSpeed", "Coverage Wind Speed", "float"},
	{"CoverageWindAngle", "Coverage Wind Angle", "float"},
	{"CloudGradientSmall", "Cloud Gradient Small", "float3"},
	{"CloudGradientMedium", "Cloud Gradient Medium", "float3"},
	{"CloudGradientLarge", "Cloud Gradient Large", "float3"},
}

local compio_weather = {
    {"sunColor", "Sun Color" , "float3"},
    {"sunDirection", "Sun Direction"  , "float3"},
    {"skyExposure", "Sky Exposure" , "float"},
    {"horizon", "Horizon Color" , "float3"},
    {"zenith", "Zenith Color" , "float3"},
    {"ambient", "Ambient Color" , "float3"},
    {"fogStart", "Fog Start" , "float"},
    {"fogEnd", "Fog End" , "float"},
    {"fogHeightStart", "Fog Height Start" , "float"},
    {"fogHeightEnd", "Fog Height End" , "float"},
    {"windDirection", "Wind Direction" , "float3"},
    {"windRandomness", "Wind Randomness" , "float"},
    {"windWaveSize", "Wind Wave Size" , "float"},
    {"windSpeed", "Wind Speed" , "float"},
    {"stars", "Star Density" , "float"}
}

local compio_sound = {
    {"Filename", "Filename", "text"},
    {"Volume", "Volume", "float"}
}

local compio_collider = {
    {"Shape", "Shape", "combo", { choices = "Sphere\0Capsule\0Plane\0" }},
    {"Radius", "Radius", "float"},
    {"Offset", "Offset", "float3"},
    {"Tail", "Tail", "float3"},
}

local compio_instance = {
    {"File", "File", "text"},
    {"EntityName", "Subtarget Entity Name", "text"},
    {"Strategy", "Loading Strategy", "combo", { choices = "Direct\0Instance\0Preload\0" }},
    {"Type", "Type", "combo", { choices = "Default\0Library\0" }},
    {"Lock", "Lock For Editing", "check"},
}

local compio_stream = {
    {"ExternalSubstitute", "External Substitute Model", "text"},
    {"Zone", "Stream Zone", "aabb_center"}
}

local object_creators = {
    {"\xef\x86\xb2 Add Object", {type = "object", name = "New Object"}},
    {"\xef\x83\xab Add Light", {type = "light", name = "New Light"}},
    {"\xef\x80\xa8 Add Sound", {type = "sound", name = "New Sound"}},
    {"\xef\x81\xad Add Emitter", {type = "emitter", name = "New Emitter"}},
    {"\xef\x93\x98 Add HairParticle", {type = "hairparticle", name = "New Hair Particle System"}},
    {"\xef\x9b\x84 Add Weather", {type = "weather", name = "New Weather"}},
    {"\xef\x87\x80 Add Instance", {type = "instance", name = "New Instance"}},
    {"\xef\x95\xb6 Add Material", {type = "material", name = "New Material"}}
}

local component_creators = {
    {"\xef\x86\xb2 Add Name", "name"},
    {"\xef\x86\xb2 Add Transform", "transform"},
    {"\xef\x86\xb2 Add Layer", "layer"},
    {"\xef\x86\xb2 Add Object", "object"},
    {"\xef\x83\xab Add Light", "light"},
    {"\xef\x80\xa8 Add Sound", "sound"},
    {"\xef\x95\xb6 Add Material", "material"},
    {"\xef\x81\xad Add Emitter", "emitter"},
    {"\xef\x93\x98 Add HairParticle", "hairparticle"},
    {"\xef\x9b\x84 Add Weather", "weather"},
    {"\xef\x87\x80 Add Instance", "instance"},
    {"\xef\x87\x80 Add Stream", "stream"},
}

local entityselector_modestring = {
    "Mesh",
    "Material",
    "Animation"
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

local component_set_stream = function(component, editdata)
    component.ExternalSubstitute = editdata.ExternalSubstitute
    component.Zone = editdata.Zone
end

local edit_execcmd = function(command, extradata, holdout)
    if command == "add_obj" then
        if type(extradata) == "table" then
            if extradata.entity == nil then
                extradata.entity = CreateEntity()
            end
            
            local entity = extradata.entity
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
            if extradata.type == "emitter" then
                local transform = wiscene.Component_CreateTransform(entity)
                local layer = wiscene.Component_CreateLayer(entity)
                local material = wiscene.Component_CreateMaterial(entity)
                local emitter = wiscene.Component_CreateEmitter(entity)
            end
            if extradata.type == "hairparticle" then
                local transform = wiscene.Component_CreateTransform(entity)
                local layer = wiscene.Component_CreateLayer(entity)
                local material = wiscene.Component_CreateMaterial(entity)
                local hairparticle = wiscene.Component_CreateHairParticleSystem(entity)
            end
            if extradata.type == "weather" then
                local weather = wiscene.Component_CreateWeather(entity)
                weather.SetRealisticSky(true)
                weather.SetVolumetricClouds(true)
            end
            if extradata.type == "material" then
                local material = wiscene.Component_CreateMaterial(entity)
            end
            if extradata.type == "instance" then
                local transform = wiscene.Component_CreateTransform(entity)
                local layer = wiscene.Component_CreateLayer(entity)
                local instance = scene.Component_CreateInstance(entity)
                instance.Lock = true
            end
        end
    end

    if command == "add_comp" then
        if type(extradata) == "table" then
            if extradata.type == "name" then wiscene.Component_CreateName(extradata.entity) end
            if extradata.type == "transform" then wiscene.Component_CreateTransform(extradata.entity) end
            if extradata.type == "layer" then wiscene.Component_CreateLayer(extradata.entity) end
            if extradata.type == "object" then wiscene.Component_CreateObject(extradata.entity) end
            if extradata.type == "light" then wiscene.Component_CreateLight(extradata.entity) end
            if extradata.type == "sound" then wiscene.Component_CreateSound(extradata.entity) end
            if extradata.type == "material" then wiscene.Component_CreateMaterial(extradata.entity) end
            if extradata.type == "emitter" then wiscene.Component_CreateEmitter(extradata.entity) end
            if extradata.type == "hairparticle" then wiscene.Component_CreateHairParticleSystem(extradata.entity) end
            if extradata.type == "weather" then 
                local weather wiscene.Component_CreateWeather(extradata.entity) 
                weather.SetRealisticSky(true)
                weather.SetVolumetricClouds(true)
            end
            if extradata.type == "instance" then 
                local instance = scene.Component_CreateInstance(extradata.entity)
                instance.Lock = true
            end
            if extradata.type == "stream" then 
                local initial_bounds = AABB()
                initial_bounds.Max = Vector(1,1,1)
                initial_bounds.Min = Vector(-1,-1,-1)
                scene.Entity_SetStreamable(extradata.entity, true, initial_bounds) 
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
                if transformcomponent then 
                    component_set_transform(transformcomponent, extradata.post) 
                    transformcomponent.SetDirty(true)
                    transformcomponent.UpdateTransform()
                end
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
            if extradata.type == "hairparticle" then
                local hairparticlecomponent = wiscene.Component_GetHairParticleSystem(extradata.entity)
                if hairparticlecomponent then component_set_generic(hairparticlecomponent, extradata.post) end
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
                if instancecomponent then component_set_generic(instancecomponent, extradata.post) end
            end
            if extradata.type == "stream" then
                local streamcomponent = scene.Component_GetStream(extradata.entity)
                if streamcomponent then component_set_stream(streamcomponent, extradata.post) end
            end
        end
    end

    if command == "del_obj" then
        if holdout == nil then
            extradata.index = #D.editor_data.actions.command_list
            Editor_StashDeletedEntity(extradata.entity, extradata.index)
        end
        local instance = scene.Component_GetInstance(extradata.entity)
        if instance then
            instance.Unload()
        end
        wiscene.Entity_Remove(extradata.entity)
    end

    if command == "del_comp" then
        if type(extradata) == "table" then
            if extradata.type == "name" then wiscene.Component_RemoveName(extradata.entity) end
            if extradata.type == "transform" then wiscene.Component_RemoveTransform(extradata.entity) end
            if extradata.type == "layer" then wiscene.Component_RemoveLayer(extradata.entity) end
            if extradata.type == "object" then wiscene.Component_RemoveObject(extradata.entity) end
            if extradata.type == "light" then wiscene.Component_RemoveLight(extradata.entity) end
            if extradata.type == "sound" then wiscene.Component_RemoveSound(extradata.entity) end
            if extradata.type == "material" then wiscene.Component_RemoveMaterial(extradata.entity) end
            if extradata.type == "emitter" then wiscene.Component_RemoveEmitter(extradata.entity) end
            if extradata.type == "hairparticle" then wiscene.Component_RemoveHairParticleSystem(extradata.entity) end
            if extradata.type == "weather" then wiscene.Component_RemoveWeather(extradata.entity) end
            if extradata.type == "instance" then scene.Component_RemoveInstance(extradata.entity) end
            if extradata.type == "stream" then scene.Entity_SetStreamable(extradata.entity, false) end
        end
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

function editor_hook_execcmd(command, editdata, holdout)
    edit_execcmd(command,editdata)
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
    if command == "add_comp" then
        if type(extradata) == "table" then
            if extradata.type == "name" then wiscene.Component_RemoveName(extradata.entity) end
            if extradata.type == "transform" then wiscene.Component_RemoveTransform(extradata.entity) end
            if extradata.type == "layer" then wiscene.Component_RemoveLayer(extradata.entity) end
            if extradata.type == "object" then wiscene.Component_RemoveObject(extradata.entity) end
            if extradata.type == "light" then wiscene.Component_RemoveLight(extradata.entity) end
            if extradata.type == "sound" then wiscene.Component_RemoveSound(extradata.entity) end
            if extradata.type == "material" then wiscene.Component_RemoveMaterial(extradata.entity) end
            if extradata.type == "emitter" then wiscene.Component_RemoveEmitter(extradata.entity) end
            if extradata.type == "hairparticle" then wiscene.Component_RemoveHairParticleSystem(extradata.entity) end
            if extradata.type == "weather" then wiscene.Component_RemoveWeather(extradata.entity) end
            if extradata.type == "instance" then scene.Component_RemoveInstance(extradata.entity) end
            if extradata.type == "stream" then scene.Entity_SetStreamable(extradata.entity, false) end
        end
    end
    if command == "del_comp" then
        if type(extradata) == "table" then
            if extradata.type == "name" then wiscene.Component_CreateName(extradata.entity) end
            if extradata.type == "transform" then wiscene.Component_CreateTransform(extradata.entity) end
            if extradata.type == "layer" then wiscene.Component_CreateLayer(extradata.entity) end
            if extradata.type == "object" then wiscene.Component_CreateObject(extradata.entity) end
            if extradata.type == "light" then wiscene.Component_CreateLight(extradata.entity) end
            if extradata.type == "sound" then wiscene.Component_CreateSound(extradata.entity) end
            if extradata.type == "material" then wiscene.Component_CreateMaterial(extradata.entity) end
            if extradata.type == "emitter" then wiscene.Component_CreateEmitter(extradata.entity) end
            if extradata.type == "hairparticle" then wiscene.Component_CreateHairParticleSystem(extradata.entity) end
            if extradata.type == "weather" then 
                local weather = wiscene.Component_CreateWeather(extradata.entity) 
                weather.SetRealisticSky(true)
                weather.SetVolumetricClouds(true)
            end
            if extradata.type == "instance" then 
                local instance = scene.Component_CreateInstance(extradata.entity)
                instance.Lock = true
            end
            if extradata.type == "stream" then scene.Entity_SetStreamable(extradata.entity, true) end
        end
    end
    if (command == "mod_comp") or (command == "del_comp") then
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
            if transformcomponent then 
                component_set_transform(transformcomponent, extradata.pre) 
                transformcomponent.SetDirty(true)
                transformcomponent.UpdateTransform()
            end
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
        if extradata.type == "hairparticle" then
            local hairparticlecomponent = wiscene.Component_GetHairParticleSystem(extradata.entity)
            if hairparticlecomponent then component_set_generic(hairparticlecomponent, extradata.pre) end
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
            if instancecomponent then component_set_generic(instancecomponent, extradata.pre) end
        end
        if extradata.type == "instance" then
            local instancecomponent = scene.Component_GetInstance(extradata.entity)
            if instancecomponent then component_set_generic(instancecomponent, extradata.pre) end
        end
        if extradata.type == "stream" then
            local streamcomponent = scene.Component_GetStream(extradata.entity)
            if streamcomponent then component_set_stream(streamcomponent, extradata.pre) end
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
            local actions = D.editor_data.actions
            actions.resource_new = imgui.MenuItem("\xee\x93\xae New Resource", nil, actions.resource_new)
            actions.resource_rename = imgui.MenuItem("\xef\x81\x84 Rename Resource", nil, actions.resource_rename)
            actions.resource_save = imgui.MenuItem("\xef\x83\x87 Save Resource", nil, actions.resource_save)
            imgui.EndPopup()
        end

        if imgui.BeginPopupContextWindow("MBIM") then
            local actions = D.editor_data.actions
            actions.link_dcc = imgui.MenuItem("\xef\x83\x81 Create DCC Link", nil, actions.link_dcc)
            actions.import_wiscene = imgui.MenuItem("\xee\x92\xb8 Import WiScene", nil, actions.import_wiscene)
            actions.import_gltf = imgui.MenuItem("\xee\x92\xb8 Import GLTF", nil, actions.import_gltf)
            for _, creator in ipairs(object_creators) do
                local label = creator[1]
                local packet = creator[2]

                local act = false
                act = imgui.MenuItem(label, nil, act)
                if act then edit_execcmd("add_obj", packet) end
            end
            imgui.EndPopup()
        end

        if imgui.BeginPopupContextWindow("MBWM") then
            local elements = D.editor_data.elements
            ret, elements.scenegraphview.win_visible = imgui.MenuItem_4("\xef\xa0\x82 Scene Graph Viewer","", elements.scenegraphview.win_visible)
            ret, elements.compinspect.win_visible = imgui.MenuItem_4("\xef\x82\x85 Component Inspector","", elements.compinspect.win_visible)
            ret, elements.helper_demo.win_visible = imgui.MenuItem_4("\xef\x8b\x90 IMGUI Demo Window","", elements.helper_demo.win_visible)
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
        if resexp.updated == false then
            resexp.directory_list = Editor_ListDirectory()
            resexp.updated = true
        end

        local sub_visible = false
        sub_visible, resexp.win_visible = imgui.Begin("\xef\x86\xb2 Scene Explorer", resexp.win_visible)
        if sub_visible then
            _, resexp.input = imgui.InputText("##resexp_sin", resexp.input, 255)
            imgui.SameLine() imgui.Button("\xef\x85\x8e Search")

            imgui.PushStyleVar(imgui.constant.StyleVar.ChildRounding, 5.0)
            local childflags = 0 | imgui.constant.WindowFlags.NoTitleBar
            local view_oblist = imgui.BeginChild("##listview", 0, 0, true, childflags)
            imgui.PopStyleVar()

            local containersize = imgui.GetContentRegionAvail()
            local max_sameline = math.floor(containersize.x / 100) 

            local item_counter = 1
            local sameline_count = 1

            -- Display previews of scenes here!
            -- if Editor_ImguiImageButton("Suzu", 100.0, 100.0) then end
            for path, filelist in pairs(resexp.directory_list) do
                local dirstack = {}
                for stack in string.gmatch(path, "([^,]+)/") do table.insert(dirstack,stack) end
                for _, file in ipairs(filelist) do
                    if(dirstack[1] == "Scene") then
                        local thumb_path = "Data/Editor/Thumb"
                        if #dirstack > 2 then
                            for idx = 2, #dirstack, 1 do thumb_path = thumb_path .. "/" .. dirstack[idx] end
                        end
                        local thumbname_split = {}
                        for nstack in string.gmatch(file, "([^,]+)%.") do table.insert(thumbname_split, nstack) end
                        
                        if (sameline_count <= max_sameline) and (item_counter > 1) then 
                            imgui.SameLine()
                        else
                            sameline_count = 1
                        end
                        
                        if Editor_ImguiImageButton(thumb_path .. "/" .. thumbname_split[1] .. ".png", 100, 100) then
                            resexp.selected_resname = thumbname_split[1]
                            resexp.selected_file = "Data/" .. path .. file
                            imgui.OpenPopup("REIM")
                        end
                        if imgui.IsItemHovered() then
                            imgui.SetTooltip(thumbname_split[1])
                        end
                        sameline_count = sameline_count + 1
                        item_counter = item_counter + 1
                    end
                end
            end

            if imgui.BeginPopupContextWindow("REIM") then
                local actions = D.editor_data.actions
                actions.resource_open = imgui.MenuItem("Open", nil, actions.resource_open)
                resexp.instance_popup = imgui.MenuItem("Create Instance", nil, resexp.instance_popup)
                imgui.EndPopup()
            end

            if resexp.instance_popup then
                imgui.OpenPopup("REINST")
                resexp.subinstance_list = Editor_FetchSubInstanceNames("Data/Editor/Instances/" .. resexp.selected_resname .. ".instlist")
                resexp.instance_popup = false
            end

            if imgui.BeginPopupContextWindow("REINST") then
                local actions = D.editor_data.actions
                local act_inst_scene = false
                act_inst_scene = imgui.MenuItem("Full Scene", nil, act_inst_scene)
                if act_inst_scene then
                    resexp.selected_subinstance = ""
                    actions.resource_instance = true
                end
                for _, entityname in ipairs(resexp.subinstance_list) do
                    local act_inst_entt = false
                    act_inst_entt = imgui.MenuItem("Entity - " .. entityname, nil, act_inst_entt)
                    if act_inst_entt then
                        resexp.selected_subinstance = entityname
                        actions.resource_instance = true
                    end
                end
                imgui.EndPopup()
            end

            imgui.EndChild()
        end

        imgui.End()
    else
        resexp.updated = false
    end
end

function Editor_DisplayEntityList(entities_list, selector, holdout)
    for _, entity in pairs(entities_list) do
        local name = "entity-" .. entity

        local nameComponent = wiscene.Component_GetName(entity)
        if nameComponent then name = nameComponent.Name end

        local flag = imgui.constant.TreeNodeFlags.Leaf | imgui.constant.TreeNodeFlags.SpanAvailWidth

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

        local flag = imgui.constant.TreeNodeFlags.SpanFullWidth
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

    for _, data in ipairs(parameter_list) do
        local key = data[1]
        local label = data[2]
        local type = data[3]
        local extradata = data[4]

        -- Init
        if edit_store[key] == nil then edit_store[key] = component[key] end
        -- 
        

        if type == "int" then
            _, edit_store[key] = imgui.InputInt(label, edit_store[key])
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
            ret, edit_store[key] = imgui.Checkbox(label, edit_store[key])
            if ret then changed = true end
        end

        if (type == "combo") and (extradata ~= nil) then
            local ret = false
            ret, edit_store[key] = imgui.Combo(label, edit_store[key], extradata.choices)
            if ret then changed = true end
        end

        if (type == "entity") and (extradata ~= nil) then
            local entity_name = edit_store[key] .. " - NO ENTITY"
            if edit_store[key] > 0 then
                local name_get = wiscene.Component_GetName(edit_store[key])
                if name_get then entity_name = edit_store[key] .. " - " .. name_get.GetName() end
            end
            imgui.InputText(label, entity_name, 255, imgui.constant.InputTextFlags.ReadOnly)
            imgui.SameLine()
            if imgui.Button("Set " .. entityselector_modestring[extradata.setmode]) then
                local selector = D.editor_data.elements.entityselector
                
                selector.target_entity = deepcopy(D.editor_data.elements.scenegraphview.selected_entity)
                selector.filter_type = extradata.setmode
                selector.win_visible = true

                runProcess(function()
                    waitSignal("Editor_EntitySelect_Finish")
                    local editdata = {
                        entity = selector.target_entity,
                        type = extradata.type,
                        pre = {},
                        post = {}
                    }
                    editdata.pre[key] = edit_store[key]
                    editdata.post[key] = selector.selected_entity
                    edit_execcmd("mod_comp", editdata)
                end)
            end
        end

        if (type == "aabb_center") then
            local zonecenter = edit_store[key].GetCenter()
            local zoneextent = edit_store[key].GetHalfExtents()
            _, zonecenter.X, zonecenter.Y, zonecenter.Z = imgui.InputFloat3(label .. " : Center", zonecenter.X, zonecenter.Y, zonecenter.Z, "%.12f")
            _, zoneextent.X, zoneextent.Y, zoneextent.Z = imgui.InputFloat3(label .. " : Extent", zoneextent.X, zoneextent.Y, zoneextent.Z, "%.12f")

            edit_store[key].Max = Vector(zonecenter.X + zoneextent.X, zonecenter.Y + zoneextent.Y, zonecenter.Z + zoneextent.Z)
            edit_store[key].Min = Vector(zonecenter.X - zoneextent.X, zonecenter.Y - zoneextent.Y, zonecenter.Z - zoneextent.Z)

            DrawBox(edit_store[key].GetAsBoxMatrix(),Vector(0,0.2,1,1))
        end
    end

    return changed
end

local build_edit_prestate = function(component, parameter_list, pre_storage)
    for _, data in pairs(parameter_list) do
        local key = data[1]
        pre_storage[key] = component[key]
    end
end

local drawcomp = function(tree_title, act_name, entity, component, compio, editor, custom_io, custom_def)
    if component then
        local ret_tree = imgui.TreeNode(tree_title)
        if ret_tree then
            local changed = false

            local changed_compio = display_edit_parameters(component, compio, editor)
            if changed_compio then changed = true end

            local changed_custio = custom_io(component, editor)
            if changed_custio then changed = true end
            
            if input.Press(KEYBOARD_BUTTON_ENTER) then changed = true end

            if changed then
                local editdata = {
                    entity = entity,
                    type = act_name,
                    pre = {},
                    post = deepcopy(editor)
                }
                build_edit_prestate(component, compio, editdata.pre)
                custom_def(component, editdata.pre)
                edit_execcmd("mod_comp", editdata)
            end

            if imgui.Button("Remove " .. tree_title) then
                local editdata = {
                    entity = entity,
                    type = act_name,
                    pre = {}
                }
                build_edit_prestate(component, compio, editdata.pre)
                custom_def(component, editdata.pre)
                edit_execcmd("del_comp", editdata)
            end

            imgui.TreePop()
        end
        if not imgui.IsItemFocused() then 
            build_edit_prestate(component, compio, editor) 
            custom_def(component, editor)
        end
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
            
                local namecomponent = wiscene.Component_GetName(entity)
                drawcomp("Name Component", "name", entity, namecomponent, compio_name, compinspect.component.name, 
                    function(mcomponent, meditor) end, function(mcomponent, mprestate) end)

                local layercomponent = wiscene.Component_GetLayer(entity)
                drawcomp("Layer Component", "layer", entity, layercomponent, {}, {}, 
                    function(mcomponent, meditor) 
                        local changed = false
                        local layers = layercomponent.LayerMask
                        meditor.LayerMask = 0
                        
                        for flag_id = 0, 31, 1 do
                            local block = 1 << flag_id
                            local get_check = (layers & block) > 0
                            local set_changed = false
                            
                            if (flag_id%6 > 0) and (flag_id ~= 0) then imgui.SameLine() end
                            set_changed, get_check = imgui.Checkbox(flag_id .. "##" .. flag_id, get_check)
                            if set_changed then changed = true end

                            local get = 0
                            if get_check == true then
                                meditor.LayerMask = meditor.LayerMask | block
                            end
                        end
                        if imgui.Button("Select ALL") then
                            meditor.LayerMask = 0xffffffff
                            changed = true
                        end
                        imgui.SameLine()
                        if imgui.Button("Select NONE") then 
                            meditor.LayerMask = 0 
                            changed = true
                        end
                        return changed
                    end, 
                    function(mcomponent, mprestate) 
                        mprestate.LayerMask = mcomponent.LayerMask
                    end)

                local transformcomponent = wiscene.Component_GetTransform(entity)
                drawcomp("Transform Component", "transform", entity, transformcomponent, compio_transform, compinspect.component.transform, 
                    function(mcomponent, meditor) end, function(mcomponent, mprestate) end)

                local objectcomponent = wiscene.Component_GetObject(entity)
                drawcomp("Object Component", "object", entity, objectcomponent, compio_object, compinspect.component.object, 
                    function(mcomponent, meditor) end, function(mcomponent, mprestate) end)

                local emittercomponent = wiscene.Component_GetEmitter(entity)
                drawcomp("Emitter Component", "emitter", entity, emittercomponent, compio_emitter, compinspect.component.emitter, 
                    function(mcomponent, meditor) end, function(mcomponent, mprestate) end)

                local hairparticlecomponent = wiscene.Component_GetHairParticleSystem(entity)
                drawcomp("Hair Particle System", "hairparticle", entity, hairparticlecomponent, compio_hairparticle, compinspect.component.hairparticle, 
                    function(mcomponent, meditor) end, function(mcomponent, mprestate) end)

                local lightcomponent = wiscene.Component_GetLight(entity)
                drawcomp("Light Component", "light", entity, lightcomponent, compio_light, compinspect.component.light, 
                    function(mcomponent, meditor) 
                        local changed = false

                        local changed_set_shadow = false
                        meditor.set_shadow = mcomponent.IsCastShadow()
                        changed_set_shadow, meditor.set_shadow = imgui.Checkbox("Cast Shadow##shadow", meditor.set_shadow)
                        if changed_set_shadow then changed = true end

                        local changed_set_volumetric = false
                        meditor.set_volumetric = mcomponent.IsVolumetricsEnabled()
                        changed_set_volumetric, meditor.set_volumetric = imgui.Checkbox("Contribute Volumetric##volumetric", meditor.set_volumetric)
                        if changed_set_volumetric then changed = true end

                        return changed
                    end, 
                    function(mcomponent, mprestate) end)

                local rigidbodycomponent = wiscene.Component_GetRigidBodyPhysics(entity)
                drawcomp("Rigid Body Component", "rigidbody", entity, rigidbodycomponent, compio_rigidbody, compinspect.component.rigidbody, 
                    function(mcomponent, meditor) end, function(mcomponent, mprestate) end)

                local softbodycomponent = wiscene.Component_GetSoftBodyPhysics(entity)
                drawcomp("Soft Body Component", "softbody", entity, softbodycomponent, compio_softbody, compinspect.component.softbody, 
                    function(mcomponent, meditor) end, function(mcomponent, mprestate) end)

                local forcecomponent = wiscene.Component_GetForceField(entity)
                drawcomp("Force Field Component", "force", entity, forcecomponent, compio_force, compinspect.component.force, 
                    function(mcomponent, meditor) end, function(mcomponent, mprestate) end)

                local weathercomponent = wiscene.Component_GetWeather(entity)
                drawcomp("Weather Component", "weather", entity, weathercomponent, compio_weather, compinspect.component.weather, 
                    function(mcomponent, meditor) 
                        local ret_tree_atmos = imgui.TreeNode("Atmosphere Parameters")
                        if ret_tree_atmos then
                            display_edit_parameters(mcomponent.AtmosphereParameters, compio_weather_atmos, meditor.atmosphere)
                            imgui.TreePop()
                        end

                        local ret_tree_cloud = imgui.TreeNode("Cloud Parameters")
                        if ret_tree_cloud then
                            display_edit_parameters(mcomponent.VolumetricCloudParameters, compio_weather_cloud, meditor.cloud)
                            imgui.TreePop()
                        end
                    end, 
                    function(mcomponent, mprestate) 
                        if mprestate.atmosphere == nil then mprestate.atmosphere = {} end
                        if mprestate.cloud == nil then mprestate.cloud = {} end
                        build_edit_prestate(mcomponent.AtmosphereParameters, compio_weather_atmos, mprestate.atmosphere)
                        build_edit_prestate(mcomponent.VolumetricCloudParameters, compio_weather_cloud, mprestate.cloud)
                    end)

                local soundcomponent = wiscene.Component_GetSound(entity)
                drawcomp("Sound Component", "sound", entity, soundcomponent, compio_sound, compinspect.component.sound, 
                    function(mcomponent, meditor) 
                        local changed = false

                        local changed_set_loop = false
                        meditor.set_loop = mcomponent.IsLooped()
                        changed_set_loop, meditor.set_loop = imgui.Checkbox("Loop Sound", meditor.set_loop)
                        if changed_set_loop then changed = true end

                        local changed_set_2d = false
                        meditor.set_2d = mcomponent.IsDisable3D()
                        changed_set_2d, meditor.set_2d = imgui.Checkbox("2D sound", meditor.set_2d)
                        if changed_set_2d then changed = true end

                        if mcomponent.IsPlaying() then
                            if imgui.Button("Stop") then 
                                meditor.play = false
                                changed = true
                            end
                        else
                            if imgui.Button("Play") then 
                                meditor.play = true
                                changed = true
                            end
                        end

                        return changed
                    end, 
                    function(mcomponent, mprestate) 
                        mprestate.set_loop = mcomponent.IsLooped()
                        mprestate.set_2d = mcomponent.IsDisable3D()
                        mprestate.play = mcomponent.IsPlaying()
                    end)

                local collidercomponent = wiscene.Component_GetCollider(entity)
                drawcomp("Collider Component", "collider", entity, collidercomponent, compio_collider, compinspect.component.collider, 
                    function(mcomponent, meditor) end, function(mcomponent, mprestate) end)

                local instancecomponent = scene.Component_GetInstance(entity)
                drawcomp("Instance Component", "instance", entity, instancecomponent, compio_instance, compinspect.component.instance, 
                    function(mcomponent, meditor) end, function(mcomponent, mprestate) end)

                local streamcomponent = scene.Component_GetStream(entity)
                drawcomp("Stream Component", "stream", entity, streamcomponent, compio_stream, compinspect.component.stream, 
                    function(mcomponent, meditor) end, function(mcomponent, mprestate) end)


                -- local meshcomponent = scene.Component_GetMesh(entity)
                -- drawcomp("Mesh Component", "mesh", entity, meshcomponent, compio_mesh, compinspect.component.mesh, 
                --     function(mcomponent, meditor) end, function(mcomponent, mprestate) end)

                local materialcomponent = wiscene.Component_GetMaterial(entity)
                drawcomp("Material Component", "material", entity, materialcomponent, compio_material, compinspect.component.material, 
                    function(mcomponent, meditor) 
                        for index, label in ipairs(compio_material_texturenames) do
                            -- backlog_post(index)
                            local texname = mcomponent.GetTexture(index-1)
                            local result = Editor_ImguiImageButton(texname, 100.0, 100.0)
                            if result then backlog_post(texname) end
                        end

                        Editor_RenderObjectPreview("MatPrevImg",0,entity)
                        Editor_ImguiImage("MatPrevImg",200.0, 200.0)
                    end, 
                    function(mcomponent, mprestate) 

                    end)

                if imgui.Button("\t\t\t\t\tAdd Component\t\t\t\t\t") then imgui.OpenPopup("CEAC") end -- TODO
            end
        end

        if imgui.BeginPopupContextWindow("CEAC") then
            local actions = D.editor_data.actions
            for _, creator in ipairs(component_creators) do
                local label = creator[1]
                local packet = creator[2]

                local act = false
                act = imgui.MenuItem(label, nil, act)
                if act then edit_execcmd("add_comp", {type = packet, entity = D.editor_data.elements.scenegraphview.selected_entity}) end
            end
            imgui.EndPopup()
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
                if (importwindow.type == "GLTF") or (importwindow.type == "GLB") then
                    Editor_ImportGLTF(importwindow.file)
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
                    if entityselector.filter_type == 1 then entities_list = scenegraphview.list.meshes end
                    if scenegraphview.filter_type == 2 then entities_list = scenegraphview.list.materials end
                    if scenegraphview.filter_type == 3 then entities_list = scenegraphview.list.animations end
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
    if Editor_IsEntityListUpdated() or D.editor_data.elements.scenegraphview.force_refresh then
        D.editor_data.elements.scenegraphview.list.objects = Editor_GetObjectList()
        D.editor_data.elements.scenegraphview.list.meshes = scene.Entity_GetMeshArray()
        D.editor_data.elements.scenegraphview.list.materials = wiscene.Entity_GetMaterialArray()
        D.editor_data.elements.scenegraphview.list.animations = wiscene.Entity_GetAnimationArray()
        D.editor_data.elements.scenegraphview.list.lights = wiscene.Entity_GetLightArray()
        D.editor_data.elements.scenegraphview.list.weathers = wiscene.Entity_GetWeatherArray()
        D.editor_data.elements.scenegraphview.list.instances = scene.Entity_GetInstanceArray()
        
        D.editor_data.elements.scenegraphview.force_refresh = false
    end

    D.editor_data.elements.scenegraphview.wait_update = false
end

local update_sysmenu_actions = function()
    local actions = D.editor_data.actions
    -- File menu actions
    if actions.resource_new then
        -- Delete scene data and history
        if type(actions.command_list) == "table" then
            actions.command_head = 0
            actions.command_list = {}
            Editor_WipeDeletedEntityList()
        end
        scene.Clear()
        edit_execcmd("init")

        D.editor_data.core_data.resname = "Untitled Scene"

        actions.resource_new = false
    end
    if actions.resource_rename then
        D.editor_data.elements.fmenu_rnres.win_visible = true
        actions.resource_rename = false
    end
    if actions.resource_save then
        Editor_RenderScenePreview("SaveImg")
        Editor_SaveImage("SaveImg","Data/Editor/Thumb/" .. D.editor_data.core_data.resname .. ".png")
        Editor_SaveScene(SOURCEPATH_SCENE .. "/" .. D.editor_data.core_data.resname .. DATATYPE_SCENE_DATA)
        Editor_ExtractSubInstanceNames("Data/Editor/Instances/" .. D.editor_data.core_data.resname .. ".instlist")
        actions.resource_save = false
    end
    if actions.resource_open then
        Editor_LoadScene(D.editor_data.elements.resexp.selected_file)
        D.editor_data.core_data.resname = D.editor_data.elements.resexp.selected_resname
        D.editor_data.elements.scenegraphview.force_refresh = true
        actions.resource_open = false
    end
    if actions.resource_instance then
        local editdata = {
            entity = CreateEntity(),
            type = "instance",
        }
        edit_execcmd("add_obj", editdata)
        local name = wiscene.Component_GetName(editdata.entity)
        local instance = scene.Component_GetInstance(editdata.entity)
        name.Name = D.editor_data.elements.resexp.selected_resname
        instance.Lock = false
        instance.File = D.editor_data.elements.resexp.selected_file
        instance.EntityName = D.editor_data.elements.resexp.selected_subinstance
        actions.resource_instance = false
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
    if actions.import_gltf then
        filedialog(0,"GLTF Model File","gltf;glb",function(data)
            local import_opt = D.editor_data.elements.importwindow
            import_opt.file = data.filepath
            import_opt.type = data.type
            import_opt.win_visible = true
        end)
        actions.import_gltf = false
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

local update_editmapstreamdata = function()
    local stream_radius = 50
    local navigation = D.editor_data.navigation
    local streamboundary = scene.StreamBoundary
    streamboundary.Max = Vector(navigation.camera_pos.X + stream_radius, navigation.camera_pos.Y + stream_radius, navigation.camera_pos.Z + stream_radius)
    streamboundary.Min = Vector(navigation.camera_pos.X - stream_radius, navigation.camera_pos.Y - stream_radius, navigation.camera_pos.Z - stream_radius)
    scene.StreamBoundary = streamboundary
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
        update_editmapstreamdata()
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