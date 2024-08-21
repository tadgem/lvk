#include "lvk/Shader.h"
#include "volk.h"

void lvk::ShaderProgram::Free(VulkanAPI& vk)
{
    vkDestroyDescriptorSetLayout(vk.m_LogicalDevice, m_DescriptorSetLayout, nullptr);
}

lvk::ShaderProgram lvk::ShaderProgram::CreateCompute(VulkanAPI& vk, const String& computePath)

{
    ShaderStage comp = ShaderStage::Create(vk, computePath, ShaderStage::Type::Compute);
    VkDescriptorSetLayout layout;

    std::vector<VkDescriptorSetLayoutBinding> bindings;
    for (auto& layout : comp.m_LayoutDatas)
    {
        for (auto& binding : layout.m_Bindings)
        {
            bindings.push_back(binding);
        }
    }
    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();

    VK_CHECK(vkCreateDescriptorSetLayout(vk.m_LogicalDevice, &layoutInfo, nullptr, &layout))

    return { Vector<ShaderStage> {comp} , layout };
}
