#include "example-common.h"
#include "lvk/Shader.h"
#include "lvk/Material.h"
#include <algorithm>
#include "Im3D/shaders/im3d_lines.vert.spv.h"
#include "Im3D/shaders/im3d_lines.frag.spv.h"
#include "Im3D/shaders/im3d_tris.vert.spv.h"
#include "Im3D/shaders/im3d_tris.frag.spv.h"
#include "Im3D/shaders/im3d_points.vert.spv.h"
#include "Im3D/shaders/im3d_points.frag.spv.h"
#include "Im3D/im3d.h"

using namespace lvk;



#define NUM_LIGHTS 512
using DeferredLightData = FrameLightDataT<NUM_LIGHTS>;

struct RenderItem
{
    MeshEx      m_Mesh;
    Material    m_Material;
};

struct RenderModel
{
    Model m_Original;
    Vector<RenderItem> m_RenderItems;
    void Free(VulkanAPI& vk)
    {
        for (auto& item : m_RenderItems)
        {
            item.m_Material.Free(vk);
            FreeMesh(vk, item.m_Mesh);
        }

        for (auto& mat : m_Original.m_Materials)
        {
            mat.m_Diffuse.Free(vk);
        }
        m_RenderItems.clear();
    }
};

struct Camera
{
    glm::vec3 Position;
    glm::vec3 Rotation;
    float FOV = 90.0f;
    float Near = 0.001f, Far = 300.0f;
};

static Transform g_Transform;
static Camera g_Camera;
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


    Vector<Im3d::Vec4> vertexData =
    {
        Im3d::Vec4(-1.0f, -1.0f, 0.0f, 1.0f),
        Im3d::Vec4(1.0f, -1.0f, 0.0f, 1.0f),
        Im3d::Vec4(-1.0f,  1.0f, 0.0f, 1.0f),
        Im3d::Vec4(1.0f,  1.0f, 0.0f, 1.0f)
    };


    VkBuffer vertexBuffer;
    VmaAllocation vertexBufferMemory;
    vk.CreateVertexBuffer<Im3d::Vec4>(vertexData, vertexBuffer, vertexBufferMemory);

    return { tris_prog, points_prog, lines_prog, vertexBuffer, vertexBufferMemory };
}

LvkIm3dViewState AddIm3dForViewport(VulkanAPI& vk, LvkIm3dState& state, VkRenderPass renderPass)
{
    VkPipelineLayout tris_layout;
    VkPipeline tris_pipeline = vk.CreateRasterizationGraphicsPipeline(
        state.m_TriProg.m_Stages[0].m_StageBinary, state.m_TriProg.m_Stages[1].m_StageBinary,
        state.m_TriProg.m_DescriptorSetLayout,
        Vector<VkVertexInputBindingDescription>{VertexDataPos4::GetBindingDescription() },
        VertexDataPos4::GetAttributeDescriptions(),
        vk.m_SwapchainImageRenderPass,
        vk.m_SwapChainImageExtent.width, vk.m_SwapChainImageExtent.height,
        VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, false,
        VK_COMPARE_OP_LESS, tris_layout);
    Material tris_material = Material::Create(vk, state.m_TriProg);

    VkPipelineLayout points_layout;
    VkPipeline points_pipeline = vk.CreateRasterizationGraphicsPipeline(
        state.m_PointsProg.m_Stages[0].m_StageBinary, state.m_PointsProg.m_Stages[1].m_StageBinary,
        state.m_PointsProg.m_DescriptorSetLayout,
        Vector<VkVertexInputBindingDescription>{VertexDataPos4::GetBindingDescription() },
        VertexDataPos4::GetAttributeDescriptions(),
        vk.m_SwapchainImageRenderPass,
        vk.m_SwapChainImageExtent.width, vk.m_SwapChainImageExtent.height,
        VK_POLYGON_MODE_POINT, VK_CULL_MODE_NONE, false,
        VK_COMPARE_OP_LESS, points_layout);
    Material points_material = Material::Create(vk, state.m_PointsProg);


    VkPipelineLayout lines_layout;
    VkPipeline lines_pipeline = vk.CreateRasterizationGraphicsPipeline(
        state.m_LinesProg.m_Stages[0].m_StageBinary, state.m_LinesProg.m_Stages[1].m_StageBinary,
        state.m_LinesProg.m_DescriptorSetLayout,
        Vector<VkVertexInputBindingDescription>{VertexDataPos4::GetBindingDescription() },
        VertexDataPos4::GetAttributeDescriptions(),
        vk.m_SwapchainImageRenderPass,
        vk.m_SwapChainImageExtent.width, vk.m_SwapChainImageExtent.height,
        VK_POLYGON_MODE_LINE, VK_CULL_MODE_NONE, false,
        VK_COMPARE_OP_LESS, lines_layout);
    Material lines_material = Material::Create(vk, state.m_LinesProg);


    return {tris_material, points_material, lines_material,  
            tris_pipeline, points_pipeline, lines_pipeline,
            tris_layout, points_layout, lines_layout };
}

