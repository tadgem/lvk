#pragma once
#include "lvk/Descriptor.h"
#include "lvk/Structs.h"
#include "lvk/Utils.h"
#include <filesystem>
namespace lvk
{
    VkShaderModule CreateShaderModule(VkState& vk, const StageBinary& data);
    VkShaderModule CreateShaderModuleRaw(VkState& vk, const char* data, size_t length);

    StageBinary CreateStageBinaryFromSource(VkState& vk,
      ShaderStageType type, const String& sourc, const String& shaderName);

    struct ShaderStage
    {
        String          m_Name;
        StageBinary     m_StageBinary;
        VkShaderModule  m_Module;
        Vector<PushConstantBlock>       m_PushConstants;
        Vector<DescriptorSetLayoutData> m_LayoutDatas;
        ShaderStageType m_Type;

        static String      LoadShaderSource(const String& path);

        static ShaderStage CreateFromBinary(VkState & vk, Vector<unsigned char>& binary, const ShaderStageType& type, const String& name)
        {
            auto stageLayoutDatas = descriptor::ReflectDescriptorSetLayouts(vk, binary);
            auto pushConstants = descriptor::ReflectPushConstants(vk, binary);
            auto module = CreateShaderModule(vk, binary);

            return { name, binary, module, pushConstants, stageLayoutDatas, type };
        }

        static ShaderStage
        CreateFromBinaryPath(VkState & vk, const String& stagePath, const ShaderStageType& stageType)
        {
            String name = std::filesystem::path(stagePath).filename().u8string();
            auto stageBin = utils::LoadSpirvBinary(stagePath);
            return CreateFromBinary(vk, stageBin, stageType, name);
        }

        static ShaderStage CreateFromSource(VkState & vk, const String& source, const ShaderStageType& type, const String& name, const String& path = "")
        {
            auto bin = CreateStageBinaryFromSource(vk, type, source, path);
            if(bin.empty())
            {
              return {};
            }
            auto stageLayoutDatas = descriptor::ReflectDescriptorSetLayouts(vk, bin);
            auto pushConstants = descriptor::ReflectPushConstants(vk, bin);
            auto module = CreateShaderModule(vk, bin);

            return { name, bin, module, pushConstants, stageLayoutDatas, type };
        }

        static ShaderStage CreateFromSourcePath(VkState & vk, const String& path, const ShaderStageType& type)
        {
            String name = std::filesystem::path(path).filename().u8string();
            auto source = LoadShaderSource(path);
            return CreateFromSource(vk, source, type, path, name);
        }
    };

    struct ShaderProgram
    {
        ShaderProgram(Vector<ShaderStage> shaderStages, VkDescriptorSetLayout layout);
        Vector<ShaderStage>         m_Stages;

        VkDescriptorSetLayout       m_DescriptorSetLayout;
        Vector<VkPushConstantRange> m_PushConstantRanges;

        void                        Free(VkState & vk);
        void                        BuildPushConstantRanges();
        uint32_t                    GetPushConstantRangeCount();
        VkPipelineLayoutCreateInfo  GetPipelineLayoutCreateInfo();

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

        static ShaderProgram CreateCompute(VkState & vk, ShaderStage& compute);

        static ShaderProgram CreateComputeFromBinaryPath(VkState& vk, const String& comp_path)
        {
            ShaderStage comp = ShaderStage::CreateFromBinaryPath(vk, comp_path, ShaderStageType::Compute);
            return CreateCompute(vk, comp);
        }

        static ShaderProgram CreateComputeFromSourcePath(VkState& vk, const String& compute_src_path)
        {
            ShaderStage comp = ShaderStage::CreateFromSourcePath(vk, compute_src_path, ShaderStageType::Compute);
            return CreateCompute(vk, comp);
        }
    };

}