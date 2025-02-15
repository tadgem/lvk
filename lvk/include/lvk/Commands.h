#pragma once
#include "lvk/Structs.h"
namespace lvk {
namespace commands {
  VkCommandBuffer BeginSingleTimeCommands(VkState &vk);
  void EndSingleTimeCommands(VkState &vk, VkCommandBuffer &commandBuffer);
  VkCommandBuffer &BeginGraphicsCommands(VkState &vk, uint32_t frameIndex);
  void EndGraphicsCommands(VkState &vk, uint32_t frameIndex);
  void RecordGraphicsCommands(
      VkState &vk,
      std::function<void(VkCommandBuffer &, uint32_t)> graphicsCommandsCallback);
}
} // namespace turas