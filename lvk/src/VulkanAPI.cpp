#define VMA_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "VulkanAPI.h"
#include "spirv_reflect.h"
#include <cstdint>
#include <algorithm>
#include <fstream>
#include "spdlog/spdlog.h"
#include <array>

using namespace lvk;

// comments are largely snippets from: https://vulkan-tutorial.com/. credit: Alexander Overvoorde

static const bool QUIT_ON_ERROR = false;

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData) {

    //std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;
    if (messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
    {
        spdlog::warn("VL: {0}", pCallbackData->pMessage);
    }
    if (messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
    {
        spdlog::info("VL: {0}", pCallbackData->pMessage);
    }
    if (messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
    {
        spdlog::error("VL: {0}", pCallbackData->pMessage);
        if (!QUIT_ON_ERROR)
        {
            return VK_FALSE;
        }
        VulkanAPI* api = (VulkanAPI*)pUserData;
        api->Quit();
    }
    return VK_FALSE;
}

bool lvk::VulkanAPI::QueueFamilyIndices::IsComplete()
{
    bool foundGraphicsQueue = m_QueueFamilies.find(QueueFamilyType::Graphics) != m_QueueFamilies.end();
    bool foundPresentQueue  = m_QueueFamilies.find(QueueFamilyType::Present) != m_QueueFamilies.end();
    return foundGraphicsQueue && foundPresentQueue;
}

VkApplicationInfo lvk::VulkanAPI::CreateAppInfo()
{
    VkApplicationInfo appInfo{};
    // each struct needs to explicitly be told its type
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Vulkan Example SDL";
    appInfo.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
    appInfo.pEngineName = "None";
    appInfo.engineVersion = VK_MAKE_VERSION(0, 1, 0);
    appInfo.apiVersion = VK_API_VERSION_1_3;

    return appInfo;
}

bool lvk::VulkanAPI::CheckValidationLayerSupport()
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

    for (const char* layerName : p_ValidationLayers) {
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

bool lvk::VulkanAPI::CheckDeviceExtensionSupport(VkPhysicalDevice device)
{
    std::vector<VkExtensionProperties> availableExtensions = GetDeviceAvailableExtensions(device);

    uint32_t requiredExtensionsFound = 0;

    for (auto const& extension : availableExtensions)
    {
        for (auto const& requiredExtensionName : p_DeviceExtensions)
        {
            if (strcmp(requiredExtensionName, extension.extensionName) == 0)
            {
                requiredExtensionsFound++;
            }
        }
    }

    return requiredExtensionsFound >= p_DeviceExtensions.size();

}

void lvk::VulkanAPI::PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
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
    createInfo.pUserData = this; // we can specify some data to pass the callback
                                    // dont need this right now
}

void lvk::VulkanAPI::CreateInstance()
{
    if (p_UseValidation && !CheckValidationLayerSupport())
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

    spdlog::info("Supported m_Instance extensions: ");
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

        if (shouldAdd)
        {
            extensionNames.push_back(extension.extensionName);
        }

    }

    // setup an m_Instance create info with our extensions & app info to create a vulkan m_Instance
    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensionNames.size());
    createInfo.ppEnabledExtensionNames = extensionNames.data();

    if (p_UseValidation)
    {
        VkDebugUtilsMessengerCreateInfoEXT debugMessengerCreateInfo;
        createInfo.enabledLayerCount = (uint32_t)p_ValidationLayers.size();
        createInfo.ppEnabledLayerNames = p_ValidationLayers.data();

        PopulateDebugMessengerCreateInfo(debugMessengerCreateInfo);
        createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugMessengerCreateInfo;
    }
    else
    {
        createInfo.enabledLayerCount = 0;
        createInfo.pNext = nullptr;
    }

    if (vkCreateInstance(&createInfo, nullptr, &m_Instance) != VK_SUCCESS)
    {
        spdlog::error("Failed to create vulkan m_Instance");
    }

}

void lvk::VulkanAPI::Cleanup()
{
    CleanupWindow();
    CleanupVulkan();
}

void lvk::VulkanAPI::SetupDebugOutput()
{
    if (!p_UseValidation) return;

    VkDebugUtilsMessengerCreateInfoEXT createInfo{};
    PopulateDebugMessengerCreateInfo(createInfo);

    PFN_vkVoidFunction rawFunction = vkGetInstanceProcAddr(m_Instance, "vkCreateDebugUtilsMessengerEXT");
    PFN_vkCreateDebugUtilsMessengerEXT function = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(rawFunction);

    if (function == nullptr)
    {
        spdlog::error("Could not find vkCreateDebugUtilsMessengerEXT function");
        std::cerr << "Could not find vkCreateDebugUtilsMessengerEXT function";
    }

    if (function(m_Instance, &createInfo, nullptr, &m_DebugMessenger))
    {
        spdlog::error("Failed to create debug messenger");
        std::cerr << "Failed to create debug messenger";
    }

}

void lvk::VulkanAPI::CleanupDebugOutput()
{
    PFN_vkVoidFunction rawFunction = vkGetInstanceProcAddr(m_Instance, "vkDestroyDebugUtilsMessengerEXT");
    auto function = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(rawFunction);

    if (function != nullptr) {
        function(m_Instance, m_DebugMessenger, nullptr);
    }
    else
    {
        spdlog::error("Could not find vkDestroyDebugUtilsMessengerEXT function");
        std::cerr << "Could not find vkDestroyDebugUtilsMessengerEXT function";
    }
}

void lvk::VulkanAPI::CleanupVulkan()
{
    vmaDestroyAllocator(m_Allocator);
    CleanupSwapChain();

    if (p_UseValidation)
    {
        CleanupDebugOutput();
    }
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        vkDestroySemaphore(m_LogicalDevice, m_ImageAvailableSemaphores[i], nullptr);
        vkDestroySemaphore(m_LogicalDevice, m_RenderFinishedSemaphores[i], nullptr);
        vkDestroyFence(m_LogicalDevice, m_FrameInFlightFences[i], nullptr);
    }

    vkDestroyCommandPool(m_LogicalDevice, m_GraphicsQueueCommandPool, nullptr);
    vkDestroyDescriptorPool(m_LogicalDevice, m_DescriptorPool, nullptr);

    vkDestroyRenderPass(m_LogicalDevice, m_SwapchainImageRenderPass, nullptr);
    

    vkDestroySurfaceKHR(m_Instance, m_Surface, nullptr);
    vkDestroyDevice(m_LogicalDevice, nullptr);
    vkDestroyInstance(m_Instance, nullptr);
}

