#pragma once
#include "lvk/VulkanAPI.h"

namespace lvk
{

    struct ShaderStage
    {
        enum class Type
        {
            Unknown,
            Vertex,
            Fragment,
            Compute
        };

        StageBinary m_StageBinary;
        Vector<PushConstantBlock>       m_PushConstants;
        Vector<DescriptorSetLayoutData> m_LayoutDatas;
        ShaderStage::Type m_Type;

        static ShaderStage Create(VulkanAPI& vk, const String& stagePath, const ShaderStage::Type& stageType)
        {
            auto stageBin = vk.LoadSpirvBinary(stagePath);
            auto pushConstants = vk.ReflectPushConstants(stageBin);
            auto stageLayoutDatas = vk.ReflectDescriptorSetLayouts(stageBin);

            return { stageBin, pushConstants, stageLayoutDatas, stageType };
        }

        static ShaderStage Create(VulkanAPI& vk, Vector<unsigned char>& binary, const ShaderStage::Type& type)
        {
            auto stageLayoutDatas = vk.ReflectDescriptorSetLayouts(binary);
            auto pushConstants = vk.ReflectPushConstants(binary);

            return { binary, pushConstants, stageLayoutDatas, type };
        }
    };

    struct ShaderProgram
    {
        Vector<ShaderStage> m_Stages;

        VkDescriptorSetLayout m_DescriptorSetLayout;

        void Free(VulkanAPI& vk);

        static ShaderProgram Create(VulkanAPI& vk, const String& vertPath, const String& fragPath)
        {
            ShaderStage vert = ShaderStage::Create(vk, vertPath, ShaderStage::Type::Vertex);
            ShaderStage frag = ShaderStage::Create(vk, fragPath, ShaderStage::Type::Fragment);
            VkDescriptorSetLayout layout;
            vk.CreateDescriptorSetLayout(vert.m_LayoutDatas, frag.m_LayoutDatas, layout);

            return { Vector<ShaderStage> {vert, frag} , layout };

        }

        static ShaderProgram Create(VulkanAPI& vk, ShaderStage& vert, ShaderStage& frag)
        {
            VkDescriptorSetLayout layout;
            vk.CreateDescriptorSetLayout(vert.m_LayoutDatas, frag.m_LayoutDatas, layout);

            return { Vector<ShaderStage> {vert, frag} , layout };

        }

        static ShaderProgram CreateCompute(VulkanAPI& vk, const String& computePath);
    };

}