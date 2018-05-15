#pragma once

#include "core/hash.h"
#include "core/pair.h"
#include "core/vector.h"

#include <utility>

namespace Core
{
	/**
	 * Hash map.
	 * https://www.sebastiansylvan.com/post/robin-hood-hashing-should-be-your-default-hash-table-implementation/
	 */
	template<typename KEY_TYPE, typename VALUE_TYPE, typename HASHER = Hasher<KEY_TYPE>,
	    typename ALLOCATOR = ContainerAllocator>
	class Map
	{
	public:
		using index_type = i32;
		using this_type = Map<KEY_TYPE, VALUE_TYPE, HASHER, ALLOCATOR>;

		static const index_type INITIAL_SIZE = 16;
		static const index_type LOAD_FACTOR_PERCENT = 75;
		static const u32 HASH_MSB_MASK = 0x7fffffff;
		static const u32 HASH_MSB = 0x80000000;

		struct key_value_pair
		{
			const KEY_TYPE& key;
			VALUE_TYPE& value;
		};

		struct const_key_value_pair
		{
			const KEY_TYPE& key;
			const VALUE_TYPE& value;
		};

		struct iterator_base
		{
			iterator_base() = default;
			iterator_base(const this_type* parent, i32 pos)
			    : parent_(parent)
			    , pos_(pos)
			{
			}

			iterator_base& operator++()
			{
				DBG_ASSERT(parent_);
				DBG_ASSERT(pos_ >= 0);
				pos_ = parent_->LookupIndex(pos_ + 1);
				return *this;
			}

			iterator_base operator++(int)
			{
				DBG_ASSERT(parent_);
				DBG_ASSERT(pos_ >= 0);
				iterator_base it = {parent_, -1};
				it.pos_ = parent_->LookupIndex(pos_ + 1);
				return it;
			}

			bool operator!=(const iterator_base& other) const { return parent_ != other.parent_ || pos_ != other.pos_; }

		protected:
			const this_type* parent_ = nullptr;
			index_type pos_ = -1;
		};

		struct iterator : iterator_base
		{
			iterator(const this_type* parent, i32 pos)
			    : iterator_base(parent, pos)
			{
			}

			key_value_pair operator*() { return key_value_pair{parent_->keys_[pos_], parent_->values_[pos_]}; }
		};

		struct const_iterator : iterator_base
		{
			const_iterator(const this_type* parent, i32 pos)
			    : iterator_base(parent, pos)
			{
			}

			const_key_value_pair operator*()
			{
				return const_key_value_pair{parent_->keys_[pos_], parent_->values_[pos_]};
			}
		};


		Map(ALLOCATOR& allocator, i32 initialSize = INITIAL_SIZE)
		    : allocator_(allocator)
		{
			capacity_ = initialSize;
			Alloc();
		}
		Map(i32 initialSize = INITIAL_SIZE)
		{
			capacity_ = initialSize;
			Alloc();
		}

		Map(const Map& other)
		    : allocator_(other.allocator_)
		{
			copy(other);
		}

		Map(Map&& other) { swap(other); }
		~Map()
		{
			if(hashes_)
			{
				for(index_type i = 0; i < capacity_; ++i)
				{
					if(hashes_[i] != 0)
					{
						keys_[i].~KEY_TYPE();
						values_[i].~VALUE_TYPE();
					}
				}
			}

			allocator_.Deallocate(keys_);
			allocator_.Deallocate(values_);
			allocator_.Deallocate(hashes_);
		}

		Map& operator=(const Map& other)
		{
			copy(other);
			return *this;
		}

		Map& operator=(Map&& other)
		{
			swap(other);
			return *this;
		}

		void swap(Map& other)
		{
			std::swap(allocator_, other.allocator_);
			std::swap(keys_, other.keys_);
			std::swap(values_, other.values_);
			std::swap(hashes_, other.hashes_);
			std::swap(capacity_, other.capacity_);
			std::swap(mask_, other.mask_);
			std::swap(numElements_, other.numElements_);
			std::swap(resizeThreshold_, other.resizeThreshold_);
		}

