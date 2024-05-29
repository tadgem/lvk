#include "lvk/Mesh.h"

lvk::Mesh* lvk::Mesh::g_ScreenSpaceQuad = nullptr;

static lvk::Vector<lvk::VertexData> g_ScreenSpaceQuadVertexData = {
    { { -1.0f, -1.0f , 0.0f}, { 1.0f, 0.0f } },
    { {1.0f, -1.0f, 0.0f}, {0.0f, 0.0f} },
    { {1.0f, 1.0f, 0.0f}, {0.0f, 1.0f} },
    { {-1.0f, 1.0f, 0.0f}, {1.0f, 1.0f} }
};

static lvk::Vector<uint32_t> g_ScreenSpaceQuadIndexData = {
    0, 1, 2, 2, 3, 0
};

void lvk::Mesh::InitScreenQuad(lvk::VulkanAPI& vk)
{
    VkBuffer vertexBuffer;
    VmaAllocation vertexBufferMemory;
    VkBuffer indexBuffer;
    VmaAllocation indexBufferMemory;
    vk.CreateVertexBuffer<VertexData>(g_ScreenSpaceQuadVertexData, vertexBuffer, vertexBufferMemory);
    vk.CreateIndexBuffer(g_ScreenSpaceQuadIndexData, indexBuffer, indexBufferMemory);

    g_ScreenSpaceQuad = new Mesh { vertexBuffer, vertexBufferMemory, indexBuffer, indexBufferMemory, 6 };
}

void lvk::Mesh::FreeScreenQuad(lvk::VulkanAPI& vk)
{
    vkDestroyBuffer(vk.m_LogicalDevice, g_ScreenSpaceQuad->m_VertexBuffer, nullptr);
    vmaFreeMemory(vk.m_Allocator, g_ScreenSpaceQuad->m_VertexBufferMemory);
    vkDestroyBuffer(vk.m_LogicalDevice, g_ScreenSpaceQuad->m_IndexBuffer, nullptr);
    vmaFreeMemory(vk.m_Allocator, g_ScreenSpaceQuad->m_IndexBufferMemory);
}
