#pragma once
#include "lvk/Texture.h"

namespace lvk
{
    class Attachment
    {
    public:

        Vector<Texture>         m_AttachmentSwapchainImages;
        VkFormat                m_Format;
        VkSampleCountFlagBits   m_SampleCount;

        void Free(VkBackend & vk)
        {
            for (auto& t : m_AttachmentSwapchainImages)
            {
                t.Free(vk);
            }
        }

        static Attachment CreateColourAttachment(lvk::VkBackend & vk, VkExtent2D resolution,
            uint32_t numMips, VkSampleCountFlagBits sampleCount,
            VkFormat format, VkImageUsageFlags usageFlags,
            VkMemoryPropertyFlagBits memoryFlags, VkImageAspectFlagBits imageAspect,
            VkFilter samplerFilter = VK_FILTER_LINEAR, VkSamplerAddressMode samplerAddressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT)
        {
            Attachment a{ {}, format, sampleCount };
            VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL;
            for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
            {
                a.m_AttachmentSwapchainImages.push_back(Texture::CreateAttachment(vk, resolution.width, resolution.height, numMips, sampleCount, format, tiling, usageFlags, memoryFlags, imageAspect, samplerFilter, samplerAddressMode));
            }
            return a;
        }

        static Attachment CreateDepthAttachment(lvk::VkBackend & vk, VkExtent2D resolution,
            uint32_t numMips, VkSampleCountFlagBits sampleCount,
            VkImageUsageFlags usageFlags,
            VkMemoryPropertyFlagBits memoryFlags, VkImageAspectFlagBits imageAspect,
            VkFilter samplerFilter = VK_FILTER_LINEAR, VkSamplerAddressMode samplerAddressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT)
        {
            Attachment a{ {}, vk.FindDepthFormat(), sampleCount };
            VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL;
            for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
            {
                a.m_AttachmentSwapchainImages.push_back(Texture::CreateAttachment(vk, resolution.width, resolution.height, numMips, sampleCount, a.m_Format, tiling, usageFlags, memoryFlags, imageAspect, samplerFilter, samplerAddressMode));
            }
            return a;
        }

    };

    class Framebuffer
    {
    public:
        Vector<Attachment> m_ColourAttachments;
        Vector<Attachment> m_DepthAttachments;
        Vector<Attachment> m_ResolveAttachments;

        Vector<VkFramebuffer>   m_SwapchainFramebuffers;
        VkRenderPass            m_RenderPass;


        Vector <VkClearColorValue>  m_ClearValues;
        VkAttachmentLoadOp          m_AttachmentLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        VkExtent2D                  m_Resolution;
        
        void AddColourAttachment(lvk::VkBackend & vk, ResolutionScale scale,
            uint32_t numMips, VkSampleCountFlagBits sampleCount,
            VkFormat format, VkImageUsageFlags usageFlags,
            VkMemoryPropertyFlagBits memoryFlags, VkImageAspectFlagBits imageAspect,
            VkFilter samplerFilter = VK_FILTER_LINEAR, VkSamplerAddressMode samplerAddressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT)
        {
            VkExtent2D resolution = vk.GetMaxFramebufferExtent();
            ResolveResolutionScale(scale, resolution.width, resolution.height, resolution.width, resolution.height);
            m_ColourAttachments.push_back(Attachment::CreateColourAttachment(vk,
                resolution, numMips, sampleCount, format, usageFlags, memoryFlags, imageAspect, samplerFilter, samplerAddressMode));
            m_Resolution = resolution;
        }

        void AddDepthAttachment(lvk::VkBackend & vk, ResolutionScale scale,
            uint32_t numMips, VkSampleCountFlagBits sampleCount,
            VkImageUsageFlags usageFlags,
            VkMemoryPropertyFlagBits memoryFlags, VkImageAspectFlagBits imageAspect,
            VkFilter samplerFilter = VK_FILTER_LINEAR, VkSamplerAddressMode samplerAddressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT)
        {
            VkExtent2D resolution = vk.GetMaxFramebufferExtent();
            ResolveResolutionScale(scale, resolution.width, resolution.height, resolution.width, resolution.height);
            m_DepthAttachments.push_back(Attachment::CreateDepthAttachment(vk,
                resolution, numMips, sampleCount, usageFlags, memoryFlags, imageAspect, samplerFilter, samplerAddressMode));
            m_Resolution = resolution;

        }

        void AddResolveAttachment(lvk::VkBackend & vk, ResolutionScale scale,
            uint32_t numMips, VkSampleCountFlagBits sampleCount,
            VkFormat format, VkImageUsageFlags usageFlags,
            VkMemoryPropertyFlagBits memoryFlags, VkImageAspectFlagBits imageAspect,
            VkFilter samplerFilter = VK_FILTER_LINEAR, VkSamplerAddressMode samplerAddressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT)
        {
            VkExtent2D resolution = vk.GetMaxFramebufferExtent();
            ResolveResolutionScale(scale, resolution.width, resolution.height, resolution.width, resolution.height);
            m_ResolveAttachments.push_back(Attachment::CreateColourAttachment(vk,
                resolution, numMips, sampleCount, format, usageFlags, memoryFlags, imageAspect, samplerFilter, samplerAddressMode));
            m_Resolution = resolution;

        }


        void Build(lvk::VkBackend & vk)
        {
            // build renderpass
            Vector<VkAttachmentDescription> colourAttachmentDescriptions{};
            for (auto& col : m_ColourAttachments)
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

            for (auto& col : m_ResolveAttachments)
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
            bool hasDepth = m_DepthAttachments.size() > 0;
            if (hasDepth)
            {
                Attachment& depth = m_DepthAttachments[0];
                depthAttachmentDescription.format = depth.m_Format;
                depthAttachmentDescription.samples = depth.m_SampleCount;
                depthAttachmentDescription.loadOp = m_AttachmentLoadOp;
                depthAttachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
                depthAttachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                depthAttachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
                depthAttachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                depthAttachmentDescription.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            }

            vk.CreateRenderPass(m_RenderPass,
                colourAttachmentDescriptions, resolveAttachmentDescriptions, hasDepth,
                depthAttachmentDescription, m_AttachmentLoadOp);

            for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
            {
                bool hasDepth = m_DepthAttachments.size() > 0;
                Vector<VkImageView> framebufferAttachments;
                for (auto& colour : m_ColourAttachments)
                {
                    framebufferAttachments.push_back(colour.m_AttachmentSwapchainImages[i].m_ImageView);
                }
                if (hasDepth)
                {
                    framebufferAttachments.push_back(m_DepthAttachments[0].m_AttachmentSwapchainImages[i].m_ImageView);
                }
                for (auto& resolve : m_ResolveAttachments)
                {
                    framebufferAttachments.push_back(resolve.m_AttachmentSwapchainImages[i].m_ImageView);
                }
                VkFramebuffer fb;
                vk.CreateFramebuffer(framebufferAttachments, m_RenderPass, m_Resolution, fb);
                m_SwapchainFramebuffers.push_back(fb);
            }
        }

        void Free(VkBackend & vk);
    };
}