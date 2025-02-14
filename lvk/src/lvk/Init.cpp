#define VK_NO_PROTOTYPES
#include "lvk/Init.h"
#include "ImGui/imgui_impl_vulkan.h"
#include "lvk/Commands.h"
#include "lvk/Macros.h"
#include "lvk/RenderPass.h"
#include "lvk/Texture.h"
#include "lvk/Utils.h"
#include "spdlog/spdlog.h"

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
  }
  return VK_FALSE;
}

void lvk::init::Cleanup(VkState& vk)
{
  //Texture::FreeDefaultTexture(*this);
  // Mesh::FreeBuiltInMeshes(*this);
  CleanupImGui(vk);
  vk.m_Backend->CleanupWindow(vk);
  CleanupVulkan(vk);
}

void lvk::init::CleanupImGui(VkState& vk)
{
  if (vk.m_UseImGui)
  {
    // TODO: destroy with other built in render passes
    // vkDestroyRenderPass(m_LogicalDevice, m_ImGuiRenderPass, nullptr);
    ImGui_ImplVulkan_Shutdown();
    vk.m_Backend->CleanupImGuiBackend(vk);
  }
}

bool lvk::init::CheckValidationLayerSupport(VkState& vk)
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

  for (const char* layerName : s_ValidationLayers) {
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

bool lvk::init::CheckDeviceExtensionSupport(VkState& vk, VkPhysicalDevice device)
{
  std::vector<VkExtensionProperties> availableExtensions = GetDeviceAvailableExtensions(vk, device);

  uint32_t requiredExtensionsFound = 0;

  for (auto const& extension : availableExtensions)
  {
    for (auto const& requiredExtensionName : s_DeviceExtensions)
    {
      if (strcmp(requiredExtensionName, extension.extensionName) == 0)
      {
        requiredExtensionsFound++;
      }
    }
  }

  return requiredExtensionsFound >= s_DeviceExtensions.size();

}

void lvk::init::SetupDebugOutput(VkState& vk)
{
  if (!vk.m_UseValidation) return;

  VkDebugUtilsMessengerCreateInfoEXT createInfo{};
  PopulateDebugMessengerCreateInfo(vk, createInfo);

  PFN_vkVoidFunction rawFunction = vkGetInstanceProcAddr(vk.m_Instance, "vkCreateDebugUtilsMessengerEXT");
  PFN_vkCreateDebugUtilsMessengerEXT function = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(rawFunction);

  if (!function)
  {
    spdlog::error("Could not find vkCreateDebugUtilsMessengerEXT function");
    std::cerr << "Could not find vkCreateDebugUtilsMessengerEXT function";
    return;
  }

  if (function(vk.m_Instance, &createInfo, nullptr, &vk.m_DebugMessenger))
  {
    spdlog::error("Failed to create debug messenger");
    std::cerr << "Failed to create debug messenger";
  }

}

void lvk::init::CleanupDebugOutput(VkState& vk)
{
  PFN_vkVoidFunction rawFunction = vkGetInstanceProcAddr(vk.m_Instance, "vkDestroyDebugUtilsMessengerEXT");
  auto function = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(rawFunction);

  if (function != nullptr) {
    function(vk.m_Instance, vk.m_DebugMessenger, nullptr);
  }
  else
  {
    spdlog::error("Could not find vkDestroyDebugUtilsMessengerEXT function");
    std::cerr << "Could not find vkDestroyDebugUtilsMessengerEXT function";
  }
}

void lvk::init::ListDeviceExtensions(VkState& vk, VkPhysicalDevice physicalDevice)
{
  std::vector<VkExtensionProperties> extensions = GetDeviceAvailableExtensions(vk, physicalDevice);

  VkPhysicalDeviceProperties deviceProperties{};
  vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);
  for (int i = 0; i < extensions.size(); i++)
  {
    spdlog::info("Physical Device : {} : {}", &deviceProperties.deviceName[0], &extensions[i].extensionName[0]);
  }
}

void lvk::init::PopulateDebugMessengerCreateInfo(VkState& vk, VkDebugUtilsMessengerCreateInfoEXT& createInfo)
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
  // TODO: Find a way to quit on a debug error
}



void lvk::init::InitVulkan(VkState& vk, bool enableSwapchainMsaa)
{
  vk.m_CurrentFrameIndex = 0;
  vk.m_UseSwapchainMsaa = enableSwapchainMsaa;
  VK_CHECK(volkInitialize());
  CreateInstance(vk);
  SetupDebugOutput(vk);
  // TODO: Renable on backkend rework
  vk.m_Backend->CreateSurface(vk);
  PickPhysicalDevice(vk);
  CreateLogicalDevice(vk);
  GetMaxUsableSampleCount(vk);
  GetQueueHandles(vk);
  CreateCommandPool(vk);
  CreateSwapChain(vk);
  CreateSwapChainImageViews(vk);
  CreateSwapChainDepthTexture(vk, vk.m_UseSwapchainMsaa);
  CreateSwapChainColourTexture(vk, vk.m_UseSwapchainMsaa);
  CreateBuiltInRenderPasses(vk);
  CreateSwapChainFramebuffers(vk);
  CreateDescriptorSetAllocator(vk);
  CreateSemaphores(vk);
  CreateFences(vk);
  CreateCommandBuffers(vk);
  CreateVmaAllocator(vk);
}

