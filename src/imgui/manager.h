#pragma once

#include "imgui/dll.h"
#include "imgui/private/imgui-master/imgui.h"

namespace Client
{
	class IInputProvider;
} // namespace Client

namespace GPU
{
	class CommandList;
	class Manager;
	class Handle;
} // namespace GPU

namespace ImGui
{
	class IMGUI_DLL Manager final
	{
	public:
		/**
		 * Initialize ImGui.
		 */
		static void Initialize();

		/**
		 * Finalize ImGui.
		 */
		static void Finalize();

		/**
		 * Is ImGui initialized?
		 */
		static bool IsInitialized();

		/**
		 * Begin ImGui frame.
 		 */
		static void BeginFrame(const Client::IInputProvider& input, i32 w, i32 h);

		/**
		 * End ImGui frame.
		 */
		static void EndFrame(const GPU::Handle& fbs, GPU::CommandList& cmdList);

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
	

} // namespace ImGui
