#pragma once
#include <iostream>
#include <map>
#include <vector>
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

    VkInstance                  m_Instance;
    VkSurfaceKHR                m_Surface;
    VkSwapchainKHR              m_SwapChain;
    VkDebugUtilsMessengerEXT    m_DebugMessenger;
    VkPhysicalDevice            m_PhysicalDevice    = VK_NULL_HANDLE;
    VkDevice                    m_LogicalDevice     = VK_NULL_HANDLE;
    VkRenderPass                m_RenderPass;
    VkPipelineLayout            m_PipelineLayout;
    VkPipeline                  m_Pipeline;
    VkCommandPool               m_CommandPool;
    QueueFamilyIndices          m_QueueFamilyIndices;

    VkQueue                     m_GraphicsQueue     = VK_NULL_HANDLE;
    VkQueue                     m_PresentQueue      = VK_NULL_HANDLE;
    
    VulkanAPIWindowHandle*      m_WindowHandle;

    std::vector<VkImage>            m_SwapChainImages;
    std::vector<VkImageView>        m_SwapChainImageViews;
    std::vector<VkFramebuffer>      m_SwapChainFramebuffers;
    std::vector<VkCommandBuffer>    m_CommandBuffers;
    VkFormat                        m_SwapChainImageFormat;
    VkExtent2D                      m_SwapChainImageExtent;

    // Debug
    bool                        CheckValidationLayerSupport();
    bool                        CheckDeviceExtensionSupport(VkPhysicalDevice device);
    void                        SetupDebugOutput();
    void                        CleanupDebugOutput();
    void                        ListDeviceExtensions(VkPhysicalDevice physicalDevice);
    void                        PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);

    // Main API Functionality
    VkApplicationInfo           CreateAppInfo();
    void                        CreateInstance();
    void                        CleanupVulkan();
    QueueFamilyIndices          FindQueueFamilies(VkPhysicalDevice physicalDevice);
    SwapChainSupportDetais      GetSwapChainSupportDetails(VkPhysicalDevice physicalDevice);
    bool                        IsDeviceSuitable(VkPhysicalDevice physicalDevice);
    uint32_t                    AssessDeviceSuitability(VkPhysicalDevice physicalDevice);
    void                        PickPhysicalDevice();
    void                        CreateLogicalDevice();
    void                        GetQueueHandles();
    VkSurfaceFormatKHR          ChooseSwapChainSurfaceFormat(std::vector<VkSurfaceFormatKHR> availableFormats);
    VkPresentModeKHR            ChooseSwapChainPresentMode(std::vector<VkPresentModeKHR> availableModes);
    VkExtent2D                  ChooseSwapExtent(VkSurfaceCapabilitiesKHR& surfaceCapabilities);
    void                        CreateSwapChain();
    void                        CreateSwapChainImageViews();
    void                        CreateGraphicsPipeline();
    VkShaderModule              CreateShaderModule(const std::vector<char>& data);
    void                        CreateRenderPass();
    void                        CreateFramebuffers();
    void                        CreateCommandPool();
    void                        CreateCommandBuffers();

    std::vector<VkExtensionProperties>  GetDeviceAvailableExtensions(VkPhysicalDevice physicalDevice);
    std::vector<char>                   LoadSpirvBinary(const std::string& path);

    // Implement for a windowing system (e.g. SDL)
    virtual std::vector<const char*>    GetRequiredExtensions() = 0;
    virtual void                        CreateSurface() = 0;
    virtual void                        CreateWindow(uint32_t width, uint32_t height) = 0;
    virtual void                        CleanupWindow() = 0;
    virtual VkExtent2D                  GetSurfaceExtent(VkSurfaceCapabilitiesKHR surface) = 0;
    void                                InitVulkan();


    
    



};