void lvk::init::InitImGui(VkState& vk)
{
  if (!vk.m_UseImGui)
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

  // TODO : Reinit imgui backend on backend refactor
  // InitImGuiBackend();

  ImGui_ImplVulkan_InitInfo init_info = {};

  init_info.Instance = vk.m_Instance;
  init_info.PhysicalDevice = vk.m_PhysicalDevice;
  init_info.Device = vk.m_LogicalDevice;
  init_info.QueueFamily = vk.m_QueueFamilyIndices.m_QueueFamilies[QueueFamilyType::GraphicsAndCompute];
  init_info.Queue = vk.m_GraphicsQueue;
  init_info.PipelineCache = VK_NULL_HANDLE;
  // TODO: this is a bit shit, need to clean this pool some wheere
  init_info.DescriptorPool = vk.m_DescriptorSetAllocator.CreatePool(vk.m_LogicalDevice, 4096);
  init_info.Allocator = nullptr;
  init_info.MinImageCount = 2;
  init_info.ImageCount = 2;
  init_info.RenderPass = vk.m_ImGuiRenderPass;
  if (vk.m_UseSwapchainMsaa)
  {
    init_info.MSAASamples = vk.m_MaxMsaaSamples;
  }
  ImGui_ImplVulkan_Init(&init_info);

}

void lvk::init::RenderImGui(VkState& vk)
{
  ImGui::Render();
  VkCommandBuffer imguiCommandBuffer = BeginSingleTimeCommands(vk);

  VkRenderPassBeginInfo renderPassInfo{};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  renderPassInfo.renderPass = vk.m_ImGuiRenderPass;
  renderPassInfo.framebuffer = vk.m_SwapChainFramebuffers[vk.m_CurrentFrameIndex];
  renderPassInfo.renderArea.offset = { 0,0 };
  renderPassInfo.renderArea.extent = vk.m_SwapChainImageExtent;

  std::array<VkClearValue, 2> clearValues{};
  clearValues[0].color = { {0.0f, 0.0f, 0.0f, 1.0f} };
  clearValues[1].depthStencil = { 1.0f, 0 };

  renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
  renderPassInfo.pClearValues = clearValues.data();

  vkCmdBeginRenderPass(imguiCommandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
  ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), imguiCommandBuffer);
  vkCmdEndRenderPass(imguiCommandBuffer);
  EndSingleTimeCommands(vk, imguiCommandBuffer);
}

VkApplicationInfo lvk::init::CreateAppInfo(VkState& vk)
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

