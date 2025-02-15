#include "example-common.h"
#include "lvk/Shader.h"
#include "lvk/Material.h"
#include "lvk/Pipeline.h"
#include <algorithm>
#include "Im3D/im3d_lvk.h"
#include "ImGui/lvk_extensions.h"
using namespace lvk;
struct View
{
    Camera      m_Camera;
    VkExtent2D  m_CurrentResolution;
};

using VkRecordCommandCallback = std::function<void(VkCommandBuffer&, uint32_t, View&, Mesh&, Vector<Renderable>)>;

struct ViewData
{
    PipelineT<VkRecordCommandCallback>    m_Pipeline;
    View        m_View;
    Mesh        m_ViewQuad;
};

PipelineT<VkRecordCommandCallback> CreateViewPipeline(VkAPI & vk, LvkIm3dState& im3dState, ShaderProgram& gbufferProg, ShaderProgram& lightPassProg)
{
    PipelineT<VkRecordCommandCallback> p{};
    auto* gbuffer = p.AddFramebuffer(vk);
    gbuffer->AddColourAttachment(vk, ResolutionScale::Full, 1, VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_ASPECT_COLOR_BIT);
    gbuffer->AddColourAttachment(vk, ResolutionScale::Full, 1, VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_ASPECT_COLOR_BIT);
    gbuffer->AddColourAttachment(vk, ResolutionScale::Full, 1, VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_ASPECT_COLOR_BIT);
    gbuffer->AddDepthAttachment(vk, ResolutionScale::Full, 1, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_ASPECT_DEPTH_BIT);
    gbuffer->Build(vk);

    auto* lightPassImage = p.AddFramebuffer(vk);
    lightPassImage->AddColourAttachment(vk, ResolutionScale::Full, 1, VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_ASPECT_COLOR_BIT);
    lightPassImage->Build(vk);

    auto* lightPassMat = p.AddMaterial(vk, lightPassProg);
    lightPassMat->SetColourAttachment(vk, "positionBufferSampler", *gbuffer, 1);
    lightPassMat->SetColourAttachment(vk, "normalBufferSampler", *gbuffer, 2);
    lightPassMat->SetColourAttachment(vk, "colourBufferSampler", *gbuffer, 0);

    p.SetOutputFramebuffer(lightPassImage);


    // create gbuffer pipeline
    VkPipelineLayout gbufferPipelineLayout;
    auto gbufferBindingDescriptions = Vector<VkVertexInputBindingDescription>{VertexDataPosNormalUv::GetBindingDescription() };
    auto gbufferAttributeDescriptions = VertexDataPosNormalUv::GetAttributeDescriptions();
    VkPipeline gbufferPipeline = vk.CreateRasterPipeline(
        gbufferProg, gbufferBindingDescriptions, gbufferAttributeDescriptions,
        gbuffer->m_RenderPass, vk.m_SwapChainImageExtent.width,
        vk.m_SwapChainImageExtent.height, VK_POLYGON_MODE_FILL,
        VK_CULL_MODE_BACK_BIT, false, VK_COMPARE_OP_LESS, gbufferPipelineLayout,
        3);

    // create present graphics pipeline
    // Pipeline stage?
    VkPipelineLayout lightPassPipelineLayout;
    auto lightPassBindingDescriptions = Vector<VkVertexInputBindingDescription>{VertexDataPosUv::GetBindingDescription() };
    auto lightPassAttributeDescriptions = VertexDataPosUv::GetAttributeDescriptions();
    VkPipeline lightPassPipeline = vk.CreateRasterPipeline(
        lightPassProg, lightPassBindingDescriptions,
        lightPassAttributeDescriptions, lightPassImage->m_RenderPass,
        vk.m_SwapChainImageExtent.width, vk.m_SwapChainImageExtent.height,
        VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, false, VK_COMPARE_OP_LESS,
        lightPassPipelineLayout);


    VkPipelineData* gbufferPipelineData = p.AddPipeline(vk, gbufferPipeline, gbufferPipelineLayout );
    VkPipelineData* lightPassPipelineData = p.AddPipeline(vk, lightPassPipeline, lightPassPipelineLayout);
    auto* im3dViewState = p.AddIm3d(vk, im3dState);

    p.RecordCommands(
        [
            gbuffer, gbufferPipelineData, 
            lightPassImage, lightPassPipelineData, lightPassMat, 
            &vk, &im3dState, im3dViewState
        ]
            (auto& commandBuffer, uint32_t frameIndex, View& view, Mesh& screenQuad, Vector<Renderable> renderables)
        {
            static int frameCount = 0;
            // prepare push constant data
            struct PCViewData
            {
                glm::mat4 view;
                glm::mat4 proj;
            };

            PCViewData pcData{ view.m_Camera.View, view.m_Camera.Proj };
            VkExtent2D viewExtent = view.m_CurrentResolution;

            // update screen quad
            {
                auto max = vk.GetMaxFramebufferResolution();
                float w = static_cast<float>((float)viewExtent.width / (float)max.width);
                float h = static_cast<float>((float)viewExtent.height / (float)max.height);

                Vector<VertexDataPosUv> newScreenQuadData = {
                    { { -1.0f, -1.0f , 0.0f}, { 0.0f, 0.0f } },
                    { {1.0f, -1.0f, 0.0f}, {w, 0.0f} },
                    { {1.0f, 1.0f, 0.0f}, {w, h} },
                    { {-1.0f, 1.0f, 0.0f}, {0.0f, h} }
                };

                vkCmdUpdateBuffer(commandBuffer, screenQuad.m_VertexBuffer, 0, 4 * sizeof(VertexDataPosUv), &newScreenQuadData[0]);
            }
            // Gbuffer pass
            {
                Array<VkClearValue, 4> clearValues{};
                clearValues[0].color = { {0.0f, 0.0f, 0.0f, 1.0f} };
                clearValues[1].color = { {0.0f, 0.0f, 0.0f, 1.0f} };
                clearValues[2].color = { {0.0f, 0.0f, 0.0f, 1.0f} };
                clearValues[3].depthStencil = { 1.0f, 0 };

                VkRenderPassBeginInfo renderPassInfo{};
                renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
                renderPassInfo.renderPass = gbuffer->m_RenderPass;
                renderPassInfo.framebuffer = gbuffer->m_SwapchainFramebuffers[frameIndex];
                renderPassInfo.renderArea.offset = { 0,0 };
                renderPassInfo.renderArea.extent = viewExtent;

                renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
                renderPassInfo.pClearValues = clearValues.data();

                vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
                vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, gbufferPipelineData->m_Pipeline);
                VkViewport viewport{};
                viewport.x = 0.0f;
                viewport.x = 0.0f;
                viewport.width = static_cast<float>(viewExtent.width);
                viewport.height = static_cast<float>(viewExtent.height);
                viewport.minDepth = 0.0f;
                viewport.maxDepth = 1.0f;

                VkRect2D scissor{};
                scissor.offset = { 0,0 };
                scissor.extent = VkExtent2D{
                    static_cast<uint32_t>(viewExtent.width) ,
                    static_cast<uint32_t>(viewExtent.height)
                };

                vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
                vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
                vkCmdPushConstants(commandBuffer, gbufferPipelineData->m_PipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PCViewData), &pcData);

                for (auto& renderable : renderables)
                {
                    renderable.RecordGraphicsCommands(commandBuffer);
                }
                vkCmdEndRenderPass(commandBuffer);
            }
            // Light pass
            {
                Array<VkClearValue, 1> clearValues{};
                clearValues[0].color = { {0.0f, 0.0f, 0.0f, 1.0f} };

                VkRenderPassBeginInfo renderPassInfo{};
                renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
                renderPassInfo.renderPass = lightPassImage->m_RenderPass;
                renderPassInfo.framebuffer = lightPassImage->m_SwapchainFramebuffers[frameIndex];
                renderPassInfo.renderArea.offset = { 0,0 };
                renderPassInfo.renderArea.extent = viewExtent;

                renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
                renderPassInfo.pClearValues = clearValues.data();

                vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

                vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, lightPassPipelineData->m_Pipeline);
                VkViewport viewport{};
                viewport.x = 0.0f;
                viewport.x = 0.0f;
                viewport.width = static_cast<float>(viewExtent.width);
                viewport.height = static_cast<float>(viewExtent.height);
                viewport.minDepth = 0.0f;
                viewport.maxDepth = 1.0f;

                VkRect2D scissor{};
                scissor.offset = { 0,0 };
                scissor.extent = VkExtent2D{
                    static_cast<uint32_t>(viewExtent.width) ,
                    static_cast<uint32_t>(viewExtent.height)
                };

                // issue with lighting pass is that uvs are just 0,0 -> 1,1
                // meaning the entire buffer will be resampled
                vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
                vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
                vkCmdPushConstants(commandBuffer, lightPassPipelineData->m_PipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PCViewData), &pcData);
                VkDeviceSize sizes[] = { 0 };
                vkCmdBindVertexBuffers(commandBuffer, 0, 1, &screenQuad.m_VertexBuffer, sizes);
                vkCmdBindIndexBuffer(commandBuffer, screenQuad.m_IndexBuffer, 0, VK_INDEX_TYPE_UINT32);
                vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, lightPassPipelineData->m_PipelineLayout, 0, 1, &lightPassMat->m_DescriptorSets[0].m_Sets[frameIndex], 0, nullptr);
                vkCmdDrawIndexed(commandBuffer, screenQuad.m_IndexCount, 1, 0, 0, 0);
                DrawIm3d(vk, commandBuffer, frameIndex, im3dState, *im3dViewState, view.m_Camera.Proj * view.m_Camera.View, viewExtent.width, viewExtent.height);
                vkCmdEndRenderPass(commandBuffer);
            }
        }
    );
    return p;
}

