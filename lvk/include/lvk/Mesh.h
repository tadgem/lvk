#pragma once
#include "lvk/VulkanAPI.h"
#include "glm/glm.hpp"

namespace lvk
{
    struct VertexDataPosColUv
    {
        glm::vec3 Position;
        glm::vec3 Colour;
        glm::vec2 UV;

        static VkVertexInputBindingDescription GetBindingDescription();

        static std::vector<VkVertexInputAttributeDescription> GetAttributeDescriptions() {
            std::vector<VkVertexInputAttributeDescription> attributeDescriptions{};

            attributeDescriptions.resize(3);

            attributeDescriptions[0].binding = 0;
            attributeDescriptions[0].location = 0;
            attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
            attributeDescriptions[0].offset = offsetof(VertexDataPosColUv, Position);

            attributeDescriptions[1].binding = 0;
            attributeDescriptions[1].location = 1;
            attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
            attributeDescriptions[1].offset = offsetof(VertexDataPosColUv, Colour);

            attributeDescriptions[2].binding = 0;
            attributeDescriptions[2].location = 2;
            attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
            attributeDescriptions[2].offset = offsetof(VertexDataPosColUv, UV);

            return attributeDescriptions;
        }
    };

    struct VertexDataPosUv
    {
        glm::vec3 Position;
        glm::vec2 UV;

        static VkVertexInputBindingDescription GetBindingDescription() {
            VkVertexInputBindingDescription bindingDescription{};

            bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
            bindingDescription.stride = sizeof(VertexDataPosUv);
            bindingDescription.binding = 0;

            return bindingDescription;
        }

        static std::vector<VkVertexInputAttributeDescription> GetAttributeDescriptions() {
            std::vector<VkVertexInputAttributeDescription> attributeDescriptions{};

            attributeDescriptions.resize(2);

            attributeDescriptions[0].binding = 0;
            attributeDescriptions[0].location = 0;
            attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
            attributeDescriptions[0].offset = offsetof(VertexDataPosUv, Position);

            attributeDescriptions[1].binding = 0;
            attributeDescriptions[1].location = 1;
            attributeDescriptions[1].format = VK_FORMAT_R32G32_SFLOAT;
            attributeDescriptions[1].offset = offsetof(VertexDataPosUv, UV);

            return attributeDescriptions;
        }
    };

    struct VertexDataPosNormalUv
    {
        glm::vec3 Position;
        glm::vec3 Normal;
        glm::vec2 UV;

        static VkVertexInputBindingDescription GetBindingDescription() {
            VkVertexInputBindingDescription bindingDescription{};

            bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
            bindingDescription.stride = sizeof(VertexDataPosNormalUv);
            bindingDescription.binding = 0;

            return bindingDescription;
        }

        static std::vector<VkVertexInputAttributeDescription> GetAttributeDescriptions() {
            std::vector<VkVertexInputAttributeDescription> attributeDescriptions{};

            attributeDescriptions.resize(3);

            attributeDescriptions[0].binding = 0;
            attributeDescriptions[0].location = 0;
            attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
            attributeDescriptions[0].offset = offsetof(VertexDataPosNormalUv, Position);

            attributeDescriptions[1].binding = 0;
            attributeDescriptions[1].location = 1;
            attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
            attributeDescriptions[1].offset = offsetof(VertexDataPosNormalUv, Normal);

            attributeDescriptions[2].binding = 0;
            attributeDescriptions[2].location = 2;
            attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
            attributeDescriptions[2].offset = offsetof(VertexDataPosNormalUv, UV);

            return attributeDescriptions;
        }
    };

    class Mesh
    {
    public:
        VkBuffer m_VertexBuffer;
        VmaAllocation m_VertexBufferMemory;
        VkBuffer m_IndexBuffer;
        VmaAllocation m_IndexBufferMemory;

        uint32_t m_IndexCount;

        static Mesh* g_ScreenSpaceQuad;

        static void InitScreenQuad(lvk::VulkanAPI& vk);
        static void FreeScreenQuad(lvk::VulkanAPI& vk);
    };
}