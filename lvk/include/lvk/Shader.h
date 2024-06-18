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

        static ShaderStage Create(VulkanAPI& vk, Vector<char>& binary, const ShaderStage::Type& type)
        {
            auto stageLayoutDatas = vk.ReflectDescriptorSetLayouts(binary);
            return { binary, stageLayoutDatas, type };
        }
    };

    struct ShaderProgram
    {
        Vector<ShaderStage> m_Stages;

        VkDescriptorSetLayout m_DescriptorSetLayout;

        void Free(VulkanAPI& vk)
        {
            vkDestroyDescriptorSetLayout(vk.m_LogicalDevice, m_DescriptorSetLayout, nullptr);
        }

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

        static ShaderProgram CreateCompute(VulkanAPI& vk, const String& computePath)
        {
            ShaderStage comp = ShaderStage::Create(vk, computePath, ShaderStage::Type::Compute);
            VkDescriptorSetLayout layout;

            std::vector<VkDescriptorSetLayoutBinding> bindings;
            for (auto& layout : comp.m_LayoutDatas)
            {
                for (auto& binding : layout.m_Bindings)
                {
                    bindings.push_back(binding);
                }
            }
            VkDescriptorSetLayoutCreateInfo layoutInfo{};
            layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
            layoutInfo.pBindings = bindings.data();

            VK_CHECK(vkCreateDescriptorSetLayout(vk.m_LogicalDevice, &layoutInfo, nullptr, &layout))

            return { Vector<ShaderStage> {comp} , layout };

        }
    };

}