ViewData CreateView(VkAPI & vk, LvkIm3dState im3dState, ShaderProgram gbufferProg, ShaderProgram lightPassProg)
{
    PipelineT<VkRecordCommandCallback> pipeline = CreateViewPipeline(vk, im3dState, gbufferProg, lightPassProg);

    static Vector<VertexDataPosUv> screenQuadVerts = {
                    { { -1.0f, -1.0f , 0.0f}, { 0.0f, 0.0f } },
                    { {1.0f, -1.0f, 0.0f}, {1.0, 0.0f} },
                    { {1.0f, 1.0f, 0.0f}, {1.0, 1.0} },
                    { {-1.0f, 1.0f, 0.0f}, {0.0f, 1.0} }
    };

    static lvk::Vector<uint32_t> screenQuadIndices = {
    0, 1, 2, 2, 3, 0
    };

    VkBuffer vertBuffer;
    VmaAllocation vertAlloc;
    vk.CreateVertexBuffer<VertexDataPosUv>(screenQuadVerts, vertBuffer, vertAlloc);

    VkBuffer indexBuffer;
    VmaAllocation indexAlloc;
    vk.CreateIndexBuffer(screenQuadIndices, indexBuffer, indexAlloc);

    Mesh screenQuad{ vertBuffer, vertAlloc, indexBuffer, indexAlloc, 6 };

    return { pipeline, {{}, {1920, 1080}}, screenQuad };
}

