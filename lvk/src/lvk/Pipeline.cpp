#include "lvk/Pipeline.h"
#include "spdlog/spdlog.h"

namespace lvk::pipelines {

VkPipeline CreateRasterPipeline(
    VkState &vk, ShaderProgram &shader,
    VertexDescription& vertexDescription,
    RasterState& rasterState,
    VkRenderPass &pipelineRenderPass, VkExtent2D resolution,
    VkCompareOp depthCompareOp, VkPipelineLayout &pipelineLayout,
    uint32_t colorAttachmentCount) {
  VkShaderModule vertShaderModule =
      CreateShaderModule(vk, shader.m_Stages[0].m_StageBinary);
  VkShaderModule fragShaderModule =
      CreateShaderModule(vk, shader.m_Stages[1].m_StageBinary);

  spdlog::info("Loaded vertex and fragment shaders & created shader modules");

  // ..
  VkPipelineShaderStageCreateInfo vertexShaderStageInfo{};
  vertexShaderStageInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  vertexShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
  vertexShaderStageInfo.module = vertShaderModule;
  vertexShaderStageInfo.pName = "main";

  VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
  fragShaderStageInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  fragShaderStageInfo.module = fragShaderModule;
  fragShaderStageInfo.pName = "main";

  std::vector<VkPipelineShaderStageCreateInfo> shaderStageCreateInfos = {
      vertexShaderStageInfo, fragShaderStageInfo};

  VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
  vertexInputInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  vertexInputInfo.vertexBindingDescriptionCount =
      static_cast<uint32_t>(vertexDescription.m_BindingDescriptions.size());
  vertexInputInfo.pVertexBindingDescriptions = vertexDescription.m_BindingDescriptions.data();
  vertexInputInfo.vertexAttributeDescriptionCount =
      static_cast<uint32_t>(vertexDescription.m_AttributeDescriptions.size());
  vertexInputInfo.pVertexAttributeDescriptions =
      vertexDescription.m_AttributeDescriptions.data();

  VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo{};
  inputAssemblyInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;

  VkViewport viewport{};
  viewport.x = 0.0f;
  viewport.x = 0.0f;
  viewport.width = static_cast<float>(resolution.width);
  viewport.height = static_cast<float>(resolution.height);
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;

  VkRect2D scissor{};
  scissor.offset = {0, 0};
  scissor.extent = VkExtent2D{resolution.width, resolution.height};

  VkPipelineViewportStateCreateInfo viewportInfo{};
  viewportInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  viewportInfo.viewportCount = 1;
  viewportInfo.pViewports = &viewport;
  viewportInfo.scissorCount = 1;
  viewportInfo.pScissors = &scissor;

  VkPipelineRasterizationStateCreateInfo rasterizerInfo{};
  rasterizerInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  rasterizerInfo.depthClampEnable = VK_FALSE;

  rasterizerInfo.rasterizerDiscardEnable = VK_FALSE;
  // using anything other than FILL requires enableing a gpu feature.
  rasterizerInfo.polygonMode = rasterState.m_PolygonMode;
  // thickness of lines in terms of pixels. > 1.0f requires wide lines gpu feature.
  rasterizerInfo.lineWidth = 1.0f;

  // rasterizerInfo.cullMode = VK_CULL_MODE_NONE;
  //// From vk-tutorial: The frontFace variable specifies the vertex order
  //// for faces to be considered front-facing and can be clockwise or counterclockwise. / ??
  rasterizerInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;

  rasterizerInfo.cullMode = rasterState.m_CullMode;
  rasterizerInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

  rasterizerInfo.depthBiasEnable = VK_FALSE;
  rasterizerInfo.depthBiasConstantFactor = 0.0f;
  rasterizerInfo.depthBiasClamp = 0.0f;
  rasterizerInfo.depthBiasSlopeFactor = 0.0f;

  VkSampleCountFlagBits sampleCount = VK_SAMPLE_COUNT_1_BIT;

  if (rasterState.m_EnableMSAA) {
    sampleCount = vk.m_MaxMsaaSamples;
  }

  // ToDo : Do something with enableMultisampling here
  VkPipelineMultisampleStateCreateInfo multisampleInfo{};
  multisampleInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  multisampleInfo.sampleShadingEnable =
      static_cast<VkBool32>(rasterState.m_EnableMSAA);
  multisampleInfo.rasterizationSamples = sampleCount;
  multisampleInfo.minSampleShading = .2f;
  multisampleInfo.pSampleMask = nullptr;
  multisampleInfo.alphaToCoverageEnable = VK_FALSE;
  multisampleInfo.alphaToOneEnable = VK_FALSE;

  Vector<VkPipelineColorBlendAttachmentState> colorAttachmentBlendStates;

  for (uint32_t i = 0; i < colorAttachmentCount; i++) {
    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

    colorAttachmentBlendStates.push_back(colorBlendAttachment);
  }

  VkPipelineColorBlendStateCreateInfo colorBlendStateInfo{};
  colorBlendStateInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  colorBlendStateInfo.logicOpEnable = VK_FALSE;
  colorBlendStateInfo.logicOp = VK_LOGIC_OP_COPY;
  colorBlendStateInfo.attachmentCount =
      static_cast<uint32_t>(colorAttachmentBlendStates.size());
  colorBlendStateInfo.pAttachments = colorAttachmentBlendStates.data();

  VkDynamicState dynamicStates[] = {
      VK_DYNAMIC_STATE_VIEWPORT,
      VK_DYNAMIC_STATE_SCISSOR,
  };

  VkPipelineDynamicStateCreateInfo dynamicStateInfo{};
  dynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
  dynamicStateInfo.dynamicStateCount =
      sizeof(dynamicStates) / sizeof(VkDynamicState);
  dynamicStateInfo.pDynamicStates = dynamicStates;

  VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
  pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipelineLayoutInfo.setLayoutCount = 1;
  pipelineLayoutInfo.pSetLayouts = &shader.m_DescriptorSetLayout;

  // update
  // valid combos:
  // 1 stage has 1 push constant block
  // both stages share the same push constant block
  // each stage has a separate push constant block
  // e.g. only ever 1 block per stage

  Vector<VkPushConstantRange> pushConstantRanges{};

  for (auto &stage : shader.m_Stages) {
    if (stage.m_PushConstants.empty()) {
      continue;
    }

    if (stage.m_PushConstants.size() > 1) {
      spdlog::error(
          "VulkanAPI : CreateRasterizationPipeline : Supplied stage has more than 1 push constant block, this is not allowed.");
      continue;
    }

    PushConstantBlock &block = stage.m_PushConstants[0];

    bool skip = false;

    for (auto &range : pushConstantRanges) {
      if (block.m_Offset == range.offset && block.m_Size == range.size) {
        range.stageFlags |= block.m_Stage;
      }
    }

    if (skip) {
      continue;
    }

    VkPushConstantRange range{};
    range.offset = block.m_Offset;
    range.size = block.m_Size;
    range.stageFlags = block.m_Stage;

    pushConstantRanges.push_back(range);
  }

  pipelineLayoutInfo.pushConstantRangeCount =
      static_cast<uint32_t>(pushConstantRanges.size());
  pipelineLayoutInfo.pPushConstantRanges =
      pushConstantRanges.size() > 0 ? pushConstantRanges.data() : 0;

  VK_CHECK(vkCreatePipelineLayout(vk.m_LogicalDevice, &pipelineLayoutInfo,
                                  nullptr, &pipelineLayout))

  VkPipelineDepthStencilStateCreateInfo depthStencil{};
  depthStencil.sType =
      VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
  depthStencil.depthTestEnable = VK_TRUE;
  depthStencil.depthWriteEnable = VK_TRUE;
  depthStencil.depthCompareOp = depthCompareOp;
  depthStencil.depthBoundsTestEnable = VK_FALSE;
  depthStencil.minDepthBounds = 0.0f; // Optional
  depthStencil.maxDepthBounds = 1.0f; // Optional
  depthStencil.stencilTestEnable = VK_FALSE;
  depthStencil.front = {}; // Optional
  depthStencil.back = {};  // Optional

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
  pipelineCreateInfo.pDynamicState = &dynamicStateInfo;
  pipelineCreateInfo.pDepthStencilState = &depthStencil;

  pipelineCreateInfo.layout = pipelineLayout;

  pipelineCreateInfo.renderPass = pipelineRenderPass;
  pipelineCreateInfo.subpass = 0;

  VkPipeline pipeline;
  VK_CHECK(vkCreateGraphicsPipelines(vk.m_LogicalDevice, VK_NULL_HANDLE, 1,
                                     &pipelineCreateInfo, nullptr, &pipeline))

  vkDestroyShaderModule(vk.m_LogicalDevice, vertShaderModule, nullptr);
  vkDestroyShaderModule(vk.m_LogicalDevice, fragShaderModule, nullptr);

  spdlog::info("Destroyed vertex and fragment shader modules");

  return pipeline;
}

VkPipeline
CreateComputePipeline(VkState &vk, StageBinary &comp,
                           VkDescriptorSetLayout &descriptorSetLayout,
                           VkPipelineLayout &pipelineLayout) {

  auto compStage = CreateShaderModule(vk, comp);

  VkPipelineShaderStageCreateInfo compShaderStageInfo{};
  compShaderStageInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  compShaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
  compShaderStageInfo.module = compStage;
  compShaderStageInfo.pName = "main";

  VkComputePipelineCreateInfo pipelineInfo{};
  pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
  pipelineInfo.layout = pipelineLayout;
  pipelineInfo.stage = compShaderStageInfo;

  VkPipeline pipeline;

  if (vkCreateComputePipelines(vk.m_LogicalDevice, VK_NULL_HANDLE, 1,
                               &pipelineInfo, nullptr,
                               &pipeline) != VK_SUCCESS) {
    spdlog::error("failed to create compute pipeline!");
    return pipeline;
  }

  return VK_NULL_HANDLE;
}
}
