#include "client/client.h"
#include "client/private/client_impl.h"
#include "client/private/window_impl.h"

#define SDL_MAIN_HANDLED
#include <SDL.h>

#include <utility>

namespace Client
{
	namespace
	{
		ClientImpl* impl_ = nullptr;
	}

	void Initialize()
	{
		SDL_Init(SDL_INIT_EVERYTHING);

		DBG_ASSERT(!impl_);
		impl_ = new ClientImpl();
	}

	void Finalize()
	{
		DBG_ASSERT(impl_);
		delete impl_;
		impl_ = nullptr;

		SDL_Quit();

	}

	bool Update()
	{
		// Update window input state.
		for(auto it = impl_->windows_.begin(); it != impl_->windows_.end(); ++it)
		{
			WindowImpl* window = (*it);
			window->UpdateInputState();
		}

		return PumpMessages();
	}

	bool PumpMessages()
	{
		DBG_ASSERT(impl_);
		Core::ScopedMutex lock(impl_->resourceMutex_);
		
		SDL_Event event;
		while(SDL_PollEvent(&event))
		{
			switch(event.type)
			{
			case SDL_QUIT:
				return false;
				break;
			default:
				HandleEvent(event);
				break;
			}
			return true;
		}

		return true;
	}

	void RegisterWindow(WindowImpl* window)
	{
		DBG_ASSERT(impl_);
		Core::ScopedMutex lock(impl_->resourceMutex_);
		impl_->windows_.push_back(window);
	}

	void DeregisterWindow(WindowImpl* window)
	{
		DBG_ASSERT(impl_);
		Core::ScopedMutex lock(impl_->resourceMutex_);
		for(auto it = impl_->windows_.begin();it != impl_->windows_.end(); ++it)
		{
			if(*it == window)
			{
				impl_->windows_.erase(it);
				return;
			}
		}
	}

	void HandleEvent(const SDL_Event& event)
	{
		switch(event.type)
		{
		case SDL_WINDOWEVENT:
		case SDL_KEYDOWN:
		case SDL_KEYUP:
		case SDL_TEXTEDITING:
		case SDL_TEXTINPUT:
		case SDL_MOUSEMOTION:
		case SDL_MOUSEBUTTONDOWN:
		case SDL_MOUSEBUTTONUP:
		case SDL_MOUSEWHEEL:
		case SDL_DROPFILE:
		case SDL_DROPTEXT:
		case SDL_DROPBEGIN:
		case SDL_DROPCOMPLETE:
			{
				for(auto it = impl_->windows_.begin(); it != impl_->windows_.end(); ++it)
				{
					WindowImpl* window = (*it);
					if(event.window.windowID == SDL_GetWindowID(window->sdlWindow_))
					{
						window->HandleEvent(event);
					}
				}
			}
		}
	}


} // namespace Client
