#pragma once

#include "core/concurrency.h"
#include "core/debug.h"

namespace Core
{
	/**
	 * MPMC Queue, by Dmitry Vyukov.
	 * http://www.1024cores.net/home/lock-free-algorithms/queues/bounded-mpmc-queue
	 */
	template<typename TYPE>
	class MPMCBoundedQueue
	{
	public:
		MPMCBoundedQueue() = default;
		MPMCBoundedQueue(i32 size)
		    : buffer_(new Cell[size])
		    , bufferMask_(size - 1)
		{
			DBG_ASSERT((size >= 2) && ((size & (size - 1)) == 0));
			for(i32 i = 0; i != size; i += 1)
				Core::AtomicExchg(&buffer_[i].sequence_, i);
			Core::AtomicExchg(&enqueuePos_, 0);
			Core::AtomicExchg(&dequeuePos_, 0);
		}
		MPMCBoundedQueue(MPMCBoundedQueue&& other)
		{
			using std::swap;
			swap(buffer_, other.buffer_);
			swap(bufferMask_, other.bufferMask_);
			swap(enqueuePos_, other.enqueuePos_);
			swap(dequeuePos_, other.dequeuePos_);
		}
		MPMCBoundedQueue& operator = (MPMCBoundedQueue&& other)
		{
			using std::swap;
			swap(buffer_, other.buffer_);
			swap(bufferMask_, other.bufferMask_);
			swap(enqueuePos_, other.enqueuePos_);
			swap(dequeuePos_, other.dequeuePos_);
			return *this;
		}

		~MPMCBoundedQueue()
		{
			delete [] buffer_;
		}

		bool Enqueue(const TYPE& data)
		{
			Cell* cell = nullptr;
			i32 pos = Core::AtomicCmpExchg(&enqueuePos_, 0, 0);
			for(;;)
			{
				cell = &buffer_[pos & bufferMask_];
				i32 seq = Core::AtomicCmpExchgAcq(&cell->sequence_, 0, 0);
				i32 dif = seq - pos;
				if(dif == 0)
				{
					if(Core::AtomicCmpExchg(&enqueuePos_, pos + 1, pos) == pos) // was compare_exchange_weak
						break;
				}
				else if (dif < 0)
					return false;
				else
					pos = Core::AtomicCmpExchg(&enqueuePos_, 0, 0);
			}
			cell->data_ = data;
			Core::AtomicExchg(&cell->sequence_, pos + 1); // was release
			return true;
		}

		bool Dequeue(TYPE& data)
		{
			Cell* cell = nullptr;
			i32 pos = Core::AtomicCmpExchg(&dequeuePos_, 0, 0);
			for(;;)
			{
				cell = &buffer_[pos & bufferMask_];
				i32 seq = Core::AtomicCmpExchgAcq(&cell->sequence_, 0, 0);
				i32 dif = seq - (pos + 1);
				if(dif == 0)
				{
					if(Core::AtomicCmpExchg(&dequeuePos_, pos + 1, pos) == pos) // was compare_exchange_weak
						break;
				}
				else if (dif < 0)
					return false;
				else
					pos = Core::AtomicCmpExchg(&dequeuePos_, 0, 0);
			}
			data = cell->data_;
			Core::AtomicExchg(&cell->sequence_, pos + bufferMask_ + 1); // was release
			return true;
		}

	private:
		struct Cell
		{
			volatile i32 sequence_ = 0;
			TYPE data_ = TYPE();
		};

		static i32 const CACHE_LINE_SIZE = 64;
		typedef char CacheLinePad[CACHE_LINE_SIZE];

		CacheLinePad pad0_ = { 0 };
		Cell* buffer_ = nullptr;
		i32 bufferMask_ = 0;
		CacheLinePad pad1_ = { 0 };
		volatile i32 enqueuePos_ = 0;
		CacheLinePad pad2_ = { 0 };
		volatile i32 dequeuePos_ = 0;
		CacheLinePad pad3_ = { 0 };

		MPMCBoundedQueue(const MPMCBoundedQueue&) = delete;
		void operator=(const MPMCBoundedQueue&) = delete;
	};
} // namespace Core