static Transform g_Transform;

void UpdateRenderItemUniformBuffer(VkAPI & vk, Material& renderItemMaterial)
{
    static auto startTime = std::chrono::high_resolution_clock::now();

    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

    MvpData2 ubo{};
    ubo.Model = g_Transform.to_mat4();

    renderItemMaterial.SetBuffer(vk.GetFrameIndex(), 0, 0, ubo);
}

void UpdateViewData(VkAPI & vk, ViewData* view, DeferredLightData& lightData)
{
    glm::quat qPitch = glm::angleAxis(glm::radians(-view->m_View.m_Camera.Rotation.x), glm::vec3(1, 0, 0));
    glm::quat qYaw = glm::angleAxis(glm::radians(view->m_View.m_Camera.Rotation.y), glm::vec3(0, 1, 0));
    // omit roll
    glm::quat Rotation = qPitch * qYaw;
    Rotation = glm::normalize(Rotation);
    glm::mat4 rotate = glm::mat4_cast(Rotation);
    glm::mat4 translate = glm::mat4(1.0f);
    translate = glm::translate(translate, -view->m_View.m_Camera.Position);

    view->m_View.m_Camera.View = rotate * translate;
    if (vk.m_SwapChainImageExtent.width > 0 || vk.m_SwapChainImageExtent.height)
    {
        view->m_View.m_Camera.Proj = glm::perspective(glm::radians(45.0f),(float) view->m_View.m_CurrentResolution.width / (float)view->m_View.m_CurrentResolution.height, 0.1f, 10000.0f);
        view->m_View.m_Camera.Proj[1][1] *= -1;
    }

    view->m_Pipeline.m_PipelineMaterials[0]->SetBuffer(vk.GetFrameIndex(), 0, 3, lightData);
}

