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
#endif

struct ThreadImpl
{
	DWORD threadId_ = 0;
	HANDLE threadHandle_ = 0;
	Thread::EntryPointFunc entryPointFunc_;
	void* userData_ = nullptr;
};

static DWORD WINAPI ThreadEntryPoint(LPVOID lpThreadParameter)
{
	auto* impl = reinterpret_cast<ThreadImpl*>(lpThreadParameter);
	return impl->entryPointFunc_(impl->userData_);
}

Thread::Thread(EntryPointFunc entryPointFunc, void* userData, i32 stackSize)
{
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


struct EventImpl
{
	HANDLE handle_;
};

Event::Event(bool manualReset, bool initialState)
{
	impl_ = new EventImpl();
	impl_->handle_ = ::CreateEvent(nullptr, manualReset ? TRUE : FALSE, initialState ? TRUE : FALSE, "<debug name>");
}

Event::~Event()
{
	::CloseHandle(impl_->handle_);
	delete impl_;
}

bool Event::Wait(i32 timeout)
{
	return (::WaitForSingleObject(impl_->handle_, timeout) == WAIT_OBJECT_0);
}

bool Event::Signal()
{
	return !!::SetEvent(impl_->handle_);
}

bool Event::Reset()
{
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
	::EnterCriticalSection(&impl_->critSec_);
}

bool Mutex::TryLock()
{
	return !!::TryEnterCriticalSection(&impl_->critSec_);
}

void Mutex::Unlock()
{
	::LeaveCriticalSection(&impl_->critSec_);
}
