#include "client/window.h"
#include "client/private/manager_impl.h"
#include "client/key_input.h"
#include "core/debug.h"
#include "core/misc.h"

#include <SDL.h>
#include <SDL_syswm.h>

#include <utility>

namespace Client
{
	WindowImpl::WindowImpl()
	{
		prevInputState_.keyCodeStates_.fill(false);
		prevInputState_.scanCodeStates_.fill(false);
		prevInputState_.mouseButtonStates_.fill(false);
		currInputState_ = prevInputState_;
	}

	void WindowImpl::UpdateInputState()
	{
		using std::swap;
		prevInputState_.keyCodeStates_ = currInputState_.keyCodeStates_;
		prevInputState_.scanCodeStates_ = currInputState_.scanCodeStates_;
		prevInputState_.mousePosition_ = currInputState_.mousePosition_;
		prevInputState_.mouseButtonStates_ = currInputState_.mouseButtonStates_;
		swap(prevInputState_.textInput_, currInputState_.textInput_);
		currInputState_.textInput_.clear();
		currInputState_.mouseWheelDelta_ = Math::Vec2(0.0f, 0.0f);
	}

	void WindowImpl::HandleEvent(const SDL_Event& event)
	{
		switch(event.type)
		{
		case SDL_WINDOWEVENT:
			HandleEventWindow(event);
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
	{ //
	}

	void WindowImpl::HandleEventKey(const SDL_Event& event)
	{
		bool state = event.key.type == SDL_KEYDOWN ? true : false;
		currInputState_.SetKeyState(event.key.keysym.sym, state);
	}

	void WindowImpl::HandleEventTextEditing(const SDL_Event& event)
	{ //
	}

	void WindowImpl::HandleEventTextInput(const SDL_Event& event)
	{ //
		auto& textInput = currInputState_.textInput_;

		// Pop off null terminator to append.
		if(textInput.size() > 0)
		{
			if(textInput.back() == '\0')
				textInput.pop_back();
		}

		// Add all text input.
		for(i32 i = 0; i < SDL_TEXTINPUTEVENT_TEXT_SIZE; ++i)
		{
			char text = event.text.text[i];
			if(text != '\0')
				textInput.push_back(text);
		}
		textInput.push_back('\0');
	}

	void WindowImpl::HandleEventMouse(const SDL_Event& event)
	{
		static const i32 BUTTON_MAPPING[8] = {
		    -1,
		    0,
		    2,
		    1,
		    3,
		    4,
		    5,
		    6,
		};
		if(event.button.button < 0 || event.button.button > 7)
			return;

		switch(event.type)
		{
		case SDL_MOUSEMOTION:
		{
			currInputState_.mousePosition_.x = (f32)event.motion.x;
			currInputState_.mousePosition_.y = (f32)event.motion.y;
		}
		break;
		case SDL_MOUSEBUTTONDOWN:
		case SDL_MOUSEBUTTONUP:
		{
			if(event.button.button > 0)
			{
				const i32 button = BUTTON_MAPPING[event.button.button];
				if(button != -1)
				{
					currInputState_.mouseButtonStates_[button] = event.button.state == SDL_PRESSED;
					currInputState_.mousePosition_.x = (f32)event.button.x;
					currInputState_.mousePosition_.y = (f32)event.button.y;
				}
			}
		}
		break;
		case SDL_MOUSEWHEEL:
		{
			currInputState_.mouseWheelDelta_.x = (f32)event.wheel.x;
			currInputState_.mouseWheelDelta_.y = (f32)event.wheel.y;
		}
		break;
		default:
			DBG_ASSERT(false);
		}
	}

	void WindowImpl::HandleEventDrop(const SDL_Event& event)
	{ //
	}

	bool WindowImpl::IsKeyDown(i32 keyCode) const
	{ //
		return currInputState_.GetKeyState(keyCode);
	}

	bool WindowImpl::IsKeyUp(i32 keyCode) const
	{ //
		return !currInputState_.GetKeyState(keyCode);
	}

	bool WindowImpl::WasKeyPressed(i32 keyCode) const
	{
		return !prevInputState_.GetKeyState(keyCode) && currInputState_.GetKeyState(keyCode);
	}

	bool WindowImpl::WasKeyReleased(i32 keyCode) const
	{
		return prevInputState_.GetKeyState(keyCode) && !currInputState_.GetKeyState(keyCode);
	}

	i32 WindowImpl::GetTextInput(char* outBuffer, i32 bytes) const
	{
		if(outBuffer == nullptr)
		{
			return currInputState_.textInput_.size();
		}

		if(currInputState_.textInput_.size() > 0)
		{
			strcpy_s(outBuffer, bytes, currInputState_.textInput_.data());
			return (i32)Core::Min(strlen(currInputState_.textInput_.data()), bytes);
		}
		return 0;
	}

	Math::Vec2 WindowImpl::GetMousePosition() const
	{ //
		return currInputState_.mousePosition_;
	}

	Math::Vec2 WindowImpl::GetMouseWheelDelta() const
	{ //
		return currInputState_.mouseWheelDelta_;
	}

	bool WindowImpl::IsMouseButtonDown(i32 buttonIdx) const
	{ //
		return currInputState_.mouseButtonStates_[buttonIdx];
	}

	bool WindowImpl::IsMouseButtonUp(i32 buttonIdx) const
	{ //
		return !currInputState_.mouseButtonStates_[buttonIdx];
	}

	bool WindowImpl::WasMouseButtonPressed(i32 buttonIdx) const
	{ //
		return !currInputState_.mouseButtonStates_[buttonIdx] && currInputState_.mouseButtonStates_[buttonIdx];
	}

	bool WindowImpl::WasMouseButtonReleased(i32 buttonIdx) const
	{ //
		return currInputState_.mouseButtonStates_[buttonIdx] && !currInputState_.mouseButtonStates_[buttonIdx];
	}

	Window::Window(const char* title, i32 x, i32 y, i32 w, i32 h, bool visible, bool resizable)
	{
		DBG_ASSERT(title);
		DBG_ASSERT(w >= 0);
		DBG_ASSERT(h >= 0)

		u32 flags = 0;
		if(visible)
			flags |= SDL_WINDOW_SHOWN;
		else
			flags |= SDL_WINDOW_HIDDEN;

		if(resizable)
			flags |= SDL_WINDOW_RESIZABLE;

		impl_ = new WindowImpl();
		impl_->sdlWindow_ = SDL_CreateWindow(title, x, y, w, h, flags);
		if(impl_->sdlWindow_)
		{
			SDL_SetWindowData(impl_->sdlWindow_, "owner", this);
			RegisterWindow(impl_);
		}
		else
		{
			const char* sdlError = SDL_GetError();
			DBG_LOG("Client::Window: Failed to create window. SDL error: \"%s\"", sdlError);
			delete impl_;
			impl_ = nullptr;
		}
	}

	Window::~Window()
	{
		DeregisterWindow(impl_);
		SDL_DestroyWindow(impl_->sdlWindow_);
		delete impl_;
	}

	void Window::SetVisible(bool isVisible)
	{
		DBG_ASSERT(impl_);
		if(isVisible)
			SDL_ShowWindow(impl_->sdlWindow_);
		else
			SDL_HideWindow(impl_->sdlWindow_);
	}

	void Window::SetPosition(i32 x, i32 y)
	{
		DBG_ASSERT(impl_);
		SDL_SetWindowPosition(impl_->sdlWindow_, (int)x, (int)y);
	}

	void Window::GetPosition(i32& x, i32& y)
	{
		DBG_ASSERT(impl_);
		int ix = 0, iy = 0;
		SDL_GetWindowPosition(impl_->sdlWindow_, &ix, &iy);
		x = (i32)ix;
		y = (i32)iy;
	}

	void Window::SetSize(i32 w, i32 h)
	{
		DBG_ASSERT(impl_);
		DBG_ASSERT(w >= 0);
		DBG_ASSERT(h >= 0)
		SDL_SetWindowSize(impl_->sdlWindow_, (int)w, (int)h);
	}

	void Window::GetSize(i32& w, i32& h) const
	{
		DBG_ASSERT(impl_);
		int iw = 0, ih = 0;
		SDL_GetWindowSize(impl_->sdlWindow_, &iw, &ih);
		w = (i32)iw;
		h = (i32)ih;
	}

	WindowPlatformData Window::GetPlatformData()
	{
		DBG_ASSERT(impl_);
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

	const IInputProvider& Window::GetInputProvider()
	{
		DBG_ASSERT(impl_);
		return *impl_;
	}

} // namespace Client
