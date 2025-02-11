#pragma once
#define VK_NO_PROTOTYPES
#include "vulkan/vulkan.h"
#include "ThirdParty/VulkanMemoryAllocator.h"

#include "Alias.h"


namespace lvk {

  class VulkanAPI;

  enum class ShaderBindingType
  {
    UniformBuffer,
    ShaderStorageBuffer,
    PushConstants,
    Sampler
  };

  enum class ShaderBufferMemberType
  {
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

  struct PushConstantBlock
  {
    uint32_t                m_Size;
    uint32_t                m_Offset;
    String                  m_Name;
    VkShaderStageFlags      m_Stage;
  };

  struct ShaderBufferMember
  {
    uint32_t                m_Size;
    uint32_t                m_Offset;
    uint32_t                m_Stride;
    String                  m_Name;
    ShaderBufferMemberType  m_Type;
  };

  struct DescriptorSetLayoutBindingData
  {
    String                      m_BindingName;
    uint32_t                    m_BindingIndex;
    uint32_t                    m_ExpectedBufferSize;
    ShaderBindingType           m_BufferType;
    Vector<ShaderBufferMember>  m_Members;
  };

  struct DescriptorSetLayoutData
  {
    uint32_t                                    m_SetNumber;
    VkDescriptorSetLayoutCreateInfo             m_CreateInfo;
    VkDescriptorSetLayout                       m_Layout;
    Vector<VkDescriptorSetLayoutBinding>        m_Bindings;
    Vector<DescriptorSetLayoutBindingData>      m_BindingDatas;
  };

  // reuse this for generic cpu dynamic buffer
  struct MappedBuffer
  {
    VkBuffer        m_GpuBuffer;
    VmaAllocation   m_GpuMemory;
    void*           m_MappedAddr;

    void Free(VulkanAPI& vk);
  };

  struct ShaderBufferFrameData
  {
    Vector<MappedBuffer>            m_UniformBuffers;

    template<typename _Ty>
    void Set(uint32_t frameIndex, const _Ty& data, uint32_t offset = 0)
    {
      constexpr size_t _ty_size = sizeof(_Ty);
      uint64_t base_addr = (uint64_t)m_UniformBuffers[frameIndex].m_MappedAddr;
      void* addr = (void*)(base_addr + static_cast<uint64_t>(offset));
      memcpy(addr, &data, _ty_size);
    }

    template<typename _Ty>
    void SetMemory(uint32_t frameIndex, const _Ty* start, uint64_t count)
    {
      constexpr size_t _ty_size = sizeof(_Ty);
      void* addr = m_UniformBuffers[frameIndex].m_MappedAddr;
      memcpy(addr, start, count);

    }

    void Free(VulkanAPI& vk);
  };


  enum class ShaderStageType
  {
    Vertex,
    Fragment
  };

  enum QueueFamilyType
  {
    GraphicsAndCompute = VK_QUEUE_GRAPHICS_BIT,
    Transfer = VK_QUEUE_TRANSFER_BIT,
    Present = 8

  };

  struct QueueFamilyIndices
  {
    HashMap<QueueFamilyType, uint32_t> m_QueueFamilies;

    bool IsComplete();

  };

  struct SwapChainSupportDetais
  {
    VkSurfaceCapabilitiesKHR   m_Capabilities;
    Vector<VkSurfaceFormatKHR> m_SupportedFormats;
    Vector<VkPresentModeKHR>   m_SupportedPresentModes;
  };

}