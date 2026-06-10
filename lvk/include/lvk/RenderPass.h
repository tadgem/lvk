#pragma once
#include "lvk/Structs.h"

namespace lvk
{
    class RenderPass {
    public:
        VkRenderPass    m_RenderPass;
        VkFramebuffer   m_Framebuffer;

        VkRenderPassBeginInfo   GetBeginInfo();
    };

    namespace render_passes {

        void  CreateRenderPass(VkState& vk, VkRenderPass& renderPass, Vector<VkAttachmentDescription>& colourAttachments, Vector<VkAttachmentDescription>& resolveAttachments, bool hasDepthAttachment = true, VkAttachmentDescription depthAttachment = {}, VkAttachmentLoadOp attachmentLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR);
        void  BeginSwapchainRenderPass(VkState& vk, VkCommandBuffer& cmd);

    }
}