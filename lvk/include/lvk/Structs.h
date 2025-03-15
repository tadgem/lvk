#pragma once

#define VK_NO_PROTOTYPES
#include "volk.h"
#include "ThirdParty/VulkanMemoryAllocator.h"
#include "Alias.h"
#include "lvk/DescriptorSetAllocator.h"
#include "lvk/Macros.h"

namespace lvk {

  struct VkState;

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
    uint32_t m_ExpectedBufferSizeOrDivisor;
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

  class Buffer
  {
  public:
    enum BufferStorageType
    {
        GPUOnly,
        Mapped
    };

    enum BufferType
    {
        Uniform,
        ShaderStorage
    };

    VkBuffer                    m_GpuBuffer = VK_NULL_HANDLE;
    VmaAllocation               m_GpuMemory = VK_NULL_HANDLE;
    VkDeviceSize                m_Size = 0;
    BufferStorageType           m_Type;

    Buffer(const BufferStorageType& bufferType, VkBuffer buf, VmaAllocation alloc, VkDeviceSize size);
    Buffer() = default;
    virtual ~Buffer() = default;
    virtual void Free(VkState& vk);
  };

  // reuse this for generic cpu dynamic buffer
  class MappedBuffer : public Buffer {
  public:
    
    MappedBuffer(Buffer& b);
    MappedBuffer() = default;

    void *m_MappedAddr = nullptr;


    void Map(VkState& vk);
    void Free(VkState &vk) override;
  };

  struct ShaderBufferFrameData {
    Array<RefCntPtr<Buffer>, MAX_FRAMES_IN_FLIGHT> m_UniformBuffers;
    ShaderBufferFrameData() = default;

    bool Ready();
    bool CanSet(uint32_t frameIndex);

    template <typename _Ty>
    void SetMemory(uint32_t frameIndex, const _Ty *start, uint64_t count) {
      constexpr size_t _ty_size = sizeof(_Ty);
      if (!CanSet(frameIndex))
      {
          return;
      }
      MappedBuffer* mb = static_cast<MappedBuffer*>(m_UniformBuffers[frameIndex].get());
      void *addr = mb->m_MappedAddr;
      memcpy(addr, start, count);
    }


    template <typename _Ty>
    void Set(uint32_t frameIndex, const _Ty& data, uint32_t offset = 0) {
        constexpr size_t _ty_size = sizeof(_Ty);
        if (!CanSet(frameIndex))
        {
            return;
        }
        MappedBuffer* mb = static_cast<MappedBuffer*>(m_UniformBuffers[frameIndex].get());
        uint64_t base_addr = (uint64_t)mb->m_MappedAddr;
        void* addr = (void*)(base_addr + static_cast<uint64_t>(offset));
        memcpy(addr, &data, _ty_size);
    }


    void Free(VkState &vk);
  };

  struct DescriptorSetBinding {
      union {
          uint64_t        m_Data;
          struct {
              uint32_t    m_Set;
              uint32_t    m_Binding;
          };
      };
      VkDeviceSize        m_BindingSize;

      DescriptorSetBinding(uint32_t set, uint32_t binding, VkDeviceSize size) :
          m_Set(set), m_Binding(binding), m_BindingSize(size) {}
      DescriptorSetBinding() = default;
      

      bool operator ==(const DescriptorSetBinding& other) const {
          return (this->m_Data == other.m_Data);
          // TODO: This size should account for alignment diffs between GPU & CPU
          // && (this->m_BindingSize== other.m_BindingSize);
      }
  };

  struct VertexDescription
  {
    Vector<VkVertexInputBindingDescription>   m_BindingDescriptions;
    Vector<VkVertexInputAttributeDescription> m_AttributeDescriptions;
  };

  struct RasterizationState {
    VkPolygonMode         m_PolygonMode;
    VkCullModeFlags       m_CullMode;
    bool                  m_EnableMSAA;
    VkCompareOp           m_DepthCompareOp;
    VkPrimitiveTopology   m_InputAssemblyTopology;
    VkFrontFace           m_FrontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    float                 m_LineWidth = 1.0f;
  };

  struct PipelineAttachmentState
  {
    Vector<VkPipelineColorBlendAttachmentState> m_ColourAttachmentStates;
    VkPipelineColorBlendStateCreateInfo         m_BlendStateInfo;
  };

  struct PipelineDynamicState
  {
    Vector<VkDynamicState>            m_DynamicStates;
    VkPipelineDynamicStateCreateInfo  m_DynamicStateInfo;
  };

