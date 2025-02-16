#pragma once
#include "lvk/Structs.h"
#include "ThirdParty/spirv_reflect.h"

namespace lvk {
namespace descriptor{

    Vector<VkDescriptorSetLayoutBinding>GetDescriptorSetLayoutBindings(VkState& vk, Vector<DescriptorSetLayoutData>& vertLayoutDatas, Vector<DescriptorSetLayoutData>& fragLayoutDatas);
    void                                CreateDescriptorSetLayout(VkState& vk, Vector<DescriptorSetLayoutData>& vertLayoutDatas, Vector<DescriptorSetLayoutData>& fragLayoutDatas, VkDescriptorSetLayout& descriptorSetLayout);

    Vector<DescriptorSetLayoutData>     ReflectDescriptorSetLayouts(VkState& vk, StageBinary& stageBin);
    Vector<DescriptorSetLayoutData>     ReflectDescriptorSetLayoutsRaw(VkState& vk, const char* stage_bin, size_t stage_size);

    Vector<PushConstantBlock>           ReflectPushConstants(VkState& vk, StageBinary& stageBin);
    Vector<PushConstantBlock>           ReflectPushConstantsRaw(VkState& vk, const char* stage_bin, size_t stage_size);

    VkDescriptorSet                     CreateDescriptorSet(VkState& vk, DescriptorSetLayoutData& layoutData);
    ShaderBufferMemberType              GetTypeFromSpvReflect(SpvReflectTypeDescription* typeDescription);
}
}