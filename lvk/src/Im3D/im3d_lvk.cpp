#include "Im3D/im3d_lvk.h"
#include "lvk/Mesh.h"
#include "Im3D/shaders/im3d_lines.vert.spv.h"
#include "Im3D/shaders/im3d_lines.frag.spv.h"
#include "Im3D/shaders/im3d_tris.vert.spv.h"
#include "Im3D/shaders/im3d_tris.frag.spv.h"
#include "Im3D/shaders/im3d_points.vert.spv.h"
#include "Im3D/shaders/im3d_points.frag.spv.h"
#include "Im3D/im3d_math.h"
#include "ImGui/imgui_internal.h"
#include "ImGui/imgui.h"

namespace lvk
{
    Vector<char> ToVector(const char* src, uint32_t count)
    {
        Vector<char> chars{};
        for (int i = 0; i < count; i++)
        {
            chars.push_back(src[i]);
        }
        return chars;
    }

    LvkIm3dState LoadIm3D(VulkanAPI& vk)
    {
        Vector<char> tris_vert_bin = ToVector(&im3d_tris_vert_spv_bin[0], im3d_tris_vert_spv_bin_SIZE);
        ShaderStage tris_vert = ShaderStage::Create(vk, tris_vert_bin, ShaderStage::Type::Vertex);
        Vector<char> tris_frag_bin = ToVector(&im3d_tris_frag_spv_bin[0], im3d_tris_frag_spv_bin_SIZE);
        ShaderStage tris_frag = ShaderStage::Create(vk, tris_frag_bin, ShaderStage::Type::Fragment);
        ShaderProgram tris_prog = ShaderProgram::Create(vk, tris_vert, tris_frag);

        Vector<char> lines_vert_bin = ToVector(&im3d_lines_vert_spv_bin[0], im3d_lines_vert_spv_bin_SIZE);
        ShaderStage lines_vert = ShaderStage::Create(vk, lines_vert_bin, ShaderStage::Type::Vertex);
        Vector<char> lines_frag_bin = ToVector(&im3d_lines_frag_spv_bin[0], im3d_lines_frag_spv_bin_SIZE);
        ShaderStage lines_frag = ShaderStage::Create(vk, lines_frag_bin, ShaderStage::Type::Fragment);
        ShaderProgram lines_prog = ShaderProgram::Create(vk, lines_vert, lines_frag);

        Vector<char> points_vert_bin = ToVector(&im3d_points_vert_spv_bin[0], im3d_points_vert_spv_bin_SIZE);
        ShaderStage points_vert = ShaderStage::Create(vk, points_vert_bin, ShaderStage::Type::Vertex);
        Vector<char> points_frag_bin = ToVector(&im3d_points_frag_spv_bin[0], im3d_points_frag_spv_bin_SIZE);
        ShaderStage points_frag = ShaderStage::Create(vk, points_frag_bin, ShaderStage::Type::Fragment);
        ShaderProgram points_prog = ShaderProgram::Create(vk, points_vert, points_frag);


        Vector<VertexDataPos4> vertexData =
        {
            VertexDataPos4{{-1.0f, -1.0f, 0.0f,1.0f}},
            VertexDataPos4{{1.0f, -1.0f, 0.0f, 1.0f}},
            VertexDataPos4{{1.0f,  1.0f, 0.0f, 1.0f}},
            VertexDataPos4{{-1.0f,  1.0f, 0.0f, 1.0f}}
        };


        VkBuffer vertexBuffer;
        VmaAllocation vertexBufferMemory;
        vk.CreateVertexBuffer<VertexDataPos4>(vertexData, vertexBuffer, vertexBufferMemory);

        return { tris_prog, points_prog, lines_prog, vertexBuffer, vertexBufferMemory };
    }

