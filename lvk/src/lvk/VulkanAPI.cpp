#define VMA_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include <cstdint>
#include <algorithm>
#include <fstream>
#include <array>
#include "lvk/VulkanAPI.h"
#include "ThirdParty/spirv_reflect.h"
#include "spdlog/spdlog.h"
#include "ImGui/imgui_impl_vulkan.h"
#include "lvk/Texture.h"
#include "lvk/Mesh.h"
#include "lvk/Shader.h"

//#ifdef WIN32
//#include "windows.h"
//#include <dwmapi.h>
//#endif

using namespace lvk;

static const bool QUIT_ON_ERROR = false;

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData) {

    //std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;
    if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
    {
        spdlog::warn("VL: {}", pCallbackData->pMessage);
    }
    if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
    {
        spdlog::info("VL: {}", pCallbackData->pMessage);
    }
    if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
    {
        spdlog::error("VL: {}", pCallbackData->pMessage);
        if (!QUIT_ON_ERROR)
        {
            return VK_FALSE;
        }
        VulkanAPI* api = (VulkanAPI*)pUserData;
        api->Quit();
    }
    return VK_FALSE;
}

void lvk::MappedBuffer::Free(VulkanAPI& vk)
{
    vmaUnmapMemory(vk.m_Allocator, m_GpuMemory);
    vkDestroyBuffer(vk.m_LogicalDevice, m_GpuBuffer, nullptr);
    vmaFreeMemory(vk.m_Allocator, m_GpuMemory);
}


void lvk::ShaderBufferFrameData::Free(lvk::VulkanAPI& vk)
{
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        m_UniformBuffers[i].Free(vk);
    }

    m_UniformBuffers.clear();
}

