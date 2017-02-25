#pragma once

#include "core/concurrency.h"

namespace Job
{
	class Worker final
	{
	public:
		Worker();
		~Worker();

	private:
		static i32 ThreadEntryPoint(void* param);

		Core::Thread thread_;
	};
}
