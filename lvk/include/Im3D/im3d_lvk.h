#pragma once

#include "lvk/Material.h"
#include "lvk/Shader.h"
#include "glm/glm.hpp"
#include "Im3D/im3d.h"

namespace lvk
{
    struct LvkIm3dState
    {
        ShaderProgram m_TriProg;
        ShaderProgram m_PointsProg;
        ShaderProgram m_LinesProg;
        // also ss quad mesh
        VkBuffer        m_ScreenQuad;
        VmaAllocation   m_ScreenQuadMemory;
    };

    struct LvkIm3dViewState
    {
        Material m_TrisMaterial;
        Material m_PointsMaterial;
        Material m_LinesMaterial;

        VkPipeline m_TrisPipeline;
        VkPipeline m_PointsPipeline;
        VkPipeline m_LinesPipeline;

        VkPipelineLayout m_TrisPipelineLayout;
        VkPipelineLayout m_PointsPipelineLayout;
        VkPipelineLayout m_LinesPipelineLayout;
    };

    LvkIm3dState LoadIm3D(VkBackend & vk);
    LvkIm3dViewState AddIm3dForViewport(VkBackend & vk, LvkIm3dState& state, VkRenderPass renderPass, bool enableMSAA);
    void FreeIm3dViewport(VkBackend & vk, LvkIm3dViewState& viewState);
    void FreeIm3d(VkBackend & vk, LvkIm3dState& state);
    void DrawIm3d(VkBackend & vk, VkCommandBuffer& buffer, uint32_t frameIndex, LvkIm3dState& state, LvkIm3dViewState& viewState, glm::mat4 _viewProj, uint32_t width, uint32_t height, bool drawText = false);
    void DrawIm3dTextListsImGui(const Im3d::TextDrawList _textDrawLists[], uint32_t _count, uint32_t width, uint32_t height, glm::mat4 _viewProj);
    void DrawIm3dTextListsImGuiAsChild(const Im3d::TextDrawList _textDrawLists[], uint32_t _count, uint32_t width, uint32_t height, glm::mat4 _viewProj);

}