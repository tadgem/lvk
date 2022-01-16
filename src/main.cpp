#include <iostream>
#include <map>
#include <vulkan/vulkan.h>
#include "SDL.h"
#include "SDL_vulkan.h"
#include "glm/glm.hpp"
#include "spdlog/spdlog.h"

#include "sdl_helpers.hpp"

struct QueueFamilyIndices
{
    std::map<VkQueueFlagBits, uint32_t> m_QueueFamilies;

    bool IsComplete()
    {
        bool foundGraphicsQueue = m_QueueFamilies.find(VK_QUEUE_GRAPHICS_BIT) != m_QueueFamilies.end();
        return foundGraphicsQueue;
    }
};


VkInstance                  instance;
VkSurfaceKHR                surface;
VkDebugUtilsMessengerEXT    debugMessenger;
VkPhysicalDevice            physicalDevice  = VK_NULL_HANDLE;
VkDevice                    logicalDevice   = VK_NULL_HANDLE;

QueueFamilyIndices          queueFamilyIndices;

VkQueue                     graphicsQueue   = VK_NULL_HANDLE;


static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData) {

    //std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;
    if (messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
    {
        spdlog::warn("Validation Layer: {0}", pCallbackData->pMessage);
    }
    if (messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
    {
        spdlog::info("Validation Layer: {0}", pCallbackData->pMessage);
    }
    if (messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
    {
        spdlog::error("Validation Layer: {0}", pCallbackData->pMessage);
    }
    return VK_FALSE;
}

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
    appInfo.apiVersion = VK_API_VERSION_1_1;

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

void PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
{
    createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;

    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT;

    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;

    createInfo.pfnUserCallback = debugCallback;
    createInfo.pUserData = nullptr; // we can specify some data to pass the callback
                                    // dont need this right now
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
        bool shouldAdd = true;
        for (int i = 0; i < extensionNames.size(); i++)
        {
            if (extensionNames[i] == extension.extensionName)
            {
                shouldAdd = false;
            }
        }

        if(shouldAdd)
        {
            extensionNames.push_back(extension.extensionName);
        }
        
    }
        
    // setup an instance create info with our extensions & app info to create a vulkan instance
    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledExtensionCount = extensionNames.size();
    createInfo.ppEnabledExtensionNames = extensionNames.data();

    if (use_validation)
    {
        VkDebugUtilsMessengerCreateInfoEXT debugMessengerCreateInfo;
        createInfo.enabledLayerCount = (uint32_t) validationLayers.size();
        createInfo.ppEnabledLayerNames = validationLayers.data();

        PopulateDebugMessengerCreateInfo(debugMessengerCreateInfo);
        createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*) &debugMessengerCreateInfo;
    }
    else
    {
        createInfo.enabledLayerCount = 0;
        createInfo.pNext = nullptr;
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

void SetupDebugOutput()
{
    if (!use_validation) return;

    VkDebugUtilsMessengerCreateInfoEXT createInfo{};
    PopulateDebugMessengerCreateInfo(createInfo);

    PFN_vkVoidFunction rawFunction = vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    auto function = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(rawFunction);

    if (function == nullptr)
    {
        spdlog::error("Could not find vkCreateDebugUtilsMessengerEXT function");
        std::cerr << "Could not find vkCreateDebugUtilsMessengerEXT function";
    }

    if (function(instance, &createInfo, nullptr, &debugMessenger))
    {
        spdlog::error("Failed to create debug messenger");
        std::cerr << "Failed to create debug messenger";
    }

}

void CleanupDebugOutput()
{
    PFN_vkVoidFunction rawFunction = vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    auto function = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(rawFunction);

    if (function != nullptr) {
        function(instance, debugMessenger, nullptr);
    }
    else
    {
        spdlog::error("Could not find vkDestroyDebugUtilsMessengerEXT function");
        std::cerr << "Could not find vkDestroyDebugUtilsMessengerEXT function";
    }
}

void CleanupVulkan()
{
    if (use_validation)
    {
        CleanupDebugOutput();
    }
    vkDestroyDevice(logicalDevice, nullptr);
    vkDestroyInstance(instance, nullptr);
}

QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice physicalDevice)
{
    QueueFamilyIndices indices;

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilyProperties(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilyProperties.data());

    for (int i = 0; i < queueFamilyProperties.size(); i++)
    {
        if (queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            indices.m_QueueFamilies.emplace(VK_QUEUE_GRAPHICS_BIT, i);
        }
    }
    return indices;
}

bool IsDeviceSuitable(VkPhysicalDevice physicalDevice)
{
    QueueFamilyIndices indices = FindQueueFamilies(physicalDevice);
    return indices.IsComplete();
}

uint32_t AssessDeviceSuitability(VkPhysicalDevice physicalDevice)
{
    uint32_t score = 0;

    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);

    VkPhysicalDeviceFeatures deviceFeatures;
    vkGetPhysicalDeviceFeatures(physicalDevice, &deviceFeatures);

    if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
    {
        score += 1000;
    }

    score += deviceFeatures.geometryShader;
    score += deviceFeatures.shaderStorageImageMultisample;
    score += deviceFeatures.multiViewport;

    return score;
}

void PickPhysicalDevice()
{
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

    if (deviceCount == 0)
    {
        spdlog::error("Failed to find any physical devices.");
        std::cerr << "Failed to find any physical devices.\n";
    }

    std::vector<VkPhysicalDevice> physicalDevices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, physicalDevices.data());

    std::unordered_map<uint32_t, VkPhysicalDevice> deviceScores;

    for (int i = 0; i < physicalDevices.size(); i++)
    {
        uint32_t score = AssessDeviceSuitability(physicalDevices[i]);
        if (IsDeviceSuitable(physicalDevices[i]))
        {
            deviceScores.emplace(score, physicalDevices[i]);
        }
    }

    if (deviceScores.begin()->first > 0)
    {
        physicalDevice = deviceScores.begin()->second;
    }

    if (physicalDevice == VK_NULL_HANDLE)
    {
        spdlog::error("Failed to find a suitable physical device.");
        std::cerr << "Failed to find a suitable physical device.\n";
    }

}

