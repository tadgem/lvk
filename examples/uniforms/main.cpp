#include "VulkanAPI_SDL.h"
#include "spdlog/spdlog.h"
#include "glm/glm.hpp"
#include <array>

struct VertexData
{
    glm::vec3 Position;
    glm::vec3 Colour;

    static VkVertexInputBindingDescription GetBindingDescription() {
        VkVertexInputBindingDescription bindingDescription{};

        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        bindingDescription.stride = sizeof(VertexData);
        bindingDescription.binding = 0;

        return bindingDescription;
    }

    static std::array<VkVertexInputAttributeDescription, 2> GetAttributeDescriptions() {
        std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{};

        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(VertexData, Position);

        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(VertexData, Colour);

        return attributeDescriptions;
    }
};

const std::vector<VertexData> vertices = {
    {{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}},
    {{0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}},
    {{0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}},
    {{-0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}}
};

const std::vector<uint16_t> indices = {
    0, 1, 2, 2, 3, 0
};

void CreateVertexBuffer(VulkanAPI_SDL& vk, VkBuffer& buffer, VkDeviceMemory& deviceMemory)
{
    VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

    // create a CPU side buffer to dump vertex data into
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    vk.CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

    // dump vert data
    void* data;
    vkMapMemory(vk.m_LogicalDevice, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, vertices.data(), bufferSize);
    vkUnmapMemory(vk.m_LogicalDevice, stagingBufferMemory);

    // create GPU side buffer
    vk.CreateBuffer(bufferSize,
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, 
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        buffer, deviceMemory);

    vk.CopyBuffer(stagingBuffer, buffer, bufferSize);

    vkDestroyBuffer(vk.m_LogicalDevice, stagingBuffer, nullptr);
    vkFreeMemory(vk.m_LogicalDevice, stagingBufferMemory, nullptr);
}

void CreateIndexBuffer(VulkanAPI_SDL& vk, VkBuffer& buffer, VkDeviceMemory& deviceMemory)
{
    VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

    // create a CPU side buffer to dump vertex data into
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    vk.CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

    // dump vert data
    void* data;
    vkMapMemory(vk.m_LogicalDevice, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, indices.data(), bufferSize);
    vkUnmapMemory(vk.m_LogicalDevice, stagingBufferMemory);

    // create GPU side buffer
    vk.CreateBuffer(bufferSize,
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        buffer, deviceMemory);

    vk.CopyBuffer(stagingBuffer, buffer, bufferSize);

    vkDestroyBuffer(vk.m_LogicalDevice, stagingBuffer, nullptr);
    vkFreeMemory(vk.m_LogicalDevice, stagingBufferMemory, nullptr);
}

void CreateCommandBuffers(VulkanAPI_SDL& vk, VkPipeline& pipeline)
{
    vk.m_CommandBuffers.resize(vk.m_SwapChainFramebuffers.size());

    VkCommandBufferAllocateInfo allocateInfo{};
    allocateInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocateInfo.commandPool        = vk.m_GraphicsQueueCommandPool;
    allocateInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocateInfo.commandBufferCount = static_cast<uint32_t>(vk.m_CommandBuffers.size());

    if (vkAllocateCommandBuffers(vk.m_LogicalDevice, &allocateInfo, vk.m_CommandBuffers.data()) != VK_SUCCESS)
    {
        spdlog::error("Failed to create Command Buffers!");
        std::cerr << "Failed to create Command Buffers!" << std::endl;
    }
}

void RecordCommandBuffers(VulkanAPI_SDL& vk, VkPipeline& pipeline, VkBuffer& vertexBuffer, VkBuffer& indexBuffer, uint32_t numIndices)
{
    for (uint32_t i = 0; i < vk.m_CommandBuffers.size(); i++)
    {
        VkCommandBufferBeginInfo commandBufferBeginInfo{};
        commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        commandBufferBeginInfo.flags = 0;
        commandBufferBeginInfo.pInheritanceInfo = nullptr;

        if (vkBeginCommandBuffer(vk.m_CommandBuffers[i], &commandBufferBeginInfo) != VK_SUCCESS)
        {
            spdlog::error("Failed to begin recording to Command Buffer!");
            std::cerr << "Failed to begin recording to Command Buffer!" << std::endl;
        }

        // push to example

        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = vk.m_RenderPass;
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
        vkCmdDrawIndexed(vk.m_CommandBuffers[i], numIndices, 1, 0, 0, 0);

        vkCmdEndRenderPass(vk.m_CommandBuffers[i]);

        // pop example

        if (vkEndCommandBuffer(vk.m_CommandBuffers[i]) != VK_SUCCESS)
        {
            spdlog::error("Failed to finalize recording Command Buffer!");
            std::cerr << "Failed to finalize recording Command Buffer!" << std::endl;
        }
    }
}

