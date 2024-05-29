#include "example-common.h"
#include <algorithm>
using namespace lvk;

#define NUM_LIGHTS 512
using DeferredLightData = FrameLightDataT<NUM_LIGHTS>;

struct RenderItem 
{
    Mesh m_Mesh;
    UniformBufferFrameData m_MvpBuffer;
    Vector<VkDescriptorSet> m_DescriptorSets;
};

struct RenderModel
{
    Vector<RenderItem> m_RenderItems;
};

struct ShaderStage
{
    StageBinary m_StageBinary;
    Vector<DescriptorSetLayoutData> m_LayoutDatas;

    static ShaderStage Create(VulkanAPI& vk, const String& stagePath)
    {
        auto stageBin = vk.LoadSpirvBinary(stagePath);
        auto stageLayoutDatas = vk.ReflectDescriptorSetLayouts(stageBin);

        return { stageBin, stageLayoutDatas };
    }
};

struct ShaderProgramVF
{
    ShaderStage m_VertexStage;
    ShaderStage m_FragmentStage;

    VkDescriptorSetLayout m_DescriptorSetLayout;

    static ShaderProgramVF Create(VulkanAPI& vk, const String& vertPath, const String& fragPath)
    {
        ShaderStage vert = ShaderStage::Create(vk, vertPath);
        ShaderStage frag = ShaderStage::Create(vk, fragPath);
        VkDescriptorSetLayout layout;
        vk.CreateDescriptorSetLayout(vert.m_LayoutDatas, frag.m_LayoutDatas, layout);

        return { vert, frag, layout };

    }
};

class MaterialVF
{
public:
    // create a material from a shader
    // material is the interface to an instance of a shader
    // contains descriptor set and associated buffers.
    // set mat4, mat3, vec4, vec3, sampler etc.
    // reflect the size of each bound thing in each set (one set for now)

    struct UniformBufferBindingData
    {
        uint32_t m_SetNumber;
        uint32_t m_BindingNumber;
        uint32_t m_BufferSize;
        UniformBufferFrameData m_UBO;
    };

    struct SamplerBindingData
    {
        uint32_t        m_SetNumber;
        uint32_t        m_BindingNumber;
        VkImageView&    m_ImageView;
        VkSampler&      m_Sampler;
    };

    struct UniformAccessorData
    {
        uint32_t    m_ExpectedSize;
        uint32_t    m_Offset;
        uint32_t    m_Stride;
        uint16_t    m_ArraySize;
        uint16_t    m_BufferIndex;
    };
     
    Vector<VkDescriptorSet>                 m_DescriptorSets;
    Vector<UniformBufferBindingData>        m_UniformBuffers;
    HashMap<String, SamplerBindingData>     m_Samplers;
    HashMap<String, UniformAccessorData>    m_UniformBufferAccessors;

    static MaterialVF Create(VulkanAPI& vk, ShaderProgramVF& shader)
    {
        MaterialVF mat{};
        Vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, shader.m_DescriptorSetLayout);
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = vk.m_DescriptorPool;
        allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
        allocInfo.pSetLayouts = layouts.data();

        mat.m_DescriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
        VK_CHECK(vkAllocateDescriptorSets(vk.m_LogicalDevice, &allocInfo, mat.m_DescriptorSets.data()));


        static auto collect_uniform_data = [&mat, &vk](ShaderStage& stage)
        {
            for (auto& descriptorSetInfo : stage.m_LayoutDatas)
            {
                // ..and each binding
                for (auto& bindingInfo : descriptorSetInfo.m_BindingDatas)
                {
                    // if a sampler ignore for now
                    if (bindingInfo.m_ExpectedBlockSize == 0)
                    {
                        SamplerBindingData sbd{
                            descriptorSetInfo.m_SetNumber,
                            bindingInfo.m_BindingIndex,
                            Texture::g_DefaultTexture->m_ImageView,
                            Texture::g_DefaultTexture->m_Sampler
                        };
                        mat.m_Samplers.emplace(bindingInfo.m_BindingName, sbd);
                        continue;
                    }
                    // if a uniform buffer
                    UniformBufferFrameData uniform;
                    vk.CreateUniformBuffers(uniform, VkDeviceSize{ bindingInfo.m_ExpectedBlockSize });
                    // build accessors
                    for (auto& member : bindingInfo.m_Members)
                    {
                        String accessorName = bindingInfo.m_BindingName + "." + member.m_Name;
                        uint16_t arraySize = member.m_Stride > 0 ? (member.m_Size / member.m_Stride) : 0;
                        UniformAccessorData data{ member.m_Size , member.m_Offset, member.m_Stride, arraySize, static_cast<uint32_t>(mat.m_UniformBuffers.size()) };
                        mat.m_UniformBufferAccessors.emplace(accessorName, data);
                    }
                    mat.m_UniformBuffers.push_back({descriptorSetInfo.m_SetNumber, bindingInfo.m_BindingIndex, bindingInfo.m_ExpectedBlockSize,  uniform });
                }
            }
        };
        
