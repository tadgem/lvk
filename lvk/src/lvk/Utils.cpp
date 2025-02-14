#include "lvk/Utils.h"
#include "spdlog/spdlog.h"
#include <fstream>

uint32_t lvk::FindMemoryType(VkState& vk, uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
  VkPhysicalDeviceMemoryProperties memProperties;
  vkGetPhysicalDeviceMemoryProperties(vk.m_PhysicalDevice, &memProperties);
  for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
    if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
      return i;
    }
  }
  return UINT32_MAX;
}

VkFormat lvk::FindSupportedFormat(VkState& vk, const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features)
{
  for (VkFormat format : candidates) {
    VkFormatProperties props;
    vkGetPhysicalDeviceFormatProperties(vk.m_PhysicalDevice, format, &props);
    if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
      return format;
    }
    else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
      return format;
    }
  }
  spdlog::error("Failed to find appropriate supported format from candidates");
  return VkFormat{};

}

VkFormat lvk::FindDepthFormat(VkState& vk)
{
  return FindSupportedFormat(vk,
      { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
      VK_IMAGE_TILING_OPTIMAL,
      VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
  );
}
bool lvk::HasStencilComponent(VkFormat &format) {
  return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}


lvk::StageBinary lvk::LoadSpirvBinary(const String& path)
{
  std::ifstream file(path, std::ios::ate | std::ios::binary);

  if (!file.is_open())
  {
    spdlog::error("Failed to open file at path {} as binary!", path);
    std::cerr << "Failed to open file!" << std::endl;
    return StageBinary();
  }

  size_t fileSize = static_cast<size_t>(file.tellg());
  StageBinary data(fileSize);

  file.seekg(0);

  file.read((char*) data.data(), fileSize);

  file.close();
  return data;
}