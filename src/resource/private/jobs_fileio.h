#include "core/file.h"
#include "resource/types.h"

namespace Resource
{
	/// File IO Job. Can be read or write.
	struct FileIOJob
	{
		static const i64 READ_CHUNK_SIZE = 8 * 1024 * 1024;  // TODO: Possibly have this configurable?
		static const i64 WRITE_CHUNK_SIZE = 8 * 1024 * 1024; // TODO: Possibly have this configurable?

		Core::File* file_ = nullptr;
		i64 offset_ = 0;
		i64 size_ = 0;
		void* addr_ = nullptr;
		AsyncResult* result_ = nullptr;

		Result DoRead();
		Result DoWrite();
	};

} // namespace Resource
