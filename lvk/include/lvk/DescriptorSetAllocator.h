#pragma once
#include "Alias.h"
#include "vulkan/vulkan.h"

namespace lvk
{
    // ty https://vkguide.dev/docs/new_chapter_4/descriptor_abstractions/
    class DescriptorSetAllocator
    {
    public:



        template<AllocatorType A = MallocAllocator>
        DescriptorSetAllocator(A alloc) :
            m_FreePool(alloc), m_FullPool(alloc), m_Ratios(alloc) {
        };

        struct PoolSizeRatio {
                VkDescriptorType m_DescriptorType;
                float m_Ratio;
        };

        void Init(VkDevice logical_device, uint32_t initialSetAmount, Vector<PoolSizeRatio> ratios);
        void Reset(VkDevice device);
        void Free(VkDevice device);

        VkDescriptorSet Allocate(VkDevice device, VkDescriptorSetLayout layout, void* pNext = nullptr);

        VkDescriptorPool GetPool(VkDevice device);
        VkDescriptorPool CreatePool(VkDevice device, uint32_t setCount);

        Vector<VkDescriptorPool>	m_FreePool;
        Vector<VkDescriptorPool>	m_FullPool;
        Vector<PoolSizeRatio>		m_Ratios;

    protected:
        uint32_t p_SetsPerPool;

    };
}