void RecordCommandBuffersV2(VkSDL & vk, Vector<ViewData*> views, RenderModel& model, Mesh& screenQuad, LvkIm3dState& im3dState, DeferredLightData& lightData)
{
    static Vector<VertexDataPosUv> originalScreenQuadData = {
                    { { -1.0f, -1.0f , 0.0f}, { 0.0f, 0.0f } },
                    { {1.0f, -1.0f, 0.0f}, {1.0, 0.0f} },
                    { {1.0f, 1.0f, 0.0f}, {1.0, 1.0} },
                    { {-1.0f, 1.0f, 0.0f}, {0.0f, 1.0} }
    };

    

    vk.RecordGraphicsCommands([&](VkCommandBuffer& commandBuffer, uint32_t frameIndex) {
        {
            Array<VkClearValue, 4> clearValues{};
            clearValues[0].color = { {0.0f, 0.0f, 0.0f, 1.0f} };
            clearValues[1].color = { {0.0f, 0.0f, 0.0f, 1.0f} };
            clearValues[2].color = { {0.0f, 0.0f, 0.0f, 1.0f} };
            clearValues[3].depthStencil = { 1.0f, 0 };

            VkRenderPassBeginInfo renderPassInfo{};
            renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            renderPassInfo.renderPass = vk.m_SwapchainImageRenderPass;
            renderPassInfo.framebuffer = vk.m_SwapChainFramebuffers[frameIndex];
            renderPassInfo.renderArea.offset = { 0,0 };
            renderPassInfo.renderArea.extent = vk.m_SwapChainImageExtent;

            renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
            renderPassInfo.pClearValues = clearValues.data();

            vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
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

            vkCmdEndRenderPass(commandBuffer);
        }


        for (auto& view : views)
        {
            if (view->m_Pipeline.m_CommandCallback.has_value())
            {
                Vector<Renderable> renderables{};
                for (auto item : model.m_RenderItems)
                {
                    Mesh m = {
                            item.m_Mesh.m_VertexBuffer, item.m_Mesh.m_VertexBufferMemory,
                            item.m_Mesh.m_IndexBuffer, item.m_Mesh.m_IndexBufferMemory,
                            item.m_Mesh.m_IndexCount
                    };
                    Renderable renderable =
                    {
                        item.m_Material.m_DescriptorSets[0].m_Sets[frameIndex],
                        view->m_Pipeline.m_PipelineDatas[0]->m_PipelineLayout,
                        m
                    };
                    renderables.push_back(renderable);
                }
                auto& draw_commands = view->m_Pipeline.m_CommandCallback.value();

                //todo: convert meshes to renderables
                draw_commands(commandBuffer, frameIndex, view->m_View, view->m_ViewQuad, renderables);
            }
        }
    });
}

RenderModel CreateRenderModelGbuffer(VkAPI & vk, const String& modelPath, ShaderProgram& shader)
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

