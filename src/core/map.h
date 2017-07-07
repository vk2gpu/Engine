#pragma once

#include "core/hash.h"
#include "core/pair.h"
#include "core/vector.h"

#include <utility>

namespace Core
{
	/**
	 * Hash map.
	 * TODO: Implement HashTable using robinhood hashing.
	 */
	template<typename KEY_TYPE, typename VALUE_TYPE, typename HASHER = Hasher<KEY_TYPE>, typename ALLOCATOR = Allocator>
	class Map
	{
	public:
		typedef i32 index_type;
		typedef Pair<KEY_TYPE, VALUE_TYPE> value_type;
		typedef value_type* iterator;
		typedef const value_type* const_iterator;

		static const index_type INVALID_INDEX = (index_type)-1;

		Map() { indices_.resize(maxIndex_, INVALID_INDEX); }

		Map(const Map& other)
		{
			values_ = other.values_;
			indices_ = other.indices_;
			maxIndex_ = other.maxIndex_;
			mask_ = other.mask_;
		}

		Map(Map&& other) { swap(other); }
		~Map() {}

		Map& operator=(const Map& other)
		{
			values_ = other.values_;
			indices_ = other.indices_;
			maxIndex_ = other.maxIndex_;
			mask_ = other.mask_;
			return *this;
		}

		Map& operator=(Map&& other)
		{
			swap(other);
			return *this;
		}

		void swap(Map& other)
		{
			std::swap(values_, other.values_);
			std::swap(indices_, other.indices_);
			std::swap(maxIndex_, other.maxIndex_);
			std::swap(mask_, other.mask_);
		}

		VALUE_TYPE& operator[](const KEY_TYPE& key)
		{
			iterator foundValue = find(key);
			if(foundValue == end())
			{
				foundValue = insert(key, VALUE_TYPE());
			}
			DBG_ASSERT_MSG(foundValue != end(), "Failed to insert element for key.");
			return foundValue->second;
		}

		const VALUE_TYPE& operator[](const KEY_TYPE& key) const
		{
			iterator foundValue = find(key);
			DBG_ASSERT_MSG(foundValue != end(), "key does not exist in map.");
			return foundValue->second;
		}

		void clear()
		{
			values_.clear();
			indices_.fill(INVALID_INDEX);
		}

		iterator insert(const KEY_TYPE& key, const VALUE_TYPE& value)
		{
			const auto keyValuePair = Pair<typename KEY_TYPE, typename VALUE_TYPE>(key, value);
			const u32 keyHash = hasher_(0, key);
			const index_type indicesIdx = keyHash & mask_;
			const index_type idx = indices_[indicesIdx];
			if(idx != INVALID_INDEX)
			{
				// Got hit, check if hashes are same and replace or insert.
				if(hasher_(0, values_[idx].first) == keyHash)
				{
					values_[idx] = keyValuePair;
					return values_.data() + idx;
				}
				else
				{
					values_.push_back(keyValuePair);
					resizeIndices(maxIndex_ * 2);
					return values_.data() + values_.size() - 1;
				}
			}
			else
			{
				// No hit, insert.
				values_.push_back(keyValuePair);
				indices_[indicesIdx] = values_.size() - 1;
				return values_.data() + values_.size() - 1;
			}
		}

		// TODO: TEST.
		iterator erase(iterator it)
		{
			DBG_ASSERT_MSG(it >= begin() && it < end(), "Invalid iterator.");
			index_type baseIdx = (index_type)(it - begin());
			values_.erase(it);
			for(auto& idx : indices_)
			{
				if(idx == baseIdx)
					idx = INVALID_INDEX;
				else if(idx > baseIdx)
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
			const index_type indicesIdx = keyHash & mask_;
			const index_type idx = indices_[indicesIdx];
			if(idx != INVALID_INDEX)
			{
				return values_.data() + idx;
			}
			return end();
		}

		iterator find(const KEY_TYPE& key)
		{
			const u32 keyHash = hasher_(0, key);
			const index_type indicesIdx = keyHash & mask_;
			const index_type idx = indices_[indicesIdx];
			if(idx != INVALID_INDEX)
			{
				iterator retVal = values_.data() + idx;
				if(hasher_(0, retVal->first) == keyHash)
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
					const u32 keyHash = hasher_(0, values_[idx].first);
					const index_type indicesIdx = keyHash & mask_;
					if(indices_[indicesIdx] == INVALID_INDEX)
					{
						indices_[indicesIdx] = idx;
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

		Vector<value_type, ALLOCATOR> values_;
		Vector<index_type, ALLOCATOR> indices_;

		index_type maxIndex_ = 0x8;
		index_type mask_ = 0x7;

		HASHER hasher_;
	};
} // namespace Core
