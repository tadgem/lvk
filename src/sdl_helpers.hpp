#include "SDL.h"
#include <functional>

#undef main

class sdl_helpers
{
public:

	inline static double g_DeltaTime = 0.0;
	inline static SDL_Window* g_Window = nullptr;

	static SDL_Window* InitSDL(std::function<void()> callback)
	{
		g_Window = CreateWindow(1920, 1080);
		callback();
		return g_Window;
	}


	static void HandleSDLEvent(SDL_Event& sdl_event)
	{
		if (sdl_event.type == SDL_QUIT)
		{
			p_ShouldRun = false;
		}
	}

	static void RunSDL(std::function<void()> update_callback, std::function<void()> cleanup_callback)
	{
		while (p_ShouldRun)
		{
			uint64_t currentFrame = SDL_GetPerformanceCounter();
			g_DeltaTime = (currentFrame - p_LastFrame) / (double) SDL_GetPerformanceFrequency();
			SDL_Event sdl_event;
			while (SDL_PollEvent(&sdl_event))
			{
				HandleSDLEvent(sdl_event);
			}

			update_callback();
		}

		cleanup_callback();
		CleanupSDL();
	}

private:
	static SDL_Window* CreateWindow(const uint32_t& height, const uint32_t& width)
	{
		SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS);
		SDL_Window* window = SDL_CreateWindow("SDL Vulkan Example", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, height, width, SDL_WINDOW_VULKAN);
		return window;
	}
	
	static void CleanupSDL()
	{
		SDL_DestroyWindow(g_Window);
		SDL_Quit();
	}

	inline static bool p_ShouldRun = true;
	inline static uint64_t p_LastFrame = 0;
};