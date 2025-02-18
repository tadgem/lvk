#include "lvk/Buffer.h"
#include "lvk/Macros.h"
#include "lvk/Commands.h"
#include "spdlog/spdlog.h"
namespace lvk
{
namespace buffers {
void CreateBuffer(VkState& vk, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VmaAllocation& allocation)
{
  VkBufferCreateInfo bufferInfo{};
  bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferInfo.size = size;
  bufferInfo.usage = usage;
  bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  VmaAllocationCreateInfo allocInfo = {};
  allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
  allocInfo.requiredFlags = properties;

  if (properties && VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
  {
    // if we access members of the array non sequentially,
    // we may need to request random access instead of sequential
    // VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT
    allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
  }

  VK_CHECK(vmaCreateBuffer(vk.m_Allocator, &bufferInfo, &allocInfo, &buffer, &allocation, nullptr));
}

void CopyBuffer(VkState& vk, VkBuffer& src, VkBuffer& dst, VkDeviceSize size)
{
  // create a new command buffer to record the buffer copy
  VkCommandBuffer commandBuffer = commands::BeginSingleTimeCommands(vk);

  // record copy command
  VkBufferCopy copyRegion{};
  copyRegion.srcOffset = 0; // Optional
  copyRegion.dstOffset = 0; // Optional
  copyRegion.size = size;
  vkCmdCopyBuffer(commandBuffer, src, dst, 1, &copyRegion);
  commands::EndSingleTimeCommands(vk, commandBuffer);
}

void CreateIndexBuffer(VkState& vk, std::vector<uint32_t> indices, VkBuffer& buffer, VmaAllocation& deviceMemory)
{
  VkDeviceSize bufferSize = sizeof(uint32_t) * indices.size();

  // create a CPU side buffer to dump vertex data into
  VkBuffer stagingBuffer;
  VmaAllocation stagingBufferMemory;
  CreateBuffer(vk, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

  // dump vert data
  void* data;
  vmaMapMemory(vk.m_Allocator, stagingBufferMemory, &data);
  memcpy(data, indices.data(), bufferSize);
  vmaUnmapMemory(vk.m_Allocator, stagingBufferMemory);

  // create GPU side buffer
  CreateBuffer(vk, bufferSize,
                  VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                  VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                  buffer, deviceMemory);

  CopyBuffer(vk, stagingBuffer, buffer, bufferSize);

  vkDestroyBuffer(vk.m_LogicalDevice, stagingBuffer, nullptr);
  vmaFreeMemory(vk.m_Allocator, stagingBufferMemory);
}

void CreateMappedBuffer(VkState& vk, MappedBuffer& buf, VkBufferUsageFlags bufferUsage, VkMemoryPropertyFlags memoryProperties, uint32_t size)
{
  CreateBuffer(vk,VkDeviceSize{ size }, bufferUsage, memoryProperties, buf.m_GpuBuffer, buf.m_GpuMemory);
  VK_CHECK(vmaMapMemory(vk.m_Allocator, buf.m_GpuMemory, &buf.m_MappedAddr));
}

void CreateUniformBuffers (VkState& vk, ShaderBufferFrameData& uniformData, VkDeviceSize bufferSize)
{
  uniformData.m_UniformBuffers.resize (MAX_FRAMES_IN_FLIGHT);

  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    CreateMappedBuffer (vk, uniformData.m_UniformBuffers[i], VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                       static_cast<uint32_t> (bufferSize));
  }
}
}
}

