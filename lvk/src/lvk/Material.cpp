#include "lvk/Material.h"
#include "lvk/Shader.h"
#include "lvk/Texture.h"

lvk::Material lvk::Material::Create(VulkanAPI& vk, ShaderProgram& shader)
{
    Material mat{};
    Vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, shader.m_DescriptorSetLayout);
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = vk.m_DescriptorPool;
    allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
    allocInfo.pSetLayouts = layouts.data();

    mat.m_DescriptorSets.push_back(FrameDescriptorSets{});
    VK_CHECK(vkAllocateDescriptorSets(vk.m_LogicalDevice, &allocInfo, mat.m_DescriptorSets.front().m_Sets.data()));


    static auto collect_uniform_data = [&mat, &vk](ShaderStage& stage)
        {
            for (auto& descriptorSetInfo : stage.m_LayoutDatas)
            {
                for (auto& bindingInfo : descriptorSetInfo.m_BindingDatas)
                {
                    if (bindingInfo.m_ExpectedBlockSize == 0)
                    {
                        SamplerBindingData sbd{
                            descriptorSetInfo.m_SetNumber,
                            bindingInfo.m_BindingIndex,
                            Texture::g_DefaultTexture->m_ImageView,
                            Texture::g_DefaultTexture->m_Sampler
                        };
                        mat.m_Samplers.emplace(bindingInfo.m_BindingName, sbd);
                        continue;
                    }
                    // if a uniform buffer
                    UniformBufferFrameData uniform;
                    vk.CreateUniformBuffers(uniform, VkDeviceSize{ bindingInfo.m_ExpectedBlockSize });
                    // build accessors
                    for (auto& member : bindingInfo.m_Members)
                    {
                        String accessorName = bindingInfo.m_BindingName + "." + member.m_Name;
                        uint16_t arraySize = member.m_Stride > 0 ? (member.m_Size / member.m_Stride) : 0;
                        UniformAccessorData data{ member.m_Size , member.m_Offset, member.m_Stride, arraySize, static_cast<uint32_t>(mat.m_UniformBuffers.size()) };
                        mat.m_UniformBufferAccessors.emplace(accessorName, data);
                    }
                    mat.m_UniformBuffers.push_back({ descriptorSetInfo.m_SetNumber, bindingInfo.m_BindingIndex, bindingInfo.m_ExpectedBlockSize,  uniform });
                }
            }
        };
    // vert = 0, frag = 1
    collect_uniform_data(shader.m_Stages[0]);
    collect_uniform_data(shader.m_Stages[1]);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        // write buffers to descriptor set + default texture for any samplers
        Vector<VkDescriptorBufferInfo>  bufferWriteInfos;
        for (auto& bufferInfo : mat.m_UniformBuffers)
        {
            VkDescriptorBufferInfo bufferWriteInfo{};
            bufferWriteInfo.buffer = bufferInfo.m_UBO.m_UniformBuffers[0];
            bufferWriteInfo.offset = 0;
            bufferWriteInfo.range = bufferInfo.m_BufferSize;
            bufferWriteInfos.push_back(bufferWriteInfo);
        }
        Vector<VkDescriptorImageInfo>   imageWriteInfos;
        Vector<uint32_t> bindings;
        for (auto& [name, sampler] : mat.m_Samplers)
        {
            VkDescriptorImageInfo imageInfo{};
            imageInfo.imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
            imageInfo.imageView = sampler.m_ImageView;
            imageInfo.sampler = sampler.m_Sampler;
            imageWriteInfos.push_back(imageInfo);
            bindings.push_back(sampler.m_BindingNumber);
        }

        Vector<VkWriteDescriptorSet> descriptorWrites{};
        for (int j = 0; j < mat.m_UniformBuffers.size(); j++)
        {
            VkWriteDescriptorSet write{};
            write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            write.dstSet = mat.m_DescriptorSets.front().m_Sets[i];
            write.dstBinding = mat.m_UniformBuffers[j].m_BindingNumber;
            write.dstArrayElement = 0; // todo
            write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            write.descriptorCount = 1;
            write.pBufferInfo = &bufferWriteInfos[j];
        }
        for (int j = 0; j < imageWriteInfos.size(); j++)
        {
            VkWriteDescriptorSet write{};
            write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            write.dstSet = mat.m_DescriptorSets.front().m_Sets[i];
            write.dstBinding = 1;
            write.dstArrayElement = 0;
            write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            write.descriptorCount = 1;
            write.pImageInfo = &imageWriteInfos[j];
        }

        vkUpdateDescriptorSets(vk.m_LogicalDevice, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);

    }
    return mat;
}

bool lvk::Material::SetSampler(VulkanAPI& vk, const String& name, const VkImageView& imageView, const VkSampler& sampler)
{
    if (m_Samplers.find(name) == m_Samplers.end())
    {
        return false;
    }

    SamplerBindingData& samplerBinding = m_Samplers.at(name);
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
        imageInfo.imageView = imageView;
        imageInfo.sampler = sampler;

        VkWriteDescriptorSet write{};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstSet = m_DescriptorSets[samplerBinding.m_SetNumber].m_Sets[i];
        write.dstBinding = samplerBinding.m_BindingNumber;
        write.dstArrayElement = 0;
        write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        write.descriptorCount = 1;
        write.pImageInfo = &imageInfo;

        vkUpdateDescriptorSets(vk.m_LogicalDevice, 1, &write, 0, nullptr);
    }


}
