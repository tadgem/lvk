#include "VulkanAPI_SDL.h"
#include "spdlog/spdlog.h"
#include "SDL_vulkan.h"

void VulkanAPI_SDL::HandleSDLEvent(SDL_Event& sdl_event)
{
    if (sdl_event.type == SDL_QUIT)
    {
        p_ShouldRun = false;
    }
}
std::vector<const char*> VulkanAPI_SDL::GetRequiredExtensions()
{
    uint32_t extensionCount = 0;
    if (SDL_Vulkan_GetInstanceExtensions(nullptr, &extensionCount, nullptr) != SDL_TRUE)
    {
        spdlog::error("Failed to enumerate instance extensions!");
    }

    std::vector<const char*> extensionNames(extensionCount);
    SDL_Vulkan_GetInstanceExtensions(m_SdlHandle->m_SdlWindow, &extensionCount, extensionNames.data());

    if (p_UseValidation)
    {
        extensionNames.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    return extensionNames;
}

void VulkanAPI_SDL::CreateSurface()
{
    if (!SDL_Vulkan_CreateSurface(m_SdlHandle->m_SdlWindow, m_Instance, &m_Surface))
    {
        spdlog::error("Failed to create SDL Vulkan surface");
        std::cerr << "Failed to create SDL Vulkan surface";
    }
}

void VulkanAPI_SDL::CleanupWindow()
{
    VulkanAPIWindowHandle_SDL* derived = static_cast<VulkanAPIWindowHandle_SDL*>(m_WindowHandle);

    if (derived == nullptr)
    {
        spdlog::error("Failed to cast Window Handle to SDL WindowHandle");
        return;
    }
    SDL_DestroyWindow(m_SdlHandle->m_SdlWindow);
    SDL_Quit();
}

void VulkanAPI_SDL::Run(std::function<void()> callback)
{
    p_ShouldRun = true;
    while (p_ShouldRun)
    {
        uint64_t currentFrame = SDL_GetPerformanceCounter();
        m_DeltaTime = (currentFrame - p_LastFrameTime) / (double)SDL_GetPerformanceFrequency();
        SDL_Event sdl_event;
        while (SDL_PollEvent(&sdl_event))
        {
            HandleSDLEvent(sdl_event);
        }

        DrawFrame();

        callback();
    }
    if (vkDeviceWaitIdle(m_LogicalDevice) != VK_SUCCESS)
    {
        spdlog::error("Failed to wait for device idle");
        std::cerr << "Failed to wait for device idle" << std::endl;
    }
}

VkExtent2D VulkanAPI_SDL::GetSurfaceExtent(VkSurfaceCapabilitiesKHR surface)
{
    SDL_DisplayMode displayMode;
    SDL_GetCurrentDisplayMode(0, &displayMode);

    return VkExtent2D();
}

VulkanAPIWindowHandle_SDL::VulkanAPIWindowHandle_SDL(SDL_Window* sdlWindow) : m_SdlWindow(sdlWindow)
{
    
}