void DrawIm3d(VulkanAPI& vk, VkCommandBuffer& buffer, uint32_t frameIndex, LvkIm3dState& state, LvkIm3dViewState& viewState)
{
    auto& context = Im3d::GetContext();
    // TODO: Pass in a camera
    glm::quat qPitch = glm::angleAxis(glm::radians(-g_Camera.Rotation.x), glm::vec3(1, 0, 0));
    glm::quat qYaw = glm::angleAxis(glm::radians(g_Camera.Rotation.y), glm::vec3(0, 1, 0));
    // omit roll
    glm::quat Rotation = qPitch * qYaw;
    Rotation = glm::normalize(Rotation);
    glm::mat4 rotate = glm::mat4_cast(Rotation);
    glm::mat4 translate = glm::mat4(1.0f);
    translate = glm::translate(translate, -g_Camera.Position);

    glm::mat4 View = rotate * translate;
    
    glm::mat4 Proj = glm::perspective(glm::radians(45.0f), vk.m_SwapChainImageExtent.width / (float)vk.m_SwapChainImageExtent.height, 0.1f, 1000.0f);
    Proj[1][1] *= -1;
    

    glm::mat4 viewProj = Proj * View;

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

        CameraUniformData camData{ viewProj, {vk.m_SwapChainImageExtent.width, vk.m_SwapChainImageExtent.height} };

        while (remainingPrimCount > 0)
        {
            int passPrimCount = remainingPrimCount < kPrimsPerPass ? remainingPrimCount : kPrimsPerPass;
            int passVertexCount = passPrimCount * primVertexCount;

            // set appropriate buffers in material
            // might need support for sending data when specifying where the data is cpu side
            // e.g. someway into an existing array, need a T* to array elem and uint& count,
            //glAssert(glBindBuffer(GL_UNIFORM_BUFFER, g_Im3dUniformBuffer));
            //glAssert(glBufferData(GL_UNIFORM_BUFFER, (GLsizeiptr)passVertexCount * sizeof(Im3d::VertexData), (GLvoid*)vertexData, GL_DYNAMIC_DRAW));

            // access violation
            mat->SetBuffer(frameIndex, 0, 0, vertexData, passVertexCount * sizeof(Im3d::VertexData));
            mat->SetBuffer(frameIndex, 0, 1, camData);
            //// instanced draw call, 1 instance per prim
            //glAssert(glBindBufferBase(GL_UNIFORM_BUFFER, 0, g_Im3dUniformBuffer));
            //glDrawArraysInstanced(prim, 0, prim == GL_TRIANGLES ? 3 : 4, passPrimCount); // for triangles just use the first 3 verts of the strip
            // mode=prim, starting index, number of indices to be rendered, the number of instances of the specified range of indices to be rendered

            // get correct pipeline and bind

            VkViewport viewport{};
            viewport.x = 0.0f;
            viewport.x = 0.0f;
            viewport.width = static_cast<float>(vk.m_SwapChainImageExtent.width);
            viewport.height = static_cast<float>(vk.m_SwapChainImageExtent.height);
            viewport.minDepth = 0.0f;
            viewport.maxDepth = 1.0f;

            VkRect2D scissor{};
            scissor.offset = { 0,0 };
            scissor.extent = VkExtent2D{
                static_cast<uint32_t>(vk.m_SwapChainImageExtent.width) ,
                static_cast<uint32_t>(vk.m_SwapChainImageExtent.height)
            };
            VkDeviceSize sizes[] = { 0 };
            vkCmdBindPipeline(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *pipeline);
            vkCmdSetViewport(buffer, 0, 1, &viewport);
            vkCmdSetScissor(buffer, 0, 1, &scissor);
            vkCmdBindVertexBuffers(buffer, 0, 1, &state.m_ScreenQuad, sizes);
            vkCmdBindDescriptorSets(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *pipelineLayout, 0, 1, &mat->m_DescriptorSets[0].m_Sets[frameIndex], 0, nullptr);

            vkCmdDrawIndexed(buffer, primVertexCount == 3 ? 3 : 4, passPrimCount, 0, 0, 0);

            vertexData += passVertexCount;
            remainingPrimCount -= passPrimCount;
        }
    }
}