		void copy(const Map& other)
		{
			allocator_.Deallocate(keys_);
			allocator_.Deallocate(values_);
			allocator_.Deallocate(hashes_);
			allocator_ = other.allocator_;
			keys_ = nullptr;
			values_ = nullptr;
			hashes_ = nullptr;

			capacity_ = other.capacity_;
			Alloc();

			for(index_type i = 0; i < other.capacity_; ++i)
			{
				auto& k = other.keys_[i];
				auto& v = other.values_[i];
				u32 h = other.hashes_[i];
				if(h != 0 && !IsDeleted(h))
					InsertHelper(h, std::move(KEY_TYPE(k)), std::move(VALUE_TYPE(v)));
			}
		}

		VALUE_TYPE& operator[](const KEY_TYPE& key)
		{
			VALUE_TYPE* foundValue = find(key);
			if(foundValue == nullptr)
			{
				foundValue = insert(key, VALUE_TYPE());
			}
			DBG_ASSERT_MSG(foundValue != nullptr, "Failed to insert element for key.");
			return *foundValue;
		}

		const VALUE_TYPE& operator[](const KEY_TYPE& key) const
		{
			VALUE_TYPE* foundValue = find(key);
			DBG_ASSERT_MSG(foundValue != nullptr, "key does not exist in map.");
			return *foundValue;
		}

		void clear()
		{
			for(index_type i = 0; i < capacity_; ++i)
			{
				if(hashes_[i] != 0)
				{
					keys_[i].~KEY_TYPE();
					values_[i].~VALUE_TYPE();
					hashes_[i] = 0;
				}
			}
			numElements_ = 0;
		}

		VALUE_TYPE* insert(KEY_TYPE key, VALUE_TYPE value)
		{
			if(auto* found = find(key))
			{
				std::swap(*found, value);
				return found;
			}

			if(++numElements_ >= resizeThreshold_)
				Grow();
			return InsertHelper(HashKey(key), std::move(key), std::move(value));
		}


		bool erase(const KEY_TYPE& key)
		{
			const index_type i = LookupIndexByKey(key);

			if(i == -1)
				return false;
			keys_[i].~KEY_TYPE();
			values_[i].~VALUE_TYPE();
			hashes_[i] |= HASH_MSB;
			--numElements_;
			return true;
		}

		VALUE_TYPE* find(const KEY_TYPE& key)
		{
			const index_type i = LookupIndexByKey(key);
			return i != -1 ? &values_[i] : nullptr;
		}

		const VALUE_TYPE* find(const KEY_TYPE& key) const
		{
			const index_type i = LookupIndexByKey(key);
			return i != -1 ? &values_[i] : nullptr;
		}

		index_type size() const { return numElements_; }
		bool empty() const { return numElements_ == 0; }

		f32 AverageProbeCount() const
		{
			f32 probeTotal = 0.0f;
			for(index_type i = 0; i < capacity_; ++i)
			{
				u32 hash = hashes_[i];
				if(hash != 0 && !IsDeleted(hash))
				{
					probeTotal += ProbeDistance(hash, i);
				}
			}
			return probeTotal / size() + 1.0f;
		}

		iterator begin() { return iterator{this, LookupIndex(0)}; }
		const_iterator begin() const { return const_iterator{this, LookupIndex(0)}; }
		iterator end() { return iterator{this, -1}; }
		const_iterator end() const { return const_iterator{this, -1}; }

	private:
		void Alloc()
		{
			DBG_ASSERT(keys_ == nullptr && values_ == nullptr && hashes_ == nullptr);
			keys_ = reinterpret_cast<KEY_TYPE*>(allocator_.Allocate(capacity_ * sizeof(KEY_TYPE), alignof(KEY_TYPE)));
			values_ =
			    reinterpret_cast<VALUE_TYPE*>(allocator_.Allocate(capacity_ * sizeof(VALUE_TYPE), alignof(VALUE_TYPE)));
			hashes_ = reinterpret_cast<u32*>(allocator_.Allocate(capacity_ * sizeof(u32), alignof(u32)));
			for(index_type i = 0; i < capacity_; ++i)
				hashes_[i] = 0;
			resizeThreshold_ = (capacity_ * LOAD_FACTOR_PERCENT) / 100;
			mask_ = capacity_ - 1;
		}

