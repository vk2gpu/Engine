#pragma once

#include "client/dll.h"

namespace Client
{
	class CLIENT_DLL Manager final
	{
	public:
		/**
		 * Initialize client.
		 */
		static void Initialize();

		/**
		 * Finalize client.
		 */
		static void Finalize();

		/**
		 * Is client initialized?
		 */
		static bool IsInitialized();

		/**
		 * Update.
		 * Will ensure all client systems are updated (input, messages, etc).
		 * @return false if we should exit.
		 */
		static bool Update();

		/**
		 * Pump messages.
		 * Only call manually when needed by the OS, for example
		 * on Windows: when working with DXGI from another thread.
		 * @return false if we should exit.
		 */
		static bool PumpMessages();

		/**
		 * Scoped manager init/fini.
		 * Mostly a convenience for unit tests.
		 */
		class Scoped
		{
		public:
			Scoped() { Initialize(); }
			~Scoped() { Finalize(); }
		};

	private:
		Manager() = delete;
		~Manager() = delete;
		Manager(const Manager&) = delete;
	};

} // namespace Client
