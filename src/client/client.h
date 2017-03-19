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
	 * Pump messages.
	 * @return false if we should exit.
	 */
	CLIENT_DLL bool PumpMessages();

} // namespace Client
