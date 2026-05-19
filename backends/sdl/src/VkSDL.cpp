#include "ThirdParty/nanovg.h"
#include "ImGui/imgui_impl_sdl3.h"
#include "ImGui/imgui_impl_vulkan.h"
#include "SDL3/SDL_vulkan.h"
#include "VkSDL.h"
#include "lvk/Init.h"
#include "lvk/Submission.h"
#include "lvk/Log.h"
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

lvk::Vector<const char*> lvk::VkSDL::GetRequiredInstanceExtensions(VkState& vk)
{
    uint32_t extensionCount = 0;

    const char* const * extensionNamesC =
        SDL_Vulkan_GetInstanceExtensions(&extensionCount);
    if(extensionCount == 0)
    {
        LVK_LOG_ERR("Failed to enumerate required SDL device extensions");
        return Vector<const char*>(*vk.m_CPUAllocator);
    }
    STLAllocator<const char*> alloc(*vk.m_CPUAllocator);
    Vector<const char*> extensionNames(alloc);

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
        LVK_LOG_ERR("Failed to create SDL Vulkan surface");
        std::cerr << "Failed to create SDL Vulkan surface";
    }
}

void lvk::VkSDL::CleanupWindow(VkState& vk)
{
    VulkanAPIWindowHandle_SDL* derived = static_cast<VulkanAPIWindowHandle_SDL*>(vk.m_WindowHandle);

    if (derived == nullptr)
    {
        LVK_LOG_ERR("Failed to cast Window Handle to SDL WindowHandle");
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
    ImGui::EndFrame();
    ImGui::UpdatePlatformWindows();
    ImGui::RenderPlatformWindowsDefault();
    submission::SubmitFrame(vk);

    if (vkDeviceWaitIdle(vk.m_LogicalDevice) != VK_SUCCESS)
    {
        LVK_LOG_ERR("Failed to wait for device idle");
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

        submission::SubmitFrame(vk);
    }
    if (vkDeviceWaitIdle(vk.m_LogicalDevice) != VK_SUCCESS)
    {
        LVK_LOG_ERR("Failed to wait for device idle");
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

lvk::StageBinary lvk::VkSDL::LoadBinaryFromPath(VkState& vk, const char* path)
{
    size_t numBytes;
    void* addr = SDL_LoadFile(path, &numBytes);
    StageBinary binary(*vk.m_CPUAllocator);
    binary.resize(numBytes);
    memcpy(binary.data(), addr, numBytes);
    SDL_free(addr);
    return binary;
}

lvk::String lvk::VkSDL::LoadStringFromPath(VkState& vk, const char* path)
{
    size_t numBytes;
    void* addr = SDL_LoadFile(path, &numBytes);
    String str(*vk.m_CPUAllocator);
    str.resize(numBytes);
    memcpy(str.data(), addr, numBytes);
    SDL_free(addr);
    return str;
}

lvk::VkSDL::VkSDL(bool enableDebugValidation)
{
    LVK_LOG_INFO("LVK : current working directory : %s", std::filesystem::current_path().string().c_str());
}

lvk::VulkanAPIWindowHandle_SDL::VulkanAPIWindowHandle_SDL(SDL_Window* sdlWindow) : m_SdlWindow(sdlWindow)
{
    
}

