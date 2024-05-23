#pragma once
#include "Alias.h"
#include <vulkan/vulkan.h>
#include "spirv_reflect.h"
#define VK_CHECK(X) {int _lineNumber = __LINE__; const char* _filePath = __FILE__;\
if(X != VK_SUCCESS){\
spdlog::error("VK check failed at {} Line {} : {}",_filePath, _lineNumber, #X);}}
#include "VulkanMemoryAllocator.h"
#include "stb_image.h"
#include "ImGui/imgui.h"

namespace lvk
{
    class VulkanAPI;

    static constexpr int        MAX_FRAMES_IN_FLIGHT = 2;
    
    using StageBinary = std::vector<char>;

    class VulkanAPIWindowHandle {};

    struct DescriptorSetLayoutBindingData
    {
        uint32_t                     m_ExpectedBlockSize;
    };

    struct DescriptorSetLayoutData 
    {
        uint32_t                                    m_SetNumber;
        VkDescriptorSetLayoutCreateInfo             m_CreateInfo;
        VkDescriptorSetLayout                       m_Layout;
        Vector<VkDescriptorSetLayoutBinding>        m_Bindings;
        Vector<DescriptorSetLayoutBindingData>      m_BindingDatas;
    };

    struct ShaderModule
    {
        Vector<DescriptorSetLayoutData>     m_DescriptorSetLayoutData;
        StageBinary                         m_Binary;
    };

    template<typename T>
    struct UniformBufferFrameData
    {
        Vector<VkBuffer>            m_UniformBuffers;
        Vector<VmaAllocation>       m_UniformBuffersMemory;
        Vector<void*>               m_UniformBuffersMapped;

        void Set(uint32_t frameIndex, T& data)
        {
            memcpy(m_UniformBuffersMapped[frameIndex], &data, sizeof(T));
        }

        void Free(VulkanAPI& vk)
        {
            for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
                vmaUnmapMemory(vk.m_Allocator, m_UniformBuffersMemory[i]);
                vkDestroyBuffer(vk.m_LogicalDevice, m_UniformBuffers[i], nullptr);
                vmaFreeMemory(vk.m_Allocator, m_UniformBuffersMemory[i]);
            }
        }
    };

    class VulkanAPI
    {
    protected:
        const bool                  p_UseValidation = true;
        const bool                  p_UseImGui      = true;

        
        const Vector<const char*>   p_ValidationLayers = {
            "VK_LAYER_KHRONOS_validation"
        };
        const Vector<const char*>   p_DeviceExtensions = {
            "VK_KHR_swapchain"
        };
    public:
        bool    m_UseSwapchainMsaa = false;


        enum class ShaderStage
        {
            Vertex,
            Fragment
        };

        enum class QueueFamilyType
        {
            Graphics = VK_QUEUE_GRAPHICS_BIT,
            Compute = VK_QUEUE_COMPUTE_BIT,
            Transfer = VK_QUEUE_TRANSFER_BIT,
            Present

        };

        struct QueueFamilyIndices
        {
            HashMap<QueueFamilyType, uint32_t> m_QueueFamilies;

            bool IsComplete();

        };

        struct SwapChainSupportDetais
        {
            VkSurfaceCapabilitiesKHR   m_Capabilities;
            Vector<VkSurfaceFormatKHR> m_SupportedFormats;
            Vector<VkPresentModeKHR>   m_SupportedPresentModes;
        };

        VkInstance                      m_Instance;
        VkSurfaceKHR                    m_Surface;
        VkSwapchainKHR                  m_SwapChain;
        VkDebugUtilsMessengerEXT        m_DebugMessenger;
        VkPhysicalDevice                m_PhysicalDevice = VK_NULL_HANDLE;
        VkDevice                        m_LogicalDevice = VK_NULL_HANDLE;
        VkRenderPass                    m_SwapchainImageRenderPass;
        VkRenderPass                    m_ImGuiRenderPass;
        VkCommandPool                   m_GraphicsQueueCommandPool;
        VkDescriptorPool                m_DescriptorPool;
        VmaAllocator                    m_Allocator;

        Vector<VkSemaphore>             m_ImageAvailableSemaphores;
        Vector<VkSemaphore>             m_RenderFinishedSemaphores;
        Vector<VkFence>                 m_FrameInFlightFences;
        Vector<VkFence>                 m_ImagesInFlight;
        QueueFamilyIndices              m_QueueFamilyIndices;

        VkQueue                         m_GraphicsQueue = VK_NULL_HANDLE;
        VkQueue                         m_PresentQueue = VK_NULL_HANDLE;

        VulkanAPIWindowHandle*          m_WindowHandle;

        Vector<VkImage>                 m_SwapChainImages;
        Vector<VkImageView>             m_SwapChainImageViews;
        Vector<VkFramebuffer>           m_SwapChainFramebuffers;
        Vector<VkCommandBuffer>         m_CommandBuffers;

        VkFormat                        m_SwapChainImageFormat;
        VkExtent2D                      m_SwapChainImageExtent;
        
        VkImage                         m_SwapChainColourImage;
        VkDeviceMemory                  m_SwapChainColourImageMemory;
        VkImageView                     m_SwapChainColourImageView;
        
        VkImage                         m_SwapChainDepthImage;
        VkDeviceMemory                  m_SwapChainDepthImageMemory;
        VkImageView                     m_SwapChainDepthImageView;

        double                          m_DeltaTime;
        VkSampleCountFlagBits           m_MaxMsaaSamples;
        bool                            m_EnableSwapchainMsaa = false ;

    protected:
        // Debug
        bool                                CheckValidationLayerSupport();
        bool                                CheckDeviceExtensionSupport(VkPhysicalDevice device);
        void                                SetupDebugOutput();
        void                                CleanupDebugOutput();
        void                                ListDeviceExtensions(VkPhysicalDevice physicalDevice);
        void                                PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);

        void                                InitVulkan(bool enableSwapchainMsaa = false);
        void                                InitImGui();
        void                                RenderImGui();
        VkApplicationInfo                   CreateAppInfo();
        void                                CreateInstance();
        void                                Cleanup();
        void                                CleanupVulkan();
        QueueFamilyIndices                  FindQueueFamilies(VkPhysicalDevice physicalDevice);
        SwapChainSupportDetais              GetSwapChainSupportDetails(VkPhysicalDevice physicalDevice);
        bool                                IsDeviceSuitable(VkPhysicalDevice physicalDevice);
        uint32_t                            AssessDeviceSuitability(VkPhysicalDevice physicalDevice);
        void                                PickPhysicalDevice();
        void                                CreateLogicalDevice();
        void                                GetQueueHandles();
        VkSurfaceFormatKHR                  ChooseSwapChainSurfaceFormat(Vector<VkSurfaceFormatKHR> availableFormats);
        VkPresentModeKHR                    ChooseSwapChainPresentMode(Vector<VkPresentModeKHR> availableModes);
        void                                CreateSwapChain();
        void                                CreateSwapChainFramebuffers();
        void                                CreateSwapChainImageViews();
        void                                CleanupSwapChain();
        void                                RecreateSwapChain();
        void                                CreateSwapChainColourTexture(bool enableMsaa = false);
        void                                CreateSwapChainDepthTexture(bool enableMsaa = false);
        VkExtent2D                          ChooseSwapExtent(VkSurfaceCapabilitiesKHR& surfaceCapabilities);
        void                                CreateCommandPool();
        void                                CreateDescriptorPool();
        void                                CreateSemaphores();
        void                                CreateFences();
        void                                DrawFrame();
        void                                CreateCommandBuffers();
        void                                ClearCommandBuffers();
        void                                CreateVmaAllocator();
        void                                GetMaxUsableSampleCount();
public:
        void                                Start(uint32_t width, uint32_t height, bool enableSwapchainMsaa = false);
        void                                Quit();
        inline int                          GetFrameIndex() { return p_CurrentFrameIndex; }

        // API Begin
        uint32_t                            FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
        VkFormat                            FindSupportedFormat(const Vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
        VkFormat                            FindDepthFormat();
        bool                                HasStencilComponent(VkFormat& format);
        void                                CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& allocation);
        void                                CreateBufferVMA(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VmaAllocation& allocation);
        void                                CreateImage(uint32_t width, uint32_t height, uint32_t numMips, VkSampleCountFlagBits sampleCount, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);
        void                                CreateImageView(VkImage& image, VkFormat format, uint32_t numMips, VkImageAspectFlags aspectFlags, VkImageView& imageView);
        void                                CreateImageSampler(VkImageView& imageView, uint32_t numMips, VkFilter filterMode, VkSamplerAddressMode addressMode, VkSampler& sampler);
        void                                CreateFramebuffer(Vector<VkImageView>& attachments, VkRenderPass renderPass, VkExtent2D extent, VkFramebuffer& framebuffer);
        void                                CreateTexture(const String& path, VkFormat format, VkImage& image, VkImageView& imageView, VkDeviceMemory& imageMemory, uint32_t* numMips = nullptr);
        void                                CopyBuffer(VkBuffer& src, VkBuffer& dst, VkDeviceSize size);
        void                                CopyBufferToImage(VkBuffer& src, VkImage& image,  uint32_t width, uint32_t height);
        VkCommandBuffer                     BeginSingleTimeCommands();
        void                                EndSingleTimeCommands(VkCommandBuffer& commandBuffer);
        void                                RecordGraphicsCommands(std::function<void(VkCommandBuffer&, uint32_t)> graphicsCommandsCallback);
        void                                TransitionImageLayout(VkImage image, VkFormat format, uint32_t numMips, VkImageLayout oldLayout, VkImageLayout newLayout);
        void                                GenerateMips(VkImage image, VkFormat format, uint32_t imageWidth, uint32_t imageHeight, uint32_t numMips, VkFilter filterMethod);
        void                                CreateIndexBuffer(Vector<uint32_t> indices, VkBuffer& buffer, VmaAllocation& deviceMemory);
        template<typename _Ty>
        void                                CreateVertexBuffer(Vector<_Ty> verts, VkBuffer& buffer, VmaAllocation& deviceMemory)
        {
            VkDeviceSize bufferSize = sizeof(_Ty) * verts.size();

            // create a CPU side buffer to dump vertex data into
            VkBuffer stagingBuffer;
            VmaAllocation stagingBufferMemory;
            CreateBufferVMA(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

            // dump vert data
            void* data;
            vmaMapMemory(m_Allocator, stagingBufferMemory, &data);
            memcpy(data, verts.data(), bufferSize);
            vmaUnmapMemory(m_Allocator, stagingBufferMemory);

            // create GPU side buffer
            CreateBufferVMA(bufferSize,
                VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                buffer, deviceMemory);

            CopyBuffer(stagingBuffer, buffer, bufferSize);

            vkDestroyBuffer(m_LogicalDevice, stagingBuffer, nullptr);
            vmaFreeMemory(m_Allocator, stagingBufferMemory);
        }
        void                                CreateDescriptorSetLayout(Vector<DescriptorSetLayoutData>& vertLayoutDatas, Vector<DescriptorSetLayoutData>& fragLayoutDatas, VkDescriptorSetLayout& descriptorSetLayout);
        void                                CreateRenderPass(VkRenderPass& renderPass, Vector<VkAttachmentDescription>& colourAttachments, Vector<VkAttachmentDescription>& resolveAttachments, bool hasDepthAttachment = true, VkAttachmentDescription depthAttachment = {}, VkAttachmentLoadOp attachmentLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR);
        void                                CreateBuiltInRenderPasses();
        VkPipeline                          CreateRasterizationGraphicsPipeline(
                                            StageBinary& vert, 
                                            StageBinary& frag, 
                                            VkDescriptorSetLayout& descriptorSetLayout, 
                                            Vector<VkVertexInputBindingDescription>& vertexBindingDescriptions,
                                            Vector<VkVertexInputAttributeDescription>& vertexAttributeDescriptions,
                                            VkRenderPass& pipelineRenderPass,
                                            uint32_t width, uint32_t height,
                                            VkPolygonMode polyMode,
                                            VkCullModeFlags cullMode,
                                            bool enableMultisampling,
                                            VkCompareOp depthCompareOp,
                                            VkPipelineLayout& pipelineLayout,
                                            uint32_t colorAttachmentCount = 1);

        Vector<VkExtensionProperties>       GetDeviceAvailableExtensions(VkPhysicalDevice physicalDevice);
        StageBinary                         LoadSpirvBinary(const String& path);
        ShaderModule                        LoadShaderModule(const String& path);
        VkShaderModule                      CreateShaderModule(const StageBinary& data);
        Vector<DescriptorSetLayoutData>     ReflectDescriptorSetLayouts(StageBinary& stageBin);
        VkDescriptorSet                     CreateDescriptorSet(DescriptorSetLayoutData& layoutData);

        template<typename _Ty>
        void                                CreateUniformBuffers(Vector<VkBuffer>& uniformBuffersFrames, Vector<VmaAllocation>& uniformBuffersMemoryFrames, Vector<void*>& uniformBufferMappedMemoryFrames)
        {
            VkDeviceSize bufferSize = sizeof(_Ty);

            uniformBuffersFrames.resize(MAX_FRAMES_IN_FLIGHT);
            uniformBuffersMemoryFrames.resize(MAX_FRAMES_IN_FLIGHT);
            uniformBufferMappedMemoryFrames.resize(MAX_FRAMES_IN_FLIGHT);

            for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
                CreateBufferVMA(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uniformBuffersFrames[i], uniformBuffersMemoryFrames[i]);

                VK_CHECK(vmaMapMemory(m_Allocator, uniformBuffersMemoryFrames[i], &uniformBufferMappedMemoryFrames[i]))
            }

        }

        template<typename _Ty>
        void                                CreateUniformBuffers(UniformBufferFrameData<_Ty>& uniformData)
        {
            VkDeviceSize bufferSize = sizeof(_Ty);

            uniformData.m_UniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
            uniformData.m_UniformBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
            uniformData.m_UniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

            for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
                CreateBufferVMA(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uniformData.m_UniformBuffers[i], uniformData.m_UniformBuffersMemory[i]);

                VK_CHECK(vmaMapMemory(m_Allocator, uniformData.m_UniformBuffersMemory[i], &uniformData.m_UniformBuffersMapped[i]))
            }

        }

        virtual Vector<const char*>         GetRequiredExtensions() = 0;
        virtual void                        CreateSurface() = 0;
        virtual void                        CreateWindow(uint32_t width, uint32_t height) = 0;
        virtual void                        CleanupWindow() = 0;
        virtual VkExtent2D                  GetSurfaceExtent(VkSurfaceCapabilitiesKHR surface) = 0;
        virtual bool                        ShouldRun() = 0;
        virtual void                        PreFrame() = 0;
        virtual void                        PostFrame() = 0;
        virtual void                        Run(std::function<void()> callback) = 0;
        virtual void                        InitImGuiBackend() = 0;
        virtual void                        CleanupImGuiBackend() = 0;

        // API End
    protected:
        bool            p_ShouldRun = true;
        uint64_t        p_LastFrameTime;
        int             p_CurrentFrameIndex;
    };
}