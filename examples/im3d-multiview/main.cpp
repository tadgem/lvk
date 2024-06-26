#include "example-common.h"
#include "lvk/Shader.h"
#include "lvk/Material.h"
#include <algorithm>
#include "Im3D/im3d_lvk.h"
#include "ImGui/lvk_extensions.h"

using namespace lvk;

struct ViewData
{
    Framebuffer m_GBuffer;
    Framebuffer m_LightPassFB;
    Material    m_LightPassMaterial;

    VkPipeline          m_GBufferPipeline, m_LightPassPipeline;
    VkPipelineLayout    m_GBufferPipelineLayout, m_LightPassPipelineLayour;

    LvkIm3dViewState m_Im3dState;

    Camera      m_Camera;
};

ViewData CreateView(VulkanAPI& vk, LvkIm3dState im3dState, ShaderProgram gbufferProg, ShaderProgram lightPassProg)
{
    Framebuffer gbuffer{};
    gbuffer.m_Width = vk.m_SwapChainImageExtent.width;
    gbuffer.m_Height = vk.m_SwapChainImageExtent.height;

    gbuffer.AddColourAttachment(vk, ResolutionScale::Full, 1, VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_ASPECT_COLOR_BIT);
    gbuffer.AddColourAttachment(vk, ResolutionScale::Full, 1, VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_ASPECT_COLOR_BIT);
    gbuffer.AddColourAttachment(vk, ResolutionScale::Full, 1, VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_ASPECT_COLOR_BIT);
    gbuffer.AddDepthAttachment(vk, ResolutionScale::Full, 1, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_ASPECT_DEPTH_BIT);
    gbuffer.Build(vk);

    Framebuffer finalImage{};
    finalImage.m_Width = vk.m_SwapChainImageExtent.width;
    finalImage.m_Height = vk.m_SwapChainImageExtent.height;

    finalImage.AddColourAttachment(vk, ResolutionScale::Full, 1, VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_ASPECT_COLOR_BIT);
    finalImage.AddDepthAttachment(vk, ResolutionScale::Full, 1, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_ASPECT_DEPTH_BIT);
    finalImage.Build(vk);

    Material lightPassMat = Material::Create(vk, lightPassProg);

    lightPassMat.SetColourAttachment(vk, "positionBufferSampler", gbuffer, 1);
    lightPassMat.SetColourAttachment(vk, "normalBufferSampler", gbuffer, 2);
    lightPassMat.SetColourAttachment(vk, "colourBufferSampler", gbuffer, 0);

    // create gbuffer pipeline
    VkPipelineLayout gbufferPipelineLayout;
    VkPipeline gbufferPipeline = vk.CreateRasterizationGraphicsPipeline(
        gbufferProg,
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
        lightPassProg,
        Vector<VkVertexInputBindingDescription>{VertexDataPosUv::GetBindingDescription() },
        VertexDataPosUv::GetAttributeDescriptions(),
        finalImage.m_RenderPass,
        vk.m_SwapChainImageExtent.width, vk.m_SwapChainImageExtent.height,
        VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, false,
        VK_COMPARE_OP_LESS, lightPassPipelineLayout);

    auto im3dViewState = AddIm3dForViewport(vk, im3dState, finalImage.m_RenderPass, false);

    return { gbuffer, finalImage, lightPassMat, gbufferPipeline, pipeline, gbufferPipelineLayout, lightPassPipelineLayout, im3dViewState };
}

void FreeView(VulkanAPI& vk, ViewData& view)
{
    FreeIm3dViewport(vk, view.m_Im3dState);
}

static Transform g_Transform;

void UpdateRenderItemUniformBuffer(VulkanAPI& vk, Material& renderItemMaterial)
{
    static auto startTime = std::chrono::high_resolution_clock::now();

    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

    MvpData2 ubo{};
    ubo.Model = g_Transform.to_mat4();

    renderItemMaterial.SetBuffer(vk.GetFrameIndex(), 0, 0, ubo);
}

