#include "VulkanAPI.h"

#include <cstdint>
#include <algorithm>
#include <fstream>
#include "spdlog/spdlog.h"

// comments are largely snippets from: https://vulkan-tutorial.com/. credit: Alexander Overvoorde

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData) {

    //std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;
    if (messageSeverity && VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
    {
        spdlog::warn("VL: {0}", pCallbackData->pMessage);
    }
    if (messageSeverity && VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
    {
        spdlog::info("VL: {0}", pCallbackData->pMessage);
    }
    if (messageSeverity && VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
    {
        spdlog::error("VL: {0}", pCallbackData->pMessage);
    }
    return VK_FALSE;
}


bool VulkanAPI::QueueFamilyIndices::IsComplete()
{
    bool foundGraphicsQueue = m_QueueFamilies.find(QueueFamilyType::Graphics) != m_QueueFamilies.end();
    bool foundPresentQueue  = m_QueueFamilies.find(QueueFamilyType::Present) != m_QueueFamilies.end();
    return foundGraphicsQueue && foundPresentQueue;
}

VkApplicationInfo VulkanAPI::CreateAppInfo()
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

bool VulkanAPI::CheckValidationLayerSupport()
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

bool VulkanAPI::CheckDeviceExtensionSupport(VkPhysicalDevice device)
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

void VulkanAPI::PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
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

void VulkanAPI::CreateInstance()
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

void VulkanAPI::Cleanup()
{
    CleanupWindow();
    CleanupVulkan();
}

void VulkanAPI::SetupDebugOutput()
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

void VulkanAPI::CleanupDebugOutput()
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

void VulkanAPI::CleanupVulkan()
{
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

    vkDestroyCommandPool(m_LogicalDevice, m_CommandPool, nullptr);

    for (int i = 0; i < m_SwapChainFramebuffers.size(); i++)
    {
        vkDestroyFramebuffer(m_LogicalDevice, m_SwapChainFramebuffers[i], nullptr);
    }
    for (int i = 0; i < m_SwapChainImageViews.size(); i++)
    {
        vkDestroyImageView(m_LogicalDevice, m_SwapChainImageViews[i], nullptr);
    }
    vkDestroyPipeline(m_LogicalDevice, m_Pipeline, nullptr);
    vkDestroyPipelineLayout(m_LogicalDevice, m_PipelineLayout, nullptr);
    vkDestroyRenderPass(m_LogicalDevice, m_RenderPass, nullptr);
    vkDestroySwapchainKHR(m_LogicalDevice, m_SwapChain, nullptr);
    vkDestroySurfaceKHR(m_Instance, m_Surface, nullptr);
    vkDestroyDevice(m_LogicalDevice, nullptr);
    vkDestroyInstance(m_Instance, nullptr);
}

VulkanAPI::QueueFamilyIndices VulkanAPI::FindQueueFamilies(VkPhysicalDevice m_PhysicalDevice)
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

VulkanAPI::SwapChainSupportDetais VulkanAPI::GetSwapChainSupportDetails(VkPhysicalDevice physicalDevice)
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

bool VulkanAPI::IsDeviceSuitable(VkPhysicalDevice physicalDevice)
{
    QueueFamilyIndices indices = FindQueueFamilies(physicalDevice);
    bool extensionsSupported = CheckDeviceExtensionSupport(physicalDevice);
    bool swapChainSupport = false;
    if (extensionsSupported)
    {
        SwapChainSupportDetais swapChainDetails = GetSwapChainSupportDetails(physicalDevice);
        swapChainSupport = swapChainDetails.m_SupportedFormats.size() > 0 && swapChainDetails.m_SupportedPresentModes.size() > 0;
    }

    return indices.IsComplete() && extensionsSupported && swapChainSupport;
}

uint32_t VulkanAPI::AssessDeviceSuitability(VkPhysicalDevice m_PhysicalDevice)
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

void VulkanAPI::PickPhysicalDevice()
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

void VulkanAPI::CreateLogicalDevice()
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

void VulkanAPI::GetQueueHandles()
{
    vkGetDeviceQueue(m_LogicalDevice, m_QueueFamilyIndices.m_QueueFamilies[QueueFamilyType::Graphics],      0, &m_GraphicsQueue);
    vkGetDeviceQueue(m_LogicalDevice, m_QueueFamilyIndices.m_QueueFamilies[QueueFamilyType::Present],       0, &m_PresentQueue);
}

VkSurfaceFormatKHR VulkanAPI::ChooseSwapChainSurfaceFormat(std::vector<VkSurfaceFormatKHR> availableFormats)
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

VkPresentModeKHR VulkanAPI::ChooseSwapChainPresentMode(std::vector<VkPresentModeKHR> availableModes)
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

VkExtent2D VulkanAPI::ChooseSwapExtent(VkSurfaceCapabilitiesKHR& surfaceCapabilities)
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

void VulkanAPI::CreateSwapChain()
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

void VulkanAPI::CreateSwapChainImageViews()
{
    m_SwapChainImageViews.resize(m_SwapChainImages.size());

    for (uint32_t i = 0; i < m_SwapChainImages.size(); i++)
    {
        VkImageViewCreateInfo createInfo{};
        createInfo.sType            = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image            = m_SwapChainImages[i];

        createInfo.viewType         = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format           = m_SwapChainImageFormat;

        createInfo.components.r     = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g     = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b     = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a     = VK_COMPONENT_SWIZZLE_IDENTITY;

        // wtf
        createInfo.subresourceRange.aspectMask      = VK_IMAGE_ASPECT_COLOR_BIT; // ?
        createInfo.subresourceRange.baseMipLevel    = 0; // just use the texture, no mip mapping
        createInfo.subresourceRange.levelCount      = 1; // only 1 layer as it is not stereoscopic
        createInfo.subresourceRange.baseArrayLayer  = 0; // index of the layer ( 0 because there is only 1?)
        createInfo.subresourceRange.layerCount      = 1; // only 1 layer as 2D

        if (vkCreateImageView(m_LogicalDevice, &createInfo, nullptr, &m_SwapChainImageViews[i]) != VK_SUCCESS)
        {
            spdlog::error("Failed to create image view!");
            std::cerr << "Failed to create image view" << std::endl;
        }
    }
}

void VulkanAPI::CreateGraphicsPipeline()
{
    auto vertBin = LoadSpirvBinary("shaders/tri.vert.spv");
    auto fragBin = LoadSpirvBinary("shaders/tri.frag.spv");

    VkShaderModule vertShaderModule = CreateShaderModule(vertBin);
    VkShaderModule fragShaderModule = CreateShaderModule(fragBin);

    spdlog::info("Loaded vertex and fragment shaders & created shader modules");

    // ..
    VkPipelineShaderStageCreateInfo vertexShaderStageInfo{};
    vertexShaderStageInfo.sType         = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertexShaderStageInfo.stage         = VK_SHADER_STAGE_VERTEX_BIT;
    vertexShaderStageInfo.module        = vertShaderModule;
    vertexShaderStageInfo.pName         = "main";

    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType           = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage           = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module          = fragShaderModule;
    fragShaderStageInfo.pName           = "main";

    std::vector<VkPipelineShaderStageCreateInfo> shaderStageCreateInfos = { vertexShaderStageInfo, fragShaderStageInfo };

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount   = 0;
    vertexInputInfo.pVertexBindingDescriptions      = nullptr;
    vertexInputInfo.vertexAttributeDescriptionCount = 0;
    vertexInputInfo.pVertexAttributeDescriptions    = nullptr;

    VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo{};
    inputAssemblyInfo.sType                     = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssemblyInfo.topology                  = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssemblyInfo.primitiveRestartEnable    = VK_FALSE;

    VkViewport viewport{};
    viewport.x          = 0.0f;
    viewport.x          = 0.0f;
    viewport.width      = static_cast<float>(m_SwapChainImageExtent.width);
    viewport.height     = static_cast<float>(m_SwapChainImageExtent.height);
    viewport.minDepth   = 0.0f;
    viewport.maxDepth   = 1.0f;

    VkRect2D scissor{};
    scissor.offset = { 0,0 };
    scissor.extent = m_SwapChainImageExtent;

    VkPipelineViewportStateCreateInfo viewportInfo{};
    viewportInfo.sType          = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportInfo.viewportCount  = 1;
    viewportInfo.pViewports     = &viewport;
    viewportInfo.scissorCount   = 1;
    viewportInfo.pScissors      = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizerInfo{};
    rasterizerInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizerInfo.depthClampEnable         = VK_FALSE;

    rasterizerInfo.rasterizerDiscardEnable  = VK_FALSE;
    // using anything other than FILL requires enableing a gpu feature.
    rasterizerInfo.polygonMode              = VK_POLYGON_MODE_FILL;
    // thickness of lines in terms of pixels. > 1.0f requires wide lines gpu feature.
    rasterizerInfo.lineWidth                = 1.0f;

    rasterizerInfo.cullMode                 = VK_CULL_MODE_NONE;
    // From vk-tutorial: The frontFace variable specifies the vertex order 
    // for faces to be considered front-facing and can be clockwise or counterclockwise.
    // ??
    rasterizerInfo.frontFace                = VK_FRONT_FACE_CLOCKWISE;
    rasterizerInfo.depthBiasEnable          = VK_FALSE;
    rasterizerInfo.depthBiasConstantFactor  = 0.0f;
    rasterizerInfo.depthBiasClamp           = 0.0f;
    rasterizerInfo.depthBiasSlopeFactor     = 0.0f;
    
    VkPipelineMultisampleStateCreateInfo multisampleInfo{};
    multisampleInfo.sType                   = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampleInfo.sampleShadingEnable     = VK_FALSE;
    multisampleInfo.rasterizationSamples    = VK_SAMPLE_COUNT_1_BIT;
    multisampleInfo.minSampleShading        = 1.0f;
    multisampleInfo.pSampleMask             = nullptr;
    multisampleInfo.alphaToCoverageEnable   = VK_FALSE;
    multisampleInfo.alphaToOneEnable        = VK_FALSE;

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask         =   VK_COLOR_COMPONENT_R_BIT |
                                                    VK_COLOR_COMPONENT_G_BIT |
                                                    VK_COLOR_COMPONENT_B_BIT |
                                                    VK_COLOR_COMPONENT_A_BIT;

    colorBlendAttachment.blendEnable            = VK_FALSE;
    colorBlendAttachment.srcColorBlendFactor    = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstColorBlendFactor    = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.colorBlendOp           = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor    = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor    = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp           = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo colorBlendStateInfo{};
    colorBlendStateInfo.sType               = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlendStateInfo.logicOpEnable       = VK_FALSE;
    colorBlendStateInfo.logicOp             = VK_LOGIC_OP_COPY;
    colorBlendStateInfo.attachmentCount     = 1;
    colorBlendStateInfo.pAttachments        = &colorBlendAttachment;

    VkDynamicState dynamicStates[] =
    {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_LINE_WIDTH
    };

    VkPipelineDynamicStateCreateInfo dynamicStateInfo{};
    dynamicStateInfo.sType              = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicStateInfo.dynamicStateCount  = 2;
    dynamicStateInfo.pDynamicStates     = dynamicStates;

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType                    = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount           = 0;
    pipelineLayoutInfo.pSetLayouts              = nullptr;
    pipelineLayoutInfo.pushConstantRangeCount   = 0;
    pipelineLayoutInfo.pPushConstantRanges      = 0;

    if (vkCreatePipelineLayout(m_LogicalDevice, &pipelineLayoutInfo, nullptr, &m_PipelineLayout) != VK_SUCCESS)
    {
        spdlog::error("Failed to create pipeline layout object!");
        std::cerr << "Failed to create pipeline layout object!" << std::endl;
    }

    VkGraphicsPipelineCreateInfo pipelineCreateInfo{};
    pipelineCreateInfo.sType                = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineCreateInfo.stageCount           = 2;
    pipelineCreateInfo.pStages              = shaderStageCreateInfos.data();

    pipelineCreateInfo.pVertexInputState    = &vertexInputInfo;
    pipelineCreateInfo.pInputAssemblyState  = &inputAssemblyInfo;
    pipelineCreateInfo.pViewportState       = &viewportInfo;
    pipelineCreateInfo.pRasterizationState  = &rasterizerInfo;
    pipelineCreateInfo.pMultisampleState    = &multisampleInfo;
    pipelineCreateInfo.pDepthStencilState   = nullptr;
    pipelineCreateInfo.pColorBlendState     = &colorBlendStateInfo;    
    pipelineCreateInfo.pDynamicState        = nullptr;

    pipelineCreateInfo.layout               = m_PipelineLayout;
    
    pipelineCreateInfo.renderPass           = m_RenderPass;
    pipelineCreateInfo.subpass              = 0;

    if (vkCreateGraphicsPipelines(m_LogicalDevice, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &m_Pipeline) != VK_SUCCESS)
    {
        spdlog::error("Failed to create graphics pipeline!");
        std::cerr << "Failed to create graphics pipeline!" << std::endl;
    }

    vkDestroyShaderModule(m_LogicalDevice, vertShaderModule, nullptr);
    vkDestroyShaderModule(m_LogicalDevice, fragShaderModule, nullptr);
    spdlog::info("Destroyed vertex and fragment shader modules");
}

VkShaderModule VulkanAPI::CreateShaderModule(const std::vector<char>& data)
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

void VulkanAPI::CreateRenderPass()
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

    if (vkCreateRenderPass(m_LogicalDevice, &createInfo, nullptr, &m_RenderPass) != VK_SUCCESS)
    {
        spdlog::error("Failed to create Render Pass!");
        std::cerr << "Failed to create Render Pass!" << std::endl;
    }
}

void VulkanAPI::CreateFramebuffers()
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
        framebufferCreateInfo.renderPass        = m_RenderPass;
        framebufferCreateInfo.height            = m_SwapChainImageExtent.height;
        framebufferCreateInfo.width             = m_SwapChainImageExtent.width;

        if (vkCreateFramebuffer(m_LogicalDevice, &framebufferCreateInfo, nullptr, &m_SwapChainFramebuffers[i]) != VK_SUCCESS)
        {
            spdlog::error("Failed to create Framebuffer!");
            std::cerr << "Failed to create Framebuffer!" << std::endl;
        }
    }
}

