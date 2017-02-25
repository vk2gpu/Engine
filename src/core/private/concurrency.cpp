#pragma once

#include "core/concurrency.h"
#include "core/debug.h"

#include <utility>

#if !CODE_INLINE
#include "core/private/concurrency.inl"
#endif

#if PLATFORM_WINDOWS
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

namespace Core
{
	struct ThreadImpl
	{
		DWORD threadId_ = 0;
		HANDLE threadHandle_ = 0;
		Thread::EntryPointFunc entryPointFunc_ = nullptr;
		void* userData_ = nullptr;
	};

	static DWORD WINAPI ThreadEntryPoint(LPVOID lpThreadParameter)
	{
		auto* impl = reinterpret_cast<ThreadImpl*>(lpThreadParameter);
		return impl->entryPointFunc_(impl->userData_);
	}

	Thread::Thread(EntryPointFunc entryPointFunc, void* userData, i32 stackSize)
	{
		DBG_ASSERT(entryPointFunc);
		DWORD creationFlags = 0;
		impl_ = new ThreadImpl();
		impl_->entryPointFunc_ = entryPointFunc;
		impl_->userData_ = userData;
		impl_->threadHandle_ =
		    ::CreateThread(nullptr, stackSize, ThreadEntryPoint, impl_, creationFlags, &impl_->threadId_);
	}

	Thread::~Thread()
	{
		if(impl_)
		{
			Join();
		}
	}

	Thread::Thread(Thread&& other)
	{
		using std::swap;
		swap(impl_, other.impl_);
	}

	Thread& Thread::operator=(Thread&& other)
	{
		using std::swap;
		swap(impl_, other.impl_);
		return *this;
	}

	i32 Thread::Join()
	{
		DBG_ASSERT(impl_);
		DBG_ASSERT(impl_->threadHandle_);
		if(impl_)
		{
			::WaitForSingleObject(impl_->threadHandle_, INFINITE);
			DWORD exitCode = 0;
			BOOL success = ::GetExitCodeThread(impl_->threadHandle_, &exitCode);
			DBG_ASSERT(success);
			::CloseHandle(impl_->threadHandle_);
			delete impl_;
			impl_ = nullptr;
			return exitCode;
		}
		return 0;
	}


	struct FiberImpl
	{
		void* callingFiber_ = nullptr;
		void* fiber_ = nullptr;
		Fiber::EntryPointFunc entryPointFunc_ = nullptr;
		void* userData_ = nullptr;
	};

	static void __stdcall FiberEntryPoint(LPVOID lpParameter)
	{
		auto* impl = reinterpret_cast<FiberImpl*>(lpParameter);
		impl->entryPointFunc_(impl->userData_);
		auto callingFiber = impl->callingFiber_;
		impl->callingFiber_ = nullptr;
		::SwitchToFiber(callingFiber);
	}

	Fiber::Fiber(EntryPointFunc entryPointFunc, void* userData, i32 stackSize)
	{
		DBG_ASSERT(entryPointFunc);
		impl_ = new FiberImpl();
		impl_->entryPointFunc_ = entryPointFunc;
		impl_->userData_ = userData;
		impl_->fiber_ = ::CreateFiber(stackSize, FiberEntryPoint, impl_);
		DBG_ASSERT_MSG(impl_->fiber_, "Unable to create fiber.");
		if(impl_->fiber_ == nullptr)
		{
			delete impl_;
			impl_ = nullptr;
		}
	}

	Fiber::Fiber(ThisThread)
	{
		impl_ = new FiberImpl();
		impl_->entryPointFunc_ = nullptr;
		impl_->userData_ = nullptr;
		impl_->fiber_ = ::ConvertThreadToFiber(impl_);
		DBG_ASSERT_MSG(impl_->fiber_, "Unable to create fiber. Is there already one for this thread?");
		if(impl_->fiber_ == nullptr)
		{
			delete impl_;
			impl_ = nullptr;
		}
	}

	Fiber::~Fiber()
	{
		if(impl_)
		{
			DBG_ASSERT_MSG(impl_->callingFiber_ == nullptr, "Destructing whilst still active.");
			if(impl_->entryPointFunc_)
			{
				::DeleteFiber(impl_->fiber_);
			}
			else
			{
				::ConvertFiberToThread();
			}
			delete impl_;
		}
	}

	Fiber::Fiber(Fiber&& other)
	{
		using std::swap;
		swap(impl_, other.impl_);
	}

	Fiber& Fiber::operator=(Fiber&& other)
	{
		using std::swap;
		swap(impl_, other.impl_);
		return *this;
	}

	void Fiber::SwitchTo(Fiber& caller)
	{
		DBG_ASSERT(impl_);
		DBG_ASSERT(caller.impl_);
		if(impl_)
		{
			impl_->callingFiber_ = caller.impl_->fiber_;
			::SwitchToFiber(impl_->fiber_);
			impl_->callingFiber_ = nullptr;
		}
	}


	struct EventImpl
	{
		HANDLE handle_;
	};

	Event::Event(bool manualReset, bool initialState)
	{
		impl_ = new EventImpl();
		impl_->handle_ =
		    ::CreateEvent(nullptr, manualReset ? TRUE : FALSE, initialState ? TRUE : FALSE, "<debug name>");
	}

	Event::~Event()
	{
		::CloseHandle(impl_->handle_);
		delete impl_;
	}

	bool Event::Wait(i32 timeout)
	{
		DBG_ASSERT(impl_);
		return (::WaitForSingleObject(impl_->handle_, timeout) == WAIT_OBJECT_0);
	}

	bool Event::Signal()
	{
		DBG_ASSERT(impl_);
		return !!::SetEvent(impl_->handle_);
	}

	bool Event::Reset()
	{
		DBG_ASSERT(impl_);
		return !!::ResetEvent(impl_->handle_);
	}

	struct MutexImpl
	{
		CRITICAL_SECTION critSec_;
	};

	Mutex::Mutex()
	{
		impl_ = new MutexImpl();
		::InitializeCriticalSection(&impl_->critSec_);
	}

	Mutex::~Mutex()
	{
		::DeleteCriticalSection(&impl_->critSec_);
		delete impl_;
	}

	void Mutex::Lock()
	{
		DBG_ASSERT(impl_);
		::EnterCriticalSection(&impl_->critSec_);
	}

	bool Mutex::TryLock()
	{
		DBG_ASSERT(impl_);
		return !!::TryEnterCriticalSection(&impl_->critSec_);
	}

	void Mutex::Unlock()
	{
		DBG_ASSERT(impl_);
		::LeaveCriticalSection(&impl_->critSec_);
	}

	struct TLSImpl
	{
		DWORD handle_ = 0;
	};

	TLS::TLS()
	{ //
		impl_ = new TLSImpl();
	}

	TLS::~TLS()
	{
		::TlsFree(impl_->handle_);
		delete impl_;
	}

	bool TLS::Set(void* data)
	{
		DBG_ASSERT(impl_);
		return !!::TlsSetValue(impl_->handle_, data);
	}

	void* TLS::Get() const
	{
		DBG_ASSERT(impl_);
		return ::TlsGetValue(impl_->handle_);
	}
} // namespace Core
#else
#error "Not implemented for platform!""
#endif
