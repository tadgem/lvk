#include "SDL.h"
#include <functional>

#undef main

class sdl_helpers
{
public:

	inline static double g_DeltaTime = 0.0;
	static void HandleSDLEvent(SDL_Event& sdl_event)
	{
		if (sdl_event.type == SDL_QUIT)
		{
			p_ShouldRun = false;
		}
	}

	static void RunSDL(std::function<void()> update_callback)
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
	}

private:

	inline static bool p_ShouldRun = true;
	inline static uint64_t p_LastFrame = 0;
};