#include "VulkanAPI_SDL.h"
#include "sdl_helpers.hpp"

int main()
{
    VulkanAPI_SDL api;
    api.CreateWindow(1280, 720);
    api.InitVulkan();
        
    sdl_helpers::RunSDL([&]()
    {

    });

    api.CleanupWindow();
    api.CleanupVulkan();
    return 1;
}