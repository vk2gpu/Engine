#pragma once

#include "imgui/dll.h"
#include "imgui/private/imgui-master/imgui.h"
#include "gpu/resources.h"
#include "gpu/types.h"

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
	/**
	 * Draw call data for passing to callbacks.
	 */
	struct DrawCallData
	{
		/// Draw binding set that ImGui is using.
		GPU::Handle dbs_;
		/// Frame binding set.
		GPU::Handle fbs_;
		/// Draw state.
		GPU::DrawState ds_;
		/// Index offset for draw.
		i32 indexOffset_;
		/// Element count for draw.
		i32 elemCount_;
	};

	/**
	 * Draw callback for user rendering.
	 */
	using DrawCallback = void (*)(GPU::CommandList&, const DrawCallData& drawState, void* userData);

	/**
	 * ImGui manager.
	 * Encapsulates all update/draw logic.
	 */
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
		static void BeginFrame(const Client::IInputProvider& input, i32 w, i32 h, f32 tick);

		/**
		 * End ImGui frame.
		 */
		static void EndFrame();

		/**
		 * Render ImGui frame.
		 */
		static void Render(const GPU::Handle& fbs, GPU::CommandList& cmdList);

		/**
		 * Setup texture override.
		 * This will call a user function to perform the draw, since more than
		 * simply texture state may need to be setup.
		 */
		static ImTextureID AddTextureOverride(DrawCallback bc, void* userData);

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