void RecordCommandBuffersV2(VulkanAPI_SDL& vk,
    VkPipeline& gbufferPipeline, VkPipelineLayout& gbufferPipelineLayout, VkRenderPass gbufferRenderPass, Vector<VkFramebuffer>& gbufferFramebuffers,
    VkPipeline& lightingPassPipeline, VkPipelineLayout& lightingPassPipelineLayout, VkRenderPass lightingPassRenderPass, Material& lightPassMaterial, Vector<VkFramebuffer>& lightingPassFramebuffers,
    RenderModel& model, Mesh& screenQuad, LvkIm3dState& im3dState, LvkIm3dViewState& im3dViewState)
{
    vk.RecordGraphicsCommands([&](VkCommandBuffer& commandBuffer, uint32_t frameIndex) {
        {
            Array<VkClearValue, 4> clearValues{};
            clearValues[0].color = { {0.0f, 0.0f, 0.0f, 1.0f} };
            clearValues[1].color = { {0.0f, 0.0f, 0.0f, 1.0f} };
            clearValues[2].color = { {0.0f, 0.0f, 0.0f, 1.0f} };
            clearValues[3].depthStencil = { 1.0f, 0 };

            VkRenderPassBeginInfo renderPassInfo{};
            renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            renderPassInfo.renderPass = gbufferRenderPass;
            renderPassInfo.framebuffer = gbufferFramebuffers[frameIndex];
            renderPassInfo.renderArea.offset = { 0,0 };
            renderPassInfo.renderArea.extent = vk.m_SwapChainImageExtent;

            renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
            renderPassInfo.pClearValues = clearValues.data();

            vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, gbufferPipeline);
            VkViewport viewport{};
            viewport.x = 0.0f;
            viewport.x = 0.0f;
            viewport.width = static_cast<float>(vk.m_SwapChainImageExtent.width);
            viewport.height = static_cast<float>(vk.m_SwapChainImageExtent.height);
            viewport.minDepth = 0.0f;
            viewport.maxDepth = 1.0f;

            VkRect2D scissor{};
            scissor.offset = { 0,0 };
            scissor.extent = VkExtent2D{
                static_cast<uint32_t>(vk.m_SwapChainImageExtent.width) ,
                static_cast<uint32_t>(vk.m_SwapChainImageExtent.height)
            };
            vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
            vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

            for (int i = 0; i < model.m_RenderItems.size(); i++)
            {
                MeshEx& mesh = model.m_RenderItems[i].m_Mesh;
                VkBuffer vertexBuffers[]{ mesh.m_VertexBuffer };
                VkDeviceSize sizes[] = { 0 };


                vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, sizes);
                vkCmdBindIndexBuffer(commandBuffer, mesh.m_IndexBuffer, 0, VK_INDEX_TYPE_UINT32);
                vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, gbufferPipelineLayout, 0, 1, &model.m_RenderItems[i].m_Material.m_DescriptorSets[0].m_Sets[frameIndex], 0, nullptr);
                vkCmdDrawIndexed(commandBuffer, mesh.m_IndexCount, 1, 0, 0, 0);
            }
            vkCmdEndRenderPass(commandBuffer);
        }

        Array<VkClearValue, 2> clearValues{};
        clearValues[0].color = { {0.0f, 0.0f, 0.0f, 1.0f} };
        clearValues[1].depthStencil = { 1.0f, 0 };

        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = vk.m_SwapchainImageRenderPass;
        renderPassInfo.framebuffer = vk.m_SwapChainFramebuffers[frameIndex];
        renderPassInfo.renderArea.offset = { 0,0 };
        renderPassInfo.renderArea.extent = vk.m_SwapChainImageExtent;

        renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
        renderPassInfo.pClearValues = clearValues.data();

        vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, lightingPassPipeline);
        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.x = 0.0f;
        viewport.width = static_cast<float>(vk.m_SwapChainImageExtent.width);
        viewport.height = static_cast<float>(vk.m_SwapChainImageExtent.height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        VkRect2D scissor{};
        scissor.offset = { 0,0 };
        scissor.extent = VkExtent2D{
            static_cast<uint32_t>(vk.m_SwapChainImageExtent.width) ,
            static_cast<uint32_t>(vk.m_SwapChainImageExtent.height)
        };
        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
        VkDeviceSize sizes[] = { 0 };
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, &screenQuad.m_VertexBuffer, sizes);
        vkCmdBindIndexBuffer(commandBuffer, screenQuad.m_IndexBuffer, 0, VK_INDEX_TYPE_UINT32);
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, lightingPassPipelineLayout, 0, 1, &lightPassMaterial.m_DescriptorSets[0].m_Sets[frameIndex], 0, nullptr);
        vkCmdDrawIndexed(commandBuffer, screenQuad.m_IndexCount, 1, 0, 0, 0);

        DrawIm3d(vk, commandBuffer, frameIndex, im3dState, im3dViewState);
        vkCmdEndRenderPass(commandBuffer);
        });
}

