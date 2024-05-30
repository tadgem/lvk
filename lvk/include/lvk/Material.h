#pragma once
#include "lvk/VulkanAPI.h"

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

        Vector<FrameDescriptorSets>                 m_DescriptorSets;
        Vector<UniformBufferBindingData>        m_UniformBuffers;
        HashMap<String, SamplerBindingData>     m_Samplers;
        HashMap<String, UniformAccessorData>    m_UniformBufferAccessors;

        static Material Create(VulkanAPI& vk, ShaderProgram& shader);

        template<typename _Ty>
        bool SetBuffer(uint32_t set, uint32_t binding, const _Ty& value)
        {
            for (auto& buffer : m_UniformBuffers)
            {
                if (buffer.m_SetNumber == set && buffer.m_BindingNumber == binding)
                {
                    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
                    {
                        buffer.m_UBO.Set(i, value);
                    }
                    return true;
                }
            }
            return false;
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

        bool SetSampler(VulkanAPI& vk, const String& name, const VkImageView& imageView, const VkSampler& sampler);
    };

}