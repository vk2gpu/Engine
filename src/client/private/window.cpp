#include "client/window.h"
#include "core/debug.h"

#include <SDL.h>
#include <SDL_syswm.h>

namespace Client
{
	struct WindowImpl
	{
		SDL_Window* window_ = nullptr;
	};

	Window::Window(const char* title, i32 x, i32 y, i32 w, i32 h)
	{
		DBG_ASSERT(title);
		DBG_ASSERT(w >= 0);
		DBG_ASSERT(h >= 0)

		impl_ = new WindowImpl();
		impl_->window_ = SDL_CreateWindow(title, x, y, w, h, 0);
		SDL_SetWindowData(impl_->window_, "owner", this);
	}

	Window::~Window()
	{
		SDL_DestroyWindow(impl_->window_);
		delete impl_;
	}

	void Window::SetPosition(i32 x, i32 y)
	{ //
		SDL_SetWindowPosition(impl_->window_, (int)x, (int)y);
	}

	void Window::GetPosition(i32& x, i32& y)
	{
		int ix, iy;
		SDL_GetWindowPosition(impl_->window_, &ix, &iy);
		x = (i32)ix;
		y = (i32)iy;
	}

	void Window::SetSize(i32 w, i32 h)
	{
		DBG_ASSERT(w >= 0);
		DBG_ASSERT(h >= 0)
		SDL_SetWindowSize(impl_->window_, (int)w, (int)h);
	}

	void Window::GetSize(i32& w, i32& h)
	{
		int iw, ih;
		SDL_GetWindowSize(impl_->window_, &iw, &ih);
		w = (i32)iw;
		h = (i32)ih;
	}

	WindowPlatformData Window::GetPlatformData()
	{
		WindowPlatformData data = {};
		SDL_SysWMinfo wmInfo = {};
		SDL_GetWindowWMInfo(impl_->window_, &wmInfo);

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