void UpdateUniformBuffer(VulkanAPI_SDL& vk, Material& renderItemMaterial, Material& lightPassMaterial, DeferredLightData& lightDataCpu)
{
    static auto startTime = std::chrono::high_resolution_clock::now();

    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

    MvpData ubo{};
    ubo.Model = g_Transform.to_mat4();

    glm::quat qPitch = glm::angleAxis(glm::radians(-g_Camera.Rotation.x), glm::vec3(1, 0, 0));
    glm::quat qYaw = glm::angleAxis(glm::radians(g_Camera.Rotation.y), glm::vec3(0, 1, 0));
    // omit roll
    glm::quat Rotation = qPitch * qYaw;
    Rotation = glm::normalize(Rotation);
    glm::mat4 rotate = glm::mat4_cast(Rotation);
    glm::mat4 translate = glm::mat4(1.0f);
    translate = glm::translate(translate, -g_Camera.Position);

    ubo.View = rotate * translate;
    if (vk.m_SwapChainImageExtent.width > 0 || vk.m_SwapChainImageExtent.height)
    {
        ubo.Proj = glm::perspective(glm::radians(45.0f), vk.m_SwapChainImageExtent.width / (float)vk.m_SwapChainImageExtent.height, 0.1f, 1000.0f);
        ubo.Proj[1][1] *= -1;
    }
    renderItemMaterial.SetBuffer(vk.GetFrameIndex(), 0, 0, ubo);
    lightPassMaterial.SetBuffer(vk.GetFrameIndex(), 0, 0, ubo);
    lightPassMaterial.SetBuffer(vk.GetFrameIndex(), 0, 4, lightDataCpu);
}

RenderModel CreateRenderModelGbuffer(VulkanAPI& vk, const String& modelPath, ShaderProgram& shader)
{
    Model model;
    LoadModelAssimp(vk, model, modelPath, true);

    RenderModel renderModel{};
    renderModel.m_Original = model;
    for (auto& mesh : model.m_Meshes)
    {
        RenderItem item{};
        int materialIndex = std::min(mesh.m_MaterialIndex, (uint32_t)model.m_Materials.size() - 1);
        item.m_Mesh = mesh;
        item.m_Material = Material::Create(vk, shader);

        MaterialEx& material = model.m_Materials[mesh.m_MaterialIndex];
        item.m_Material.SetSampler(vk, "texSampler", material.m_Diffuse.m_ImageView, material.m_Diffuse.m_Sampler);
        renderModel.m_RenderItems.push_back(item);
    }

    return renderModel;
}

