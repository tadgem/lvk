#include "VulkanAPI_SDL.h"
#include "spdlog/spdlog.h"
#include "glm/glm.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <array>
using namespace lvk;

struct VertexDataCol
{
    glm::vec3 Position;
    glm::vec3 Colour;
    glm::vec2 UV;

    static VkVertexInputBindingDescription GetBindingDescription() {
        VkVertexInputBindingDescription bindingDescription{};

        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        bindingDescription.stride = sizeof(VertexDataCol);
        bindingDescription.binding = 0;

        return bindingDescription;
    }

    static std::array<VkVertexInputAttributeDescription, 3> GetAttributeDescriptions() {
        std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};

        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(VertexDataCol, Position);

        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(VertexDataCol, Colour);

        attributeDescriptions[2].binding = 0;
        attributeDescriptions[2].location = 2;
        attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[2].offset = offsetof(VertexDataCol, UV);

        return attributeDescriptions;
    }
};

struct MvpData {
    glm::mat4 Model;
    glm::mat4 View;
    glm::mat4 Proj;
};
const std::vector<VertexDataCol> vertices = {
    { { -0.5f, -0.5f , 0.0f}, { 1.0f, 0.0f, 0.0f }, { 1.0f, 0.0f } },
    { {0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f} },
    { {0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f} },
    { {-0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f} }
};
const std::vector<uint16_t> indices = {
    0, 1, 2, 2, 3, 0
};

static std::vector<VkBuffer>            uniformBuffers;
static std::vector<VmaAllocation>       uniformBuffersMemory;
static std::vector<void*>               uniformBuffersMapped;
static std::vector<VkDescriptorSet>     descriptorSets;

