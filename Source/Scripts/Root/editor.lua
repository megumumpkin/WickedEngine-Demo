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
            }
        },
        compinspect = {
            win_visible = true,
            component = {
                name = {},
                transform = {},
                object = {},
                light = {},
                sound = {}
            }
        },
        fmenu_rnres = {
            win_visible = false,
            input = "",
            apply = false,
        },
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
        camera_speed_mul_select = 1,
        camera_speed_mul = 3.0,
        translatormode = 1,
    },
    core_data = {
        resname = "Untitled Scene",
    }
})

local scene = GetGlobalGameScene()
local wiscene = scene.GetWiScene()

local CAM_MOVE_SPD = 0.3
local CAM_ROT_SPD = 0.03

local component_set_name = function(component, editdata)
    component.Name = editdata.namestr
end

local component_set_layer = function(component, editdata)
    component.LayerMask = editdata.mask
end

local component_set_object = function(component, editdata)
    component.Color = editdata.col
    component.EmissiveColor = editdata.emissivecol
    component.CascadeMask = editdata.cascademask
    component.RendertypeMask = editdata.rendertypemask
end

local component_set_light = function(component, editdata)
    component.Range = editdata.range
    component.Intensity = editdata.intensity
    component.Color = editdata.col
    component.OuterConeAngle = editdata.outcang
    component.InnerConeAngle = editdata.incang
    component.SetCastShadow(editdata.set_shadow)
    component.SetVolumetricsEnabled(editdata.set_volumetric)
end

local component_set_sound = function(component, editdata)
    component.Filename = editdata.fname
    component.Volume = editdata.vol
    component.SetLooped(editdata.set_loop)
    component.SetDisable3D(editdata.set_3d)
    if editdata.play then
        component.Play()
    else
        component.Stop()
    end
end

local edit_execcmd = function(command, extradata, holdout)
    if command == "add_obj" then
        if type(extradata) == "table" then
            local entity = 0
            if extradata.entity ~= nil then
                entity = extradata.entity
            else
                entity = scene.Entity_Create()
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
                if namecomponent then component_set_name(namecomponent, extradata.post) end  
            end
            if extradata.type == "layer" then
                local layercomponent = wiscene.Component_GetLayer(extradata.entity)
                if layercomponent then component_set_layer(layercomponent, extradata.post) end
            end
            if extradata.type == "object" then
                local objectcomponent = wiscene.Component_GetObject(extradata.entity)
                if objectcomponent then component_set_layer(objectcomponent, extradata.post) end
            end
            if extradata.type == "light" then
                local lightcomponent = wiscene.Component_GetLight(extradata.entity)
                if lightcomponent then component_set_layer(lightcomponent, extradata.post) end
            end
            if extradata.type == "sound" then
                local soundcomponent = wiscene.Component_GetSound(extradata.entity)
                if soundcomponent then component_set_layer(soundcomponent, extradata.post) end
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
            if namecomponent then component_set_name(namecomponent, extradata.pre) end  
        end
        if extradata.type == "layer" then
            local layercomponent = wiscene.Component_GetLayer(extradata.entity)
            if layercomponent then component_set_layer(layercomponent, extradata.pre) end
        end
        if extradata.type == "object" then
            local objectcomponent = wiscene.Component_GetObject(extradata.entity)
            if objectcomponent then component_set_layer(objectcomponent, extradata.pre) end
        end
        if extradata.type == "light" then
            local lightcomponent = wiscene.Component_GetLight(extradata.entity)
            if lightcomponent then component_set_layer(lightcomponent, extradata.pre) end
        end
        if extradata.type == "sound" then
            local soundcomponent = wiscene.Component_GetSound(extradata.entity)
            if soundcomponent then component_set_layer(soundcomponent, extradata.pre) end
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
        sub_visible, fmenu_rnres.win_visible = imgui.Begin("\xef\x81\x84 Rename Scene", fmenu_rnres.win_visible)
        if sub_visible then
            ret, fmenu_rnres.input = imgui.InputText("##fmenu_rnres_input", fmenu_rnres.input, 255)
            imgui.SameLine()
            if imgui.Button("\xef\x81\x84 ") then
                backlog_post("ACT_INF: rename to > " .. fmenu_rnres.input)
                backlog_post("ACT_STUB: rename resource apply to object is not implemented yet")
                fmenu_rnres.apply = false
            end
        end
    end
end

