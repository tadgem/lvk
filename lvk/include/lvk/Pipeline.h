#pragma once
#include "lvk/Material.h"
#include "lvk/Shader.h"
#include "Im3D/im3d_lvk.h"

namespace lvk
{
namespace pipelines{

    VkPipeline                          CreateComputePipeline(
        VkState& vk,
        StageBinary& comp,
        VkDescriptorSetLayout& descriptorSetLayout,
        VkPipelineLayout& pipelineLayout);

    VkPipeline                          CreateRasterPipeline(
        VkState& vk,
        ShaderProgram& shader,
        VertexDescription& vertexDescription,
        RasterizationState & rasterState,
        RasterPipelineState& pipelineState,
        VkRenderPass& pipelineRenderPass,
        VkExtent2D resolution,
        VkPipelineLayout& pipelineLayout,
        uint32_t colorAttachmentCount = 1);

    struct VkPipelineData
    {
        VkPipelineData(VkPipeline pipeline, VkPipelineLayout layout)
        {
            m_Pipeline = pipeline;
            m_PipelineLayout = layout;
        }

    	VkPipelineData() = default;

    	void Free(VkState & vk)
    	{
    		vkDestroyPipelineLayout (vk.m_LogicalDevice, m_PipelineLayout, nullptr);
    		vkDestroyPipeline(vk.m_LogicalDevice, m_Pipeline, nullptr);
    	}

        VkPipeline          m_Pipeline = VK_NULL_HANDLE;
        VkPipelineLayout    m_PipelineLayout = VK_NULL_HANDLE;
    };

    class Pipeline
    {
    public:
        Vector<Framebuffer*>                 m_FBs;
        Vector<Material*>                    m_PipelineMaterials;
        Vector<VkPipelineData*>              m_PipelineDatas;

        Optional<Framebuffer*>              m_OutputFramebuffer;
        LvkIm3dViewState* m_Im3dState = nullptr;

        Framebuffer* AddFramebuffer(VkState & vk)
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

        Material* AddMaterial(VkState & vk, ShaderProgram& prog)
        {
            m_PipelineMaterials.push_back(new Material(Material::Create(vk, prog)));
            return m_PipelineMaterials.back();
        }

        VkPipelineData* AddPipeline(VkState & vk, VkPipeline pipeline, VkPipelineLayout layout)
        {
            m_PipelineDatas.emplace_back(new VkPipelineData(pipeline, layout));
            return m_PipelineDatas.back();
        }

        LvkIm3dViewState* AddIm3d(VkState & vk, LvkIm3dState im3dState)
        {
            m_Im3dState = new LvkIm3dViewState(AddIm3dForViewport(vk, im3dState, m_OutputFramebuffer.value()->m_RenderPass, false));
            return m_Im3dState;
        }

    	void Free(VkState & vk)
        {
        	if(m_Im3dState != nullptr) {
        		lvk::FreeIm3dViewport (vk,*m_Im3dState);
        	}
	        for(auto* fb : m_FBs) {
		        fb->Free (vk);
	        }

        	for(auto* material : m_PipelineMaterials) {
        		material->Free (vk);
        	}

        	for(auto* pipeline_data : m_PipelineDatas) {
        		pipeline_data->Free (vk);
        	}
        }
    };

    template<typename _Ty>
    class PipelineT
    {
    public:
        Vector<Framebuffer*>                 m_FBs;
        Vector<Material*>                    m_PipelineMaterials;
        Vector<VkPipelineData*>              m_PipelineDatas;

        Optional<Framebuffer*>              m_OutputFramebuffer;
        LvkIm3dViewState* m_Im3dState = nullptr;

        Optional<_Ty>   m_CommandCallback;

        Framebuffer* AddFramebuffer(VkState & vk)
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

        Material* AddMaterial(VkState & vk, ShaderProgram& prog)
        {
            m_PipelineMaterials.push_back(new Material(Material::Create(vk, prog)));
            return m_PipelineMaterials.back();
        }

        VkPipelineData* AddPipeline(VkState & vk, VkPipeline pipeline, VkPipelineLayout layout)
        {
            m_PipelineDatas.emplace_back(new VkPipelineData(pipeline, layout));
            return m_PipelineDatas.back();
        }

        LvkIm3dViewState* AddIm3d(VkState & vk, LvkIm3dState im3dState)
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
}