void CreateVertexBuffer(VulkanAPI_SDL& vk, VkBuffer& buffer, VmaAllocation& deviceMemory)
{
    VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

    // create a CPU side buffer to dump vertex data into
    VkBuffer stagingBuffer;
    VmaAllocation stagingBufferMemory;
    vk.CreateBufferVMA(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

    // dump vert data
    void* data;
    vmaMapMemory(vk.m_Allocator, stagingBufferMemory, &data);
    memcpy(data, vertices.data(), bufferSize);
    vmaUnmapMemory(vk.m_Allocator, stagingBufferMemory);

    // create GPU side buffer
    vk.CreateBufferVMA(bufferSize,
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, 
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        buffer, deviceMemory);

    vk.CopyBuffer(stagingBuffer, buffer, bufferSize);

    vkDestroyBuffer(vk.m_LogicalDevice, stagingBuffer, nullptr);
    vmaFreeMemory(vk.m_Allocator, stagingBufferMemory);
}

void CreateIndexBuffer(VulkanAPI_SDL& vk, VkBuffer& buffer, VmaAllocation& deviceMemory)
{
    VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

    // create a CPU side buffer to dump vertex data into
    VkBuffer stagingBuffer;
    VmaAllocation stagingBufferMemory;
    vk.CreateBufferVMA(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

    // dump vert data
    void* data;
    vmaMapMemory(vk.m_Allocator, stagingBufferMemory, &data);
    memcpy(data, indices.data(), bufferSize);
    vmaUnmapMemory(vk.m_Allocator, stagingBufferMemory);

    // create GPU side buffer
    vk.CreateBufferVMA(bufferSize,
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        buffer, deviceMemory);

    vk.CopyBuffer(stagingBuffer, buffer, bufferSize);

    vkDestroyBuffer(vk.m_LogicalDevice, stagingBuffer, nullptr);
    vmaFreeMemory(vk.m_Allocator, stagingBufferMemory);
}

// this probably could be created by spirv reflect
// this should likely be part of shader abstraction
void CreateDescriptorSetLayout(VulkanAPI_SDL& vk, DescriptorSetLayoutData& layoutData,  VkDescriptorSetLayout& descriptorSetLayout)
{
    VK_CHECK(vkCreateDescriptorSetLayout(vk.m_LogicalDevice, &layoutData.m_CreateInfo, nullptr, & descriptorSetLayout))
}

void CreateDescriptorSetLayoutV2(VulkanAPI_SDL& vk, std::vector<DescriptorSetLayoutData>& vertLayoutDatas, std::vector<DescriptorSetLayoutData>& fragLayoutDatas, VkDescriptorSetLayout& descriptorSetLayout)
{ 
    //VK_CHECK(vkCreateDescriptorSetLayout(vk.m_LogicalDevice, &layoutData.m_CreateInfo, nullptr, &descriptorSetLayout))
    std::vector<VkDescriptorSetLayoutBinding> bindings;
    bindings.resize(vertLayoutDatas.size() + fragLayoutDatas.size());

    // .. do the things
    uint8_t count = 0;
    for (auto& vertLayoutData : vertLayoutDatas)
    {
        bindings[count] = vertLayoutData.m_Bindings.front();
        count++;
    }

    for (auto& fragLayoutData : fragLayoutDatas)
    {
        bindings[count] = fragLayoutData.m_Bindings.front();
        count++;
    }

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();

    VK_CHECK(vkCreateDescriptorSetLayout(vk.m_LogicalDevice, &layoutInfo, nullptr, &descriptorSetLayout))
}

void CreateUniformBuffers(VulkanAPI_SDL& vk)
{
    VkDeviceSize bufferSize = sizeof(MvpData);

    uniformBuffers.resize(vk.MAX_FRAMES_IN_FLIGHT);
    uniformBuffersMemory.resize(vk.MAX_FRAMES_IN_FLIGHT);
    uniformBuffersMapped.resize(vk.MAX_FRAMES_IN_FLIGHT);

    for (size_t i = 0; i < vk.MAX_FRAMES_IN_FLIGHT; i++) {
        vk.CreateBufferVMA(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uniformBuffers[i], uniformBuffersMemory[i]);

        VK_CHECK(vmaMapMemory(vk.m_Allocator, uniformBuffersMemory[i], &uniformBuffersMapped[i]))
    }

}

void RecordCommandBuffers(VulkanAPI_SDL& vk, VkPipeline& pipeline, VkPipelineLayout& pipelineLayout, VkBuffer& vertexBuffer, VkBuffer& indexBuffer, uint32_t numIndices)
{
    for (uint32_t i = 0; i < vk.m_CommandBuffers.size(); i++)
    {
        VkCommandBufferBeginInfo commandBufferBeginInfo{};
        commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        commandBufferBeginInfo.flags = 0;
        commandBufferBeginInfo.pInheritanceInfo = nullptr;

        VK_CHECK(vkBeginCommandBuffer(vk.m_CommandBuffers[i], &commandBufferBeginInfo))
        

        // push to example

        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = vk.m_SwapchainImageRenderPass;
        renderPassInfo.framebuffer = vk.m_SwapChainFramebuffers[i];
        renderPassInfo.renderArea.offset = { 0,0 };
        renderPassInfo.renderArea.extent = vk.m_SwapChainImageExtent;

        VkClearValue clearValue = { {{0.0f, 0.0f, 0.0f, 1.0f}} };
        renderPassInfo.clearValueCount = 1;
        renderPassInfo.pClearValues = &clearValue;

        vkCmdBeginRenderPass(vk.m_CommandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        vkCmdBindPipeline(vk.m_CommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

        VkBuffer vertexBuffers[]{ vertexBuffer };
        VkDeviceSize sizes[] = { 0 };
        vkCmdBindVertexBuffers(vk.m_CommandBuffers[i], 0, 1, vertexBuffers, sizes);

        vkCmdBindIndexBuffer(vk.m_CommandBuffers[i], indexBuffer, 0, VK_INDEX_TYPE_UINT16);
        vkCmdBindDescriptorSets(vk.m_CommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets[vk.GetFrameIndex()], 0, nullptr);
        vkCmdDrawIndexed(vk.m_CommandBuffers[i], numIndices, 1, 0, 0, 0);

        vkCmdEndRenderPass(vk.m_CommandBuffers[i]);

        // pop example

        VK_CHECK(vkEndCommandBuffer(vk.m_CommandBuffers[i]))
    }
}

VkPipeline CreateGraphicsPipeline(VulkanAPI_SDL& vk, VkDescriptorSetLayout& descriptorSetLayout, VkPipelineLayout& layout, std::vector<char>& vertBin, std::vector<char>& fragBin)
{
    VkShaderModule vertShaderModule = vk.CreateShaderModule(vertBin);
    VkShaderModule fragShaderModule = vk.CreateShaderModule(fragBin);

    spdlog::info("Loaded vertex and fragment shaders & created shader modules");

    // ..
    VkPipelineShaderStageCreateInfo vertexShaderStageInfo{};
    vertexShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertexShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertexShaderStageInfo.module = vertShaderModule;
    vertexShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName = "main";

    std::vector<VkPipelineShaderStageCreateInfo> shaderStageCreateInfos = { vertexShaderStageInfo, fragShaderStageInfo };

    auto bindingDescription = VertexDataCol::GetBindingDescription();
    auto attributeDescriptions = VertexDataCol::GetAttributeDescriptions();

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

    VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo{};
    inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.x = 0.0f;
    viewport.width = static_cast<float>(vk.m_SwapChainImageExtent.width);
    viewport.height = static_cast<float>(vk.m_SwapChainImageExtent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor{};
    scissor.offset = { 0,0 };
    scissor.extent = vk.m_SwapChainImageExtent;

    VkPipelineViewportStateCreateInfo viewportInfo{};
    viewportInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportInfo.viewportCount = 1;
    viewportInfo.pViewports = &viewport;
    viewportInfo.scissorCount = 1;
    viewportInfo.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizerInfo{};
    rasterizerInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizerInfo.depthClampEnable = VK_FALSE;

    rasterizerInfo.rasterizerDiscardEnable = VK_FALSE;
    // using anything other than FILL requires enableing a gpu feature.
    rasterizerInfo.polygonMode = VK_POLYGON_MODE_FILL;
    // thickness of lines in terms of pixels. > 1.0f requires wide lines gpu feature.
    rasterizerInfo.lineWidth = 1.0f;

    //rasterizerInfo.cullMode = VK_CULL_MODE_NONE;
    //// From vk-tutorial: The frontFace variable specifies the vertex order 
    //// for faces to be considered front-facing and can be clockwise or counterclockwise.
    //// ??
    //rasterizerInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;

    rasterizerInfo.cullMode = VK_CULL_MODE_NONE;
    rasterizerInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

    rasterizerInfo.depthBiasEnable = VK_FALSE;
    rasterizerInfo.depthBiasConstantFactor = 0.0f;
    rasterizerInfo.depthBiasClamp = 0.0f;
    rasterizerInfo.depthBiasSlopeFactor = 0.0f;

    VkPipelineMultisampleStateCreateInfo multisampleInfo{};
    multisampleInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampleInfo.sampleShadingEnable = VK_FALSE;
    multisampleInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampleInfo.minSampleShading = 1.0f;
    multisampleInfo.pSampleMask = nullptr;
    multisampleInfo.alphaToCoverageEnable = VK_FALSE;
    multisampleInfo.alphaToOneEnable = VK_FALSE;

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
        VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT |
        VK_COLOR_COMPONENT_A_BIT;

    colorBlendAttachment.blendEnable = VK_FALSE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo colorBlendStateInfo{};
    colorBlendStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlendStateInfo.logicOpEnable = VK_FALSE;
    colorBlendStateInfo.logicOp = VK_LOGIC_OP_COPY;
    colorBlendStateInfo.attachmentCount = 1;
    colorBlendStateInfo.pAttachments = &colorBlendAttachment;

    VkDynamicState dynamicStates[] =
    {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_LINE_WIDTH
    };

    VkPipelineDynamicStateCreateInfo dynamicStateInfo{};
    dynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicStateInfo.dynamicStateCount = 2;
    dynamicStateInfo.pDynamicStates = dynamicStates;

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;
    pipelineLayoutInfo.pushConstantRangeCount = 0;
    pipelineLayoutInfo.pPushConstantRanges = 0;

    VK_CHECK(vkCreatePipelineLayout(vk.m_LogicalDevice, &pipelineLayoutInfo, nullptr, &layout))
    
    VkGraphicsPipelineCreateInfo pipelineCreateInfo{};
    pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineCreateInfo.stageCount = 2;
    pipelineCreateInfo.pStages = shaderStageCreateInfos.data();

    pipelineCreateInfo.pVertexInputState = &vertexInputInfo;
    pipelineCreateInfo.pInputAssemblyState = &inputAssemblyInfo;
    pipelineCreateInfo.pViewportState = &viewportInfo;
    pipelineCreateInfo.pRasterizationState = &rasterizerInfo;
    pipelineCreateInfo.pMultisampleState = &multisampleInfo;
    pipelineCreateInfo.pDepthStencilState = nullptr;
    pipelineCreateInfo.pColorBlendState = &colorBlendStateInfo;
    pipelineCreateInfo.pDynamicState = nullptr;

    pipelineCreateInfo.layout = layout;

    pipelineCreateInfo.renderPass = vk.m_SwapchainImageRenderPass;
    pipelineCreateInfo.subpass = 0;

    VkPipeline pipeline;

    VK_CHECK(vkCreateGraphicsPipelines(vk.m_LogicalDevice, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &pipeline))

    vkDestroyShaderModule(vk.m_LogicalDevice, vertShaderModule, nullptr);
    vkDestroyShaderModule(vk.m_LogicalDevice, fragShaderModule, nullptr);

    // vkDestroyPipelineLayout(vk.m_LogicalDevice, pipelineLayout, nullptr);
    spdlog::info("Destroyed vertex and fragment shader modules");

    return pipeline;
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
    std::vector<VkDescriptorSetLayout> layouts(vk.MAX_FRAMES_IN_FLIGHT, descriptorSetLayout);
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = vk.m_DescriptorPool;
    allocInfo.descriptorSetCount = static_cast<uint32_t>(vk.MAX_FRAMES_IN_FLIGHT);
    allocInfo.pSetLayouts = layouts.data();

    descriptorSets.resize(vk.MAX_FRAMES_IN_FLIGHT);
    VK_CHECK(vkAllocateDescriptorSets(vk.m_LogicalDevice, &allocInfo, descriptorSets.data()));

    for (size_t i = 0; i < vk.MAX_FRAMES_IN_FLIGHT; i++) {
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

void CreateTextureImage(VulkanAPI_SDL& vk, const String& texturePath, VkImage& textureImage, VkDeviceMemory& textureMemory)
{
    int texWidth, texHeight, texChannels;
    stbi_uc* pixels = stbi_load(texturePath.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    VkDeviceSize imageSize = texWidth * texHeight * 4;

    if (!pixels)
    {
        spdlog::error("Failed to load texture image at path {}", texturePath);
        return;
    }

    // create staging buffer to copy texture to gpu
    VkBuffer stagingBuffer;
    VmaAllocation stagingBufferMemory;
    constexpr VkBufferUsageFlags bufferUsageFlags = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    constexpr VkMemoryPropertyFlags memoryPropertiesFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    vk.CreateBufferVMA(imageSize, bufferUsageFlags, memoryPropertiesFlags, stagingBuffer, stagingBufferMemory);

    void* data;
    vmaMapMemory(vk.m_Allocator, stagingBufferMemory, &data);
    memcpy(data, pixels, static_cast<size_t>(imageSize));
    vmaUnmapMemory(vk.m_Allocator, stagingBufferMemory);
    stbi_image_free(pixels);


    vk.CreateImage(texWidth, texHeight, 
        VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, 
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
        textureImage, textureMemory);

    vk.TransitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    vk.CopyBufferToImage(stagingBuffer, textureImage, texWidth, texHeight);
    vk.TransitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    
    vkDestroyBuffer(vk.m_LogicalDevice, stagingBuffer, nullptr);
    vmaFreeMemory(vk.m_Allocator, stagingBufferMemory);
}

void CreateTextureImageView(VulkanAPI_SDL& vk, VkImage& image, VkImageView& imageView)
{
    vk.CreateImageView(image, VK_FORMAT_R8G8B8A8_SRGB, imageView);
}

std::vector<DescriptorSetLayoutData> CreateDescriptorSetLayoutDatasSVR(VulkanAPI_SDL& vk, std::vector<char>& stageBin)
{
    SpvReflectShaderModule shaderReflectModule;
    SpvReflectResult result = spvReflectCreateShaderModule(stageBin.size(), stageBin.data(), &shaderReflectModule);

    uint32_t descriptorSetCount = 0;
    spvReflectEnumerateDescriptorSets(&shaderReflectModule, &descriptorSetCount, nullptr);

    std::vector<SpvReflectDescriptorSet*> reflectedDescriptorSets;
    reflectedDescriptorSets.resize(descriptorSetCount);
    spvReflectEnumerateDescriptorSets(&shaderReflectModule, &descriptorSetCount, &reflectedDescriptorSets[0]);

    std::vector<DescriptorSetLayoutData> layoutDatas(descriptorSetCount, DescriptorSetLayoutData{});
    
    for (int i = 0; i < reflectedDescriptorSets.size(); i++)
    {
        const SpvReflectDescriptorSet& reflectedSet = *reflectedDescriptorSets[i];
        DescriptorSetLayoutData& layoutData = layoutDatas[i];

        layoutData.m_Bindings.resize(reflectedSet.binding_count);
        for (int bc = 0; bc < reflectedSet.binding_count; bc++)
        {
            const SpvReflectDescriptorBinding& reflectedBinding = *reflectedSet.bindings[bc];
            VkDescriptorSetLayoutBinding& layoutBinding = layoutData.m_Bindings[bc];
            layoutBinding.binding = reflectedBinding.binding;
            layoutBinding.descriptorType = static_cast<VkDescriptorType>(reflectedBinding.descriptor_type);
            layoutBinding.descriptorCount = 1; // sus
            for (uint32_t i_dim = 0; i_dim < reflectedBinding.array.dims_count; ++i_dim) {
                layoutBinding.descriptorCount *= reflectedBinding.array.dims[i_dim];
            }
            layoutBinding.stageFlags = static_cast<VkShaderStageFlagBits>(shaderReflectModule.shader_stage);
            layoutData.m_BindingDatas.push_back(DescriptorSetLayoutBindingData{ reflectedBinding.block.size });
        }

        layoutData.m_SetNumber = reflectedSet.set;
        layoutData.m_CreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutData.m_CreateInfo.bindingCount = reflectedSet.binding_count;
        layoutData.m_CreateInfo.pBindings = layoutData.m_Bindings.data();
    }

    return layoutDatas;

}

void CreateTextureSampler(VulkanAPI_SDL& vk, VkImageView& imageView, VkSampler& sampler)
{
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;

    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(vk.m_PhysicalDevice, &properties);
    
    samplerInfo.anisotropyEnable = VK_TRUE;
    samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;

    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f;

    VK_CHECK(vkCreateSampler(vk.m_LogicalDevice, &samplerInfo, nullptr, &sampler))


}

int main()
{
    VulkanAPI_SDL vk;
    vk.CreateWindow(1280, 720);
    vk.InitVulkan();

    auto vertBin = vk.LoadSpirvBinary("shaders/texture.vert.spv");
    auto fragBin = vk.LoadSpirvBinary("shaders/texture.frag.spv");

    auto vertexLayoutDatas = CreateDescriptorSetLayoutDatasSVR(vk, vertBin);
    auto fragmentLayoutDatas = CreateDescriptorSetLayoutDatasSVR(vk, fragBin);
    VkDescriptorSetLayout descriptorSetLayout;
    CreateDescriptorSetLayoutV2(vk,vertexLayoutDatas, fragmentLayoutDatas, descriptorSetLayout);

    VkImage textureImage;
    VkDeviceMemory textureMemory;
    CreateTextureImage(vk, "assets/crate.jpg", textureImage, textureMemory);
    VkImageView imageView;
    CreateTextureImageView(vk, textureImage, imageView);
    VkSampler imageSampler;
    CreateTextureSampler(vk, imageView, imageSampler);

    VkPipelineLayout pipelineLayout;
    VkPipeline pipeline = CreateGraphicsPipeline(vk, descriptorSetLayout, pipelineLayout, vertBin, fragBin);

    // create vertex and index buffer
    VkBuffer vertexBuffer; 
    VmaAllocation vertexBufferMemory;
    VkBuffer indexBuffer;
    VmaAllocation indexBufferMemory;

    CreateVertexBuffer(vk, vertexBuffer, vertexBufferMemory);
    CreateIndexBuffer(vk, indexBuffer, indexBufferMemory);
    CreateUniformBuffers(vk);

    CreateDescriptorSets(vk, descriptorSetLayout, imageView, imageSampler);

    while (vk.ShouldRun())
    {    
        vk.PreFrame();
        
        UpdateUniformBuffer(vk);

        RecordCommandBuffers(vk, pipeline, pipelineLayout,  vertexBuffer, indexBuffer, indices.size());

        vk.DrawFrame();
        
        vk.PostFrame();

        vk.ClearCommandBuffers();

    }

    for (size_t i = 0; i < vk.MAX_FRAMES_IN_FLIGHT; i++) {
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
    vk.Cleanup();

    return 0;
}
