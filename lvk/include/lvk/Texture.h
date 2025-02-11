#pragma once
#include "ImGui/imgui_impl_vulkan.h"
#include "lvk/VkBackend.h"

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
        VkDescriptorSet         m_ImGuiHandle;

        Texture(VkImage image, VkImageView imageView, VkDeviceMemory memory, VkSampler sampler, VkFormat format, VkSampleCountFlagBits sampleCount, VkDescriptorSet imguiHandle = VK_NULL_HANDLE) :
            m_Image(image),
            m_ImageView(imageView),
            m_Memory(memory),
            m_Sampler(sampler),
            m_Format(format),
            m_SampleCount(sampleCount),
            m_ImGuiHandle(imguiHandle)
        {

        }

        static Texture CreateAttachment(lvk::VkBackend & vk, uint32_t width, uint32_t height,
            uint32_t numMips, VkSampleCountFlagBits sampleCount,
            VkFormat format, VkImageTiling tiling, VkImageUsageFlags usageFlags,
            VkMemoryPropertyFlagBits memoryFlags, VkImageAspectFlagBits imageAspect,
            VkFilter samplerFilter = VK_FILTER_NEAREST, VkSamplerAddressMode samplerAddressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT)
        {
            VkImage image;
            VkImageView imageView;
            VkDeviceMemory memory;
            VkSampler sampler;
            vk.CreateImage(width, height, numMips, sampleCount, format, tiling, usageFlags, memoryFlags, image, memory);
            vk.CreateImageView(image, format, numMips, imageAspect, imageView);
            vk.CreateImageSampler(imageView, numMips, samplerFilter, samplerAddressMode, sampler);

            VkDescriptorSet imguiTextureHandle = VK_NULL_HANDLE;
            if (vk.m_UseImGui)
            {
                imguiTextureHandle = ImGui_ImplVulkan_AddTexture(sampler, imageView, VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL);
            }

            return Texture(image, imageView, memory, sampler, format, sampleCount, imguiTextureHandle);
        }

        static Texture CreateTexture(lvk::VkBackend & vk, const lvk::String& path, VkFormat format, VkFilter samplerFilter = VK_FILTER_LINEAR, VkSamplerAddressMode samplerAddressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT)
        {
            VkImage image;
            VkImageView imageView;
            VkDeviceMemory memory;
            // Texture abstraction
            uint32_t mipLevels;
            vk.CreateTexture(path, format, image, imageView, memory, &mipLevels);
            VkSampler sampler;
            vk.CreateImageSampler(imageView, mipLevels, samplerFilter, samplerAddressMode, sampler);

            VkDescriptorSet imguiTextureHandle = VK_NULL_HANDLE;
            if (vk.m_UseImGui)
            {
                imguiTextureHandle = ImGui_ImplVulkan_AddTexture(sampler, imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
            }

            return Texture(image, imageView, memory, sampler, format, VK_SAMPLE_COUNT_1_BIT, imguiTextureHandle);
        }

        static Texture CreateTextureFromMemory(lvk::VkBackend & vk, unsigned char* tex_data, uint32_t length, VkFormat format, VkFilter samplerFilter = VK_FILTER_LINEAR, VkSamplerAddressMode samplerAddressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT)
        {
            VkImage image;
            VkImageView imageView;
            VkDeviceMemory memory;
            // Texture abstraction
            uint32_t mipLevels;
            vk.CreateTextureFromMemory(tex_data, length, format, image, imageView, memory, &mipLevels);
            VkSampler sampler;
            vk.CreateImageSampler(imageView, mipLevels, samplerFilter, samplerAddressMode, sampler);

            VkDescriptorSet imguiTextureHandle = VK_NULL_HANDLE;
            if (vk.m_UseImGui)
            {
                // imguiTextureHandle = ImGui_ImplVulkan_AddTexture(sampler, imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
            }

            return Texture(image, imageView, memory, sampler, format, VK_SAMPLE_COUNT_1_BIT, imguiTextureHandle);
        }

        static Texture CreateTexture3DFromMemory(lvk::VkBackend & vk, VkExtent3D extent, unsigned char* tex_data, uint32_t length, VkFormat format, VkFilter samplerFilter = VK_FILTER_LINEAR, VkSamplerAddressMode samplerAddressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT)
        {
            VkImage image;
            VkImageView imageView;
            VkDeviceMemory memory;
            // Texture abstraction
            uint32_t mipLevels;
            vk.CreateTexture3DFromMemory(tex_data, extent, length, format, image, imageView, memory, &mipLevels);
            VkSampler sampler;
            vk.CreateImageSampler(imageView, mipLevels, samplerFilter, samplerAddressMode, sampler);

            VkDescriptorSet imguiTextureHandle = VK_NULL_HANDLE;
            if (vk.m_UseImGui)
            {
                // imguiTextureHandle = ImGui_ImplVulkan_AddTexture(sampler, imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
            }

            return Texture(image, imageView, memory, sampler, format, VK_SAMPLE_COUNT_1_BIT, imguiTextureHandle);
        }

        static void InitDefaultTexture(lvk::VkBackend & vk);
        static void FreeDefaultTexture(lvk::VkBackend & vk);

        void Free(lvk::VkBackend & vk);
    };
}