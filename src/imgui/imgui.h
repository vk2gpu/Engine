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
	/**
	 * Initialize ImGui.
	 */
	IMGUI_DLL void Initialize();

	/**
	 * Finalize ImGui.
	 */
	IMGUI_DLL void Finalize();

	/**
	 * Begin ImGui frame.
	 */
	IMGUI_DLL void BeginFrame(const Client::IInputProvider& input, i32 w, i32 h);

	/**
	 * End ImGui frame.
	 */
	IMGUI_DLL void EndFrame(const GPU::Handle& fbs, GPU::CommandList& cmdList);

} // namespace ImGui