        collect_uniform_data(shader.m_VertexStage);
        collect_uniform_data(shader.m_FragmentStage);

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            // write buffers to descriptor set + default texture for any samplers
            Vector<VkDescriptorBufferInfo>  bufferWriteInfos;
            for (auto& bufferInfo : mat.m_UniformBuffers)
            {
                VkDescriptorBufferInfo bufferWriteInfo{};
                bufferWriteInfo.buffer = bufferInfo.m_UBO.m_UniformBuffers[0];
                bufferWriteInfo.offset = 0;
                bufferWriteInfo.range = bufferInfo.m_BufferSize;
                bufferWriteInfos.push_back(bufferWriteInfo);
            }
            Vector<VkDescriptorImageInfo>   imageWriteInfos;
            Vector<uint32_t> bindings;
            for (auto& [name, sampler] : mat.m_Samplers)
            {
                VkDescriptorImageInfo imageInfo{};
                imageInfo.imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
                imageInfo.imageView = sampler.m_ImageView;
                imageInfo.sampler = sampler.m_Sampler;
                imageWriteInfos.push_back(imageInfo);
                bindings.push_back(sampler.m_BindingNumber);
            }

            Vector<VkWriteDescriptorSet> descriptorWrites{};
            for (int j = 0; j < mat.m_UniformBuffers.size(); j++)
            {
                VkWriteDescriptorSet write{};
                write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                write.dstSet = mat.m_DescriptorSets[i];
                write.dstBinding = mat.m_UniformBuffers[j].m_BindingNumber;
                write.dstArrayElement = 0; // todo
                write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                write.descriptorCount = 1;
                write.pBufferInfo = &bufferWriteInfos[j];
            }
            for (int j = 0; j < imageWriteInfos.size(); j++)
            {
                VkWriteDescriptorSet write{};
                write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                write.dstSet = mat.m_DescriptorSets[i];
                write.dstBinding = 1;
                write.dstArrayElement = 0;
                write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                write.descriptorCount = 1;
                write.pImageInfo = &imageWriteInfos[j];
            }

            vkUpdateDescriptorSets(vk.m_LogicalDevice, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);

        }
        return mat;
    }

    template<typename _Ty>
    bool SetMember(const String& name, const _Ty& value)
    {
        static constexpr size_t _type_size = sizeof(_Ty);
        if (m_UniformBufferAccessors.find(name) == m_UniformBufferAccessors.end())
        {
            return false;
        }

        UniformAccessorData& data = m_UniformBufferAccessors.at(name);

        if (_type_size != data.m_ExpectedSize)
        {
            return false;
        }

        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            // update uniform buffer
            m_UniformBuffers[data.m_BufferIndex].m_UBO.Set(i, value, data.m_Offset);
        }
        
        return true;
    }
};

static Vector<VertexData> g_ScreenSpaceQuadVertexData = {
    { { -1.0f, -1.0f , 0.0f}, { 1.0f, 0.0f } },
    { {1.0f, -1.0f, 0.0f}, {0.0f, 0.0f} },
    { {1.0f, 1.0f, 0.0f}, {0.0f, 1.0f} },
    { {-1.0f, 1.0f, 0.0f}, {1.0f, 1.0f} }
};

static Vector<uint32_t> g_ScreenSpaceQuadIndexData = {
    0, 1, 2, 2, 3, 0
};

static Transform g_Transform; 

