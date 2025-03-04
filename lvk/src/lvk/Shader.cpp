#include "lvk/Shader.h"
#include "lvk/Macros.h"
#include "spdlog/spdlog.h"
#include "volk.h"
#include "shaderc/shaderc.h"
#include <filesystem>
#include <sstream>
#include <regex>

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
ShaderProgram::ShaderProgram(Vector<ShaderStage> shaderStages,
                             VkDescriptorSetLayout layout) :
 m_DescriptorSetLayout(layout), m_Stages(std::move(shaderStages))
{
  BuildPushConstantRanges();
}
void ShaderProgram::BuildPushConstantRanges() {
  // update
  // valid combos:
  // 1 stage has 1 push constant block
  // both stages share the same push constant block
  // each stage has a separate push constant block
  // e.g. only ever 1 block per stage
  for (auto &stage : m_Stages) {
    if (stage.m_PushConstants.empty()) {
      continue;
    }

    if (stage.m_PushConstants.size() > 1) {
      spdlog::error(
          "VulkanAPI : CreateRasterizationPipeline : Supplied stage has more than 1 push constant block, this is not allowed.");
      continue;
    }

    PushConstantBlock &block = stage.m_PushConstants[0];

    bool skip = false;

    for (auto &range : m_PushConstantRanges) {
      if (block.m_Offset == range.offset && block.m_Size == range.size) {
        range.stageFlags |= block.m_Stage;
      }
    }

    if (skip) {
      continue;
    }

    VkPushConstantRange range{};
    range.offset = block.m_Offset;
    range.size = block.m_Size;
    range.stageFlags = block.m_Stage;

    m_PushConstantRanges.push_back(range);
  }
}
uint32_t ShaderProgram::GetPushConstantRangeCount() {
  return static_cast<uint32_t>(m_PushConstantRanges.size());
}
VkPipelineLayoutCreateInfo ShaderProgram::GetPipelineLayoutCreateInfo() {
  VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
  pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipelineLayoutInfo.setLayoutCount = 1;
  pipelineLayoutInfo.pSetLayouts = &m_DescriptorSetLayout;
  pipelineLayoutInfo.pushConstantRangeCount = GetPushConstantRangeCount();
  pipelineLayoutInfo.pPushConstantRanges = !m_PushConstantRanges.empty() ?
                                            m_PushConstantRanges.data() :
                                            nullptr;

  return pipelineLayoutInfo;
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

shaderc_shader_kind GetShadercShaderKind(lvk::ShaderStageType type)
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
                                    const std::string &source, const std::string& shaderName) {
  shaderc_compiler* c = shaderc_compiler_initialize();
  shaderc_compile_options_t opt {};

  auto result = shaderc_compile_into_spv(c,
                          source.c_str(),
                          source.size(),
                          GetShadercShaderKind(type),
                          shaderName.c_str(),
                          "main",
                           opt);

  const char* spirv_bytes = shaderc_result_get_bytes(result);
  size_t      spirv_size  = shaderc_result_get_length(result);
  StageBinary bin {};
  if(shaderc_result_get_num_errors(result) != 0)
  {
      spdlog::error("Failed to compile shader : {}",
                    shaderc_result_get_error_message(result));
      return bin;
  }

  bin.resize(spirv_size);
  for(auto i = 0; i < spirv_size; i++)
  {
      bin[i] = spirv_bytes[i];
  }

  shaderc_result_release(result);
  shaderc_compiler_release(c);

  return bin;
}

void RecurseStringInclude(String inputDir, String& output, const String& path)
{
  String input = utils::LoadStringFromPath(inputDir + "/" + path);
  String dir = std::filesystem::path(path).parent_path().u8string();
  std::istringstream iss(input);
  std::regex include_dir_regex("\\\"(.*)\\\"");
  for (std::string line; std::getline(iss, line); )
  {
      if(line.find("#include") != String::npos)
      {
        auto words_begin =
            std::sregex_iterator(line.begin(), line.end(), include_dir_regex);
        auto words_end = std::sregex_iterator();
        for (std::sregex_iterator i = words_begin; i != words_end; ++i)
        {
          const std::smatch& match = *i;
          // remove first and last ""
          std::string include_dir = match.str().substr(1, match.str().size() - 2);
          RecurseStringInclude(inputDir, output, include_dir);
        }

        RecurseStringInclude(dir, output, input);
      }
      else
      {
        output += line + "\n";
      }
  }
}

String ShaderStage::LoadShaderSource(const String &path) {
  String final_shader_src {};
  std::filesystem::path inputPath(path);
  RecurseStringInclude(inputPath.parent_path().u8string(),
                       final_shader_src, inputPath.filename().u8string());
  // do includes
  return final_shader_src;
}
}
