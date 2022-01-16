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
public:

    enum class QueueFamilyType
    {
        Graphics = VK_QUEUE_GRAPHICS_BIT,
        Presentation

    };

    struct QueueFamilyIndices
    {
        std::map<QueueFamilyType, uint32_t> m_QueueFamilies;

        bool IsComplete();
        
    };

    VkInstance                  m_Instance;
    VkSurfaceKHR                m_Surface;
    VkDebugUtilsMessengerEXT    m_DebugMessenger;
    VkPhysicalDevice            m_PhysicalDevice    = VK_NULL_HANDLE;
    VkDevice                    m_LogicalDevice     = VK_NULL_HANDLE;

    QueueFamilyIndices          m_QueueFamilyIndices;

    VkQueue                     m_GraphicsQueue     = VK_NULL_HANDLE;
    
    VulkanAPIWindowHandle*      m_WindowHandle;

    VkApplicationInfo           CreateAppInfo();
    bool                        CheckValidationLayerSupport();
    void                        CreateInstance();
    void                        SetupDebugOutput();
    void                        CleanupDebugOutput();
    void                        CleanupVulkan();
    QueueFamilyIndices          FindQueueFamilies(VkPhysicalDevice physicalDevice);
    bool                        IsDeviceSuitable(VkPhysicalDevice physicalDevice);
    uint32_t                    AssessDeviceSuitability(VkPhysicalDevice physicalDevice);
    void                        PickPhysicalDevice();
    void                        CreateLogicalDevice();
    void                        GetQueueHandles();

    virtual std::vector<const char*>    GetRequiredExtensions() = 0;
    virtual VkSurfaceKHR                CreateSurface() = 0;
    virtual void                        CreateWindow(uint32_t width, uint32_t height) = 0;
    virtual void                        CleanupWindow() = 0;
    void                        InitVulkan();


    void PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
    



};