void RecordCommandBuffersV2(VulkanAPI_SDL& vk,
    VkPipeline& gbufferPipeline , VkPipelineLayout& gbufferPipelineLayout, VkRenderPass gbufferRenderPass, Vector<VkFramebuffer>& gbufferFramebuffers,
    VkPipeline& lightingPassPipeline, VkPipelineLayout& lightingPassPipelineLayout, VkRenderPass lightingPassRenderPass, Vector<VkDescriptorSet>& lightingPassDescriptorSets, Vector<VkFramebuffer>& lightingPassFramebuffers,
    RenderModel& model, Mesh& screenQuad)
{
    vk.RecordGraphicsCommands([&](VkCommandBuffer& commandBuffer, uint32_t frameIndex) {
        // push to example
        {
            Array<VkClearValue, 4> clearValues{};
            clearValues[0].color = { {0.0f, 0.0f, 0.0f, 1.0f} };
            clearValues[1].color = { {0.0f, 0.0f, 0.0f, 1.0f} };
            clearValues[2].color = { {0.0f, 0.0f, 0.0f, 1.0f} };
            clearValues[3].depthStencil = { 1.0f, 0 };

            VkRenderPassBeginInfo renderPassInfo{};
            renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            renderPassInfo.renderPass = gbufferRenderPass;
            renderPassInfo.framebuffer = gbufferFramebuffers[frameIndex];
            renderPassInfo.renderArea.offset = { 0,0 };
            renderPassInfo.renderArea.extent = vk.m_SwapChainImageExtent;

            renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
            renderPassInfo.pClearValues = clearValues.data();

            vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, gbufferPipeline);
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

            for (int i = 0; i < model.m_RenderItems.size(); i++)
            {
                Mesh& mesh = model.m_RenderItems[i].m_Mesh;
                VkBuffer vertexBuffers[]{ mesh.m_VertexBuffer };
                VkDeviceSize sizes[] = { 0 };


                vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, sizes);
                vkCmdBindIndexBuffer(commandBuffer, mesh.m_IndexBuffer, 0, VK_INDEX_TYPE_UINT32);
                vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, gbufferPipelineLayout, 0, 1, &model.m_RenderItems[i].m_DescriptorSets[frameIndex], 0, nullptr);
                vkCmdDrawIndexed(commandBuffer, mesh.m_IndexCount, 1, 0, 0, 0);
            }
            vkCmdEndRenderPass(commandBuffer);
        }

        // push to example
        Array<VkClearValue, 2> clearValues{};
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
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, lightingPassPipeline);
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
        VkDeviceSize sizes[] = { 0 };
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, &screenQuad.m_VertexBuffer, sizes);
        vkCmdBindIndexBuffer(commandBuffer, screenQuad.m_IndexBuffer, 0, VK_INDEX_TYPE_UINT32);
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, lightingPassPipelineLayout, 0, 1, &lightingPassDescriptorSets[frameIndex], 0, nullptr);
        vkCmdDrawIndexed(commandBuffer, screenQuad.m_IndexCount, 1, 0, 0, 0);
        vkCmdEndRenderPass(commandBuffer);
        });
}

