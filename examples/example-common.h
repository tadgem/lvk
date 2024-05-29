#pragma once

#include "VulkanAPI_SDL.h"
#include "lvk/Framebuffer.h"
#include "lvk/Mesh.h"
#include "assimp/Importer.hpp"
#include "assimp/postprocess.h"
#include "assimp/cimport.h"
#include "assimp/mesh.h"
#include "assimp/scene.h"
#include "spdlog/spdlog.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"
#include "glm/gtc/matrix_transform.hpp"

glm::vec3 CalculateVec3Radians(glm::vec3 eulerDegrees) {
    return glm::vec3(glm::radians(eulerDegrees.x), glm::radians(eulerDegrees.y), glm::radians(eulerDegrees.z));
}

glm::vec3 CalculateVec3Degrees(glm::vec3 eulerRadians) {
    return glm::vec3(glm::degrees(eulerRadians.x), glm::degrees(eulerRadians.y), glm::degrees(eulerRadians.z));
}

glm::quat CalculateRotationQuat(glm::vec3 eulerDegrees) {
    glm::vec3 eulerRadians = CalculateVec3Radians(eulerDegrees);
    glm::quat xRotation = glm::angleAxis(eulerRadians.x, glm::vec3(1, 0, 0));
    glm::quat yRotation = glm::angleAxis(eulerRadians.y, glm::vec3(0, 1, 0));
    glm::quat zRotation = glm::angleAxis(eulerRadians.z, glm::vec3(0, 0, 1));

    return zRotation * yRotation * xRotation;
}


struct MvpData {
    glm::mat4 Model;
    glm::mat4 View;
    glm::mat4 Proj;
};

struct MeshEx
{
    VkBuffer m_VertexBuffer;
    VmaAllocation m_VertexBufferMemory;
    VkBuffer m_IndexBuffer;
    VmaAllocation m_IndexBufferMemory;

    uint32_t m_IndexCount;
    uint32_t m_MaterialIndex;
};

struct MaterialEx
{
    lvk::Texture m_Diffuse;
};

struct Model
{
    lvk::Vector<MeshEx>        m_Meshes;
    lvk::Vector<MaterialEx>  m_Materials;
};

struct Transform {
    glm::vec3 m_Position = glm::vec3(0);
    glm::vec3 m_Rotation = glm::vec3(0);
    glm::vec3 m_Scale = glm::vec3(1);
    glm::mat4 to_mat4() {
        glm::mat4 m = glm::translate(glm::mat4(1), m_Position);
        glm::vec3 radians = CalculateVec3Radians(m_Rotation);
        m *= glm::mat4_cast(glm::quat(radians));
        m = glm::scale(m, m_Scale);
        return m;
    };
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

float Random(float max = 1.0f)
{
    return static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / max));
}

template<size_t _Size>
void FillExampleLightData(FrameLightDataT<_Size>& lightData)
{
    lightData.m_DirectionalLight.Colour = { 0.66f, 0.66f, 0.66f, 0.0f };
    lightData.m_DirectionalLight.Direction = { -13.0f, -2.33f, -10.0f, 0.0f };

    glm::vec4 centerPos = { -0.2f, -0.2f, 0.1f, 0.0f };
    float areaSpread = 1.0f;
    float directionSpread = 12.0f;
    float colourSpread = 100.0f;
    float radiusSpread = 0.02f;

    for (size_t i = 0; i < _Size; i++)
    {
        lightData.m_PointLights[i].PositionRadius = { centerPos.x + Random(areaSpread), centerPos.y + Random(areaSpread), centerPos.z + Random(areaSpread), Random(radiusSpread) };
        lightData.m_PointLights[i].Colour = { Random(colourSpread), Random(colourSpread), Random(colourSpread), 0.0f };

        lightData.m_SpotLights[i].PositionRadius = { centerPos.x + Random(areaSpread), centerPos.y + Random(areaSpread), centerPos.z + Random(areaSpread), Random(radiusSpread) };
        lightData.m_SpotLights[i].DirectionAngle = { Random(directionSpread), Random(directionSpread), -Random(directionSpread), Random(areaSpread) };
        lightData.m_SpotLights[i].Colour = { Random(colourSpread), Random(colourSpread), Random(colourSpread), 0.0f };
    }
}


static glm::vec3 AssimpToGLM(aiVector3D aiVec) {
    return glm::vec3(aiVec.x, aiVec.y, aiVec.z);
}

static glm::vec2 AssimpToGLM(aiVector2D aiVec) {
    return glm::vec2(aiVec.x, aiVec.y);
}

static lvk::String AssimpToSTD(aiString str) {
    return lvk::String(str.C_Str());
}

