#include "example-common.h"
#include "lvk/Material.h"
#include "lvk/Shader.h"

#include <algorithm>
using namespace lvk;

#define NUM_LIGHTS 512
using DeferredLightData = FrameLightDataT<NUM_LIGHTS>;

class FramebufferEx {
public:
    Vector<Texture> m_Attachments;
    VkFramebuffer   m_FB;

    void Free(VkBackend & vk)
    {
        for (auto& t : m_Attachments)
        {
            t.Free(vk);
        }
        vkDestroyFramebuffer(vk.m_LogicalDevice, m_FB, nullptr);
    }
} ;

class FramebufferSetEx {
public:
    Array<FramebufferEx, MAX_FRAMES_IN_FLIGHT> m_Framebuffers;

    void Free(VkBackend & vk)
    {
        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            m_Framebuffers[i].Free(vk);
        }
    }

};

static Vector<VertexDataPosUv> g_ScreenSpaceQuadVertexData = {
    { { -1.0f, -1.0f , 0.0f}, { 1.0f, 0.0f } },
    { {1.0f, -1.0f, 0.0f}, {0.0f, 0.0f} },
    { {1.0f, 1.0f, 0.0f}, {0.0f, 1.0f} },
    { {-1.0f, 1.0f, 0.0f}, {1.0f, 1.0f} }
};

static Vector<uint32_t> g_ScreenSpaceQuadIndexData = {
    0, 1, 2, 2, 3, 0
};

struct TransformEx {
    glm::vec3 position = glm::vec3(0);
    glm::vec3 rotation = glm::vec3(0);
    glm::vec3 scale = glm::vec3(1);
    glm::mat4 to_mat4() {
        glm::mat4 m = glm::translate(glm::mat4(1), position);
        glm::vec3 radians = CalculateVec3Radians(rotation);
        m *= glm::mat4_cast(glm::quat(radians));
        m = glm::scale(m, scale);
        return m;
    };
};

static TransformEx g_Transform; 

void RecordCommandBuffersV2(VulkanAPI_SDL& vk,
    VkPipeline& gbufferPipeline , VkPipelineLayout& gbufferPipelineLayout, VkRenderPass gbufferRenderPass, Vector<VkDescriptorSet>& gbufferDescriptorSets, Vector<VkFramebuffer>& gbufferFramebuffers,
    VkPipeline& lightingPassPipeline, VkPipelineLayout& lightingPassPipelineLayout, VkRenderPass lightingPassRenderPass, Vector<VkDescriptorSet>& lightingPassDescriptorSets, Vector<VkFramebuffer>& lightingPassFramebuffers,
    RenderModel& model, MeshEx& screenQuad)
{
    vk.RecordGraphicsCommands([&](VkCommandBuffer& commandBuffer, uint32_t frameIndex) {
        // push to example
        {
            std::array<VkClearValue, 4> clearValues{};
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
                vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, gbufferPipelineLayout, 0, 1, &model.m_RenderItems[i].m_Material.m_DescriptorSets[0].m_Sets[vk.GetFrameIndex()], 0, nullptr);
                vkCmdDrawIndexed(commandBuffer, mesh.m_IndexCount, 1, 0, 0, 0);
            }
            vkCmdEndRenderPass(commandBuffer);
        }

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
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, lightingPassPipelineLayout, 0, 1, &lightingPassDescriptorSets[frameIndex], 0, nullptr);
        vkCmdDrawIndexed(commandBuffer, screenQuad.m_IndexCount, 1, 0, 0, 0);
        vkCmdEndRenderPass(commandBuffer);
        });
}