local drawsceneexp = function()
    local resexp = D.editor_data.elements.resexp

    if resexp.win_visible then
        sub_visible, resexp.win_visible = imgui.Begin("\xef\x86\xb2 Scene Explorer", resexp.win_visible)
        if sub_visible then
            ret, resexp.input = imgui.InputText("##resexp_sin", resexp.input, 255)
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

local drawscenegraphview = function()
    local scenegraphview = D.editor_data.elements.scenegraphview

    if scenegraphview.win_visible then
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
                        for _, entity in pairs(entities_list) do
                            local name = wiscene.Component_GetName(entity)
                            if name then
                                local flag = 0
                                if scenegraphview.selected_entity == entity then
                                    flag = flag | imgui.constant.TreeNodeFlags.Selected
                                end

                                local ret_tree = imgui.TreeNodeEx(name.GetName() .. "##" .. entity, flag)
                                if imgui.IsItemClicked() then
                                    scenegraphview.selected_entity = entity
                                    Editor_FetchSelection(scenegraphview.selected_entity)
                                end
                                if ret_tree then
                                    imgui.TreePop()
                                end
                            end
                        end
                    end
                end    
            end
            imgui.EndChild()
        end
        imgui.End()
    end
end

local drawcompinspect = function()
    local compinspect = D.editor_data.elements.compinspect
    if compinspect.win_visible then
        sub_visible, compinspect.win_visible = imgui.Begin("\xef\x82\x85 Component Inspector", compinspect.win_visible)
        if sub_visible then
            local entity = D.editor_data.elements.scenegraphview.selected_entity
            if entity > 0 then
                
                -- NameComponent
                local namecomponent = wiscene.Component_GetName(entity)
                if namecomponent then
                    local editor_name = compinspect.component.name
                    --Init
                    if editor_name.namestr == nil then editor_name.namestr = namecomponent.Name end
                    --
                    local ret_tree = imgui.TreeNode("Name Component")
                    if ret_tree then
                        _, editor_name.namestr = imgui.InputText("Name", editor_name.namestr, 255)
                        
                        if input.Press(KEYBOARD_BUTTON_ENTER) then
                            local editdata = {
                                entity = entity,
                                type = "name",
                                pre = {
                                    namestr = namecomponent.Name
                                },
                                post = deepcopy(editor_name)
                            }
                            edit_execcmd("mod_comp", editdata)
                        end
                        imgui.TreePop()
                    end
                    if not imgui.IsItemFocused() then editor_name.namestr = namecomponent.Name end
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
                                    mask = layercomponent.LayerMask
                                },
                                post = {
                                    mask = set
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
                    --Init
                    if editor_transform.pos == nil then editor_transform.pos = transformcomponent.GetPosition() end
                    if editor_transform.rot == nil then 
                        local rot_quat = transformcomponent.GetRotation()
                        editor_transform.rot = vector.QuaternionToRollPitchYaw(rot_quat)
                    end
                    if editor_transform.sca == nil then editor_transform.sca = transformcomponent.GetScale() end
                    --
                    if ret_tree then
                        local changed = false

                        _, editor_transform.pos.X, editor_transform.pos.Y, editor_transform.pos.Z = imgui.InputFloat3("Position", editor_transform.pos.X, editor_transform.pos.Y, editor_transform.pos.Z)
                        _, editor_transform.rot.X, editor_transform.rot.Y, editor_transform.rot.Z = imgui.InputFloat3("Euler Rotation", editor_transform.rot.X, editor_transform.rot.Y, editor_transform.rot.Z)
                        _, editor_transform.sca.X, editor_transform.sca.Y, editor_transform.sca.Z = imgui.InputFloat3("Scale", editor_transform.sca.X, editor_transform.sca.Y, editor_transform.sca.Z)
                        
                        -- if input.Press(KEYBOARD_BUTTON_ENTER) then
                        --     transformcomponent.ClearTransform();
                        --     transformcomponent.Translate(editor_transform.pos)
                        --     local rot_quat = vector.QuaternionFromRollPitchYaw(editor_transform.rot)
                        --     stransformcomponent.Rotate(rot_quat)
                        --     transformcomponent.Scale(editor_transform.sca)
                        -- end

                        imgui.TreePop()
                    end
                    if not imgui.IsItemFocused() then 
                        editor_transform.pos = transformcomponent.GetPosition()
                        local rot_quat = transformcomponent.GetRotation()
                        editor_transform.rot = vector.QuaternionToRollPitchYaw(rot_quat)
                        editor_transform.sca = transformcomponent.GetScale()
                    end
                end
                --

                -- ObjectComponent
                local objectcomponent = wiscene.Component_GetObject(entity)
                if objectcomponent then
                    local editor_object = compinspect.component.transform
                    --Init
                    if editor_object.cascademask == nil then editor_object.cascademask = objectcomponent.CascadeMask end
                    if editor_object.rendertypemask == nil then editor_object.rendertypemask = objectcomponent.RendertypeMask end
                    if editor_object.col == nil then editor_object.col = objectcomponent.Color end
                    if editor_object.emscol == nil then editor_object.emscol = objectcomponent.EmissiveColor end
                    --
                    local ret_tree = imgui.TreeNode("Object Component")
                    if ret_tree then
                        local changed = false

                        local meshID = objectcomponent.GetMeshID()
                        local mesh_name = meshID .. " - NO MESH"
                        if meshID > 0 then
                            local mesh_namecomponent = wiscene.Component_GetName(meshID)
                            if mesh_namecomponent then mesh_name = meshID .. " - " .. mesh_namecomponent.GetName() end
                        end

                        imgui.InputText("Mesh ID##meshid", mesh_name, 255, imgui.constant.InputTextFlags.ReadOnly)
                        imgui.SameLine()
                        if imgui.Button("\xef\x86\xb2 Set Mesh") then end

                        _, editor_object.cascademask = imgui.InputInt("Cascade Mask##ccmask", editor_object.cascademask)
                        _, editor_object.rendertypemask = imgui.InputInt("Render Type Mask##rtmask", editor_object.rendertypemask)
                        _, editor_object.col.X, editor_object.col.Y, editor_object.col.Z, editor_object.col.W = imgui.InputFloat4("Color##col", editor_object.col.X, editor_object.col.Y, editor_object.col.Z, editor_object.col.W)
                        _, editor_object.emscol.X, editor_object.emscol.Y, editor_object.emscol.Z, editor_object.emscol.W = imgui.InputFloat4("Emissive Color##emscol", editor_object.emscol.X, editor_object.emscol.Y, editor_object.emscol.Z, editor_object.emscol.W)

                        if input.Press(KEYBOARD_BUTTON_ENTER) then changed = true end

                        if changed then
                            local editdata = {
                                entity = entity,
                                type = "object",
                                pre = {
                                    col = objectcomponent.Color,
                                    emissivecol = objectcomponent.EmissiveColor,
                                    cascademask = objectcomponent.CascadeMask,
                                    rendertypemask = objectcomponent.RendertypeMask
                                },
                                post = deepcopy(editor_object)
                            }
                            edit_execcmd("mod_comp", editdata)
                        end

                        imgui.TreePop()
                    end
                    if not imgui.IsItemFocused() then 
                        editor_object.cascademask = objectcomponent.CascadeMask
                        editor_object.rendertypemask = objectcomponent.RendertypeMask
                        editor_object.col = objectcomponent.Color
                        editor_object.emscol = objectcomponent.EmissiveColor
                    end
                end
                --

                -- LightComponent
                local lightcomponent = wiscene.Component_GetLight(entity)
                if lightcomponent then
                    local editor_light = compinspect.component.light
                    local ret_tree = imgui.TreeNode("Light Component")
                    --Init
                    if editor_light.type == nil then editor_light.type = lightcomponent.Type end
                    if editor_light.col == nil then editor_light.col = lightcomponent.Color end
                    if editor_light.range == nil then editor_light.range = lightcomponent.Range end
                    if editor_light.intensity == nil then editor_light.intensity = lightcomponent.Intensity end
                    if editor_light.outcang == nil then editor_light.outcang = lightcomponent.OuterConeAngle end
                    if editor_light.incang == nil then editor_light.incang = lightcomponent.InnerConeAngle end
                    --
                    if ret_tree then
                        local changed = false

                        _, editor_light.type = imgui.Combo("\xef\x82\xb0##filter",editor_light.type,"Directional\0Point\0Spot\0")
                        _, editor_light.range = imgui.InputFloat("Range", editor_light.range)
                        _, editor_light.intensity = imgui.InputFloat("Intensity", editor_light.intensity)
                        _, editor_light.col.X, editor_light.col.Y, editor_light.col.Z = imgui.InputFloat3("Color##col", editor_light.col.X, editor_light.col.Y, editor_light.col.Z)
                        _, editor_light.outcang = imgui.InputFloat("Outer Cone Angle##outcang", editor_light.outcang)
                        _, editor_light.incang = imgui.InputFloat("Inner Cone Angle##incang", editor_light.incang)
                        
                        local changed_set_shadow = false
                        editor_light.set_shadow = lightcomponent.IsCastShadow()
                        changed_set_shadow, editor_light.set_shadow = imgui.Checkbox("Cast Shadow##shadow", editor_light.set_shadow)
                        if changed_set_shadow then changed = true end

                        local changed_set_volumetric = false
                        editor_light.set_volumetric = lightcomponent.IsVolumetricsEnabled()
                        changed_set_volumetric, set_volumetric = imgui.Checkbox("Contribute Volumetric##volumetric", editor_light.set_volumetric)
                        if changed_set_volumetric then changed = true end

                        if input.Press(KEYBOARD_BUTTON_ENTER) then changed = true end

                        if changed then
                            local editdata = {
                                entity = entity,
                                type = "light",
                                pre = {
                                    type = lightcomponent.Type,
                                    col = lightcomponent.Color,
                                    range = lightcomponent.Range,
                                    intensity = lightcomponent.Intensity,
                                    outcang = lightcomponent.OuterConeAngle,
                                    incang = lightcomponent.InnerConeAngle,
                                    set_shadow = lightcomponent.IsCastShadow(),
                                    set_volumetric = lightcomponent.IsVolumetricsEnabled()
                                },
                                post = deepcopy(editor_light)
                            }
                            edit_execcmd("mod_comp", editdata)
                        end
                        
                        imgui.TreePop()
                    end
                    if not imgui.IsItemFocused() then 
                        editor_light.col = lightcomponent.Color
                        editor_light.range = lightcomponent.Range
                        editor_light.intensity = lightcomponent.Intensity 
                        editor_light.outcang = lightcomponent.OuterConeAngle
                        editor_light.incang = lightcomponent.InnerConeAngle
                    end
                end
                --

                -- SoundComponent
                local soundcomponent = wiscene.Component_GetSound(entity)
                if soundcomponent then
                    local editor_sound = compinspect.component.sound
                    --Init
                    if editor_sound.fname == nil then editor_sound.fname = soundcomponent.Filename end
                    if editor_sound.vol == nil then editor_sound.vol = soundcomponent.Volume end
                    --
                    local ret_tree = imgui.TreeNode("Sound Component")
                    if ret_tree then
                        local changed = false

                        _, editor_sound.fname = imgui.InputText("Filename", editor_sound.fname, 255)
                        _, editor_sound.vol = imgui.InputFloat("Volume", editor_sound.vol, 255)

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
                                    fname = soundcomponent.Filename,
                                    vol = soundcomponent.Volume,
                                    set_loop = soundcomponent.IsLooped(),
                                    set_2d = soundcomponent.IsDisable3D(),
                                    play = soundcomponent.IsPlaying()
                                },
                                post = deepcopy(editor_sound)
                            }
                            edit_execcmd("mod_comp", editdata)
                        end
                        imgui.TreePop()
                    end
                    if not imgui.IsItemFocused() then editor_sound.fname = soundcomponent.Filename end
                end
                --

                -- WeatherComponent
                local weathercomponent = wiscene.Component_GetWeather(entity)
                if weathercomponent then
                    local editor_name = compinspect.component.name
                    --Init
                    -- if editor_name.namestr == nil then editor_name.namestr = namecomponent.Name end
                    --
                    local ret_tree = imgui.TreeNode("Weather Component")
                    if ret_tree then
                        local changed = false

                        -- _, editor_name.namestr = imgui.InputText("Name", editor_name.namestr, 255)
                        
                        if input.Down(KEYBOARD_BUTTON_ENTER) then
                            
                        end
                        imgui.TreePop()
                    end
                    if not imgui.IsItemFocused() then  end
                end
                --

                if imgui.Button("               Add Component               ") then end -- TODO
            end
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
        elements.fmenu_rnres.win_visible = true
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
            scene.LoadScene(data.filepath)
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

    if editor_ui_focused() == false then

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
        editor_dev_griddraw(true)
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
        if D.editor_data.elements.helper_demo.win_visible then
            imgui.ShowDemoWindow()
        end
    end
end)