void UpdateUniformBuffer(VulkanAPI_SDL& vk, UniformBufferFrameData& mvpUniformData, UniformBufferFrameData& lightsUniformData, DeferredLightData& lightDataCpu)
{
    static auto startTime = std::chrono::high_resolution_clock::now();

    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

    MvpData ubo{};
    ubo.Model = g_Transform.to_mat4();

    ubo.View = glm::lookAt(glm::vec3(20.0f, 20.0f, 20.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    if (vk.m_SwapChainImageExtent.width > 0 || vk.m_SwapChainImageExtent.height)
    {
        ubo.Proj = glm::perspective(glm::radians(45.0f), vk.m_SwapChainImageExtent.width / (float)vk.m_SwapChainImageExtent.height, 0.1f, 1000.0f);
        ubo.Proj[1][1] *= -1;
    }
    mvpUniformData.Set(vk.GetFrameIndex(), ubo);

    lightsUniformData.Set(vk.GetFrameIndex(), lightDataCpu);
}

void CreateGBufferDescriptorSets(VulkanAPI& vk, ShaderProgramVF& shader, VkImageView& textureImageView, VkSampler& textureSampler, Vector<VkDescriptorSet>& descriptorSets, UniformBufferFrameData& mvpUniformData)
{
    Vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, shader.m_DescriptorSetLayout);
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = vk.m_DescriptorPool;
    allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
    allocInfo.pSetLayouts = layouts.data();

    descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
    VK_CHECK(vkAllocateDescriptorSets(vk.m_LogicalDevice, &allocInfo, descriptorSets.data()));

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        VkDescriptorBufferInfo mvpBufferInfo{};
        mvpBufferInfo.buffer = mvpUniformData.m_UniformBuffers[i];
        mvpBufferInfo.offset = 0;
        mvpBufferInfo.range = sizeof(MvpData);

        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = textureImageView;
        imageInfo.sampler = textureSampler;

        Array<VkWriteDescriptorSet, 2> descriptorWrites{};

        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstSet = descriptorSets[i];
        descriptorWrites[0].dstBinding = 0;
        descriptorWrites[0].dstArrayElement = 0;
        descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].pBufferInfo = &mvpBufferInfo;

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

void CreateLightingPassDescriptorSets(VulkanAPI_SDL& vk, VkDescriptorSetLayout& descriptorSetLayout, FramebufferSet gbuffers, Vector<VkDescriptorSet>& descriptorSets, UniformBufferFrameData& mvpUniformData, UniformBufferFrameData& lightsUniformData)
{
    Vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, descriptorSetLayout);
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = vk.m_DescriptorPool;
    allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
    allocInfo.pSetLayouts = layouts.data();

    descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
    VK_CHECK(vkAllocateDescriptorSets(vk.m_LogicalDevice, &allocInfo, descriptorSets.data()));

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        VkDescriptorBufferInfo mvpBufferInfo{};
        mvpBufferInfo.buffer = mvpUniformData.m_UniformBuffers[i];
        mvpBufferInfo.offset = 0;
        mvpBufferInfo.range = sizeof(MvpData);

        VkDescriptorImageInfo positionBufferInfo{};
        positionBufferInfo.imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
        positionBufferInfo.imageView = gbuffers.m_Framebuffers[i].m_ColourAttachments[1].m_ImageView;
        positionBufferInfo.sampler = gbuffers.m_Framebuffers[i].m_ColourAttachments[1].m_Sampler;

        VkDescriptorImageInfo normalBufferInfo{};
        normalBufferInfo.imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
        normalBufferInfo.imageView = gbuffers.m_Framebuffers[i].m_ColourAttachments[2].m_ImageView;
        normalBufferInfo.sampler = gbuffers.m_Framebuffers[i].m_ColourAttachments[2].m_Sampler;

        VkDescriptorImageInfo colourBufferInfo{};
        colourBufferInfo.imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
        colourBufferInfo.imageView = gbuffers.m_Framebuffers[i].m_ColourAttachments[0].m_ImageView;
        colourBufferInfo.sampler = gbuffers.m_Framebuffers[i].m_ColourAttachments[0].m_Sampler;

        VkDescriptorBufferInfo lightBufferInfo{};
        lightBufferInfo.buffer = lightsUniformData.m_UniformBuffers[i];
        lightBufferInfo.offset = 0;
        lightBufferInfo.range = sizeof(DeferredLightData);

        Array<VkWriteDescriptorSet, 5> descriptorWrites{};

        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstSet = descriptorSets[i];
        descriptorWrites[0].dstBinding = 0;
        descriptorWrites[0].dstArrayElement = 0;
        descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].pBufferInfo = &mvpBufferInfo;

        descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[1].dstSet = descriptorSets[i];
        descriptorWrites[1].dstBinding = 1;
        descriptorWrites[1].dstArrayElement = 0;
        descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrites[1].descriptorCount = 1;
        descriptorWrites[1].pImageInfo = &positionBufferInfo;

        descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[2].dstSet = descriptorSets[i];
        descriptorWrites[2].dstBinding = 2;
        descriptorWrites[2].dstArrayElement = 0;
        descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrites[2].descriptorCount = 1;
        descriptorWrites[2].pImageInfo = &normalBufferInfo;

        descriptorWrites[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[3].dstSet = descriptorSets[i];
        descriptorWrites[3].dstBinding = 3;
        descriptorWrites[3].dstArrayElement = 0;
        descriptorWrites[3].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrites[3].descriptorCount = 1;
        descriptorWrites[3].pImageInfo = &colourBufferInfo;

        descriptorWrites[4].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[4].dstSet = descriptorSets[i];
        descriptorWrites[4].dstBinding = 4;
        descriptorWrites[4].dstArrayElement = 0;
        descriptorWrites[4].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[4].descriptorCount = 1;
        descriptorWrites[4].pBufferInfo = &lightBufferInfo;


        vkUpdateDescriptorSets(vk.m_LogicalDevice, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
    }

}

