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

#define GET_VERTEX_DESCRIPTION_IMPL \
VertexDescription vd(alloc);\
vd.m_AttributeDescriptions = GetAttributeDescriptions(alloc);\
vd.m_BindingDescriptions = Vector<VkVertexInputBindingDescription>(alloc);\
vd.m_BindingDescriptions.push_back(GetBindingDescription());\
return vd;

lvk::VertexDescription lvk::VertexDataPosColUv::GetVertexDescription(IAllocator& alloc) {
    GET_VERTEX_DESCRIPTION_IMPL;
}
lvk::VertexDescription lvk::VertexDataPos4::GetVertexDescription(IAllocator& alloc) {
    GET_VERTEX_DESCRIPTION_IMPL;
}
lvk::VertexDescription lvk::VertexDataPosUv::GetVertexDescription(IAllocator& alloc) {
    GET_VERTEX_DESCRIPTION_IMPL;
}
lvk::VertexDescription lvk::VertexDataPosNormalUv::GetVertexDescription(IAllocator& alloc) {
    GET_VERTEX_DESCRIPTION_IMPL;
}
