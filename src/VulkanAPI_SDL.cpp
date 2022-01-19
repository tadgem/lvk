#include "VulkanAPI_SDL.h"
#include "spdlog/spdlog.h"
#include "SDL_vulkan.h"
std::vector<const char*> VulkanAPI_SDL::GetRequiredExtensions()
{
    // as vulkan is truely platform independent, we need to query the GPU for extensions
    // which tell us what the GPU is capable of doing...
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

void VulkanAPI_SDL::CreateWindow(uint32_t width, uint32_t height)
{
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS);
    SDL_Window* window = SDL_CreateWindow("SDL Vulkan Example", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, SDL_WINDOW_VULKAN);
    m_SdlHandle = new VulkanAPIWindowHandle_SDL(window);
    m_WindowHandle = m_SdlHandle;
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

VulkanAPIWindowHandle_SDL::VulkanAPIWindowHandle_SDL(SDL_Window* sdlWindow)
{
    m_SdlWindow = sdlWindow;
}