void lvk::VulkanAPI::Quit()
{
    p_ShouldRun = false;
}

lvk::VulkanAPI::QueueFamilyIndices lvk::VulkanAPI::FindQueueFamilies(VkPhysicalDevice m_PhysicalDevice)
{
    QueueFamilyIndices indices;

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(m_PhysicalDevice, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilyProperties(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(m_PhysicalDevice, &queueFamilyCount, queueFamilyProperties.data());

    for (int i = 0; i < queueFamilyProperties.size(); i++)
    {
        if (queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            indices.m_QueueFamilies.emplace(QueueFamilyType::Graphics, i);
        }

        VkBool32 presentSupport = VK_FALSE;
        vkGetPhysicalDeviceSurfaceSupportKHR(m_PhysicalDevice, i, m_Surface, &presentSupport);

        if (presentSupport == VK_TRUE) {
            indices.m_QueueFamilies[QueueFamilyType::Present] = i;
        }
    }
    return indices;
}

lvk::VulkanAPI::SwapChainSupportDetais lvk::VulkanAPI::GetSwapChainSupportDetails(VkPhysicalDevice physicalDevice)
{
    SwapChainSupportDetais details;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, m_Surface, &details.m_Capabilities);

    uint32_t supportedNumberFormats = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, m_Surface, &supportedNumberFormats, nullptr);

    if (supportedNumberFormats > 0)
    {
        details.m_SupportedFormats.resize(supportedNumberFormats);
        vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, m_Surface, &supportedNumberFormats, details.m_SupportedFormats.data());
    }

    uint32_t supportedPresentModes = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, m_Surface, &supportedPresentModes, nullptr);

    if (supportedNumberFormats > 0)
    {
        details.m_SupportedPresentModes.resize(supportedPresentModes);
        vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, m_Surface, &supportedPresentModes, details.m_SupportedPresentModes.data());
    }

    return details;
}

bool lvk::VulkanAPI::IsDeviceSuitable(VkPhysicalDevice physicalDevice)
{
    QueueFamilyIndices indices = FindQueueFamilies(physicalDevice);
    bool extensionsSupported = CheckDeviceExtensionSupport(physicalDevice);
    bool swapChainSupport = false;
    if (extensionsSupported)
    {
        SwapChainSupportDetais swapChainDetails = GetSwapChainSupportDetails(physicalDevice);
        swapChainSupport = swapChainDetails.m_SupportedFormats.size() > 0 && swapChainDetails.m_SupportedPresentModes.size() > 0;
    }

    VkPhysicalDeviceFeatures supportedFeatures;
    vkGetPhysicalDeviceFeatures(physicalDevice, &supportedFeatures);

    return indices.IsComplete() && extensionsSupported && swapChainSupport && supportedFeatures.samplerAnisotropy;
}

uint32_t lvk::VulkanAPI::AssessDeviceSuitability(VkPhysicalDevice m_PhysicalDevice)
{
    uint32_t score = 0;

    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(m_PhysicalDevice, &deviceProperties);

    VkPhysicalDeviceFeatures deviceFeatures;
    vkGetPhysicalDeviceFeatures(m_PhysicalDevice, &deviceFeatures);

    if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
    {
        score += 1000;
    }

    score += deviceFeatures.geometryShader;
    score += deviceFeatures.shaderStorageImageMultisample;
    score += deviceFeatures.multiViewport;

    return score;
}