void lvk::init::CreateInstance(VkState& vk)
{
  if (vk.m_UseValidation && !CheckValidationLayerSupport(vk))
  {
    spdlog::error("Validation layers requested but not available.");
    std::cerr << "Validation layers requested but not available.";
  }

  VkApplicationInfo appInfo = CreateAppInfo(vk);

  std::vector<const char*> extensionNames = vk.m_Backend->GetRequiredExtensions(vk);

  if(vk.m_UseValidation) {
    for (const auto &extension: extensionNames) {
      spdlog::info("Backend Extension : {}", extension);
    }
  }
  uint32_t extensionCount;
  vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
  std::vector<VkExtensionProperties> extensions(extensionCount);
  vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

  if(vk.m_UseValidation) {
    spdlog::info("Supported m_Instance extensions: ");
  }
  for (const auto& extension : extensions) {
    if (vk.m_UseValidation) {
      spdlog::info("{}", &extension.extensionName[0]);
    }
    bool shouldAdd = true;
    for (int i = 0; i < extensionNames.size(); i++)
    {
      if (extensionNames[i] == extension.extensionName)
      {
        shouldAdd = false;
      }
    }

    if (shouldAdd && extension.extensionName != NULL)
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

  if (vk.m_UseValidation)
  {
    VkDebugUtilsMessengerCreateInfoEXT debugMessengerCreateInfo;
    createInfo.enabledLayerCount = (uint32_t)s_ValidationLayers.size();
    createInfo.ppEnabledLayerNames = s_ValidationLayers.data();

    PopulateDebugMessengerCreateInfo(vk, debugMessengerCreateInfo);
    createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugMessengerCreateInfo;
  }
  else
  {
    createInfo.enabledLayerCount = 0;
    createInfo.pNext = nullptr;
  }

  if (vkCreateInstance(&createInfo, nullptr, &vk.m_Instance) != VK_SUCCESS)
  {
    spdlog::error("Failed to create vulkan m_Instance");
  }

  volkLoadInstance(vk.m_Instance);

}

void lvk::init::CleanupVulkan(VkState& vk)
{
  vmaDestroyAllocator(vk.m_Allocator);
  CleanupSwapChain(vk);

  vk.m_DescriptorSetAllocator.Free(vk.m_LogicalDevice);

  if (vk.m_UseValidation)
  {
    CleanupDebugOutput(vk);
  }
  for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
  {
    vkDestroySemaphore(vk.m_LogicalDevice, vk.m_ImageAvailableSemaphores[i], nullptr);
    vkDestroySemaphore(vk.m_LogicalDevice, vk.m_RenderFinishedSemaphores[i], nullptr);
    vkDestroyFence(vk.m_LogicalDevice, vk.m_FrameInFlightFences[i], nullptr);
  }

  vkDestroyCommandPool(vk.m_LogicalDevice, vk.m_GraphicsQueueCommandPool, nullptr);
  vkDestroyRenderPass(vk.m_LogicalDevice, vk.m_SwapchainImageRenderPass, nullptr);


  vkDestroySurfaceKHR(vk.m_Instance, vk.m_Surface, nullptr);
  vkDestroyDevice(vk.m_LogicalDevice, nullptr);
  vkDestroyInstance(vk.m_Instance, nullptr);
}

lvk::QueueFamilyIndices lvk::init::FindQueueFamilies(VkState& vk, VkPhysicalDevice m_PhysicalDevice)
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
    vkGetPhysicalDeviceSurfaceSupportKHR(m_PhysicalDevice, i, vk.m_Surface, &presentSupport);

    if (presentSupport == VK_TRUE) {
      indices.m_QueueFamilies[QueueFamilyType::Present] = i;
    }
  }
  return indices;
}

lvk::SwapChainSupportDetais lvk::init::GetSwapChainSupportDetails(VkState& vk, VkPhysicalDevice physicalDevice)
{
  SwapChainSupportDetais details;

  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, vk.m_Surface, &details.m_Capabilities);

  uint32_t supportedNumberFormats = 0;
  vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, vk.m_Surface, &supportedNumberFormats, nullptr);

  if (supportedNumberFormats > 0)
  {
    details.m_SupportedFormats.resize(supportedNumberFormats);
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, vk.m_Surface, &supportedNumberFormats, details.m_SupportedFormats.data());
  }

  uint32_t supportedPresentModes = 0;
  vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, vk.m_Surface, &supportedPresentModes, nullptr);

  if (supportedNumberFormats > 0)
  {
    details.m_SupportedPresentModes.resize(supportedPresentModes);
    vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, vk.m_Surface, &supportedPresentModes, details.m_SupportedPresentModes.data());
  }

  return details;
}

bool lvk::init::IsDeviceSuitable(VkState& vk,VkPhysicalDevice physicalDevice)
{
  QueueFamilyIndices indices = FindQueueFamilies(vk, physicalDevice);
  bool extensionsSupported = CheckDeviceExtensionSupport(vk, physicalDevice);
  bool swapChainSupport = false;
  if (extensionsSupported)
  {
    SwapChainSupportDetais swapChainDetails = GetSwapChainSupportDetails(vk, physicalDevice);
    swapChainSupport = swapChainDetails.m_SupportedFormats.size() > 0 && swapChainDetails.m_SupportedPresentModes.size() > 0;
  }

  VkPhysicalDeviceFeatures supportedFeatures;
  vkGetPhysicalDeviceFeatures(physicalDevice, &supportedFeatures);

  return indices.IsComplete() && extensionsSupported && swapChainSupport && supportedFeatures.samplerAnisotropy && supportedFeatures.wideLines;
}

uint32_t lvk::init::AssessDeviceSuitability(VkState& vk,VkPhysicalDevice m_PhysicalDevice)
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

