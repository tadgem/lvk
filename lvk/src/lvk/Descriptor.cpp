#include "lvk/Descriptor.h"
#include "lvk/Macros.h"
#include "spdlog/spdlog.h"

namespace lvk
{
namespace descriptor{

std::vector<VkDescriptorSetLayoutBinding> CleanDescriptorSetLayout(std::vector<VkDescriptorSetLayoutBinding>& arr)
{
  std::vector<VkDescriptorSetLayoutBinding> clean;

  for (int k = 0; k < arr.size(); k++)
  {
    auto& layoutSetData = arr[k];
    if (clean.empty())
    {
      clean.push_back(layoutSetData);
      continue;
    }

    int start = static_cast<int>(clean.size() - 1);
    for (int i = start; i >= 0; i--)
    {
      auto& newLayoutSet = clean[i];
      if (newLayoutSet.binding == layoutSetData.binding)
      {
        if (newLayoutSet.descriptorCount == layoutSetData.descriptorCount &&
            newLayoutSet.descriptorType == layoutSetData.descriptorType)
        {
          newLayoutSet.stageFlags = layoutSetData.stageFlags + newLayoutSet.stageFlags;
          continue;
        }
      }

      clean.push_back(layoutSetData);
      break;

    }
  }

  return clean;
}

ShaderBindingType GetBindingType(const SpvReflectDescriptorBinding& binding)
{
  if (binding.descriptor_type == SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
  {
    return ShaderBindingType::UniformBuffer;
  }
  if (binding.descriptor_type == SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER)
  {
    return ShaderBindingType::ShaderStorageBuffer;
  }
  if (binding.descriptor_type == SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLER ||
      binding.descriptor_type == SPV_REFLECT_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
  {
    return ShaderBindingType::Sampler;
  }

  return ShaderBindingType::UniformBuffer;
}


ShaderBufferMemberType GetTypeFromSpvReflect(SpvReflectTypeDescription* typeDescription)
{

  if (typeDescription->type_flags & SPV_REFLECT_TYPE_FLAG_MATRIX)
  {
    if (typeDescription->traits.numeric.matrix.column_count == 4 && typeDescription->traits.numeric.matrix.row_count == 4)
    {
      return ShaderBufferMemberType::_mat4;
    }

    if (typeDescription->traits.numeric.matrix.column_count == 3 && typeDescription->traits.numeric.matrix.row_count == 3)
    {
      return ShaderBufferMemberType::_mat3;
    }

    return ShaderBufferMemberType::UNKNOWN;
  }

  if (typeDescription->type_flags & SPV_REFLECT_TYPE_FLAG_VECTOR)
  {
    if (typeDescription->traits.numeric.vector.component_count == 2)
    {
      return ShaderBufferMemberType::_vec2;
    }

    if (typeDescription->traits.numeric.vector.component_count == 3)
    {
      return ShaderBufferMemberType::_vec3;
    }

    if (typeDescription->traits.numeric.vector.component_count == 4)
    {
      return ShaderBufferMemberType::_vec4;
    }
    return ShaderBufferMemberType::UNKNOWN;
  }

  if (typeDescription->type_flags & SPV_REFLECT_TYPE_FLAG_ARRAY)
  {
    return ShaderBufferMemberType::_array;
  }

  if (typeDescription->type_flags & SPV_REFLECT_TYPE_FLAG_FLOAT)
  {
    return ShaderBufferMemberType::_float;
  }

  if (typeDescription->type_flags & SPV_REFLECT_TYPE_FLAG_INT)
  {
    // signedness: unsigned = 0, signed = 1(?)
    // might need to come back to this if we need 64 bit ints.
    if (typeDescription->traits.numeric.scalar.signedness == 0)
    {
      return ShaderBufferMemberType::_uint;
    }
    else
    {
      return ShaderBufferMemberType::_int;
    }
  }

  return ShaderBufferMemberType::UNKNOWN;
}


Vector<VkDescriptorSetLayoutBinding> GetDescriptorSetLayoutBindings(VkState& vk, Vector<DescriptorSetLayoutData>& vertLayoutDatas, Vector<DescriptorSetLayoutData>& fragLayoutDatas)
{
  Vector<VkDescriptorSetLayoutBinding> bindings;
  uint8_t count = 0;

  for (auto& vertLayoutData : vertLayoutDatas)
  {
    count += static_cast<uint8_t>(vertLayoutData.m_Bindings.size());
  }

  for (auto& fragLayoutData : fragLayoutDatas)
  {
    count += static_cast<uint8_t>(fragLayoutData.m_Bindings.size());
  }

  bindings.resize(count);

  count = 0;
  // .. do the things
  for (auto& vertLayoutData : vertLayoutDatas)
  {
    for (auto& binding : vertLayoutData.m_Bindings)
    {
      bindings[count] = binding;
      count++;
    }
  }

  for (auto& fragLayoutData : fragLayoutDatas)
  {
    for (auto& binding : fragLayoutData.m_Bindings)
    {
      bindings[count] = binding;
      count++;
    }
  }

  return CleanDescriptorSetLayout(bindings);
}

void CreateDescriptorSetLayout(VkState& vk, std::vector<DescriptorSetLayoutData>& vertLayoutDatas, std::vector<DescriptorSetLayoutData>& fragLayoutDatas, VkDescriptorSetLayout& descriptorSetLayout)
{
  std::vector<VkDescriptorSetLayoutBinding> bindings;
  uint8_t count = 0;

  for (auto& vertLayoutData : vertLayoutDatas)
  {
    count += static_cast<uint8_t>(vertLayoutData.m_Bindings.size());
  }

  for (auto& fragLayoutData : fragLayoutDatas)
  {
    count += static_cast<uint8_t>(fragLayoutData.m_Bindings.size());
  }

  bindings.resize(count);

  count = 0;
  // .. do the things
  for (auto& vertLayoutData : vertLayoutDatas)
  {
    for (auto& binding : vertLayoutData.m_Bindings)
    {
      bindings[count] = binding;
      count++;
    }
  }

  for (auto& fragLayoutData : fragLayoutDatas)
  {
    for (auto& binding : fragLayoutData.m_Bindings)
    {
      bindings[count] = binding;
      count++;
    }
  }

  Vector<VkDescriptorSetLayoutBinding> cleanBindings = CleanDescriptorSetLayout(bindings);

  VkDescriptorSetLayoutCreateInfo layoutInfo{};
  layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  layoutInfo.bindingCount = static_cast<uint32_t>(cleanBindings.size());
  layoutInfo.pBindings = cleanBindings.data();

  VK_CHECK(vkCreateDescriptorSetLayout(vk.m_LogicalDevice, &layoutInfo, nullptr, &descriptorSetLayout))
}

Vector<DescriptorSetLayoutData> ReflectDescriptorSetLayouts(VkState& vk, StageBinary& stageBin)
{
  SpvReflectShaderModule shaderReflectModule;
  SpvReflectResult result = spvReflectCreateShaderModule(stageBin.size(), stageBin.data(), &shaderReflectModule);

  uint32_t descriptorSetCount = 0;
  spvReflectEnumerateDescriptorSets(&shaderReflectModule, &descriptorSetCount, nullptr);

  if (descriptorSetCount == 0)
  {
    return {};
  }

  std::vector<SpvReflectDescriptorSet*> reflectedDescriptorSets;
  reflectedDescriptorSets.resize(descriptorSetCount);
  spvReflectEnumerateDescriptorSets(&shaderReflectModule, &descriptorSetCount, &reflectedDescriptorSets[0]);


  std::vector<DescriptorSetLayoutData> layoutDatas(descriptorSetCount, DescriptorSetLayoutData{});

  for (int i = 0; i < reflectedDescriptorSets.size(); i++)
  {
    const SpvReflectDescriptorSet& reflectedSet = *reflectedDescriptorSets[i];
    DescriptorSetLayoutData& layoutData = layoutDatas[i];

    layoutData.m_Bindings.resize(reflectedSet.binding_count);
    for (uint32_t bc = 0; bc < reflectedSet.binding_count; bc++)
    {
      const SpvReflectDescriptorBinding& reflectedBinding = *reflectedSet.bindings[bc];
      VkDescriptorSetLayoutBinding& layoutBinding = layoutData.m_Bindings[bc];
      layoutBinding.binding = reflectedBinding.binding;
      layoutBinding.descriptorType = static_cast<VkDescriptorType>(reflectedBinding.descriptor_type);
      layoutBinding.descriptorCount = 1; // sus
      for (uint32_t i_dim = 0; i_dim < reflectedBinding.array.dims_count; ++i_dim) {
        layoutBinding.descriptorCount *= reflectedBinding.array.dims[i_dim];
      }
      layoutBinding.stageFlags = static_cast<VkShaderStageFlagBits>(shaderReflectModule.shader_stage);

      ShaderBindingType bufferType = GetBindingType(reflectedBinding);
      DescriptorSetLayoutBindingData binding{ String(reflectedBinding.name), reflectedBinding.binding, reflectedBinding.block.size , bufferType};

      if (bufferType == ShaderBindingType::ShaderStorageBuffer)
      {
        uint32_t requiredBufferSize = 0;
        for (uint32_t i = 0; i < reflectedBinding.type_description->member_count; i++)
        {
          auto* member = &reflectedBinding.type_description->members[i];
          if (member->type_flags & SPV_REFLECT_TYPE_FLAG_ARRAY)
          {
            requiredBufferSize += member->traits.array.dims[0] * member->traits.array.stride; // todo: support more than 1D arrays
          }

          for (uint32_t j = 0; j < member->member_count; j++)
          {
            auto* childMember = &member->members[i];
            int jkj = 420;
          }
        }
        binding.m_ExpectedBufferSize = requiredBufferSize;
        continue;
      }

      for (uint32_t i = 0; i < reflectedBinding.block.member_count; i++)
      {
        auto member = reflectedBinding.block.members[i];
        ShaderBufferMember reflectedMember{};
        reflectedMember.m_Name = String(member.name);
        reflectedMember.m_Offset = member.absolute_offset; // this might be an issue with padded types?
        reflectedMember.m_Size = member.padded_size;
        reflectedMember.m_Type = GetTypeFromSpvReflect(member.type_description);

        if (member.array.dims_count > 0)
        {
          reflectedMember.m_Stride = member.array.stride;
        }
        else
        {
          reflectedMember.m_Stride = 0;
        }

        binding.m_Members.push_back(reflectedMember);
      }
      if (reflectedBinding.resource_type & SPV_REFLECT_RESOURCE_FLAG_SAMPLER)
      {
        ShaderBufferMember reflectedMember{};
        reflectedMember.m_Name = String(reflectedBinding.name);
        reflectedMember.m_Type = ShaderBufferMemberType::_sampler;
        binding.m_Members.push_back(reflectedMember);
      }
      layoutData.m_BindingDatas.push_back(binding);
    }

    layoutData.m_SetNumber = reflectedSet.set;
    layoutData.m_CreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutData.m_CreateInfo.bindingCount = reflectedSet.binding_count;
    layoutData.m_CreateInfo.pBindings = layoutData.m_Bindings.data();
  }

  return layoutDatas;
}

Vector<PushConstantBlock> ReflectPushConstants(VkState& vk, StageBinary& stageBin)
{
  Vector<PushConstantBlock> pushConstants{};
  SpvReflectShaderModule shaderReflectModule;
  SpvReflectResult result = spvReflectCreateShaderModule(stageBin.size(), stageBin.data(), &shaderReflectModule);

  uint32_t pushConstantBlockCount = 0;
  spvReflectEnumeratePushConstantBlocks(&shaderReflectModule, &pushConstantBlockCount, nullptr);
  std::vector<SpvReflectBlockVariable*> reflectedPushConstantBlocks;
  reflectedPushConstantBlocks.resize(pushConstantBlockCount);
  spvReflectEnumeratePushConstantBlocks(&shaderReflectModule, &pushConstantBlockCount, reflectedPushConstantBlocks.data());


  for (uint32_t i = 0; i < pushConstantBlockCount; i++)
  {
    const SpvReflectBlockVariable& pcBlock = *reflectedPushConstantBlocks[i];
    pushConstants.push_back({ pcBlock.size, pcBlock.offset, pcBlock.name, (VkShaderStageFlags) shaderReflectModule.shader_stage });
  }

  return pushConstants;
}

VkDescriptorSet CreateDescriptorSet(VkState& vk, DescriptorSetLayoutData& layoutData)
{
  return vk.m_DescriptorSetAllocator.Allocate(vk.m_LogicalDevice, layoutData.m_Layout, nullptr);
}
}
}
