#pragma once
#include "Mesh.h"
#include "lvk/Structs.h"

namespace lvk {

namespace init
{
  const Vector<const char*>   s_ValidationLayers = {
      "VK_LAYER_KHRONOS_validation"
  };
  const Vector<const char*>   s_DeviceExtensions = {
      "VK_KHR_swapchain"
  };

  void                                CleanupImGui(VkState& vk);
  void                                Cleanup(VkState& vk);

  bool                                CheckValidationLayerSupport(VkState& vk);
  bool                                CheckDeviceExtensionSupport(VkState& vk, VkPhysicalDevice device);
  void                                SetupDebugOutput(VkState& vk);
  void                                CleanupDebugOutput(VkState& vk);
  void                                ListDeviceExtensions(VkState& vk, VkPhysicalDevice physicalDevice);
  void                                PopulateDebugMessengerCreateInfo(VkState& vk,VkDebugUtilsMessengerCreateInfoEXT& createInfo);
  
  void                                InitVulkan(VkState& vk, bool enableSwapchainMsaa = false);
  void                                InitImGui(VkState& vk);
  void                                RenderImGui(VkState& vk);
  VkApplicationInfo                   CreateAppInfo(VkState& vk);
  void                                CreateInstance(VkState& vk);
  void                                CleanupVulkan(VkState& vk);
  QueueFamilyIndices                  FindQueueFamilies(VkState& vk,VkPhysicalDevice physicalDevice);
  SwapChainSupportDetais              GetSwapChainSupportDetails(VkState& vk, VkPhysicalDevice physicalDevice);
  bool                                IsDeviceSuitable(VkState& vk, VkPhysicalDevice physicalDevice);
  uint32_t                            AssessDeviceSuitability(VkState& vk, VkPhysicalDevice physicalDevice);
  void                                PickPhysicalDevice(VkState& vk);
  void                                CreateLogicalDevice(VkState& vk);
  void                                GetQueueHandles(VkState& vk);
  VkSurfaceFormatKHR                  ChooseSwapChainSurfaceFormat(VkState& vk,Vector<VkSurfaceFormatKHR> availableFormats);
  VkPresentModeKHR                    ChooseSwapChainPresentMode(VkState& vk, Vector<VkPresentModeKHR> availableModes);
  void                                CreateSwapChain(VkState& vk);
  void                                CreateSwapChainFramebuffers(VkState& vk);
  void                                CreateSwapChainImageViews(VkState& vk);
  void                                CleanupSwapChain(VkState& vk);
  void                                RecreateSwapChain(VkState& vk);
  void                                CreateSwapChainColourTexture(VkState& vk, bool enableMsaa = false);
  void                                CreateSwapChainDepthTexture(VkState& vk, bool enableMsaa = false);
  VkExtent2D                          ChooseSwapExtent(VkState& vk, VkSurfaceCapabilitiesKHR& surfaceCapabilities);
  void                                CreateCommandPool(VkState& vk);
  void                                CreateDescriptorSetAllocator(VkState& vk);
  void                                CreateSemaphores(VkState& vk);
  void                                CreateFences(VkState& vk);
  void                                DrawFrame(VkState& vk);
  void                                CreateCommandBuffers(VkState& vk);
  void                                ClearCommandBuffers(VkState& vk);
  void                                CreateVmaAllocator(VkState& vk);
  void                                GetMaxUsableSampleCount(VkState& vk);


  Vector<VkExtensionProperties>       GetDeviceAvailableExtensions(VkState& vk, VkPhysicalDevice physicalDevice);

  void                                CreateBuiltInRenderPasses(VkState& vk);
  void                                Quit(VkState& vk);


  template<typename _BackendTy>
  VkState                             Create(const String& appName, uint32_t width, uint32_t height, bool enableSwapchainMsaa)
  {
    static_assert(std::is_base_of<VkBackend, _BackendTy>::value, "Backend must inherit from VkBackend");
    VkState vk;
    vk.m_AppName = appName;
    vk.m_Backend = std::make_unique<_BackendTy>();
    vk.m_Backend->CreateWindowLVK(vk, width, height);
    InitVulkan(vk, enableSwapchainMsaa);
    vk.m_MaxFramebufferExtent = vk.m_Backend->GetMaxFramebufferResolution(vk);
    InitImGui(vk);
    Texture::InitDefaultTexture(vk);
    Mesh::InitBuiltInMeshes(vk);

    return vk;
  }
}

}