void lvk::init::PickPhysicalDevice(VkState& vk)
{
  uint32_t deviceCount = 0;
  vkEnumeratePhysicalDevices(vk.m_Instance, &deviceCount, nullptr);

  if (deviceCount == 0)
  {
    spdlog::error("Failed to find any physical devices.");
    std::cerr << "Failed to find any physical devices.\n";
  }

  std::vector<VkPhysicalDevice> physicalDevices(deviceCount);
  vkEnumeratePhysicalDevices(vk.m_Instance, &deviceCount, physicalDevices.data());
  VkPhysicalDevice physicalDeviceCandidate = VK_NULL_HANDLE;
  uint32_t bestScore = 0;

  for (int i = 0; i < physicalDevices.size(); i++)
  {
    uint32_t score = AssessDeviceSuitability(vk, physicalDevices[i]);
    if (IsDeviceSuitable(vk, physicalDevices[i]) && score > bestScore)
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

  vk.m_PhysicalDevice = physicalDeviceCandidate;

  if (vk.m_PhysicalDevice == VK_NULL_HANDLE)
  {
    spdlog::error("Failed to find a suitable physical device.");
    std::cerr << "Failed to find a suitable physical device.\n";
  }

  VkPhysicalDeviceProperties deviceProperties{};
  vkGetPhysicalDeviceProperties(vk.m_PhysicalDevice, &deviceProperties);

  if(vk.m_UseValidation) {
    spdlog::info("Chose GPU : {}", &deviceProperties.deviceName[0]);
    ListDeviceExtensions(vk, vk.m_PhysicalDevice);
  }
}

void lvk::init::CreateLogicalDevice(VkState& vk)
{
  vk.m_QueueFamilyIndices = FindQueueFamilies(vk, vk.m_PhysicalDevice);

  std::vector< VkDeviceQueueCreateInfo> queueCreateInfos;
  float priority = 1.0f;
  for (auto const& [type, index] : vk.m_QueueFamilyIndices.m_QueueFamilies)
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

  createInfo.enabledExtensionCount    = static_cast<uint32_t>(s_DeviceExtensions.size());
  createInfo.ppEnabledExtensionNames  = s_DeviceExtensions.data();

  if (vk.m_UseValidation)
  {
    createInfo.enabledLayerCount    = static_cast<uint32_t>(s_ValidationLayers.size());
    createInfo.ppEnabledLayerNames  = s_ValidationLayers.data();
  }
  else
  {
    createInfo.enabledLayerCount    = 0;
  }

  if (vkCreateDevice(vk.m_PhysicalDevice, &createInfo, nullptr, &vk.m_LogicalDevice))
  {
    spdlog::error("Failed to create logical device.");
    std::cerr << "Failed to create logical device. \n";
  }

  volkLoadDevice(vk.m_LogicalDevice);
}

void lvk::init::GetQueueHandles(VkState& vk)
{
  vkGetDeviceQueue(vk.m_LogicalDevice, vk.m_QueueFamilyIndices.m_QueueFamilies[QueueFamilyType::GraphicsAndCompute],    0, &vk.m_GraphicsQueue);
  vkGetDeviceQueue(vk.m_LogicalDevice, vk.m_QueueFamilyIndices.m_QueueFamilies[QueueFamilyType::GraphicsAndCompute],    0, &vk.m_ComputeQueue);
  vkGetDeviceQueue(vk.m_LogicalDevice, vk.m_QueueFamilyIndices.m_QueueFamilies[QueueFamilyType::Present],               0, &vk.m_PresentQueue);
}

VkSurfaceFormatKHR lvk::init::ChooseSwapChainSurfaceFormat(VkState& vk, std::vector<VkSurfaceFormatKHR> availableFormats)
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

VkPresentModeKHR lvk::init::ChooseSwapChainPresentMode(VkState& vk, std::vector<VkPresentModeKHR> availableModes)
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

void lvk::init::CreateSwapChain(VkState& vk)
{
  SwapChainSupportDetais swapChainDetails = GetSwapChainSupportDetails(vk, vk.m_PhysicalDevice);

  VkSurfaceFormatKHR format       = ChooseSwapChainSurfaceFormat(vk, swapChainDetails.m_SupportedFormats);
  VkPresentModeKHR presentMode    = ChooseSwapChainPresentMode(vk, swapChainDetails.m_SupportedPresentModes);
  VkExtent2D surfaceExtent        = ChooseSwapExtent(vk, swapChainDetails.m_Capabilities);
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
  createInfo.surface          = vk.m_Surface;
  createInfo.minImageCount    = swapChainImageCount;
  createInfo.clipped          = VK_TRUE;
  createInfo.imageFormat      = format.format;
  createInfo.imageColorSpace  = format.colorSpace;
  createInfo.presentMode      = presentMode;
  createInfo.imageExtent      = surfaceExtent;
  // This is always 1 unless you are developing a stereoscopic 3D application
  createInfo.imageArrayLayers = 1;
  createInfo.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

  uint32_t queueFamilyIndices[] = { vk.m_QueueFamilyIndices.m_QueueFamilies[QueueFamilyType::GraphicsAndCompute], vk.m_QueueFamilyIndices.m_QueueFamilies[QueueFamilyType::Present] };

  if (vk.m_QueueFamilyIndices.m_QueueFamilies[QueueFamilyType::GraphicsAndCompute] != vk.m_QueueFamilyIndices.m_QueueFamilies[QueueFamilyType::Present])
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

  if (vkCreateSwapchainKHR(vk.m_LogicalDevice, &createInfo, nullptr, &vk.m_SwapChain) != VK_SUCCESS)
  {
    spdlog::error("Failed to create swapchain.");
    std::cerr << "Failed to create swapchain" << std::endl;
  }

  vkGetSwapchainImagesKHR(vk.m_LogicalDevice, vk.m_SwapChain, &swapChainImageCount, nullptr);
  vk.m_SwapChainImages.resize(swapChainImageCount);
  vkGetSwapchainImagesKHR(vk.m_LogicalDevice, vk.m_SwapChain, &swapChainImageCount, vk.m_SwapChainImages.data());

  vk.m_SwapChainImageFormat = format.format;
  vk.m_SwapChainImageExtent = surfaceExtent;
}

void lvk::init::CreateSwapChainFramebuffers(VkState& vk)
{
  vk.m_SwapChainFramebuffers.resize(vk.m_SwapChainImageViews.size());

  for (uint32_t i = 0; i < vk.m_SwapChainImageViews.size(); i++)
  {
    std::vector<VkImageView> attachments;

    if (vk.m_UseSwapchainMsaa)
    {
      attachments.resize(3);
      attachments[0] = vk.m_SwapChainColourImageView;
      attachments[1] = vk.m_SwapChainDepthImageView;
      attachments[2] = vk.m_SwapChainImageViews[i];
    }
    else
    {
      attachments.resize(2);
      attachments[0] = vk.m_SwapChainImageViews[i];
      attachments[1] = vk.m_SwapChainDepthImageView;

    }

    CreateFramebuffer(vk, attachments, vk.m_SwapchainImageRenderPass, vk.m_SwapChainImageExtent, vk.m_SwapChainFramebuffers[i]);
  }
}

void lvk::init::CreateSwapChainImageViews(VkState& vk)
{
  vk.m_SwapChainImageViews.resize(vk.m_SwapChainImages.size());

  for (uint32_t i = 0; i < vk.m_SwapChainImages.size(); i++)
  {
    CreateImageView(vk, vk.m_SwapChainImages[i], vk.m_SwapChainImageFormat, 1, VK_IMAGE_ASPECT_COLOR_BIT,  vk.m_SwapChainImageViews[i]);
  }
}

void lvk::init::CleanupSwapChain(VkState& vk)
{
  for (int i = 0; i < vk.m_SwapChainFramebuffers.size(); i++)
  {
    vkDestroyFramebuffer(vk.m_LogicalDevice, vk.m_SwapChainFramebuffers[i], nullptr);
  }
  for (int i = 0; i < vk.m_SwapChainImageViews.size(); i++)
  {
    vkDestroyImageView(vk.m_LogicalDevice, vk.m_SwapChainImageViews[i], nullptr);
  }

  vkDestroyImageView(vk.m_LogicalDevice, vk.m_SwapChainColourImageView, nullptr);
  vkDestroyImage(vk.m_LogicalDevice, vk.m_SwapChainColourImage, nullptr);
  vkFreeMemory(vk.m_LogicalDevice, vk.m_SwapChainColourImageMemory, nullptr);

  vkDestroyImageView(vk.m_LogicalDevice, vk.m_SwapChainDepthImageView, nullptr);
  vkDestroyImage(vk.m_LogicalDevice, vk.m_SwapChainDepthImage, nullptr);
  vkFreeMemory(vk.m_LogicalDevice, vk.m_SwapChainDepthImageMemory, nullptr);

  vkDestroySwapchainKHR(vk.m_LogicalDevice, vk.m_SwapChain, nullptr);
}

void lvk::init::RecreateSwapChain(VkState& vk)
{
  spdlog::info("VulkanAPI : Recreating Swapchain");
  while (vkDeviceWaitIdle(vk.m_LogicalDevice) != VK_SUCCESS);

  CleanupSwapChain(vk);
  CleanupImGui(vk);

  CreateSwapChain(vk);
  CreateSwapChainImageViews(vk);
  CreateSwapChainColourTexture(vk, vk.m_UseSwapchainMsaa);
  CreateSwapChainDepthTexture(vk, vk.m_UseSwapchainMsaa);
  CreateSwapChainFramebuffers(vk);

  InitImGui(vk);
}

void lvk::init::CreateSwapChainColourTexture(VkState& vk, bool enableMsaa)
{
  VkSampleCountFlagBits sampleCount = VK_SAMPLE_COUNT_1_BIT;

  if (enableMsaa)
  {
    sampleCount = vk.m_MaxMsaaSamples;
  }

  CreateImage(vk, vk.m_SwapChainImageExtent.width, vk.m_SwapChainImageExtent.height, 1, sampleCount,
              vk.m_SwapChainImageFormat,
              VK_IMAGE_TILING_OPTIMAL,
              VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
              VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
              vk.m_SwapChainColourImage,
              vk.m_SwapChainColourImageMemory);

  CreateImageView(vk, vk.m_SwapChainColourImage, vk.m_SwapChainImageFormat, 1, VK_IMAGE_ASPECT_COLOR_BIT, vk.m_SwapChainColourImageView);
}

void lvk::init::CreateSwapChainDepthTexture(VkState& vk, bool enableMsaa )
{
  VkFormat depthFormat = FindDepthFormat(vk);

  VkSampleCountFlagBits sampleCount = VK_SAMPLE_COUNT_1_BIT;

  if (enableMsaa)
  {
    sampleCount = vk.m_MaxMsaaSamples;
  }

  CreateImage(vk, vk.m_SwapChainImageExtent.width, vk.m_SwapChainImageExtent.height, 1, sampleCount,
              depthFormat,
              VK_IMAGE_TILING_OPTIMAL,
              VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
              VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
              vk.m_SwapChainDepthImage,
              vk.m_SwapChainDepthImageMemory);

  CreateImageView(vk, vk.m_SwapChainDepthImage, depthFormat, 1, VK_IMAGE_ASPECT_DEPTH_BIT, vk.m_SwapChainDepthImageView);
  TransitionImageLayout(vk, vk.m_SwapChainDepthImage, depthFormat, 1, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
}

VkExtent2D lvk::init::ChooseSwapExtent(VkState& vk, VkSurfaceCapabilitiesKHR& surfaceCapabilities)
{
  if(surfaceCapabilities.currentExtent.width != UINT32_MAX)
  {
    return surfaceCapabilities.currentExtent;
  }
  else
  {
    return vk.m_Backend->GetSurfaceExtent(vk, surfaceCapabilities);
  }
}

void lvk::init::CreateCommandPool(VkState& vk)
{
  VkCommandPoolCreateInfo createInfo{};
  createInfo.sType                = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  createInfo.queueFamilyIndex     = vk.m_QueueFamilyIndices.m_QueueFamilies[QueueFamilyType::GraphicsAndCompute];
  createInfo.flags                = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

  if(vkCreateCommandPool(vk.m_LogicalDevice, &createInfo, nullptr, &vk.m_GraphicsQueueCommandPool) != VK_SUCCESS)
  {
    spdlog::error("Failed to create Command Pool!");
    std::cerr << "Failed to create Command Pool!" << std::endl;
  }
}

void lvk::init::CreateDescriptorSetAllocator(VkState& vk)
{
  vk.m_DescriptorSetAllocator.Init(vk.m_LogicalDevice, MAX_FRAMES_IN_FLIGHT * 128, {
                                                                       {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0.33f},
                                                                       {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0.33f},
                                                                       {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 0.33f},
                                                                   });
}

void lvk::init::CreateSemaphores(VkState& vk)
{
  vk.m_ImageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
  vk.m_RenderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
  VkSemaphoreCreateInfo createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

  for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
  {
    if (vkCreateSemaphore(vk.m_LogicalDevice, &createInfo, nullptr, &vk.m_ImageAvailableSemaphores[i]) != VK_SUCCESS ||
        vkCreateSemaphore(vk.m_LogicalDevice, &createInfo, nullptr, &vk.m_RenderFinishedSemaphores[i]) != VK_SUCCESS)
    {
      spdlog::error("Failed to create semaphores!");
      std::cerr << "Failed to create semaphores!" << std::endl;
    }
  }

}

void lvk::init::CreateFences(VkState& vk)
{
  vk.m_FrameInFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
  vk.m_ImagesInFlight.resize(vk.m_SwapChainImages.size(), VK_NULL_HANDLE);

  VkFenceCreateInfo createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  createInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

  for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
  {
    if (vkCreateFence(vk.m_LogicalDevice, &createInfo, nullptr, &vk.m_FrameInFlightFences[i]) != VK_SUCCESS)
    {
      spdlog::error("Failed to create Fences!");
      std::cerr << "Failed to create Fences!" << std::endl;
    }
  }
}

void lvk::init::DrawFrame(VkState& vk)
{
  vkWaitForFences(vk.m_LogicalDevice, 1, &vk.m_FrameInFlightFences[vk.m_CurrentFrameIndex], VK_TRUE, UINT64_MAX);

  uint32_t imageIndex;
  VkResult result = vkAcquireNextImageKHR(vk.m_LogicalDevice, vk.m_SwapChain,
          UINT64_MAX, vk.m_ImageAvailableSemaphores[vk.m_CurrentFrameIndex], VK_NULL_HANDLE, &imageIndex);

  if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
    RecreateSwapChain(vk);
    return;
  }
  else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
    spdlog::error("VulkanAPI : Failed to acquire swap chain image!");
    return;
  }

  vkResetFences(vk.m_LogicalDevice, 1, &vk.m_FrameInFlightFences[vk.m_CurrentFrameIndex]);

  vk.m_ImagesInFlight[imageIndex] = vk.m_FrameInFlightFences[vk.m_CurrentFrameIndex];

  VkSubmitInfo submitInfo{};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

  VkSemaphore waitSemaphores[]        = { vk.m_ImageAvailableSemaphores[vk.m_CurrentFrameIndex]};
  VkSemaphore signalSemaphores[]       = { vk.m_RenderFinishedSemaphores[vk.m_CurrentFrameIndex]};
  VkPipelineStageFlags waitStages[]   = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
  submitInfo.waitSemaphoreCount       = 1;
  submitInfo.pWaitSemaphores          = waitSemaphores;
  submitInfo.pWaitDstStageMask        = waitStages;
  submitInfo.commandBufferCount       = 1;
  submitInfo.pCommandBuffers          = &vk.m_CommandBuffers[imageIndex];
  submitInfo.signalSemaphoreCount     = 1;
  submitInfo.pSignalSemaphores        = signalSemaphores;

  vkResetFences(vk.m_LogicalDevice, 1, &vk.m_FrameInFlightFences[vk.m_CurrentFrameIndex]);

  if (vkQueueSubmit(vk.m_GraphicsQueue, 1, &submitInfo, vk.m_FrameInFlightFences[vk.m_CurrentFrameIndex]) != VK_SUCCESS)
  {
    spdlog::error("VulkanAPI : Failed to submit draw command buffer!");
  }

  if (vk.m_UseImGui)
  {
    RenderImGui(vk);
  }

  VkPresentInfoKHR presentInfo{};
  presentInfo.sType               = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  presentInfo.waitSemaphoreCount  = 1;
  presentInfo.pWaitSemaphores     = signalSemaphores;
  presentInfo.swapchainCount      = 1;
  VkSwapchainKHR swapchains[]     = { vk.m_SwapChain };
  presentInfo.pSwapchains         = swapchains;
  presentInfo.pImageIndices       = &imageIndex;
  presentInfo.pResults            = nullptr;

  result = vkQueuePresentKHR(vk.m_GraphicsQueue, &presentInfo);

  if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
    RecreateSwapChain(vk);
    return;
  }
  else if (result != VK_SUCCESS) {
    spdlog::error("VulkanAPI : Error presenting swapchain image");
  }

  vk.m_CurrentFrameIndex = (vk.m_CurrentFrameIndex + 1) % MAX_FRAMES_IN_FLIGHT;
}

