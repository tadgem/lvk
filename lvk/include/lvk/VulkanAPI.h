#pragma once
#include "Macros.h"
#include "Structs.h"

#include "ThirdParty/spirv_reflect.h"
#include "ThirdParty/stb_image.h"
#include "ImGui/imgui.h"
#include "spdlog/spdlog.h"
#include "lvk/DescriptorSetAllocator.h"
namespace lvk
{
    class VulkanAPI;
    struct ShaderProgram;

    static constexpr int        MAX_FRAMES_IN_FLIGHT = 2;

    static ShaderBufferMemberType GetTypeFromSpvReflect(SpvReflectTypeDescription* typeDescription);

    class VulkanAPI
    {
    protected:
        const Vector<const char*>   p_ValidationLayers = {
            "VK_LAYER_KHRONOS_validation"
        };
        const Vector<const char*>   p_DeviceExtensions = {
            "VK_KHR_swapchain"
        };
    public:
        bool                        m_UseSwapchainMsaa = false;
        const bool                  m_UseValidation = true;
        const bool                  m_UseImGui      = true;

        VulkanAPI(bool enableDebugValidation);

        VkInstance                      m_Instance;
        VkSurfaceKHR                    m_Surface;
        VkSwapchainKHR                  m_SwapChain;
        VkDebugUtilsMessengerEXT        m_DebugMessenger;
        VkPhysicalDevice                m_PhysicalDevice = VK_NULL_HANDLE;
        VkDevice                        m_LogicalDevice = VK_NULL_HANDLE;
        VkRenderPass                    m_SwapchainImageRenderPass;
        VkRenderPass                    m_ImGuiRenderPass;
        VkCommandPool                   m_GraphicsQueueCommandPool;
        VmaAllocator                    m_Allocator;
        DescriptorSetAllocator          m_DescriptorSetAllocator;

        Vector<VkSemaphore>             m_ImageAvailableSemaphores;
        Vector<VkSemaphore>             m_RenderFinishedSemaphores;
        Vector<VkFence>                 m_FrameInFlightFences;
        Vector<VkFence>                 m_ImagesInFlight;
        QueueFamilyIndices              m_QueueFamilyIndices;

        VkQueue                         m_GraphicsQueue = VK_NULL_HANDLE;
        VkQueue                         m_ComputeQueue = VK_NULL_HANDLE;
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



        virtual Vector<const char*>         GetRequiredExtensions() = 0;
        virtual void                        CreateSurface() = 0;
        virtual void                        CreateWindowLVK(uint32_t width, uint32_t height) = 0;
        virtual void                        CleanupWindow() = 0;
        virtual VkExtent2D                  GetSurfaceExtent(VkSurfaceCapabilitiesKHR surface) = 0;
    public:
        virtual VkExtent2D                  GetMaxFramebufferResolution() = 0;
        virtual bool                        ShouldRun() = 0;
        virtual void                        PreFrame() = 0;
        virtual void                        PostFrame() = 0;
        virtual void                        Run(std::function<void()> callback) = 0;
        virtual void                        InitImGuiBackend() = 0;
        virtual void                        CleanupImGuiBackend() = 0;
        void                                CleanupImGui();
        void                                Cleanup();
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
        void                                CreateDescriptorSetAllocator();
        void                                CreateSemaphores();
        void                                CreateFences();
        void                                DrawFrame();
        void                                CreateCommandBuffers();
        void                                ClearCommandBuffers();
        void                                CreateVmaAllocator();
        void                                GetMaxUsableSampleCount();
public:
        void                                Start(const String& appName, uint32_t width, uint32_t height, bool enableSwapchainMsaa = false);
        void                                Quit();
        inline int                          GetFrameIndex() { return p_CurrentFrameIndex; }
        VkExtent2D                          GetMaxFramebufferExtent();