    LvkIm3dViewState AddIm3dForViewport(VulkanAPI& vk, LvkIm3dState& state, VkRenderPass renderPass, bool enableMSAA)
    {
        VkPipelineLayout tris_layout;
        VkPipeline tris_pipeline = vk.CreateRasterizationGraphicsPipeline(
            state.m_TriProg,
            Vector<VkVertexInputBindingDescription>{VertexDataPos4::GetBindingDescription() },
            VertexDataPos4::GetAttributeDescriptions(),
            renderPass,
            vk.m_SwapChainImageExtent.width, vk.m_SwapChainImageExtent.height,
            VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, enableMSAA,
            VK_COMPARE_OP_LESS, tris_layout);
        Material tris_material = Material::Create(vk, state.m_TriProg);

        VkPipelineLayout points_layout;
        VkPipeline points_pipeline = vk.CreateRasterizationGraphicsPipeline(
            state.m_PointsProg,
            Vector<VkVertexInputBindingDescription>{VertexDataPos4::GetBindingDescription() },
            VertexDataPos4::GetAttributeDescriptions(),
            renderPass,
            vk.m_SwapChainImageExtent.width, vk.m_SwapChainImageExtent.height,
            VK_POLYGON_MODE_POINT, VK_CULL_MODE_NONE, enableMSAA,
            VK_COMPARE_OP_LESS, points_layout);
        Material points_material = Material::Create(vk, state.m_PointsProg);


        VkPipelineLayout lines_layout;
        VkPipeline lines_pipeline = vk.CreateRasterizationGraphicsPipeline(
            state.m_LinesProg,
            Vector<VkVertexInputBindingDescription>{VertexDataPos4::GetBindingDescription() },
            VertexDataPos4::GetAttributeDescriptions(),
            renderPass,
            vk.m_SwapChainImageExtent.width, vk.m_SwapChainImageExtent.height,
            VK_POLYGON_MODE_LINE, VK_CULL_MODE_NONE, enableMSAA,
            VK_COMPARE_OP_LESS, lines_layout);
        Material lines_material = Material::Create(vk, state.m_LinesProg);


        return { tris_material, points_material, lines_material,
                tris_pipeline, points_pipeline, lines_pipeline,
                tris_layout, points_layout, lines_layout };
    }

    void FreeIm3dViewport(VulkanAPI& vk, LvkIm3dViewState& viewState)
    {
        viewState.m_TrisMaterial.Free(vk);
        viewState.m_LinesMaterial.Free(vk);
        viewState.m_PointsMaterial.Free(vk);

        vkDestroyPipelineLayout(vk.m_LogicalDevice, viewState.m_TrisPipelineLayout, nullptr);
        vkDestroyPipelineLayout(vk.m_LogicalDevice, viewState.m_LinesPipelineLayout, nullptr);
        vkDestroyPipelineLayout(vk.m_LogicalDevice, viewState.m_PointsPipelineLayout, nullptr);

        vkDestroyPipeline(vk.m_LogicalDevice, viewState.m_TrisPipeline, nullptr);
        vkDestroyPipeline(vk.m_LogicalDevice, viewState.m_LinesPipeline, nullptr);
        vkDestroyPipeline(vk.m_LogicalDevice, viewState.m_PointsPipeline, nullptr);
    }

    Im3d::Mat4 ToIm3D(const glm::mat4& _m) {
        Im3d::Mat4 m(1.0);
        for (int i = 0; i < 16; ++i)
        {
            m[i] = *(&(_m[0][0]) + i);
        }
        return m;
    }

    void FreeIm3d(VulkanAPI& vk, LvkIm3dState& state)
    {
        state.m_TriProg.Free(vk);
        state.m_LinesProg.Free(vk);
        state.m_PointsProg.Free(vk);

        vkDestroyBuffer(vk.m_LogicalDevice, state.m_ScreenQuad, nullptr);
        vmaFreeMemory(vk.m_Allocator, state.m_ScreenQuadMemory);
    }

