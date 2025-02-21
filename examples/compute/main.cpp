#include "example-common.h"
#include "lvk/Shader.h"
#include <random>

using namespace lvk;

#define NUM_LIGHTS 16
using ForwardLightData = FrameLightDataT<NUM_LIGHTS>;
struct Particle
{
  glm::vec2 position;
  glm::vec2 velocity;
  glm::vec4 colour;

  static std::array<VkVertexInputAttributeDescription, 2> GetAttributeDescriptions()
  {
    std::array<VkVertexInputAttributeDescription, 2> attrs {};
    attrs[0].binding = 0;
    attrs[0].location = 0;
    attrs[0].offset = offsetof(Particle, position);
    attrs[0].format = VK_FORMAT_R32G32_SFLOAT;

    attrs[0].binding = 0;
    attrs[0].location = 1;
    attrs[0].offset = offsetof(Particle, colour);
    attrs[0].format = VK_FORMAT_R32G32B32A32_SFLOAT;

    return attrs;
  }
};

struct UBOData
{
  float delta;
};

constexpr size_t PARTICLE_COUNT = 8192;
constexpr VkDeviceSize buffer_size = PARTICLE_COUNT * sizeof(Particle);

static ShaderBufferFrameData mvpUniformData;
static ShaderBufferFrameData lightsUniformData;

static ForwardLightData lightDataCpu {};
static std::vector<VkDescriptorSet> forwardPassDescriptorSets;
static std::vector<VkDescriptorSet> computePassDescriptorSets;

void CreateGraphicsDescriptorSets(VkState & vk, VkDescriptorSetLayout& descriptorSetLayout, VkImageView& textureImageView, VkSampler& textureSampler)
{
  std::vector<VkDescriptorSetLayout> forward_layouts(MAX_FRAMES_IN_FLIGHT, descriptorSetLayout);
  VkDescriptorSetAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  allocInfo.descriptorPool = vk.m_DescriptorSetAllocator.GetPool(vk.m_LogicalDevice);
  allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
  allocInfo.pSetLayouts = forward_layouts.data();

  forwardPassDescriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
  VK_CHECK(vkAllocateDescriptorSets(vk.m_LogicalDevice, &allocInfo,
                                    forwardPassDescriptorSets.data()));



  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    VkDescriptorBufferInfo mvpBufferInfo{};
    mvpBufferInfo.buffer = mvpUniformData.m_UniformBuffers[i].m_GpuBuffer;
    mvpBufferInfo.offset = 0;
    mvpBufferInfo.range = sizeof(MvpData);

    VkDescriptorImageInfo imageInfo{};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = textureImageView;
    imageInfo.sampler = textureSampler;

    VkDescriptorBufferInfo lightBufferInfo{};
    lightBufferInfo.buffer = lightsUniformData.m_UniformBuffers[i].m_GpuBuffer;
    lightBufferInfo.offset = 0;
    lightBufferInfo.range = sizeof(ForwardLightData);

    std::array<VkWriteDescriptorSet, 3> descriptorWrites{};

    descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[0].dstSet = forwardPassDescriptorSets[i];
    descriptorWrites[0].dstBinding = 0;
    descriptorWrites[0].dstArrayElement = 0;
    descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorWrites[0].descriptorCount = 1;
    descriptorWrites[0].pBufferInfo = &mvpBufferInfo;

    descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[1].dstSet = forwardPassDescriptorSets[i];
    descriptorWrites[1].dstBinding = 1;
    descriptorWrites[1].dstArrayElement = 0;
    descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrites[1].descriptorCount = 1;
    descriptorWrites[1].pImageInfo = &imageInfo;

    descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[2].dstSet = forwardPassDescriptorSets[i];
    descriptorWrites[2].dstBinding = 2;
    descriptorWrites[2].dstArrayElement = 0;
    descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorWrites[2].descriptorCount = 1;
    descriptorWrites[2].pBufferInfo = &lightBufferInfo;


    vkUpdateDescriptorSets(vk.m_LogicalDevice, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
  }

}


