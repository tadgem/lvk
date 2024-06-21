#include "VulkanAPI_SDL.h"
#include "spdlog/spdlog.h"
#include "SDL_vulkan.h"
#include "ImGui/imgui_impl_sdl2.h"
#include "ImGui/imgui_impl_vulkan.h"


lvk::VulkanAPI_SDL::~VulkanAPI_SDL()
{
    Cleanup();
}

void lvk::VulkanAPI_SDL::HandleSDLEvent(SDL_Event& sdl_event)
{
    if (sdl_event.type == SDL_QUIT)
    {
        p_ShouldRun = false;
    }
    if (sdl_event.type == SDL_WINDOWEVENT) {
        if (sdl_event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
            RecreateSwapChain();
        }
    }
}
std::vector<const char*> lvk::VulkanAPI_SDL::GetRequiredExtensions()
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

void lvk::VulkanAPI_SDL::CreateSurface()
{
    if (!SDL_Vulkan_CreateSurface(m_SdlHandle->m_SdlWindow, m_Instance, &m_Surface))
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
    while (SDL_PollEvent(&sdl_event) > 0)
    {
        HandleSDLEvent(sdl_event);
        ImGui_ImplSDL2_ProcessEvent(&sdl_event);
    }
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplSDL2_NewFrame();
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
    ImGui_ImplSDL2_InitForVulkan(m_SdlHandle->m_SdlWindow);
}

void lvk::VulkanAPI_SDL::CleanupImGuiBackend()
{
    ImGui_ImplSDL2_Shutdown();
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
    SDL_DisplayMode displayMode;
    SDL_GetCurrentDisplayMode(0, &displayMode);

    return VkExtent2D();
}

VkExtent2D lvk::VulkanAPI_SDL::GetMaxFramebufferResolution()
{
    auto numDispays = SDL_GetNumVideoDisplays();
    VkExtent2D res{};
    for (auto i = 0; i < numDispays; i++)
    {
        auto displayModes = SDL_GetNumDisplayModes(i);
        for (int j = 0; j < displayModes; j++)
        {
            SDL_DisplayMode displayMode;
            SDL_GetDisplayMode(i, j, &displayMode);

            if (res.width < displayMode.w)
            {
                res.width = displayMode.w;
            }

            if (res.height < displayMode.h)
            {
                res.height = displayMode.h;
            }
        }
    }
    return res;
}

lvk::VulkanAPIWindowHandle_SDL::VulkanAPIWindowHandle_SDL(SDL_Window* sdlWindow) : m_SdlWindow(sdlWindow)
{
    
}

