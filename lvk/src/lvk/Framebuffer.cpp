#include "lvk/Framebuffer.h"
#include "volk.h"

void lvk::Framebuffer::Free(VulkanAPI& vk)
{
    for (auto& t : m_ColourAttachments)
    {
        t.Free(vk);
    }
    for (auto& t : m_DepthAttachments)
    {
        t.Free(vk);
    }
    for (auto& t : m_ResolveAttachments)
    {
        t.Free(vk);
    }

    for (auto& fb : m_SwapchainFramebuffers)
    {
        vkDestroyFramebuffer(vk.m_LogicalDevice, fb, nullptr);
    }
}
