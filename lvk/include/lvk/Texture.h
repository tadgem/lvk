#pragma once
#include "lvk/VulkanAPI.h"

namespace lvk
{
    enum class ResolutionScale
    {
        Full,
        Half,
        Quarter,
        Eighth
    };

    static void ResolveResolutionScale(ResolutionScale scale, uint32_t inWidth, uint32_t inHeight, uint32_t& outWidth, uint32_t& outHeight)
    {
        switch (scale)
        {
        case ResolutionScale::Full:
            outWidth = inWidth;
            outHeight = inHeight;
            return;
        case ResolutionScale::Half:
            outWidth = inWidth / 2;
            outHeight = inHeight / 2;
            return;
        case ResolutionScale::Quarter:
            outWidth = inWidth / 4;
            outHeight = inHeight / 4;
        case ResolutionScale::Eighth:
            outWidth = inWidth / 8;
            outHeight = inHeight / 8;
            return;
        }
    }

    class Texture
    {
    public:

        static Texture* g_DefaultTexture;

        VkImage                 m_Image;
        VkImageView             m_ImageView;
        VkDeviceMemory          m_Memory;
        VkSampler               m_Sampler;
        VkFormat                m_Format;
        VkSampleCountFlagBits   m_SampleCount;

        Texture(VkImage image, VkImageView imageView, VkDeviceMemory memory, VkSampler sampler, VkFormat format, VkSampleCountFlagBits sampleCount) :
            m_Image(image),
            m_ImageView(imageView),
            m_Memory(memory),
            m_Sampler(sampler),
            m_Format(format),
            m_SampleCount(sampleCount)
        {

        }

        static Texture CreateAttachment(lvk::VulkanAPI& vk, uint32_t width, uint32_t height,
            uint32_t numMips, VkSampleCountFlagBits sampleCount,
            VkFormat format, VkImageTiling tiling, VkImageUsageFlags usageFlags,
            VkMemoryPropertyFlagBits memoryFlags, VkImageAspectFlagBits imageAspect,
            VkFilter samplerFilter = VK_FILTER_LINEAR, VkSamplerAddressMode samplerAddressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT)
        {
            VkImage image;
            VkImageView imageView;
            VkDeviceMemory memory;
            VkSampler sampler;
            vk.CreateImage(width, height, numMips, sampleCount, format, tiling, usageFlags, memoryFlags, image, memory);
            vk.CreateImageView(image, format, numMips, imageAspect, imageView);
            vk.CreateImageSampler(imageView, numMips, samplerFilter, samplerAddressMode, sampler);

            return Texture(image, imageView, memory, sampler, format, sampleCount);
        }

        static Texture CreateTexture(lvk::VulkanAPI& vk, const lvk::String& path, VkFormat format, VkFilter samplerFilter = VK_FILTER_LINEAR, VkSamplerAddressMode samplerAddressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT)
        {
            VkImage image;
            VkImageView imageView;
            VkDeviceMemory memory;
            // Texture abstraction
            uint32_t mipLevels;
            vk.CreateTexture(path, format, image, imageView, memory, &mipLevels);
            VkSampler sampler;
            vk.CreateImageSampler(imageView, mipLevels, samplerFilter, samplerAddressMode, sampler);

            return Texture(image, imageView, memory, sampler, format, VK_SAMPLE_COUNT_1_BIT);
        }

        static Texture CreateTextureFromMemory(lvk::VulkanAPI& vk, unsigned char* tex_data, uint32_t length, VkFormat format, VkFilter samplerFilter = VK_FILTER_LINEAR, VkSamplerAddressMode samplerAddressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT)
        {
            VkImage image;
            VkImageView imageView;
            VkDeviceMemory memory;
            // Texture abstraction
            uint32_t mipLevels;
            vk.CreateTextureFromMemory(tex_data, length, format, image, imageView, memory, &mipLevels);
            VkSampler sampler;
            vk.CreateImageSampler(imageView, mipLevels, samplerFilter, samplerAddressMode, sampler);

            return Texture(image, imageView, memory, sampler, format, VK_SAMPLE_COUNT_1_BIT);
        }

        static void InitDefaultTexture(lvk::VulkanAPI& vk);
        static void FreeDefaultTexture(lvk::VulkanAPI& vk);

        void Free(lvk::VulkanAPI& vk)
        {
            vkDestroySampler(vk.m_LogicalDevice, m_Sampler, nullptr);
            vkDestroyImageView(vk.m_LogicalDevice, m_ImageView, nullptr);
            vkDestroyImage(vk.m_LogicalDevice, m_Image, nullptr);
            vkFreeMemory(vk.m_LogicalDevice, m_Memory, nullptr);
        }
    };
}