void ClearCommandBuffers(VulkanAPI_SDL& vk)
{
    for (uint32_t i = 0; i < vk.m_CommandBuffers.size(); i++)
    {
        vkResetCommandBuffer(vk.m_CommandBuffers[i], 0);
    }
}

VkPipeline CreateGraphicsPipeline(VulkanAPI_SDL& vk)
{
    auto vertBin = vk.LoadSpirvBinary("shaders/uniform.vert.spv");
    auto fragBin = vk.LoadSpirvBinary("shaders/uniform.frag.spv");
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

    auto bindingDescription = VertexData::GetBindingDescription();
    auto attributeDescriptions = VertexData::GetAttributeDescriptions();

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

    rasterizerInfo.cullMode = VK_CULL_MODE_NONE;
    // From vk-tutorial: The frontFace variable specifies the vertex order 
    // for faces to be considered front-facing and can be clockwise or counterclockwise.
    // ??
    rasterizerInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
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
    pipelineLayoutInfo.setLayoutCount = 0;
    pipelineLayoutInfo.pSetLayouts = nullptr;
    pipelineLayoutInfo.pushConstantRangeCount = 0;
    pipelineLayoutInfo.pPushConstantRanges = 0;

    VkPipelineLayout pipelineLayout;

    if (vkCreatePipelineLayout(vk.m_LogicalDevice, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS)
    {
        spdlog::error("Failed to create pipeline layout object!");
        std::cerr << "Failed to create pipeline layout object!" << std::endl;
    }

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

    pipelineCreateInfo.layout = pipelineLayout;

    pipelineCreateInfo.renderPass = vk.m_RenderPass;
    pipelineCreateInfo.subpass = 0;

    VkPipeline pipeline;

    if (vkCreateGraphicsPipelines(vk.m_LogicalDevice, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &pipeline) != VK_SUCCESS)
    {
        spdlog::error("Failed to create graphics pipeline!");
        std::cerr << "Failed to create graphics pipeline!" << std::endl;
    }

    vkDestroyShaderModule(vk.m_LogicalDevice, vertShaderModule, nullptr);
    vkDestroyShaderModule(vk.m_LogicalDevice, fragShaderModule, nullptr);

    vkDestroyPipelineLayout(vk.m_LogicalDevice, pipelineLayout, nullptr);
    spdlog::info("Destroyed vertex and fragment shader modules");

    return pipeline;
}



int main()
{
    VulkanAPI_SDL vk;
    vk.CreateWindow(1280, 720);
    vk.InitVulkan();

    VkPipeline pipeline = CreateGraphicsPipeline(vk);
    // draw the triangles
    VkBuffer vertexBuffer; 
    VkDeviceMemory vertexBufferMemory;
    VkBuffer indexBuffer;
    VkDeviceMemory indexBufferMemory;
    CreateVertexBuffer(vk, vertexBuffer, vertexBufferMemory);
    CreateIndexBuffer(vk, indexBuffer, indexBufferMemory);
   
    CreateCommandBuffers(vk, pipeline);
    
    while (vk.ShouldRun())
    {    
        vk.PreFrame();
        
        RecordCommandBuffers(vk, pipeline, vertexBuffer, indexBuffer, indices.size());

        vk.DrawFrame();
        
        vk.PostFrame();

        ClearCommandBuffers(vk);

    }
    vkDestroyBuffer(vk.m_LogicalDevice, vertexBuffer, nullptr);
    vkFreeMemory(vk.m_LogicalDevice, vertexBufferMemory, nullptr);
    vkDestroyBuffer(vk.m_LogicalDevice, indexBuffer, nullptr);
    vkFreeMemory(vk.m_LogicalDevice, indexBufferMemory, nullptr);
    vkDestroyPipeline(vk.m_LogicalDevice, pipeline, nullptr);
    vk.Cleanup();

    return 0;
}