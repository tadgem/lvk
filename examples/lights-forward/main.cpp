#include "example-common.h"
#include "lvk/Shader.h"
using namespace lvk;

#define NUM_LIGHTS 16
using ForwardLightData = FrameLightDataT<NUM_LIGHTS>;

static ShaderBufferFrameData mvpUniformData;
static ShaderBufferFrameData lightsUniformData;

static ForwardLightData lightDataCpu {};
static std::vector<VkDescriptorSet>     descriptorSets;

void RecordCommandBuffers(VulkanAPI_SDL& vk, VkPipeline& pipeline, VkPipelineLayout& pipelineLayout, Model& model)
{
    vk.RecordGraphicsCommands([&](VkCommandBuffer& commandBuffer, uint32_t frameIndex) {
        // push to example
        std::array<VkClearValue, 2> clearValues{};
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
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

        for (int i = 0; i < model.m_Meshes.size(); i++)
        {
            MeshEx& mesh = model.m_Meshes[i];
            VkBuffer vertexBuffers[]{ mesh.m_VertexBuffer};
            VkDeviceSize sizes[] = { 0 };

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
            vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, sizes);
            vkCmdBindIndexBuffer(commandBuffer, mesh.m_IndexBuffer, 0, VK_INDEX_TYPE_UINT32);
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets[frameIndex], 0, nullptr);
            vkCmdDrawIndexed(commandBuffer, mesh.m_IndexCount, 1, 0, 0, 0);
        }
        vkCmdEndRenderPass(commandBuffer);
    });
}

void UpdateUniformBuffer(VulkanAPI_SDL& vk)
{
    static auto startTime = std::chrono::high_resolution_clock::now();
    

    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

    time = 0.0f;
    MvpData ubo{};
    ubo.Model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.View = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    if (vk.m_SwapChainImageExtent.width > 0 || vk.m_SwapChainImageExtent.height)
    {
        ubo.Proj = glm::perspective(glm::radians(45.0f), vk.m_SwapChainImageExtent.width / (float)vk.m_SwapChainImageExtent.height, 0.1f, 300.0f);
        ubo.Proj[1][1] *= -1;
    }
    mvpUniformData.Set(vk.GetFrameIndex(), ubo);

    // light imgui

    if (ImGui::Begin("Lights"))
    {
        ImGui::Text("FPSS : %f", 1.0 / vk.m_DeltaTime);
        ImGui::DragFloat3("Directional Light Dir", &lightDataCpu.m_DirectionalLight.Direction[0]);
        ImGui::DragFloat4("Directional Light Colour", &lightDataCpu.m_DirectionalLight.Colour[0]);
        ImGui::DragFloat4("Directional Light Ambient Colour", &lightDataCpu.m_DirectionalLight.Ambient[0]);

        if(ImGui::TreeNode("Point Lights"))
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

    lightsUniformData.Set(vk.GetFrameIndex(), lightDataCpu);
}

int main()
{
    VulkanAPI_SDL vk;
    bool enableMSAA = true;
    vk.Start("Forward Lights", 1280, 720, enableMSAA);

    // shader abstraction

    ShaderProgram lights_prog = ShaderProgram::CreateFromBinaryPath(
        vk, "shaders/lights.vert.spv", "shaders/lights.frag.spv");

    FillExampleLightData(lightDataCpu);

    // Texture abstraction
    uint32_t mipLevels;
    VkImage textureImage;
    VkImageView imageView;
    VkDeviceMemory textureMemory;
    vk.CreateTexture("assets/viking_room.png", VK_FORMAT_R8G8B8A8_UNORM, textureImage, imageView, textureMemory, &mipLevels);
    VkSampler imageSampler;
    vk.CreateImageSampler(imageView, mipLevels, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_REPEAT, imageSampler);

    // Pipeline stage?
    VkPipelineLayout pipelineLayout;
    VkPipeline pipeline = vk.CreateRasterizationGraphicsPipeline(
        lights_prog, Vector<VkVertexInputBindingDescription>{VertexDataPosNormalUv::GetBindingDescription() }, VertexDataPosNormalUv::GetAttributeDescriptions(),
        vk.m_SwapchainImageRenderPass,
        vk.m_SwapChainImageExtent.width, vk.m_SwapChainImageExtent.height,
        VK_POLYGON_MODE_FILL,
        VK_CULL_MODE_NONE,
        enableMSAA,
        VK_COMPARE_OP_LESS,
        pipelineLayout);

    // create vertex and index buffer
    Model model;
    LoadModelAssimp(vk, model, "assets/viking_room.obj", true);

    // Shader too probably
    vk.CreateUniformBuffers<MvpData>(mvpUniformData);
    vk.CreateUniformBuffers<FrameLightDataT<NUM_LIGHTS>>(lightsUniformData);


    while (vk.ShouldRun())
    {    
        vk.PreFrame();
        
        UpdateUniformBuffer(vk);

        RecordCommandBuffers(vk, pipeline, pipelineLayout, model);

        vk.PostFrame();
    }

    mvpUniformData.Free(vk);
    lightsUniformData.Free(vk);

    FreeModel(vk, model);
    vkDestroySampler(vk.m_LogicalDevice, imageSampler, nullptr);
    vkDestroyImageView(vk.m_LogicalDevice, imageView, nullptr);
    vkDestroyImage(vk.m_LogicalDevice, textureImage, nullptr);
    vkFreeMemory(vk.m_LogicalDevice, textureMemory, nullptr);
    vkDestroyPipelineLayout(vk.m_LogicalDevice, pipelineLayout, nullptr);
    vkDestroyPipeline(vk.m_LogicalDevice, pipeline, nullptr);

    return 0;
}