void lvk::init::CreateCommandBuffers(VkState& vk)
{
  vk.m_CommandBuffers.resize(vk.m_SwapChainFramebuffers.size());

  VkCommandBufferAllocateInfo allocateInfo{};
  allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocateInfo.commandPool = vk.m_GraphicsQueueCommandPool;
  allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocateInfo.commandBufferCount = static_cast<uint32_t>(vk.m_CommandBuffers.size());

  VK_CHECK(vkAllocateCommandBuffers(vk.m_LogicalDevice, &allocateInfo, vk.m_CommandBuffers.data()))

}

void lvk::init::ClearCommandBuffers(VkState& vk)
{
  for (uint32_t i = 0; i < vk.m_CommandBuffers.size(); i++)
  {
    vkResetCommandBuffer(vk.m_CommandBuffers[i], 0);
  }
}

void lvk::init::CreateVmaAllocator(VkState& vk)
{
  VmaVulkanFunctions vulkanFunctions = {};
  vulkanFunctions.vkGetInstanceProcAddr = vkGetInstanceProcAddr;
  vulkanFunctions.vkGetDeviceProcAddr = vkGetDeviceProcAddr;

  VmaAllocatorCreateInfo allocatorCreateInfo = {};
  allocatorCreateInfo.flags = VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT;
  allocatorCreateInfo.vulkanApiVersion = VK_API_VERSION_1_3;
  allocatorCreateInfo.physicalDevice = vk.m_PhysicalDevice;
  allocatorCreateInfo.device = vk.m_LogicalDevice;
  allocatorCreateInfo.instance = vk.m_Instance;
  allocatorCreateInfo.pVulkanFunctions = &vulkanFunctions;

  VK_CHECK(vmaCreateAllocator(&allocatorCreateInfo, &vk.m_Allocator));
}

