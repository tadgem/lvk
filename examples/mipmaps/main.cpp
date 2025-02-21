#include "example-common.h"
#include "lvk/Shader.h"
using namespace lvk;

static std::vector<VkBuffer>            uniformBuffers;
static std::vector<VmaAllocation>       uniformBuffersMemory;
static std::vector<void*>               uniformBuffersMapped;
static std::vector<VkDescriptorSet>     descriptorSets;

void RecordGraphicsCommandBuffers(VkState & vk, VkPipeline& pipeline, VkPipelineLayout& pipelineLayout, Model& model)
{
    lvk::commands::RecordGraphicsCommands(vk, [&](VkCommandBuffer& commandBuffer, uint32_t frameIndex) {
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

void UpdateUniformBuffer(VkState & vk)
{
    static auto startTime = std::chrono::high_resolution_clock::now();

    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

    MvpData ubo{};
    ubo.Model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));

    ubo.View = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));

    if (vk.m_SwapChainImageExtent.width > 0 || vk.m_SwapChainImageExtent.height)
    {
        ubo.Proj = glm::perspective(glm::radians(45.0f), vk.m_SwapChainImageExtent.width / (float)vk.m_SwapChainImageExtent.height, 0.1f, 300.0f);
        ubo.Proj[1][1] *= -1;
    }

    memcpy(uniformBuffersMapped[vk.m_CurrentFrameIndex], &ubo, sizeof(ubo));
}

void CreateGraphicsDescriptorSets(VkState & vk, VkDescriptorSetLayout& descriptorSetLayout, VkImageView& textureImageView, VkSampler& textureSampler)
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
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = uniformBuffers[i];
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(MvpData);

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
        descriptorWrites[0].pBufferInfo = &bufferInfo;

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

int main()
{
    VkState vk = init::Create<VkSDL>("Im3D Multiview", 1920, 1080, false);
    bool enableMSAA = false;

    ShaderProgram tex_prog = ShaderProgram::CreateGraphicsFromSourcePath(
        vk, "shaders/texture.vert", "shaders/texture.frag");

    uint32_t mipLevels;
    VkImage textureImage;
    VkImageView imageView;
    VkDeviceMemory textureMemory;
    VkSampler imageSampler;
    textures::CreateTexture(vk, "assets/viking_room.png", VK_FORMAT_R8G8B8A8_UNORM, textureImage, imageView, textureMemory, &mipLevels);
    textures::CreateImageSampler(vk, imageView, mipLevels, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_REPEAT, imageSampler);

    VkPipelineLayout pipelineLayout;
    auto vertexDescription = VertexDataPosUv::GetVertexDescription();
    VkPipeline pipeline = lvk::pipelines::CreateRasterPipeline(vk,
        tex_prog, vertexDescription,
        vk.m_SwapchainImageRenderPass, vk.m_SwapChainImageExtent,
        VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, false, // no msaa atm
        VK_COMPARE_OP_LESS, pipelineLayout);

    // create vertex and index buffer
    Model model;
    LoadModelAssimp(vk, model, "assets/viking_room.obj");

    buffers::CreateUniformBuffers<MvpData>(vk, uniformBuffers, uniformBuffersMemory, uniformBuffersMapped);

    CreateGraphicsDescriptorSets(vk, tex_prog.m_DescriptorSetLayout, imageView,
                                 imageSampler);

    while (vk.m_ShouldRun)
    {    
        vk.m_Backend->PreFrame(vk);
        
        UpdateUniformBuffer(vk);

        if (ImGui::Begin("Help"))
        {

        }
        ImGui::End();
        RecordGraphicsCommandBuffers(vk, pipeline, pipelineLayout, model);

        vk.m_Backend->PostFrame(vk);
    }

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vmaUnmapMemory(vk.m_Allocator, uniformBuffersMemory[i]);
        vkDestroyBuffer(vk.m_LogicalDevice, uniformBuffers[i], nullptr);
        vmaFreeMemory(vk.m_Allocator, uniformBuffersMemory[i]);
    }
    FreeModel(vk, model);

    vkDestroySampler(vk.m_LogicalDevice, imageSampler, nullptr);
    vkDestroyImageView(vk.m_LogicalDevice, imageView, nullptr);
    vkDestroyImage(vk.m_LogicalDevice, textureImage, nullptr);
    vkFreeMemory(vk.m_LogicalDevice, textureMemory, nullptr);
    vkDestroyPipelineLayout(vk.m_LogicalDevice, pipelineLayout, nullptr);
    vkDestroyPipeline(vk.m_LogicalDevice, pipeline, nullptr);

    return 0;
}