void OnImGui(VulkanAPI& vk, DeferredLightData& lightDataCpu)
{
    if (ImGui::Begin("Debug"))
    {
        ImGui::Text("Frametime: %f", (1.0 / vk.m_DeltaTime));
        ImGui::Separator();
        ImGui::DragFloat3("Position", &g_Transform.m_Position[0]);
        ImGui::DragFloat3("Euler Rotation", &g_Transform.m_Rotation[0]);
        ImGui::DragFloat3("Scale", &g_Transform.m_Scale[0]);

        ImGui::Separator();

        ImGui::DragFloat3("Cam Position", &g_Camera.Position[0]);
        ImGui::DragFloat3("Cam Euler Rotation", &g_Camera.Rotation[0]);
    }
    ImGui::End();

    if (ImGui::Begin("Menu"))
    {
        ImGui::Text("Frametime: %f", (1.0 / vk.m_DeltaTime));
        ImGui::DragFloat3("Directional Light Dir", &lightDataCpu.m_DirectionalLight.Direction[0]);
        ImGui::DragFloat4("Directional Light Colour", &lightDataCpu.m_DirectionalLight.Colour[0]);
        ImGui::DragFloat4("Directional Light Ambient Colour", &lightDataCpu.m_DirectionalLight.Ambient[0]);

        if (ImGui::TreeNode("Point Lights"))
        {
            for (int i = 0; i < NUM_LIGHTS; i++)
            {
                ImGui::PushID(i);
                if (ImGui::TreeNode("Point Light"))
                {
                    ImGui::DragFloat3("Position", &lightDataCpu.m_PointLights[i].PositionRadius[0]);
                    ImGui::DragFloat("Radius", &lightDataCpu.m_PointLights[i].PositionRadius[3]);
                    ImGui::DragFloat4("Colour", &lightDataCpu.m_PointLights[i].Colour[0]);
                    ImGui::DragFloat4("Ambient Colour", &lightDataCpu.m_PointLights[i].Ambient[0]);

                    ImGui::TreePop();
                }

                ImGui::PopID();
            }
            ImGui::TreePop();
        }

        if (ImGui::TreeNode("Spot Lights"))
        {
            for (int i = 0; i < NUM_LIGHTS; i++)
            {
                ImGui::PushID(NUM_LIGHTS + i);
                if (ImGui::TreeNode("Spot Light"))
                {
                    ImGui::DragFloat3("Position", &lightDataCpu.m_SpotLights[i].PositionRadius[0]);
                    ImGui::DragFloat("Radius", &lightDataCpu.m_SpotLights[i].PositionRadius[3]);
                    ImGui::DragFloat3("Direction", &lightDataCpu.m_SpotLights[i].DirectionAngle[0]);
                    ImGui::DragFloat("Angle", &lightDataCpu.m_SpotLights[i].DirectionAngle[3]);
                    ImGui::DragFloat4("Colour", &lightDataCpu.m_SpotLights[i].Colour[0]);
                    ImGui::DragFloat4("Ambient Colour", &lightDataCpu.m_SpotLights[i].Ambient[0]);

                    ImGui::TreePop();
                }

                ImGui::PopID();
            }
            ImGui::TreePop();
        }
    }
    ImGui::End();
}

