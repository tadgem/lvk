#pragma once

#include "VulkanAPI_SDL.h"
#include "assimp/Importer.hpp"
#include "assimp/postprocess.h"
#include "assimp/cimport.h"
#include "assimp/mesh.h"
#include "assimp/scene.h"
#include "spdlog/spdlog.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include "glm/glm.hpp"
#include <glm/gtc/matrix_transform.hpp>

struct VertexDataCol
{
    glm::vec3 Position;
    glm::vec3 Colour;
    glm::vec2 UV;

    static VkVertexInputBindingDescription GetBindingDescription() {
        VkVertexInputBindingDescription bindingDescription{};

        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        bindingDescription.stride = sizeof(VertexDataCol);
        bindingDescription.binding = 0;

        return bindingDescription;
    }

    static std::vector<VkVertexInputAttributeDescription> GetAttributeDescriptions() {
        std::vector<VkVertexInputAttributeDescription> attributeDescriptions{};

        attributeDescriptions.resize(3);

        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(VertexDataCol , Position);

        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(VertexDataCol, Colour);

        attributeDescriptions[2].binding = 0;
        attributeDescriptions[2].location = 2;
        attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[2].offset = offsetof(VertexDataCol, UV);

        return attributeDescriptions;
    }
};

struct VertexData
{
    glm::vec3 Position;
    glm::vec2 UV;

    static VkVertexInputBindingDescription GetBindingDescription() {
        VkVertexInputBindingDescription bindingDescription{};

        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        bindingDescription.stride = sizeof(VertexData);
        bindingDescription.binding = 0;

        return bindingDescription;
    }

    static std::vector<VkVertexInputAttributeDescription> GetAttributeDescriptions() {
        std::vector<VkVertexInputAttributeDescription> attributeDescriptions{};

        attributeDescriptions.resize(2);

        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(VertexData, Position);

        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(VertexData, UV);

        return attributeDescriptions;
    }
};

struct VertexDataNormal
{
    glm::vec3 Position;
    glm::vec3 Normal;
    glm::vec2 UV;

    static VkVertexInputBindingDescription GetBindingDescription() {
        VkVertexInputBindingDescription bindingDescription{};

        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        bindingDescription.stride = sizeof(VertexDataNormal);
        bindingDescription.binding = 0;

        return bindingDescription;
    }

    static std::vector<VkVertexInputAttributeDescription> GetAttributeDescriptions() {
        std::vector<VkVertexInputAttributeDescription> attributeDescriptions{};

        attributeDescriptions.resize(3);

        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(VertexDataNormal, Position);

        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(VertexDataNormal, Normal);

        attributeDescriptions[2].binding = 0;
        attributeDescriptions[2].location = 2;
        attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[2].offset = offsetof(VertexDataNormal, UV);

        return attributeDescriptions;
    }
};

struct MvpData {
    glm::mat4 Model;
    glm::mat4 View;
    glm::mat4 Proj;
};

struct Mesh
{
    VkBuffer m_VertexBuffer;
    VmaAllocation m_VertexBufferMemory;
    VkBuffer m_IndexBuffer;
    VmaAllocation m_IndexBufferMemory;

    uint32_t m_IndexCount;
};

struct Model
{
    std::vector<Mesh> m_Meshes;
};

struct DirectionalLight
{
    glm::vec4 Direction;
    glm::vec4 Ambient;
    glm::vec4 Colour;
};

struct PointLight
{
    glm::vec4 PositionRadius;
    glm::vec4 Ambient;
    glm::vec4 Colour;
};

struct SpotLight
{
    glm::vec4 PositionRadius;
    glm::vec4 DirectionAngle;
    glm::vec4 Ambient;
    glm::vec4 Colour;

};

template<size_t _Size>
struct FrameLightDataT
{
    DirectionalLight                m_DirectionalLight;
    std::array<PointLight, _Size>   m_PointLights;
    std::array<SpotLight, _Size>    m_SpotLights;

    uint32_t            m_DirectionalLightActive;
    uint32_t            m_PointLightsActive;
    uint32_t            m_SpotLightsActive;
};


static glm::vec3 AssimpToGLM(aiVector3D aiVec) {
    return glm::vec3(aiVec.x, aiVec.y, aiVec.z);
}

static glm::vec2 AssimpToGLM(aiVector2D aiVec) {
    return glm::vec2(aiVec.x, aiVec.y);
}

static lvk::String AssimpToSTD(aiString str) {
    return lvk::String(str.C_Str());
}

void FreeMesh(lvk::VulkanAPI& vk, Mesh& m)
{
    vkDestroyBuffer(vk.m_LogicalDevice, m.m_VertexBuffer, nullptr);
    vmaFreeMemory(vk.m_Allocator, m.m_VertexBufferMemory);
    vkDestroyBuffer(vk.m_LogicalDevice, m.m_IndexBuffer, nullptr);
    vmaFreeMemory(vk.m_Allocator, m.m_IndexBufferMemory);
}

void FreeModel(lvk::VulkanAPI_SDL& vk, Model& model)
{
    for (Mesh& m : model.m_Meshes)
    {
        FreeMesh(vk, m);
    }
}

