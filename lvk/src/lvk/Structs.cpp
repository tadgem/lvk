#include "lvk/Structs.h"
#include "lvk/Macros.h"
#include "lvk/Log.h"

bool lvk::QueueFamilyIndices::IsComplete() {
  bool foundGraphicsQueue = m_QueueFamilies.find(QueueFamilyType::GraphicsAndCompute) != m_QueueFamilies.end();
  bool foundPresentQueue  = m_QueueFamilies.find(QueueFamilyType::Present) != m_QueueFamilies.end();
  return foundGraphicsQueue && foundPresentQueue;
}
void lvk::MappedBuffer::Free(lvk::VkState &vk) {
  vmaUnmapMemory(vk.m_Allocator, m_GpuMemory);
  Buffer::Free(vk);
}

void lvk::ShaderBufferFrameData::Free(lvk::VkState& vk) {
    if (!Ready())
    {
        return;
    }
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        m_UniformBuffers[i]->Free(vk);
    }
}

void lvk::VkPipelineData::Free(lvk::VkState &vk) const
{
  vkDestroyPipelineLayout (vk.m_LogicalDevice, m_PipelineLayout, nullptr);
  vkDestroyPipeline(vk.m_LogicalDevice, m_Pipeline, nullptr);
}
void lvk::Buffer::Free(lvk::VkState &vk) {
  vkDestroyBuffer(vk.m_LogicalDevice, m_GpuBuffer, nullptr);
  vmaFreeMemory(vk.m_Allocator, m_GpuMemory);
}

lvk::Buffer::Buffer(const BufferStorageType& bufferType, VkBuffer buf, VmaAllocation alloc, VkDeviceSize size)
    : m_Type(bufferType), m_GpuBuffer(buf), m_GpuMemory(alloc), m_Size(size)
{}

void lvk::MappedBuffer::Map(VkState& vk)
{
    vmaMapMemory(vk.m_Allocator, m_GpuMemory, &m_MappedAddr);
}

lvk::MappedBuffer::MappedBuffer(Buffer& buf)
    : Buffer(Buffer::BufferStorageType::Mapped, buf.m_GpuBuffer, buf.m_GpuMemory, buf.m_Size), m_MappedAddr(nullptr)
{
}

bool lvk::ShaderBufferFrameData::CanSet(uint32_t frameIndex)
{
    if (!Ready())
    {
        LVK_LOG_ERR("Buffer has not beed set or allocated");
        return false;
    }
    if (m_UniformBuffers[frameIndex]->m_Type != Buffer::BufferStorageType::Mapped)
    {
        LVK_LOG_ERR("Attempting to set data for non mapped buffer");
        return false;
    }
    return true;
}

bool lvk::ShaderBufferFrameData::Ready()
{
    bool ready = true;
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        if (m_UniformBuffers[i].get() == nullptr)
        {
            ready = false;
            break;
        }
    }
    return ready;
}
