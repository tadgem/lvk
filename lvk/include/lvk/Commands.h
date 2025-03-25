#pragma once
#include "lvk/Structs.h"
namespace lvk {
namespace commands {
  VkCommandBuffer BeginSingleTimeCommands(VkState &vk);

  void EndSingleTimeCommands(VkState &vk, VkCommandBuffer &commandBuffer);

  void RecordGraphicsCommands(
      VkState &vk,
      LVK_STL::function<void(VkCommandBuffer &, uint32_t)> graphicsCommandsCallback);

  void RecordComputeCommands(
      VkState &vk,
      LVK_STL::function<void(VkCommandBuffer &, uint32_t)> computeCommandsCallback);
}
} // namespace turas