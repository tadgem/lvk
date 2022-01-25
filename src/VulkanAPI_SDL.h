#pragma once
#include "VulkanAPI.h"
#include "SDL.h"
#undef main // why is this a thing SDL?!
class VulkanAPIWindowHandle_SDL : public VulkanAPIWindowHandle
{
public:
	VulkanAPIWindowHandle_SDL(SDL_Window* sdlWindow);
	SDL_Window* m_SdlWindow;
};

class VulkanAPI_SDL : public VulkanAPI
{
public:

	void HandleSDLEvent(SDL_Event& sdl_event);
	// Inherited via VulkanAPI
	virtual std::vector<const char*> GetRequiredExtensions() override;
	virtual void CreateSurface() override;
	virtual void CreateWindow(uint32_t width, uint32_t height) override;
	virtual void CleanupWindow() override;
	virtual void Run(std::function<void()> callback) override;
	virtual VkExtent2D	GetSurfaceExtent(VkSurfaceCapabilitiesKHR surface) override;

	VulkanAPIWindowHandle_SDL* m_SdlHandle;

	 
};