void lvk::VulkanAPI::PickPhysicalDevice()
{
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(m_Instance, &deviceCount, nullptr);

    if (deviceCount == 0)
    {
        spdlog::error("Failed to find any physical devices.");
        std::cerr << "Failed to find any physical devices.\n";
    }

    std::vector<VkPhysicalDevice> physicalDevices(deviceCount);
    vkEnumeratePhysicalDevices(m_Instance, &deviceCount, physicalDevices.data());
    VkPhysicalDevice physicalDeviceCandidate = VK_NULL_HANDLE;
    uint32_t bestScore = 0;

    for (int i = 0; i < physicalDevices.size(); i++)
    {
        uint32_t score = AssessDeviceSuitability(physicalDevices[i]);
        if (IsDeviceSuitable(physicalDevices[i]) && score > bestScore)
        {
            physicalDeviceCandidate = physicalDevices[i];
            bestScore = score;
        }
    }

    if (bestScore == 0)
    {
        spdlog::error("failed to find a suitable device.");
        return;
    }

    m_PhysicalDevice = physicalDeviceCandidate;

    if (m_PhysicalDevice == VK_NULL_HANDLE)
    {
        spdlog::error("Failed to find a suitable physical device.");
        std::cerr << "Failed to find a suitable physical device.\n";
    }

    VkPhysicalDeviceProperties deviceProperties{};
    vkGetPhysicalDeviceProperties(m_PhysicalDevice, &deviceProperties);

    spdlog::info("Chose GPU : {0}", deviceProperties.deviceName);
    ListDeviceExtensions(m_PhysicalDevice);

}

