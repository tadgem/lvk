#pragma once
#include "lvk/Structs.h"
#include "lvk/Macros.h"
#include "spdlog/spdlog.h"

namespace lvk {
namespace buffers {
Buffer CreateBuffer(VkState &vk, VkDeviceSize size, VkBufferUsageFlags usage,
                     VkMemoryPropertyFlags properties);

void CopyBuffer(VkState &vk, VkBuffer &src, VkBuffer &dst, VkDeviceSize size);

Buffer CreateIndexBuffer(VkState &vk, Vector<uint32_t> indices);

template <typename _Ty>
Vector<MappedBuffer> CreateUniformBuffers(VkState &vk) {
  VkDeviceSize bufferSize = sizeof(_Ty);
  Vector<MappedBuffer> uniformBuffers{};
  uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);

  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    uniformBuffers[i] = CreateMappedBuffer(vk, bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
  }
  return uniformBuffers;
}

MappedBuffer CreateMappedBuffer(VkState &vk, VkDeviceSize size, VkBufferUsageFlags bufferUsage,
                        VkMemoryPropertyFlags memoryProperties);

void CreateUniformBuffers(VkState &vk, ShaderBufferFrameData &uniformData,
                          VkDeviceSize bufferSize);

template <typename _Ty>
void CreateUniformBuffers(VkState &vk, ShaderBufferFrameData &uniformData) {
  constexpr VkDeviceSize bufferSize = sizeof(_Ty);
  CreateUniformBuffers(vk, uniformData, bufferSize);
}

template <typename _Ty>
Buffer CreateVertexBuffer(VkState &vk, Vector<_Ty> verts) {
  VkDeviceSize bufferSize = sizeof(_Ty) * verts.size();

  // create a CPU side buffer to dump vertex data into
  
  MappedBuffer stagingBuffer = CreateMappedBuffer(vk, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                      VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

  // dump vert data
  memcpy(stagingBuffer.m_MappedAddr, verts.data(), bufferSize);

  // create GPU side buffer
  Buffer vb = CreateBuffer(vk, bufferSize,
                  VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
                      VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                  VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  CopyBuffer(vk, stagingBuffer.m_GpuBuffer, vb.m_GpuBuffer, bufferSize);

  stagingBuffer.Free(vk);
  return vb;
}
}
}