void OnIm3D()
{
    static Im3d::Mat4 transform(1.0f);
    Im3d::PushMatrix(transform);
    Im3d::PushDrawState();
    Im3d::SetSize(2.0f);
    Im3d::SetColor(Im3d::Color_White);
    Im3d::DrawCircle({ 0.0f, 1.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, 0.5f, 32);
    Im3d::PopDrawState();
    Im3d::PopMatrix();
}

int main() {
    VulkanAPI_SDL vk;
    bool enableMSAA = false;
    vk.Start(1280, 720, enableMSAA);
    auto im3dState = LoadIm3D(vk);
    auto im3dViewState = AddIm3dForViewport(vk, im3dState, vk.m_SwapchainImageRenderPass);

    DeferredLightData lightDataCpu{};
    FillExampleLightData(lightDataCpu);

    ShaderProgram gbufferProg = ShaderProgram::Create(vk, "shaders/gbuffer.vert.spv", "shaders/gbuffer.frag.spv");
    ShaderProgram lightPassProg = ShaderProgram::Create(vk, "shaders/lights.vert.spv", "shaders/lights.frag.spv");
    Material lightPassMat = Material::Create(vk, lightPassProg);

    Framebuffer gbuffer{};
    gbuffer.m_Width = vk.m_SwapChainImageExtent.width;
    gbuffer.m_Height = vk.m_SwapChainImageExtent.height;
    gbuffer.AddColourAttachment(vk, ResolutionScale::Full, 1, VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_ASPECT_COLOR_BIT);
    gbuffer.AddColourAttachment(vk, ResolutionScale::Full, 1, VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_ASPECT_COLOR_BIT);
    gbuffer.AddColourAttachment(vk, ResolutionScale::Full, 1, VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_ASPECT_COLOR_BIT);
    gbuffer.AddDepthAttachment(vk, ResolutionScale::Full, 1, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_ASPECT_DEPTH_BIT);
    gbuffer.Build(vk);

    // todo: idk why this works, we need to respect each image in the swap chain, not just the 0th element
    lightPassMat.SetColourAttachment(vk, "positionBufferSampler", gbuffer, 1);
    lightPassMat.SetColourAttachment(vk, "normalBufferSampler", gbuffer, 2);
    lightPassMat.SetColourAttachment(vk, "colourBufferSampler", gbuffer, 0);

    // create gbuffer pipeline
    VkPipelineLayout gbufferPipelineLayout;
    VkPipeline gbufferPipeline = vk.CreateRasterizationGraphicsPipeline(
        gbufferProg.m_Stages[0].m_StageBinary, gbufferProg.m_Stages[1].m_StageBinary,
        gbufferProg.m_DescriptorSetLayout,
        Vector<VkVertexInputBindingDescription>{VertexDataPosNormalUv::GetBindingDescription() },
        VertexDataPosNormalUv::GetAttributeDescriptions(),
        gbuffer.m_RenderPass,
        vk.m_SwapChainImageExtent.width, vk.m_SwapChainImageExtent.height,
        VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, false,
        VK_COMPARE_OP_LESS, gbufferPipelineLayout, 3);

    // create present graphics pipeline
    // Pipeline stage?
    VkPipelineLayout lightPassPipelineLayout;
    VkPipeline pipeline = vk.CreateRasterizationGraphicsPipeline(
        lightPassProg.m_Stages[0].m_StageBinary, lightPassProg.m_Stages[1].m_StageBinary,
        lightPassProg.m_DescriptorSetLayout,
        Vector<VkVertexInputBindingDescription>{VertexDataPosUv::GetBindingDescription() },
        VertexDataPosUv::GetAttributeDescriptions(),
        vk.m_SwapchainImageRenderPass,
        vk.m_SwapChainImageExtent.width, vk.m_SwapChainImageExtent.height,
        VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, enableMSAA,
        VK_COMPARE_OP_LESS, lightPassPipelineLayout);

    // create vertex and index buffer
    // allocate materials instead of raw buffers etc.
    RenderModel m = CreateRenderModelGbuffer(vk, "assets/sponza/sponza.gltf", gbufferProg);

    while (vk.ShouldRun())
    {
        vk.PreFrame();

        Im3d::NewFrame();

        for (auto& item : m.m_RenderItems)
        {
            UpdateUniformBuffer(vk, item.m_Material, lightPassMat, lightDataCpu);
        }

        OnIm3D();

        Im3d::EndFrame();

        RecordCommandBuffersV2(vk,
            gbufferPipeline, gbufferPipelineLayout, gbuffer.m_RenderPass, gbuffer.m_SwapchainFramebuffers,
            pipeline, lightPassPipelineLayout, vk.m_SwapchainImageRenderPass, lightPassMat, vk.m_SwapChainFramebuffers,
            m, *Mesh::g_ScreenSpaceQuad, im3dState, im3dViewState);

        OnImGui(vk, lightDataCpu);

        vk.PostFrame();
    }

    lightPassMat.Free(vk);
    gbuffer.Free(vk);
    m.Free(vk);

    vkDestroyRenderPass(vk.m_LogicalDevice, gbuffer.m_RenderPass, nullptr);

    vkDestroyDescriptorSetLayout(vk.m_LogicalDevice, gbufferProg.m_DescriptorSetLayout, nullptr);
    vkDestroyDescriptorSetLayout(vk.m_LogicalDevice, lightPassProg.m_DescriptorSetLayout, nullptr);

    vkDestroyPipelineLayout(vk.m_LogicalDevice, gbufferPipelineLayout, nullptr);
    vkDestroyPipeline(vk.m_LogicalDevice, gbufferPipeline, nullptr);
    vkDestroyPipelineLayout(vk.m_LogicalDevice, lightPassPipelineLayout, nullptr);
    vkDestroyPipeline(vk.m_LogicalDevice, pipeline, nullptr);

    return 0;
}