        // API Begin
        uint32_t                            FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
        VkFormat                            FindSupportedFormat(const Vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
        VkFormat                            FindDepthFormat();
        bool                                HasStencilComponent(VkFormat& format);
        void                                CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& allocation);
        void                                CreateBufferVMA(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VmaAllocation& allocation);
        void                                CreateImage(uint32_t width, uint32_t height, uint32_t numMips, VkSampleCountFlagBits sampleCount, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory, uint32_t depth = 1);
        void                                CreateImageView(VkImage& image, VkFormat format, uint32_t numMips, VkImageAspectFlags aspectFlags, VkImageView& imageView, VkImageViewType imageViewType= VK_IMAGE_VIEW_TYPE_2D);
        void                                CreateImageSampler(VkImageView& imageView, uint32_t numMips, VkFilter filterMode, VkSamplerAddressMode addressMode, VkSampler& sampler);
        void                                CreateFramebuffer(Vector<VkImageView>& attachments, VkRenderPass renderPass, VkExtent2D extent, VkFramebuffer& framebuffer);
        void                                CreateTexture(const String& path, VkFormat format, VkImage& image, VkImageView& imageView, VkDeviceMemory& imageMemory, uint32_t* numMips = nullptr);
        void                                CreateTextureFromMemory(unsigned char* tex_data, uint32_t dataSize, VkFormat format, VkImage& image, VkImageView& imageView, VkDeviceMemory& imageMemory, uint32_t* numMips = nullptr);
        void                                CreateTexture3DFromMemory(unsigned char* tex_data, VkExtent3D extent, uint32_t dataSize, VkFormat format, VkImage& image, VkImageView& imageView, VkDeviceMemory& imageMemory, uint32_t* numMips = nullptr);
        void                                CopyBuffer(VkBuffer& src, VkBuffer& dst, VkDeviceSize size);
        void                                CopyBufferToImage(VkBuffer& src, VkImage& image,  uint32_t width, uint32_t height);
        VkCommandBuffer                     BeginSingleTimeCommands();
        void                                EndSingleTimeCommands(VkCommandBuffer& commandBuffer);
        VkCommandBuffer&                    BeginGraphicsCommands(uint32_t frameIndex);
        void                                EndGraphicsCommands(uint32_t frameIndex);
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
        Vector<VkDescriptorSetLayoutBinding>GetDescriptorSetLayoutBindings(Vector<DescriptorSetLayoutData>& vertLayoutDatas, Vector<DescriptorSetLayoutData>& fragLayoutDatas);
        void                                CreateDescriptorSetLayout(Vector<DescriptorSetLayoutData>& vertLayoutDatas, Vector<DescriptorSetLayoutData>& fragLayoutDatas, VkDescriptorSetLayout& descriptorSetLayout);
        void                                CreateRenderPass(VkRenderPass& renderPass, Vector<VkAttachmentDescription>& colourAttachments, Vector<VkAttachmentDescription>& resolveAttachments, bool hasDepthAttachment = true, VkAttachmentDescription depthAttachment = {}, VkAttachmentLoadOp attachmentLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR);
        void                                CreateBuiltInRenderPasses();
        
        VkPipeline                          CreateComputePipeline(
                                            StageBinary& comp,
                                            VkDescriptorSetLayout& descriptorSetLayout,
                                            uint32_t width, uint32_t height,
                                            VkPipelineLayout& pipelineLayout);

        VkPipeline                          CreateRasterizationGraphicsPipeline(
                                            ShaderProgram& shader,
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
        VkShaderModule                      CreateShaderModule(const StageBinary& data);
        Vector<DescriptorSetLayoutData>     ReflectDescriptorSetLayouts(StageBinary& stageBin);
        Vector<PushConstantBlock>           ReflectPushConstants(StageBinary& stageBin);
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

        void                                CreateMappedBuffer(MappedBuffer& buf, VkBufferUsageFlags bufferUsage, VkMemoryPropertyFlags memoryProperties, uint32_t size);
        void                                CreateUniformBuffers(ShaderBufferFrameData& uniformData, VkDeviceSize bufferSize);
        template<typename _Ty>
        void                                CreateUniformBuffers(ShaderBufferFrameData& uniformData)
        {
            constexpr VkDeviceSize bufferSize = sizeof(_Ty);
            CreateUniformBuffers(uniformData, bufferSize);
        }

        // API End
    protected:
        bool            p_ShouldRun = true;
        uint64_t        p_LastFrameTime;
        int             p_CurrentFrameIndex;
        VkExtent2D      p_MaxFramebufferExtent;
        String          p_AppName;
    };
}