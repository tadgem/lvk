#include "lvk/RenderPass.h"
#include "spdlog/spdlog.h"

void lvk::CreateRenderPass(VkState& vk, VkRenderPass& renderPass, Vector<VkAttachmentDescription>& colourAttachments, Vector<VkAttachmentDescription>& resolveAttachments, bool hasDepthAttachment, VkAttachmentDescription depthAttachment, VkAttachmentLoadOp attachmentLoadOp)
{
  // Layout: Colour attachments -> Depth attachments -> Resolve attachments
  VkSubpassDescription subpass{};
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

  Vector<VkAttachmentDescription> attachments;

  Vector<VkAttachmentReference>   colourAttachmentReferences;
  Vector<VkAttachmentReference>   resolveAttachmentReferences;

  uint32_t attachmentCount = 0;

  VkAttachmentReference colorAttachmentReference{};
  for (auto i = 0; i < colourAttachments.size(); i++)
  {
    attachments.push_back(colourAttachments[i]);

    colorAttachmentReference.attachment = attachmentCount++;
    colorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    colourAttachmentReferences.push_back(colorAttachmentReference);
  }

  VkAttachmentReference depthAttachmentReference{};
  if (hasDepthAttachment)
  {
    attachments.push_back(depthAttachment);
    depthAttachmentReference.attachment = attachmentCount++;
    depthAttachmentReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    subpass.pDepthStencilAttachment = &depthAttachmentReference;
  }

  VkAttachmentReference colorAttachmentResolveReference{};
  for (auto i = 0; i < resolveAttachments.size(); i++)
  {
    attachments.push_back(resolveAttachments[i]);
    colorAttachmentResolveReference.attachment = attachmentCount++;
    colorAttachmentResolveReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    resolveAttachmentReferences.push_back(colorAttachmentResolveReference);
  }

  subpass.colorAttachmentCount = static_cast<uint32_t>(colourAttachmentReferences.size());
  subpass.pColorAttachments = colourAttachmentReferences.data();
  subpass.pResolveAttachments = resolveAttachmentReferences.data();

  VkPipelineStageFlags waitFlags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  VkAccessFlags accessFlags = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

  if (hasDepthAttachment)
  {
    waitFlags |= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    accessFlags |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
  }

  VkSubpassDependency subpassDependency{};
  // implicit subpasses
  subpassDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
  // our pass
  subpassDependency.dstSubpass = 0;
  // wait for the colour output stage to finish
  subpassDependency.srcStageMask = waitFlags;
  subpassDependency.srcAccessMask = 0;
  // wait until we can write to the color attachment
  subpassDependency.dstStageMask = waitFlags;
  subpassDependency.dstAccessMask = accessFlags;


  VkRenderPassCreateInfo createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  createInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
  createInfo.pAttachments = attachments.data();
  createInfo.subpassCount = 1;
  createInfo.pSubpasses = &subpass;
  createInfo.dependencyCount = 1;
  createInfo.pDependencies = &subpassDependency;

  if (vkCreateRenderPass(vk.m_LogicalDevice, &createInfo, nullptr, &renderPass) != VK_SUCCESS)
  {
    spdlog::error("Failed to create Render Pass!");
    std::cerr << "Failed to create Render Pass!" << std::endl;
  }
}
