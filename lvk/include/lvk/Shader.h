#pragma once
#include "lvk/Descriptor.h"
#include "lvk/Structs.h"
#include "lvk/Utils.h"
namespace lvk
{
    VkShaderModule CreateShaderModule(VkState& vk, const StageBinary& data);
    VkShaderModule CreateShaderModuleRaw(VkState& vk, const char* data, size_t length);

    StageBinary CreateStageBinaryFromSource(VkState& vk, ShaderStageType type, const String& source);

    struct ShaderStage
    {

        StageBinary m_StageBinary;
        Vector<PushConstantBlock>       m_PushConstants;
        Vector<DescriptorSetLayoutData> m_LayoutDatas;
        ShaderStageType m_Type;

        static ShaderStage CreateFromBinary(VkState & vk, Vector<unsigned char>& binary, const ShaderStageType& type)
        {
            auto stageLayoutDatas = descriptor::ReflectDescriptorSetLayouts(vk, binary);
            auto pushConstants = descriptor::ReflectPushConstants(vk, binary);

            return { binary, pushConstants, stageLayoutDatas, type };
        }

        static ShaderStage
        CreateFromBinaryPath(VkState & vk, const String& stagePath, const ShaderStageType& stageType)
        {
            auto stageBin = utils::LoadSpirvBinary(stagePath);
            return CreateFromBinary(vk, stageBin, stageType);
        }

        static ShaderStage CreateFromSource(VkState & vk, const String& source, const ShaderStageType& type)
        {
            auto bin = CreateStageBinaryFromSource(vk, type, source);
            auto stageLayoutDatas = descriptor::ReflectDescriptorSetLayouts(vk, bin);
            auto pushConstants = descriptor::ReflectPushConstants(vk, bin);

            return { bin, pushConstants, stageLayoutDatas, type };
        }

        static ShaderStage CreateFromSourcePath(VkState & vk, const String& path, const ShaderStageType& type)
        {
            auto source = utils::LoadStringFromPath(path);
            return CreateFromSource(vk, source, type);
        }
    };

    struct ShaderProgram
    {
        Vector<ShaderStage> m_Stages;

        VkDescriptorSetLayout m_DescriptorSetLayout;

        void Free(VkState & vk);

        static ShaderProgram CreateGraphics(VkState & vk, ShaderStage& vert, ShaderStage& frag)
        {
            VkDescriptorSetLayout layout;
            descriptor::CreateDescriptorSetLayout(vk, vert.m_LayoutDatas, frag.m_LayoutDatas, layout);

            return { Vector<ShaderStage> {vert, frag} , layout };
        }

        static ShaderProgram
        CreateGraphicsFromBinaryPath(VkState & vk, const String& vertPath, const String& fragPath)
        {
            ShaderStage vert = ShaderStage::CreateFromBinaryPath(
                vk, vertPath, ShaderStageType::Vertex);
            ShaderStage frag = ShaderStage::CreateFromBinaryPath(
                vk, fragPath, ShaderStageType::Fragment);
            return CreateGraphics(vk, vert, frag);
        }

        static ShaderProgram
        CreateGraphicsFromSourcePath(VkState & vk, const String& vertPath, const String& fragPath)
        {
            ShaderStage vert = ShaderStage::CreateFromSourcePath(
                vk, vertPath, ShaderStageType::Vertex);
            ShaderStage frag = ShaderStage::CreateFromSourcePath(
                vk, fragPath, ShaderStageType::Fragment);
            return CreateGraphics(vk, vert, frag);
        }

        static ShaderProgram CreateCompute(VkState & vk, const String& computePath);
    };

}