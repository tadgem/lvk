#include "lvk/Mesh.h"
#include "lvk/Buffer.h"
#include "volk.h"

lvk::Mesh* lvk::Mesh::g_ScreenSpaceQuad = nullptr;

static lvk::StaticVector<lvk::VertexDataPosUv> g_ScreenSpaceQuadVertexData = {
    { { -1.0f, -1.0f , 0.0f}, { 0.0f, 0.0f } },
    { {1.0f, -1.0f, 0.0f}, {1.0f, 0.0f} },
    { {1.0f, 1.0f, 0.0f}, {1.0f, 1.0f} },
    { {-1.0f, 1.0f, 0.0f}, {0.0f, 1.0f} }
};

static lvk::StaticVector<uint32_t> g_ScreenSpaceQuadIndexData = {
    0, 1, 2, 2, 3, 0
};

void lvk::Mesh::InitBuiltInMeshes(lvk::VkState & vk)
{
    Buffer vertexBuffer = buffers::CreateVertexBuffer<VertexDataPosUv>(vk, g_ScreenSpaceQuadVertexData.data(), g_ScreenSpaceQuadVertexData.size());
    Buffer indexBuffer = buffers::CreateIndexBuffer(vk, g_ScreenSpaceQuadIndexData.data(), g_ScreenSpaceQuadIndexData.size());

    g_ScreenSpaceQuad = new Mesh { vertexBuffer, indexBuffer,  6 };
}

void lvk::Mesh::FreeBuiltInMeshes(lvk::VkState & vk)
{
    g_ScreenSpaceQuad->Free(vk);
}

void lvk::Mesh::Free (VkState & vk)
{
    m_VertexBuffer.Free(vk);
    m_IndexBuffer.Free(vk);
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
