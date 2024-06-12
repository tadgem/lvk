#pragma once
#include "lvk/VulkanAPI.h"
#include "lvk/Texture.h"
#include "lvk/Framebuffer.h"
namespace lvk
{
    class ShaderProgram;
    class Material
    {
    public:
        // create a material from a shader
        // material is the interface to an instance of a shader
        // contains descriptor set and associated buffers.
        // set mat4, mat3, vec4, vec3, sampler etc.
        // reflect the size of each bound thing in each set (one set for now)

        struct UniformBufferBindingData
        {
            uint32_t m_SetNumber;
            uint32_t m_BindingNumber;
            uint32_t m_BufferSize;
            UniformBufferFrameData m_UBO;
        };

        struct SamplerBindingData
        {
            uint32_t        m_SetNumber;
            uint32_t        m_BindingNumber;
            VkImageView& m_ImageView;
            VkSampler& m_Sampler;
        };

        struct UniformAccessorData
        {
            uint32_t    m_ExpectedSize;
            uint32_t    m_Offset;
            uint32_t    m_Stride;
            uint16_t    m_ArraySize;
            uint16_t    m_BufferIndex;
        };

        struct FrameDescriptorSets
        {
            Array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT> m_Sets;
        };

        Vector<FrameDescriptorSets>             m_DescriptorSets;
        
        union SetBinding {
            uint64_t m_Data;
            struct {
                uint32_t m_Set;
                uint32_t m_Binding;
            };
        };

        // todo: rework this to be a hashmap
        // uint64 - buffer data

        HashMap<uint64_t, UniformBufferBindingData>   m_UniformBuffers;
        HashMap<String, SamplerBindingData>             m_Samplers;
        HashMap<String, UniformAccessorData>            m_UniformBufferAccessors;

        static Material Create(VulkanAPI& vk, ShaderProgram& shader);

        template<typename _Ty>
        bool SetBuffer(uint32_t frameIndex, uint32_t set, uint32_t binding, const _Ty& value)
        {
            Material::SetBinding sb = {};
            sb.m_Set = set;
            sb.m_Binding = binding;

            m_UniformBuffers[sb.m_Data].m_UBO.Set(frameIndex, value);

            return true;
        }

        template<typename _Ty>
        bool SetMember(const String& name, const _Ty& value)
        {
            static constexpr size_t _type_size = sizeof(_Ty);
            if (m_UniformBufferAccessors.find(name) == m_UniformBufferAccessors.end())
            {
                return false;
            }

            UniformAccessorData& data = m_UniformBufferAccessors.at(name);

            if (_type_size != data.m_ExpectedSize)
            {
                return false;
            }

            for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
            {
                // update uniform buffer
                m_UniformBuffers[data.m_BufferIndex].m_UBO.Set(i, value, data.m_Offset);
            }

            return true;
        }

        bool SetSampler(VulkanAPI& vk, const String& name, const VkImageView& imageView, const VkSampler& sampler, bool isAttachment = false);
        bool SetSampler(VulkanAPI& vk, const String& name, Texture& texture);
        bool SetColourAttachment(VulkanAPI& vk, const String& name, Framebuffer& framebuffer, uint32_t colourAttachmentIndex);
        bool SetDepthAttachment(VulkanAPI& vk, const String& name, Framebuffer& framebuffer);

        
        void Free(VulkanAPI& vk);
    };

}