void RecordComputeCommandBuffers(VkState& vk, VkPipeline& p, VkPipelineLayout& layout, Vector<VkDescriptorSet>& computeDescriptors)
{
  lvk::commands::RecordComputeCommands(vk, [&](VkCommandBuffer& cmd, uint32_t frameIndex)
  {
      vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, p);
      vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, layout, 0, 1, &computeDescriptors[frameIndex], 0,0);
      vkCmdDispatch(cmd, PARTICLE_COUNT / 256, 1, 1);
  });
}

void RecordGraphicsCommandBuffers(VkState & vk, VkPipeline& pipeline, VkPipelineLayout& pipelineLayout, Model& model)
{
    lvk::commands::RecordGraphicsCommands(vk, [&](VkCommandBuffer& commandBuffer, uint32_t frameIndex) {
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
            MeshEx& mesh = model.m_Meshes[i];
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
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &forwardPassDescriptorSets[frameIndex], 0, nullptr);
            vkCmdDrawIndexed(commandBuffer, mesh.m_IndexCount, 1, 0, 0, 0);
        }
        vkCmdEndRenderPass(commandBuffer);
    });
}



void UpdateUniformBuffer(VkState & vk)
{
    static auto startTime = std::chrono::high_resolution_clock::now();
    

    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

    time = 0.0f;
    MvpData ubo{};
    ubo.Model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.View = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    if (vk.m_SwapChainImageExtent.width > 0 || vk.m_SwapChainImageExtent.height)
    {
        ubo.Proj = glm::perspective(glm::radians(45.0f), vk.m_SwapChainImageExtent.width / (float)vk.m_SwapChainImageExtent.height, 0.1f, 300.0f);
        ubo.Proj[1][1] *= -1;
    }
    mvpUniformData.Set(vk.m_CurrentFrameIndex, ubo);

    // light imgui

    if (ImGui::Begin("Lights"))
    {
        ImGui::Text("FPSS : %f", 1.0 / vk.m_DeltaTime);
        ImGui::DragFloat3("Directional Light Dir", &lightDataCpu.m_DirectionalLight.Direction[0]);
        ImGui::DragFloat4("Directional Light Colour", &lightDataCpu.m_DirectionalLight.Colour[0]);
        ImGui::DragFloat4("Directional Light Ambient Colour", &lightDataCpu.m_DirectionalLight.Ambient[0]);

        if(ImGui::TreeNode("Point Lights"))
        {
            for (int i = 0; i < NUM_LIGHTS; i++)
            {
                ImGui::PushID(i);
                if (ImGui::TreeNode("Point Light"))
                {
                    ImGui::DragFloat3("Position", &lightDataCpu.m_PointLights[i].PositionRadius[0]);
                    ImGui::DragFloat("Radius", &lightDataCpu.m_PointLights[i].PositionRadius[3]);
                    ImGui::DragFloat4("Colour", &lightDataCpu.m_PointLights[i].Colour[0]);
                    ImGui::DragFloat4("Ambient Colour", &lightDataCpu.m_PointLights[i].Ambient[0]);

                    ImGui::TreePop();
                }                

                ImGui::PopID();
            }
            ImGui::TreePop();
        }

        if (ImGui::TreeNode("Spot Lights"))
        {
            for (int i = 0; i < NUM_LIGHTS; i++)
            {
                ImGui::PushID(NUM_LIGHTS + i);
                if (ImGui::TreeNode("Spot Light"))
                {
                    ImGui::DragFloat3("Position", &lightDataCpu.m_SpotLights[i].PositionRadius[0]);
                    ImGui::DragFloat("Radius", &lightDataCpu.m_SpotLights[i].PositionRadius[3]);
                    ImGui::DragFloat3("Direction", &lightDataCpu.m_SpotLights[i].DirectionAngle[0]);
                    ImGui::DragFloat("Angle", &lightDataCpu.m_SpotLights[i].DirectionAngle[3]);
                    ImGui::DragFloat4("Colour", &lightDataCpu.m_SpotLights[i].Colour[0]);
                    ImGui::DragFloat4("Ambient Colour", &lightDataCpu.m_SpotLights[i].Ambient[0]);

                    ImGui::TreePop();
                }

                ImGui::PopID();
            }
            ImGui::TreePop();
        }
    }
    ImGui::End();

    lightsUniformData.Set(vk.m_CurrentFrameIndex, lightDataCpu);
}

