#pragma once

#include <iostream>
#include <map>
#include <vector>
#include <functional>
#include <vulkan/vulkan.h>

class VulkanAPIWindowHandle
{
};

class VulkanAPI
{
protected:
    const bool                  p_UseValidation = true;
    const std::vector<const char*> p_ValidationLayers = {
        "VK_LAYER_KHRONOS_validation"
    };

    const std::vector<const char*> p_DeviceExtensions = {
        "VK_KHR_swapchain"
    };
public:

    enum class ShaderStage
    {
        Vertex,
        Fragment
    };

    enum class QueueFamilyType
    {
        Graphics = VK_QUEUE_GRAPHICS_BIT,
        Present

    };

    struct QueueFamilyIndices
    {
        std::map<QueueFamilyType, uint32_t> m_QueueFamilies;

        bool IsComplete();
        
    };

    struct SwapChainSupportDetais
    {
        VkSurfaceCapabilitiesKHR        m_Capabilities;
        std::vector<VkSurfaceFormatKHR> m_SupportedFormats;
        std::vector<VkPresentModeKHR>   m_SupportedPresentModes;
    };

    VkInstance                      m_Instance;
    VkSurfaceKHR                    m_Surface;
    VkSwapchainKHR                  m_SwapChain;
    VkDebugUtilsMessengerEXT        m_DebugMessenger;
    VkPhysicalDevice                m_PhysicalDevice    = VK_NULL_HANDLE;
    VkDevice                        m_LogicalDevice     = VK_NULL_HANDLE;
    VkRenderPass                    m_RenderPass;
    VkCommandPool                   m_GraphicsQueueCommandPool;
    std::vector<VkSemaphore>        m_ImageAvailableSemaphores;
    std::vector<VkSemaphore>        m_RenderFinishedSemaphores;
    std::vector<VkFence>            m_FrameInFlightFences;
    std::vector<VkFence>            m_ImagesInFlight;
    QueueFamilyIndices              m_QueueFamilyIndices;

    VkQueue                         m_GraphicsQueue     = VK_NULL_HANDLE;
    VkQueue                         m_PresentQueue      = VK_NULL_HANDLE;
    
    VulkanAPIWindowHandle*          m_WindowHandle;

    std::vector<VkImage>            m_SwapChainImages;
    std::vector<VkImageView>        m_SwapChainImageViews;
    std::vector<VkFramebuffer>      m_SwapChainFramebuffers;
    std::vector<VkCommandBuffer>    m_CommandBuffers;
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
    VkSurfaceFormatKHR                  ChooseSwapChainSurfaceFormat(std::vector<VkSurfaceFormatKHR> availableFormats);
    VkPresentModeKHR                    ChooseSwapChainPresentMode(std::vector<VkPresentModeKHR> availableModes);
    void                                CreateSwapChain();
    void                                CreateSwapChainFramebuffers();
    void                                CreateSwapChainImageViews();
    void                                CleanupSwapChain();
    void                                RecreateSwapChain();
    VkExtent2D                          ChooseSwapExtent(VkSurfaceCapabilitiesKHR& surfaceCapabilities);
    VkShaderModule                      CreateShaderModule(const std::vector<char>& data);
    void                                CreateCommandPool();
    void                                CreateSemaphores();
    void                                CreateFences();
    void                                DrawFrame();
    inline int                          GetFrameIndex() { return p_CurrentFrameIndex; }

    // todo: app specific
    void                                CreateRenderPass();
    
    std::vector<VkExtensionProperties>  GetDeviceAvailableExtensions(VkPhysicalDevice physicalDevice);
    std::vector<char>                   LoadSpirvBinary(const std::string& path);

    // Implement for a windowing system (e.g. SDL)
    virtual std::vector<const char*>    GetRequiredExtensions() = 0;
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
    const int       MAX_FRAMES_IN_FLIGHT = 2;
};