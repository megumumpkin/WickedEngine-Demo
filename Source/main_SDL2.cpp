// WickedEngineTests.cpp : Defines the entry point for the application.
//

#include "Distribution/Icon.h"
#include "sdl2.h"
#include "stdafx.h"

#include <iostream>
#include <SDL2/SDL.h>

#if IS_DEV
#include "ImGui/imgui_impl_sdl.h"
#endif

using std::cout;

int sdl_loop(Game::Application &app)
{
    SDL_Event event;

    bool quit = false;
    while (!quit)
    {
        app.Run();
        SDL_Event event;
        while(SDL_PollEvent(&event)) {
            bool textinput_action_delete = false; // Temporary script input
            switch (event.type) {
            case SDL_QUIT:
                quit = true;
                break;
            case SDL_WINDOWEVENT:
                switch (event.window.event) {
                case SDL_WINDOWEVENT_CLOSE:   // exit game
                    quit = true;
                    break;
                case SDL_WINDOWEVENT_RESIZED:
                    // Tells the engine to reload window configuration (size and dpi)
                    app.SetWindow(app.window);
                    break;
                default:
                    break;
                }
            // Temporary script input ----------------------------------
            case SDL_KEYDOWN:
                if(event.key.keysym.scancode == SDL_SCANCODE_BACKSPACE 
                    || event.key.keysym.scancode == SDL_SCANCODE_DELETE
                    || event.key.keysym.scancode == SDL_SCANCODE_KP_BACKSPACE){
                        if (wi::backlog::isActive())
                            wi::backlog::deletefromInput();
                        wi::gui::TextInputField::DeleteFromInput();
                        textinput_action_delete = true;
                    }
                break;
            case SDL_TEXTINPUT:
                if(!textinput_action_delete){
                    if(event.text.text[0] >= 21){
                        if (wi::backlog::isActive())
                            wi::backlog::input(event.text.text[0]);
                        wi::gui::TextInputField::AddInput(event.text.text[0]);
                    }
                }
                break;
            // ---------------------------------------------------------
            default:
                break;
            }
            wi::input::sdlinput::ProcessEvent(event);
#if IS_DEV
            ImGui_ImplSDL2_ProcessEvent(&event);
#endif
        }
    }

    return 0;
}

/*
void set_window_icon(SDL_Window *window) {
    // these masks are necessary to tell SDL_CreateRGBSurface(From)
    // to assume the data it gets is byte-wise RGB(A) data
    Uint32 rmask, gmask, bmask, amask;
  
    #if SDL_BYTEORDER == SDL_BIG_ENDIAN
    int shift = (gimp_image.bytes_per_pixel == 3) ? 8 : 0;
    rmask = 0xff000000 >> shift;
    gmask = 0x00ff0000 >> shift;
    bmask = 0x0000ff00 >> shift;
    amask = 0x000000ff >> shift;
    #else // little endian, like x86
    rmask = 0x000000ff;
    gmask = 0x0000ff00;
    bmask = 0x00ff0000;
    amask = (gimp_image.bytes_per_pixel == 3) ? 0 : 0xff000000;
    #endif

    SDL_Surface *icon = SDL_CreateRGBSurfaceFrom(
        (void *)gimp_image.pixel_data, gimp_image.width, gimp_image.height,
        gimp_image.bytes_per_pixel * 8,
        gimp_image.bytes_per_pixel * gimp_image.width, rmask, gmask, bmask,
        amask);

    SDL_SetWindowIcon(window, icon);

    SDL_FreeSurface(icon);
}
*/

int main(int argc, char *argv[])
{
    Game::Application app;

    wi::arguments::Parse(argc, argv);

    sdl2::sdlsystem_ptr_t system = sdl2::make_sdlsystem(SDL_INIT_EVERYTHING | SDL_INIT_EVENTS);
    if (!system) {
        throw sdl2::SDLError("Error creating SDL2 system");
    }

    sdl2::window_ptr_t window = sdl2::make_window(
            Game::GetApplicationName(),
            SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
            1280, 800,
            SDL_WINDOW_SHOWN | SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
    if (!window) {
        throw sdl2::SDLError("Error creating window");
    }

    app.SetWindow(window.get());

    int ret = sdl_loop(app);

    SDL_Quit();
    return ret;
}