void lvk::init::GetMaxUsableSampleCount(VkState& vk)
{
  VkPhysicalDeviceProperties physicalDeviceProperties;
  vkGetPhysicalDeviceProperties(vk.m_PhysicalDevice, &physicalDeviceProperties);

  VK_SAMPLE_COUNT_2_BIT;
  VK_SAMPLE_COUNT_4_BIT;
  VK_SAMPLE_COUNT_8_BIT;
  VK_SAMPLE_COUNT_16_BIT;

  auto counts = physicalDeviceProperties.limits.framebufferColorSampleCounts & physicalDeviceProperties.limits.framebufferDepthSampleCounts;
  if (counts & VK_SAMPLE_COUNT_64_BIT) {  vk.m_MaxMsaaSamples = VK_SAMPLE_COUNT_64_BIT;  return ;}
  if (counts & VK_SAMPLE_COUNT_32_BIT) {  vk.m_MaxMsaaSamples = VK_SAMPLE_COUNT_32_BIT;  return ;}
  if (counts & VK_SAMPLE_COUNT_16_BIT) {  vk.m_MaxMsaaSamples = VK_SAMPLE_COUNT_16_BIT;  return ;}
  if (counts & VK_SAMPLE_COUNT_8_BIT) {   vk.m_MaxMsaaSamples = VK_SAMPLE_COUNT_8_BIT;   return ;}
  if (counts & VK_SAMPLE_COUNT_4_BIT) {   vk.m_MaxMsaaSamples = VK_SAMPLE_COUNT_4_BIT;   return ;}
  if (counts & VK_SAMPLE_COUNT_2_BIT) {   vk.m_MaxMsaaSamples = VK_SAMPLE_COUNT_2_BIT;   return ;}
}