static void CreateComputeBuffers(VkState& vk, std::vector<VkBuffer>& uniformBuffers,
                                 std::vector<VmaAllocation>& uniformBuffersMemory,

                                 std::vector<VkBuffer>& shaderStorageBuffers,
                                 std::vector<VmaAllocation>& shaderStorageBuffersMemory)
{
    uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    uniformBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);

    shaderStorageBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    shaderStorageBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);

    std::default_random_engine rndEngine((unsigned)time(nullptr));
    std::uniform_real_distribution<float> rndDist(0.0f, 1.0f);

    // Initial particle positions on a circle
    std::vector<Particle> particles(PARTICLE_COUNT);
    for (auto& particle : particles) {
        float r = 0.25f * sqrt(rndDist(rndEngine));
        float theta = rndDist(rndEngine) * 2 * 3.14159265358979323846;
        float x = r * cos(theta) * vk.m_SwapChainImageExtent.height / vk.m_SwapChainImageExtent.width;
        float y = r * sin(theta);
        particle.position = glm::vec2(x, y);
        particle.velocity = glm::normalize(glm::vec2(x,y)) * 0.00025f;
        particle.colour = glm::vec4(rndDist(rndEngine), rndDist(rndEngine), rndDist(rndEngine), 1.0f);
    }

    VkBuffer stagingBuffer;
    VmaAllocation stagingBufferMemory;
    buffers::CreateBuffer(vk, buffer_size,
                          VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                          stagingBuffer, stagingBufferMemory);

    void* data;
    vmaMapMemory(vk.m_Allocator, stagingBufferMemory, &data);
    memcpy(data, particles.data(), buffer_size);
    vmaUnmapMemory(vk.m_Allocator, stagingBufferMemory);
    for(auto f = 0; f < MAX_FRAMES_IN_FLIGHT; f++)
    {
        buffers::CreateBuffer(vk, sizeof(UBOData),
                              VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                              VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
                              uniformBuffers[f], uniformBuffersMemory[f]);

        buffers::CreateBuffer(vk, buffer_size,
                              VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                                  VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                                  VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                              shaderStorageBuffers[f], shaderStorageBuffersMemory[f]);
        buffers::CopyBuffer(vk, stagingBuffer, shaderStorageBuffers[f], buffer_size);


    }
}