RenderModel CreateRenderModelGbuffer(VulkanAPI& vk, const String& modelPath, ShaderProgramVF& shader)
{
    Model model;
    LoadModelAssimp(vk, model, modelPath, true);

    RenderModel renderModel{};
    for (auto& mesh : model.m_Meshes)
    {
        RenderItem item{};
        int materialIndex = std::min(mesh.m_MaterialIndex, (uint32_t)model.m_Materials.size() - 1);
        item.m_Mesh = mesh;
        item.m_DescriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
        vk.CreateUniformBuffers<MvpData>(item.m_MvpBuffer);
        CreateGBufferDescriptorSets(vk, shader, model.m_Materials[materialIndex].m_Diffuse.m_ImageView, model.m_Materials[materialIndex].m_Diffuse.m_Sampler, item.m_DescriptorSets, item.m_MvpBuffer);
        renderModel.m_RenderItems.push_back(item);
    }

    return renderModel;
}

void OnImGui(VulkanAPI& vk, DeferredLightData& lightDataCpu)
{
    if (ImGui::Begin("Debug"))
    {
        ImGui::Text("Frametime: %f", (1.0 / vk.m_DeltaTime));
        ImGui::Separator();
        ImGui::DragFloat3("Position", &g_Transform.m_Position[0]);
        ImGui::DragFloat3("Euler Rotation", & g_Transform.m_Rotation[0]);
        ImGui::DragFloat3("Scale", & g_Transform.m_Scale[0]);
    }
    ImGui::End();

    if (ImGui::Begin("Menu"))
    {
        ImGui::Text("Frametime: %f", (1.0 / vk.m_DeltaTime));
        ImGui::DragFloat3("Directional Light Dir", &lightDataCpu.m_DirectionalLight.Direction[0]);
        ImGui::DragFloat4("Directional Light Colour", &lightDataCpu.m_DirectionalLight.Colour[0]);
        ImGui::DragFloat4("Directional Light Ambient Colour", &lightDataCpu.m_DirectionalLight.Ambient[0]);

        if (ImGui::TreeNode("Point Lights"))
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
}

int main()
{
    VulkanAPI_SDL vk;
    bool enableMSAA = false;
    vk.Start(1280, 720, enableMSAA);


    DeferredLightData lightDataCpu{};
    FillExampleLightData(lightDataCpu);


    ShaderProgramVF gbufferProg     = ShaderProgramVF::Create(vk, "shaders/gbuffer.vert.spv", "shaders/gbuffer.frag.spv");
    MaterialVF gbufferMat = MaterialVF::Create(vk, gbufferProg);
    ShaderProgramVF lightPassProg   = ShaderProgramVF::Create(vk, "shaders/lights.vert.spv", "shaders/lights.frag.spv");
    MaterialVF lightPassMat = MaterialVF::Create(vk, lightPassProg);

    bool succeeded = gbufferMat.SetMember("ubo.model", glm::mat4(1.0));
    succeeded = gbufferMat.SetMember("ubo.proj", glm::mat4(1.0));

    FramebufferSet gbufferSet{};
    gbufferSet.m_Width = vk.m_SwapChainImageExtent.width;
    gbufferSet.m_Height = vk.m_SwapChainImageExtent.height;
    gbufferSet.AddColourAttachment(vk, ResolutionScale::Full, 1, VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_ASPECT_COLOR_BIT);
    gbufferSet.AddColourAttachment(vk, ResolutionScale::Full, 1, VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_ASPECT_COLOR_BIT);
    gbufferSet.AddColourAttachment(vk, ResolutionScale::Full, 1, VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_ASPECT_COLOR_BIT);
    gbufferSet.AddDepthAttachment(vk, ResolutionScale::Full, 1, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_ASPECT_DEPTH_BIT);
    gbufferSet.Build(vk);

    // create gbuffer pipeline
    VkPipelineLayout gbufferPipelineLayout;
    VkPipeline gbufferPipeline = vk.CreateRasterizationGraphicsPipeline(
        gbufferProg.m_VertexStage.m_StageBinary, gbufferProg.m_FragmentStage.m_StageBinary,
        gbufferProg.m_DescriptorSetLayout, Vector<VkVertexInputBindingDescription>{VertexDataNormal::GetBindingDescription() }, VertexDataNormal::GetAttributeDescriptions(),
        gbufferSet.m_RenderPass,
        vk.m_SwapChainImageExtent.width, vk.m_SwapChainImageExtent.height,
        VK_POLYGON_MODE_FILL,
        VK_CULL_MODE_NONE,
        false,
        VK_COMPARE_OP_LESS,
        gbufferPipelineLayout, 3
    );

    // create present graphics pipeline
    // Pipeline stage?
    VkPipelineLayout lightPassPipelineLayout;
    VkPipeline pipeline = vk.CreateRasterizationGraphicsPipeline(
        lightPassProg.m_VertexStage.m_StageBinary, lightPassProg.m_FragmentStage.m_StageBinary,
        lightPassProg.m_DescriptorSetLayout, Vector<VkVertexInputBindingDescription>{VertexData::GetBindingDescription() }, VertexData::GetAttributeDescriptions(),
        vk.m_SwapchainImageRenderPass,
        vk.m_SwapChainImageExtent.width, vk.m_SwapChainImageExtent.height,
        VK_POLYGON_MODE_FILL,
        VK_CULL_MODE_NONE,
        enableMSAA,
        VK_COMPARE_OP_LESS,
        lightPassPipelineLayout);

    // create vertex and index buffer
    RenderModel m   = CreateRenderModelGbuffer(vk, "assets/sponza/sponza.gltf", gbufferProg);
    Mesh screenQuad = BuildScreenSpaceQuad(vk, g_ScreenSpaceQuadVertexData, g_ScreenSpaceQuadIndexData);


    UniformBufferFrameData mvpUniformData;
    UniformBufferFrameData lightsUniformData;
    vk.CreateUniformBuffers<FrameLightDataT<NUM_LIGHTS>>(lightsUniformData);
    vk.CreateUniformBuffers<MvpData>(mvpUniformData);

    Vector<VkDescriptorSet>     lightPassDescriptorSets;
    CreateLightingPassDescriptorSets(vk, lightPassProg.m_DescriptorSetLayout, gbufferSet, lightPassDescriptorSets, mvpUniformData, lightsUniformData);

    Vector<VkFramebuffer> gbufferFramebuffers{ gbufferSet.m_Framebuffers[0].m_FB, gbufferSet.m_Framebuffers[1].m_FB };

    while (vk.ShouldRun())
    {
        vk.PreFrame();

        for (auto& item : m.m_RenderItems)
        {
            UpdateUniformBuffer(vk, item.m_MvpBuffer, lightsUniformData, lightDataCpu);
        }

        RecordCommandBuffersV2(vk, 
            gbufferPipeline, gbufferPipelineLayout, gbufferSet.m_RenderPass, gbufferFramebuffers,
            pipeline, lightPassPipelineLayout, vk.m_SwapchainImageRenderPass, lightPassDescriptorSets, vk.m_SwapChainFramebuffers,
            m, screenQuad);

        OnImGui(vk, lightDataCpu);

        vk.PostFrame();
    }

    mvpUniformData.Free(vk);
    lightsUniformData.Free(vk);

    FreeMesh(vk, screenQuad);

    gbufferSet.Free(vk);

    vkDestroyRenderPass(vk.m_LogicalDevice, gbufferSet.m_RenderPass, nullptr);

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        vkFreeDescriptorSets(vk.m_LogicalDevice, vk.m_DescriptorPool, 1,  &lightPassDescriptorSets[i]);
    }
    vkDestroyDescriptorSetLayout(vk.m_LogicalDevice, gbufferProg.m_DescriptorSetLayout, nullptr);
    vkDestroyDescriptorSetLayout(vk.m_LogicalDevice, lightPassProg.m_DescriptorSetLayout, nullptr);
    
    vkDestroyPipelineLayout(vk.m_LogicalDevice, gbufferPipelineLayout, nullptr);
    vkDestroyPipeline(vk.m_LogicalDevice, gbufferPipeline, nullptr);
    vkDestroyPipelineLayout(vk.m_LogicalDevice, lightPassPipelineLayout, nullptr);
    vkDestroyPipeline(vk.m_LogicalDevice, pipeline, nullptr);

    return 0;
}
