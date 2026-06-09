#pragma once

#include "lvk/Material.h"
#include "lvk/Shader.h"
#pragma warning(push)
#pragma warning (disable: 4324)
#include "Im3D/im3d.h"
#pragma warning(pop)
#include "glm/glm.hpp"

namespace lvk
{
    struct LvkIm3dState
    {
        ShaderProgram m_TriProg;
        ShaderProgram m_PointsProg;
        ShaderProgram m_LinesProg;
        // also ss quad mesh
        Buffer        m_ScreenQuadBuffer;
    };

    struct LvkIm3dViewState
    {
        Material m_TrisMaterial;
        Material m_PointsMaterial;
        Material m_LinesMaterial;

        VkPipelineData m_TrisPipeline;
        VkPipelineData m_PointsPipeline;
        VkPipelineData m_LinesPipeline;
    };

    LvkIm3dState LoadIm3D(VkState & vk);
    LvkIm3dViewState AddIm3dForViewport(VkState & vk, LvkIm3dState& state, VkRenderPass renderPass, bool enableMSAA, bool enableDynamicRendering = false);
    LvkIm3dViewState AddIm3dForGBuffer(VkState & vk, LvkIm3dState& state);
    LvkIm3dViewState AddIm3dForDeferredLightPass(VkState & vk, LvkIm3dState& state);
    LvkIm3dViewState AddIm3dForGBufferRenderPass(VkState & vk, LvkIm3dState& state, VkRenderPass gbufferRenderPass, uint32_t colourAttachmentCount, uint32_t colourWriteAttachment);
    LvkIm3dViewState AddIm3dForForwardPass(VkState & vk, LvkIm3dState& state, bool enableDynamicRendering);
    void FreeIm3dViewport(VkState & vk, LvkIm3dViewState& viewState);
    void FreeIm3d(VkState & vk, LvkIm3dState& state);
    void DrawIm3d(VkState & vk, VkCommandBuffer& buffer, uint32_t frameIndex, LvkIm3dState& state, LvkIm3dViewState& viewState, glm::mat4 _viewProj, uint32_t width, uint32_t height, bool drawText = false);
    void DrawIm3dTextListsImGui(uint32_t width, uint32_t height, glm::mat4 _viewProj);
    void DrawIm3dTextListsImGuiAsChild(glm::mat4 _viewProj);

}