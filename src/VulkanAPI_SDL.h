#pragma once
#include "VulkanAPI.h"
#include "SDL.h"

class VulkanAPIWindowHandle_SDL : public VulkanAPIWindowHandle
{
public:
	VulkanAPIWindowHandle_SDL(SDL_Window* sdlWindow);
	SDL_Window* m_SdlWindow;
};

class VulkanAPI_SDL : public VulkanAPI
{
public:
	// Inherited via VulkanAPI
	virtual std::vector<const char*> GetRequiredExtensions() override;
	virtual void CreateSurface() override;
	virtual void CreateWindow(uint32_t width, uint32_t height) override;
	virtual void CleanupWindow() override;

	VulkanAPIWindowHandle_SDL* m_SdlHandle;

	 
};