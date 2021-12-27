#include <iostream>

#include <vulkan/vulkan.h>
#include "SDL.h"
#include "SDL_vulkan.h"
#include "glm/glm.hpp"
#include "spdlog/spdlog.h"

#include "sdl_helpers.hpp"

VkInstance instance;
VkSurfaceKHR surface;

const bool use_validation = true;

const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

VkApplicationInfo CreateAppInfo()
{
    VkApplicationInfo appInfo{};
    // each struct needs to explicitly be told its type
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Vulkan Example SDL";
    appInfo.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
    appInfo.pEngineName = "None";
    appInfo.engineVersion = VK_MAKE_VERSION(0, 1, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    return appInfo;
}

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

    for (const char* layerName : validationLayers) {
        bool layerFound = false;

        for (const auto& layerProperties : availableLayers) {
            if (strcmp(layerName, layerProperties.layerName) == 0) {
                layerFound = true;
                break;
            }
        }

        if (!layerFound) {
            return false;
        }
    }

    return true;
}

std::vector<const char*> GetRequiredExtensions()
{
    // as vulkan is truely platform independent, we need to query the GPU for extensions
    // which tell us what the GPU is capable of doing...
    uint32_t extensionCount = 0;
    if (SDL_Vulkan_GetInstanceExtensions(nullptr, &extensionCount, nullptr) != SDL_TRUE)
    {
        spdlog::error("Failed to enumerate instance extensions!");
    }

    std::vector<const char*> extensionNames(extensionCount);
    SDL_Vulkan_GetInstanceExtensions(sdl_helpers::g_Window, &extensionCount, extensionNames.data());

    if (use_validation)
    {
        extensionNames.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    return extensionNames;
}

void CreateInstance()
{
    if (use_validation && !CheckValidationLayerSupport())
    {
        spdlog::error("Validation layers requested but not available.");
        std::cerr << "Validation layers requested but not available.";
    }

    VkApplicationInfo appInfo = CreateAppInfo();

    std::vector<const char*> extensionNames = GetRequiredExtensions();
   
    for (const auto& extension : extensionNames)
    {
        spdlog::info("SDL Extension : {0}", extension);
    }

    uint32_t extensionCount;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
    std::vector<VkExtensionProperties> extensions(extensionCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

    spdlog::info("Supported instance extensions: ");
    for (const auto& extension : extensions)
    {
        spdlog::info("{0}", extension.extensionName);
        extensionNames.push_back(extension.extensionName);
    }
        
    // setup an instance create info with our extensions & app info to create a vulkan instance
    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledExtensionCount = extensionNames.size();
    createInfo.ppEnabledExtensionNames = extensionNames.data();
    if (use_validation)
    {
        createInfo.enabledLayerCount = (uint32_t) validationLayers.size();
        createInfo.ppEnabledLayerNames = validationLayers.data();
    }
    else
    {
        createInfo.enabledLayerCount = 0;
    }
    
    if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS)
    {
        spdlog::error("Failed to create vulkan instance");
    }
   
}

VkSurfaceKHR CreateSurface()
{
    VkSurfaceKHR surface;
    if (!SDL_Vulkan_CreateSurface(sdl_helpers::g_Window, instance, &surface)) 
    {
        spdlog::error("Failed to create SDL Vulkan surface");
        std::cerr << "Failed to create SDL Vulkan surface";
    }
    return surface;
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
            surface = CreateSurface();
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