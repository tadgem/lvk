#pragma once
#include "VulkanAPI_SDL.h"
#include "assimp/Importer.hpp"
#include "assimp/cimport.h"
#include "assimp/mesh.h"
#include "assimp/postprocess.h"
#include "assimp/scene.h"
#include "lvk/Framebuffer.h"
#include "lvk/Material.h"
#include "lvk/Mesh.h"
#include "lvk/VkBackend.h"
#include "spdlog/spdlog.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"
#include "glm/gtc/matrix_transform.hpp"

#include "volk.h"

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

struct MvpData2 {
    glm::mat4 Model;
};


struct AABB
{
    glm::vec3 m_Min;
    glm::vec3 m_Max;
};



struct MeshEx
{
    VkBuffer m_VertexBuffer;
    VmaAllocation m_VertexBufferMemory;
    VkBuffer m_IndexBuffer;
    VmaAllocation m_IndexBufferMemory;
    AABB m_AABB;
    glm::mat4 m_OBB;

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

void FreeMesh(lvk::VkBackend & vk, MeshEx& m)
{
    vkDestroyBuffer(vk.m_LogicalDevice, m.m_VertexBuffer, nullptr);
    vmaFreeMemory(vk.m_Allocator, m.m_VertexBufferMemory);
    vkDestroyBuffer(vk.m_LogicalDevice, m.m_IndexBuffer, nullptr);
    vmaFreeMemory(vk.m_Allocator, m.m_IndexBufferMemory);
}

void FreeModel(lvk::VkBackend & vk, Model& model)
{
    for (MeshEx& m : model.m_Meshes)
    {
        FreeMesh(vk, m);
    }
}

void ProcessMesh(lvk::VkBackend & vk, Model& model, aiMesh* mesh, aiNode* node, const aiScene* scene) {
    using namespace lvk;
    bool hasPositions = mesh->HasPositions();
    bool hasUVs = mesh->HasTextureCoords(0);
    bool hasIndices = mesh->HasFaces();

    Vector<VertexDataPosUv> verts;
    if (hasPositions && hasUVs) {
        for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
            VertexDataPosUv vert {};
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
    AABB aabb = { {mesh->mAABB.mMin.x, mesh->mAABB.mMin.y, mesh->mAABB.mMin.z},
                    {mesh->mAABB.mMax.x, mesh->mAABB.mMax.y, mesh->mAABB.mMax.z} };
    MeshEx m{};
    vk.CreateVertexBuffer<VertexDataPosUv>(verts, m.m_VertexBuffer, m.m_VertexBufferMemory);
    vk.CreateIndexBuffer(indices, m.m_IndexBuffer, m.m_IndexBufferMemory);
    m.m_IndexCount = static_cast<uint32_t>(indices.size());
    m.m_AABB = aabb;
    model.m_Meshes.push_back(m);
}

AABB TransformAABB(AABB& in, glm::mat4& m)
{
    float scalingFactor = m[0][0] * m[0][0] + m[0][1] * m[0][1] + m[0][2] * m[0][2];
    float f = 1.0f / scalingFactor;
    glm::mat3 rotationMatrix =
    {
        {m[0][0] * f, m[0][1] * f, m[0][2] * f},
        {m[1][0] * f, m[1][1] * f, m[1][2] * f},
        {m[2][0] * f, m[2][1] * f, m[2][2] * f}
    };

    glm::vec3 translation = { m[0][3],m[1][3], m[2][3] };
    AABB ret;
    ret.m_Min = glm::vec3(0.0f);
    ret.m_Max = glm::vec3(0.0f);

    for (int i = 0; i < 3; i++)
    {
        for (int j = 0; j < 3; j++)
        {
            auto a = rotationMatrix[i][j] * in.m_Min[j];
            auto b = rotationMatrix[i][j] * in.m_Max[j];

            ret.m_Min[i] += a < b ? a : b;
            ret.m_Max[i] += a < b ? b : a;
        }
    }

    return ret;
}

void ProcessMeshWithNormals(lvk::VkBackend & vk, Model& model, aiMesh* mesh, aiNode* node, const aiScene* scene) {
    using namespace lvk;
    bool hasPositions = mesh->HasPositions();
    bool hasUVs = mesh->HasTextureCoords(0);
    bool hasNormals = mesh->HasNormals();
    bool hasIndices = mesh->HasFaces();

    Vector<aiMaterialProperty*> properties;
    aiMaterial* meshMaterial = scene->mMaterials[mesh->mMaterialIndex];

    for (unsigned int i = 0; i < meshMaterial->mNumProperties; i++)
    {
        aiMaterialProperty* prop = meshMaterial->mProperties[i];
        properties.push_back(prop);
    }
    Vector<VertexDataPosNormalUv> verts;
    if (hasPositions && hasUVs && hasNormals) {
        for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
            VertexDataPosNormalUv vert{};
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
    AABB aabb = { {mesh->mAABB.mMin.x, mesh->mAABB.mMin.y, mesh->mAABB.mMin.z},
                {mesh->mAABB.mMax.x, mesh->mAABB.mMax.y, mesh->mAABB.mMax.z} };

    MeshEx m{};
    vk.CreateVertexBuffer<VertexDataPosNormalUv>(verts, m.m_VertexBuffer, m.m_VertexBufferMemory);
    vk.CreateIndexBuffer(indices, m.m_IndexBuffer, m.m_IndexBufferMemory);
    m.m_IndexCount = static_cast<uint32_t>(indices.size());
    m.m_MaterialIndex = mesh->mMaterialIndex;
    m.m_AABB = aabb;
    model.m_Meshes.push_back(m);
}

void ProcessNode(lvk::VkBackend & vk, Model& model, aiNode* node, const aiScene* scene, bool withNormals = false) {

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

void LoadModelAssimp(lvk::VkBackend & vk, Model& model, const lvk::String& path, bool withNormals = false)
{
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(path.c_str(),
        aiProcess_Triangulate |
        aiProcess_CalcTangentSpace |
        aiProcess_OptimizeMeshes |
        aiProcess_GenSmoothNormals |
        aiProcess_OptimizeGraph |
        aiProcess_FixInfacingNormals |
        aiProcess_FindInvalidData | 
        aiProcess_GenBoundingBoxes
    );
    //
    if (scene == nullptr) {
        spdlog::error("AssimpModelAssetFactory : Failed to load asset at path : {}", path);
        return;
    }
    ProcessNode(vk, model, scene->mRootNode, scene, withNormals);

    lvk::String directory = path.substr(0, path.find_last_of('/') + 1);
    for (unsigned int i = 0; i < scene->mNumMaterials; i++)
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

MeshEx BuildScreenSpaceQuad(lvk::VkBackend & vk, lvk::Vector <lvk::VertexDataPosUv > & verts, lvk::Vector<uint32_t>& indices)
{
    VkBuffer vertexBuffer;
    VmaAllocation vertexBufferMemory;
    VkBuffer indexBuffer;
    VmaAllocation indexBufferMemory;
    vk.CreateVertexBuffer<lvk::VertexDataPosUv>(verts, vertexBuffer, vertexBufferMemory);
    vk.CreateIndexBuffer(indices, indexBuffer, indexBufferMemory);

    return MeshEx{ vertexBuffer, vertexBufferMemory, indexBuffer, indexBufferMemory, {}, 6 };
}


#define NUM_LIGHTS 512
using DeferredLightData = FrameLightDataT<NUM_LIGHTS>;

struct RenderItem
{
    MeshEx              m_Mesh;
    lvk::Material       m_Material;
};

struct RenderModel
{
    Model m_Original;
    lvk::Vector<RenderItem> m_RenderItems;
    void Free(lvk::VkBackend & vk)
    {
        for (auto& item : m_RenderItems)
        {
            item.m_Material.Free(vk);
            FreeMesh(vk, item.m_Mesh);
        }

        for (auto& mat : m_Original.m_Materials)
        {
            mat.m_Diffuse.Free(vk);
        }
        m_RenderItems.clear();
    }
};

struct Camera
{
    glm::vec3 Position;
    glm::vec3 Rotation;
    float FOV = 90.0f;
    float Near = 0.001f, Far = 300.0f;

    glm::mat4 View;
    glm::mat4 Proj;
};