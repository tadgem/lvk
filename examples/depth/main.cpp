#include "example-common.h"
#include "lvk/Mesh.h"
using namespace lvk;

const std::vector<lvk::VertexDataPosColUv> vertices = {
    {{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
    {{0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
    {{0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
    {{-0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}},

    {{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
    {{0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
    {{0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
    {{-0.5f, 0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}}
};

const std::vector<uint32_t> indices = {
    0, 1, 2, 2, 3, 0,
    4, 5, 6, 6, 7, 4
};

static std::vector<VkBuffer>            uniformBuffers;
static std::vector<VmaAllocation>       uniformBuffersMemory;
static std::vector<void*>               uniformBuffersMapped;
static std::vector<VkDescriptorSet>     descriptorSets;

void RecordCommandBuffers(VulkanAPI_SDL& vk, VkPipeline& pipeline, VkPipelineLayout& pipelineLayout, VkBuffer& vertexBuffer, VkBuffer& indexBuffer, uint32_t numIndices)
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

        VkBuffer vertexBuffers[]{ vertexBuffer };
        VkDeviceSize sizes[] = { 0 };

        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, sizes);
        vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets[frameIndex], 0, nullptr);
        vkCmdDrawIndexed(commandBuffer, numIndices, 1, 0, 0, 0);

        vkCmdEndRenderPass(commandBuffer);

    });
}

void UpdateUniformBuffer(VulkanAPI_SDL& vk)
{
    static auto startTime = std::chrono::high_resolution_clock::now();

    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

    MvpData ubo{};
    ubo.Model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));

    ubo.View = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));

    ubo.Proj = glm::perspective(glm::radians(45.0f), vk.m_SwapChainImageExtent.width / (float)vk.m_SwapChainImageExtent.height, 0.1f, 10.0f);
    ubo.Proj[1][1] *= -1;

    memcpy(uniformBuffersMapped[vk.GetFrameIndex()], &ubo, sizeof(ubo));
}

void CreateDescriptorSets(VulkanAPI_SDL& vk, VkDescriptorSetLayout& descriptorSetLayout, VkImageView& textureImageView, VkSampler& textureSampler)
{
    std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, descriptorSetLayout);
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = vk.m_DescriptorPool;
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
    VulkanAPI_SDL vk;
    vk.Start(1280, 720);

    auto vertBin = vk.LoadSpirvBinary("shaders/texture.vert.spv");
    auto fragBin = vk.LoadSpirvBinary("shaders/texture.frag.spv");

    auto vertexLayoutDatas = vk.ReflectDescriptorSetLayouts(vertBin);
    auto fragmentLayoutDatas = vk.ReflectDescriptorSetLayouts(fragBin);
    VkDescriptorSetLayout descriptorSetLayout;
    vk.CreateDescriptorSetLayout(vertexLayoutDatas, fragmentLayoutDatas, descriptorSetLayout);

    VkImage textureImage;
    VkImageView imageView;
    VkDeviceMemory textureMemory;
    vk.CreateTexture("assets/crate.jpg", VK_FORMAT_R8G8B8A8_UNORM, textureImage, imageView, textureMemory);
    VkSampler imageSampler;
    vk.CreateImageSampler(imageView, 1, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_REPEAT, imageSampler);

    VkPipelineLayout pipelineLayout;

    VkPipeline pipeline = vk.CreateRasterizationGraphicsPipeline(
        vertBin, fragBin,
        descriptorSetLayout, Vector<VkVertexInputBindingDescription>{VertexDataPosColUv::GetBindingDescription() }, VertexDataPosColUv::GetAttributeDescriptions(),
        vk.m_SwapchainImageRenderPass,
        vk.m_SwapChainImageExtent.width, vk.m_SwapChainImageExtent.height,
        VK_POLYGON_MODE_FILL,
        VK_CULL_MODE_BACK_BIT,
        false, // no msaa atm
        VK_COMPARE_OP_LESS,
        pipelineLayout);

    // create vertex and index buffer
    VkBuffer vertexBuffer; 
    VmaAllocation vertexBufferMemory;
    VkBuffer indexBuffer;
    VmaAllocation indexBufferMemory;

    vk.CreateVertexBuffer<VertexDataPosColUv>(vertices, vertexBuffer, vertexBufferMemory);
    vk.CreateIndexBuffer(indices, indexBuffer, indexBufferMemory);
    vk.CreateUniformBuffers<MvpData>(uniformBuffers, uniformBuffersMemory, uniformBuffersMapped);

    CreateDescriptorSets(vk, descriptorSetLayout, imageView, imageSampler);

    while (vk.ShouldRun())
    {    
        vk.PreFrame();
        
        UpdateUniformBuffer(vk);
        RecordCommandBuffers(vk, pipeline, pipelineLayout, vertexBuffer, indexBuffer, indices.size());
        vk.PostFrame();
    }

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vmaUnmapMemory(vk.m_Allocator, uniformBuffersMemory[i]);
        vkDestroyBuffer(vk.m_LogicalDevice, uniformBuffers[i], nullptr);
        vmaFreeMemory(vk.m_Allocator, uniformBuffersMemory[i]);
    }

    vkDestroySampler(vk.m_LogicalDevice, imageSampler, nullptr);
    vkDestroyImageView(vk.m_LogicalDevice, imageView, nullptr);
    vkDestroyImage(vk.m_LogicalDevice, textureImage, nullptr);
    vkFreeMemory(vk.m_LogicalDevice, textureMemory, nullptr);
    vkDestroyDescriptorSetLayout(vk.m_LogicalDevice, descriptorSetLayout, nullptr);
    vkDestroyPipelineLayout(vk.m_LogicalDevice, pipelineLayout, nullptr);
    vkDestroyBuffer(vk.m_LogicalDevice, vertexBuffer, nullptr);
    vmaFreeMemory(vk.m_Allocator, vertexBufferMemory);
    vkDestroyBuffer(vk.m_LogicalDevice, indexBuffer, nullptr);
    vmaFreeMemory(vk.m_Allocator, indexBufferMemory);
    vkDestroyPipeline(vk.m_LogicalDevice, pipeline, nullptr);

    return 0;
}
