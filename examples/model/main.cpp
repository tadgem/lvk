#include "example-common.h"
#include "assimp/Importer.hpp"
#include "assimp/postprocess.h"
#include "assimp/cimport.h"
#include "assimp/mesh.h"
#include "assimp/scene.h"
using namespace lvk;

static glm::vec3 AssimpToGLM(aiVector3D aiVec) {
    return glm::vec3(aiVec.x, aiVec.y, aiVec.z);
}

static glm::vec2 AssimpToGLM(aiVector2D aiVec) {
    return glm::vec2(aiVec.x, aiVec.y);
}

static String AssimpToSTD(aiString str) {
    return String(str.C_Str());
}

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

void FreeModel(VulkanAPI_SDL& vk, Model& model)
{
    for (Mesh& m : model.m_Meshes)
    {
        vkDestroyBuffer(vk.m_LogicalDevice, m.m_VertexBuffer, nullptr);
        vmaFreeMemory(vk.m_Allocator, m.m_VertexBufferMemory);
        vkDestroyBuffer(vk.m_LogicalDevice, m.m_IndexBuffer, nullptr);
        vmaFreeMemory(vk.m_Allocator, m.m_IndexBufferMemory);
    }
}

void ProcessMesh(VulkanAPI_SDL& vk, Model& model, aiMesh* mesh, aiNode* node, const aiScene* scene) {

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

void ProcessNode(VulkanAPI_SDL& vk, Model& model, aiNode* node, const aiScene* scene) {

    if (node->mNumMeshes > 0) {
        for (unsigned int i = 0; i < node->mNumMeshes; i++) {
            unsigned int sceneIndex = node->mMeshes[i];
            aiMesh* mesh = scene->mMeshes[sceneIndex];
            ProcessMesh(vk, model, mesh, node, scene);
        }
    }

    if (node->mNumChildren == 0) {
        return;
    }

    for (unsigned int i = 0; i < node->mNumChildren; i++) {
        ProcessNode(vk, model, node->mChildren[i], scene);
    }
}

void LoadModelAssimp(VulkanAPI_SDL& vk, Model& model, const String& path)
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
    ProcessNode(vk, model, scene->mRootNode, scene);
}

static std::vector<VkBuffer>            uniformBuffers;
static std::vector<VmaAllocation>       uniformBuffersMemory;
static std::vector<void*>               uniformBuffersMapped;
static std::vector<VkDescriptorSet>     descriptorSets;

void RecordCommandBuffers(VulkanAPI_SDL& vk, VkPipeline& pipeline, VkPipelineLayout& pipelineLayout, Model& model)
{
    vk.RecordGraphicsCommands([&](VkCommandBuffer& commandBuffer, uint32_t frameIndex) {
        // push to example
        std::array<VkClearValue, 2> clearValues{};
        clearValues[0].color = { {0.0f, 0.0f, 0.0f, 1.0f} };
        clearValues[1].depthStencil = { 1.0f, 0 };

        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = vk.m_SwapchainImageRenderPass;
        renderPassInfo.framebuffer = vk.m_SwapChainFramebuffers[frameIndex];
        renderPassInfo.renderArea.offset = { 0,0 };
        renderPassInfo.renderArea.extent = vk.m_SwapChainImageExtent;

        renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
        renderPassInfo.pClearValues = clearValues.data();

        vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

        for (int i = 0; i < model.m_Meshes.size(); i++)
        {
            Mesh& mesh = model.m_Meshes[i];
            VkBuffer vertexBuffers[]{ mesh.m_VertexBuffer};
            VkDeviceSize sizes[] = { 0 };

            VkViewport viewport{};
            viewport.x = 0.0f;
            viewport.x = 0.0f;
            viewport.width = static_cast<float>(vk.m_SwapChainImageExtent.width);
            viewport.height = static_cast<float>(vk.m_SwapChainImageExtent.height);
            viewport.minDepth = 0.0f;
            viewport.maxDepth = 1.0f;

            VkRect2D scissor{};
            scissor.offset = { 0,0 };
            scissor.extent = VkExtent2D{ 
                static_cast<uint32_t>(vk.m_SwapChainImageExtent.width) , 
                static_cast<uint32_t>(vk.m_SwapChainImageExtent.height)
            };

            vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
            vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
            vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, sizes);
            vkCmdBindIndexBuffer(commandBuffer, mesh.m_IndexBuffer, 0, VK_INDEX_TYPE_UINT32);
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets[frameIndex], 0, nullptr);
            vkCmdDrawIndexed(commandBuffer, mesh.m_IndexCount, 1, 0, 0, 0);
        }
        vkCmdEndRenderPass(commandBuffer);
    });
}

void UpdateUniformBuffer(VulkanAPI_SDL& vk)
{
    static auto startTime = std::chrono::high_resolution_clock::now();

    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

    MvpData ubo{};
    ubo.Model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));

    ubo.View = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));

    ubo.Proj = glm::perspective(glm::radians(45.0f), vk.m_SwapChainImageExtent.width / (float)vk.m_SwapChainImageExtent.height, 0.1f, 300.0f);
    ubo.Proj[1][1] *= -1;

    memcpy(uniformBuffersMapped[vk.GetFrameIndex()], &ubo, sizeof(ubo));
}

