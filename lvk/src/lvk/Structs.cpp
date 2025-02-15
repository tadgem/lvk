#include "lvk/Structs.h"
#include "lvk/Macros.h"
bool lvk::QueueFamilyIndices::IsComplete() {
  bool foundGraphicsQueue = m_QueueFamilies.find(QueueFamilyType::GraphicsAndCompute) != m_QueueFamilies.end();
  bool foundPresentQueue  = m_QueueFamilies.find(QueueFamilyType::Present) != m_QueueFamilies.end();
  return foundGraphicsQueue && foundPresentQueue;
}
void lvk::MappedBuffer::Free(lvk::VkState &vk) {
  vmaUnmapMemory(vk.m_Allocator, m_GpuMemory);
  vkDestroyBuffer(vk.m_LogicalDevice, m_GpuBuffer, nullptr);
  vmaFreeMemory(vk.m_Allocator, m_GpuMemory);
}
void lvk::ShaderBufferFrameData::Free(lvk::VkState &vk) {
  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    m_UniformBuffers[i].Free(vk);
  }

  m_UniformBuffers.clear();
}