bool lvk::VulkanAPI::QueueFamilyIndices::IsComplete()
{
    bool foundGraphicsQueue = m_QueueFamilies.find(QueueFamilyType::GraphicsAndCompute) != m_QueueFamilies.end();
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
    if (m_UseValidation && !CheckValidationLayerSupport())
    {
        spdlog::error("Validation layers requested but not available.");
        std::cerr << "Validation layers requested but not available.";
    }

    VkApplicationInfo appInfo = CreateAppInfo();

    std::vector<const char*> extensionNames = GetRequiredExtensions();

    for (const auto& extension : extensionNames)
    {
        spdlog::info("SDL Extension : {}", extension);
    }

    uint32_t extensionCount;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
    std::vector<VkExtensionProperties> extensions(extensionCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

    spdlog::info("Supported m_Instance extensions: ");
    for (const auto& extension : extensions)
    {
        spdlog::info("{}", &extension.extensionName[0]);
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

    if (m_UseValidation)
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
    Texture::FreeDefaultTexture(*this);
    Mesh::FreeScreenQuad(*this);
    CleanupImGui();
    CleanupWindow();
    CleanupVulkan();
}

void lvk::VulkanAPI::SetupDebugOutput()
{
    if (!m_UseValidation) return;

    VkDebugUtilsMessengerCreateInfoEXT createInfo{};
    PopulateDebugMessengerCreateInfo(createInfo);

    PFN_vkVoidFunction rawFunction = vkGetInstanceProcAddr(m_Instance, "vkCreateDebugUtilsMessengerEXT");
    PFN_vkCreateDebugUtilsMessengerEXT function = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(rawFunction);

    if (!function)
    {
        spdlog::error("Could not find vkCreateDebugUtilsMessengerEXT function");
        std::cerr << "Could not find vkCreateDebugUtilsMessengerEXT function";
        return;
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

    m_DescriptorSetAllocator.Free(m_LogicalDevice);

    if (m_UseValidation)
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
        if (queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT && queueFamilyProperties[i].queueFlags & VK_QUEUE_COMPUTE_BIT)
        {
            indices.m_QueueFamilies.emplace(QueueFamilyType::GraphicsAndCompute, i);
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

    return indices.IsComplete() && extensionsSupported && swapChainSupport && supportedFeatures.samplerAnisotropy && supportedFeatures.wideLines;
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

    spdlog::info("Chose GPU : {}", &deviceProperties.deviceName[0]);
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
    physicalDeviceFeatures.sampleRateShading = VK_TRUE;
    physicalDeviceFeatures.fillModeNonSolid = VK_TRUE;
    physicalDeviceFeatures.wideLines = VK_TRUE;

    VkDeviceCreateInfo createInfo{};
    createInfo.sType                    = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.pQueueCreateInfos        = queueCreateInfos.data();
    createInfo.queueCreateInfoCount     = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pEnabledFeatures         = &physicalDeviceFeatures;

    createInfo.enabledExtensionCount    = static_cast<uint32_t>(p_DeviceExtensions.size());
    createInfo.ppEnabledExtensionNames  = p_DeviceExtensions.data();

    if (m_UseValidation)
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
    vkGetDeviceQueue(m_LogicalDevice, m_QueueFamilyIndices.m_QueueFamilies[QueueFamilyType::GraphicsAndCompute],    0, &m_GraphicsQueue);
    vkGetDeviceQueue(m_LogicalDevice, m_QueueFamilyIndices.m_QueueFamilies[QueueFamilyType::GraphicsAndCompute],    0, &m_ComputeQueue);
    vkGetDeviceQueue(m_LogicalDevice, m_QueueFamilyIndices.m_QueueFamilies[QueueFamilyType::Present],               0, &m_PresentQueue);
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
        if (format.format == VK_FORMAT_B8G8R8A8_UNORM )
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

VkExtent2D lvk::VulkanAPI::GetMaxFramebufferExtent()
{
    return p_MaxFramebufferExtent;
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
    uint32_t swapChainImageCount = swapChainDetails.m_Capabilities.minImageCount;
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

    uint32_t queueFamilyIndices[] = { m_QueueFamilyIndices.m_QueueFamilies[QueueFamilyType::GraphicsAndCompute], m_QueueFamilyIndices.m_QueueFamilies[QueueFamilyType::Present] };

    if (m_QueueFamilyIndices.m_QueueFamilies[QueueFamilyType::GraphicsAndCompute] != m_QueueFamilyIndices.m_QueueFamilies[QueueFamilyType::Present])
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
        CreateImageView(m_SwapChainImages[i], m_SwapChainImageFormat, 1, VK_IMAGE_ASPECT_COLOR_BIT,  m_SwapChainImageViews[i]);
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

    vkDestroyImageView(m_LogicalDevice, m_SwapChainColourImageView, nullptr);
    vkDestroyImage(m_LogicalDevice, m_SwapChainColourImage, nullptr);
    vkFreeMemory(m_LogicalDevice, m_SwapChainColourImageMemory, nullptr);

    vkDestroyImageView(m_LogicalDevice, m_SwapChainDepthImageView, nullptr);
    vkDestroyImage(m_LogicalDevice, m_SwapChainDepthImage, nullptr);
    vkFreeMemory(m_LogicalDevice, m_SwapChainDepthImageMemory, nullptr);

    vkDestroySwapchainKHR(m_LogicalDevice, m_SwapChain, nullptr);
}

void lvk::VulkanAPI::RecreateSwapChain()
{
    spdlog::info("resize swapchain function");
    while (vkDeviceWaitIdle(m_LogicalDevice) != VK_SUCCESS);

    CleanupSwapChain();
    CleanupImGui();
    
    CreateSwapChain();
    CreateSwapChainImageViews();
    CreateSwapChainColourTexture(m_EnableSwapchainMsaa);
    CreateSwapChainDepthTexture(m_EnableSwapchainMsaa);
    CreateSwapChainFramebuffers();

    InitImGui();
}

void lvk::VulkanAPI::CreateSwapChainColourTexture(bool enableMsaa)
{
    VkSampleCountFlagBits sampleCount = VK_SAMPLE_COUNT_1_BIT;
    
    if (enableMsaa)
    {
        sampleCount = m_MaxMsaaSamples;
    }

    CreateImage(m_SwapChainImageExtent.width, m_SwapChainImageExtent.height, 1, sampleCount,
        m_SwapChainImageFormat,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        m_SwapChainColourImage,
        m_SwapChainColourImageMemory);

    CreateImageView(m_SwapChainColourImage, m_SwapChainImageFormat, 1, VK_IMAGE_ASPECT_COLOR_BIT, m_SwapChainColourImageView);
}

void lvk::VulkanAPI::CreateSwapChainDepthTexture(bool enableMsaa )
{
    VkFormat depthFormat = FindDepthFormat();

    VkSampleCountFlagBits sampleCount = VK_SAMPLE_COUNT_1_BIT;

    if (enableMsaa)
    {
        sampleCount = m_MaxMsaaSamples;
    }

    CreateImage(m_SwapChainImageExtent.width, m_SwapChainImageExtent.height, 1, sampleCount,
        depthFormat,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        m_SwapChainDepthImage,
        m_SwapChainDepthImageMemory);

    CreateImageView(m_SwapChainDepthImage, depthFormat, 1, VK_IMAGE_ASPECT_DEPTH_BIT, m_SwapChainDepthImageView);
    TransitionImageLayout(m_SwapChainDepthImage, depthFormat, 1, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
}

VkShaderModule lvk::VulkanAPI::CreateShaderModule(const StageBinary& data)
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

VkFormat lvk::VulkanAPI::FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features)
{
    for (VkFormat format : candidates) {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(m_PhysicalDevice, format, &props);
        if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
            return format;
        }
        else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
            return format;
        }
    }
    spdlog::error("Failed to find appropriate supported format from candidates");
    return VkFormat{};

}

VkFormat lvk::VulkanAPI::FindDepthFormat()
{
    return FindSupportedFormat(
        { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
        VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
    );
}

bool lvk::VulkanAPI::HasStencilComponent(VkFormat& format)
{
    return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
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

void lvk::VulkanAPI::CreateImage(uint32_t width, uint32_t height, uint32_t numMips, VkSampleCountFlagBits sampleCount, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory)
{
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = numMips;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = tiling;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = usage;
    imageInfo.samples = sampleCount;
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

void lvk::VulkanAPI::CreateImageView(VkImage& image, VkFormat format, uint32_t numMips, VkImageAspectFlags aspectFlags, VkImageView& imageView)
{
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = format;
    viewInfo.subresourceRange.aspectMask = aspectFlags;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = numMips;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    VK_CHECK(vkCreateImageView(m_LogicalDevice, &viewInfo, nullptr, &imageView))
}

void lvk::VulkanAPI::CreateImageSampler(VkImageView& imageView, uint32_t numMips, VkFilter filterMode, VkSamplerAddressMode addressMode, VkSampler& sampler)
{
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = filterMode;
    samplerInfo.minFilter = filterMode;

    samplerInfo.addressModeU = addressMode;
    samplerInfo.addressModeV = addressMode;
    samplerInfo.addressModeW = addressMode;

    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(m_PhysicalDevice, &properties);

    samplerInfo.anisotropyEnable = VK_TRUE;
    samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;

    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f; // static_cast<float>(numMips / 2); to test mips are working
    samplerInfo.maxLod = static_cast<float>(numMips);

    VK_CHECK(vkCreateSampler(m_LogicalDevice, &samplerInfo, nullptr, &sampler))
}

void lvk::VulkanAPI::CreateTexture(const String& path, VkFormat format, VkImage& image, VkImageView& imageView, VkDeviceMemory& imageMemory, uint32_t* numMips)
{
    bool generateMips = numMips != nullptr;

    int texWidth, texHeight, texChannels;
    stbi_uc* pixels = stbi_load(path.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    VkDeviceSize imageSize = texWidth * texHeight * 4;

    if (!pixels)
    {
        spdlog::error("Failed to load texture image at path {}", path);
        return;
    }

    uint32_t mips = 1;
    if (generateMips)
    {
        mips = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;
        *numMips = mips;
    }
    
    // create staging buffer to copy texture to gpu
    VkBuffer stagingBuffer;
    VmaAllocation stagingBufferMemory;
    constexpr VkBufferUsageFlags bufferUsageFlags = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    constexpr VkMemoryPropertyFlags memoryPropertiesFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    CreateBufferVMA(imageSize, bufferUsageFlags, memoryPropertiesFlags, stagingBuffer, stagingBufferMemory);

    void* data;
    vmaMapMemory(m_Allocator, stagingBufferMemory, &data);
    memcpy(data, pixels, static_cast<size_t>(imageSize));
    vmaUnmapMemory(m_Allocator, stagingBufferMemory);
    stbi_image_free(pixels);

    CreateImage(texWidth, texHeight, mips, VK_SAMPLE_COUNT_1_BIT,
        format, VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        image, imageMemory);
    CreateImageView(image, format, mips, VK_IMAGE_ASPECT_COLOR_BIT, imageView);

    TransitionImageLayout(image, format, mips, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    CopyBufferToImage(stagingBuffer, image, texWidth, texHeight);
    
    GenerateMips(image, format, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texWidth), mips, VK_FILTER_LINEAR);
    
    TransitionImageLayout(image, format, mips, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    vkDestroyBuffer(m_LogicalDevice, stagingBuffer, nullptr);
    vmaFreeMemory(m_Allocator, stagingBufferMemory);
}

void lvk::VulkanAPI::CreateTextureFromMemory(unsigned char* tex_data, uint32_t dataSize, VkFormat format, VkImage& image, VkImageView& imageView, VkDeviceMemory& imageMemory, uint32_t* numMips)
{
    bool generateMips = numMips != nullptr;

    int texWidth, texHeight, texChannels;
    stbi_uc* pixels = stbi_load_from_memory(tex_data, dataSize, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    VkDeviceSize imageSize = texWidth * texHeight * 4;

    if (!pixels)
    {
        spdlog::error("Failed to load texture image from memory");
        return;
    }

    uint32_t mips = 1;
    if (generateMips)
    {
        mips = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;
        *numMips = mips;
    }

    // create staging buffer to copy texture to gpu
    VkBuffer stagingBuffer;
    VmaAllocation stagingBufferMemory;
    constexpr VkBufferUsageFlags bufferUsageFlags = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    constexpr VkMemoryPropertyFlags memoryPropertiesFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    CreateBufferVMA(imageSize, bufferUsageFlags, memoryPropertiesFlags, stagingBuffer, stagingBufferMemory);

    void* data;
    vmaMapMemory(m_Allocator, stagingBufferMemory, &data);
    memcpy(data, pixels, static_cast<size_t>(imageSize));
    vmaUnmapMemory(m_Allocator, stagingBufferMemory);
    stbi_image_free(pixels);

    CreateImage(texWidth, texHeight, mips, VK_SAMPLE_COUNT_1_BIT,
        format, VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        image, imageMemory);
    CreateImageView(image, format, mips, VK_IMAGE_ASPECT_COLOR_BIT, imageView);

    TransitionImageLayout(image, format, mips, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    CopyBufferToImage(stagingBuffer, image, texWidth, texHeight);

    GenerateMips(image, format, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texWidth), mips, VK_FILTER_LINEAR);

    TransitionImageLayout(image, format, mips, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    vkDestroyBuffer(m_LogicalDevice, stagingBuffer, nullptr);
    vmaFreeMemory(m_Allocator, stagingBufferMemory);
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

std::vector<VkDescriptorSetLayoutBinding> CleanDescriptorSetLayout(std::vector<VkDescriptorSetLayoutBinding>& arr)
{
    std::vector<VkDescriptorSetLayoutBinding> clean;

    for (int k = 0; k < arr.size(); k++)
    {
        auto& layoutSetData = arr[k];
        if (clean.empty())
        {
            clean.push_back(layoutSetData);
            continue;
        }

        int start = static_cast<int>(clean.size() - 1);
        for (int i = start; i >= 0; i--)
        {
            auto& newLayoutSet = clean[i];
            if (newLayoutSet.binding == layoutSetData.binding)
            {
                if (newLayoutSet.descriptorCount == layoutSetData.descriptorCount &&
                    newLayoutSet.descriptorType == layoutSetData.descriptorType)
                {
                    newLayoutSet.stageFlags = layoutSetData.stageFlags + newLayoutSet.stageFlags;
                    continue;
                }
            }
            
            clean.push_back(layoutSetData);
            break;
            
        }
    }

    return clean;
}

void lvk::VulkanAPI::CreateDescriptorSetLayout(std::vector<DescriptorSetLayoutData>& vertLayoutDatas, std::vector<DescriptorSetLayoutData>& fragLayoutDatas, VkDescriptorSetLayout& descriptorSetLayout)
{
    std::vector<VkDescriptorSetLayoutBinding> bindings;
    uint8_t count = 0;

    for (auto& vertLayoutData : vertLayoutDatas)
    {
        count += static_cast<uint8_t>(vertLayoutData.m_Bindings.size());
    }

    for (auto& fragLayoutData : fragLayoutDatas)
    {
        count += static_cast<uint8_t>(fragLayoutData.m_Bindings.size());
    }

    bindings.resize(count);

    count = 0;
    // .. do the things
    for (auto& vertLayoutData : vertLayoutDatas)
    {
        for (auto& binding : vertLayoutData.m_Bindings)
        {
            bindings[count] = binding;
            count++;
        }
    }

    for (auto& fragLayoutData : fragLayoutDatas)
    {
        for (auto& binding : fragLayoutData.m_Bindings)
        {
            bindings[count] = binding;
            count++;
        }
    }

    Vector<VkDescriptorSetLayoutBinding> cleanBindings = CleanDescriptorSetLayout(bindings);

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(cleanBindings.size());
    layoutInfo.pBindings = cleanBindings.data();

    VK_CHECK(vkCreateDescriptorSetLayout(m_LogicalDevice, &layoutInfo, nullptr, &descriptorSetLayout))
}

void lvk::VulkanAPI::CreateFramebuffer(Vector<VkImageView>& attachments, VkRenderPass renderPass, VkExtent2D extent, VkFramebuffer& framebuffer)
{
    VkFramebufferCreateInfo framebufferCreateInfo{};
    framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferCreateInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    framebufferCreateInfo.pAttachments = attachments.data();
    framebufferCreateInfo.layers = 1;
    framebufferCreateInfo.renderPass = renderPass;
    framebufferCreateInfo.height = extent.height;
    framebufferCreateInfo.width = extent.width;

    VK_CHECK (vkCreateFramebuffer(m_LogicalDevice, &framebufferCreateInfo, nullptr, &framebuffer) != VK_SUCCESS)
}

VkPipeline lvk::VulkanAPI::CreateRasterizationGraphicsPipeline(ShaderProgram& shader,
    Vector<VkVertexInputBindingDescription>& vertexBindingDescriptions, Vector<VkVertexInputAttributeDescription>& vertexAttributeDescriptions, 
    VkRenderPass& pipelineRenderPass, 
    uint32_t width, uint32_t height, 
    VkPolygonMode polyMode, 
    VkCullModeFlags cullMode, 
    bool enableMultisampling, 
    VkCompareOp depthCompareOp, 
    VkPipelineLayout& pipelineLayout,
    uint32_t colourAttachmentCount)
{
    VkShaderModule vertShaderModule = CreateShaderModule(shader.m_Stages[0].m_StageBinary);
    VkShaderModule fragShaderModule = CreateShaderModule(shader.m_Stages[1].m_StageBinary);

    spdlog::info("Loaded vertex and fragment shaders & created shader modules");

    // ..
    VkPipelineShaderStageCreateInfo vertexShaderStageInfo{};
    vertexShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertexShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertexShaderStageInfo.module = vertShaderModule;
    vertexShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName = "main";

    std::vector<VkPipelineShaderStageCreateInfo> shaderStageCreateInfos = { vertexShaderStageInfo, fragShaderStageInfo };

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(vertexBindingDescriptions.size());
    vertexInputInfo.pVertexBindingDescriptions = vertexBindingDescriptions.data();
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexAttributeDescriptions.size());
    vertexInputInfo.pVertexAttributeDescriptions = vertexAttributeDescriptions.data();

    VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo{};
    inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.x = 0.0f;
    viewport.width = static_cast<float>(width);
    viewport.height = static_cast<float>(height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor{};
    scissor.offset = { 0,0 };
    scissor.extent = VkExtent2D{ width, height };

    VkPipelineViewportStateCreateInfo viewportInfo{};
    viewportInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportInfo.viewportCount = 1;
    viewportInfo.pViewports = &viewport;
    viewportInfo.scissorCount = 1;
    viewportInfo.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizerInfo{};
    rasterizerInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizerInfo.depthClampEnable = VK_FALSE;

    rasterizerInfo.rasterizerDiscardEnable = VK_FALSE;
    // using anything other than FILL requires enableing a gpu feature.
    rasterizerInfo.polygonMode = polyMode;
    // thickness of lines in terms of pixels. > 1.0f requires wide lines gpu feature.
    rasterizerInfo.lineWidth = 1.0f;

    //rasterizerInfo.cullMode = VK_CULL_MODE_NONE;
    //// From vk-tutorial: The frontFace variable specifies the vertex order 
    //// for faces to be considered front-facing and can be clockwise or counterclockwise.
    //// ??
    rasterizerInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;

    rasterizerInfo.cullMode = cullMode;
    rasterizerInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

    rasterizerInfo.depthBiasEnable = VK_FALSE;
    rasterizerInfo.depthBiasConstantFactor = 0.0f;
    rasterizerInfo.depthBiasClamp = 0.0f;
    rasterizerInfo.depthBiasSlopeFactor = 0.0f;


    VkSampleCountFlagBits sampleCount = VK_SAMPLE_COUNT_1_BIT;

    if (enableMultisampling)
    {
        sampleCount = m_MaxMsaaSamples;
    }

    // ToDo : Do something with enableMultisampling here
    VkPipelineMultisampleStateCreateInfo multisampleInfo{};
    multisampleInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampleInfo.sampleShadingEnable = static_cast<VkBool32>(enableMultisampling);
    multisampleInfo.rasterizationSamples = sampleCount;
    multisampleInfo.minSampleShading = .2f;
    multisampleInfo.pSampleMask = nullptr;
    multisampleInfo.alphaToCoverageEnable = VK_FALSE;
    multisampleInfo.alphaToOneEnable = VK_FALSE;

    Vector<VkPipelineColorBlendAttachmentState> colorAttachmentBlendStates;

    for (uint32_t i = 0; i < colourAttachmentCount; i++)
    {
        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.colorWriteMask = 
            VK_COLOR_COMPONENT_R_BIT |
            VK_COLOR_COMPONENT_G_BIT |
            VK_COLOR_COMPONENT_B_BIT |
            VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable = VK_FALSE;
        colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
        colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
        colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
        colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

        colorAttachmentBlendStates.push_back(colorBlendAttachment);

    }

    VkPipelineColorBlendStateCreateInfo colorBlendStateInfo{};
    colorBlendStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlendStateInfo.logicOpEnable = VK_FALSE;
    colorBlendStateInfo.logicOp = VK_LOGIC_OP_COPY;
    colorBlendStateInfo.attachmentCount = static_cast<uint32_t>(colorAttachmentBlendStates.size());
    colorBlendStateInfo.pAttachments = colorAttachmentBlendStates.data();

    VkDynamicState dynamicStates[] =
    {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR,
    };

    VkPipelineDynamicStateCreateInfo dynamicStateInfo{};
    dynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicStateInfo.dynamicStateCount = sizeof(dynamicStates) / sizeof(VkDynamicState);
    dynamicStateInfo.pDynamicStates = dynamicStates;

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &shader.m_DescriptorSetLayout;

    // update
    // valid combos:
    // 1 stage has 1 push constant block
    // both stages share the same push constant block
    // each stage has a separate push constant block
    // e.g. only ever 1 block per stage

    Vector<VkPushConstantRange> pushConstantRanges{};

    for (auto& stage : shader.m_Stages)
    {
        if (stage.m_PushConstants.empty())
        {
            continue;
        }

        if (stage.m_PushConstants.size() > 1)
        {
            spdlog::error("VulkanAPI : CreateRasterizationPipeline : Supplied stage has more than 1 push constant block, this is not allowed.");
            continue;
        }

        PushConstantBlock& block = stage.m_PushConstants[0];

        bool skip = false;

        for (auto& range : pushConstantRanges)
        {
            if (block.m_Offset == range.offset && block.m_Size == range.size)
            {
                range.stageFlags |= block.m_Stage;
            }
        }

        if (skip)
        {
            continue;
        }

        VkPushConstantRange range{};
        range.offset = block.m_Offset;
        range.size = block.m_Size;
        range.stageFlags = block.m_Stage;

        pushConstantRanges.push_back(range);
    }

    pipelineLayoutInfo.pushConstantRangeCount = static_cast<uint32_t>(pushConstantRanges.size());
    pipelineLayoutInfo.pPushConstantRanges = pushConstantRanges.size() > 0 ? pushConstantRanges.data() : 0;

    VK_CHECK(vkCreatePipelineLayout(m_LogicalDevice, &pipelineLayoutInfo, nullptr, &pipelineLayout))

    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE;
    depthStencil.depthCompareOp = depthCompareOp;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.minDepthBounds = 0.0f; // Optional
    depthStencil.maxDepthBounds = 1.0f; // Optional
    depthStencil.stencilTestEnable = VK_FALSE;
    depthStencil.front = {}; // Optional
    depthStencil.back = {}; // Optional

    VkGraphicsPipelineCreateInfo pipelineCreateInfo{};
    pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineCreateInfo.stageCount = 2;
    pipelineCreateInfo.pStages = shaderStageCreateInfos.data();

    pipelineCreateInfo.pVertexInputState = &vertexInputInfo;
    pipelineCreateInfo.pInputAssemblyState = &inputAssemblyInfo;
    pipelineCreateInfo.pViewportState = &viewportInfo;
    pipelineCreateInfo.pRasterizationState = &rasterizerInfo;
    pipelineCreateInfo.pMultisampleState = &multisampleInfo;
    pipelineCreateInfo.pDepthStencilState = nullptr;
    pipelineCreateInfo.pColorBlendState = &colorBlendStateInfo;
    pipelineCreateInfo.pDynamicState = &dynamicStateInfo;
    pipelineCreateInfo.pDepthStencilState = &depthStencil;

    pipelineCreateInfo.layout = pipelineLayout;

    pipelineCreateInfo.renderPass = pipelineRenderPass;
    pipelineCreateInfo.subpass = 0;

    VkPipeline pipeline;
    VK_CHECK(vkCreateGraphicsPipelines(m_LogicalDevice, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &pipeline))

    vkDestroyShaderModule(m_LogicalDevice, vertShaderModule, nullptr);
    vkDestroyShaderModule(m_LogicalDevice, fragShaderModule, nullptr);

    spdlog::info("Destroyed vertex and fragment shader modules");

    return pipeline;
}

void lvk::VulkanAPI::CreateSwapChainFramebuffers()
{
    m_SwapChainFramebuffers.resize(m_SwapChainImageViews.size());

    for (uint32_t i = 0; i < m_SwapChainImageViews.size(); i++)
    {
        std::vector<VkImageView> attachments;

        if (m_EnableSwapchainMsaa)
        {
            attachments.resize(3);
            attachments[0] = m_SwapChainColourImageView;
            attachments[1] = m_SwapChainDepthImageView;
            attachments[2] = m_SwapChainImageViews[i];
        }
        else
        {
            attachments.resize(2);
            attachments[0] = m_SwapChainImageViews[i];
            attachments[1] = m_SwapChainDepthImageView;

        }

        CreateFramebuffer(attachments, m_SwapchainImageRenderPass, m_SwapChainImageExtent, m_SwapChainFramebuffers[i]);
    }
}

void lvk::VulkanAPI::CreateCommandPool()
{
    VkCommandPoolCreateInfo createInfo{};
    createInfo.sType                = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    createInfo.queueFamilyIndex     = m_QueueFamilyIndices.m_QueueFamilies[QueueFamilyType::GraphicsAndCompute];
    createInfo.flags                = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    if(vkCreateCommandPool(m_LogicalDevice, &createInfo, nullptr, &m_GraphicsQueueCommandPool) != VK_SUCCESS)
    {
        spdlog::error("Failed to create Command Pool!");
        std::cerr << "Failed to create Command Pool!" << std::endl;
    }
}

void lvk::VulkanAPI::CreateDescriptorSetAllocator()
{
    m_DescriptorSetAllocator.Init(*this, MAX_FRAMES_IN_FLIGHT * 128, {
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0.33f},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0.33f},
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 0.33f},
    });
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
    }

    if (m_UseImGui)
    {
        RenderImGui();
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
        spdlog::info("Physical Device : {} : {}", &deviceProperties.deviceName[0], &extensions[i].extensionName[0]);
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

StageBinary lvk::VulkanAPI::LoadSpirvBinary(const String& path)
{
    std::ifstream file(path, std::ios::ate | std::ios::binary);

    if (!file.is_open())
    {
        spdlog::error("Failed to open file at path {} as binary!", path);
        std::cerr << "Failed to open file!" << std::endl;
        return StageBinary();
    }

    size_t fileSize = static_cast<size_t>(file.tellg());
    StageBinary data(fileSize);

    file.seekg(0);

    file.read((char*) data.data(), fileSize);

    file.close();
    return data;
}

VkDescriptorSet lvk::VulkanAPI::CreateDescriptorSet(DescriptorSetLayoutData& layoutData)
{
    return m_DescriptorSetAllocator.Allocate(m_LogicalDevice, layoutData.m_Layout, nullptr);
}

void lvk::VulkanAPI::CreateBuiltInRenderPasses()
{
    {
        Vector<VkAttachmentDescription> colourAttachmentDescriptions{};
        Vector<VkAttachmentDescription> resolveAttachmentDescriptions{};
        VkAttachmentDescription depthAttachmentDescription{};

        VkAttachmentDescription colorAttachment{};
        colorAttachment.format = m_SwapChainImageFormat;
        colorAttachment.samples = m_EnableSwapchainMsaa ? m_MaxMsaaSamples : VK_SAMPLE_COUNT_1_BIT;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        if (m_EnableSwapchainMsaa)
        {
            colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        }
        else
        {
            colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        }
        colourAttachmentDescriptions.push_back(colorAttachment);

        depthAttachmentDescription.format = FindDepthFormat();
        depthAttachmentDescription.samples = m_EnableSwapchainMsaa ? m_MaxMsaaSamples : VK_SAMPLE_COUNT_1_BIT;;
        depthAttachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depthAttachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depthAttachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        depthAttachmentDescription.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkAttachmentDescription colorAttachmentResolve{};
        if (m_EnableSwapchainMsaa)
        {
            colorAttachmentResolve.format = m_SwapChainImageFormat;
            colorAttachmentResolve.samples = VK_SAMPLE_COUNT_1_BIT;
            colorAttachmentResolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            colorAttachmentResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            colorAttachmentResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            colorAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            colorAttachmentResolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            colorAttachmentResolve.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
            resolveAttachmentDescriptions.push_back(colorAttachmentResolve);
        }

        CreateRenderPass(m_SwapchainImageRenderPass, colourAttachmentDescriptions, resolveAttachmentDescriptions, true, depthAttachmentDescription, VK_ATTACHMENT_LOAD_OP_CLEAR);
    }
    {
        Vector<VkAttachmentDescription> colourAttachmentDescriptions{};
        Vector<VkAttachmentDescription> resolveAttachmentDescriptions{};
        VkAttachmentDescription depthAttachmentDescription{};

        VkAttachmentDescription colorAttachment{};
        colorAttachment.format = m_SwapChainImageFormat;
        colorAttachment.samples = m_EnableSwapchainMsaa ? m_MaxMsaaSamples : VK_SAMPLE_COUNT_1_BIT;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        if (m_EnableSwapchainMsaa)
        {
            colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        }
        else
        {
            colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        }
        colourAttachmentDescriptions.push_back(colorAttachment);

        depthAttachmentDescription.format = FindDepthFormat();
        depthAttachmentDescription.samples = m_EnableSwapchainMsaa ? m_MaxMsaaSamples : VK_SAMPLE_COUNT_1_BIT;;
        depthAttachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depthAttachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depthAttachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        depthAttachmentDescription.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkAttachmentDescription colorAttachmentResolve{};
        if (m_EnableSwapchainMsaa)
        {
            colorAttachmentResolve.format = m_SwapChainImageFormat;
            colorAttachmentResolve.samples = VK_SAMPLE_COUNT_1_BIT;
            colorAttachmentResolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            colorAttachmentResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            colorAttachmentResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            colorAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            colorAttachmentResolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            colorAttachmentResolve.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
            resolveAttachmentDescriptions.push_back(colorAttachmentResolve);
        }
        CreateRenderPass(m_ImGuiRenderPass, colourAttachmentDescriptions, resolveAttachmentDescriptions, true, depthAttachmentDescription, VK_ATTACHMENT_LOAD_OP_DONT_CARE);
    }
}

VkPipeline lvk::VulkanAPI::CreateComputePipeline(StageBinary& comp, VkDescriptorSetLayout& descriptorSetLayout, uint32_t width, uint32_t height, VkPipelineLayout& pipelineLayout)
{

    auto compStage = CreateShaderModule(comp);

    VkPipelineShaderStageCreateInfo compShaderStageInfo{};
    compShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    compShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    compShaderStageInfo.module = compStage;
    compShaderStageInfo.pName = "main";

    VkComputePipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipelineInfo.layout = pipelineLayout;
    pipelineInfo.stage = compShaderStageInfo;

    VkPipeline pipeline;

    if (vkCreateComputePipelines(m_LogicalDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline) != VK_SUCCESS) {
        spdlog::error("failed to create compute pipeline!");
        return pipeline;
    }

    return VK_NULL_HANDLE;
}

Vector<VkDescriptorSetLayoutBinding> lvk::VulkanAPI::GetDescriptorSetLayoutBindings(Vector<DescriptorSetLayoutData>& vertLayoutDatas, Vector<DescriptorSetLayoutData>& fragLayoutDatas)
{
    Vector<VkDescriptorSetLayoutBinding> bindings;
    uint8_t count = 0;

    for (auto& vertLayoutData : vertLayoutDatas)
    {
        count += static_cast<uint8_t>(vertLayoutData.m_Bindings.size());
    }

    for (auto& fragLayoutData : fragLayoutDatas)
    {
        count += static_cast<uint8_t>(fragLayoutData.m_Bindings.size());
    }

    bindings.resize(count);

    count = 0;
    // .. do the things
    for (auto& vertLayoutData : vertLayoutDatas)
    {
        for (auto& binding : vertLayoutData.m_Bindings)
        {
            bindings[count] = binding;
            count++;
        }
    }

    for (auto& fragLayoutData : fragLayoutDatas)
    {
        for (auto& binding : fragLayoutData.m_Bindings)
        {
            bindings[count] = binding;
            count++;
        }
    }

    return CleanDescriptorSetLayout(bindings);
}

void lvk::VulkanAPI::CreateRenderPass(VkRenderPass& renderPass, Vector<VkAttachmentDescription>& colourAttachments, Vector<VkAttachmentDescription>& resolveAttachments, bool hasDepthAttachment, VkAttachmentDescription depthAttachment, VkAttachmentLoadOp attachmentLoadOp)
{
    // Layout: Colour attachments -> Depth attachments -> Resolve attachments
    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

    Vector<VkAttachmentDescription> attachments;

    Vector<VkAttachmentReference>   colourAttachmentReferences;
    Vector<VkAttachmentReference>   resolveAttachmentReferences;

    uint32_t attachmentCount = 0;

    VkAttachmentReference colorAttachmentReference{};
    for (auto i = 0; i < colourAttachments.size(); i++)
    {
        attachments.push_back(colourAttachments[i]);

        colorAttachmentReference.attachment = attachmentCount++;
        colorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        colourAttachmentReferences.push_back(colorAttachmentReference);
    }

    VkAttachmentReference depthAttachmentReference{};
    if (hasDepthAttachment)
    {
        attachments.push_back(depthAttachment);
        depthAttachmentReference.attachment = attachmentCount++;
        depthAttachmentReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        subpass.pDepthStencilAttachment = &depthAttachmentReference;
    }

    VkAttachmentReference colorAttachmentResolveReference{};
    for (auto i = 0; i < resolveAttachments.size(); i++)
    {
        attachments.push_back(resolveAttachments[i]);
        colorAttachmentResolveReference.attachment = attachmentCount++;
        colorAttachmentResolveReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        resolveAttachmentReferences.push_back(colorAttachmentResolveReference);
    }

    subpass.colorAttachmentCount = static_cast<uint32_t>(colourAttachmentReferences.size());
    subpass.pColorAttachments = colourAttachmentReferences.data();
    subpass.pResolveAttachments = resolveAttachmentReferences.data();

    VkPipelineStageFlags waitFlags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkAccessFlags accessFlags = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    if (hasDepthAttachment)
    {
        waitFlags |= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        accessFlags |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    }

    VkSubpassDependency subpassDependency{};
    // implicit subpasses 
    subpassDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    // our pass
    subpassDependency.dstSubpass = 0;
    // wait for the colour output stage to finish
    subpassDependency.srcStageMask = waitFlags;
    subpassDependency.srcAccessMask = 0;
    // wait until we can write to the color attachment
    subpassDependency.dstStageMask = waitFlags;
    subpassDependency.dstAccessMask = accessFlags;


    VkRenderPassCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    createInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    createInfo.pAttachments = attachments.data();
    createInfo.subpassCount = 1;
    createInfo.pSubpasses = &subpass;
    createInfo.dependencyCount = 1;
    createInfo.pDependencies = &subpassDependency;

    if (vkCreateRenderPass(m_LogicalDevice, &createInfo, nullptr, &renderPass) != VK_SUCCESS)
    {
        spdlog::error("Failed to create Render Pass!");
        std::cerr << "Failed to create Render Pass!" << std::endl;
    }
}
#ifdef WIN32
#include "windows.h"
#include <dwmapi.h>
#endif
void lvk::VulkanAPI::Start(const String& appName, uint32_t width, uint32_t height, bool enableSwapchainMsaa)
{
    p_AppName = appName;
#ifdef WIN32
    SetProcessDPIAware();
#endif

    CreateWindowLVK(width, height);
    InitVulkan(enableSwapchainMsaa);

    p_MaxFramebufferExtent = GetMaxFramebufferResolution();

    InitImGui();

    Texture::InitDefaultTexture(*this);
    Mesh::InitScreenQuad(*this);
}

lvk::ShaderBindingType GetBindingType(const SpvReflectDescriptorBinding& binding)
{
    if (binding.descriptor_type == SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
    {
        return lvk::ShaderBindingType::UniformBuffer;
    }
    if (binding.descriptor_type == SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER)
    {
        return lvk::ShaderBindingType::ShaderStorageBuffer;
    }
    if (binding.descriptor_type == SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLER || 
        binding.descriptor_type == SPV_REFLECT_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
    {
        return lvk::ShaderBindingType::Sampler;
    }

    return ShaderBindingType::UniformBuffer;
}

Vector<DescriptorSetLayoutData> lvk::VulkanAPI::ReflectDescriptorSetLayouts(StageBinary& stageBin)
{
    SpvReflectShaderModule shaderReflectModule;
    SpvReflectResult result = spvReflectCreateShaderModule(stageBin.size(), stageBin.data(), &shaderReflectModule);

    uint32_t descriptorSetCount = 0;
    spvReflectEnumerateDescriptorSets(&shaderReflectModule, &descriptorSetCount, nullptr);

    if (descriptorSetCount == 0)
    {
        return {};
    }

    std::vector<SpvReflectDescriptorSet*> reflectedDescriptorSets;
    reflectedDescriptorSets.resize(descriptorSetCount);
    spvReflectEnumerateDescriptorSets(&shaderReflectModule, &descriptorSetCount, &reflectedDescriptorSets[0]);
    

    std::vector<DescriptorSetLayoutData> layoutDatas(descriptorSetCount, DescriptorSetLayoutData{});

    for (int i = 0; i < reflectedDescriptorSets.size(); i++)
    {
        const SpvReflectDescriptorSet& reflectedSet = *reflectedDescriptorSets[i];
        DescriptorSetLayoutData& layoutData = layoutDatas[i];

        layoutData.m_Bindings.resize(reflectedSet.binding_count);
        for (uint32_t bc = 0; bc < reflectedSet.binding_count; bc++)
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

            ShaderBindingType bufferType = GetBindingType(reflectedBinding);
            DescriptorSetLayoutBindingData binding{ String(reflectedBinding.name), reflectedBinding.binding, reflectedBinding.block.size , bufferType};

            if (bufferType == ShaderBindingType::ShaderStorageBuffer)
            {
                uint32_t requiredBufferSize = 0;
                for (uint32_t i = 0; i < reflectedBinding.type_description->member_count; i++)
                {
                    auto* member = &reflectedBinding.type_description->members[i];
                    if (member->type_flags & SPV_REFLECT_TYPE_FLAG_ARRAY)
                    {
                        requiredBufferSize += member->traits.array.dims[0] * member->traits.array.stride; // todo: support more than 1D arrays
                    }

                    for (uint32_t j = 0; j < member->member_count; j++)
                    {
                        auto* childMember = &member->members[i];
                        int jkj = 420;
                    }
                }
                binding.m_ExpectedBufferSize = requiredBufferSize;
                continue;
            }

            for (uint32_t i = 0; i < reflectedBinding.block.member_count; i++)
            {
                auto member = reflectedBinding.block.members[i];
                ShaderBufferMember reflectedMember{};
                reflectedMember.m_Name = String(member.name);
                reflectedMember.m_Offset = member.absolute_offset; // this might be an issue with padded types?
                reflectedMember.m_Size = member.padded_size;
                reflectedMember.m_Type = GetTypeFromSpvReflect(member.type_description);
                
                if (member.array.dims_count > 0)
                {
                    reflectedMember.m_Stride = member.array.stride;
                }
                else
                {
                    reflectedMember.m_Stride = 0;
                }

                binding.m_Members.push_back(reflectedMember);
            }
            if (reflectedBinding.resource_type & SPV_REFLECT_RESOURCE_FLAG_SAMPLER)
            {
                ShaderBufferMember reflectedMember{};
                reflectedMember.m_Name = String(reflectedBinding.name);
                reflectedMember.m_Type = ShaderBufferMemberType::_sampler;
                binding.m_Members.push_back(reflectedMember);
            }
            layoutData.m_BindingDatas.push_back(binding);
        }

        layoutData.m_SetNumber = reflectedSet.set;
        layoutData.m_CreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutData.m_CreateInfo.bindingCount = reflectedSet.binding_count;
        layoutData.m_CreateInfo.pBindings = layoutData.m_Bindings.data();
    }

    return layoutDatas;
}

Vector<PushConstantBlock> lvk::VulkanAPI::ReflectPushConstants(StageBinary& stageBin)
{
    Vector<PushConstantBlock> pushConstants{};
    SpvReflectShaderModule shaderReflectModule;
    SpvReflectResult result = spvReflectCreateShaderModule(stageBin.size(), stageBin.data(), &shaderReflectModule);

    uint32_t pushConstantBlockCount = 0;
    spvReflectEnumeratePushConstantBlocks(&shaderReflectModule, &pushConstantBlockCount, nullptr);
    std::vector<SpvReflectBlockVariable*> reflectedPushConstantBlocks;
    reflectedPushConstantBlocks.resize(pushConstantBlockCount);
    spvReflectEnumeratePushConstantBlocks(&shaderReflectModule, &pushConstantBlockCount, reflectedPushConstantBlocks.data());

    
    for (uint32_t i = 0; i < pushConstantBlockCount; i++)
    {
        const SpvReflectBlockVariable& pcBlock = *reflectedPushConstantBlocks[i];
        pushConstants.push_back({ pcBlock.size, pcBlock.offset, pcBlock.name, (VkShaderStageFlags) shaderReflectModule.shader_stage });
    }

    return pushConstants;
}

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

void lvk::VulkanAPI::GetMaxUsableSampleCount()
{
    VkPhysicalDeviceProperties physicalDeviceProperties;
    vkGetPhysicalDeviceProperties(m_PhysicalDevice, &physicalDeviceProperties);

    VK_SAMPLE_COUNT_2_BIT;
    VK_SAMPLE_COUNT_4_BIT;
    VK_SAMPLE_COUNT_8_BIT;
    VK_SAMPLE_COUNT_16_BIT;

    auto counts = physicalDeviceProperties.limits.framebufferColorSampleCounts & physicalDeviceProperties.limits.framebufferDepthSampleCounts;
    if (counts & VK_SAMPLE_COUNT_64_BIT) {  m_MaxMsaaSamples = VK_SAMPLE_COUNT_64_BIT;  return ;}
    if (counts & VK_SAMPLE_COUNT_32_BIT) {  m_MaxMsaaSamples = VK_SAMPLE_COUNT_32_BIT;  return ;}
    if (counts & VK_SAMPLE_COUNT_16_BIT) {  m_MaxMsaaSamples = VK_SAMPLE_COUNT_16_BIT;  return ;}
    if (counts & VK_SAMPLE_COUNT_8_BIT) {   m_MaxMsaaSamples = VK_SAMPLE_COUNT_8_BIT;   return ;}
    if (counts & VK_SAMPLE_COUNT_4_BIT) {   m_MaxMsaaSamples = VK_SAMPLE_COUNT_4_BIT;   return ;}
    if (counts & VK_SAMPLE_COUNT_2_BIT) {   m_MaxMsaaSamples = VK_SAMPLE_COUNT_2_BIT;   return ;}
}

void lvk::VulkanAPI::InitVulkan(bool enableSwapchainMsaa)
{
    p_CurrentFrameIndex = 0;
    m_EnableSwapchainMsaa = enableSwapchainMsaa;
    CreateInstance();
    SetupDebugOutput();
    CreateSurface();
    PickPhysicalDevice();
    CreateLogicalDevice();
    GetMaxUsableSampleCount();
    GetQueueHandles();
    CreateCommandPool();
    CreateSwapChain();
    CreateSwapChainImageViews();
    CreateSwapChainDepthTexture(m_EnableSwapchainMsaa);
    CreateSwapChainColourTexture(m_EnableSwapchainMsaa);
    CreateBuiltInRenderPasses();
    CreateSwapChainFramebuffers();
    CreateDescriptorSetAllocator();
    CreateSemaphores();
    CreateFences();
    CreateCommandBuffers();
    CreateVmaAllocator();
}

void lvk::VulkanAPI::InitImGui()
{
    if (!m_UseImGui)
    {
        return;
    }
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    auto style = ImGui::GetStyle();
    style.Colors[ImGuiCol_FrameBg].w = 1.0f;
    style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    style.Colors[ImGuiCol_ChildBg].w = 1.0f;
    InitImGuiBackend();

    ImGui_ImplVulkan_InitInfo init_info = {};

    init_info.Instance = m_Instance;
    init_info.PhysicalDevice = m_PhysicalDevice;
    init_info.Device = m_LogicalDevice;
    init_info.QueueFamily = m_QueueFamilyIndices.m_QueueFamilies[QueueFamilyType::GraphicsAndCompute];
    init_info.Queue = m_GraphicsQueue;
    init_info.PipelineCache = VK_NULL_HANDLE;
    // TODO: this is a bit shit, need to clean this pool some wheere
    init_info.DescriptorPool = m_DescriptorSetAllocator.CreatePool(m_LogicalDevice, 4096);
    init_info.Allocator = nullptr;
    init_info.MinImageCount = 2;
    init_info.ImageCount = 2;
    init_info.RenderPass = m_ImGuiRenderPass;
    if (m_EnableSwapchainMsaa)
    {
        init_info.MSAASamples = m_MaxMsaaSamples;
    }
    ImGui_ImplVulkan_Init(&init_info);

}

void lvk::VulkanAPI::RenderImGui()
{    
    ImGui::Render();
    VkCommandBuffer imguiCommandBuffer = BeginSingleTimeCommands();

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = m_ImGuiRenderPass;
    renderPassInfo.framebuffer = m_SwapChainFramebuffers[GetFrameIndex()];
    renderPassInfo.renderArea.offset = { 0,0 };
    renderPassInfo.renderArea.extent = m_SwapChainImageExtent;

    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = { {0.0f, 0.0f, 0.0f, 1.0f} };
    clearValues[1].depthStencil = { 1.0f, 0 };

    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();

    vkCmdBeginRenderPass(imguiCommandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), imguiCommandBuffer);
    vkCmdEndRenderPass(imguiCommandBuffer);
    EndSingleTimeCommands(imguiCommandBuffer);
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

VkCommandBuffer& VulkanAPI::BeginGraphicsCommands(uint32_t frameIndex) {
    VkCommandBufferBeginInfo commandBufferBeginInfo{};
    commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    commandBufferBeginInfo.flags = 0;
    commandBufferBeginInfo.pInheritanceInfo = nullptr;

    VK_CHECK(vkBeginCommandBuffer(m_CommandBuffers[frameIndex], &commandBufferBeginInfo));
    return m_CommandBuffers[frameIndex];
}

void VulkanAPI::EndGraphicsCommands(uint32_t frameIndex) {
    VK_CHECK(vkEndCommandBuffer(m_CommandBuffers[frameIndex]));
}



void lvk::VulkanAPI::RecordGraphicsCommands(std::function<void(VkCommandBuffer&, uint32_t)> graphicsCommandsCallback)
{
    for (uint32_t i = 0; i < m_CommandBuffers.size(); i++)
    {
        VkCommandBufferBeginInfo commandBufferBeginInfo{};
        commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        commandBufferBeginInfo.flags = 0;
        commandBufferBeginInfo.pInheritanceInfo = nullptr;

        VK_CHECK(vkBeginCommandBuffer(m_CommandBuffers[i], &commandBufferBeginInfo))

        // Callback 
        graphicsCommandsCallback(m_CommandBuffers[i], i);


        VK_CHECK(vkEndCommandBuffer(m_CommandBuffers[i]))
    } 
}

void lvk::VulkanAPI::TransitionImageLayout(VkImage image, VkFormat format, uint32_t numMips, VkImageLayout oldLayout, VkImageLayout newLayout)
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
    barrier.subresourceRange.levelCount = numMips;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;
    if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

        if (HasStencilComponent(format)) {
            barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
        }
    }
    else {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    }

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
    else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
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

void lvk::VulkanAPI::GenerateMips(VkImage image, VkFormat format, uint32_t imageWidth, uint32_t imageHeight, uint32_t numMips, VkFilter filterMethod)
{
    VkFormatProperties formatProperties;
    vkGetPhysicalDeviceFormatProperties(m_PhysicalDevice, format, &formatProperties);

    auto supportsLinearSampling = formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT;
    
    if (supportsLinearSampling <= 0)
    {
        spdlog::error("GenerateMips : No support for linear blitting!");
        return;
    }

    VkCommandBuffer cmd = BeginSingleTimeCommands();

    VkImageMemoryBarrier barrier{ };
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.image = image;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.subresourceRange.levelCount = 1;

    int32_t mipWidth = imageWidth;
    int32_t mipHeight = imageHeight;

    for (uint32_t i = 1; i < numMips; i++) {
        barrier.subresourceRange.baseMipLevel = i - 1;
        barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
            0, nullptr,
            0, nullptr,
            1, &barrier);

        VkImageBlit imageBlit{};
        imageBlit.srcOffsets[0] = { 0, 0, 0 };
        imageBlit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
        imageBlit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imageBlit.srcSubresource.mipLevel = i - 1;
        imageBlit.srcSubresource.baseArrayLayer = 0;
        imageBlit.srcSubresource.layerCount = 1;
        imageBlit.dstOffsets[0] = { 0, 0, 0 };
        imageBlit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
        imageBlit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imageBlit.dstSubresource.mipLevel = i;
        imageBlit.dstSubresource.baseArrayLayer = 0;
        imageBlit.dstSubresource.layerCount = 1;

        vkCmdBlitImage(cmd,
            image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1, &imageBlit, filterMethod);

        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(cmd,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
            0, nullptr,
            0, nullptr,
            1, &barrier);

        if (mipWidth > 1) mipWidth /= 2;
        if (mipHeight > 1) mipHeight /= 2;
    }

    EndSingleTimeCommands(cmd);
}

void lvk::VulkanAPI::CreateIndexBuffer(std::vector<uint32_t> indices, VkBuffer& buffer, VmaAllocation& deviceMemory)
{
    VkDeviceSize bufferSize = sizeof(uint32_t) * indices.size();

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

ShaderBufferMemberType lvk::GetTypeFromSpvReflect(SpvReflectTypeDescription* typeDescription)
{

    if (typeDescription->type_flags & SPV_REFLECT_TYPE_FLAG_MATRIX)
    {
        if (typeDescription->traits.numeric.matrix.column_count == 4 && typeDescription->traits.numeric.matrix.row_count == 4)
        {
            return ShaderBufferMemberType::_mat4;
        }

        if (typeDescription->traits.numeric.matrix.column_count == 3 && typeDescription->traits.numeric.matrix.row_count == 3)
        {
            return ShaderBufferMemberType::_mat3;
        }

        return ShaderBufferMemberType::UNKNOWN;
    }

    if (typeDescription->type_flags & SPV_REFLECT_TYPE_FLAG_VECTOR)
    {
        if (typeDescription->traits.numeric.vector.component_count == 2)
        {
            return ShaderBufferMemberType::_vec2;
        }

        if (typeDescription->traits.numeric.vector.component_count == 3)
        {
            return ShaderBufferMemberType::_vec3;
        }

        if (typeDescription->traits.numeric.vector.component_count == 4)
        {
            return ShaderBufferMemberType::_vec4;
        }
        return ShaderBufferMemberType::UNKNOWN;
    }

    if (typeDescription->type_flags & SPV_REFLECT_TYPE_FLAG_ARRAY)
    {
        return ShaderBufferMemberType::_array;
    }

    if (typeDescription->type_flags & SPV_REFLECT_TYPE_FLAG_FLOAT)
    {
        return ShaderBufferMemberType::_float;
    }

    if (typeDescription->type_flags & SPV_REFLECT_TYPE_FLAG_INT)
    {
        // signedness: unsigned = 0, signed = 1(?) 
        // might need to come back to this if we need 64 bit ints.
        if (typeDescription->traits.numeric.scalar.signedness == 0)
        {
            return ShaderBufferMemberType::_uint;
        }
        else
        {
            return ShaderBufferMemberType::_int;
        }
    }
    
    return ShaderBufferMemberType::UNKNOWN;
}


void lvk::VulkanAPI::CreateMappedBuffer(MappedBuffer& buf, VkBufferUsageFlags bufferUsage, VkMemoryPropertyFlags memoryProperties, uint32_t size)
{
    CreateBufferVMA(VkDeviceSize{ size }, bufferUsage, memoryProperties, buf.m_GpuBuffer, buf.m_GpuMemory);
    VK_CHECK(vmaMapMemory(m_Allocator, buf.m_GpuMemory, &buf.m_MappedAddr));
}

void lvk::VulkanAPI::CreateUniformBuffers(ShaderBufferFrameData& uniformData, VkDeviceSize bufferSize)
{
    uniformData.m_UniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        CreateMappedBuffer(uniformData.m_UniformBuffers[i], VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                           static_cast<uint32_t>(bufferSize));
    }

}

void lvk::VulkanAPI::CleanupImGui()
{
    if (m_UseImGui)
    {
        // TODO: destroy with other built in render passes
        // vkDestroyRenderPass(m_LogicalDevice, m_ImGuiRenderPass, nullptr);
        ImGui_ImplVulkan_Shutdown();
        CleanupImGuiBackend();
    }
}

