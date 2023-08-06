#include "Gameplay.h"

#include "Scene.h"

#include <functional>
#include <iostream>

#ifdef _WIN32
HINSTANCE gameplay_lib;
#else
void* gameplay_lib = NULL;
#endif

namespace Gameplay
{
    wi::unordered_map<std::string, std::unique_ptr<GameplayHook>> gameplay_hooks = {};
    wi::vector<std::function<void(wi::scene::Scene*)>> scene_init_hooks;

    void Init()
    {
        typedef void (*lib_hook_t)();

#ifdef IS_DEV
        static const std::string library_name = "Gameplay_DEV";
#else
        static const std::string library_name = "Gameplay";
#endif
        // Load library
#ifdef _WIN32
        auto lib_str = library_name+".dll";
#else
        auto lib_str = "./lib"+library_name+".so";
#endif
        gameplay_lib = wiLoadLibrary(lib_str.c_str());
        lib_hook_t lib_hook = (lib_hook_t)wiGetProcAddress(gameplay_lib, "Gameplay_Hook");
        lib_hook();

        // Init all hooks
        for(auto& gameplay_hook : gameplay_hooks)
        {
            gameplay_hook.second->Hook_Init();
        }
    }

    void SceneInit(wi::scene::Scene* scene)
    {
        for(auto& scene_init_hook : scene_init_hooks)
        {
            scene_init_hook(scene);
        }
    }

    void PreUpdate(float dt)
    {
        // Update all hooks
        for(auto& gameplay_hook : gameplay_hooks)
        {
            gameplay_hook.second->Hook_PreUpdate(dt);
        }
    }

    void Update(float dt)
    {
        // Update all hooks
        for(auto& gameplay_hook : gameplay_hooks)
        {
            gameplay_hook.second->Hook_Update(dt);
        }
    }

    void FixedUpdate()
    {
        // Update all hooks
        for(auto& gameplay_hook : gameplay_hooks)
        {
            gameplay_hook.second->Hook_FixedUpdate();
        }
    }

    void Deinit()
    {
        gameplay_hooks.clear();
#ifdef _WIN32
        FreeLibrary((HINSTANCE)gameplay_lib)
#else
        dlclose(gameplay_lib);
#endif
    }
}