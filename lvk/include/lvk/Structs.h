#pragma once

#define VK_NO_PROTOTYPES
#include "volk.h"
#include "ThirdParty/VulkanMemoryAllocator.h"
#include "Alias.h"
#include "lvk/DescriptorSetAllocator.h"


namespace lvk {

  class VkAPI;

  enum class ShaderBindingType {
    UniformBuffer,
    ShaderStorageBuffer,
    PushConstants,
    Sampler
  };

  enum class ShaderBufferMemberType {
    UNKNOWN,
    _vec2,
    _vec3,
    _vec4,
    _mat2,
    _mat3,
    _mat4,
    _float,
    _double,
    _int,
    _uint,
    _array,
    _sampler
  };

  class VulkanAPIWindowHandle {};

  using StageBinary = std::vector<unsigned char>;

  struct PushConstantBlock {
    uint32_t m_Size;
    uint32_t m_Offset;
    String m_Name;
    VkShaderStageFlags m_Stage;
  };

  struct ShaderBufferMember {
    uint32_t m_Size;
    uint32_t m_Offset;
    uint32_t m_Stride;
    String m_Name;
    ShaderBufferMemberType m_Type;
  };

  struct DescriptorSetLayoutBindingData {
    String m_BindingName;
    uint32_t m_BindingIndex;
    uint32_t m_ExpectedBufferSize;
    ShaderBindingType m_BufferType;
    Vector<ShaderBufferMember> m_Members;
  };

  struct DescriptorSetLayoutData {
    uint32_t m_SetNumber;
    VkDescriptorSetLayoutCreateInfo m_CreateInfo;
    VkDescriptorSetLayout m_Layout;
    Vector<VkDescriptorSetLayoutBinding> m_Bindings;
    Vector<DescriptorSetLayoutBindingData> m_BindingDatas;
  };

  // reuse this for generic cpu dynamic buffer
  struct MappedBuffer {
    VkBuffer m_GpuBuffer;
    VmaAllocation m_GpuMemory;
    void *m_MappedAddr;

    void Free(VkAPI &vk);
  };

  struct ShaderBufferFrameData {
    Vector<MappedBuffer> m_UniformBuffers;

    template <typename _Ty>
    void Set(uint32_t frameIndex, const _Ty &data, uint32_t offset = 0) {
      constexpr size_t _ty_size = sizeof(_Ty);
      uint64_t base_addr = (uint64_t)m_UniformBuffers[frameIndex].m_MappedAddr;
      void *addr = (void *)(base_addr + static_cast<uint64_t>(offset));
      memcpy(addr, &data, _ty_size);
    }

    template <typename _Ty>
    void SetMemory(uint32_t frameIndex, const _Ty *start, uint64_t count) {
      constexpr size_t _ty_size = sizeof(_Ty);
      void *addr = m_UniformBuffers[frameIndex].m_MappedAddr;
      memcpy(addr, start, count);
    }

    void Free(VkAPI &vk);
  };

  enum class ShaderStageType { Vertex, Fragment };

  enum QueueFamilyType {
    GraphicsAndCompute = VK_QUEUE_GRAPHICS_BIT,
    Transfer = VK_QUEUE_TRANSFER_BIT,
    Present = 8

  };

  struct QueueFamilyIndices {
    HashMap<QueueFamilyType, uint32_t> m_QueueFamilies;

    bool IsComplete();
  };

  struct SwapChainSupportDetais {
    VkSurfaceCapabilitiesKHR    m_Capabilities;
    Vector<VkSurfaceFormatKHR>  m_SupportedFormats;
    Vector<VkPresentModeKHR>    m_SupportedPresentModes;
  };

  struct VkState;

  class VkBackend
  {
  public:
    virtual Vector<const char*>         GetRequiredExtensions(VkState& vk) = 0;
    virtual void                        CreateSurface(VkState& vk) = 0;
    virtual void                        CreateWindowLVK(VkState& vk, uint32_t width, uint32_t height) = 0;
    virtual void                        CleanupWindow(VkState& vk) = 0;
    virtual VkExtent2D                  GetSurfaceExtent(VkState& vk, VkSurfaceCapabilitiesKHR surface) = 0;
    virtual VkExtent2D                  GetMaxFramebufferResolution(VkState& vk) = 0;
    virtual bool                        ShouldRun(VkState& vk) = 0;
    virtual void                        PreFrame(VkState& vk) = 0;
    virtual void                        PostFrame(VkState& vk) = 0;
    virtual void                        Run(VkState& vk, std::function<void()> callback) = 0;
    virtual void                        InitImGuiBackend(VkState& vk) = 0;
    virtual void                        CleanupImGuiBackend(VkState& vk) = 0;
  };

  struct VkState
  {
    Unique<VkBackend>               m_Backend;

    VkInstance                      m_Instance;
    VkSurfaceKHR                    m_Surface;
    VkSwapchainKHR                  m_SwapChain;
    VkDebugUtilsMessengerEXT        m_DebugMessenger;
    VkPhysicalDevice                m_PhysicalDevice = VK_NULL_HANDLE;
    VkDevice                        m_LogicalDevice = VK_NULL_HANDLE;
    VkRenderPass                    m_SwapchainImageRenderPass;
    VkRenderPass                    m_ImGuiRenderPass;
    VkCommandPool                   m_GraphicsQueueCommandPool;
    VmaAllocator                    m_Allocator;
    DescriptorSetAllocator          m_DescriptorSetAllocator;

    Vector<VkSemaphore>             m_ImageAvailableSemaphores;
    Vector<VkSemaphore>             m_RenderFinishedSemaphores;
    Vector<VkFence>                 m_FrameInFlightFences;
    Vector<VkFence>                 m_ImagesInFlight;
    QueueFamilyIndices              m_QueueFamilyIndices;

    VkQueue                         m_GraphicsQueue = VK_NULL_HANDLE;
    VkQueue                         m_ComputeQueue = VK_NULL_HANDLE;
    VkQueue                         m_PresentQueue = VK_NULL_HANDLE;

    VulkanAPIWindowHandle*          m_WindowHandle;

    Vector<VkImage>                 m_SwapChainImages;
    Vector<VkImageView>             m_SwapChainImageViews;
    Vector<VkFramebuffer>           m_SwapChainFramebuffers;
    Vector<VkCommandBuffer>         m_CommandBuffers;

    VkFormat                        m_SwapChainImageFormat;
    VkExtent2D                      m_SwapChainImageExtent;

    VkImage                         m_SwapChainColourImage;
    VkDeviceMemory                  m_SwapChainColourImageMemory;
    VkImageView                     m_SwapChainColourImageView;

    VkImage                         m_SwapChainDepthImage;
    VkDeviceMemory                  m_SwapChainDepthImageMemory;
    VkImageView                     m_SwapChainDepthImageView;

    VkSampleCountFlagBits           m_MaxMsaaSamples;

    double                          m_DeltaTime;
    bool                            m_ShouldRun = true;
    bool                            m_UseSwapchainMsaa = false;
    const bool                      m_UseValidation = true;
    const bool                      m_UseImGui      = true;
    uint64_t                        m_LastFrameTime;
    int                             m_CurrentFrameIndex;
    VkExtent2D                      m_MaxFramebufferExtent;
    String                          m_AppName;
  };

}
