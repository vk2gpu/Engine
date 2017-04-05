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
	template<typename KEY_TYPE>
	class Set
	{
	public:
		typedef i32 index_type;
		typedef KEY_TYPE* iterator;
		typedef const KEY_TYPE* const_iterator;

		static const index_type INVALID_INDEX = (index_type)-1;

		Set() { Indices_.resize(MaxIndex_, INVALID_INDEX); }

		Set(const Set& Other)
		{
			Values_ = Other.Values_;
			Indices_ = Other.Indices_;
			MaxIndex_ = Other.MaxIndex_;
			Mask_ = Other.Mask_;
		}

		Set(Set&& Other) { swap(Other); }
		~Set() {}

		Set& operator=(const Set& Other)
		{
			Values_ = Other.Values_;
			Indices_ = Other.Indices_;
			MaxIndex_ = Other.MaxIndex_;
			Mask_ = Other.Mask_;
			return *this;
		}

		Set& operator=(Set&& Other)
		{
			swap(Other);
			return *this;
		}

		void swap(Set& Other)
		{
			// Include <utility> in your cpp.
			std::swap(Values_, Other.Values_);
			std::swap(Indices_, Other.Indices_);
			std::swap(MaxIndex_, Other.MaxIndex_);
			std::swap(Mask_, Other.Mask_);
		}

		void clear()
		{
			Values_.clear();
			Indices_.fill(INVALID_INDEX);
		}

		iterator insert(const KEY_TYPE& Key)
		{
			const u32 KeyHash = Hash(0, Key);
			const index_type IndicesIdx = KeyHash & Mask_;
			const index_type Idx = Indices_[IndicesIdx];
			if(Idx != INVALID_INDEX)
			{
				// Got hit, check if hashes are same and replace or insert.
				if(Hash(0, Values_[Idx]) == KeyHash)
				{
					Values_[Idx] = Key;
					return Values_.data() + Idx;
				}
				else
				{
					Values_.push_back(Key);
					resizeIndices(MaxIndex_ * 2);
					return Values_.data() + Values_.size() - 1;
				}
			}
			else
			{
				// No hit, insert.
				Values_.push_back(Key);
				Indices_[IndicesIdx] = Values_.size() - 1;
				return Values_.data() + Values_.size() - 1;
			}
		}

		// TODO: TEST.
		iterator erase(iterator It)
		{
			DBG_ASSERT_MSG(It >= begin() && It < end(), "Invalid iterator.");
			index_type BaseIdx = (index_type)(It - begin());
			Values_.erase(It);
			for(auto& Idx : Indices_)
			{
				if(Idx == BaseIdx)
					Idx = INVALID_INDEX;
				else if(Idx > BaseIdx)
					--Idx;
			}
			return It;
		}


		iterator begin() { return Values_.data(); }
		const_iterator begin() const { return Values_.data(); }
		iterator end() { return Values_.data() + Values_.size(); }
		const_iterator end() const { return Values_.data() + Values_.size(); }

		const_iterator find(const KEY_TYPE& Key) const
		{
			const u32 KeyHash = Hash(0, Key);
			const index_type IndicesIdx = KeyHash & Mask_;
			const index_type Idx = Indices_[IndicesIdx];
			if(Idx != INVALID_INDEX)
			{
				return Values_.data() + Idx;
			}
			return end();
		}

		iterator find(const KEY_TYPE& Key)
		{
			const u32 KeyHash = Hash(0, Key);
			const index_type IndicesIdx = KeyHash & Mask_;
			const index_type Idx = Indices_[IndicesIdx];
			if(Idx != INVALID_INDEX)
			{
				iterator RetVal = Values_.data() + Idx;
				if(Hash(0, *RetVal) == KeyHash)
				{
					return RetVal;
				}
			}
			return end();
		}

		index_type size() const { return Values_.size(); }
		bool empty() const { return Values_.size() == 0; }

	private:
		void resizeIndices(index_type Size)
		{
			bool Collisions = true;

			// Enter loop and increase size of indices.
			while(Collisions)
			{
				Collisions = false;

				Indices_.resize(Size);
				Indices_.fill(INVALID_INDEX);
				MaxIndex_ = Size;
				Mask_ = Size - 1;

				// Reinsert all keys.
				for(index_type Idx = 0; Idx < Values_.size(); ++Idx)
				{
					const u32 KeyHash = Hash(0, Values_[Idx]);
					const index_type IndicesIdx = KeyHash & Mask_;
					if(Indices_[IndicesIdx] == INVALID_INDEX)
					{
						Indices_[IndicesIdx] = Idx;
					}
					else
					{
						Collisions = true;
						break;
					}
				}

				if(Collisions)
				{
					Size *= 2;
					continue;
				}
			}
		}

		Vector<KEY_TYPE> Values_;
		Vector<index_type> Indices_;

		index_type MaxIndex_ = 0x8;
		index_type Mask_ = 0x7;
	};
} // namespace Core