  struct RenderPassInfo
  {
    Vector<VkFramebuffer>           m_SwapchainFramebuffers;
    VkRenderPass                    m_RenderPass;
    Vector<VkRenderPassBeginInfo>   m_RenderPassInfos;
  };

  struct DynamicRenderingInfo
  {
    Array<Vector<VkRenderingAttachmentInfoKHR>, MAX_FRAMES_IN_FLIGHT>  m_ColourAttachmentInfos;
    Array<Vector<VkRenderingAttachmentInfoKHR>, MAX_FRAMES_IN_FLIGHT>  m_DepthAttachmentInfos;
    Array<Vector<VkRenderingAttachmentInfoKHR>, MAX_FRAMES_IN_FLIGHT>  m_ResolveAttachmentInfos;

    Vector<VkRenderingInfoKHR>            m_RenderingInfos;
  };

  enum class ShaderStageType { Vertex, Fragment, Compute };

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

  struct VkPipelineData
  {
    VkPipelineData(VkPipeline pipeline, VkPipelineLayout layout)
    {
      m_Pipeline = pipeline;
      m_PipelineLayout = layout;
    }

    VkPipelineData() = default;

    void Free(VkState & vk) const;

    VkPipeline          m_Pipeline = VK_NULL_HANDLE;
    VkPipelineLayout    m_PipelineLayout = VK_NULL_HANDLE;
  };

  class VkBackend
  {
  public:
    virtual Vector<const char*>         GetRequiredInstanceExtensions(VkState& vk) = 0;
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
    VkCommandPool                   m_GraphicsComputeQueueCommandPool;
    VmaAllocator                    m_Allocator;
    DescriptorSetAllocator          m_DescriptorSetAllocator;

    Vector<VkSemaphore>             m_ImageAvailableSemaphores;
    Vector<VkSemaphore>             m_RenderFinishedSemaphores;
    Vector<VkSemaphore>             m_ComputeFinishedSemaphores;
    Vector<VkFence>                 m_FrameInFlightFences;
    Vector<VkFence>                 m_ImagesInFlightFences;
    Vector<VkFence>                 m_ComputeInFlightFences;
    QueueFamilyIndices              m_QueueFamilyIndices;

    VkQueue                         m_GraphicsQueue = VK_NULL_HANDLE;
    VkQueue                         m_ComputeQueue = VK_NULL_HANDLE;
    VkQueue                         m_PresentQueue = VK_NULL_HANDLE;

    VulkanAPIWindowHandle*          m_WindowHandle;

    Vector<VkImage>                 m_SwapChainImages;
    Vector<VkImageView>             m_SwapChainImageViews;
    Vector<VkFramebuffer>           m_SwapChainFramebuffers;
    Vector<VkCommandBuffer>         m_GraphicsCommandBuffers;
    Vector<VkCommandBuffer>         m_ComputeCommandBuffers;

    VkFormat                        m_SwapChainImageFormat;
    VkExtent2D                      m_SwapChainImageExtent;

    VkImage                         m_SwapChainColourImage;
    VkDeviceMemory                  m_SwapChainColourImageMemory;
    VkImageView                     m_SwapChainColourImageView;

    VkImage                         m_SwapChainDepthImage;
    VkDeviceMemory                  m_SwapChainDepthImageMemory;
    VkImageView                     m_SwapChainDepthImageView;

    VkSampleCountFlagBits           m_MaxMsaaSamples;
    Vector<const char*>             m_DesiredDeviceExtensions;

    double                          m_DeltaTime;
    bool                            m_ShouldRun = true;
    bool                            m_RunComputeCommands = false;
    bool                            m_UseSwapchainMsaa = false;
    bool                            m_WaitForVerticalSync = false;
    bool                            m_UseDynamicRendering = false;
    const bool                      m_UseValidation = true;
    const bool                      m_UseImGui      = true;
    uint64_t                        m_LastFrameTime;
    int                             m_CurrentFrameIndex;
    VkExtent2D                      m_MaxFramebufferExtent;
    String                          m_AppName;
  };

  struct VkViewportData
  {
    VkViewport                          m_Viewport;
    VkRect2D                            m_Scissor;
    VkPipelineViewportStateCreateInfo   m_CreateInfo;
  };

}

template <>
struct std::hash<lvk::DescriptorSetBinding>
{
    std::size_t operator()(const lvk::DescriptorSetBinding& sb) const
    {
        using std::hash;

        return ((hash<uint64_t>()(sb.m_Data)));

            // TODO: This size should account for alignment diffs between GPU & CPU
            // ^ (hash<uint64_t>()(sb.m_BindingSize))));
    }
};
