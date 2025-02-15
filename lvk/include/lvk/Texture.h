#pragma once
#include "ImGui/imgui_impl_vulkan.h"
#include "lvk/Structs.h"
#include "lvk/Texture.h"

namespace lvk
{
    namespace textures {
    void  CreateImage(VkState& vk, uint32_t width, uint32_t height, uint32_t numMips, VkSampleCountFlagBits sampleCount, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory, uint32_t depth = 1);
    void  CreateImageView(VkState& vk, VkImage& image, VkFormat format, uint32_t numMips, VkImageAspectFlags aspectFlags, VkImageView& imageView, VkImageViewType imageViewType= VK_IMAGE_VIEW_TYPE_2D);
    void  CreateImageSampler(VkState& vk, VkImageView& imageView, uint32_t numMips, VkFilter filterMode, VkSamplerAddressMode addressMode, VkSampler& sampler);
    void  CreateFramebuffer(VkState& vk, Vector<VkImageView>& attachments, VkRenderPass renderPass, VkExtent2D extent, VkFramebuffer& framebuffer);
    void  CreateTexture(VkState& vk, const String& path, VkFormat format, VkImage& image, VkImageView& imageView, VkDeviceMemory& imageMemory, uint32_t* numMips = nullptr);
    void  CreateTextureFromMemory(VkState& vk, unsigned char* tex_data, uint32_t dataSize, VkFormat format, VkImage& image, VkImageView& imageView, VkDeviceMemory& imageMemory, uint32_t* numMips = nullptr);
    void  CreateTexture3DFromMemory(VkState& vk, unsigned char* tex_data, VkExtent3D extent, uint32_t dataSize, VkFormat format, VkImage& image, VkImageView& imageView, VkDeviceMemory& imageMemory, uint32_t* numMips = nullptr);
    void  CopyBufferToImage(VkState& vk, VkBuffer& src, VkImage& image,  uint32_t width, uint32_t height);
    void  GenerateMips(VkState& vk, VkImage image, VkFormat format, uint32_t imageWidth, uint32_t imageHeight, uint32_t numMips, VkFilter filterMethod);
    void  TransitionImageLayout(VkState& vk, VkImage image, VkFormat format, uint32_t numMips, VkImageLayout oldLayout, VkImageLayout newLayout);
    }
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

        static Texture CreateAttachment(lvk::VkState & vk, uint32_t width, uint32_t height,
            uint32_t numMips, VkSampleCountFlagBits sampleCount,
            VkFormat format, VkImageTiling tiling, VkImageUsageFlags usageFlags,
            VkMemoryPropertyFlagBits memoryFlags, VkImageAspectFlagBits imageAspect,
            VkFilter samplerFilter = VK_FILTER_NEAREST, VkSamplerAddressMode samplerAddressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT)
        {
            VkImage image;
            VkImageView imageView;
            VkDeviceMemory memory;
            VkSampler sampler;
            textures::CreateImage(vk,width, height, numMips, sampleCount, format, tiling, usageFlags, memoryFlags, image, memory);
            textures::CreateImageView(vk,image, format, numMips, imageAspect, imageView);
            textures::CreateImageSampler(vk,imageView, numMips, samplerFilter, samplerAddressMode, sampler);

            VkDescriptorSet imguiTextureHandle = VK_NULL_HANDLE;
            if (vk.m_UseImGui)
            {
                imguiTextureHandle = ImGui_ImplVulkan_AddTexture(sampler, imageView, VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL);
            }

            return Texture(image, imageView, memory, sampler, format, sampleCount, imguiTextureHandle);
        }

        static Texture CreateTexture(lvk::VkState & vk, const lvk::String& path, VkFormat format, VkFilter samplerFilter = VK_FILTER_LINEAR, VkSamplerAddressMode samplerAddressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT)
        {
            VkImage image;
            VkImageView imageView;
            VkDeviceMemory memory;
            // Texture abstraction
            uint32_t mipLevels;
            lvk::textures::CreateTexture(vk, path, format, image, imageView, memory, &mipLevels);
            VkSampler sampler;
            textures::CreateImageSampler(vk, imageView, mipLevels, samplerFilter, samplerAddressMode, sampler);

            VkDescriptorSet imguiTextureHandle = VK_NULL_HANDLE;
            if (vk.m_UseImGui)
            {
                imguiTextureHandle = ImGui_ImplVulkan_AddTexture(sampler, imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
            }

            return Texture(image, imageView, memory, sampler, format, VK_SAMPLE_COUNT_1_BIT, imguiTextureHandle);
        }

        static Texture CreateTextureFromMemory(lvk::VkState & vk, unsigned char* tex_data, uint32_t length, VkFormat format, VkFilter samplerFilter = VK_FILTER_LINEAR, VkSamplerAddressMode samplerAddressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT)
        {
            VkImage image;
            VkImageView imageView;
            VkDeviceMemory memory;
            // Texture abstraction
            uint32_t mipLevels;
            lvk::textures::CreateTextureFromMemory(vk, tex_data, length, format, image, imageView, memory, &mipLevels);
            VkSampler sampler;
            textures::CreateImageSampler(vk, imageView, mipLevels, samplerFilter, samplerAddressMode, sampler);

            VkDescriptorSet imguiTextureHandle = VK_NULL_HANDLE;
            if (vk.m_UseImGui)
            {
                // imguiTextureHandle = ImGui_ImplVulkan_AddTexture(sampler, imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
            }

            return Texture(image, imageView, memory, sampler, format, VK_SAMPLE_COUNT_1_BIT, imguiTextureHandle);
        }

        static Texture CreateTexture3DFromMemory(lvk::VkState & vk, VkExtent3D extent, unsigned char* tex_data, uint32_t length, VkFormat format, VkFilter samplerFilter = VK_FILTER_LINEAR, VkSamplerAddressMode samplerAddressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT)
        {
            VkImage image;
            VkImageView imageView;
            VkDeviceMemory memory;
            // Texture abstraction
            uint32_t mipLevels;
            lvk::textures::CreateTexture3DFromMemory(vk, tex_data, extent, length, format, image, imageView, memory, &mipLevels);
            VkSampler sampler;
            textures::CreateImageSampler(vk, imageView, mipLevels, samplerFilter, samplerAddressMode, sampler);

            VkDescriptorSet imguiTextureHandle = VK_NULL_HANDLE;
            if (vk.m_UseImGui)
            {
                // imguiTextureHandle = ImGui_ImplVulkan_AddTexture(sampler, imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
            }

            return Texture(image, imageView, memory, sampler, format, VK_SAMPLE_COUNT_1_BIT, imguiTextureHandle);
        }

        static void InitDefaultTexture(lvk::VkState & vk);
        static void FreeDefaultTexture(lvk::VkState & vk);

        void Free(lvk::VkState & vk);
    };
}