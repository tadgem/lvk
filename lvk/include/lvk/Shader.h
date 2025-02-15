#pragma once
#include "lvk/Descriptor.h"
#include "lvk/Structs.h"
#include "lvk/Utils.h"
namespace lvk
{
    VkShaderModule CreateShaderModule(VkState& vk, const StageBinary& data);

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

        static ShaderStage
        CreateFromBinaryPath(VkState & vk, const String& stagePath, const ShaderStage::Type& stageType)
        {
            auto stageBin = utils::LoadSpirvBinary(stagePath);
            auto pushConstants = descriptor::ReflectPushConstants(vk, stageBin);
            auto stageLayoutDatas = descriptor::ReflectDescriptorSetLayouts(vk, stageBin);

            return { stageBin, pushConstants, stageLayoutDatas, stageType };
        }

        static ShaderStage CreateFromBinary(VkState & vk, Vector<unsigned char>& binary, const ShaderStage::Type& type)
        {
            auto stageLayoutDatas = descriptor::ReflectDescriptorSetLayouts(vk, binary);
            auto pushConstants = descriptor::ReflectPushConstants(vk, binary);

            return { binary, pushConstants, stageLayoutDatas, type };
        }
    };

    struct ShaderProgram
    {
        Vector<ShaderStage> m_Stages;

        VkDescriptorSetLayout m_DescriptorSetLayout;

        void Free(VkState & vk);

        static ShaderProgram CreateFromBinaryPath(VkState & vk, const String& vertPath, const String& fragPath)
        {
            ShaderStage vert = ShaderStage::CreateFromBinaryPath(
                vk, vertPath, ShaderStage::Type::Vertex);
            ShaderStage frag = ShaderStage::CreateFromBinaryPath(
                vk, fragPath, ShaderStage::Type::Fragment);
            VkDescriptorSetLayout layout;
            descriptor::CreateDescriptorSetLayout(vk, vert.m_LayoutDatas, frag.m_LayoutDatas, layout);

            return { Vector<ShaderStage> {vert, frag} , layout };

        }

        static ShaderProgram Create(VkState & vk, ShaderStage& vert, ShaderStage& frag)
        {
            VkDescriptorSetLayout layout;
            descriptor::CreateDescriptorSetLayout(vk, vert.m_LayoutDatas, frag.m_LayoutDatas, layout);

            return { Vector<ShaderStage> {vert, frag} , layout };

        }

        static ShaderProgram CreateCompute(VkState & vk, const String& computePath);
    };

}