#pragma once

#include "core/file.h"

namespace Core
{
	/**
	 * Base file implementation interface.
	 * @see Core::File.
	 */
	class FileImpl
	{
	public:
		virtual ~FileImpl() {}
		virtual i64 Read(void* buffer, i64 bytes) = 0;
		virtual i64 Write(const void* buffer, i64 bytes) = 0;
		virtual bool Seek(i64 offset) = 0;
		virtual i64 Tell() const = 0;
		virtual i64 Size() const = 0;
		virtual FileFlags GetFlags() const = 0;
		virtual bool IsValid() const = 0;
		virtual const char* GetPath() const = 0;
	};

} // namespace Core