void lvk::VulkanAPI::CreateLogicalDevice()
{
    m_QueueFamilyIndices = FindQueueFamilies(m_PhysicalDevice);

    std::vector< VkDeviceQueueCreateInfo> queueCreateInfos;
    float priority = 1.0f;
    for (auto const& [type, index] : m_QueueFamilyIndices.m_QueueFamilies)
    {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.queueFamilyIndex = index;
        queueCreateInfo.pQueuePriorities = &priority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    VkPhysicalDeviceFeatures physicalDeviceFeatures{};
    physicalDeviceFeatures.samplerAnisotropy = VK_TRUE;

    VkDeviceCreateInfo createInfo{};
    createInfo.sType                    = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.pQueueCreateInfos        = queueCreateInfos.data();
    createInfo.queueCreateInfoCount     = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pEnabledFeatures         = &physicalDeviceFeatures;

    createInfo.enabledExtensionCount    = static_cast<uint32_t>(p_DeviceExtensions.size());
    createInfo.ppEnabledExtensionNames  = p_DeviceExtensions.data();

    if (p_UseValidation)
    {
        createInfo.enabledLayerCount    = static_cast<uint32_t>(p_ValidationLayers.size());
        createInfo.ppEnabledLayerNames  = p_ValidationLayers.data();
    }
    else
    {
        createInfo.enabledLayerCount    = 0;
    }

    if (vkCreateDevice(m_PhysicalDevice, &createInfo, nullptr, &m_LogicalDevice))
    {
        spdlog::error("Failed to create logical device.");
        std::cerr << "Failed to create logical device. \n";
    }
}

void lvk::VulkanAPI::GetQueueHandles()
{
    vkGetDeviceQueue(m_LogicalDevice, m_QueueFamilyIndices.m_QueueFamilies[QueueFamilyType::Graphics],      0, &m_GraphicsQueue);
    vkGetDeviceQueue(m_LogicalDevice, m_QueueFamilyIndices.m_QueueFamilies[QueueFamilyType::Present],       0, &m_PresentQueue);
}

VkSurfaceFormatKHR lvk::VulkanAPI::ChooseSwapChainSurfaceFormat(std::vector<VkSurfaceFormatKHR> availableFormats)
{
    if (availableFormats.size() == 0)
    {
        spdlog::error("Could not find any suitable Swapchain Surface Format in provided collection!");
        std::cerr << "Could not find any suitable Swapchain Surface Format in provided collection!" << std::endl;
    }

    for (auto const& format : availableFormats)
    {
        if (format.format == VK_FORMAT_B8G8R8A8_SRGB && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            return format;
        }
    }

    spdlog::error("Could not find suitable Swapchain Surface Format in provided collection!");
    std::cerr << "Could not find suitable Swapchain Surface Format in provided collection!" << std::endl;
    return availableFormats[0];
}

VkPresentModeKHR lvk::VulkanAPI::ChooseSwapChainPresentMode(std::vector<VkPresentModeKHR> availableModes)
{
    for (auto const& presentMode : availableModes)
    {
        if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR)
        {
            return presentMode;
        }
    }
    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D lvk::VulkanAPI::ChooseSwapExtent(VkSurfaceCapabilitiesKHR& surfaceCapabilities)
{
    if(surfaceCapabilities.currentExtent.width != UINT32_MAX)
    {
        return surfaceCapabilities.currentExtent;
    }
    else
    {
        return GetSurfaceExtent(surfaceCapabilities);
    }
}

void lvk::VulkanAPI::CreateSwapChain()
{
    SwapChainSupportDetais swapChainDetails = GetSwapChainSupportDetails(m_PhysicalDevice);

    VkSurfaceFormatKHR format       = ChooseSwapChainSurfaceFormat(swapChainDetails.m_SupportedFormats);
    VkPresentModeKHR presentMode    = ChooseSwapChainPresentMode(swapChainDetails.m_SupportedPresentModes);
    VkExtent2D surfaceExtent        = ChooseSwapExtent(swapChainDetails.m_Capabilities);
    // request one more than minimum supported number of images in swap chain
    // from vk-tutorial : "we may sometimes have to wait on the driver to complete
    // internal operations before we can acquire another image to render to. 
    // Therefore it is recommended to request at least one more image than the minimum"
    uint32_t swapChainImageCount = swapChainDetails.m_Capabilities.minImageCount + 1;
    if (swapChainImageCount > swapChainDetails.m_Capabilities.maxImageCount)
    {
        swapChainImageCount = swapChainDetails.m_Capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface          = m_Surface;
    createInfo.minImageCount    = swapChainImageCount;
    createInfo.clipped          = VK_TRUE;
    createInfo.imageFormat      = format.format;
    createInfo.imageColorSpace  = format.colorSpace;
    createInfo.presentMode      = presentMode;
    createInfo.imageExtent      = surfaceExtent;
    // This is always 1 unless you are developing a stereoscopic 3D application
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    uint32_t queueFamilyIndices[] = { m_QueueFamilyIndices.m_QueueFamilies[QueueFamilyType::Graphics], m_QueueFamilyIndices.m_QueueFamilies[QueueFamilyType::Present] };

    if (m_QueueFamilyIndices.m_QueueFamilies[QueueFamilyType::Graphics] != m_QueueFamilyIndices.m_QueueFamilies[QueueFamilyType::Present])
    {
        createInfo.imageSharingMode         = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount    = 2;
        createInfo.pQueueFamilyIndices      = queueFamilyIndices;
    }
    else
    {
        createInfo.imageSharingMode         = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount    = 0;
        createInfo.pQueueFamilyIndices      = nullptr;
    }
    // flip, rotate, if specified possible in capabilities.
    createInfo.preTransform     = swapChainDetails.m_Capabilities.currentTransform;
    // specifies if the alpha channel should be used for blending with other windows in the window system.
    // You'll almost always want to simply ignore the alpha channel, hence OPAQUE_BIT_KHR.
    createInfo.compositeAlpha   = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.oldSwapchain     = VK_NULL_HANDLE;

    if (vkCreateSwapchainKHR(m_LogicalDevice, &createInfo, nullptr, &m_SwapChain) != VK_SUCCESS)
    {
        spdlog::error("Failed to create swapchain.");
        std::cerr << "Failed to create swapchain" << std::endl;
    }

    vkGetSwapchainImagesKHR(m_LogicalDevice, m_SwapChain, &swapChainImageCount, nullptr);
    m_SwapChainImages.resize(swapChainImageCount);
    vkGetSwapchainImagesKHR(m_LogicalDevice, m_SwapChain, &swapChainImageCount, m_SwapChainImages.data());

    m_SwapChainImageFormat = format.format;
    m_SwapChainImageExtent = surfaceExtent;
}

void lvk::VulkanAPI::CreateSwapChainImageViews()
{
    m_SwapChainImageViews.resize(m_SwapChainImages.size());

    for (uint32_t i = 0; i < m_SwapChainImages.size(); i++)
    {
        CreateImageView(m_SwapChainImages[i], m_SwapChainImageFormat, m_SwapChainImageViews[i]);
    }
}

void lvk::VulkanAPI::CleanupSwapChain()
{
    for (int i = 0; i < m_SwapChainFramebuffers.size(); i++)
    {
        vkDestroyFramebuffer(m_LogicalDevice, m_SwapChainFramebuffers[i], nullptr);
    }
    for (int i = 0; i < m_SwapChainImageViews.size(); i++)
    {
        vkDestroyImageView(m_LogicalDevice, m_SwapChainImageViews[i], nullptr);
    }

    vkDestroySwapchainKHR(m_LogicalDevice, m_SwapChain, nullptr);
}

void lvk::VulkanAPI::RecreateSwapChain()
{
    spdlog::info("resize swapchain function");
    while (vkDeviceWaitIdle(m_LogicalDevice) != VK_SUCCESS);

    CleanupSwapChain();

    CreateSwapChain();
    CreateSwapChainImageViews();
    CreateSwapChainFramebuffers();
}

VkShaderModule lvk::VulkanAPI::CreateShaderModule(const std::vector<char>& data)
{
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = static_cast<uint32_t>(data.size());
    createInfo.pCode    = reinterpret_cast<const uint32_t*>(data.data());
    
    VkShaderModule shaderModule;
    if (vkCreateShaderModule(m_LogicalDevice, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
    {
        spdlog::error("Failed to create shader module!");
        std::cerr << "Failed to create shader module" << std::endl;
    }
    return shaderModule;
}

uint32_t lvk::VulkanAPI::FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(m_PhysicalDevice, &memProperties);
    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }
    return UINT32_MAX;
}

void lvk::VulkanAPI::CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory)
{
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    
#ifdef LVK_USE_VMA
    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
    allocInfo.requiredFlags = properties;

    VmaAllocation allocation;

    vmaCreateBuffer(m_Allocator, &bufferInfo, &allocInfo, &buffer, &allocation, nullptr);

    deviceMemory = allocation->GetMemory();
#endif
    VK_CHECK(vkCreateBuffer(m_LogicalDevice, &bufferInfo, nullptr, &buffer) != VK_SUCCESS)

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(m_LogicalDevice, buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, properties);

    VK_CHECK(vkAllocateMemory(m_LogicalDevice, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS)
    VK_CHECK(vkBindBufferMemory(m_LogicalDevice, buffer, bufferMemory, 0))
    
}

void lvk::VulkanAPI::CreateBufferVMA(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VmaAllocation& allocation)
{
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
    allocInfo.requiredFlags = properties;

    if (properties && VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
    {
        // if we access members of the array non sequentially,
        // we may need to request random access instead of sequential
        // VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT
        allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
    }

    VK_CHECK(vmaCreateBuffer(m_Allocator, &bufferInfo, &allocInfo, &buffer, &allocation, nullptr));
}

void lvk::VulkanAPI::CreateImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory)
{
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = tiling;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = usage;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateImage(m_LogicalDevice, &imageInfo, nullptr, &image) != VK_SUCCESS) {
        throw std::runtime_error("failed to create image!");
    }

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(m_LogicalDevice, image, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, properties);

    VK_CHECK(vkAllocateMemory(m_LogicalDevice, &allocInfo, nullptr, &imageMemory))

    vkBindImageMemory(m_LogicalDevice, image, imageMemory, 0);
}

void lvk::VulkanAPI::CreateImageView(VkImage& image, VkFormat format, VkImageView& imageView)
{
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = format;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    VK_CHECK(vkCreateImageView(m_LogicalDevice, &viewInfo, nullptr, &imageView))
}

void lvk::VulkanAPI::CopyBuffer(VkBuffer& src, VkBuffer& dst, VkDeviceSize size)
{
    // create a new command buffer to record the buffer copy
    VkCommandBuffer commandBuffer = BeginSingleTimeCommands();
   
    // record copy command
    VkBufferCopy copyRegion{};
    copyRegion.srcOffset = 0; // Optional
    copyRegion.dstOffset = 0; // Optional
    copyRegion.size = size;
    vkCmdCopyBuffer(commandBuffer, src, dst, 1, &copyRegion);
    EndSingleTimeCommands(commandBuffer);
}

void lvk::VulkanAPI::CopyBufferToImage(VkBuffer& src, VkImage& image, uint32_t width, uint32_t height)
{
    VkCommandBuffer commandBuffer = BeginSingleTimeCommands();

    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;

    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;

    region.imageOffset = { 0, 0, 0 };
    region.imageExtent = {
        width,
        height,
        1
    };

    vkCmdCopyBufferToImage(commandBuffer, src, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    EndSingleTimeCommands(commandBuffer);
}

void lvk::VulkanAPI::CreateRenderPass()
{
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format          = m_SwapChainImageFormat;
    colorAttachment.samples         = VK_SAMPLE_COUNT_1_BIT;
    // clear image each frame and store contents after rendering.
    colorAttachment.loadOp          = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp         = VK_ATTACHMENT_STORE_OP_STORE;
    // ignore stencil for now
    colorAttachment.stencilLoadOp   = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp  = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    // from vk-tutorial : images need to be transitioned to specific layouts
    // that are suitable for the operation that they're going to be involved in next.
    colorAttachment.initialLayout   = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout     = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentReference{};
    colorAttachmentReference.attachment     = 0;
    colorAttachmentReference.layout         = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount    = 1;
    subpass.pColorAttachments       = &colorAttachmentReference;

    VkSubpassDependency subpassDependency{};
    // implicit subpasses 
    subpassDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    // our pass
    subpassDependency.dstSubpass = 0;
    // wait for the colour output stage to finish
    subpassDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpassDependency.srcAccessMask = 0;
    // wait until we can write to the color attachment
    subpassDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpassDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo createInfo{};
    createInfo.sType            = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    createInfo.attachmentCount  = 1;
    createInfo.pAttachments     = &colorAttachment;
    createInfo.subpassCount     = 1;
    createInfo.pSubpasses       = &subpass;
    createInfo.dependencyCount  = 1;
    createInfo.pDependencies    = &subpassDependency;

    if (vkCreateRenderPass(m_LogicalDevice, &createInfo, nullptr, &m_SwapchainImageRenderPass) != VK_SUCCESS)
    {
        spdlog::error("Failed to create Render Pass!");
        std::cerr << "Failed to create Render Pass!" << std::endl;
    }
}

void lvk::VulkanAPI::CreateSwapChainFramebuffers()
{
    m_SwapChainFramebuffers.resize(m_SwapChainImageViews.size());

    for (uint32_t i = 0; i < m_SwapChainImageViews.size(); i++)
    {
        VkImageView attachments[] =
        {
            m_SwapChainImageViews[i]
        };

        VkFramebufferCreateInfo framebufferCreateInfo{};
        framebufferCreateInfo.sType             = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferCreateInfo.attachmentCount   = 1;
        framebufferCreateInfo.pAttachments      = attachments;
        framebufferCreateInfo.layers = 1;
        framebufferCreateInfo.renderPass        = m_SwapchainImageRenderPass;
        framebufferCreateInfo.height            = m_SwapChainImageExtent.height;
        framebufferCreateInfo.width             = m_SwapChainImageExtent.width;

        if (vkCreateFramebuffer(m_LogicalDevice, &framebufferCreateInfo, nullptr, &m_SwapChainFramebuffers[i]) != VK_SUCCESS)
        {
            spdlog::error("Failed to create Framebuffer!");
            std::cerr << "Failed to create Framebuffer!" << std::endl;
        }
    }
}

void lvk::VulkanAPI::CreateCommandPool()
{
    VkCommandPoolCreateInfo createInfo{};
    createInfo.sType                = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    createInfo.queueFamilyIndex     = m_QueueFamilyIndices.m_QueueFamilies[QueueFamilyType::Graphics];
    createInfo.flags                = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    if(vkCreateCommandPool(m_LogicalDevice, &createInfo, nullptr, &m_GraphicsQueueCommandPool) != VK_SUCCESS)
    {
        spdlog::error("Failed to create Command Pool!");
        std::cerr << "Failed to create Command Pool!" << std::endl;
    }
}

void lvk::VulkanAPI::CreateDescriptorPool()
{
    std::array<VkDescriptorPoolSize, 2> poolSizes{};
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[1].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

    VK_CHECK(vkCreateDescriptorPool(m_LogicalDevice, &poolInfo, nullptr, &m_DescriptorPool) != VK_SUCCESS);
}

void lvk::VulkanAPI::CreateSemaphores()
{
    m_ImageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    m_RenderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    VkSemaphoreCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        if (vkCreateSemaphore(m_LogicalDevice, &createInfo, nullptr, &m_ImageAvailableSemaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(m_LogicalDevice, &createInfo, nullptr, &m_RenderFinishedSemaphores[i]) != VK_SUCCESS)
        {
            spdlog::error("Failed to create semaphores!");
            std::cerr << "Failed to create semaphores!" << std::endl;
        }
    }
    
}

void lvk::VulkanAPI::CreateFences()
{
    m_FrameInFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
    m_ImagesInFlight.resize(m_SwapChainImages.size(), VK_NULL_HANDLE);

    VkFenceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    createInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        if (vkCreateFence(m_LogicalDevice, &createInfo, nullptr, &m_FrameInFlightFences[i]) != VK_SUCCESS)
        {
            spdlog::error("Failed to create Fences!");
            std::cerr << "Failed to create Fences!" << std::endl;
        }
    }
}

void lvk::VulkanAPI::DrawFrame()
{
    vkWaitForFences(m_LogicalDevice, 1, &m_FrameInFlightFences[p_CurrentFrameIndex], VK_TRUE, UINT64_MAX);

    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(m_LogicalDevice, m_SwapChain, UINT64_MAX, m_ImageAvailableSemaphores[p_CurrentFrameIndex], VK_NULL_HANDLE, &imageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        spdlog::info("resizing swapchain");
        RecreateSwapChain();
        return;
    }
    else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        spdlog::error("failed to acquire swap chain image!");
        return;
    }

    vkResetFences(m_LogicalDevice, 1, &m_FrameInFlightFences[p_CurrentFrameIndex]);

     m_ImagesInFlight[imageIndex] = m_FrameInFlightFences[p_CurrentFrameIndex];

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    
    VkSemaphore waitSemaphores[]        = { m_ImageAvailableSemaphores[p_CurrentFrameIndex]};
    VkSemaphore signalSemaphores[]       = { m_RenderFinishedSemaphores[p_CurrentFrameIndex]};
    VkPipelineStageFlags waitStages[]   = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submitInfo.waitSemaphoreCount       = 1;
    submitInfo.pWaitSemaphores          = waitSemaphores;
    submitInfo.pWaitDstStageMask        = waitStages;
    submitInfo.commandBufferCount       = 1;
    submitInfo.pCommandBuffers          = &m_CommandBuffers[imageIndex];
    submitInfo.signalSemaphoreCount     = 1;
    submitInfo.pSignalSemaphores        = signalSemaphores;

    vkResetFences(m_LogicalDevice, 1, &m_FrameInFlightFences[p_CurrentFrameIndex]);

    if (vkQueueSubmit(m_GraphicsQueue, 1, &submitInfo, m_FrameInFlightFences[p_CurrentFrameIndex]) != VK_SUCCESS)
    {
        spdlog::error("Failed to submit draw command buffer!");
        std::cerr << "Failed to submit draw command buffer!" << std::endl;
    }

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType               = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount  = 1;
    presentInfo.pWaitSemaphores     = signalSemaphores;
    presentInfo.swapchainCount      = 1;
    VkSwapchainKHR swapchains[]     = { m_SwapChain };
    presentInfo.pSwapchains         = swapchains;
    presentInfo.pImageIndices       = &imageIndex;
    presentInfo.pResults            = nullptr;

    result = vkQueuePresentKHR(m_GraphicsQueue, &presentInfo);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        RecreateSwapChain();
        return;
    }
    else if (result != VK_SUCCESS) {
        spdlog::error("Error presenting");
    }

    p_CurrentFrameIndex = (p_CurrentFrameIndex + 1) % MAX_FRAMES_IN_FLIGHT;
}