void lvk::init::CreateBuiltInRenderPasses(lvk::VkState &vk) {

  {
    Vector<VkAttachmentDescription> colourAttachmentDescriptions{};
    Vector<VkAttachmentDescription> resolveAttachmentDescriptions{};
    VkAttachmentDescription depthAttachmentDescription{};

    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = vk.m_SwapChainImageFormat;
    colorAttachment.samples = vk.m_UseSwapchainMsaa ? vk.m_MaxMsaaSamples : VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    if (vk.m_UseSwapchainMsaa)
    {
      colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    }
    else
    {
      colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    }
    colourAttachmentDescriptions.push_back(colorAttachment);

    depthAttachmentDescription.format = FindDepthFormat(vk);
    depthAttachmentDescription.samples = vk.m_UseSwapchainMsaa ? vk.m_MaxMsaaSamples : VK_SAMPLE_COUNT_1_BIT;;
    depthAttachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachmentDescription.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentDescription colorAttachmentResolve{};
    if (vk.m_UseSwapchainMsaa)
    {
      colorAttachmentResolve.format = vk.m_SwapChainImageFormat;
      colorAttachmentResolve.samples = VK_SAMPLE_COUNT_1_BIT;
      colorAttachmentResolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
      colorAttachmentResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
      colorAttachmentResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
      colorAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
      colorAttachmentResolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
      colorAttachmentResolve.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
      resolveAttachmentDescriptions.push_back(colorAttachmentResolve);
    }

    CreateRenderPass(vk, vk.m_SwapchainImageRenderPass, colourAttachmentDescriptions, resolveAttachmentDescriptions, true, depthAttachmentDescription, VK_ATTACHMENT_LOAD_OP_CLEAR);
  }
  {
    Vector<VkAttachmentDescription> colourAttachmentDescriptions{};
    Vector<VkAttachmentDescription> resolveAttachmentDescriptions{};
    VkAttachmentDescription depthAttachmentDescription{};

    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = vk.m_SwapChainImageFormat;
    colorAttachment.samples = vk.m_UseSwapchainMsaa ? vk.m_MaxMsaaSamples : VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    if (vk.m_UseSwapchainMsaa)
    {
      colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    }
    else
    {
      colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    }
    colourAttachmentDescriptions.push_back(colorAttachment);

    depthAttachmentDescription.format = FindDepthFormat(vk);
    depthAttachmentDescription.samples = vk.m_UseSwapchainMsaa ? vk.m_MaxMsaaSamples : VK_SAMPLE_COUNT_1_BIT;;
    depthAttachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachmentDescription.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentDescription colorAttachmentResolve{};
    if (vk.m_UseSwapchainMsaa)
    {
      colorAttachmentResolve.format = vk.m_SwapChainImageFormat;
      colorAttachmentResolve.samples = VK_SAMPLE_COUNT_1_BIT;
      colorAttachmentResolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
      colorAttachmentResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
      colorAttachmentResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
      colorAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
      colorAttachmentResolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
      colorAttachmentResolve.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
      resolveAttachmentDescriptions.push_back(colorAttachmentResolve);
    }
    CreateRenderPass(vk, vk.m_ImGuiRenderPass, colourAttachmentDescriptions, resolveAttachmentDescriptions, true, depthAttachmentDescription, VK_ATTACHMENT_LOAD_OP_DONT_CARE);
  }
}


std::vector<VkExtensionProperties> lvk::init::GetDeviceAvailableExtensions(VkState& vk, VkPhysicalDevice physicalDevice)
{
  uint32_t extensionCount;
  vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, nullptr);

  std::vector<VkExtensionProperties> availableExtensions(extensionCount);
  vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, availableExtensions.data());

  return availableExtensions;


}