    void DrawIm3d(VulkanAPI& vk, VkCommandBuffer& buffer, uint32_t frameIndex, LvkIm3dState& state, LvkIm3dViewState& viewState, glm::mat4 _viewProj, uint32_t width, uint32_t height, bool drawText)
    {
        auto& context = Im3d::GetContext();

        for (int i = 0; i < context.getDrawListCount(); i++)
        {
            auto drawList = &context.getDrawLists()[i];
            int primVertexCount;

            ShaderProgram* shader = nullptr;
            Material* mat = nullptr;
            VkPipelineLayout* pipelineLayout = nullptr;
            VkPipeline* pipeline = nullptr;
            switch (drawList->m_primType)
            {
            case Im3d::DrawPrimitiveType::DrawPrimitive_Triangles:
                shader = &state.m_TriProg;
                pipelineLayout = &viewState.m_TrisPipelineLayout;
                pipeline = &viewState.m_TrisPipeline;
                mat = &viewState.m_TrisMaterial;
                primVertexCount = 3;
                break;
            case Im3d::DrawPrimitiveType::DrawPrimitive_Lines:
                shader = &state.m_LinesProg;
                pipelineLayout = &viewState.m_LinesPipelineLayout;
                pipeline = &viewState.m_LinesPipeline;
                mat = &viewState.m_LinesMaterial;
                primVertexCount = 2;
                break;
            case Im3d::DrawPrimitiveType::DrawPrimitive_Points:
                shader = &state.m_PointsProg;
                pipelineLayout = &viewState.m_PointsPipelineLayout;
                pipeline = &viewState.m_PointsPipeline;
                mat = &viewState.m_PointsMaterial;
                primVertexCount = 1;
                break;
            default:
                spdlog::error("Im3d: unknown primitive type");
                return;
            }

            const int kMaxBufferSize = 64 * 1024; // assuming 64kb here but the application should check the implementation limit
            const int kPrimsPerPass = kMaxBufferSize / (sizeof(Im3d::VertexData) * primVertexCount);

            int remainingPrimCount = drawList->m_vertexCount / primVertexCount;
            const Im3d::VertexData* vertexData = drawList->m_vertexData;

            struct CameraUniformData
            {
                glm::mat4 ViewProj;
                glm::vec2 ViewPort;
            };

            CameraUniformData camData{ _viewProj, {width, height} };

            while (remainingPrimCount > 0)
            {
                int passPrimCount = remainingPrimCount < kPrimsPerPass ? remainingPrimCount : kPrimsPerPass;
                int passVertexCount = passPrimCount * primVertexCount;

                // access violation
                mat->SetBuffer(frameIndex, 0, 0, vertexData, passVertexCount * sizeof(Im3d::VertexData));
                mat->SetBuffer(frameIndex, 0, 1, camData);

                VkViewport viewport{};
                viewport.x = 0.0f;
                viewport.x = 0.0f;
                viewport.width = static_cast<float>(width);
                viewport.height = static_cast<float>(height);
                viewport.minDepth = 0.0f;
                viewport.maxDepth = 1.0f;

                VkRect2D scissor{};
                scissor.offset = { 0,0 };
                scissor.extent = VkExtent2D{
                    static_cast<uint32_t>(width) ,
                    static_cast<uint32_t>(height)
                };
                VkDeviceSize sizes[] = { 0 };
                vkCmdBindPipeline(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *pipeline);
                vkCmdSetViewport(buffer, 0, 1, &viewport);
                vkCmdSetScissor(buffer, 0, 1, &scissor);
                vkCmdBindVertexBuffers(buffer, 0, 1, &state.m_ScreenQuad, sizes);
                vkCmdBindDescriptorSets(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *pipelineLayout, 0, 1, &mat->m_DescriptorSets[0].m_Sets[frameIndex], 0, nullptr);

                vkCmdDraw(buffer, primVertexCount == 3 ? 3 : 4, passPrimCount, 0, 0);
                vertexData += passVertexCount;
                remainingPrimCount -= passPrimCount;
            }
        }
        if (!drawText)
        {
            return;
        }

        DrawIm3dTextListsImGui(context.getTextDrawLists(), context.getTextDrawListCount(),
            vk.m_SwapChainImageExtent.width, vk.m_SwapChainImageExtent.height, _viewProj);
    }

    void DrawIm3dTextListsImGui(const Im3d::TextDrawList _textDrawLists[], uint32_t _count, uint32_t width, uint32_t height, glm::mat4 _viewProj)
    {
        // Using ImGui here as a simple means of rendering text draw lists, however as with primitives the application is free to draw text in any conceivable  manner.

            // Invisible ImGui window which covers the screen.
        ImGui::PushStyleColor(ImGuiCol_WindowBg, IM_COL32_BLACK_TRANS);
        ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
        ImGui::SetNextWindowSize(ImVec2((float)width, (float)height));
        ImGui::Begin("Invisible", nullptr, 0
            | ImGuiWindowFlags_NoTitleBar
            | ImGuiWindowFlags_NoResize
            | ImGuiWindowFlags_NoScrollbar
            | ImGuiWindowFlags_NoInputs
            | ImGuiWindowFlags_NoSavedSettings
            | ImGuiWindowFlags_NoFocusOnAppearing
            | ImGuiWindowFlags_NoBringToFrontOnFocus
        );

        DrawIm3dTextListsImGuiAsChild(_textDrawLists, _count, width, height, _viewProj);

        ImGui::End();
        ImGui::PopStyleColor(1);
    }

