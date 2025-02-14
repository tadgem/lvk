#define VK_NO_PROTOTYPES
#include "ImGui/imgui_impl_vulkan.h"
#include "lvk/Init.h"
#include "lvk/Commands.h"
#include "lvk/Macros.h"
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

std::vector<VkExtensionProperties> lvk::init::GetDeviceAvailableExtensions(VkState& vk, VkPhysicalDevice physicalDevice)
{
  uint32_t extensionCount;
  vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, nullptr);

  std::vector<VkExtensionProperties> availableExtensions(extensionCount);
  vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, availableExtensions.data());

  return availableExtensions;


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
void lvk::init::CreateBuiltInRenderPasses(lvk::VkState &vk) {

  // todo: port from VkAPi
}
