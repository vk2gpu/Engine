#pragma once

#include "core/types.h"

#include <SDL_events.h>

namespace Client
{
	struct WindowImpl
	{
		SDL_Window* sdlWindow_ = nullptr;

		void HandleEvent(const SDL_Event& event);
		void HandleEventWindow(const SDL_Event& event);
		void HandleEventKey(const SDL_Event& event);
		void HandleEventTextEditing(const SDL_Event& event);
		void HandleEventTextInput(const SDL_Event& event);
		void HandleEventMouse(const SDL_Event& event);
		void HandleEventDrop(const SDL_Event& event);
		void HandleEventDropComplete(const SDL_Event& event);
	};

} // namespace Client

