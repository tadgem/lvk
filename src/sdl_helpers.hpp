#include "SDL.h"
#include <functional>




class sdl_helpers
{
public:

	
	static SDL_Window* InitSDL(std::function<void()> callback)
	{
		g_Window = CreateWindow(1920, 1080);
		callback();
	}


	static void HandleSDLEvent(SDL_Event& sdl_event)
	{

	}

	static void RunSDL(std::function<void()> callback)
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

			callback();
		}
	}

	inline static double g_DeltaTime = 0.0;
	inline static SDL_Window* g_Window = nullptr;
private:
	static SDL_Window* CreateWindow(const uint32_t& height, const uint32_t& width)
	{
		SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS);
		SDL_Window* window = SDL_CreateWindow("SDL Vulkan Example", 0, 0, height, width, SDL_WINDOW_SHOWN | SDL_WINDOW_VULKAN);
		return window;
	}

	
	inline static bool p_ShouldRun = true;
	inline static uint64_t p_LastFrame = 0;
};