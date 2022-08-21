D("editor_data",{
    elements = {
        resexp = {
            win_visible = false,
            input = "",
        },
        scenegraphview = {
            win_visible = false,
            filter_selected = 0,
            selected_entity = 0,
            wait_update = false,
            list = {
                objects = {},
                meshes = {},
                materials = {},
                animation = {}
            }
        },
        compinspect = {
            win_visible = false,
            component = {
                name = {},
                transform = {},
                object = {},
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
        command_head = 0,
        resource_new = false,
        resource_rename = false,
        resource_save = false,
        link_dcc = false,
        import_wiscene = false,
        add_object = false,
    },
    navigation = {
        camera = GetCamera(),
        camera_transform = TransformComponent(),
        camera_pos = Vector(0,2,-5),
        camera_rot = Vector(0,0,0),
    },
    core_data = {
        resname = "Untitled Scene",
    }
})

local scene = GetGlobalGameScene()
local wiscene = scene.GetWiScene()

local CAM_MOVE_SPD = 0.3
local CAM_ROT_SPD = 0.03

local edit_execcmd = function(command, extradata, holdout)
    if command == "add" then
        if type(extradata) == "table" then
            local entity = scene.Entity_Create()
            extradata.entity = entity
            local name = wiscene.Component_CreateName(entity)
            name.SetName(extradata.name)
            if extradata.type == "object" then
                local transform = wiscene.Component_CreateTransform(entity)
                local layer = wiscene.Component_CreateLayer(entity)
                local object = wiscene.Component_CreateObject(entity)
            end
        end
    end

    if command == "del" then
        wiscene.Entity_Remove(extradata.entity)
    end

    -- To run new command or just redo previous commands
    if holdout == nil then
        if D.editor_data.actions.command_head < #D.editor_data.actions.command_list then
            for idx = #D.editor_data.actions.command_list, D.editor_data.actions.command_head, -1 do
                if idx > 1 then table.remove(D.editor_data.actions.command_list, idx) end
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

    if command == "add" then
        wiscene.Entity_Remove(extradata.entity)
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

        if imgui.BeginPopupContextWindow("MBFM") then
            D.editor_data.actions.resource_new = imgui.MenuItem("\xee\x93\xae New Resource",nil,D.editor_data.actions.resource_new)
            D.editor_data.actions.resource_rename = imgui.MenuItem("\xef\x81\x84 Rename Resource",nil,D.editor_data.actions.resource_rename)
            D.editor_data.actions.resource_save = imgui.MenuItem("\xef\x83\x87 Save Resource",nil,D.editor_data.actions.resource_save)
            imgui.EndPopup()
        end

        if imgui.BeginPopupContextWindow("MBIM") then
            D.editor_data.actions.link_dcc = imgui.MenuItem("\xef\x86\xb2 Create DCC Link",nil,D.editor_data.actions.link_dcc)
            D.editor_data.actions.import_wiscene = imgui.MenuItem("\xef\x86\xb2 Import wiScene",nil,D.editor_data.actions.import_wiscene)
            D.editor_data.actions.add_object = imgui.MenuItem("\xef\x80\xa8 Add Object",nil,D.editor_data.actions.add_object)
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
            ret_filter, scenegraphview.filter_selected = imgui.Combo("\xef\x82\xb0##filter",scenegraphview.filter_selected,"Objects\0Meshes\0Materials\0Animation")
            
            imgui.PushStyleVar(imgui.constant.StyleVar.ChildRounding, 5.0)
            local childflags = 0 | imgui.constant.WindowFlags.NoTitleBar
            local view_oblist = imgui.BeginChild("##listview", 0, 0, true, childflags)
            imgui.PopStyleVar()
            
            if view_oblist then
                if not scenegraphview.wait_update then
                    for _, entity in pairs(scenegraphview.list.objects) do
                        local name = wiscene.Component_GetName(entity)
                        if name then
                            local flag = 0
                            if scenegraphview.selected_entity == entity then
                                flag = flag | imgui.constant.TreeNodeFlags.Selected
                            end

                            local ret_tree = imgui.TreeNodeEx(name.GetName() .. "##" .. entity, flag)
                            if imgui.IsItemClicked() then
                                scenegraphview.selected_entity = entity
                            end
                            if ret_tree then
                                imgui.TreePop()
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
                    if editor_name.namestr == nil then editor_name.namestr = namecomponent.GetName() end
                    --
                    local ret_tree = imgui.TreeNode("Name Component")
                    if ret_tree then
                        local changed = false

                        local get_namestr = editor_name.namestr

                        local set_name, get_namestr = imgui.InputText("Name", get_namestr, 255)
                        if set_name == true then changed = true end
                        
                        local apply = false
                        if changed and input.Down(KEYBOARD_BUTTON_ENTER) then apply = true end
                        if apply then
                            namecomponent.SetName(get_namestr)
                        end
                        imgui.TreePop()
                    end
                    if not imgui.IsItemFocused() then editor_name.namestr = namecomponent.GetName() end
                end
                --

                -- LayerComponent
                local layercomponent = wiscene.Component_GetLayer(entity)
                if layercomponent then
                    local ret_tree = imgui.TreeNode("Layer Component")
                    if ret_tree then
                        local layers = layercomponent.GetLayerMask()
                        local set = 0
                        for flag_id = 0, 31, 1 do
                            local block = 1 << flag_id
                            local get_check = (layers & block) > 0
                            if (flag_id%6 > 0) and (flag_id ~= 0) then imgui.SameLine() end
                            _, get_check = imgui.Checkbox(flag_id .. "##" .. flag_id, get_check)
                            local get = 0
                            if get_check == true then
                                set = set | block
                            end
                        end
                        layercomponent.SetLayerMask(set)
                        if imgui.Button("Select ALL") then layercomponent.SetLayerMask(0xffffffff) end
                        imgui.SameLine()
                        if imgui.Button("Select NONE") then layercomponent.SetLayerMask(0) end
                        imgui.TreePop()
                    end
                end
                --

                -- TransformComponent
                local transformcomponent = wiscene.Component_GetTransform(entity)
                if transformcomponent then
                    local editor_transform = compinspect.component.transform
                    local ret_tree = imgui.TreeNode("Transform Component")
                    --Init
                    if editor_transform.edit_pos == nil then editor_transform.edit_pos = transformcomponent.GetPosition() end
                    if editor_transform.edit_rot == nil then 
                        local rot_quat = transformcomponent.GetRotation()
                        editor_transform.edit_rot = vector.QuaternionToRollPitchYaw(rot_quat)
                    end
                    if editor_transform.edit_sca == nil then editor_transform.edit_sca = transformcomponent.GetScale() end
                    --
                    if ret_tree then
                        local changed = false

                        local tx = editor_transform.edit_pos.GetX()
                        local ty = editor_transform.edit_pos.GetY()
                        local tz = editor_transform.edit_pos.GetZ()

                        local rx = editor_transform.edit_rot.GetX()
                        local ry = editor_transform.edit_rot.GetY()
                        local rz = editor_transform.edit_rot.GetZ()

                        local sx = editor_transform.edit_sca.GetX()
                        local sy = editor_transform.edit_sca.GetY()
                        local sz = editor_transform.edit_sca.GetZ()

                        local set_pos, tx, ty, tz = imgui.InputFloat3("Position", tx, ty, tz)
                        if set_pos == true then changed = true end
                        
                        local set_rot, rx, ry, rz = imgui.InputFloat3("Euler Rotation", rx, ry, rz)
                        if set_rot == true then changed = true end
                        
                        local set_sca, sx, sy, sz = imgui.InputFloat3("Scale", sx, sy, sz)
                        if set_sca == true then changed = true end
                        
                        
                        local apply = false
                        if changed and input.Down(KEYBOARD_BUTTON_ENTER) then apply = true end
                        if apply then
                            editor_transform.edit_pos.SetX(tx)
                            editor_transform.edit_pos.SetY(ty)
                            editor_transform.edit_pos.SetZ(tz)

                            editor_transform.edit_rot.SetX(rx)
                            editor_transform.edit_rot.SetY(ry)
                            editor_transform.edit_rot.SetZ(rz)

                            editor_transform.edit_sca.SetX(sx)
                            editor_transform.edit_sca.SetY(sy)
                            editor_transform.edit_sca.SetZ(sz)

                            transformcomponent.ClearTransform();
                            transformcomponent.Translate(editor_transform.edit_pos)
                            local rot_quat = vector.QuaternionFromRollPitchYaw(editor_transform.edit_rot)
                            transformcomponent.Rotate(rot_quat)
                            transformcomponent.Scale(editor_transform.edit_sca)
                        end

                        imgui.TreePop()
                    end
                    if not imgui.IsItemFocused() then 
                        editor_transform.edit_pos = transformcomponent.GetPosition()
                        local rot_quat = transformcomponent.GetRotation()
                        editor_transform.edit_rot = vector.QuaternionToRollPitchYaw(rot_quat)
                        editor_transform.edit_sca = transformcomponent.GetScale()
                    end
                end
                --

                -- ObjectComponent
                local objectcomponent = wiscene.Component_GetObject(entity)
                if objectcomponent then
                    local editor_object = compinspect.component.transform
                    --Init
                    if editor_object.edit_col == nil then editor_object.edit_col = objectcomponent.GetColor() end
                    if editor_object.edit_stencilref == nil then editor_object.edit_stencilref = objectcomponent.GetUserStencilRef() end
                    --
                    local ret_tree = imgui.TreeNode("Object Component")
                    if ret_tree then
                        local changed = false
                        
                        local col_r = editor_object.edit_col.GetX()
                        local col_g = editor_object.edit_col.GetY()
                        local col_b = editor_object.edit_col.GetZ()
                        local col_a = editor_object.edit_col.GetW()

                        local stencilref = editor_object.edit_stencilref

                        local set_col, col_r, col_g, col_b, col_a = imgui.InputFloat4("Color##edit_col", col_r, col_g, col_b, col_a)
                        if set_col == true then changed = true end

                        local set_stencilref, stencilref = imgui.InputInt("Stencil Ref ID##edit_stencil", stencilref)
                        if set_stencilref == true then changed = true end

                        local apply = false
                        if changed and input.Down(KEYBOARD_BUTTON_ENTER) then apply = true end
                        if apply then
                            editor_object.edit_col.SetX(col_r)
                            editor_object.edit_col.SetY(col_g)
                            editor_object.edit_col.SetZ(col_b)
                            editor_object.edit_col.SetW(col_a)

                            objectcomponent.SetColor(editor_object.edit_col)
                            objectcomponent.SetUserStencilRef(stencilref)
                        end

                        imgui.TreePop()
                    end
                    if not imgui.IsItemFocused() then 
                        editor_object.edit_col = objectcomponent.GetColor()
                        editor_object.edit_stencilref = objectcomponent.GetUserStencilRef()
                    end
                end
                --

                -- LightComponent
                local lightcomponent = wiscene.Component_GetLight(entity)
                if lightcomponent then
                    local ret_tree = imgui.TreeNode("Light Component")
                    if ret_tree then
                        
                        imgui.TreePop()
                    end
                end
                --

                if imgui.Button("               Add Component               ") then end -- TODO
            end
        end

        imgui.End()
    end
end

local update_scenegraph = function()
    D.editor_data.elements.scenegraphview.list.objects = wiscene.Entity_GetTransformArray()
    D.editor_data.elements.scenegraphview.wait_update = false
end

local update_sysmenu_actions = function()
    local actions = D.editor_data.actions
    -- File menu actions
    if actions.resource_new then
        backlog_post("ACT_STUB: new resource not working")
        filedialog(0,"Wicked Engine Scene","wiscene",function(data)
            backlog_post(data.filepath)
        end)
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
    if actions.add_object then
        edit_execcmd("add", {type = "object", name = "New Object"})
        actions.add_object = false
    end
end

local update_navigation = function()
    local navigation = D.editor_data.navigation

    if editor_ui_focused() == false then

        local camera_pos_delta = Vector()
        local camera_rot_delta = Vector()
        
        -- Camera movement WASDQE
        if(input.Down(string.byte('W'))) then camera_pos_delta.SetZ(1.0*CAM_MOVE_SPD) end
        if(input.Down(string.byte('S'))) then camera_pos_delta.SetZ(-1.0*CAM_MOVE_SPD) end
        if(input.Down(string.byte('A'))) then camera_pos_delta.SetX(-1.0*CAM_MOVE_SPD) end
        if(input.Down(string.byte('D'))) then camera_pos_delta.SetX(1.0*CAM_MOVE_SPD) end
        if(input.Down(string.byte('Q'))) then camera_pos_delta.SetY(-1.0*CAM_MOVE_SPD) end
        if(input.Down(string.byte('E'))) then camera_pos_delta.SetY(1.0*CAM_MOVE_SPD) end
        -- Camera rotation keyboard
        if(input.Down(KEYBOARD_BUTTON_UP)) then camera_rot_delta.SetX(-1.0*CAM_ROT_SPD) end
        if(input.Down(KEYBOARD_BUTTON_DOWN)) then camera_rot_delta.SetX(1.0*CAM_ROT_SPD) end
        if(input.Down(KEYBOARD_BUTTON_LEFT)) then camera_rot_delta.SetY(-1.0*CAM_ROT_SPD) end
        if(input.Down(KEYBOARD_BUTTON_RIGHT)) then camera_rot_delta.SetY(1.0*CAM_ROT_SPD) end
        -- Get rotated movement
        local camera_rot_matrix = matrix.Rotation(navigation.camera_rot)
        camera_pos_delta = vector.Transform(camera_pos_delta, camera_rot_matrix)

        -- Apply rotation delta
        navigation.camera_rot.SetX(navigation.camera_rot.GetX() + camera_rot_delta.GetX())
        navigation.camera_rot.SetY(navigation.camera_rot.GetY() + camera_rot_delta.GetY())
        -- Apply movement delta
        navigation.camera_pos.SetZ(navigation.camera_pos.GetZ() + camera_pos_delta.GetZ())
        navigation.camera_pos.SetX(navigation.camera_pos.GetX() + camera_pos_delta.GetX())
        navigation.camera_pos.SetY(navigation.camera_pos.GetY() + camera_pos_delta.GetY())

        -- Camera transform update
        navigation.camera_transform.ClearTransform()
        navigation.camera_transform.Translate(navigation.camera_pos)
        navigation.camera_transform.Rotate(navigation.camera_rot)
        navigation.camera_transform.UpdateTransform()
        navigation.camera.TransformCamera(navigation.camera_transform)
        navigation.camera.UpdateCamera()

    end
end

local update_editaction = function()
    if(input.Down(KEYBOARD_BUTTON_LCONTROL) and (input.Press(string.byte('Z')))) then
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