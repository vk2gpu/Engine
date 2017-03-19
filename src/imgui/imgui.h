#pragma once

#include "imgui/dll.h"
#include "imgui/private/imgui-master/imgui.h"

namespace GPU
{
	class CommandList;
	class Manager;
	class Handle;
}

namespace ImGui
{
	/**
	 * Initialize ImGui.
	 * @param gpuManager GPU manager to use for setup.
	 */
	IMGUI_DLL void Initialize(GPU::Manager& gpuManager);

	/**
	 * Finalize ImGui.
	 */
	IMGUI_DLL void Finalize();

	/**
	 * Begin ImGui frame.
	 */
	IMGUI_DLL void BeginFrame(i32 w, i32 h);

	/**
	 * End ImGui frame.
	 */
	IMGUI_DLL void EndFrame(const GPU::Handle& fbs, GPU::CommandList& cmdList);

} // namespace ImGui
