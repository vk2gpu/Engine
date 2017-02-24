#pragma once

#include "core/types.h"
#include "client/dll.h"

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
	 * @pre title != nullptr.
	 * @pre w >= 0.
	 * @pre h >= 0.
	 * @return New window.
	 */
	Window(const char* title, i32 x, i32 y, i32 w, i32 h);

	~Window();

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
	void GetSize(i32& w, i32& h);

	/**
	 * Get platform data.
	 * - Windows: HWND.
	 */
	WindowPlatformData GetPlatformData();

private:
	Window(const Window&) = delete;
	struct WindowImpl* impl_ = nullptr;
};