void lvk::VulkanAPI::ListDeviceExtensions(VkPhysicalDevice physicalDevice)
{
    std::vector<VkExtensionProperties> extensions = GetDeviceAvailableExtensions(physicalDevice);

    VkPhysicalDeviceProperties deviceProperties{};
    vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);
    for (int i = 0; i < extensions.size(); i++)
    {
        spdlog::info("Physical Device : {0} : {1}", deviceProperties.deviceName, extensions[i].extensionName);
    }
}

std::vector<VkExtensionProperties> lvk::VulkanAPI::GetDeviceAvailableExtensions(VkPhysicalDevice physicalDevice)
{
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, availableExtensions.data());

    return availableExtensions;


}

std::vector<char> lvk::VulkanAPI::LoadSpirvBinary(const std::string& path)
{
    std::ifstream file(path, std::ios::ate | std::ios::binary);

    if (!file.is_open())
    {
        spdlog::error("Failed to open file at path {0} as binary!", path);
        std::cerr << "Failed to open file!" << std::endl;
        return std::vector<char>();
    }

    size_t fileSize = static_cast<size_t>(file.tellg());
    std::vector<char> data(fileSize);

    file.seekg(0);
    file.read(data.data(), fileSize);

    file.close();
    return data;
}

ShaderModule lvk::VulkanAPI::LoadShaderModule(const std::string& path)
{
    ShaderModule sm{};

    sm.m_Binary = LoadSpirvBinary(path);
    sm.m_DescriptorSetLayoutData = CreateDescriptorSetLayoutDatas(sm.m_Binary);
    return sm;
}

