#include "lvk/Utils.h"
#include "lvk/Log.h"
#include <fstream>
#include <sstream>

uint32_t lvk::utils::FindMemoryType(VkState& vk, uint32_t typeFilter, VkMemoryPropertyFlags properties)
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

VkFormat lvk::utils::FindSupportedFormat(VkState& vk, const Vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features)
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
  LVK_LOG_ERR("Failed to find appropriate supported format from candidates");
  return VkFormat{};

}

VkFormat lvk::utils::FindDepthFormat(VkState& vk)
{
    STLAllocator<VkFormat> alloc(*vk.m_CPUAllocator);
    Vector<VkFormat> formats(alloc);
    formats.push_back(VK_FORMAT_D32_SFLOAT);
    formats.push_back(VK_FORMAT_D32_SFLOAT_S8_UINT);
    formats.push_back(VK_FORMAT_D24_UNORM_S8_UINT);
    return FindSupportedFormat(vk,
      formats,
      VK_IMAGE_TILING_OPTIMAL,
      VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
  );
}
bool lvk::utils::HasStencilComponent(VkFormat &format) {
  return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}


lvk::StageBinary lvk::utils::LoadSpirvBinary(VkState& vk, const String& path)
{
  STLAllocator<unsigned char> alloc(*vk.m_CPUAllocator);
  std::ifstream file(path.c_str(), std::ios::ate | std::ios::binary);

  if (!file.is_open())
  {
    LVK_LOG_ERR("Failed to open file at path {} as binary!", path);
    std::cerr << "Failed to open file!" << std::endl;
    return StageBinary(alloc);
  }

  size_t fileSize = static_cast<size_t>(file.tellg());
  StageBinary data(alloc);
  data.resize(fileSize);

  file.seekg(0);

  file.read((char*) data.data(), fileSize);

  file.close();
  return data;
}

lvk::String lvk::utils::LoadStringFromPath(VkState& vk, const lvk::String &path) {
  std::ifstream in(path.c_str());
  std::stringstream stream;
  if (!in.is_open()) {
    return String(*vk.m_CPUAllocator);
  }

  stream << in.rdbuf();
  String str(*vk.m_CPUAllocator);
  str = stream.str();
  return str;
}
