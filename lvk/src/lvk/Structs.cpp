#include "lvk/Structs.h"
#include "lvk/Macros.h"
bool lvk::QueueFamilyIndices::IsComplete() {
  bool foundGraphicsQueue = m_QueueFamilies.find(QueueFamilyType::GraphicsAndCompute) != m_QueueFamilies.end();
  bool foundPresentQueue  = m_QueueFamilies.find(QueueFamilyType::Present) != m_QueueFamilies.end();
  return foundGraphicsQueue && foundPresentQueue;
}
void lvk::MappedBuffer::Free(lvk::VkState &vk) {
  vmaUnmapMemory(vk.m_Allocator, m_GpuMemory);
  Buffer::Free(vk);
}

void lvk::ShaderBufferFrameData::Free(lvk::VkState &vk) {
  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    m_UniformBuffers[i]->Free(vk);
  }

  m_UniformBuffers.clear();
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

lvk::Buffer::Buffer(const BufferType& bufferType, VkBuffer buf, VmaAllocation alloc, VkDeviceSize size)
    : m_Type(bufferType), m_GpuBuffer(buf), m_GpuMemory(alloc), m_Size(size)
{}

void lvk::MappedBuffer::Map(VkState& vk)
{
    vmaMapMemory(vk.m_Allocator, m_GpuMemory, &m_MappedAddr);
}

lvk::MappedBuffer::MappedBuffer(Buffer& buf)
    : Buffer(Buffer::BufferType::Mapped, buf.m_GpuBuffer, buf.m_GpuMemory, buf.m_Size), m_MappedAddr(nullptr)
{
}