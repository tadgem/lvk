#pragma once
#include "lvk/Framebuffer.h"
#include "lvk/Texture.h"
namespace lvk
{
    struct ShaderProgram;
    class Material
    {
    public:
        // create a material from a shader
        // material is the interface to an instance of a shader
        // contains descriptor set and associated buffers.
        // set mat4, mat3, vec4, vec3, sampler etc.
        // reflect the size of each bound thing in each set (one set for now)

        struct ShaderBufferBindingData
        {
            uint32_t m_SetNumber;
            uint32_t m_BindingNumber;
            uint32_t m_BufferSize;
            ShaderBufferFrameData m_Buffer;
        };

        struct SamplerBindingData
        {
            uint32_t        m_SetNumber;
            uint32_t        m_BindingNumber;
            VkImageView&    m_ImageView;
            VkSampler&      m_Sampler;
        };

        struct ShaderAccessorData
        {
            uint32_t    m_ExpectedSize;
            uint32_t    m_Offset;
            uint32_t    m_Stride;
            uint32_t    m_ArraySize;
            uint32_t    m_BufferIndex;
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

        Vector<PushConstantBlock>                       m_PushConstants;
        HashMap<uint64_t, ShaderBufferBindingData>      m_UniformBuffers;
        HashMap<String, SamplerBindingData>             m_Samplers;
        HashMap<String, ShaderAccessorData>             m_UniformBufferAccessors;

        static Material Create(VkState & vk, ShaderProgram& shader);

        template<typename _Ty>
        bool SetBuffer(uint32_t frameIndex, uint32_t set, uint32_t binding, const _Ty& value)
        {
            Material::SetBinding sb = {};
            sb.m_Set = set;
            sb.m_Binding = binding;

            m_UniformBuffers[sb.m_Data].m_Buffer.Set(frameIndex, value);

            return true;
        }

        template<typename _Ty>
        bool SetBuffer(uint32_t frameIndex, uint32_t set, uint32_t binding, _Ty* start, uint32_t count)
        {
            Material::SetBinding sb = {};
            sb.m_Set = set;
            sb.m_Binding = binding;

            m_UniformBuffers[sb.m_Data].m_Buffer.SetMemory(frameIndex, start, count);

            return true;
        }


        template<typename _Ty>
        bool SetBufferArrayElement(uint32_t frameIndex, uint32_t set, uint32_t binding, uint32_t index, const _Ty& value, uint32_t innerElementOffset = 0)
        {
            // size of each element
            static constexpr size_t _type_size = sizeof(_Ty);
            Material::SetBinding sb = {};
            uint64_t offset = (_type_size * index) + innerElementOffset;
            m_UniformBuffers[sb.m_Data].m_Buffer.Set(frameIndex, value, offset);
        }

        template<typename _Ty>
        bool SetMember(const String& name, const _Ty& value)
        {
            static constexpr size_t _type_size = sizeof(_Ty);
            if (m_UniformBufferAccessors.find(name) == m_UniformBufferAccessors.end())
            {
                return false;
            }

            ShaderAccessorData& data = m_UniformBufferAccessors.at(name);

            if (_type_size != data.m_ExpectedSize)
            {
                return false;
            }

            for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
            {
                // update uniform buffer
                m_UniformBuffers[data.m_BufferIndex].m_Buffer.Set(i, value, data.m_Offset);
            }

            return true;
        }

        bool SetSampler(VkState & vk, const String& name, const VkImageView& imageView, const VkSampler& sampler, bool isAttachment = false);
        bool SetSampler(VkState & vk, const String& name, Texture& texture);
        bool SetColourAttachment(VkState & vk, const String& name, Framebuffer& framebuffer, uint32_t colourAttachmentIndex);
        bool SetDepthAttachment(VkState & vk, const String& name, Framebuffer& framebuffer);

        
        void Free(VkState & vk);
    };

}