#pragma once

#include "core/concurrency.h"
#include "core/debug.h"

namespace Core
{
	/*  Multi-producer/multi-consumer bounded queue.
	 *  Copyright (c) 2010-2011, Dmitry Vyukov. All rights reserved.
	 *  Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
	 *     1. Redistributions of source code must retain the above copyright notice, this list of
	 *        conditions and the following disclaimer.
	 *     2. Redistributions in binary form must reproduce the above copyright notice, this list
	 *        of conditions and the following disclaimer in the documentation and/or other materials
	 *        provided with the distribution.
	 *  THIS SOFTWARE IS PROVIDED BY DMITRY VYUKOV "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
	 *  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
	 *  DMITRY VYUKOV OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
	 *  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
	 *  HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
	 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
	 *  The views and conclusions contained in the software and documentation are those of the authors and should not be interpreted
	 *  as representing official policies, either expressed or implied, of Dmitry Vyukov.
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
			Core::Barrier();
			for(i32 i = 0; i != size; i += 1)
				buffer_[i].sequence_ = i;
			enqueuePos_ = 0;
			dequeuePos_ = 0;
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
					if(Core::AtomicCmpExchg(&enqueuePos_, pos + 1, pos) == pos)
						break;
				}
				else if (dif < 0)
					return false;
				else
					pos = Core::AtomicCmpExchg(&enqueuePos_, 0, 0);
			}
			cell->data_ = data;
			Core::Barrier();
			cell->sequence_ = pos + 1;
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
					if(Core::AtomicCmpExchg(&dequeuePos_, pos + 1, pos) == pos)
						break;
				}
				else if (dif < 0)
					return false;
				else
					pos = Core::AtomicCmpExchg(&dequeuePos_, 0, 0);
			}
			data = cell->data_;
			Core::Barrier();
			cell->sequence_ = pos + bufferMask_ + 1;
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