void UpdateViewData(VulkanAPI& vk, ViewData* view, DeferredLightData& lightData)
{
    glm::quat qPitch = glm::angleAxis(glm::radians(-view->m_Camera.Rotation.x), glm::vec3(1, 0, 0));
    glm::quat qYaw = glm::angleAxis(glm::radians(view->m_Camera.Rotation.y), glm::vec3(0, 1, 0));
    // omit roll
    glm::quat Rotation = qPitch * qYaw;
    Rotation = glm::normalize(Rotation);
    glm::mat4 rotate = glm::mat4_cast(Rotation);
    glm::mat4 translate = glm::mat4(1.0f);
    translate = glm::translate(translate, -view->m_Camera.Position);

    view->m_Camera.View = rotate * translate;
    if (vk.m_SwapChainImageExtent.width > 0 || vk.m_SwapChainImageExtent.height)
    {
        view->m_Camera.Proj = glm::perspective(glm::radians(45.0f), vk.m_SwapChainImageExtent.width / (float)vk.m_SwapChainImageExtent.height, 0.1f, 10000.0f);
        view->m_Camera.Proj[1][1] *= -1;
    }

    view->m_LightPassMaterial.SetBuffer(vk.GetFrameIndex(), 0, 3, lightData);
}

void RecordCommandBuffersV2(VulkanAPI_SDL& vk, Vector<ViewData*> views, RenderModel& model, Mesh& screenQuad, LvkIm3dState& im3dState, DeferredLightData& lightData)
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
            struct PCViewData
            {
                glm::mat4 view;
                glm::mat4 proj;
            };


            PCViewData pcData{ view->m_Camera.View, view->m_Camera.Proj };

            {
                Array<VkClearValue, 4> clearValues{};
                clearValues[0].color = { {0.0f, 0.0f, 0.0f, 1.0f} };
                clearValues[1].color = { {0.0f, 0.0f, 0.0f, 1.0f} };
                clearValues[2].color = { {0.0f, 0.0f, 0.0f, 1.0f} };
                clearValues[3].depthStencil = { 1.0f, 0 };

                VkRenderPassBeginInfo renderPassInfo{};
                renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
                renderPassInfo.renderPass = view->m_GBuffer.m_RenderPass;
                renderPassInfo.framebuffer = view->m_GBuffer.m_SwapchainFramebuffers[frameIndex];
                renderPassInfo.renderArea.offset = { 0,0 };
                renderPassInfo.renderArea.extent = vk.m_SwapChainImageExtent;

                renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
                renderPassInfo.pClearValues = clearValues.data();

                vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
                vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, view->m_GBufferPipeline);
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
                vkCmdPushConstants(commandBuffer, view->m_GBufferPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PCViewData), &pcData);

                for (int i = 0; i < model.m_RenderItems.size(); i++)
                {
                    MeshEx& mesh = model.m_RenderItems[i].m_Mesh;

                    VkBuffer vertexBuffers[]{ mesh.m_VertexBuffer };
                    VkDeviceSize sizes[] = { 0 };

                    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, sizes);
                    vkCmdBindIndexBuffer(commandBuffer, mesh.m_IndexBuffer, 0, VK_INDEX_TYPE_UINT32);
                    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, view->m_GBufferPipelineLayout, 0, 1, &model.m_RenderItems[i].m_Material.m_DescriptorSets[0].m_Sets[frameIndex], 0, nullptr);
                    vkCmdDrawIndexed(commandBuffer, mesh.m_IndexCount, 1, 0, 0, 0);
                }
                vkCmdEndRenderPass(commandBuffer);
            }

            Array<VkClearValue, 2> clearValues{};
            clearValues[0].color = { {0.0f, 0.0f, 0.0f, 1.0f} };
            clearValues[1].depthStencil = { 1.0f, 0 };

            VkRenderPassBeginInfo renderPassInfo{};
            renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            renderPassInfo.renderPass = view->m_LightPassFB.m_RenderPass;
            renderPassInfo.framebuffer = view->m_LightPassFB.m_SwapchainFramebuffers[frameIndex];
            renderPassInfo.renderArea.offset = { 0,0 };
            renderPassInfo.renderArea.extent = vk.m_SwapChainImageExtent;

            renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
            renderPassInfo.pClearValues = clearValues.data();

            vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, view->m_LightPassPipeline);
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
            vkCmdPushConstants(commandBuffer, view->m_LightPassPipelineLayour, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PCViewData), &pcData);
            VkDeviceSize sizes[] = { 0 };
            vkCmdBindVertexBuffers(commandBuffer, 0, 1, &screenQuad.m_VertexBuffer, sizes);
            vkCmdBindIndexBuffer(commandBuffer, screenQuad.m_IndexBuffer, 0, VK_INDEX_TYPE_UINT32);
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, view->m_LightPassPipelineLayour, 0, 1, &view->m_LightPassMaterial.m_DescriptorSets[0].m_Sets[frameIndex], 0, nullptr);
            vkCmdDrawIndexed(commandBuffer, screenQuad.m_IndexCount, 1, 0, 0, 0);

            DrawIm3d(vk, commandBuffer, frameIndex, im3dState, view->m_Im3dState, view->m_Camera.Proj * view->m_Camera.View);
            vkCmdEndRenderPass(commandBuffer);
        }
        }
    );
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