VkDescriptorSet lvk::VulkanAPI::CreateDescriptorSet(DescriptorSetLayoutData& layoutData)
{
    VkDescriptorSet descriptorSet;
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = m_DescriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &layoutData.m_Layout;

    VK_CHECK(vkAllocateDescriptorSets(m_LogicalDevice, &allocInfo, &descriptorSet));

    VkDescriptorBufferInfo bufferInfo{};
    // bufferInfo.buffer = uniformBuffers[i];
    bufferInfo.offset = 0;
    // bufferInfo.range = sizeof(MvpData);

    VkWriteDescriptorSet descriptorWrite{};
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    // descriptorWrite.dstSet = descriptorSets[i];
    descriptorWrite.dstBinding = 0;
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.pBufferInfo = &bufferInfo;
    descriptorWrite.pImageInfo = nullptr; // Optional
    descriptorWrite.pTexelBufferView = nullptr; // Optional

    vkUpdateDescriptorSets(m_LogicalDevice, 1, &descriptorWrite, 0, nullptr);
    return descriptorSet;
}

Vector<DescriptorSetLayoutData> lvk::VulkanAPI::CreateDescriptorSetLayoutDatas(Vector<char>& stageBin)
{
    SpvReflectShaderModule shaderReflectModule;
    SpvReflectResult result = spvReflectCreateShaderModule(stageBin.size(), stageBin.data(), &shaderReflectModule);

    uint32_t descriptorSetCount = 0;
    spvReflectEnumerateDescriptorSets(&shaderReflectModule, &descriptorSetCount, nullptr);

    std::vector<SpvReflectDescriptorSet*> reflectedDescriptorSets;
    reflectedDescriptorSets.resize(descriptorSetCount);
    spvReflectEnumerateDescriptorSets(&shaderReflectModule, &descriptorSetCount, &reflectedDescriptorSets[0]);

    std::vector<DescriptorSetLayoutData> layoutDatas(descriptorSetCount, DescriptorSetLayoutData{});

    for (int i = 0; i < reflectedDescriptorSets.size(); i++)
    {
        const SpvReflectDescriptorSet& reflectedSet = *reflectedDescriptorSets[i];
        DescriptorSetLayoutData& layoutData = layoutDatas[i];

        layoutData.m_Bindings.resize(reflectedSet.binding_count);
        for (int bc = 0; bc < reflectedSet.binding_count; bc++)
        {
            const SpvReflectDescriptorBinding& reflectedBinding = *reflectedSet.bindings[bc];
            VkDescriptorSetLayoutBinding& layoutBinding = layoutData.m_Bindings[bc];
            layoutBinding.binding = reflectedBinding.binding;
            layoutBinding.descriptorType = static_cast<VkDescriptorType>(reflectedBinding.descriptor_type);
            layoutBinding.descriptorCount = 1; // sus
            for (uint32_t i_dim = 0; i_dim < reflectedBinding.array.dims_count; ++i_dim) {
                layoutBinding.descriptorCount *= reflectedBinding.array.dims[i_dim];
            }
            layoutBinding.stageFlags = static_cast<VkShaderStageFlagBits>(shaderReflectModule.shader_stage);
            layoutData.m_BindingDatas.push_back(DescriptorSetLayoutBindingData{ reflectedBinding.block.size });
        }

        layoutData.m_SetNumber = reflectedSet.set;
        layoutData.m_CreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutData.m_CreateInfo.bindingCount = reflectedSet.binding_count;
        layoutData.m_CreateInfo.pBindings = layoutData.m_Bindings.data();

        VK_CHECK(vkCreateDescriptorSetLayout(m_LogicalDevice, &layoutData.m_CreateInfo, nullptr, &layoutData.m_Layout))
    }

    return layoutDatas;
}

