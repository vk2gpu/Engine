#pragma once

#include "core/types.h"
#include "client/dll.h"

namespace Client
{
	class IInputProvider;

	using WindowPlatformHandle = void*;
	struct WindowPlatformData
	{
		void* handle_ = nullptr;
	};

	class CLIENT_DLL Window final
	{
	public:
		/**
		 * Create new window.
		 * @param title Title text of window.
		 * @param x x position of window.
		 * @param y y position of window.
		 * @param w width of window.
		 * @param h height of window.
		 * @param visible Should show window.
		 * @pre title != nullptr.
		 * @pre w >= 0.
		 * @pre h >= 0.
		 * @return New window.
		 */
		Window(const char* title, i32 x, i32 y, i32 w, i32 h, bool visible = true);

		~Window();

		/**
		 * Show/hide window..
		 */
		void SetVisible(bool isVisible);

		/**
		 * Set window position.
		 */
		void SetPosition(i32 x, i32 y);

		/**
		 * Get window position.
		 */
		void GetPosition(i32& x, i32& y);

		/**
		 * Set window size.
		 * @pre w >= 0.
		 * @pre h >= 0.
		 */
		void SetSize(i32 w, i32 h);

		/**
		 * Get window size.
		 */
		void GetSize(i32& w, i32& h) const;

		/**
		 * Get platform data.
		 * - Windows: HWND.
		 */
		WindowPlatformData GetPlatformData();

		/**
		 * Get input provider.
		 */
		const IInputProvider& GetInputProvider();

		/**
		 * Bool operator for validity checking.
		 */
		operator bool() const { return !!impl_; }

	private:
		Window(const Window&) = delete;
		class WindowImpl* impl_ = nullptr;
	};
} // namespace Client
