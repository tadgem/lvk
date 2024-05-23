#pragma once
#include "lvk/Texture.h"

namespace lvk
{
    class Framebuffer {
    public:
        Vector<Texture> m_ColourAttachments;
        Vector<Texture> m_DepthAttachments;
        Vector<Texture> m_ResolveAttachments;

        VkFramebuffer   m_FB;

        Texture& AddColourAttachment(lvk::VulkanAPI& vk, VkExtent2D resolution,
            uint32_t numMips, VkSampleCountFlagBits sampleCount,
            VkFormat format, VkImageUsageFlags usageFlags,
            VkMemoryPropertyFlagBits memoryFlags, VkImageAspectFlagBits imageAspect,
            VkFilter samplerFilter = VK_FILTER_LINEAR, VkSamplerAddressMode samplerAddressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT)
        {
            VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL;
            m_ColourAttachments.push_back(Texture::CreateAttachment(vk, resolution.width, resolution.height, numMips, sampleCount, format, tiling, usageFlags, memoryFlags, imageAspect, samplerFilter, samplerAddressMode));
            return m_ColourAttachments.back();
        }

        Texture& AddDepthAttachment(lvk::VulkanAPI& vk, VkExtent2D resolution,
            uint32_t numMips, VkSampleCountFlagBits sampleCount,
            VkImageUsageFlags usageFlags,
            VkMemoryPropertyFlagBits memoryFlags, VkImageAspectFlagBits imageAspect,
            VkFilter samplerFilter = VK_FILTER_LINEAR, VkSamplerAddressMode samplerAddressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT)
        {
            VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL;
            m_DepthAttachments.push_back(Texture::CreateAttachment(vk, resolution.width, resolution.height, numMips, sampleCount, vk.FindDepthFormat(), tiling, usageFlags, memoryFlags, imageAspect, samplerFilter, samplerAddressMode));
            return m_DepthAttachments.back();
        }

        Texture& AddResolveAttachment(lvk::VulkanAPI& vk, VkExtent2D resolution,
            uint32_t numMips, VkSampleCountFlagBits sampleCount,
            VkFormat format, VkImageUsageFlags usageFlags,
            VkMemoryPropertyFlagBits memoryFlags, VkImageAspectFlagBits imageAspect,
            VkFilter samplerFilter = VK_FILTER_LINEAR, VkSamplerAddressMode samplerAddressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT)
        {
            VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL;
            m_ResolveAttachments.push_back(Texture::CreateAttachment(vk, resolution.width, resolution.height, numMips, sampleCount, format, tiling, usageFlags, memoryFlags, imageAspect, samplerFilter, samplerAddressMode));
            return m_ResolveAttachments.back();
        }

        bool Build(lvk::VulkanAPI& vk, VkRenderPass& renderPass, VkExtent2D extent)
        {
            bool hasDepth = m_DepthAttachments.size() > 0;
            Vector<VkImageView> framebufferAttachments;
            for (auto& colour : m_ColourAttachments)
            {
                framebufferAttachments.push_back(colour.m_ImageView);
            }
            if (hasDepth)
            {
                framebufferAttachments.push_back(m_DepthAttachments[0].m_ImageView);
            }
            for (auto& resolve : m_ResolveAttachments)
            {
                framebufferAttachments.push_back(resolve.m_ImageView);
            }
            vk.CreateFramebuffer(framebufferAttachments, renderPass, extent, m_FB);
            return false;
        }


