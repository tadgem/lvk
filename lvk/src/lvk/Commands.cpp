#include "lvk/Commands.h"
#include "lvk/Macros.h"
#include "spdlog/spdlog.h"

VkCommandBuffer lvk::BeginSingleTimeCommands(VkState& vk)
{
  VkCommandBufferAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandPool = vk.m_GraphicsQueueCommandPool;
  allocInfo.commandBufferCount = 1;

  VkCommandBuffer commandBuffer;
  vkAllocateCommandBuffers(vk.m_LogicalDevice, &allocInfo, &commandBuffer);

  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  vkBeginCommandBuffer(commandBuffer, &beginInfo);

  return commandBuffer;
}

void lvk::EndSingleTimeCommands(VkState& vk, VkCommandBuffer& commandBuffer)
{
  vkEndCommandBuffer(commandBuffer);

  VkSubmitInfo submitInfo{};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &commandBuffer;

  vkQueueSubmit(vk.m_GraphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
  vkQueueWaitIdle(vk.m_GraphicsQueue);

  vkFreeCommandBuffers(vk.m_LogicalDevice , vk.m_GraphicsQueueCommandPool, 1, &commandBuffer);
}

VkCommandBuffer &lvk::BeginGraphicsCommands(VkState& vk, uint32_t frameIndex) {
  VkCommandBufferBeginInfo commandBufferBeginInfo{};
  commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  commandBufferBeginInfo.flags = 0;
  commandBufferBeginInfo.pInheritanceInfo = nullptr;

  VK_CHECK(vkBeginCommandBuffer(vk.m_CommandBuffers[frameIndex], &commandBufferBeginInfo));
  return vk.m_CommandBuffers[frameIndex];

}
void lvk::EndGraphicsCommands(lvk::VkState &vk, uint32_t frameIndex) {
  VK_CHECK(vkEndCommandBuffer(vk.m_CommandBuffers[frameIndex]));
}
void lvk::RecordGraphicsCommands(
    lvk::VkState &vk,
    std::function<void(VkCommandBuffer &, uint32_t)> graphicsCommandsCallback) {
  for (uint32_t i = 0; i < vk.m_CommandBuffers.size(); i++) {
    VkCommandBufferBeginInfo commandBufferBeginInfo{};
    commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    commandBufferBeginInfo.flags = 0;
    commandBufferBeginInfo.pInheritanceInfo = nullptr;

    VK_CHECK(
        vkBeginCommandBuffer(vk.m_CommandBuffers[i], &commandBufferBeginInfo))

    // Callback
    graphicsCommandsCallback(vk.m_CommandBuffers[i], i);

    VK_CHECK(vkEndCommandBuffer(vk.m_CommandBuffers[i]));
  }
}