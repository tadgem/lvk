#pragma once
#include "lvk/Structs.h"
namespace lvk::utils {
  uint32_t                            FindMemoryType(VkState& vk,uint32_t typeFilter, VkMemoryPropertyFlags properties);
  VkFormat                            FindSupportedFormat(VkState& vk, const Vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
  VkFormat                            FindDepthFormat(VkState& vk);
  bool                                HasStencilComponent(VkFormat& format);
  StageBinary                         LoadSpirvBinary(const String& path);
}