// this is probably agnostic of each app, can move to api
void lvk::VulkanAPI::CreateCommandBuffers()
{
    m_CommandBuffers.resize(m_SwapChainFramebuffers.size());

    VkCommandBufferAllocateInfo allocateInfo{};
    allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocateInfo.commandPool = m_GraphicsQueueCommandPool;
    allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocateInfo.commandBufferCount = static_cast<uint32_t>(m_CommandBuffers.size());

    VK_CHECK(vkAllocateCommandBuffers(m_LogicalDevice, &allocateInfo, m_CommandBuffers.data()))

}

void lvk::VulkanAPI::ClearCommandBuffers()
{
    for (uint32_t i = 0; i < m_CommandBuffers.size(); i++)
    {
        vkResetCommandBuffer(m_CommandBuffers[i], 0);
    }
}

void lvk::VulkanAPI::CreateVmaAllocator()
{
    VmaVulkanFunctions vulkanFunctions = {};
    vulkanFunctions.vkGetInstanceProcAddr = &vkGetInstanceProcAddr;
    vulkanFunctions.vkGetDeviceProcAddr = &vkGetDeviceProcAddr;

    VmaAllocatorCreateInfo allocatorCreateInfo = {};
    allocatorCreateInfo.flags = VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT;
    allocatorCreateInfo.vulkanApiVersion = VK_API_VERSION_1_3;
    allocatorCreateInfo.physicalDevice = m_PhysicalDevice;
    allocatorCreateInfo.device = m_LogicalDevice;
    allocatorCreateInfo.instance = m_Instance;
    allocatorCreateInfo.pVulkanFunctions = &vulkanFunctions;

    VK_CHECK(vmaCreateAllocator(&allocatorCreateInfo, &m_Allocator));
}

