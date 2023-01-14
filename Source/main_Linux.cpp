#include "stdafx.h"
#include <SDL2/SDL.h>

#include "Config.h"
#include "Core.h"

#ifdef IS_DEV
#include "Dev.h"
#endif

int sdl_loop(wi::Application &application)
{
    SDL_Event event;

    bool quit = false;
    while (!quit)
    {
        SDL_PumpEvents();
        application.Run();

        while( SDL_PollEvent(&event)) 
        {
            bool textinput_action_delete = false;
            switch (event.type) 
            {
                case SDL_QUIT:      
                    quit = true;
                    break;
                case SDL_WINDOWEVENT:
                    switch (event.window.event) 
                    {
                    case SDL_WINDOWEVENT_CLOSE:
                        quit = true;
                        break;
                    case SDL_WINDOWEVENT_RESIZED:
                        application.SetWindow(application.window);
                        break;
                    case SDL_WINDOWEVENT_FOCUS_LOST:
                        // application.is_window_active = false;
                        break;
                    case SDL_WINDOWEVENT_FOCUS_GAINED:
                        // application.is_window_active = true;
                        break;
                    default:
                        break;
                    }
                case SDL_KEYDOWN:
                    if(event.key.keysym.scancode == SDL_SCANCODE_BACKSPACE 
                        || event.key.keysym.scancode == SDL_SCANCODE_DELETE
                        || event.key.keysym.scancode == SDL_SCANCODE_KP_BACKSPACE){
                            wi::gui::TextInputField::DeleteFromInput();
                            textinput_action_delete = true;
                        }
                    break;
                case SDL_TEXTINPUT:
                    if(!textinput_action_delete){
                        if(event.text.text[0] >= 21){
                            wi::gui::TextInputField::AddInput(event.text.text[0]);
                        }
                    }
                    break;
                default:
                    break;
            }
            wi::input::sdlinput::ProcessEvent(event);
        }
    }

    return 0;

}

int main(int argc, char *argv[])
{
#ifdef IS_DEV
    if (Dev::ReadCMD(argc, argv)){
#endif

    AppSettings settings = AppSettings_Load();

    Game::App application;

    // application.infoDisplay.active = true;
    // application.infoDisplay.watermark = true;
    // application.infoDisplay.resolution = true;
    // application.infoDisplay.fpsinfo = true;

    sdl2::sdlsystem_ptr_t system = sdl2::make_sdlsystem(SDL_INIT_EVERYTHING | SDL_INIT_EVENTS);
    sdl2::window_ptr_t window = sdl2::make_window(
            softinfo_title,
            SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
            settings.video_width, settings.video_height,
            SDL_WINDOW_SHOWN | SDL_WINDOW_VULKAN | SDL_WINDOW_ALLOW_HIGHDPI | ((settings.video_fullscreen) ? SDL_WINDOW_FULLSCREEN : 0));

    SDL_Event event;

    application.SetWindow(window.get());

    int ret = sdl_loop(application);

    SDL_Quit();

    return ret;

#ifdef IS_DEV
    }
    return 0;
#endif
}