void OnImGui(VkAPI & vk, DeferredLightData& lightDataCpu, Vector<ViewData*> views)
{
    if (ImGui::Begin("Statistics"))
    {
        ImGui::Text("FPS: %.3f", (1.0 / vk.m_DeltaTime));
        ImGui::Text("Frametime: %.3f ms", vk.m_DeltaTime * 1000.0f);
        ImGui::Separator();
        
        VmaBudget budgets[VK_MAX_MEMORY_HEAPS];
        vmaGetHeapBudgets(vk.m_Allocator, budgets);

        for (int i = 0; i < 3; i++)
        {
            const String heapName = "Heap " + std::to_string(i);
            if (ImGui::TreeNode(heapName.c_str()))
            {
                ImGui::Text("Allocs : %i", budgets[i].statistics.allocationCount);
                ImGui::Text("Block Count: %i", budgets[i].statistics.blockCount);
                ImGui::Text("Memory Usage: %d MB / %d MB", (budgets[i].usage / 1024 / 1024), (budgets[i].budget / 1024 / 1024));
                ImGui::TreePop();
            }
        }
    }
    ImGui::End();
    if (ImGui::Begin("View 1"))
    {
        // size needs to be the current resolution
        // uv0 will likely always be 0,0
        // uv1 needs to be MaxResolution / CurrentResolution;
        auto extent = ImGui::GetContentRegionAvail();
        auto max = vk.GetMaxFramebufferExtent();
        ImVec2 uv1 = { extent.x / max.width, extent.y / max.height };
        // auto& image = views[0]->m_LightPassFB.m_ColourAttachments[0].m_AttachmentSwapchainImages[vk.GetFrameIndex()];
        auto& image = views[0]->m_Pipeline.GetOutputFramebuffer()->m_ColourAttachments[0].m_AttachmentSwapchainImages[vk.GetFrameIndex()];
        ImGuiX::Image(image, extent, { 0,0 }, uv1);
        DrawIm3dTextListsImGuiAsChild(Im3d::GetTextDrawLists(), Im3d::GetTextDrawListCount(), (float)views[0]->m_View.m_CurrentResolution.width, (float)views[0]->m_View.m_CurrentResolution.height, views[0]->m_View.m_Camera.Proj * views[0]->m_View.m_Camera.View);
        views[0]->m_View.m_CurrentResolution = { (uint32_t)extent.x, (uint32_t)extent.y };

    }
    ImGui::End();

    if (ImGui::Begin("View 2"))
    {
        auto extent = ImGui::GetContentRegionAvail();
        auto max = vk.GetMaxFramebufferExtent();
        ImVec2 uv1 = { extent.x / max.width, extent.y / max.height };
        // auto& image = views[0]->m_LightPassFB.m_ColourAttachments[0].m_AttachmentSwapchainImages[vk.GetFrameIndex()];
        auto& image = views[1]->m_Pipeline.GetOutputFramebuffer()->m_ColourAttachments[0].m_AttachmentSwapchainImages[vk.GetFrameIndex()];
        ImGuiX::Image(image, extent, { 0,0 }, uv1);
        DrawIm3dTextListsImGuiAsChild(Im3d::GetTextDrawLists(), Im3d::GetTextDrawListCount(), (float)views[1]->m_View.m_CurrentResolution.width, (float)views[1]->m_View.m_CurrentResolution.height, views[1]->m_View.m_Camera.Proj * views[1]->m_View.m_Camera.View);
        views[1]->m_View.m_CurrentResolution = { (uint32_t)extent.x, (uint32_t)extent.y };

    }
    ImGui::End();

    if (ImGui::Begin("ECS Debug"))
    {        
        ImGui::DragFloat3("Position", &g_Transform.m_Position[0]);
        ImGui::DragFloat3("Euler Rotation", &g_Transform.m_Rotation[0]);
        ImGui::DragFloat3("Scale", &g_Transform.m_Scale[0]);

        ImGui::Separator();

        ImGui::DragFloat3("Cam 1 Position", &views[0]->m_View.m_Camera.Position[0]);
        ImGui::DragFloat3("Cam 1 Euler Rotation", &views[0]->m_View.m_Camera.Rotation[0]);

        ImGui::Separator();

        ImGui::DragFloat3("Cam 2 Position", &views[1]->m_View.m_Camera.Position[0]);
        ImGui::DragFloat3("Cam 2 Euler Rotation", &views[1]->m_View.m_Camera.Rotation[0]);
    }
    ImGui::End();

    if (ImGui::Begin("Lights Menu"))
    {
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
                    Im3d::Vec3 pos = { lightDataCpu.m_PointLights[i].PositionRadius.x , lightDataCpu.m_PointLights[i].PositionRadius.y, lightDataCpu.m_PointLights[i].PositionRadius.z };
                    float radius = lightDataCpu.m_PointLights[i].PositionRadius.w * lightDataCpu.m_PointLights[i].PositionRadius.w;
                    Im3d::DrawSphere(pos, radius);

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
    Im3d::PushAlpha(1.0f);
    Im3d::PushColor(Im3d::Color_Red);
    Im3d::SetAlpha(1.0f);
    Im3d::DrawCircle({ 0.0f, 1.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, 20.5f, 32);
    Im3d::DrawCone({ 40.0f, 0.0f, 0.0f }, { 0.0, 1.0, 0.0 }, 30.0f, 30.0f, 32);
    Im3d::DrawPrism({ 80.0f, 0.0f, 0.0f }, { 80.0f, 80.0f, 0.0f }, 10.0f, 32);
    Im3d::PopColor();
    Im3d::PushColor(Im3d::Color_Green);
    Im3d::DrawArrow({ 120.0f, 0.0f, 0.0f }, { 3.0f, 23.0f, 0.0f }, 2.0f, 2.0f);
    Im3d::DrawXyzAxes();
    Im3d::DrawCylinder({ 160.0f, 0.0f, 0.0f }, { 160.0f,20.0f, 0.0f }, 20.0f, 32);
    Im3d::PopColor();
    Im3d::PushColor(Im3d::Color_White);
    Im3d::Text({ 0.0, 20.0f, 0.0f }, 0, "Hello from you fuck you bloody");
    Im3d::PopColor();
    Im3d::PopAlpha();
}

int main() {
    VkSDL vk;
    bool enableMSAA = false;
    vk.Start("Pipeline Example",1920, 1080, enableMSAA);
    auto im3dState = LoadIm3D(vk);

    DeferredLightData lightDataCpu{};
    FillExampleLightData(lightDataCpu);

    ShaderProgram gbufferProg = ShaderProgram::CreateFromBinaryPath(
        vk, "shaders/gbuffer.vert.spv", "shaders/gbuffer.frag.spv");
    ShaderProgram lightPassProg = ShaderProgram::CreateFromBinaryPath(
        vk, "shaders/lights.vert.spv", "shaders/lights.frag.spv");
    
    ViewData viewA = CreateView(vk, im3dState, gbufferProg, lightPassProg);
    viewA.m_View.m_Camera.Position = { -40.0, 10.0f, 30.0f };
    ViewData viewB = CreateView(vk, im3dState, gbufferProg, lightPassProg);
    viewB.m_View.m_Camera.Position = { 30.0, 0.0f, -20.0f };

    Vector<ViewData*> views{ &viewA, &viewB };

    // create vertex and index buffer
    // allocate materials instead of raw buffers etc.
    RenderModel m = CreateRenderModelGbuffer(vk, "../assets/sponza/sponza.gltf", gbufferProg);

    while (vk.ShouldRun())
    {
        vk.PreFrame();

        Im3d::NewFrame();

        for (int i = 0; i < m.m_RenderItems.size(); i++)
        {
            UpdateRenderItemUniformBuffer(vk, m.m_RenderItems[i].m_Material);
        }

        for (int i = 0; i < views.size(); i++)
        {
            UpdateViewData(vk, views[i], lightDataCpu);
        }


        OnIm3D();
        OnImGui(vk, lightDataCpu, views);

        Im3d::EndFrame();

        RecordCommandBuffersV2(vk, views, m, *Mesh::g_ScreenSpaceQuad, im3dState, lightDataCpu);


        vk.PostFrame();
    }
    gbufferProg.Free(vk);
    lightPassProg.Free(vk);

    m.Free(vk);

    FreeIm3d(vk, im3dState);

    return 0;
}