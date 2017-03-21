#pragma once

#include "core/types.h"
#include "math/vec2.h"

namespace Client
{
	class IInputProvider
	{
	public:
		IInputProvider() = default;
		virtual ~IInputProvider() = default;

		/**
		 * Is key currently down?
		 * @param Key code.
		 */
		virtual bool IsKeyDown(i32 keyCode) const { return false; }

		/**
		 * Is key currently up?
		 * @param Key code.
		 */
		virtual bool IsKeyUp(i32 keyCode) const { return false; }

		/**
		 * Was key pressed?
		 * @param Key code.
		 */
		virtual bool WasKeyPressed(i32 keyCode) const { return false; }

		/**
		 * Was key released?
		 * @param Key code.
		 */
		virtual bool WasKeyReleased(i32 keyCode) const { return false; }

		/**
		 * Get text input.
		 * @param outBuffer Buffer to write text to in UTF-8.
		 * @param bytes Number of bytes in buffer.
		 * @return Number of bytes written.
		 */
		virtual i32 GetTextInput(char* outBuffer, i32 bytes) const { return 0; }

		/**
		 * Get mouse position.
		 */
		virtual Math::Vec2 GetMousePosition() const { return Math::Vec2(0.0f, 0.0f); }

		/**
		 * Get mouse wheel delta.
		 */
		virtual Math::Vec2 GetMouseWheelDelta() const { return Math::Vec2(0.0f, 0.0f); }

		/**
		 * Is mouse button down?
		 */
		virtual bool IsMouseButtonDown(i32 buttonIdx) const { return false; }

		/**
		 * Is mouse button up?
		 */
		virtual bool IsMouseButtonUp(i32 buttonIdx) const { return false; }

		/**
		 * Was mouse button pressed?
		 */
		virtual bool WasMouseButtonPressed(i32 buttonIdx) const { return false; }

		/**
		 * Was mouse button released?
		 */
		virtual bool WasMouseButtonReleased(i32 buttonIdx) const { return false; }

		/// Templated aliases for keyboard input to allow enums for appropriate devices.
		template<typename TYPE>
		bool IsKeyDown(TYPE keyCode) const
		{
			return IsKeyDown((i32)keyCode);
		}

		template<typename TYPE>
		bool IsKeyUp(TYPE keyCode) const
		{
			return IsKeyUp((i32)keyCode);
		}

		template<typename TYPE>
		bool IsKeyPressed(TYPE keyCode) const
		{
			return IsKeyPressed((i32)keyCode);
		}

		template<typename TYPE>
		bool IsKeyReleased(TYPE keyCode) const
		{
			return IsKeyReleased((i32)keyCode);
		}
	};


} // namespace Client
