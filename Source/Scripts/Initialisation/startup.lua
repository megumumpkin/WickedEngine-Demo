local scene = GetScene()

Player = {
    model = INVALID_ENTITY,

    Create = function(self, entity)
        self.model = entity
        local model_transform = scene.Component_GetTransform(self.model)
		model_transform.ClearTransform()
		model_transform.UpdateTransform()

		self.target = CreateEntity()
		local target_transform = scene.Component_CreateTransform(self.target)
		target_transform.ClearTransform()
		target_transform.Translate(Vector(0,3))
		
		scene.Component_Attach(self.target, self.model)
    end,

    Input = function(self)
        if(input.Press(KEYBOARD_BUTTON_ESCAPE)) then
            print("Input")
        end
    end
}

local player = Player

function Start()
    LoadModel('Assets/Scenes/Main.wiscene')
    player:Create(LoadModel('Assets/Prefabs/Bird.wiscene'))
end

text_color = Vector(1,1,1,1)

runProcess(function()
    while true do

        if input.Press(KEYBOARD_BUTTON_ESCAPE) then
            text_color = Vector(math.random(), math.random(), math.random(), 1)
        end

        DrawDebugText("Press ESCAPE to change text color!", Vector(0, 2, 0), text_color, 2, DEBUG_TEXT_CAMERA_FACING | DEBUG_TEXT_CAMERA_SCALING)
        
        update()
    end
end)

if pcall(getfenv, 4) then
    error("Running the startup script as a library!")
else
    print("Running the startup script as a program.")

    Start()
end
