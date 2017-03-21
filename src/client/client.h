#pragma once

#include "client/dll.h"

namespace Client
{
	/**
	 * Initialize client.
	 */
	CLIENT_DLL void Initialize();

	/**
	 * Finalize client.
	 */
	CLIENT_DLL void Finalize();

	/**
	 * Update.
	 * Will ensure all client systems are updated (input, messages, etc).
	 * @return false if we should exit.
	 */
	CLIENT_DLL bool Update();

	/**
	 * Pump messages.
	 * Only call manually when needed by the OS, for example
	 * on Windows: when working with DXGI from another thread.
	 * @return false if we should exit.
	 */
	CLIENT_DLL bool PumpMessages();

} // namespace Client
