D("editor_data",{
    elements = {
        resexp = {
            win_visible = false,
            input = "",
        },
        scenegraphview = {
            win_visible = false,
            filter_selected = 0,
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
        resource_new = false,
        resource_rename = false,
        resource_save = false,
        link_model = false,
        add_sound = false,
    },
    navigation = {
        camera = GetCamera(),
        camera_transform = TransformComponent(),
        camera_pos = Vector(0,2,-5),
        camera_rot = Vector(0,0,0),
    }
})

local CAM_MOVE_SPD = 0.3
local CAM_ROT_SPD = 0.2

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
            D.editor_data.actions.import_model = imgui.MenuItem("\xef\x86\xb2 Link Model",nil,D.editor_data.actions.import_model)
            D.editor_data.actions.import_sound = imgui.MenuItem("\xef\x80\xa8 Add Sound",nil,D.editor_data.actions.import_sound)
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
            view_oblist = imgui.BeginChild("##listview", 0, 0, true, childflags)
            imgui.PopStyleVar()
            if view_oblist then
                ret_tree = imgui.TreeNode("ObjectWW")
                if ret_tree then
                    imgui.TreePop()
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

local update_sysmenu_actions = function()
    -- File menu actions
    if D.editor_data.actions.resource_new then
        backlog_post("ACT_STUB: new resource not working")
        D.editor_data.actions.resource_new = false
    end
    if D.editor_data.actions.resource_rename then
        D.editor_data.elements.fmenu_rnres.win_visible = true
        -- backlog_post("ACT_STUB: rename resource not working")
        D.editor_data.actions.resource_rename = false
    end
    if D.editor_data.actions.resource_save then
        backlog_post("ACT_STUB: save resource not working")
        D.editor_data.actions.resource_save = false
    end
end

local update_navigation = function()
    editor_dev_griddraw(true)
    while true do
        local camera_pos_delta = Vector(0.0,0.0,0.0)
        local camera_rot_delta = Vector(0.0,0.0,0.0)
        
        -- Camera movement WASD
        if(input.Down(string.byte('W'))) then
            camera_pos_delta.SetZ(1.0*CAM_MOVE_SPD)
        end
        if(input.Down(string.byte('S'))) then
            camera_pos_delta.SetZ(-1.0*CAM_MOVE_SPD)
        end
        if(input.Down(string.byte('A'))) then
            camera_pos_delta.SetX(-1.0*CAM_MOVE_SPD)
        end
        if(input.Down(string.byte('D'))) then
            camera_pos_delta.SetX(1.0*CAM_MOVE_SPD)
        end

        -- Camera rotation mouse
        if(input.Down(MOUSE_BUTTON_MIDDLE)) then
            
        end

        -- Apply movement delta
        D.editor_data.navigation.camera_pos.SetZ(D.editor_data.navigation.camera_pos.GetZ() + camera_pos_delta.GetZ())
        D.editor_data.navigation.camera_pos.SetX(D.editor_data.navigation.camera_pos.GetX() + camera_pos_delta.GetX())

        -- Camera transform update
        D.editor_data.navigation.camera_transform.ClearTransform()
        D.editor_data.navigation.camera_transform.Translate(D.editor_data.navigation.camera_pos)
        D.editor_data.navigation.camera_transform.UpdateTransform()
        D.editor_data.navigation.camera.TransformCamera(D.editor_data.navigation.camera_transform)
        D.editor_data.navigation.camera.UpdateCamera()

        update()
    end
end

runProcess(function()
    while true do
        update_sysmenu_actions()
        update_navigation()
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