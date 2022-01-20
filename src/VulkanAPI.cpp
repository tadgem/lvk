#include "VulkanAPI.h"
#include "spdlog/spdlog.h"


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


bool VulkanAPI::QueueFamilyIndices::IsComplete()
{
    bool foundGraphicsQueue = m_QueueFamilies.find(QueueFamilyType::Graphics) != m_QueueFamilies.end();
    bool foundPresentQueue  = m_QueueFamilies.find(QueueFamilyType::Presentation) != m_QueueFamilies.end();
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
    appInfo.apiVersion = VK_API_VERSION_1_1;

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
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

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
    createInfo.enabledExtensionCount = extensionNames.size();
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

void VulkanAPI::SetupDebugOutput()
{
    if (!p_UseValidation) return;

    VkDebugUtilsMessengerCreateInfoEXT createInfo{};
    PopulateDebugMessengerCreateInfo(createInfo);

    PFN_vkVoidFunction rawFunction = vkGetInstanceProcAddr(m_Instance, "vkCreateDebugUtilsMessengerEXT");
    auto function = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(rawFunction);

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

        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(m_PhysicalDevice, i, m_Surface, &presentSupport);

        if (presentSupport == true) {
            indices.m_QueueFamilies[QueueFamilyType::Presentation] = i;
        }
    }
    return indices;
}

bool VulkanAPI::IsDeviceSuitable(VkPhysicalDevice m_PhysicalDevice)
{
    QueueFamilyIndices indices = FindQueueFamilies(m_PhysicalDevice);
    bool extensionsSupported = CheckDeviceExtensionSupport(m_PhysicalDevice);
    return indices.IsComplete() && extensionsSupported;
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
        m_PhysicalDevice = deviceScores.begin()->second;
    }

    if (m_PhysicalDevice == VK_NULL_HANDLE)
    {
        spdlog::error("Failed to find a suitable physical device.");
        std::cerr << "Failed to find a suitable physical device.\n";
    }

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
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.queueCreateInfoCount = queueCreateInfos.size();
    createInfo.pEnabledFeatures = &physicalDeviceFeatures;

    createInfo.enabledExtensionCount = static_cast<uint32_t>(p_DeviceExtensions.size());
    createInfo.ppEnabledExtensionNames = p_DeviceExtensions.data();

    if (p_UseValidation)
    {
        createInfo.enabledLayerCount = static_cast<uint32_t>(p_ValidationLayers.size());
        createInfo.ppEnabledLayerNames = p_ValidationLayers.data();
    }
    else
    {
        createInfo.enabledLayerCount = 0;
    }

    if (vkCreateDevice(m_PhysicalDevice, &createInfo, nullptr, &m_LogicalDevice))
    {
        spdlog::error("Failed to create logical device.");
        std::cerr << "Failed to create logical device. \n";
    }
}

void VulkanAPI::GetQueueHandles()
{
    vkGetDeviceQueue(m_LogicalDevice, m_QueueFamilyIndices.m_QueueFamilies[QueueFamilyType::Graphics],       0, &m_GraphicsQueue);
    vkGetDeviceQueue(m_LogicalDevice, m_QueueFamilyIndices.m_QueueFamilies[QueueFamilyType::Presentation],   0, &m_PresentQueue);
}

void VulkanAPI::InitVulkan()
{
    CreateInstance();
    SetupDebugOutput();
    CreateSurface();
    PickPhysicalDevice();
    CreateLogicalDevice();
    GetQueueHandles();
}