void lvk::VulkanAPI::InitVulkan()
{
    p_CurrentFrameIndex = 0;
    CreateInstance();
    SetupDebugOutput();
    CreateSurface();
    PickPhysicalDevice();
    CreateLogicalDevice();
    GetQueueHandles();
    CreateSwapChain();
    CreateSwapChainImageViews();
    CreateRenderPass();
    CreateSwapChainFramebuffers();
    CreateCommandPool();
    CreateDescriptorPool();
    CreateSemaphores();
    CreateFences();
    CreateCommandBuffers();
    CreateVmaAllocator();


  
}

VkCommandBuffer lvk::VulkanAPI::BeginSingleTimeCommands()
{
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = m_GraphicsQueueCommandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(m_LogicalDevice, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    return commandBuffer;
}

void lvk::VulkanAPI::EndSingleTimeCommands(VkCommandBuffer& commandBuffer)
{
    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(m_GraphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(m_GraphicsQueue);

    vkFreeCommandBuffers(m_LogicalDevice , m_GraphicsQueueCommandPool, 1, &commandBuffer);
}

void lvk::VulkanAPI::TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout)
{
    VkCommandBuffer commandBuffer = BeginSingleTimeCommands();

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;

    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

    barrier.image = image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;

    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else {
        spdlog::error("Unsupported barrier transition.");
        return;
    }

    vkCmdPipelineBarrier(commandBuffer,
        sourceStage, destinationStage,
        0, /* Dependencies*/
        0, /* Memory Barrier Count*/
        nullptr, /* ptr to memory barriers*/
        0, /* buffer memory barrier count*/
        nullptr, /* Buffer memory barriers*/
        1,  /* Image Memory barrier count*/
        &barrier
    );
    
    EndSingleTimeCommands(commandBuffer);
}

void lvk::VulkanAPI::CreateIndexBuffer(std::vector<uint32_t> indices, VkBuffer& buffer, VmaAllocation& deviceMemory)
{
    VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

    // create a CPU side buffer to dump vertex data into
    VkBuffer stagingBuffer;
    VmaAllocation stagingBufferMemory;
    CreateBufferVMA(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

    // dump vert data
    void* data;
    vmaMapMemory(m_Allocator, stagingBufferMemory, &data);
    memcpy(data, indices.data(), bufferSize);
    vmaUnmapMemory(m_Allocator, stagingBufferMemory);

    // create GPU side buffer
    CreateBufferVMA(bufferSize,
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        buffer, deviceMemory);

    CopyBuffer(stagingBuffer, buffer, bufferSize);

    vkDestroyBuffer(m_LogicalDevice, stagingBuffer, nullptr);
    vmaFreeMemory(m_Allocator, stagingBufferMemory);
}