void CreateDescriptorSets(VulkanAPI_SDL& vk, VkDescriptorSetLayout& descriptorSetLayout, VkImageView& textureImageView, VkSampler& textureSampler)
{
    std::vector<VkDescriptorSetLayout> layouts(vk.MAX_FRAMES_IN_FLIGHT, descriptorSetLayout);
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = vk.m_DescriptorPool;
    allocInfo.descriptorSetCount = static_cast<uint32_t>(vk.MAX_FRAMES_IN_FLIGHT);
    allocInfo.pSetLayouts = layouts.data();

    descriptorSets.resize(vk.MAX_FRAMES_IN_FLIGHT);
    VK_CHECK(vkAllocateDescriptorSets(vk.m_LogicalDevice, &allocInfo, descriptorSets.data()));

    for (size_t i = 0; i < vk.MAX_FRAMES_IN_FLIGHT; i++) {
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = uniformBuffers[i];
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(MvpData);

        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = textureImageView;
        imageInfo.sampler = textureSampler;

        std::array<VkWriteDescriptorSet, 2> descriptorWrites{};

        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstSet = descriptorSets[i];
        descriptorWrites[0].dstBinding = 0;
        descriptorWrites[0].dstArrayElement = 0;
        descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].pBufferInfo = &bufferInfo;

        descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[1].dstSet = descriptorSets[i];
        descriptorWrites[1].dstBinding = 1;
        descriptorWrites[1].dstArrayElement = 0;
        descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrites[1].descriptorCount = 1;
        descriptorWrites[1].pImageInfo = &imageInfo;

        vkUpdateDescriptorSets(vk.m_LogicalDevice, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
    }

}

int main()
{
    VulkanAPI_SDL vk;
    vk.Start(1280, 720);

    auto vertBin = vk.LoadSpirvBinary("shaders/texture.vert.spv");
    auto fragBin = vk.LoadSpirvBinary("shaders/texture.frag.spv");

    auto vertexLayoutDatas = vk.ReflectDescriptorSetLayouts(vertBin);
    auto fragmentLayoutDatas = vk.ReflectDescriptorSetLayouts(fragBin);
    VkDescriptorSetLayout descriptorSetLayout;
    vk.CreateDescriptorSetLayout(vertexLayoutDatas, fragmentLayoutDatas, descriptorSetLayout);

    VkImage textureImage;
    VkImageView imageView;
    VkDeviceMemory textureMemory;
    vk.CreateTexture("assets/viking_room.png", VK_FORMAT_R8G8B8A8_SRGB, textureImage, imageView, textureMemory);
    VkSampler imageSampler;
    vk.CreateImageSampler(imageView, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_REPEAT, imageSampler);

    VkPipelineLayout pipelineLayout;

    VkPipeline pipeline = vk.CreateRasterizationGraphicsPipeline(
        vertBin, fragBin,
        descriptorSetLayout, Vector<VkVertexInputBindingDescription>{VertexData::GetBindingDescription() }, VertexData::GetAttributeDescriptions(),
        vk.m_SwapchainImageRenderPass,
        vk.m_SwapChainImageExtent.width, vk.m_SwapChainImageExtent.height,
        VK_POLYGON_MODE_FILL,
        VK_CULL_MODE_NONE,
        false, // no msaa atm
        VK_COMPARE_OP_LESS,
        pipelineLayout);

    // create vertex and index buffer
    Model model;
    LoadModelAssimp(vk, model, "assets/viking_room.obj");

    vk.CreateUniformBuffers<MvpData>(uniformBuffers, uniformBuffersMemory, uniformBuffersMapped);

    CreateDescriptorSets(vk, descriptorSetLayout, imageView, imageSampler);

    while (vk.ShouldRun())
    {    
        vk.PreFrame();
        
        UpdateUniformBuffer(vk);

        if (ImGui::Begin("Help"))
        {

        }
        ImGui::End();
        RecordCommandBuffers(vk, pipeline, pipelineLayout, model);

        vk.PostFrame();
    }

    for (size_t i = 0; i < vk.MAX_FRAMES_IN_FLIGHT; i++) {
        vmaUnmapMemory(vk.m_Allocator, uniformBuffersMemory[i]);
        vkDestroyBuffer(vk.m_LogicalDevice, uniformBuffers[i], nullptr);
        vmaFreeMemory(vk.m_Allocator, uniformBuffersMemory[i]);
    }
    FreeModel(vk, model);

    vkDestroySampler(vk.m_LogicalDevice, imageSampler, nullptr);
    vkDestroyImageView(vk.m_LogicalDevice, imageView, nullptr);
    vkDestroyImage(vk.m_LogicalDevice, textureImage, nullptr);
    vkFreeMemory(vk.m_LogicalDevice, textureMemory, nullptr);
    vkDestroyDescriptorSetLayout(vk.m_LogicalDevice, descriptorSetLayout, nullptr);
    vkDestroyPipelineLayout(vk.m_LogicalDevice, pipelineLayout, nullptr);
    vkDestroyPipeline(vk.m_LogicalDevice, pipeline, nullptr);

    return 0;
}