#pragma once
#include "lvk/Structs.h"
namespace lvk {
  VkCommandBuffer                     BeginSingleTimeCommands(VkState& vk);
  void                                EndSingleTimeCommands(VkState& vk, VkCommandBuffer& commandBuffer);
} // namespace turas