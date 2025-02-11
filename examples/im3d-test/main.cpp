#include "example-common.h"
#include "lvk/Shader.h"
#include "lvk/Material.h"
#include <algorithm>
#include "Im3D/im3d_lvk.h"

using namespace lvk;

static Transform g_Transform;
static Camera g_Camera;

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

        DrawIm3d(vk, commandBuffer, frameIndex, im3dState, im3dViewState, g_Camera.Proj * g_Camera.View, vk.m_SwapChainImageExtent.width, vk.m_SwapChainImageExtent.height, true);
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
    g_Camera.View = ubo.View;
    if (vk.m_SwapChainImageExtent.width > 0 || vk.m_SwapChainImageExtent.height)
    {
        ubo.Proj = glm::perspective(glm::radians(45.0f), vk.m_SwapChainImageExtent.width / (float)vk.m_SwapChainImageExtent.height, 0.1f, 10000.0f);
        ubo.Proj[1][1] *= -1;
        g_Camera.Proj = ubo.Proj;
    }
    renderItemMaterial.SetBuffer(vk.GetFrameIndex(), 0, 0, ubo);
    lightPassMaterial.SetBuffer(vk.GetFrameIndex(), 0, 0, ubo);
    lightPassMaterial.SetBuffer(vk.GetFrameIndex(), 0, 4, lightDataCpu);
}

RenderModel CreateRenderModelGbuffer(VkBackend & vk, const String& modelPath, ShaderProgram& shader)
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

void OnImGui(VkBackend & vk, DeferredLightData& lightDataCpu)
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
    bool enableMSAA = true;
    vk.Start("IM3D",1280, 720, enableMSAA);
    auto im3dState = LoadIm3D(vk);
    auto im3dViewState = AddIm3dForViewport(vk, im3dState, vk.m_SwapchainImageRenderPass, enableMSAA);

    DeferredLightData lightDataCpu{};
    FillExampleLightData(lightDataCpu);

    ShaderProgram gbufferProg = ShaderProgram::CreateFromBinaryPath(
        vk, "shaders/gbuffer.vert.spv", "shaders/gbuffer.frag.spv");
    ShaderProgram lightPassProg = ShaderProgram::CreateFromBinaryPath(
        vk, "shaders/lights.vert.spv", "shaders/lights.frag.spv");
    Material lightPassMat = Material::Create(vk, lightPassProg);

    Framebuffer gbuffer{};

    gbuffer.AddColourAttachment(vk, ResolutionScale::Full, 1, VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_ASPECT_COLOR_BIT);
    gbuffer.AddColourAttachment(vk, ResolutionScale::Full, 1, VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_ASPECT_COLOR_BIT);
    gbuffer.AddColourAttachment(vk, ResolutionScale::Full, 1, VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
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
    VkPipeline gbufferPipeline = vk.CreateRasterPipeline(
        gbufferProg,
        Vector<VkVertexInputBindingDescription>{
            VertexDataPosNormalUv::GetBindingDescription()},
        VertexDataPosNormalUv::GetAttributeDescriptions(), gbuffer.m_RenderPass,
        vk.m_SwapChainImageExtent.width, vk.m_SwapChainImageExtent.height,
        VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, false, VK_COMPARE_OP_LESS,
        gbufferPipelineLayout, 3);

    // create present graphics pipeline
    // Pipeline stage?
    VkPipelineLayout lightPassPipelineLayout;
    VkPipeline pipeline = vk.CreateRasterPipeline(
        lightPassProg,
        Vector<VkVertexInputBindingDescription>{
            VertexDataPosUv::GetBindingDescription()},
        VertexDataPosUv::GetAttributeDescriptions(),
        vk.m_SwapchainImageRenderPass, vk.m_SwapChainImageExtent.width,
        vk.m_SwapChainImageExtent.height, VK_POLYGON_MODE_FILL,
        VK_CULL_MODE_NONE, enableMSAA, VK_COMPARE_OP_LESS,
        lightPassPipelineLayout);

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

        /*
        for (auto item : m.m_RenderItems)
        {
            auto& mesh = item.m_Mesh;

            mesh.m_AABB = TransformAABB(mesh.m_AABB, g_Transform.to_mat4());
            Im3d::SetSize(0.3f);
            Im3d::DrawAlignedBox(
                { mesh.m_AABB.m_Min.x, mesh.m_AABB.m_Min.y, mesh.m_AABB.m_Min.z },
                { mesh.m_AABB.m_Max.x, mesh.m_AABB.m_Max.y, mesh.m_AABB.m_Max.z }
            );
        }
        */

        Im3d::EndFrame();
        RecordCommandBuffersV2(vk,
            gbufferPipeline, gbufferPipelineLayout, gbuffer.m_RenderPass, gbuffer.m_SwapchainFramebuffers,
            pipeline, lightPassPipelineLayout, vk.m_SwapchainImageRenderPass, lightPassMat, vk.m_SwapChainFramebuffers,
            m, *Mesh::g_ScreenSpaceQuad, im3dState, im3dViewState);

        OnImGui(vk, lightDataCpu);

        vk.PostFrame();
    }
    gbufferProg.Free(vk);
    lightPassProg.Free(vk);

    lightPassMat.Free(vk);
    gbuffer.Free(vk);
    m.Free(vk);

    FreeIm3dViewport(vk, im3dViewState);
    FreeIm3d(vk, im3dState);

    vkDestroyRenderPass(vk.m_LogicalDevice, gbuffer.m_RenderPass, nullptr);

    vkDestroyPipelineLayout(vk.m_LogicalDevice, gbufferPipelineLayout, nullptr);
    vkDestroyPipeline(vk.m_LogicalDevice, gbufferPipeline, nullptr);
    vkDestroyPipelineLayout(vk.m_LogicalDevice, lightPassPipelineLayout, nullptr);
    vkDestroyPipeline(vk.m_LogicalDevice, pipeline, nullptr);

    return 0;
}
