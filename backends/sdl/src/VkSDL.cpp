#include "ImGui/imgui_impl_sdl3.h"
#include "ImGui/imgui_impl_vulkan.h"
#include "SDL3/SDL_vulkan.h"
#include "VkSDL.h"
#include "lvk/Init.h"
#include "spdlog/spdlog.h"
#include "volk.h"
#include <filesystem>

lvk::VkSDL::~VkSDL()
{
}

void lvk::VkSDL::HandleSDLEvent(VkState& vk, SDL_Event& sdl_event)
{
    if (sdl_event.type == SDL_EVENT_QUIT)
    {
        vk.m_ShouldRun = false;
    }

    if (sdl_event.type == SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED) {
        init::RecreateSwapChain(vk);
    }
    if (sdl_event.type == SDL_EVENT_WINDOW_MAXIMIZED) {
        init::RecreateSwapChain(vk);
    }

}

std::vector<const char*> lvk::VkSDL::GetRequiredExtensions(VkState& vk)
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
    if (vk.m_UseValidation)
    {
        extensionNames.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    return extensionNames;
}

void lvk::VkSDL::CreateSurface(VkState& vk)
{
    // todo: do we want to provide an alloc callback to SDL?
    const VkAllocationCallbacks* alloc_callback = nullptr;
    if (!SDL_Vulkan_CreateSurface(
            m_SdlHandle->m_SdlWindow, vk.m_Instance,alloc_callback, &vk.m_Surface))
    {
        spdlog::error("Failed to create SDL Vulkan surface");
        std::cerr << "Failed to create SDL Vulkan surface";
    }
}

void lvk::VkSDL::CleanupWindow(VkState& vk)
{
    VulkanAPIWindowHandle_SDL* derived = static_cast<VulkanAPIWindowHandle_SDL*>(vk.m_WindowHandle);

    if (derived == nullptr)
    {
        spdlog::error("Failed to cast Window Handle to SDL WindowHandle");
        return;
    }
    SDL_DestroyWindow(m_SdlHandle->m_SdlWindow);
    SDL_Quit();
}

bool lvk::VkSDL::ShouldRun(VkState& vk)
{
    return vk.m_ShouldRun;
}

void lvk::VkSDL::PreFrame(VkState& vk)
{
    uint64_t currentFrame = SDL_GetPerformanceCounter();
    vk.m_DeltaTime = (currentFrame - vk.m_LastFrameTime) / (double)SDL_GetPerformanceFrequency();
    vk.m_LastFrameTime = currentFrame;
    SDL_Event sdl_event;
    while (SDL_PollEvent(&sdl_event))
    {
        HandleSDLEvent(vk, sdl_event);
        ImGui_ImplSDL3_ProcessEvent(&sdl_event);
    }
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();
}

void lvk::VkSDL::PostFrame(VkState& vk)
{
    init::DrawFrame(vk);

    ImGui::UpdatePlatformWindows();
    ImGui::RenderPlatformWindowsDefault();

    if (vkDeviceWaitIdle(vk.m_LogicalDevice) != VK_SUCCESS)
    {
        spdlog::error("Failed to wait for device idle");
        std::cerr << "Failed to wait for device idle" << std::endl;
    }

    init::ClearCommandBuffers(vk);
}

void lvk::VkSDL::InitImGuiBackend(VkState& vk)
{
    ImGui_ImplSDL3_InitForVulkan(m_SdlHandle->m_SdlWindow);
}

void lvk::VkSDL::CleanupImGuiBackend(VkState& vk)
{
    ImGui_ImplSDL3_Shutdown();
}

void lvk::VkSDL::Run(VkState& vk, std::function<void()> callback)
{
    vk.m_ShouldRun = true;
    while (vk.m_ShouldRun)
    {
        uint64_t currentFrame = SDL_GetPerformanceCounter();
        vk.m_DeltaTime = (currentFrame - vk.m_LastFrameTime) / (double)SDL_GetPerformanceFrequency();
        SDL_Event sdl_event;
        while (SDL_PollEvent(&sdl_event))
        {
            HandleSDLEvent(vk, sdl_event);
        }

        callback();

        init::DrawFrame(vk);
    }
    if (vkDeviceWaitIdle(vk.m_LogicalDevice) != VK_SUCCESS)
    {
        spdlog::error("Failed to wait for device idle");
        std::cerr << "Failed to wait for device idle" << std::endl;
    }
}

VkExtent2D lvk::VkSDL::GetSurfaceExtent(VkState& vk, VkSurfaceCapabilitiesKHR surface)
{
    SDL_DisplayID id = SDL_GetPrimaryDisplay();
    SDL_DisplayMode displayMode = *SDL_GetCurrentDisplayMode(id);

    return VkExtent2D();
}

VkExtent2D lvk::VkSDL::GetMaxFramebufferResolution(VkState& vk)
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

lvk::VkSDL::VkSDL(bool enableDebugValidation)
{
    spdlog::info("LVK : current working directory : {}", std::filesystem::current_path().string());
}

lvk::VulkanAPIWindowHandle_SDL::VulkanAPIWindowHandle_SDL(SDL_Window* sdlWindow) : m_SdlWindow(sdlWindow)
{
    
}

