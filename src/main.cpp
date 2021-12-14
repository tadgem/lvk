#include <iostream>

#include <vulkan/vulkan.h>
#include "SDL.h"
#include "SDL_vulkan.h"
#include "glm/glm.hpp"

#include "sdl_helpers.hpp"

#undef main

VkInstance instance; 

void CreateInstance()
{
    VkApplicationInfo appInfo{};
    // each struct needs to explicitly be told its type
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Vulkan Example SDL";
    appInfo.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
    appInfo.pEngineName = "None";
    appInfo.engineVersion = VK_MAKE_VERSION(0, 1, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    // as vulkan is truely platform independent, we need to query the GPU for extensions
    // which tell us what the GPU is capable of doing...
    uint32_t extensionCount = 0;
    const char** extensionNames;
    SDL_Vulkan_GetInstanceExtensions(sdl_helpers::g_Window, &extensionCount, extensionNames);

    // setup an instance create info with our extensions & app info to create a vulkan instance
    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledExtensionCount = extensionCount;
    createInfo.ppEnabledExtensionNames - extensionNames;
    createInfo.enabledLayerCount = 0;

    VkInstance^ 
    vkCreateInstance(&createInfo, NULL, )
   
}

int main()
{
    sdl_helpers::InitSDL([&]()
    {
            CreateInstance();

    });

    
    
    sdl_helpers::RunSDL([&]()
    {

    });
    

    return 1;
}