void FreeMesh(lvk::VulkanAPI& vk, MeshEx& m)
{
    vkDestroyBuffer(vk.m_LogicalDevice, m.m_VertexBuffer, nullptr);
    vmaFreeMemory(vk.m_Allocator, m.m_VertexBufferMemory);
    vkDestroyBuffer(vk.m_LogicalDevice, m.m_IndexBuffer, nullptr);
    vmaFreeMemory(vk.m_Allocator, m.m_IndexBufferMemory);
}

void FreeModel(lvk::VulkanAPI& vk, Model& model)
{
    for (MeshEx& m : model.m_Meshes)
    {
        FreeMesh(vk, m);
    }
}

void ProcessMesh(lvk::VulkanAPI& vk, Model& model, aiMesh* mesh, aiNode* node, const aiScene* scene) {
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

    MeshEx m{};
    vk.CreateVertexBuffer<VertexData>(verts, m.m_VertexBuffer, m.m_VertexBufferMemory);
    vk.CreateIndexBuffer(indices, m.m_IndexBuffer, m.m_IndexBufferMemory);
    m.m_IndexCount = static_cast<uint32_t>(indices.size());

    model.m_Meshes.push_back(m);
}

void ProcessMeshWithNormals(lvk::VulkanAPI& vk, Model& model, aiMesh* mesh, aiNode* node, const aiScene* scene) {
    using namespace lvk;
    bool hasPositions = mesh->HasPositions();
    bool hasUVs = mesh->HasTextureCoords(0);
    bool hasNormals = mesh->HasNormals();
    bool hasIndices = mesh->HasFaces();

    Vector<aiMaterialProperty*> properties;
    aiMaterial* meshMaterial = scene->mMaterials[mesh->mMaterialIndex];

    for (int i = 0; i < meshMaterial->mNumProperties; i++)
    {
        aiMaterialProperty* prop = meshMaterial->mProperties[i];
        properties.push_back(prop);
    }
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

    MeshEx m{};
    vk.CreateVertexBuffer<VertexDataNormal>(verts, m.m_VertexBuffer, m.m_VertexBufferMemory);
    vk.CreateIndexBuffer(indices, m.m_IndexBuffer, m.m_IndexBufferMemory);
    m.m_IndexCount = static_cast<uint32_t>(indices.size());
    m.m_MaterialIndex = mesh->mMaterialIndex;
    model.m_Meshes.push_back(m);
}

void ProcessNode(lvk::VulkanAPI& vk, Model& model, aiNode* node, const aiScene* scene, bool withNormals = false) {

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

void LoadModelAssimp(lvk::VulkanAPI& vk, Model& model, const lvk::String& path, bool withNormals = false)
{
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(path.c_str(),
        aiProcess_Triangulate |
        aiProcess_CalcTangentSpace |
        aiProcess_OptimizeMeshes |
        aiProcess_GenSmoothNormals |
        aiProcess_OptimizeGraph |
        aiProcess_FixInfacingNormals |
        aiProcess_FindInvalidData);
    //
    if (scene == nullptr) {
        spdlog::error("AssimpModelAssetFactory : Failed to load asset at path : {}", path);
        return;
    }
    ProcessNode(vk, model, scene->mRootNode, scene, withNormals);

    lvk::String directory = path.substr(0, path.find_last_of('/') + 1);
    for (int i = 0; i < scene->mNumMaterials; i++)
    {
        aiMaterial* meshMaterial = scene->mMaterials[i];

        uint32_t diffuseCount = aiGetMaterialTextureCount(meshMaterial, aiTextureType_DIFFUSE);

        if (diffuseCount > 0)
        {
            aiString resultPath;
            aiGetMaterialTexture(meshMaterial, aiTextureType_DIFFUSE, 0, &resultPath);
            lvk::String finalPath = directory + lvk::String(resultPath.C_Str());
            lvk::Texture texture = lvk::Texture::CreateTexture(vk, finalPath, VK_FORMAT_R8G8B8A8_UNORM);
            model.m_Materials.push_back({ texture });
        }
    }
}

MeshEx BuildScreenSpaceQuad(lvk::VulkanAPI& vk, lvk::Vector <lvk::VertexData > & verts, lvk::Vector<uint32_t>& indices)
{
    VkBuffer vertexBuffer;
    VmaAllocation vertexBufferMemory;
    VkBuffer indexBuffer;
    VmaAllocation indexBufferMemory;
    vk.CreateVertexBuffer<lvk::VertexData>(verts, vertexBuffer, vertexBufferMemory);
    vk.CreateIndexBuffer(indices, indexBuffer, indexBufferMemory);

    return MeshEx{ vertexBuffer, vertexBufferMemory, indexBuffer, indexBufferMemory, 6 };
}


