#pragma once
#include "lvk/Material.h"
#include "Im3D/im3d_lvk.h"

namespace lvk
{
    struct VkPipelineData
    {
        VkPipelineData(VkPipeline pipeline, VkPipelineLayout layout)
        {
            m_Pipeline = pipeline;
            m_PipelineLayout = layout;
        }

        VkPipeline          m_Pipeline;
        VkPipelineLayout    m_PipelineLayout;
    };

    template<typename _Ty>
    class Pipeline
    {
    public:
        Vector<Framebuffer*>                 m_FBs;
        Vector<Material*>                    m_PipelineMaterials;
        Vector<VkPipelineData*>              m_PipelineDatas;

        Optional<Framebuffer*>              m_OutputFramebuffer;
        LvkIm3dViewState* m_Im3dState = nullptr;

        Optional<_Ty>   m_CommandCallback;

        Framebuffer* AddFramebuffer(VulkanAPI& vk)
        {
            m_FBs.push_back(new Framebuffer());
            return m_FBs[m_FBs.size() - 1];
        }

        void SetOutputFramebuffer(Framebuffer* fb)
        {
            m_OutputFramebuffer = fb;
        }

        Framebuffer* GetOutputFramebuffer()
        {
            if (m_OutputFramebuffer.has_value())
            {
                return m_OutputFramebuffer.value();
            }
            return nullptr;
        }

        Material* AddMaterial(VulkanAPI& vk, ShaderProgram& prog)
        {
            m_PipelineMaterials.push_back(new Material(Material::Create(vk, prog)));
            return m_PipelineMaterials.back();
        }

        VkPipelineData* AddPipeline(VulkanAPI& vk, VkPipeline pipeline, VkPipelineLayout layout)
        {
            m_PipelineDatas.emplace_back(new VkPipelineData(pipeline, layout));
            return m_PipelineDatas.back();
        }

        LvkIm3dViewState* AddIm3d(VulkanAPI& vk, LvkIm3dState im3dState)
        {
            m_Im3dState = new LvkIm3dViewState(AddIm3dForViewport(vk, im3dState, m_OutputFramebuffer.value()->m_RenderPass, false));
            return m_Im3dState;
        }

        void RecordCommands(_Ty callback)
        {
            m_CommandCallback = callback;
        }
    };
}