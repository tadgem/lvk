#pragma once
#include "lvk/Structs.h"
#include "lvk/Macros.h"
#include "spdlog/spdlog.h"

namespace lvk {
namespace buffers {
void CreateBuffer(VkState &vk, VkDeviceSize size, VkBufferUsageFlags usage,
                     VkMemoryPropertyFlags properties, VkBuffer &buffer,
                     VmaAllocation &allocation);
void CopyBuffer(VkState &vk, VkBuffer &src, VkBuffer &dst, VkDeviceSize size);
void CreateIndexBuffer(VkState &vk, Vector<uint32_t> indices, VkBuffer &buffer,
                       VmaAllocation &deviceMemory);

template <typename _Ty>
void CreateUniformBuffers(VkState &vk, Vector<VkBuffer> &uniformBuffersFrames,
                          Vector<VmaAllocation> &uniformBuffersMemoryFrames,
                          Vector<void *> &uniformBufferMappedMemoryFrames) {
  VkDeviceSize bufferSize = sizeof(_Ty);

  uniformBuffersFrames.resize(MAX_FRAMES_IN_FLIGHT);
  uniformBuffersMemoryFrames.resize(MAX_FRAMES_IN_FLIGHT);
  uniformBufferMappedMemoryFrames.resize(MAX_FRAMES_IN_FLIGHT);

  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    CreateBuffer(vk, bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                    uniformBuffersFrames[i], uniformBuffersMemoryFrames[i]);

    VK_CHECK(vmaMapMemory(vk.m_Allocator, uniformBuffersMemoryFrames[i],
                          &uniformBufferMappedMemoryFrames[i]))
  }
}

void CreateMappedBuffer(VkState &vk, MappedBuffer &buf,
                        VkBufferUsageFlags bufferUsage,
                        VkMemoryPropertyFlags memoryProperties, uint32_t size);
void CreateUniformBuffers(VkState &vk, ShaderBufferFrameData &uniformData,
                          VkDeviceSize bufferSize);
template <typename _Ty>
void CreateUniformBuffers(VkState &vk, ShaderBufferFrameData &uniformData) {
  constexpr VkDeviceSize bufferSize = sizeof(_Ty);
  CreateUniformBuffers(vk, uniformData, bufferSize);
}

template <typename _Ty>
void CreateVertexBuffer(VkState &vk, Vector<_Ty> verts, VkBuffer &buffer,
                        VmaAllocation &deviceMemory) {
  VkDeviceSize bufferSize = sizeof(_Ty) * verts.size();

  // create a CPU side buffer to dump vertex data into
  VkBuffer stagingBuffer;
  VmaAllocation stagingBufferMemory;
  CreateBuffer(vk, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                      VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                  stagingBuffer, stagingBufferMemory);

  // dump vert data
  void *data;
  vmaMapMemory(vk.m_Allocator, stagingBufferMemory, &data);
  memcpy(data, verts.data(), bufferSize);
  vmaUnmapMemory(vk.m_Allocator, stagingBufferMemory);

  // create GPU side buffer
  CreateBuffer(vk, bufferSize,
                  VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
                      VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                  VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, buffer, deviceMemory);

  CopyBuffer(vk, stagingBuffer, buffer, bufferSize);

  vkDestroyBuffer(vk.m_LogicalDevice, stagingBuffer, nullptr);
  vmaFreeMemory(vk.m_Allocator, stagingBufferMemory);
}
}
}