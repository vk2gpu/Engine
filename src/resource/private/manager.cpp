#pragma once

#include "resource/manager.h"

#include "core/concurrency.h"
#include "core/file.h"
#include "core/library.h"
#include "core/map.h"
#include "core/misc.h"
#include "core/mpmc_bounded_queue.h"
#include "core/uuid.h"

#include <utility>

namespace Resource
{
	struct ManagerImpl
	{
		static const i32 MAX_READ_JOBS = 128;
		static const i32 MAX_WRITE_JOBS = 128;
		static const i64 READ_CHUNK_SIZE = 8 * 1024 * 1024;  // TODO: Possibly have this configurable?
		static const i64 WRITE_CHUNK_SIZE = 8 * 1024 * 1024; // TODO: Possibly have this configurable?

		/// File IO Job. Can be read or write.
		struct FileIOJob
		{
			Core::File* file_ = nullptr;
			i64 offset_ = 0;
			i64 size_ = 0;
			void* addr_ = nullptr;
			AsyncResult* result_ = nullptr;

			Result DoRead()
			{
				DBG_ASSERT(file_);
				DBG_ASSERT((offset_ + size_) <= file_->Size());

				if(result_)
				{
					auto oldResult =
					    (Result)Core::AtomicExchg((volatile i32*)&result_->result_, (i32)Result::RUNNING);
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

			Result DoWrite()
			{
				DBG_ASSERT(file_);
				DBG_ASSERT(offset_ == 0);

				if(result_)
				{
					auto oldResult =
					    (Result)Core::AtomicExchg((volatile i32*)&result_->result_, (i32)Result::RUNNING);
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
		};

		/// Read job queue.
		Core::MPMCBoundedQueue<FileIOJob> readJobs_;
		/// Signalled when read job is waiting.
		Core::Event readJobEvent_;
		/// Thread to use for blocking reads.
		Core::Thread readThread_;

		/// Write job queue.
		Core::MPMCBoundedQueue<FileIOJob> writeJobs_;
		/// Signalled when write job is waiting.
		Core::Event writeJobEvent_;
		/// Thread to use for blocking reads.
		Core::Thread writeThread_;

		ManagerImpl()
		    : readJobs_(MAX_READ_JOBS)
		    , readJobEvent_(false, false, "Resource Manager Read Event")
		    , readThread_(ReadIOThread, this, 65536, "Resource Manager Read Thread")
		    , writeJobs_(MAX_WRITE_JOBS)
		    , writeJobEvent_(false, false, "Resource Manager Write Event")
		    , writeThread_(WriteIOThread, this, 65536, "Resource Manager Write Thread")
		{
		}

		~ManagerImpl()
		{
			// TODO: Mark jobs as cancelled.

			while(readJobs_.Enqueue(FileIOJob()) == false)
				Core::SwitchThread();
			readJobEvent_.Signal();
			readThread_.Join();

			while(writeJobs_.Enqueue(FileIOJob()) == false)
				Core::SwitchThread();
			writeJobEvent_.Signal();
			writeThread_.Join();
		}

		static int ReadIOThread(void* userData)
		{
			auto* impl = reinterpret_cast<ManagerImpl*>(userData);

			FileIOJob ioJob;
			for(;;)
			{
				if(impl->readJobs_.Dequeue(ioJob))
				{
					if(ioJob.file_ != nullptr)
						ioJob.DoRead();
					else
						return 0;
				}
				impl->readJobEvent_.Wait();
			}
		}

		static int WriteIOThread(void* userData)
		{
			auto* impl = reinterpret_cast<ManagerImpl*>(userData);

			FileIOJob ioJob;
			for(;;)
			{
				if(impl->writeJobs_.Dequeue(ioJob))
				{
					if(ioJob.file_ != nullptr)
						ioJob.DoWrite();
					else
						return 0;
				}
				impl->writeJobEvent_.Wait();
			}
		}
	};

	Manager::Manager() { impl_ = new ManagerImpl(); }

	Manager::~Manager() { delete impl_; }

	Result Manager::ReadFileData(Core::File& file, i64 offset, i64 size, void* dest, AsyncResult* result)
	{
		DBG_ASSERT(Core::ContainsAllFlags(file.GetFlags(), Core::FileFlags::READ));
		DBG_ASSERT(offset >= 0);
		DBG_ASSERT(size > 0);
		DBG_ASSERT(dest != nullptr);

		Result outResult = Result::PENDING;
		if(result)
		{
			auto oldResult = (Result)Core::AtomicExchg((volatile i32*)&result->result_, (i32)outResult);
			DBG_ASSERT(oldResult == Result::INITIAL);
		}

		ManagerImpl::FileIOJob job;
		job.file_ = &file;
		job.offset_ = offset;
		job.size_ = size;
		job.addr_ = dest;
		job.result_ = result;

		if(result)
		{
			Core::AtomicAddAcq(&result->workRemaining_, size);
			impl_->readJobs_.Enqueue(job);
			impl_->readJobEvent_.Signal();
		}
		else
		{
			outResult = job.DoRead();
		}
		return outResult;
	}

	Result Manager::WriteFileData(Core::File& file, i64 size, void* src, AsyncResult* result)
	{
		DBG_ASSERT(Core::ContainsAllFlags(file.GetFlags(), Core::FileFlags::WRITE));
		DBG_ASSERT(size > 0);
		DBG_ASSERT(src != nullptr);
		DBG_ASSERT(!result || (result->result_ == Result::INITIAL && result->workRemaining_ == 0));

		Result outResult = Result::PENDING;
		if(result)
		{
			auto oldResult = (Result)Core::AtomicExchg((volatile i32*)&result->result_, (i32)outResult);
			DBG_ASSERT(oldResult == Result::INITIAL);
		}

		ManagerImpl::FileIOJob job;
		job.file_ = &file;
		job.offset_ = 0;
		job.size_ = size;
		job.addr_ = src;
		job.result_ = result;

		if(result)
		{
			Core::AtomicAddAcq(&result->workRemaining_, size);
			impl_->writeJobs_.Enqueue(job);
			impl_->writeJobEvent_.Signal();
		}
		else
		{
			outResult = job.DoWrite();
		}
		return outResult;
	}


} // namespace Resource
