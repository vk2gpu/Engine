#pragma once

#include "core/hash.h"
#include "core/pair.h"
#include "core/vector.h"

namespace Core
{
	/**
	 * Hash set.
	 * TODO: Implement HashTable using robinhood hashing.
	 */
	template<typename KEY_TYPE, typename HASHER = Hasher<KEY_TYPE>, typename ALLOCATOR = Allocator>
	class Set
	{
	public:
		typedef i32 index_type;
		typedef KEY_TYPE* iterator;
		typedef const KEY_TYPE* const_iterator;

		static const index_type INVALID_INDEX = (index_type)-1;

		Set() { indices_.resize(maxIndex_, INVALID_INDEX); }

		Set(const Set& other)
		{
			values_ = other.values_;
			indices_ = other.indices_;
			maxIndex_ = other.maxIndex_;
			mask_ = other.mask_;
		}

		Set(Set&& other) { swap(other); }
		~Set() {}

		Set& operator=(const Set& other)
		{
			values_ = other.values_;
			indices_ = other.indices_;
			maxIndex_ = other.maxIndex_;
			mask_ = other.mask_;
			return *this;
		}

		Set& operator=(Set&& other)
		{
			swap(other);
			return *this;
		}

		void swap(Set& other)
		{
			// Include <utility> in your cpp.
			std::swap(values_, other.values_);
			std::swap(indices_, other.indices_);
			std::swap(maxIndex_, other.maxIndex_);
			std::swap(mask_, other.mask_);
		}

		void clear()
		{
			values_.clear();
			indices_.fill(INVALID_INDEX);
		}

		iterator insert(const KEY_TYPE& key)
		{
			const u32 keyHash = hasher_(0, key);
			const index_type indicesidx = keyHash & mask_;
			const index_type idx = indices_[indicesidx];
			if(idx != INVALID_INDEX)
			{
				// Got hit, check if hashes are same and replace or insert.
				if(hasher_(0, values_[idx]) == keyHash)
				{
					values_[idx] = key;
					return values_.data() + idx;
				}
				else
				{
					values_.push_back(key);
					resizeIndices(maxIndex_ * 2);
					return values_.data() + values_.size() - 1;
				}
			}
			else
			{
				// No hit, insert.
				values_.push_back(key);
				indices_[indicesidx] = values_.size() - 1;
				return values_.data() + values_.size() - 1;
			}
		}

		// TODO: TEST.
		iterator erase(iterator it)
		{
			DBG_ASSERT_MSG(it >= begin() && it < end(), "Invalid iterator.");
			index_type baseidx = (index_type)(it - begin());
			values_.erase(it);
			for(auto& idx : indices_)
			{
				if(idx == baseidx)
					idx = INVALID_INDEX;
				else if(idx > baseidx)
					--idx;
			}
			return it;
		}


		iterator begin() { return values_.data(); }
		const_iterator begin() const { return values_.data(); }
		iterator end() { return values_.data() + values_.size(); }
		const_iterator end() const { return values_.data() + values_.size(); }

		const_iterator find(const KEY_TYPE& key) const
		{
			const u32 keyHash = hasher_(0, key);
			const index_type indicesidx = keyHash & mask_;
			const index_type idx = indices_[indicesidx];
			if(idx != INVALID_INDEX)
			{
				return values_.data() + idx;
			}
			return end();
		}

		iterator find(const KEY_TYPE& key)
		{
			const u32 keyHash = hasher_(0, key);
			const index_type indicesidx = keyHash & mask_;
			const index_type idx = indices_[indicesidx];
			if(idx != INVALID_INDEX)
			{
				iterator retVal = values_.data() + idx;
				if(hasher_(0, *retVal) == keyHash)
				{
					return retVal;
				}
			}
			return end();
		}

		index_type size() const { return values_.size(); }
		bool empty() const { return values_.size() == 0; }

	private:
		void resizeIndices(index_type size)
		{
			bool collisions = true;

			// Enter loop and increase size of indices.
			while(collisions)
			{
				collisions = false;

				indices_.resize(size);
				indices_.fill(INVALID_INDEX);
				maxIndex_ = size;
				mask_ = size - 1;

				// Reinsert all keys.
				for(index_type idx = 0; idx < values_.size(); ++idx)
				{
					const u32 keyHash = hasher_(0, values_[idx]);
					const index_type indicesidx = keyHash & mask_;
					if(indices_[indicesidx] == INVALID_INDEX)
					{
						indices_[indicesidx] = idx;
					}
					else
					{
						collisions = true;
						break;
					}
				}

				if(collisions)
				{
					size *= 2;
					continue;
				}
			}
		}

		Vector<KEY_TYPE, ALLOCATOR> values_;
		Vector<index_type, ALLOCATOR> indices_;

		index_type maxIndex_ = 0x8;
		index_type mask_ = 0x7;

		HASHER hasher_;
	};
} // namespace Core