void VulkanAPI::CreateCommandPool()
{
    VkCommandPoolCreateInfo createInfo{};
    createInfo.sType                = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    createInfo.queueFamilyIndex     = m_QueueFamilyIndices.m_QueueFamilies[QueueFamilyType::Graphics];
    createInfo.flags                = 0;

    if(vkCreateCommandPool(m_LogicalDevice, &createInfo, nullptr, &m_CommandPool) != VK_SUCCESS)
    {
        spdlog::error("Failed to create Command Pool!");
        std::cerr << "Failed to create Command Pool!" << std::endl;
    }
}

void VulkanAPI::CreateCommandBuffers()
{
    m_CommandBuffers.resize(m_SwapChainFramebuffers.size());

    VkCommandBufferAllocateInfo allocateInfo{};
    allocateInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocateInfo.commandPool        = m_CommandPool;
    allocateInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocateInfo.commandBufferCount = static_cast<uint32_t>(m_CommandBuffers.size());

    if (vkAllocateCommandBuffers(m_LogicalDevice, &allocateInfo, m_CommandBuffers.data()) != VK_SUCCESS)
    {
        spdlog::error("Failed to create Command Buffers!");
        std::cerr << "Failed to create Command Buffers!" << std::endl;
    }

    for (uint32_t i = 0; i < m_CommandBuffers.size(); i++)
    {
        VkCommandBufferBeginInfo commandBufferBeginInfo{};
        commandBufferBeginInfo.sType                = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        commandBufferBeginInfo.flags                = 0;
        commandBufferBeginInfo.pInheritanceInfo     = nullptr;

        if (vkBeginCommandBuffer(m_CommandBuffers[i], &commandBufferBeginInfo) != VK_SUCCESS)
        {
            spdlog::error("Failed to begin recording to Command Buffer!");
            std::cerr << "Failed to begin recording to Command Buffer!" << std::endl;
        }

        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType                = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass           = m_RenderPass;
        renderPassInfo.framebuffer          = m_SwapChainFramebuffers[i];
        renderPassInfo.renderArea.offset    = { 0,0 };
        renderPassInfo.renderArea.extent    = m_SwapChainImageExtent;
        
        VkClearValue clearValue             = { {{0.0f, 0.0f, 0.0f, 1.0f}} };
        renderPassInfo.clearValueCount      = 1;
        renderPassInfo.pClearValues         = &clearValue;

        vkCmdBeginRenderPass(m_CommandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        vkCmdBindPipeline(m_CommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline);

        vkCmdDraw(m_CommandBuffers[i], 3, 1, 0, 0);

        vkCmdEndRenderPass(m_CommandBuffers[i]);

        if (vkEndCommandBuffer(m_CommandBuffers[i]) != VK_SUCCESS)
        {
            spdlog::error("Failed to finalize recording Command Buffer!");
            std::cerr << "Failed to finalize recording Command Buffer!" << std::endl;
        }
    }
}

void VulkanAPI::CreateSemaphores()
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

void VulkanAPI::CreateFences()
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

void VulkanAPI::DrawFrame()
{
    vkWaitForFences(m_LogicalDevice, 1, &m_FrameInFlightFences[p_CurrentFrameIndex], VK_TRUE, UINT64_MAX);
    vkResetFences(m_LogicalDevice, 1, &m_FrameInFlightFences[p_CurrentFrameIndex]);

    uint32_t imageIndex;
    vkAcquireNextImageKHR(m_LogicalDevice, m_SwapChain, UINT64_MAX, m_ImageAvailableSemaphores[p_CurrentFrameIndex], VK_NULL_HANDLE, &imageIndex);

    if (m_ImagesInFlight[imageIndex] != VK_NULL_HANDLE)
    {
        vkWaitForFences(m_LogicalDevice, 1, &m_ImagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
    }
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

    vkQueuePresentKHR(m_GraphicsQueue, &presentInfo);

    p_CurrentFrameIndex = (p_CurrentFrameIndex + 1) % MAX_FRAMES_IN_FLIGHT;
}

void VulkanAPI::ListDeviceExtensions(VkPhysicalDevice physicalDevice)
{
    std::vector<VkExtensionProperties> extensions = GetDeviceAvailableExtensions(physicalDevice);

    VkPhysicalDeviceProperties deviceProperties{};
    vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);
    for (int i = 0; i < extensions.size(); i++)
    {
        spdlog::info("Physical Device : {0} : {1}", deviceProperties.deviceName, extensions[i].extensionName);
    }
}

std::vector<VkExtensionProperties> VulkanAPI::GetDeviceAvailableExtensions(VkPhysicalDevice physicalDevice)
{
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, availableExtensions.data());

    return availableExtensions;


}

std::vector<char> VulkanAPI::LoadSpirvBinary(const std::string& path)
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

void VulkanAPI::InitVulkan()
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
    CreateGraphicsPipeline();
    CreateFramebuffers();
    CreateCommandPool();
    CreateCommandBuffers();
    CreateSemaphores();
    CreateFences();
}
