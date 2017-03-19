#include "client/client.h"

#define SDL_MAIN_HANDLED
#include <SDL.h>

namespace Client
{
	void Initialize()
	{ //
		SDL_Init(SDL_INIT_EVERYTHING);
	}

	void Finalize()
	{ //
		SDL_Quit();
	}

	bool PumpMessages()
	{
		SDL_Event event;
		while(SDL_PollEvent(&event))
		{
			switch(event.type)
			{
			case SDL_QUIT:
				return false;
				break;
			default:
				break;
			}
			return true;
		}

		return true;
	}

} // namespace Client