VkDescriptorSetLayout CreateComputeDescriptorLayouts(VkState& vk)
{
    VkDescriptorSetLayout layout {};
    std::array<VkDescriptorSetLayoutBinding, 3> bindings{};
    bindings[0].binding = 0;
    bindings[0].descriptorCount = 1;
    bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    bindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    bindings[0].pImmutableSamplers = nullptr;

    bindings[1].binding = 1;
    bindings[1].descriptorCount = 1;
    bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    bindings[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    bindings[1].pImmutableSamplers = nullptr;

    bindings[2].binding = 2;
    bindings[2].descriptorCount = 1;
    bindings[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    bindings[2].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT ;
    bindings[2].pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutCreateInfo create {};
    create.bindingCount = 3;
    create.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    create.pBindings = bindings.data();

    VK_CHECK(vkCreateDescriptorSetLayout(vk.m_LogicalDevice, &create, nullptr, &layout));
    return layout;
}

Vector<VkDescriptorSet> CreateComputeDescriptorSets(VkState& vk, VkDescriptorSetLayout layout,
                                 std::vector<VkBuffer>& uniformBuffers,
                                 std::vector<VkBuffer>& shaderStorageBuffers)
{

    std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, layout);
    VkDescriptorSetAllocateInfo alloc {};
    alloc.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    alloc.descriptorPool = vk.m_DescriptorSetAllocator.GetPool(vk.m_LogicalDevice);
    alloc.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
    alloc.pSetLayouts= layouts.data();

    std::vector<VkDescriptorSet> sets {};
    sets.resize(MAX_FRAMES_IN_FLIGHT);

    VK_CHECK(vkAllocateDescriptorSets(vk.m_LogicalDevice, &alloc, sets.data()));

    for(uint32_t f = 0; f < MAX_FRAMES_IN_FLIGHT; f++)
    {
        VkDescriptorBufferInfo uniformBufferInfo{};
        uniformBufferInfo.buffer = uniformBuffers[f];
        uniformBufferInfo.offset = 0;
        uniformBufferInfo.range = sizeof(UBOData);

        std::array<VkWriteDescriptorSet, 3> descriptorWrites{};
        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstSet = sets[f];
        descriptorWrites[0].dstBinding = 0;
        descriptorWrites[0].dstArrayElement = 0;
        descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].pBufferInfo = &uniformBufferInfo;

        auto last_index = (f - 1) % MAX_FRAMES_IN_FLIGHT;
        VkDescriptorBufferInfo storageBufferInfoLastFrame{};
        storageBufferInfoLastFrame.buffer = shaderStorageBuffers[last_index];
        storageBufferInfoLastFrame.offset = 0;
        storageBufferInfoLastFrame.range = sizeof(Particle) * PARTICLE_COUNT;

        descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[1].dstSet = sets[f];
        descriptorWrites[1].dstBinding = 1;
        descriptorWrites[1].dstArrayElement = 0;
        descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        descriptorWrites[1].descriptorCount = 1;
        descriptorWrites[1].pBufferInfo = &storageBufferInfoLastFrame;

        VkDescriptorBufferInfo storageBufferInfoCurrentFrame{};
        storageBufferInfoCurrentFrame.buffer = shaderStorageBuffers[f];
        storageBufferInfoCurrentFrame.offset = 0;
        storageBufferInfoCurrentFrame.range = sizeof(Particle) * PARTICLE_COUNT;

        descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[2].dstSet = sets[f];
        descriptorWrites[2].dstBinding = 2;
        descriptorWrites[2].dstArrayElement = 0;
        descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        descriptorWrites[2].descriptorCount = 1;
        descriptorWrites[2].pBufferInfo = &storageBufferInfoCurrentFrame;

        vkUpdateDescriptorSets(vk.m_LogicalDevice, 3, descriptorWrites.data(), 0, nullptr);
    }

    return sets;

}


VkPipeline CreateComputePipeline(VkState& vk, VkDescriptorSetLayout& layout, ShaderProgram& computeProg, VkPipelineLayout& computeLayout)
{
    VkPipelineLayoutCreateInfo computeLayoutInfo {};
    computeLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    computeLayoutInfo.setLayoutCount = 1;
    computeLayoutInfo.pSetLayouts = &layout;

    VK_CHECK(vkCreatePipelineLayout(vk.m_LogicalDevice, &computeLayoutInfo, nullptr, & computeLayout));

    VkPipelineShaderStageCreateInfo computeStageInfo {};
    computeStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    computeStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    computeStageInfo.module = computeProg.m_Stages.front().m_Module;
    computeStageInfo.pName = "main";


    VkComputePipelineCreateInfo create{};
    create.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    create.layout = computeLayout;
    create.stage = computeStageInfo;

    VkPipeline pipeline;
    VK_CHECK(vkCreateComputePipelines(vk.m_LogicalDevice, VK_NULL_HANDLE, 1, &create, nullptr, &pipeline));

    return pipeline;
}

