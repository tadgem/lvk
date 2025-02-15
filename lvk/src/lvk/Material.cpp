#include "lvk/Material.h"
#include "lvk/Shader.h"
#include "lvk/Texture.h"
#include "lvk/Buffer.h"
#include "volk.h"

static auto collect_uniform_data = [](lvk::ShaderStage& stage, lvk::Material &mat, lvk::VkState & vk)
    {
        using namespace lvk;

        for (auto& pushConstant : stage.m_PushConstants)
        {
            mat.m_PushConstants.push_back(pushConstant);
        }

        for (auto& descriptorSetInfo : stage.m_LayoutDatas)
        {
            for (auto& bindingInfo : descriptorSetInfo.m_BindingDatas)
            {
                if (bindingInfo.m_ExpectedBufferSize == 0 && bindingInfo.m_BufferType == ShaderBindingType::Sampler)
                {
                    Material::SamplerBindingData sbd{
                        descriptorSetInfo.m_SetNumber,
                        bindingInfo.m_BindingIndex,
                        Texture::g_DefaultTexture->m_ImageView,
                        Texture::g_DefaultTexture->m_Sampler
                    };
                    mat.m_Samplers.emplace(bindingInfo.m_BindingName, sbd);
                    continue;
                }

                if (bindingInfo.m_BufferType == ShaderBindingType::UniformBuffer)
                {
                    // if a uniform buffer
                    ShaderBufferFrameData uniform;
                    buffers::CreateUniformBuffers(vk, uniform, VkDeviceSize{ bindingInfo.m_ExpectedBufferSize });
                    // build accessors
                    for (auto& member : bindingInfo.m_Members)
                    {
                        String accessorName = bindingInfo.m_BindingName + "." + member.m_Name;
                        uint16_t arraySize = member.m_Stride > 0 ? (member.m_Size / member.m_Stride) : 0;
                        Material::ShaderAccessorData data{ member.m_Size , member.m_Offset, member.m_Stride, arraySize, static_cast<uint32_t>(mat.m_UniformBuffers.size()) };
                        mat.m_UniformBufferAccessors.emplace(accessorName, data);
                    }

                    Material::SetBinding binding = {};
                    binding.m_Set = descriptorSetInfo.m_SetNumber;
                    binding.m_Binding = bindingInfo.m_BindingIndex;

                    mat.m_UniformBuffers.emplace(binding.m_Data,
                        Material::ShaderBufferBindingData{ descriptorSetInfo.m_SetNumber, bindingInfo.m_BindingIndex, bindingInfo.m_ExpectedBufferSize,  uniform });
                }
                else if (bindingInfo.m_BufferType == ShaderBindingType::ShaderStorageBuffer)
                {
                    int k = 420; 
                }
            }
        }
    };

