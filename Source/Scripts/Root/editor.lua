D("editor_data",{
    elements = {
        resexp = {
            win_visible = false,
            input = "",
        },
        scenegraphview = {
            win_visible = false,
            filter_selected = 0,
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
        },
        fmenu_rnres = {
            win_visible = false,
            input = "",
            apply = false,
        },
        helper_demo = {
            win_visible = false,
        }
    },
    actions = {
        command_head = 0,
        command_list = {},
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
        if imgui.Button("\xef\x86\xb2 SceneName   \xef\x83\x97") then
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
        sub_visible, D.editor_data.elements.fmenu_rnres.win_visible = imgui.Begin("\xef\x81\x84 Rename Scene", D.editor_data.elements.fmenu_rnres.win_visible)
        if sub_visible then
            ret, D.editor_data.elements.fmenu_rnres.input = imgui.InputText("##fmenu_rnres_input", D.editor_data.elements.fmenu_rnres.input, 255)
            imgui.SameLine()
            if imgui.Button("\xef\x81\x84 ") then
                backlog_post("ACT_INF: rename to > " .. D.editor_data.elements.fmenu_rnres.input)
                backlog_post("ACT_STUB: rename resource apply to object is not implemented yet")
                D.editor_data.elements.fmenu_rnres.apply = false
            end
        end
    end
end

local drawsceneexp = function()
    if D.editor_data.elements.resexp.win_visible then
        sub_visible, D.editor_data.elements.resexp.win_visible = imgui.Begin("\xef\x86\xb2 Resources Explorer", D.editor_data.elements.resexp.win_visible)
        if sub_visible then
            ret, D.editor_data.elements.resexp.input = imgui.InputText("##resexp_sin", D.editor_data.elements.resexp.input, 255)
            imgui.SameLine() imgui.Button("\xef\x85\x8e Search")
        end
        imgui.End()
    end
end

local drawscenegraphview = function()
    if D.editor_data.elements.scenegraphview.win_visible then
        sub_visible, D.editor_data.elements.scenegraphview.win_visible = imgui.Begin("\xef\xa0\x82 Scene Graph Viewer", D.editor_data.elements.scenegraphview.win_visible)
        if sub_visible then
            ret_filter, D.editor_data.elements.scenegraphview.filter_selected = imgui.Combo("\xef\x82\xb0##filter",D.editor_data.elements.scenegraphview.filter_selected,"Objects\0Meshes\0Materials\0Animation")
            
            imgui.PushStyleVar(imgui.constant.StyleVar.ChildRounding, 5.0)
            local childflags = 0 | imgui.constant.WindowFlags.NoTitleBar
            local view_oblist = imgui.BeginChild("##listview", 0, 0, true, childflags)
            imgui.PopStyleVar()
            
            if view_oblist then
                if not D.editor_data.elements.scenegraphview.wait_update then
                    for _, entity in pairs(D.editor_data.elements.scenegraphview.list.objects) do
                        local name = wiscene.Component_GetName(entity)
                        ret_tree = imgui.TreeNode(name.GetName() .. "##" .. entity)
                        if ret_tree then
                            imgui.TreePop()
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
    if D.editor_data.elements.compinspect.win_visible then
        sub_visible, D.editor_data.elements.compinspect.win_visible = imgui.Begin("\xef\x82\x85 Component Inspector", D.editor_data.elements.compinspect.win_visible)
        if sub_visible then
            
        end
        imgui.End()
    end
end

local update_scenegraph = function()
    D.editor_data.elements.scenegraphview.list.objects = wiscene.Entity_GetTransformArray()
    D.editor_data.elements.scenegraphview.wait_update = false
end

local update_sysmenu_actions = function()
    -- File menu actions
    if D.editor_data.actions.resource_new then
        backlog_post("ACT_STUB: new resource not working")
        filedialog(0,"Wicked Engine Scene","wiscene",function(data)
            backlog_post(data.filepath)
        end)
        D.editor_data.actions.resource_new = false
    end
    if D.editor_data.actions.resource_rename then
        D.editor_data.elements.fmenu_rnres.win_visible = true
        D.editor_data.actions.resource_rename = false
    end
    if D.editor_data.actions.resource_save then
        backlog_post("ACT_STUB: save resource not working")
        D.editor_data.actions.resource_save = false
    end
    if D.editor_data.actions.add_object then
        edit_execcmd("add", {type = "object", name = "New Object"})
        D.editor_data.actions.add_object = false
    end
end

local update_navigation = function()
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
    local camera_rot_matrix = matrix.Rotation(D.editor_data.navigation.camera_rot)
    camera_pos_delta = vector.Transform(camera_pos_delta, camera_rot_matrix)

    -- Apply rotation delta
    D.editor_data.navigation.camera_rot.SetX(D.editor_data.navigation.camera_rot.GetX() + camera_rot_delta.GetX())
    D.editor_data.navigation.camera_rot.SetY(D.editor_data.navigation.camera_rot.GetY() + camera_rot_delta.GetY())
    -- Apply movement delta
    D.editor_data.navigation.camera_pos.SetZ(D.editor_data.navigation.camera_pos.GetZ() + camera_pos_delta.GetZ())
    D.editor_data.navigation.camera_pos.SetX(D.editor_data.navigation.camera_pos.GetX() + camera_pos_delta.GetX())
    D.editor_data.navigation.camera_pos.SetY(D.editor_data.navigation.camera_pos.GetY() + camera_pos_delta.GetY())

    -- Camera transform update
    D.editor_data.navigation.camera_transform.ClearTransform()
    D.editor_data.navigation.camera_transform.Translate(D.editor_data.navigation.camera_pos)
    D.editor_data.navigation.camera_transform.Rotate(D.editor_data.navigation.camera_rot)
    D.editor_data.navigation.camera_transform.UpdateTransform()
    D.editor_data.navigation.camera.TransformCamera(D.editor_data.navigation.camera_transform)
    D.editor_data.navigation.camera.UpdateCamera()

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

    edit_execcmd("init")
    editor_dev_griddraw(true)
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