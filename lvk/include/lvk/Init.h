#pragma once
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
}

}