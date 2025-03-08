#pragma once
#include "lvk/Macros.h"
#include "lvk/RenderPass.h"
#include "Utils.h"
#include "lvk/Texture.h"

namespace lvk
{
    class Attachment
    {
    public:

        Vector<Texture>         m_AttachmentSwapchainImages;
        VkFormat                m_Format;
        VkSampleCountFlagBits   m_SampleCount;

        void Free(VkState & vk)
        {
            for (auto& t : m_AttachmentSwapchainImages)
            {
                t.Free(vk);
            }
        }

        static Attachment CreateColourAttachment(lvk::VkState & vk, VkExtent2D resolution,
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

        static Attachment CreateDepthAttachment(lvk::VkState & vk, VkExtent2D resolution,
            uint32_t numMips, VkSampleCountFlagBits sampleCount,
            VkImageUsageFlags usageFlags,
            VkMemoryPropertyFlagBits memoryFlags, VkImageAspectFlagBits imageAspect,
            VkFilter samplerFilter = VK_FILTER_LINEAR, VkSamplerAddressMode samplerAddressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT)
        {
            Attachment a{ {}, utils::FindDepthFormat(vk), sampleCount };
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
        Vector<Attachment>  m_ColourAttachments;
        Vector<Attachment>  m_DepthAttachments;
        Vector<Attachment>  m_ResolveAttachments;

        // Render pass
        RenderPassInfo        m_RenderPassInfo;

        Vector <VkClearValue>       m_ClearValues;
        VkAttachmentLoadOp          m_AttachmentLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        VkExtent2D                  m_Resolution;

        void AddColourAttachment(lvk::VkState& vk, VkExtent2D resolution,
            uint32_t numMips, VkSampleCountFlagBits sampleCount,
            VkFormat format, VkImageUsageFlags usageFlags,
            VkMemoryPropertyFlagBits memoryFlags, VkImageAspectFlagBits imageAspect,
            VkFilter samplerFilter = VK_FILTER_LINEAR, VkSamplerAddressMode samplerAddressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT)
        {
            m_ColourAttachments.push_back(Attachment::CreateColourAttachment(vk,
                resolution, numMips, sampleCount, format, usageFlags, memoryFlags, imageAspect, samplerFilter, samplerAddressMode));
            for (auto& img : m_ColourAttachments.back().m_AttachmentSwapchainImages)
            {
                textures::TransitionImageLayout(vk, img.m_Image, format, numMips, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL);
            }
            m_Resolution = resolution;
        }
        
        void AddColourAttachment(lvk::VkState & vk, ResolutionScale scale,
            uint32_t numMips, VkSampleCountFlagBits sampleCount,
            VkFormat format, VkImageUsageFlags usageFlags,
            VkMemoryPropertyFlagBits memoryFlags, VkImageAspectFlagBits imageAspect,
            VkFilter samplerFilter = VK_FILTER_LINEAR, VkSamplerAddressMode samplerAddressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT)
        {
            VkExtent2D resolution = vk.m_MaxFramebufferExtent;
            ResolveResolutionScale(scale, resolution.width, resolution.height, resolution.width, resolution.height);
            AddColourAttachment(vk, resolution, numMips, sampleCount, format, usageFlags, memoryFlags, imageAspect, samplerFilter, samplerAddressMode);
        }

        void AddDepthAttachment(lvk::VkState& vk, VkExtent2D resolution,
            uint32_t numMips, VkSampleCountFlagBits sampleCount,
            VkImageUsageFlags usageFlags,
            VkMemoryPropertyFlagBits memoryFlags, VkImageAspectFlagBits imageAspect,
            VkFilter samplerFilter = VK_FILTER_LINEAR, VkSamplerAddressMode samplerAddressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT)
        {
            m_DepthAttachments.push_back(Attachment::CreateDepthAttachment(vk,
                resolution, numMips, sampleCount, usageFlags, memoryFlags, imageAspect, samplerFilter, samplerAddressMode));
            for (auto& img : m_DepthAttachments.back().m_AttachmentSwapchainImages)
            {
                textures::TransitionImageLayout(vk, img.m_Image, utils::FindDepthFormat(vk), numMips, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
            }
            m_Resolution = resolution;

        }

        void AddDepthAttachment(lvk::VkState & vk, ResolutionScale scale,
            uint32_t numMips, VkSampleCountFlagBits sampleCount,
            VkImageUsageFlags usageFlags,
            VkMemoryPropertyFlagBits memoryFlags, VkImageAspectFlagBits imageAspect,
            VkFilter samplerFilter = VK_FILTER_LINEAR, VkSamplerAddressMode samplerAddressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT)
        {
            VkExtent2D resolution = vk.m_MaxFramebufferExtent;
            ResolveResolutionScale(scale, resolution.width, resolution.height, resolution.width, resolution.height);
            AddDepthAttachment(vk, resolution, numMips, sampleCount, usageFlags, memoryFlags, imageAspect, samplerFilter, samplerAddressMode);
        }

        void AddResolveAttachment(lvk::VkState& vk, VkExtent2D resolution,
            uint32_t numMips, VkSampleCountFlagBits sampleCount,
            VkFormat format, VkImageUsageFlags usageFlags,
            VkMemoryPropertyFlagBits memoryFlags, VkImageAspectFlagBits imageAspect,
            VkFilter samplerFilter = VK_FILTER_LINEAR, VkSamplerAddressMode samplerAddressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT)
        {
            m_ResolveAttachments.push_back(Attachment::CreateColourAttachment(vk,
                resolution, numMips, sampleCount, format, usageFlags, memoryFlags, imageAspect, samplerFilter, samplerAddressMode));
            for (auto& img : m_ResolveAttachments.back().m_AttachmentSwapchainImages)
            {
                textures::TransitionImageLayout(vk, img.m_Image, utils::FindDepthFormat(vk), numMips, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL);
            }
            m_Resolution = resolution;
        }

        void AddResolveAttachment(lvk::VkState & vk, ResolutionScale scale,
            uint32_t numMips, VkSampleCountFlagBits sampleCount,
            VkFormat format, VkImageUsageFlags usageFlags,
            VkMemoryPropertyFlagBits memoryFlags, VkImageAspectFlagBits imageAspect,
            VkFilter samplerFilter = VK_FILTER_LINEAR, VkSamplerAddressMode samplerAddressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT)
        {
            VkExtent2D resolution = vk.m_MaxFramebufferExtent;
            ResolveResolutionScale(scale, resolution.width, resolution.height, resolution.width, resolution.height);
            AddResolveAttachment(vk, resolution, numMips, sampleCount, format, usageFlags, memoryFlags, imageAspect, samplerFilter, samplerAddressMode);
        }

        void Build(lvk::VkState & vk);
        void BeginDynamicRendering(lvk::VkState& vk, VkCommandBuffer& cmd, uint32_t frameIndex);

        void Free(VkState & vk);
      private:

        void BuildRenderPasses(lvk::VkState& vk);
    };
}