void UpdateUniformBuffer(VulkanAPI_SDL& vk, ShaderBufferFrameData& itemMvp, ShaderBufferFrameData lightsData, DeferredLightData& lightDataCpu)
{
    static auto startTime = std::chrono::high_resolution_clock::now();

    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

    MvpData ubo{};
    ubo.Model = g_Transform.to_mat4();

    ubo.View = glm::lookAt(glm::vec3(20.0f, 20.0f, 20.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    if (vk.m_SwapChainImageExtent.width > 0 || vk.m_SwapChainImageExtent.height)
    {
        ubo.Proj = glm::perspective(glm::radians(45.0f), vk.m_SwapChainImageExtent.width / (float)vk.m_SwapChainImageExtent.height, 0.1f, 1000.0f);
        ubo.Proj[1][1] *= -1;
    }
    itemMvp.Set(vk.GetFrameIndex(), ubo);


    lightsData.Set(vk.GetFrameIndex(), lightDataCpu);
}

void UpdateUniformBufferMat(VulkanAPI_SDL& vk, Material& itemMat, ShaderBufferFrameData lightsData, DeferredLightData& lightDataCpu)
{
    static auto startTime = std::chrono::high_resolution_clock::now();

    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

    MvpData ubo{};
    ubo.Model = g_Transform.to_mat4();

    ubo.View = glm::lookAt(glm::vec3(20.0f, 20.0f, 20.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    if (vk.m_SwapChainImageExtent.width > 0 || vk.m_SwapChainImageExtent.height)
    {
        ubo.Proj = glm::perspective(glm::radians(45.0f), vk.m_SwapChainImageExtent.width / (float)vk.m_SwapChainImageExtent.height, 0.1f, 1000.0f);
        ubo.Proj[1][1] *= -1;
    }
    itemMat.SetBuffer(vk.GetFrameIndex(), 0, 0, ubo);


    lightsData.Set(vk.GetFrameIndex(), lightDataCpu);
}

void CreateGBufferDescriptorSets(VkBackend & vk, VkDescriptorSetLayout& descriptorSetLayout, VkImageView& textureImageView, VkSampler& textureSampler, Vector<VkDescriptorSet>& descriptorSets, ShaderBufferFrameData& mvpUniformData)
{
    std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, descriptorSetLayout);
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = vk.m_DescriptorSetAllocator.GetPool(vk.m_LogicalDevice);
    allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
    allocInfo.pSetLayouts = layouts.data();

    descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
    VK_CHECK(vkAllocateDescriptorSets(vk.m_LogicalDevice, &allocInfo, descriptorSets.data()));

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        VkDescriptorBufferInfo mvpBufferInfo{};
        mvpBufferInfo.buffer = mvpUniformData.m_UniformBuffers[i].m_GpuBuffer;
        mvpBufferInfo.offset = 0;
        mvpBufferInfo.range = sizeof(MvpData);

        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = textureImageView;
        imageInfo.sampler = textureSampler;

        std::array<VkWriteDescriptorSet, 2> descriptorWrites{};

        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstSet = descriptorSets[i];
        descriptorWrites[0].dstBinding = 0;
        descriptorWrites[0].dstArrayElement = 0;
        descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].pBufferInfo = &mvpBufferInfo;

        descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[1].dstSet = descriptorSets[i];
        descriptorWrites[1].dstBinding = 1;
        descriptorWrites[1].dstArrayElement = 0;
        descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrites[1].descriptorCount = 1;
        descriptorWrites[1].pImageInfo = &imageInfo;


        vkUpdateDescriptorSets(vk.m_LogicalDevice, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
    }

}

void CreateLightingPassDescriptorSets(VulkanAPI_SDL& vk, VkDescriptorSetLayout& descriptorSetLayout, FramebufferSetEx gbuffers, Vector<VkDescriptorSet>& descriptorSets, ShaderBufferFrameData& mvpUniformData, ShaderBufferFrameData& lightsUniformData)
{
    std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, descriptorSetLayout);
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = vk.m_DescriptorSetAllocator.GetPool(vk.m_LogicalDevice);
    allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
    allocInfo.pSetLayouts = layouts.data();

    descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
    VK_CHECK(vkAllocateDescriptorSets(vk.m_LogicalDevice, &allocInfo, descriptorSets.data()));

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        VkDescriptorBufferInfo mvpBufferInfo{};
        mvpBufferInfo.buffer = mvpUniformData.m_UniformBuffers[i].m_GpuBuffer;
        mvpBufferInfo.offset = 0;
        mvpBufferInfo.range = sizeof(MvpData);

        VkDescriptorImageInfo positionBufferInfo{};
        positionBufferInfo.imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
        positionBufferInfo.imageView = gbuffers.m_Framebuffers[i].m_Attachments[1].m_ImageView;
        positionBufferInfo.sampler = gbuffers.m_Framebuffers[i].m_Attachments[1].m_Sampler;

        VkDescriptorImageInfo normalBufferInfo{};
        normalBufferInfo.imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
        normalBufferInfo.imageView = gbuffers.m_Framebuffers[i].m_Attachments[2].m_ImageView;
        normalBufferInfo.sampler = gbuffers.m_Framebuffers[i].m_Attachments[2].m_Sampler;

        VkDescriptorImageInfo colourBufferInfo{};
        colourBufferInfo.imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
        colourBufferInfo.imageView = gbuffers.m_Framebuffers[i].m_Attachments[0].m_ImageView;
        colourBufferInfo.sampler = gbuffers.m_Framebuffers[i].m_Attachments[0].m_Sampler;

        VkDescriptorBufferInfo lightBufferInfo{};
        lightBufferInfo.buffer = lightsUniformData.m_UniformBuffers[i].m_GpuBuffer;
        lightBufferInfo.offset = 0;
        lightBufferInfo.range = sizeof(DeferredLightData);

        std::array<VkWriteDescriptorSet, 5> descriptorWrites{};

        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstSet = descriptorSets[i];
        descriptorWrites[0].dstBinding = 0;
        descriptorWrites[0].dstArrayElement = 0;
        descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].pBufferInfo = &mvpBufferInfo;

        descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[1].dstSet = descriptorSets[i];
        descriptorWrites[1].dstBinding = 1;
        descriptorWrites[1].dstArrayElement = 0;
        descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrites[1].descriptorCount = 1;
        descriptorWrites[1].pImageInfo = &positionBufferInfo;

        descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[2].dstSet = descriptorSets[i];
        descriptorWrites[2].dstBinding = 2;
        descriptorWrites[2].dstArrayElement = 0;
        descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrites[2].descriptorCount = 1;
        descriptorWrites[2].pImageInfo = &normalBufferInfo;

        descriptorWrites[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[3].dstSet = descriptorSets[i];
        descriptorWrites[3].dstBinding = 3;
        descriptorWrites[3].dstArrayElement = 0;
        descriptorWrites[3].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrites[3].descriptorCount = 1;
        descriptorWrites[3].pImageInfo = &colourBufferInfo;

        descriptorWrites[4].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[4].dstSet = descriptorSets[i];
        descriptorWrites[4].dstBinding = 4;
        descriptorWrites[4].dstArrayElement = 0;
        descriptorWrites[4].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[4].descriptorCount = 1;
        descriptorWrites[4].pBufferInfo = &lightBufferInfo;


        vkUpdateDescriptorSets(vk.m_LogicalDevice, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
    }

}

void CreateGBufferRenderPass(VulkanAPI_SDL& vk, VkRenderPass& renderPass)
{
    Vector<VkAttachmentDescription> colourAttachmentDescriptions{};
    Vector<VkAttachmentDescription> resolveAttachmentDescriptions{};
    VkAttachmentDescription depthAttachmentDescription{};

    VkAttachmentDescription positionNormalColourAttachment{};
    positionNormalColourAttachment.format = VK_FORMAT_R32G32B32A32_SFLOAT;
    positionNormalColourAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    positionNormalColourAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    positionNormalColourAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    positionNormalColourAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    positionNormalColourAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    positionNormalColourAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    positionNormalColourAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    
    // 3 instances: position, normal and colour attachments
    colourAttachmentDescriptions.push_back(positionNormalColourAttachment);
    colourAttachmentDescriptions.push_back(positionNormalColourAttachment);
    colourAttachmentDescriptions.push_back(positionNormalColourAttachment);

    depthAttachmentDescription.format = vk.FindDepthFormat();
    depthAttachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;;
    depthAttachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachmentDescription.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    vk.CreateRenderPass(renderPass, colourAttachmentDescriptions, resolveAttachmentDescriptions, true, depthAttachmentDescription, VK_ATTACHMENT_LOAD_OP_CLEAR);
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
        ImGui::DragFloat3("Position", &g_Transform.position[0]);
        ImGui::DragFloat3("Euler Rotation", & g_Transform.rotation[0]);
        ImGui::DragFloat3("Scale", & g_Transform.scale[0]);
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
int main()
{
    VulkanAPI_SDL vk;
    bool enableMSAA = false;
    vk.Start("Deferred", 1280, 720, enableMSAA);


    ShaderBufferFrameData mvpUniformData;
    ShaderBufferFrameData lightsUniformData;

    DeferredLightData lightDataCpu{};

    FillExampleLightData(lightDataCpu);
    Vector<VkDescriptorSet>     lightPassDescriptorSets;
    Vector<VkDescriptorSet>     gbufferDescriptorSets;

    ShaderProgram gbufferProg = ShaderProgram::CreateFromBinaryPath(
        vk, "shaders/gbuffer.vert.spv", "shaders/gbuffer.frag.spv");
    ShaderProgram lightPassProg = ShaderProgram::CreateFromBinaryPath(
        vk, "shaders/lights.vert.spv", "shaders/lights.frag.spv");

    VkRenderPass gbufferRenderPass;
    CreateGBufferRenderPass(vk, gbufferRenderPass);

    // create gbuffer pipeline
    VkPipelineLayout gbufferPipelineLayout;
    VkPipeline gbufferPipeline = vk.CreateRasterPipeline(
        gbufferProg,
        Vector<VkVertexInputBindingDescription>{
            VertexDataPosNormalUv::GetBindingDescription()},
        VertexDataPosNormalUv::GetAttributeDescriptions(), gbufferRenderPass,
        vk.m_SwapChainImageExtent.width, vk.m_SwapChainImageExtent.height,
        VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, false, VK_COMPARE_OP_LESS,
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

    FramebufferSetEx gbufferSet{};

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        // create gbuffer images
        Texture positionAttachment = Texture::CreateAttachment(vk, vk.m_SwapChainImageExtent.width, vk.m_SwapChainImageExtent.height, 1,
            VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_ASPECT_COLOR_BIT);

        Texture normalAttachment = Texture::CreateAttachment(vk, vk.m_SwapChainImageExtent.width, vk.m_SwapChainImageExtent.height, 1,
            VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_ASPECT_COLOR_BIT);

        Texture colourAttachment = Texture::CreateAttachment(vk, vk.m_SwapChainImageExtent.width, vk.m_SwapChainImageExtent.height, 1,
            VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_ASPECT_COLOR_BIT);

        VkFormat depthFormat = vk.FindDepthFormat();
        Texture depthAttachment = Texture::CreateAttachment(vk, vk.m_SwapChainImageExtent.width, vk.m_SwapChainImageExtent.height, 1,
            VK_SAMPLE_COUNT_1_BIT, depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_ASPECT_DEPTH_BIT);

        VkFramebuffer gbuffer;
        Vector<VkImageView> gbufferAttachments{ colourAttachment.m_ImageView, positionAttachment.m_ImageView, normalAttachment.m_ImageView, depthAttachment.m_ImageView };
        vk.CreateFramebuffer(gbufferAttachments, gbufferRenderPass, vk.m_SwapChainImageExtent, gbuffer);

        Vector<Texture> textures{ colourAttachment, positionAttachment, normalAttachment, depthAttachment };
        FramebufferEx fb{ textures, gbuffer };
        gbufferSet.m_Framebuffers[i] = fb;
    }    

    // create vertex and index buffer
    Model model;
    LoadModelAssimp(vk, model, "assets/viking_room.obj", true);
    RenderModel m = CreateRenderModelGbuffer(vk, "assets/sponza/sponza.gltf", gbufferProg);

    MeshEx screenQuad = BuildScreenSpaceQuad(vk, g_ScreenSpaceQuadVertexData, g_ScreenSpaceQuadIndexData);

    Texture texture = Texture::CreateTexture(vk, "assets/viking_room.png", VK_FORMAT_R8G8B8A8_UNORM);

    // Shader too probably
    vk.CreateUniformBuffers<FrameLightDataT<NUM_LIGHTS>>(lightsUniformData);
    
    vk.CreateUniformBuffers<MvpData>(mvpUniformData);

    CreateGBufferDescriptorSets(vk, gbufferProg.m_DescriptorSetLayout, texture.m_ImageView, texture.m_Sampler, gbufferDescriptorSets, mvpUniformData);
    CreateLightingPassDescriptorSets(vk, lightPassProg.m_DescriptorSetLayout, gbufferSet, lightPassDescriptorSets, mvpUniformData, lightsUniformData);

    Vector<VkFramebuffer> gbufferFramebuffers{ gbufferSet.m_Framebuffers[0].m_FB, gbufferSet.m_Framebuffers[1].m_FB };

    while (vk.ShouldRun())
    {
        vk.PreFrame();

        for (auto& item : m.m_RenderItems)
        {
            UpdateUniformBufferMat(vk, item.m_Material, lightsUniformData, lightDataCpu);
        }

        UpdateUniformBuffer(vk, mvpUniformData, lightsUniformData, lightDataCpu);

        RecordCommandBuffersV2(vk, 
            gbufferPipeline, gbufferPipelineLayout, gbufferRenderPass,  gbufferDescriptorSets, gbufferFramebuffers, 
            pipeline, lightPassPipelineLayout, vk.m_SwapchainImageRenderPass, lightPassDescriptorSets, vk.m_SwapChainFramebuffers,
            m, screenQuad);

        OnImGui(vk, lightDataCpu);

        vk.PostFrame();
    }

    mvpUniformData.Free(vk);
    lightsUniformData.Free(vk);

    FreeModel(vk, model);
    FreeMesh(vk, screenQuad);

    texture.Free(vk);
    gbufferSet.Free(vk);

    vkDestroyRenderPass(vk.m_LogicalDevice, gbufferRenderPass, nullptr);

    gbufferProg.Free(vk);
    lightPassProg.Free(vk);
    
    vkDestroyPipelineLayout(vk.m_LogicalDevice, gbufferPipelineLayout, nullptr);
    vkDestroyPipeline(vk.m_LogicalDevice, gbufferPipeline, nullptr);
    vkDestroyPipelineLayout(vk.m_LogicalDevice, lightPassPipelineLayout, nullptr);
    vkDestroyPipeline(vk.m_LogicalDevice, pipeline, nullptr);

    return 0;
}
