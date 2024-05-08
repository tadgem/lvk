#pragma once
#include "Alias.h"
#include <vulkan/vulkan.h>
#include "spdlog/spdlog.h"
#include "spirv_reflect.h"
#define VK_CHECK(X) {int _lineNumber = __LINE__; const char* _filePath = __FILE__;\
if(X != VK_SUCCESS){\
spdlog::error("VK check failed at {} Line {} : {}",_filePath, _lineNumber, #X);}}
#include "VulkanMemoryAllocator.h"
#include "stb_image.h"

namespace lvk
{

    class VulkanAPIWindowHandle
    {
    };

    struct DescriptorSetLayoutBindingData
    {
        uint32_t                     m_ExpectedBlockSize;
    };

    struct DescriptorSetLayoutData {
        uint32_t                                    m_SetNumber;
        VkDescriptorSetLayoutCreateInfo             m_CreateInfo;
        VkDescriptorSetLayout                       m_Layout;
        Vector<VkDescriptorSetLayoutBinding>        m_Bindings;
        Vector<DescriptorSetLayoutBindingData>      m_BindingDatas;
    };

    struct ShaderModule
    {
        Vector<DescriptorSetLayoutData>    m_DescriptorSetLayoutData;
        Vector<char>                       m_Binary;
    };

    class VulkanAPI
    {
    protected:
        const bool                  p_UseValidation = true;
        const Vector<const char*> p_ValidationLayers = {
            "VK_LAYER_KHRONOS_validation"
        };

        const Vector<const char*> p_DeviceExtensions = {
            "VK_KHR_swapchain"
        };
    public:

        static constexpr int       MAX_FRAMES_IN_FLIGHT = 2;

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
            VkSurfaceCapabilitiesKHR        m_Capabilities;
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

        double                          m_DeltaTime;

        // Debug
        bool                                CheckValidationLayerSupport();
        bool                                CheckDeviceExtensionSupport(VkPhysicalDevice device);
        void                                SetupDebugOutput();
        void                                CleanupDebugOutput();
        void                                ListDeviceExtensions(VkPhysicalDevice physicalDevice);
        void                                PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);

        // Main API Functionality
        VkApplicationInfo                   CreateAppInfo();
        void                                CreateInstance();
        void                                Cleanup();
        void                                CleanupVulkan();
        void                                Quit();
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
        VkExtent2D                          ChooseSwapExtent(VkSurfaceCapabilitiesKHR& surfaceCapabilities);
        VkShaderModule                      CreateShaderModule(const Vector<char>& data);
        void                                CreateCommandPool();
        void                                CreateDescriptorPool();
        void                                CreateSemaphores();
        void                                CreateFences();
        void                                DrawFrame();
        void                                CreateCommandBuffers();
        void                                ClearCommandBuffers();
        void                                CreateVmaAllocator();

        inline int                          GetFrameIndex() { return p_CurrentFrameIndex; }

        // helpers
        uint32_t                            FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
        void                                CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& allocation);
        void                                CreateBufferVMA(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VmaAllocation& allocation);
        void                                CreateImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);
        
        void                                CopyBuffer(VkBuffer& src, VkBuffer& dst, VkDeviceSize size);
        void                                CopyBufferToImage(VkBuffer& src, VkImage& image,  uint32_t width, uint32_t height);
        VkCommandBuffer                     BeginSingleTimeCommands();
        void                                EndSingleTimeCommands(VkCommandBuffer& commandBuffer);
        void                                TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);

        // todo: app specific
        void                                CreateRenderPass();
        Vector<VkExtensionProperties>       GetDeviceAvailableExtensions(VkPhysicalDevice physicalDevice);
        Vector<char>                        LoadSpirvBinary(const std::string& path);
        ShaderModule                        LoadShaderModule(const std::string& path);

        Vector<DescriptorSetLayoutData>     CreateDescriptorSetLayoutDatas(std::vector<char>& stageBin);
        VkDescriptorSet                     CreateDescriptorSet(DescriptorSetLayoutData& layoutData);
            

        // Implement for a windowing system (e.g. SDL)
        virtual Vector<const char*>    GetRequiredExtensions() = 0;
        virtual void                        CreateSurface() = 0;
        virtual void                        CreateWindow(uint32_t width, uint32_t height) = 0;
        virtual void                        CleanupWindow() = 0;
        virtual VkExtent2D                  GetSurfaceExtent(VkSurfaceCapabilitiesKHR surface) = 0;
        virtual bool                        ShouldRun() = 0;
        virtual void                        PreFrame() = 0;
        virtual void                        PostFrame() = 0;
        virtual void                        Run(std::function<void()> callback) = 0;
        void                                InitVulkan();

    protected:
        bool            p_ShouldRun = true;
        double          p_LastFrameTime;
        int             p_CurrentFrameIndex;
    };
}