        void Free(VulkanAPI& vk)
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
            vkDestroyFramebuffer(vk.m_LogicalDevice, m_FB, nullptr);
        }
    };

    class FramebufferSet {
    public:
        Array<Framebuffer, MAX_FRAMES_IN_FLIGHT> m_Framebuffers;

        // render pass must be shared between all images in the swap chain
        VkRenderPass    m_RenderPass;
        // clear colours
        Vector <VkClearColorValue> m_ClearValues;

        uint32_t m_Width = 0;
        uint32_t m_Height = 0;

        VkAttachmentLoadOp m_AttachmentLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;

        void Free(VulkanAPI& vk)
        {
            for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
            {
                m_Framebuffers[i].Free(vk);
            }
        }

        void AddColourAttachment(lvk::VulkanAPI& vk, ResolutionScale scale,
            uint32_t numMips, VkSampleCountFlagBits sampleCount,
            VkFormat format, VkImageUsageFlags usageFlags,
            VkMemoryPropertyFlagBits memoryFlags, VkImageAspectFlagBits imageAspect,
            VkFilter samplerFilter = VK_FILTER_LINEAR, VkSamplerAddressMode samplerAddressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT)
        {
            VkExtent2D resolution{};
            ResolveResolutionScale(scale, m_Width, m_Height, resolution.width, resolution.height);
            for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
            {
                m_Framebuffers[i].AddColourAttachment(vk, resolution, numMips, sampleCount, format, usageFlags, memoryFlags, imageAspect, samplerFilter, samplerAddressMode);
            }
        }

        void AddDepthAttachment(lvk::VulkanAPI& vk, ResolutionScale scale,
            uint32_t numMips, VkSampleCountFlagBits sampleCount,
            VkImageUsageFlags usageFlags,
            VkMemoryPropertyFlagBits memoryFlags, VkImageAspectFlagBits imageAspect,
            VkFilter samplerFilter = VK_FILTER_LINEAR, VkSamplerAddressMode samplerAddressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT)
        {
            VkExtent2D resolution{};
            ResolveResolutionScale(scale, m_Width, m_Height, resolution.width, resolution.height);
            for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
            {
                m_Framebuffers[i].AddDepthAttachment(vk, resolution, numMips, sampleCount, usageFlags, memoryFlags, imageAspect, samplerFilter, samplerAddressMode);
            }
        }

        void AddResolveAttachment(lvk::VulkanAPI& vk, ResolutionScale scale,
            uint32_t numMips, VkSampleCountFlagBits sampleCount,
            VkFormat format, VkImageUsageFlags usageFlags,
            VkMemoryPropertyFlagBits memoryFlags, VkImageAspectFlagBits imageAspect,
            VkFilter samplerFilter = VK_FILTER_LINEAR, VkSamplerAddressMode samplerAddressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT)
        {
            VkExtent2D resolution{};
            ResolveResolutionScale(scale, m_Width, m_Height, resolution.width, resolution.height);
            for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
            {
                m_Framebuffers[i].AddResolveAttachment(vk, resolution, numMips, sampleCount, format, usageFlags, memoryFlags, imageAspect, samplerFilter, samplerAddressMode);
            }
        }

        void Build(lvk::VulkanAPI& vk)
        {
            // build renderpass
            Vector<VkAttachmentDescription> colourAttachmentDescriptions{};
            for (auto& col : m_Framebuffers[0].m_ColourAttachments)
            {
                VkAttachmentDescription colourAttachment{};
                colourAttachment.format = col.m_Format;
                colourAttachment.samples = col.m_SampleCount;
                colourAttachment.loadOp = m_AttachmentLoadOp;
                colourAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
                colourAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                colourAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
                colourAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                colourAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                colourAttachmentDescriptions.push_back(colourAttachment);
            }

            Vector<VkAttachmentDescription> resolveAttachmentDescriptions{};

            for (auto& col : m_Framebuffers[0].m_ResolveAttachments)
            {
                VkAttachmentDescription resolveAttachment{};
                resolveAttachment.format = col.m_Format;
                resolveAttachment.samples = col.m_SampleCount;
                resolveAttachment.loadOp = m_AttachmentLoadOp;
                resolveAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
                resolveAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                resolveAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
                resolveAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                resolveAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                resolveAttachmentDescriptions.push_back(resolveAttachment);
            }

            VkAttachmentDescription depthAttachmentDescription{};
            bool hasDepth = m_Framebuffers[0].m_DepthAttachments.size() > 0;
            if (hasDepth)
            {
                Texture& depth = m_Framebuffers[0].m_DepthAttachments[0];
                depthAttachmentDescription.format = depth.m_Format;
                depthAttachmentDescription.samples = depth.m_SampleCount;
                depthAttachmentDescription.loadOp = m_AttachmentLoadOp;
                depthAttachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
                depthAttachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                depthAttachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
                depthAttachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                depthAttachmentDescription.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            }

            vk.CreateRenderPass(m_RenderPass,
                colourAttachmentDescriptions, resolveAttachmentDescriptions, hasDepth,
                depthAttachmentDescription, m_AttachmentLoadOp);

            for (auto& fb : m_Framebuffers)
            {
                fb.Build(vk, m_RenderPass, { m_Width, m_Height });
            }
        }

    };
}