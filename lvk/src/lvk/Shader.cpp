#include "lvk/Shader.h"
#include "lvk/Macros.h"
#include "spdlog/spdlog.h"
#include "volk.h"
#include "shaderc/shaderc.h"

namespace lvk {
void ShaderProgram::Free(VkState &vk) {
  vkDestroyDescriptorSetLayout(vk.m_LogicalDevice, m_DescriptorSetLayout,
                               nullptr);
}

ShaderProgram ShaderProgram::CreateCompute(VkState &vk, ShaderStage &compute) {
  VkDescriptorSetLayout layout;

  std::vector<VkDescriptorSetLayoutBinding> bindings;
  for (auto &layout : compute.m_LayoutDatas) {
    for (auto &binding : layout.m_Bindings) {
      bindings.push_back(binding);
    }
  }
  VkDescriptorSetLayoutCreateInfo layoutInfo{};
  layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
  layoutInfo.pBindings = bindings.data();

  VK_CHECK(vkCreateDescriptorSetLayout(vk.m_LogicalDevice, &layoutInfo, nullptr,
                                       &layout))

  return {Vector<ShaderStage>{compute}, layout};
}

VkShaderModule CreateShaderModule(VkState &vk, const StageBinary &data) {
  VkShaderModuleCreateInfo createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  createInfo.codeSize = static_cast<uint32_t>(data.size());
  createInfo.pCode = reinterpret_cast<const uint32_t *>(data.data());

  VkShaderModule shaderModule;
  if (vkCreateShaderModule(vk.m_LogicalDevice, &createInfo, nullptr,
                           &shaderModule) != VK_SUCCESS) {
    spdlog::error("Failed to create shader module!");
    std::cerr << "Failed to create shader module" << std::endl;
  }
  return shaderModule;
}


VkShaderModule CreateShaderModuleRaw(VkState &vk, const char *data,
                                     size_t length) {
  VkShaderModuleCreateInfo createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  createInfo.codeSize = static_cast<uint32_t>(length);
  createInfo.pCode = reinterpret_cast<const uint32_t *>(data);

  VkShaderModule shaderModule;
  if (vkCreateShaderModule(vk.m_LogicalDevice, &createInfo, nullptr,
                           &shaderModule) != VK_SUCCESS) {
    spdlog::error("Failed to create shader module!");
    std::cerr << "Failed to create shader module" << std::endl;
  }
  return shaderModule;
}

shaderc_shader_kind get_shaderc_type_from_lvk(lvk::ShaderStageType type)
{
  switch(type)
  {
    case lvk::ShaderStageType::Vertex:
      return shaderc_shader_kind ::shaderc_glsl_vertex_shader;
    case lvk::ShaderStageType::Fragment:
      return shaderc_shader_kind ::shaderc_glsl_fragment_shader;
    case lvk::ShaderStageType::Compute:
      return shaderc_shader_kind ::shaderc_glsl_compute_shader;
    default:
      return shaderc_shader_kind ::shaderc_compute_shader;
  }
}

StageBinary CreateStageBinaryFromSource(VkState &vk, ShaderStageType type,
                                    const std::string &source) {
  shaderc_compiler* c = shaderc_compiler_initialize();
  shaderc_compile_options_t opt {};

  auto result = shaderc_compile_into_spv(c,
                          source.c_str(),
                          source.size(),
                          get_shaderc_type_from_lvk(type),
                          "shadername",
                          "main",
                           opt);

  const char* spirv_bytes = shaderc_result_get_bytes(result);
  size_t      spirv_size  = shaderc_result_get_length(result);

  StageBinary bin {};
  bin.resize(spirv_size);
  for(auto i = 0; i < spirv_size; i++)
  {
      bin[i] = spirv_bytes[i];
  }

  shaderc_result_release(result);
  shaderc_compiler_release(c);

  return bin;
}

}
