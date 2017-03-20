#pragma once

#include "client/client.h"
#include "client/private/window_impl.h"
#include "core/concurrency.h"
#include "core/vector.h"

namespace Client
{
	struct ClientImpl
	{
		Core::Mutex resourceMutex_;
		Core::Vector<WindowImpl*> windows_;
	};

	void RegisterWindow(WindowImpl* window);
	void DeregisterWindow(WindowImpl* window);
	void HandleEvent(const SDL_Event& event);

} // namespace Client
