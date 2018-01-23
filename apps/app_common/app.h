#pragma once

#include "dll.h"
#include "common.h"
#include "core/command_line.h"
#include "client/input_provider.h"
#include "client/window.h"

#include "render_packets.h"
#include "graphics/pipeline.h"

class APP_COMMON_DLL IApp
{
public:
	virtual const char* GetName() const = 0;
	virtual void Initialize() = 0;
	virtual void Shutdown() = 0;

	virtual void Update(const Client::IInputProvider& input, const Client::Window& window, f32 tick) = 0;
	virtual void UpdateGUI() = 0;
	virtual void PreRender(Graphics::Pipeline& pipeline) = 0;
	virtual void Render(Graphics::Pipeline& pipeline, Core::Vector<RenderPacketBase*>& outPackets) = 0;
};

APP_COMMON_DLL void RunApp(const Core::CommandLine& cmdLine, IApp& app);
