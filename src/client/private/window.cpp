#include "client/window.h"
#include "client/private/client_impl.h"
#include "client/key_input.h"
#include "core/debug.h"

#include <SDL.h>
#include <SDL_syswm.h>

namespace Client
{
	void WindowImpl::HandleEvent(const SDL_Event& event)
	{
		switch(event.type)
		{
		case SDL_WINDOWEVENT:
			HandleEvent(event);
			break;
		case SDL_KEYDOWN:
		case SDL_KEYUP:
			HandleEventKey(event);
			break;
		case SDL_TEXTEDITING:
			HandleEventTextEditing(event);
			break;
		case SDL_TEXTINPUT:
			HandleEventTextInput(event);
			break;
		case SDL_MOUSEMOTION:
		case SDL_MOUSEBUTTONDOWN:
		case SDL_MOUSEBUTTONUP:
		case SDL_MOUSEWHEEL:
			HandleEventMouse(event);
			break;
		case SDL_DROPFILE:
		case SDL_DROPTEXT:
		case SDL_DROPBEGIN:
		case SDL_DROPCOMPLETE:
			HandleEventDrop(event);
			break;
		default:
			break;
		}
	}

	void WindowImpl::HandleEventWindow(const SDL_Event& event)
	{
	}

	void WindowImpl::HandleEventKey(const SDL_Event& event)
	{
	}

	void WindowImpl::HandleEventTextEditing(const SDL_Event& event)
	{
	}

	void WindowImpl::HandleEventTextInput(const SDL_Event& event)
	{
	}

	void WindowImpl::HandleEventMouse(const SDL_Event& event)
	{
	}

	void WindowImpl::HandleEventDrop(const SDL_Event& event)
	{
	}

	Window::Window(const char* title, i32 x, i32 y, i32 w, i32 h, bool visible)
	{
		DBG_ASSERT(title);
		DBG_ASSERT(w >= 0);
		DBG_ASSERT(h >= 0)

		u32 flags = 0;
		if(visible)
			flags |= SDL_WINDOW_SHOWN;
		else
			flags |= SDL_WINDOW_HIDDEN;

		impl_ = new WindowImpl();
		impl_->sdlWindow_ = SDL_CreateWindow(title, x, y, w, h, flags);
		SDL_SetWindowData(impl_->sdlWindow_, "owner", this);

		RegisterWindow(impl_);
	}

	Window::~Window()
	{
		DeregisterWindow(impl_);
		SDL_DestroyWindow(impl_->sdlWindow_);
		delete impl_;
	}

	void Window::SetVisible(bool isVisible)
	{
		if(isVisible)
			SDL_ShowWindow(impl_->sdlWindow_);
		else
			SDL_HideWindow(impl_->sdlWindow_);
	}

	void Window::SetPosition(i32 x, i32 y)
	{ //
		SDL_SetWindowPosition(impl_->sdlWindow_, (int)x, (int)y);
	}

	void Window::GetPosition(i32& x, i32& y)
	{
		int ix, iy;
		SDL_GetWindowPosition(impl_->sdlWindow_, &ix, &iy);
		x = (i32)ix;
		y = (i32)iy;
	}

	void Window::SetSize(i32 w, i32 h)
	{
		DBG_ASSERT(w >= 0);
		DBG_ASSERT(h >= 0)
		SDL_SetWindowSize(impl_->sdlWindow_, (int)w, (int)h);
	}

	void Window::GetSize(i32& w, i32& h)
	{
		int iw, ih;
		SDL_GetWindowSize(impl_->sdlWindow_, &iw, &ih);
		w = (i32)iw;
		h = (i32)ih;
	}

	WindowPlatformData Window::GetPlatformData()
	{
		WindowPlatformData data = {};
		SDL_SysWMinfo wmInfo = {};
		SDL_GetWindowWMInfo(impl_->sdlWindow_, &wmInfo);

#if PLATFORM_WINDOWS
		data.handle_ = (void*)wmInfo.info.win.window;
#elif PLATFORM_LINUX
		data.handle_ = (void*)wmInfo.info.x11.window;
#elif PLATFORM_OSX
		data.handle_ = (void*)wmInfo.info.cocoa.window;
#elif PLATFORM_ANDROID
		data.handle_ = (void*)wmInfo.info.android.window;
#else
#error "Platform not supported."
#endif
		return data;
	}
} // namespace Client
