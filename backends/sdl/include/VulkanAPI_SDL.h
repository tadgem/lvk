#pragma once
#include "VulkanAPI.h"
#include "SDL.h"
#undef main // why is this a thing SDL?!
namespace lvk {
	class VulkanAPIWindowHandle_SDL : public VulkanAPIWindowHandle
	{
	public:
		VulkanAPIWindowHandle_SDL(SDL_Window* sdlWindow);
		SDL_Window* m_SdlWindow;
	};

	class VulkanAPI_SDL : public VulkanAPI
	{
	public:
		virtual ~VulkanAPI_SDL();
		void 								HandleSDLEvent(SDL_Event& sdl_event);
		// Inherited via VulkanAPI
		virtual std::vector<const char*> 	GetRequiredExtensions() override;
		virtual void 						CreateSurface() override;
		virtual void 						CreateWindow(uint32_t width, uint32_t height) override
		{
			SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS);
			SDL_Window* window = SDL_CreateWindow("SDL Vulkan Example", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
			m_SdlHandle = new VulkanAPIWindowHandle_SDL(window);
			m_WindowHandle = m_SdlHandle;
		}
		virtual void 						CleanupWindow() override;
		virtual void 						Run(std::function<void()> callback) override;
		virtual VkExtent2D					GetSurfaceExtent(VkSurfaceCapabilitiesKHR surface) override;
		virtual bool						ShouldRun() override;
		virtual void 						PreFrame() override;
		virtual void 						PostFrame() override;
		virtual void                        InitImGuiBackend() override;
		virtual void                        CleanupImGuiBackend() override;

		VulkanAPIWindowHandle_SDL* m_SdlHandle;
	};
}