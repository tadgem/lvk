#include "lvk/Mesh.h"
#include "lvk/Buffer.h"
#include "volk.h"

lvk::Mesh* lvk::Mesh::g_ScreenSpaceQuad = nullptr;

static lvk::Vector<lvk::VertexDataPosUv> g_ScreenSpaceQuadVertexData = {
    { { -1.0f, -1.0f , 0.0f}, { 0.0f, 0.0f } },
    { {1.0f, -1.0f, 0.0f}, {1.0f, 0.0f} },
    { {1.0f, 1.0f, 0.0f}, {1.0f, 1.0f} },
    { {-1.0f, 1.0f, 0.0f}, {0.0f, 1.0f} }
};

static lvk::Vector<uint32_t> g_ScreenSpaceQuadIndexData = {
    0, 1, 2, 2, 3, 0
};

void lvk::Mesh::InitBuiltInMeshes(lvk::VkState & vk)
{
    VkBuffer vertexBuffer;
    VmaAllocation vertexBufferMemory;
    VkBuffer indexBuffer;
    VmaAllocation indexBufferMemory;
    buffers::CreateVertexBuffer<VertexDataPosUv>(vk, g_ScreenSpaceQuadVertexData, vertexBuffer, vertexBufferMemory);
    buffers::CreateIndexBuffer(vk, g_ScreenSpaceQuadIndexData, indexBuffer, indexBufferMemory);

    g_ScreenSpaceQuad = new Mesh { vertexBuffer, vertexBufferMemory, indexBuffer, indexBufferMemory, 6 };
}

void lvk::Mesh::FreeBuiltInMeshes(lvk::VkState & vk)
{
    vkDestroyBuffer(vk.m_LogicalDevice, g_ScreenSpaceQuad->m_VertexBuffer, nullptr);
    vmaFreeMemory(vk.m_Allocator, g_ScreenSpaceQuad->m_VertexBufferMemory);
    vkDestroyBuffer(vk.m_LogicalDevice, g_ScreenSpaceQuad->m_IndexBuffer, nullptr);
    vmaFreeMemory(vk.m_Allocator, g_ScreenSpaceQuad->m_IndexBufferMemory);
}

void lvk::Mesh::Free (VkState & vk)
{
    vkDestroyBuffer (vk.m_LogicalDevice, m_VertexBuffer, nullptr);
    vkDestroyBuffer (vk.m_LogicalDevice, m_IndexBuffer, nullptr);
    vmaFreeMemory (vk.m_Allocator, m_IndexBufferMemory);
    vmaFreeMemory (vk.m_Allocator, m_VertexBufferMemory);
}

void lvk::Renderable::RecordGraphicsCommands(VkCommandBuffer& commandBuffer)
{
    VkBuffer vertexBuffers[]{ m_Mesh.m_VertexBuffer };
    VkDeviceSize sizes[] = { 0 };

    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, sizes);
    vkCmdBindIndexBuffer(commandBuffer, m_Mesh.m_IndexBuffer, 0, VK_INDEX_TYPE_UINT32);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_PipelineLayout, 0, 1, &m_DescriptorSet, 0, nullptr);
    vkCmdDrawIndexed(commandBuffer, m_Mesh.m_IndexCount, 1, 0, 0, 0);
}
lvk::VertexDescription lvk::VertexDataPosColUv::GetVertexDescription() {
    return VertexDescription {{GetBindingDescription()}, GetAttributeDescriptions()};
}
lvk::VertexDescription lvk::VertexDataPos4::GetVertexDescription() {
    return VertexDescription {{GetBindingDescription()}, GetAttributeDescriptions()};
}
lvk::VertexDescription lvk::VertexDataPosUv::GetVertexDescription() {
    return VertexDescription {{GetBindingDescription()}, GetAttributeDescriptions()};
}
lvk::VertexDescription lvk::VertexDataPosNormalUv::GetVertexDescription() {
    return VertexDescription {{GetBindingDescription()}, GetAttributeDescriptions()};
}
