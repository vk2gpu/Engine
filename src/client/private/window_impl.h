#pragma once

#include "core/types.h"
#include "core/array.h"
#include "core/map.h"
#include "core/misc.h"
#include "client/input_provider.h"
#include "client/key_input.h"
#include <SDL_events.h>

namespace Client
{
	class WindowImpl : public IInputProvider
	{
	public:
		SDL_Window* sdlWindow_ = nullptr;

		struct InputState
		{
			Core::Array<bool, 512> keyCodeStates_;
			Core::Array<bool, 512> scanCodeStates_;
			Math::Vec2 mousePosition_;
			Core::Array<bool, 8> mouseButtonStates_;
			Core::Vector<char> textInput_;
			bool GetKeyState(i32 keyCode) const
			{
				i32 maskedKeyCode = keyCode & ~SCAN_CODE_BIT;
				if(!Core::ContainsAllFlags(keyCode, SCAN_CODE_BIT))
				{
					DBG_ASSERT(maskedKeyCode < keyCodeStates_.size());
					return keyCodeStates_[maskedKeyCode];
				}
				else
				{
					DBG_ASSERT(maskedKeyCode < scanCodeStates_.size());
					return scanCodeStates_[maskedKeyCode & 0x1ff];
				}
			}

			void SetKeyState(i32 keyCode, bool state)
			{
				i32 maskedKeyCode = keyCode & ~SCAN_CODE_BIT;
				if(!Core::ContainsAllFlags(keyCode, SCAN_CODE_BIT))
				{

					DBG_ASSERT(maskedKeyCode < keyCodeStates_.size());
					keyCodeStates_[maskedKeyCode] = state;
				}
				else
				{
					DBG_ASSERT(maskedKeyCode < scanCodeStates_.size());
					scanCodeStates_[maskedKeyCode] = state;
				}
			}
		};

		InputState prevInputState_;
		InputState currInputState_;

		void UpdateInputState();

		void HandleEvent(const SDL_Event& event);
		void HandleEventWindow(const SDL_Event& event);
		void HandleEventKey(const SDL_Event& event);
		void HandleEventTextEditing(const SDL_Event& event);
		void HandleEventTextInput(const SDL_Event& event);
		void HandleEventMouse(const SDL_Event& event);
		void HandleEventDrop(const SDL_Event& event);

		/// IInputProvider.
		bool IsKeyDown(i32 keyCode) const override;
		bool IsKeyUp(i32 keyCode) const override;
		bool WasKeyPressed(i32 keyCode) const override;
		bool WasKeyReleased(i32 keyCode) const override;
		i32 GetTextInput(char* outBuffer, i32 bytes) const override;
		Math::Vec2 GetMousePosition() const override;
		bool IsMouseButtonDown(i32 buttonIdx) const override;
		bool IsMouseButtonUp(i32 buttonIdx) const override;
		bool WasMouseButtonPressed(i32 buttonIdx) const override;
		bool WasMouseButtonReleased(i32 buttonIdx) const override;
	};

} // namespace Client

