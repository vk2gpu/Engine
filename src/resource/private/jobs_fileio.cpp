#include "resource/private/jobs_fileio.h"
#include "core/concurrency.h"
#include "core/debug.h"
#include "core/misc.h"

namespace Resource
{
	Result FileIOJob::DoRead()
	{
		DBG_ASSERT(file_);
		DBG_ASSERT((offset_ + size_) <= file_->Size());

		if(result_)
		{
			auto oldResult = (Result)Core::AtomicExchg((volatile i32*)&result_->result_, (i32)Result::RUNNING);
			DBG_ASSERT(oldResult == Result::PENDING);
		}

		file_->Seek(offset_);

		// Read file in chunks.
		char* dest = (char*)addr_;
		i64 sizeRemaining = size_;
		while(sizeRemaining > 0)
		{
			i64 readSize = Core::Min(READ_CHUNK_SIZE, sizeRemaining);
			i64 bytesRead = file_->Read(dest, readSize);
			sizeRemaining -= readSize;
			dest += readSize;
			if(result_)
			{
				Core::AtomicAddRel(&result_->workRemaining_, -bytesRead);
			}

			// Check that we successfully read.
			if(bytesRead < readSize)
				break;
		}

		Result result = Result::FAILURE;
		if(sizeRemaining == 0)
			result = Result::SUCCESS;
		if(result_)
		{
			Core::AtomicExchg((volatile i32*)&result_->result_, (i32)result);
		}
		return result;
	}

	Result FileIOJob::DoWrite()
	{
		DBG_ASSERT(file_);
		DBG_ASSERT(offset_ == 0);

		if(result_)
		{
			auto oldResult = (Result)Core::AtomicExchg((volatile i32*)&result_->result_, (i32)Result::RUNNING);
			DBG_ASSERT(oldResult == Result::PENDING);
		}

		// Write file in chunks.
		char* src = (char*)addr_;
		i64 sizeRemaining = size_;
		while(sizeRemaining > 0)
		{
			i64 writeSize = Core::Min(WRITE_CHUNK_SIZE, sizeRemaining);
			i64 bytesWrote = file_->Write(src, writeSize);
			sizeRemaining -= writeSize;
			src += writeSize;
			if(result_)
			{
				Core::AtomicAddRel(&result_->workRemaining_, -bytesWrote);
			}

			// Check that we successfully read.
			if(bytesWrote < writeSize)
				break;
		}

		Result result = Result::FAILURE;
		if(sizeRemaining == 0)
			result = Result::SUCCESS;
		if(result_)
		{
			Core::AtomicExchg((volatile i32*)&result_->result_, (i32)result);
		}
		return result;
	}

} // namespace Resource
