#include "VulkanAPI_SDL.h"

int main()
{
    VulkanAPI_SDL api;
    api.CreateWindow(1280, 720);
    api.InitVulkan();
        
    api.Run([&]()
    {

    });

    api.CleanupWindow();
    api.CleanupVulkan();
    return 1;
}