int main()
{
    bool enableMSAA = true;
    VkState vk = init::Create<VkSDL>("Forward Lights", 1920, 1080, true);

    // shader abstraction


    ShaderProgram lights_prog = ShaderProgram::CreateGraphicsFromSourcePath(
        vk, "shaders/lights.vert", "shaders/lights.frag");

    ShaderProgram particles_prog = ShaderProgram::CreateComputeFromSourcePath(vk, "shaders/particles.comp");

    std::vector<VkBuffer> uniformBuffers;
    std::vector<VmaAllocation> uniformBuffersMemory;
    std::vector<VkBuffer> shaderStorageBuffers;
    std::vector<VmaAllocation> shaderStorageBuffersMemory;
    CreateComputeBuffers(vk, uniformBuffers, uniformBuffersMemory,
                         shaderStorageBuffers, shaderStorageBuffersMemory);

    VkDescriptorSetLayout layout = CreateComputeDescriptorLayouts(vk);
    Vector<VkDescriptorSet> computeDescriptors = CreateComputeDescriptorSets(vk, layout,  uniformBuffers, shaderStorageBuffers);
    VkPipelineLayout computePipelineLayout;
    VkPipeline computePipeline = CreateComputePipeline(vk, layout, particles_prog, computePipelineLayout);


    FillExampleLightData(lightDataCpu);
    // Texture abstraction
    uint32_t mipLevels;
    VkImage textureImage;
    VkImageView imageView;
    VkDeviceMemory textureMemory;
    textures::CreateTexture(vk, "assets/viking_room.png", VK_FORMAT_R8G8B8A8_UNORM, textureImage, imageView, textureMemory, &mipLevels);
    VkSampler imageSampler;
    textures::CreateImageSampler(vk, imageView, mipLevels, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_REPEAT, imageSampler);

    // Pipeline stage?
    VkPipelineLayout pipelineLayout;
    auto vertexDescription = VertexDataPosNormalUv::GetVertexDescription();
    VkPipeline pipeline = lvk::pipelines::CreateRasterPipeline(vk,
        lights_prog, vertexDescription,
        defaults::CullNoneRasterStateMSAA, defaults::DefaultRasterPipelineState,
        vk.m_SwapchainImageRenderPass, vk.m_SwapChainImageExtent, pipelineLayout);

    // create vertex and index buffer
    Model model;
    LoadModelAssimp(vk, model, "assets/viking_room.obj", true);

    // Shader too probably
    buffers::CreateUniformBuffers<MvpData>(vk, mvpUniformData);
    buffers::CreateUniformBuffers<FrameLightDataT<NUM_LIGHTS>>(vk, lightsUniformData);
    CreateGraphicsDescriptorSets(vk, lights_prog.m_DescriptorSetLayout,
                                 imageView, imageSampler);

    while (vk.m_ShouldRun)
    {    
        vk.m_Backend->PreFrame(vk);
        
        UpdateUniformBuffer(vk);

        RecordComputeCommandBuffers(vk, computePipeline, computePipelineLayout, computeDescriptors);
        RecordGraphicsCommandBuffers(vk, pipeline, pipelineLayout, model);

        vk.m_Backend->PostFrame(vk);
    }

    mvpUniformData.Free(vk);
    lightsUniformData.Free(vk);

    FreeModel(vk, model);
    vkDestroySampler(vk.m_LogicalDevice, imageSampler, nullptr);
    vkDestroyImageView(vk.m_LogicalDevice, imageView, nullptr);
    vkDestroyImage(vk.m_LogicalDevice, textureImage, nullptr);
    vkFreeMemory(vk.m_LogicalDevice, textureMemory, nullptr);
    vkDestroyPipelineLayout(vk.m_LogicalDevice, pipelineLayout, nullptr);
    vkDestroyPipeline(vk.m_LogicalDevice, pipeline, nullptr);

    return 0;
}
