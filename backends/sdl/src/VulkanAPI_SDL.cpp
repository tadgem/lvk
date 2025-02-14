#include "VulkanAPI_SDL.h"
#include "spdlog/spdlog.h"
#include "SDL3/SDL_vulkan.h"
#include "ImGui/imgui_impl_sdl3.h"
#include "ImGui/imgui_impl_vulkan.h"
#include "volk.h"
#include <filesystem>

lvk::VulkanAPI_SDL::~VulkanAPI_SDL()
{
}

void lvk::VulkanAPI_SDL::HandleSDLEvent(SDL_Event& sdl_event)
{
    if (sdl_event.type == SDL_EVENT_QUIT)
    {
        p_ShouldRun = false;
    }

    if (sdl_event.type == SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED) {
        RecreateSwapChain();
    }
    if (sdl_event.type == SDL_EVENT_WINDOW_MAXIMIZED) {
        RecreateSwapChain();
    }

}
std::vector<const char*> lvk::VulkanAPI_SDL::GetRequiredExtensions()
{
    uint32_t extensionCount = 0;

    const char* const * extensionNamesC =
        SDL_Vulkan_GetInstanceExtensions(&extensionCount);
    if(extensionCount == 0)
    {
        spdlog::error("Failed to enumerate required SDL device extensions");
        return {};
    }
    std::vector<const char*> extensionNames;

    for(uint32_t i = 0; i < extensionCount; i++)
    {
        extensionNames.push_back(extensionNamesC[i]);
    }
    if (m_UseValidation)
    {
        extensionNames.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    return extensionNames;
}

void lvk::VulkanAPI_SDL::CreateSurface()
{
    // todo: do we want to provide an alloc callback to SDL?
    const VkAllocationCallbacks* alloc_callback = nullptr;
    if (!SDL_Vulkan_CreateSurface(
            m_SdlHandle->m_SdlWindow, m_Instance,alloc_callback, &m_Surface))
    {
        spdlog::error("Failed to create SDL Vulkan surface");
        std::cerr << "Failed to create SDL Vulkan surface";
    }
}

void lvk::VulkanAPI_SDL::CleanupWindow()
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

bool lvk::VulkanAPI_SDL::ShouldRun()
{
    return p_ShouldRun;
}

void lvk::VulkanAPI_SDL::PreFrame()
{
    uint64_t currentFrame = SDL_GetPerformanceCounter();
    m_DeltaTime = (currentFrame - p_LastFrameTime) / (double)SDL_GetPerformanceFrequency();
    p_LastFrameTime = currentFrame;
    SDL_Event sdl_event;
    while (SDL_PollEvent(&sdl_event))
    {
        HandleSDLEvent(sdl_event);
        ImGui_ImplSDL3_ProcessEvent(&sdl_event);
    }
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();
}

void lvk::VulkanAPI_SDL::PostFrame()
{
    DrawFrame(); 

    ImGui::UpdatePlatformWindows();
    ImGui::RenderPlatformWindowsDefault();

    if (vkDeviceWaitIdle(m_LogicalDevice) != VK_SUCCESS)
    {
        spdlog::error("Failed to wait for device idle");
        std::cerr << "Failed to wait for device idle" << std::endl;
    }

    ClearCommandBuffers();
}

void lvk::VulkanAPI_SDL::InitImGuiBackend()
{
    ImGui_ImplSDL3_InitForVulkan(m_SdlHandle->m_SdlWindow);
}

void lvk::VulkanAPI_SDL::CleanupImGuiBackend()
{
    ImGui_ImplSDL3_Shutdown();
}

void lvk::VulkanAPI_SDL::Run(std::function<void()> callback)
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

        callback();

        DrawFrame();
    }
    if (vkDeviceWaitIdle(m_LogicalDevice) != VK_SUCCESS)
    {
        spdlog::error("Failed to wait for device idle");
        std::cerr << "Failed to wait for device idle" << std::endl;
    }
}

VkExtent2D lvk::VulkanAPI_SDL::GetSurfaceExtent(VkSurfaceCapabilitiesKHR surface)
{
    SDL_DisplayID id = SDL_GetPrimaryDisplay();
    SDL_DisplayMode displayMode = *SDL_GetCurrentDisplayMode(id);

    return VkExtent2D();
}

VkExtent2D lvk::VulkanAPI_SDL::GetMaxFramebufferResolution()
{
    int numDisplays = 0;
    SDL_DisplayID* ids = SDL_GetDisplays(&numDisplays);
    VkExtent2D res{};
    for (auto i = 0; i < numDisplays; i++)
    {
        int mode_count = 0;
        auto* displayModes = SDL_GetFullscreenDisplayModes(ids[i], &mode_count);
        for (int j = 0; j < mode_count; j++)
        {
            SDL_DisplayMode displayMode = *displayModes[i];

            if (res.width < static_cast<uint32_t>(displayMode.w))
            {
                res.width = displayMode.w;
            }

            if (res.height < static_cast<uint32_t>(displayMode.h))
            {
                res.height = displayMode.h;
            }
        }
    }
    return res;
}

lvk::VulkanAPI_SDL::VulkanAPI_SDL(bool enableDebugValidation) : VkAPI(enableDebugValidation)
{
    spdlog::info("LVK : current working directory : {}", std::filesystem::current_path().string());
}

lvk::VulkanAPIWindowHandle_SDL::VulkanAPIWindowHandle_SDL(SDL_Window* sdlWindow) : m_SdlWindow(sdlWindow)
{
    
}