void OnImGui(VulkanAPI& vk, DeferredLightData& lightDataCpu, Vector<ViewData*> views)
{

    if (ImGui::Begin("View 1"))
    {
        ImGuiX::Image(views[0]->m_LightPassFB.m_ColourAttachments[0].m_AttachmentSwapchainImages[vk.GetFrameIndex()], { 1280, 720 });
        DrawIm3dTextListsImGuiAsChild(Im3d::GetTextDrawLists(), Im3d::GetTextDrawListCount(), 1280, 720, views[0]->m_Camera.Proj * views[0]->m_Camera.View);
    }
    ImGui::End();

    if (ImGui::Begin("View 2"))
    {
        ImGuiX::Image(views[1]->m_LightPassFB.m_ColourAttachments[0].m_AttachmentSwapchainImages[vk.GetFrameIndex()], { 1280, 720 });
        DrawIm3dTextListsImGuiAsChild(Im3d::GetTextDrawLists(), Im3d::GetTextDrawListCount(), 1280, 720, views[1]->m_Camera.Proj * views[1]->m_Camera.View);
    }
    ImGui::End();

    if (ImGui::Begin("Scene Debug"))
    {
        ImGui::Text("Frametime: %f", (1.0 / vk.m_DeltaTime));
        ImGui::Separator();
        ImGui::DragFloat3("Position", &g_Transform.m_Position[0]);
        ImGui::DragFloat3("Euler Rotation", &g_Transform.m_Rotation[0]);
        ImGui::DragFloat3("Scale", &g_Transform.m_Scale[0]);

        ImGui::Separator();

        ImGui::DragFloat3("Cam 1 Position", &views[0]->m_Camera.Position[0]);
        ImGui::DragFloat3("Cam 1 Euler Rotation", &views[0]->m_Camera.Rotation[0]);

        ImGui::Separator();

        ImGui::DragFloat3("Cam 2 Position", &views[1]->m_Camera.Position[0]);
        ImGui::DragFloat3("Cam 2 Euler Rotation", &views[1]->m_Camera.Rotation[0]);
    }
    ImGui::End();

    if (ImGui::Begin("Lights Menu"))
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
    Im3d::PushColor(Im3d::Color_Red);
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
}

int main() {
    VulkanAPI_SDL vk;
    bool enableMSAA = false;
    vk.Start(1920, 1080, enableMSAA);
    auto im3dState = LoadIm3D(vk);

    DeferredLightData lightDataCpu{};
    FillExampleLightData(lightDataCpu);

    ShaderProgram gbufferProg = ShaderProgram::Create(vk, "shaders/gbuffer.vert.spv", "shaders/gbuffer.frag.spv");
    ShaderProgram lightPassProg = ShaderProgram::Create(vk, "shaders/lights.vert.spv", "shaders/lights.frag.spv");

    ViewData viewA = CreateView(vk, im3dState, gbufferProg, lightPassProg);
    viewA.m_Camera.Position = { -40.0, 10.0f, 30.0f };
    ViewData viewB = CreateView(vk, im3dState, gbufferProg, lightPassProg);
    viewB.m_Camera.Position = { 30.0, 0.0f, -20.0f };

    Vector<ViewData*> views{ &viewA, &viewB };

    // create vertex and index buffer
    // allocate materials instead of raw buffers etc.
    RenderModel m = CreateRenderModelGbuffer(vk, "assets/sponza/sponza.gltf", gbufferProg);

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

        Im3d::EndFrame();

        RecordCommandBuffersV2(vk, views, m, *Mesh::g_ScreenSpaceQuad, im3dState, lightDataCpu);

        OnImGui(vk, lightDataCpu, views);

        vk.PostFrame();
    }
    gbufferProg.Free(vk);
    lightPassProg.Free(vk);

    FreeView(vk, viewA);
    FreeView(vk, viewB);

    m.Free(vk);

    FreeIm3d(vk, im3dState);

    return 0;
}