void ProcessMesh(lvk::VulkanAPI_SDL& vk, Model& model, aiMesh* mesh, aiNode* node, const aiScene* scene) {
    using namespace lvk;
    bool hasPositions = mesh->HasPositions();
    bool hasUVs = mesh->HasTextureCoords(0);
    bool hasIndices = mesh->HasFaces();

    Vector<VertexData> verts;
    if (hasPositions && hasUVs) {
        for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
            VertexData vert {};
            vert.Position = AssimpToGLM(mesh->mVertices[i]);
            vert.UV = glm::vec2(mesh->mTextureCoords[0][i].x, 1.0f - mesh->mTextureCoords[0][i].y);
            verts.push_back(vert);
        }

    }
    Vector<uint32_t> indices;
    if (hasIndices) {
        for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
            aiFace currentFace = mesh->mFaces[i];
            if (currentFace.mNumIndices != 3) {
                spdlog::error("Attempting to import a mesh with non triangular face structure! cannot load this mesh.");
                return;
            }
            for (unsigned int index = 0; index < mesh->mFaces[i].mNumIndices; index++) {
                indices.push_back(static_cast<uint32_t>(mesh->mFaces[i].mIndices[index]));
            }
        }
    }

    Mesh m{};
    vk.CreateVertexBuffer<VertexData>(verts, m.m_VertexBuffer, m.m_VertexBufferMemory);
    vk.CreateIndexBuffer(indices, m.m_IndexBuffer, m.m_IndexBufferMemory);
    m.m_IndexCount = static_cast<uint32_t>(indices.size());

    model.m_Meshes.push_back(m);
}

void ProcessMeshWithNormals(lvk::VulkanAPI_SDL& vk, Model& model, aiMesh* mesh, aiNode* node, const aiScene* scene) {
    using namespace lvk;
    bool hasPositions = mesh->HasPositions();
    bool hasUVs = mesh->HasTextureCoords(0);
    bool hasNormals = mesh->HasNormals();
    bool hasIndices = mesh->HasFaces();


    Vector<VertexDataNormal> verts;
    if (hasPositions && hasUVs && hasNormals) {
        for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
            VertexDataNormal vert{};
            vert.Position = AssimpToGLM(mesh->mVertices[i]);
            vert.UV = glm::vec2(mesh->mTextureCoords[0][i].x, 1.0f - mesh->mTextureCoords[0][i].y);
            vert.Normal = AssimpToGLM(mesh->mNormals[i]);
            verts.push_back(vert);
        }

    }
    Vector<uint32_t> indices;
    if (hasIndices) {
        for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
            aiFace currentFace = mesh->mFaces[i];
            if (currentFace.mNumIndices != 3) {
                spdlog::error("Attempting to import a mesh with non triangular face structure! cannot load this mesh.");
                return;
            }
            for (unsigned int index = 0; index < mesh->mFaces[i].mNumIndices; index++) {
                indices.push_back(static_cast<uint32_t>(mesh->mFaces[i].mIndices[index]));
            }
        }
    }

    Mesh m{};
    vk.CreateVertexBuffer<VertexDataNormal>(verts, m.m_VertexBuffer, m.m_VertexBufferMemory);
    vk.CreateIndexBuffer(indices, m.m_IndexBuffer, m.m_IndexBufferMemory);
    m.m_IndexCount = static_cast<uint32_t>(indices.size());

    model.m_Meshes.push_back(m);
}

void ProcessNode(lvk::VulkanAPI_SDL& vk, Model& model, aiNode* node, const aiScene* scene, bool withNormals = false) {

    if (node->mNumMeshes > 0) {
        for (unsigned int i = 0; i < node->mNumMeshes; i++) {
            unsigned int sceneIndex = node->mMeshes[i];
            aiMesh* mesh = scene->mMeshes[sceneIndex];
            if (withNormals)
            {
                ProcessMeshWithNormals(vk, model, mesh, node, scene);
            }
            else
            {
                ProcessMesh(vk, model, mesh, node, scene);
            }
        }
    }

    if (node->mNumChildren == 0) {
        return;
    }

    for (unsigned int i = 0; i < node->mNumChildren; i++) {
        ProcessNode(vk, model, node->mChildren[i], scene);
    }
}

void LoadModelAssimp(lvk::VulkanAPI_SDL& vk, Model& model, const lvk::String& path, bool withNormals = false)
{
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(path.c_str(),
        aiProcess_Triangulate |
        aiProcess_CalcTangentSpace |
        aiProcess_OptimizeMeshes |
        aiProcess_OptimizeGraph |
        aiProcess_FindInvalidData );
    //
    if (scene == nullptr) {
        spdlog::error("AssimpModelAssetFactory : Failed to load asset at path : {}", path);
        return;
    }
    ProcessNode(vk, model, scene->mRootNode, scene, withNormals);
}

Mesh BuildScreenSpaceQuad(lvk::VulkanAPI& vk, lvk::Vector<VertexData>& verts, lvk::Vector<uint32_t>& indices)
{
    VkBuffer vertexBuffer;
    VmaAllocation vertexBufferMemory;
    VkBuffer indexBuffer;
    VmaAllocation indexBufferMemory;
    vk.CreateVertexBuffer<VertexData>(verts, vertexBuffer, vertexBufferMemory);
    vk.CreateIndexBuffer(indices, indexBuffer, indexBufferMemory);

    return Mesh{ vertexBuffer, vertexBufferMemory, indexBuffer, indexBufferMemory, 6 };
}