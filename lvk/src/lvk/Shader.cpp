#include "lvk/Shader.h"
#include "lvk/Macros.h"
#include "spdlog/spdlog.h"
#include "volk.h"

void lvk::ShaderProgram::Free(VkState & vk)
{
    vkDestroyDescriptorSetLayout(vk.m_LogicalDevice, m_DescriptorSetLayout, nullptr);
}

lvk::ShaderProgram lvk::ShaderProgram::CreateCompute(VkState & vk, const String& computePath)

{
    ShaderStage comp = ShaderStage::CreateFromBinaryPath(
        vk, computePath, ShaderStage::Type::Compute);
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
VkShaderModule lvk::CreateShaderModule(lvk::VkState &vk,
                                       const lvk::StageBinary &data) {
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = static_cast<uint32_t>(data.size());
    createInfo.pCode    = reinterpret_cast<const uint32_t*>(data.data());

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(vk.m_LogicalDevice, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
    {
        spdlog::error("Failed to create shader module!");
        std::cerr << "Failed to create shader module" << std::endl;
    }
    return shaderModule;
}
