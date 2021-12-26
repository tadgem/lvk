#include <iostream>

#include <vulkan/vulkan.h>
#include "SDL.h"
#include "SDL_vulkan.h"
#include "glm/glm.hpp"
#include "spdlog/spdlog.h"

#include "sdl_helpers.hpp"

VkInstance instance;

const bool use_validation = true;

bool CheckValidationLayerSupport()
{
    uint32_t layerCount;
    if (vkEnumerateInstanceLayerProperties(&layerCount, nullptr) != VK_SUCCESS)
    {
        spdlog::error("Failed to enumerate supported validation layers!");
    }

    std::vector<VkLayerProperties> availableLayers(layerCount);
    if (vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data()) != VK_SUCCESS)
    {
        spdlog::error("Failed to enumerate supported validation layers!");
    }
    return false;
}

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
    if (vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr) != VK_SUCCESS)
    {
        spdlog::error("Failed to enumerate instance extensions!");
    }
    
    std::vector<const char*> extensionNames(extensionCount);
    SDL_Vulkan_GetInstanceExtensions(sdl_helpers::g_Window, &extensionCount, extensionNames.data());

    std::vector<VkExtensionProperties> extensions(extensionCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

    spdlog::info("Supported instance extensions: ");
    for (VkExtensionProperties extensionProps : extensions)
    {
        spdlog::info("{0} spec {1}", extensionProps.extensionName, extensionProps.specVersion);
    }

    // setup an instance create info with our extensions & app info to create a vulkan instance
    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledExtensionCount = extensionCount;
    createInfo.ppEnabledExtensionNames = extensionNames.data();
    createInfo.enabledLayerCount = 0;

    
    if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS)
    {
        spdlog::error("Failed to create vulkan instance");
    }
   
}

void Cleanup()
{
    vkDestroyInstance(instance, nullptr);
}

int main()
{

    sdl_helpers::InitSDL([&]()
    {
            CreateInstance();

    });

    auto cleanup_callback = [&]()
    {
        Cleanup();
    };
    
    sdl_helpers::RunSDL([&]()
    {

    }, cleanup_callback);
    

    return 1;
}