// todo: add ability to add existing buffers when creating the material
lvk::Material lvk::Material::Create(VkState & vk, ShaderProgram& shader)
{
    Material mat{};

    mat.m_DescriptorSets.push_back(FrameDescriptorSets{});
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        mat.m_DescriptorSets.front().m_Sets[i] = vk.m_DescriptorSetAllocator.Allocate(vk.m_LogicalDevice, shader.m_DescriptorSetLayout, nullptr);
    }
    
    for (auto& stage : shader.m_Stages)
    {
        collect_uniform_data(stage, mat, vk);
    }
    
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {

        for (int i = 0; i < mat.m_PushConstants.size(); i++)
        {
        }


        // write buffers to descriptor set + default texture for any samplers
        Vector<VkDescriptorBufferInfo>  bufferWriteInfos;
        for (auto& [setBinding, bufferInfo] : mat.m_UniformBuffers)
        {
            VkDescriptorBufferInfo bufferWriteInfo{};
            bufferWriteInfo.buffer = bufferInfo.m_Buffer.m_UniformBuffers[0].m_GpuBuffer;
            bufferWriteInfo.offset = 0;
            bufferWriteInfo.range = bufferInfo.m_BufferSize;
            bufferWriteInfos.push_back(bufferWriteInfo);
        }
        Vector<VkDescriptorImageInfo>   imageWriteInfos;
        Vector<uint32_t> bindings;
        for (auto& [name, sampler] : mat.m_Samplers)
        {
            VkDescriptorImageInfo imageInfo{};
            imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imageInfo.imageView = sampler.m_ImageView;
            imageInfo.sampler = sampler.m_Sampler;
            imageWriteInfos.push_back(imageInfo);
            bindings.push_back(sampler.m_BindingNumber);
        }

        Vector<VkWriteDescriptorSet> descriptorWrites{};

        int k = 0;
        for (auto& [setBinding, ubo] : mat.m_UniformBuffers)
        {
            VkWriteDescriptorSet write{};
            write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            write.dstSet = mat.m_DescriptorSets.front().m_Sets[i];
            write.dstBinding = ubo.m_BindingNumber;
            write.dstArrayElement = 0; // todo
            write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            write.descriptorCount = 1;
            write.pBufferInfo = &bufferWriteInfos[k];
            descriptorWrites.push_back(write);
            k++;
        }
        for (int j = 0; j < imageWriteInfos.size(); j++)
        {
            VkWriteDescriptorSet write{};
            write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            write.dstSet = mat.m_DescriptorSets.front().m_Sets[i];
            write.dstBinding = bindings[j];
            write.dstArrayElement = 0;
            write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            write.descriptorCount = 1;
            write.pImageInfo = &imageWriteInfos[j];
            descriptorWrites.push_back(write);
        }

        vkUpdateDescriptorSets(vk.m_LogicalDevice, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);

    }
    return mat;
}

bool lvk::Material::SetSampler(VkState & vk, const String& name, const VkImageView& imageView, const VkSampler& sampler, bool isAttachment)
{
    if (m_Samplers.find(name) == m_Samplers.end())
    {
        return false;
    }

    VkImageLayout imageLayout = isAttachment ? VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    SamplerBindingData& samplerBinding = m_Samplers.at(name);
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = imageLayout;
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
    return true;
}

bool lvk::Material::SetColourAttachment(VkState & vk, const String& name, Framebuffer& framebuffer, uint32_t colourAttachmentIndex)
{
    if (m_Samplers.find(name) == m_Samplers.end())
    {
        return false;
    }

    VkImageLayout imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;

    SamplerBindingData& samplerBinding = m_Samplers.at(name);
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = imageLayout;
        imageInfo.imageView = framebuffer.m_ColourAttachments[colourAttachmentIndex].m_AttachmentSwapchainImages[i].m_ImageView;
        imageInfo.sampler = framebuffer.m_ColourAttachments[colourAttachmentIndex].m_AttachmentSwapchainImages[i].m_Sampler;

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

    return true;

}

bool lvk::Material::SetDepthAttachment(VkState & vk, const String& name, Framebuffer& framebuffer)
{
    if (m_Samplers.find(name) == m_Samplers.end())
    {
        return false;
    }

    VkImageLayout imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;

    SamplerBindingData& samplerBinding = m_Samplers.at(name);
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = imageLayout;
        imageInfo.imageView = framebuffer.m_DepthAttachments[0].m_AttachmentSwapchainImages[i].m_ImageView;
        imageInfo.sampler = framebuffer.m_DepthAttachments[0].m_AttachmentSwapchainImages[i].m_Sampler;

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
    return true;
}

void lvk::Material::Free(VkState & vk)
{
    m_UniformBufferAccessors.clear();

    for (auto& [setBinding, buffer] : m_UniformBuffers)
    {
        buffer.m_Buffer.Free(vk);
    }

    m_UniformBuffers.clear();
    m_Samplers.clear();

    /*for (auto& frameDescriptorSets : m_DescriptorSets)
    {
        for (auto& set : frameDescriptorSets.m_Sets)
        {
            VK_CHECK(vkFreeDescriptorSets(vk.m_LogicalDevice, vk.m_DescriptorPool, 1, &set));
        }
    }*/


}