void CreateLogicalDevice()
{
    queueFamilyIndices = FindQueueFamilies(physicalDevice);

    VkDeviceQueueCreateInfo queueCreateInfo{};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueCount = queueFamilyIndices.m_QueueFamilies.size();
    queueCreateInfo.queueFamilyIndex = queueFamilyIndices.m_QueueFamilies[VK_QUEUE_GRAPHICS_BIT];

    float priority = 1.0f;
    queueCreateInfo.pQueuePriorities = &priority;

    VkPhysicalDeviceFeatures physicalDeviceFeatures{};

    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.pQueueCreateInfos = &queueCreateInfo;
    createInfo.queueCreateInfoCount = 1;

    createInfo.pEnabledFeatures = &physicalDeviceFeatures;
    
    createInfo.enabledExtensionCount = 0;

    if (use_validation)
    {
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();
    }
    else
    {
        createInfo.enabledLayerCount = 0;
    }

    if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &logicalDevice))
    {
        spdlog::error("Failed to create logical device.");
        std::cerr << "Failed to create logical device. \n";
    }
}

void GetQueueHandles()
{
    vkGetDeviceQueue(logicalDevice, queueFamilyIndices.m_QueueFamilies[VK_QUEUE_GRAPHICS_BIT], 0, &graphicsQueue);
}

void InitVulkan()
{
    CreateInstance();
    SetupDebugOutput();
    PickPhysicalDevice();
    CreateLogicalDevice();
    GetQueueHandles();
}

int main()
{
    sdl_helpers::InitSDL([&]()
    {
            InitVulkan();
    });

    auto cleanup_callback = [&]()
    {
        CleanupVulkan();
    };
    
    sdl_helpers::RunSDL([&]()
    {

    }, cleanup_callback);

    return 1;
}