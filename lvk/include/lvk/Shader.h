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
        Vector<DescriptorSetLayoutData> m_LayoutDatas;
        ShaderStage::Type m_Type;

        static ShaderStage Create(VulkanAPI& vk, const String& stagePath, const ShaderStage::Type& stageType)
        {
            auto stageBin = vk.LoadSpirvBinary(stagePath);
            auto stageLayoutDatas = vk.ReflectDescriptorSetLayouts(stageBin);

            return { stageBin, stageLayoutDatas, stageType };
        }
    };

    struct ShaderProgram
    {
        Vector<ShaderStage> m_Stages;

        VkDescriptorSetLayout m_DescriptorSetLayout;

        static ShaderProgram Create(VulkanAPI& vk, const String& vertPath, const String& fragPath)
        {
            ShaderStage vert = ShaderStage::Create(vk, vertPath, ShaderStage::Type::Vertex);
            ShaderStage frag = ShaderStage::Create(vk, fragPath, ShaderStage::Type::Fragment);
            VkDescriptorSetLayout layout;
            vk.CreateDescriptorSetLayout(vert.m_LayoutDatas, frag.m_LayoutDatas, layout);

            return { Vector<ShaderStage> {vert, frag} , layout };

        }
    };

}