    void DrawIm3dTextListsImGuiAsChild(const Im3d::TextDrawList _textDrawLists[], uint32_t _count, uint32_t width, uint32_t height, glm::mat4 _viewProj)
    {
        ImDrawList* imDrawList = ImGui::GetWindowDrawList();
        const Im3d::Mat4 viewProj = ToIm3D(_viewProj);
        for (uint32_t i = 0; i < _count; ++i)
        {
            const Im3d::TextDrawList& textDrawList = Im3d::GetTextDrawLists()[i];

            if (textDrawList.m_layerId == Im3d::MakeId("NamedLayer"))
            {
                // The application may group primitives into layers, which can be used to change the draw state (e.g. enable depth testing, use a different shader)
            }

            for (uint32_t j = 0; j < textDrawList.m_textDataCount; ++j)
            {
                const Im3d::TextData& textData = textDrawList.m_textData[j];
                if (textData.m_positionSize.w == 0.0f || textData.m_color.getA() == 0.0f)
                {
                    continue;
                }

                // Project world -> screen space.
                Im3d::Vec4 clip = viewProj * Im3d::Vec4(textData.m_positionSize.x, textData.m_positionSize.y, textData.m_positionSize.z, 1.0f);
                Im3d::Vec2 screen = Im3d::Vec2(clip.x / clip.w, clip.y / clip.w);

                // Cull text which falls offscreen. Note that this doesn't take into account text size but works well enough in practice.
                if (clip.w < 0.0f || screen.x >= 1.0f || screen.y >= 1.0f)
                {
                    continue;
                }

                // Pixel coordinates for the ImGuiWindow ImGui.
                screen = screen * Im3d::Vec2(0.5f) + Im3d::Vec2(0.5f);
                auto windowSize = ImGui::GetWindowSize();
                screen = screen * Im3d::Vec2{ windowSize.x, windowSize.y };

                // All text data is stored in a single buffer; each textData instance has an offset into this buffer.
                const char* text = textDrawList.m_textBuffer + textData.m_textBufferOffset;

                // Calculate the final text size in pixels to apply alignment flags correctly.
                ImGui::SetWindowFontScale(textData.m_positionSize.w); // NB no CalcTextSize API which takes a font/size directly...
                auto textSize = ImGui::CalcTextSize(text, text + textData.m_textLength);
                ImGui::SetWindowFontScale(1.0f);

                // Generate a pixel offset based on text flags.
                Im3d::Vec2 textOffset = Im3d::Vec2(-textSize.x * 0.5f, -textSize.y * 0.5f); // default to center
                if ((textData.m_flags & Im3d::TextFlags_AlignLeft) != 0)
                {
                    textOffset.x = -textSize.x;
                }
                else if ((textData.m_flags & Im3d::TextFlags_AlignRight) != 0)
                {
                    textOffset.x = 0.0f;
                }

                if ((textData.m_flags & Im3d::TextFlags_AlignTop) != 0)
                {
                    textOffset.y = -textSize.y;
                }
                else if ((textData.m_flags & Im3d::TextFlags_AlignBottom) != 0)
                {
                    textOffset.y = 0.0f;
                }
                ImFont* font = nullptr;
                // Add text to the window draw list.
                screen = screen + textOffset;
                ImVec2 windowPos = ImGui::GetWindowPos();
                ImVec2 imguiScreen{ windowPos.x + screen.x, windowPos.y + screen.y };
                imDrawList->AddText(
                    font,
                    (float)(textData.m_positionSize.w * ImGui::GetFontSize()),
                    imguiScreen,
                    textData.m_color.getABGR(),
                    text,
                    text + textData.m_textLength);
            }
        }
    }

}