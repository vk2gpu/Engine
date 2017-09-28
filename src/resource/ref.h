#pragma once

#include "resource/dll.h"

namespace Core
{
	class UUID;

} // namespace Core

namespace Resource
{
	/**
	 * Base resource reference for automatic handle of requesting & releasing.
	 */
	class RESOURCE_DLL RefBase
	{
	public:
		RefBase();
		RefBase(const char* name, const Core::UUID& type);
		~RefBase();

		RefBase(RefBase&&);
		RefBase& operator=(RefBase&&);

		void Reset();
		bool IsReady() const;
		void WaitUntilReady() const;
		operator bool() const { return !!resource_; }

	protected:
		RefBase(const RefBase&) = delete;
		RefBase& operator=(const RefBase&) = delete;

		void* resource_ = nullptr;
	};

	/**
	 * Typed resource reference for automatic handle of requesting & releasing.
	 */
	template<typename TYPE>
	class Ref : public RefBase
	{
	public:
		Ref() = default;
		Ref(const char* name)
		    : RefBase(name, TYPE::GetTypeUUID())
		{
		}

		Ref(Ref&&) = default;
		Ref& operator=(Ref&&) = default;

		operator const TYPE*() const { return reinterpret_cast<const TYPE*>(resource_); }
		operator TYPE*() { return reinterpret_cast<TYPE*>(resource_); }

		const TYPE* operator->() const { return reinterpret_cast<const TYPE*>(resource_); }
		TYPE* operator->() { return reinterpret_cast<TYPE*>(resource_); }

	private:
		Ref(const Ref&) = delete;
		Ref& operator=(const Ref&) = delete;
	};
} // namespace Resource