		void Grow()
		{
			const index_type oldCapacity = capacity_;
			auto oldKeys = keys_;
			auto oldValues = values_;
			auto oldHashes = hashes_;
			keys_ = nullptr;
			values_ = nullptr;
			hashes_ = nullptr;
			capacity_ *= 2;

			Alloc();

			for(index_type i = 0; i < oldCapacity; ++i)
			{
				auto& k = oldKeys[i];
				auto& v = oldValues[i];
				u32 h = oldHashes[i];
				if(h != 0 && !IsDeleted(h))
					InsertHelper(h, std::move(k), std::move(v));
			}

			allocator_.Deallocate(oldKeys);
			allocator_.Deallocate(oldValues);
			allocator_.Deallocate(oldHashes);
		}

		u32 HashKey(const KEY_TYPE& key) const
		{
			u64 h = hasher_(0, key);
			h &= HASH_MSB_MASK;
			h |= h == 0;
			return (u32)h;
		}

		bool IsDeleted(u32 h) const { return (h & HASH_MSB) != 0; }

		index_type DesiredPos(u32 h) const { return h & mask_; }

		index_type ProbeDistance(u32 h, index_type idx) const { return (idx + capacity_ - DesiredPos(h)) & mask_; }

		void Construct(index_type i, u32 hash, KEY_TYPE&& key, VALUE_TYPE&& val)
		{
			new(&keys_[i]) KEY_TYPE(std::move(key));
			new(&values_[i]) VALUE_TYPE(std::move(val));
			hashes_[i] = hash;
		}

		VALUE_TYPE* InsertHelper(u32 hash, KEY_TYPE&& key, VALUE_TYPE&& val)
		{
			index_type pos = DesiredPos(hash);
			index_type dist = 0;
			VALUE_TYPE* retVal = nullptr;
			for(;;)
			{
				if(hashes_[pos] == 0 || IsDeleted(hashes_[pos]))
				{
					Construct(pos, hash, std::move(key), std::move(val));
					if(retVal == nullptr)
						retVal = &values_[pos];
					return retVal;
				}

				// If the existing elem has probed less than us, then swap places with existing
				// elem, and keep going to find another slot for that elem.
				index_type existingElemProbeDist = ProbeDistance(hashes_[pos], pos);
				if(existingElemProbeDist < dist)
				{
					std::swap(hash, hashes_[pos]);
					std::swap(key, keys_[pos]);
					std::swap(val, values_[pos]);
					dist = existingElemProbeDist;

					if(retVal == nullptr)
						retVal = &values_[pos];
				}

				pos = (pos + 1) & mask_;
				++dist;
			}
		}

		index_type LookupIndexByKey(const KEY_TYPE& key) const
		{
			const u32 hash = HashKey(key);
			index_type pos = DesiredPos(hash);
			index_type dist = 0;
			for(;;)
			{
				if(hashes_[pos] == 0)
					return -1;
				else if(dist > capacity_)
					return -1;
				else if(hashes_[pos] == hash && keys_[pos] == key)
					return pos;

				pos = (pos + 1) & mask_;
				++dist;
			}
		}

		index_type LookupIndex(index_type i) const
		{
			while(i < capacity_)
			{
				u32 h = hashes_[i];
				if(h != 0 && !IsDeleted(h))
				{
					return i;
				}
				++i;
			}
			return -1;
		}

		KEY_TYPE* keys_ = nullptr;
		VALUE_TYPE* values_ = nullptr;
		u32* hashes_ = nullptr;

		index_type numElements_ = 0;
		index_type resizeThreshold_ = 0;
		index_type capacity_ = INITIAL_SIZE;
		u32 mask_ = 0;

		ALLOCATOR allocator_;
		HASHER hasher_;
	};

} // namespace Core
