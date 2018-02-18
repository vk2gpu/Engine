#include "core/map.h"
#include "core/string.h"
#include "core/timer.h"

#include "catch.hpp"

#include <unordered_map>

using namespace Core;

namespace std
{
	template<>
	struct hash<Core::String>
	{
		typedef Core::String argument_type;
		typedef std::size_t result_type;
		result_type operator()(argument_type const& s) const noexcept { return Core::Hash(0, s); }
	};
}

namespace
{
	struct ScopedTimer
	{
		ScopedTimer(const char* message)
		    : message_(message)
		{
			timer_.Mark();
		}

		~ScopedTimer()
		{
			f64 time = timer_.GetTime();
			Core::Log("%s: %.2fus\n", message_, time * 1000000.0);
		}
		const char* message_ = nullptr;
		Core::Timer timer_;
	};

	typedef i32 index_type;

	template<typename KEY_TYPE, typename VALUE_TYPE>
	void MapTestSize(KEY_TYPE(IdxToKey)(index_type), VALUE_TYPE(IdxToVal)(index_type))
	{
		Map<KEY_TYPE, VALUE_TYPE> TestMap;
		REQUIRE(TestMap.size() == 0);

		TestMap.insert(IdxToKey(0), IdxToVal(0));
		REQUIRE(TestMap.size() == 1);
	}

	template<typename KEY_TYPE, typename VALUE_TYPE, index_type SIZE>
	void MapTestInsert(KEY_TYPE(IdxToKey)(index_type), VALUE_TYPE(IdxToVal)(index_type))
	{
		Map<KEY_TYPE, VALUE_TYPE> TestMap;

		for(index_type idx = 0; idx < SIZE; ++idx)
		{
			TestMap.insert(IdxToKey(idx), IdxToVal(idx));
		}
		REQUIRE(TestMap.size() == SIZE);

		for(index_type idx = 0; idx < SIZE; ++idx)
		{
			TestMap.insert(IdxToKey(idx), IdxToVal(idx));
		}
		REQUIRE(TestMap.size() == SIZE);
	}

	template<typename KEY_TYPE, typename VALUE_TYPE, index_type SIZE>
	void MapTestFind(KEY_TYPE(IdxToKey)(index_type), VALUE_TYPE(IdxToVal)(index_type))
	{
		Map<KEY_TYPE, VALUE_TYPE> TestMap;

		for(index_type idx = 0; idx < SIZE; ++idx)
		{
			TestMap.insert(IdxToKey(idx), IdxToVal(idx));
		}

		bool success = true;
		for(index_type idx = 0; idx < SIZE; ++idx)
		{
			auto* Val = TestMap.find(IdxToKey(idx));
			success &= Val != nullptr;
			if(Val)
			{
				success &= *Val == IdxToVal(idx);
			}
		}
		REQUIRE(success);
	}

	template<typename KEY_TYPE, typename VALUE_TYPE, index_type SIZE>
	void MapTestOperatorInsert(KEY_TYPE(IdxToKey)(index_type), VALUE_TYPE(IdxToVal)(index_type))
	{
		Map<KEY_TYPE, VALUE_TYPE> TestMap;

		for(index_type idx = 0; idx < SIZE; ++idx)
		{
			if(idx == 0x71)
			{
				int a = 0;
				++a;
			}
			TestMap[IdxToKey(idx)] = IdxToVal(idx);
		}

		bool success = true;
		for(index_type idx = 0; idx < SIZE; ++idx)
		{
			auto* Val = TestMap.find(IdxToKey(idx));
			success &= Val != nullptr;
			if(Val)
			{
				success &= *Val == IdxToVal(idx);
				if(!success)
				{
					int a = 0;
					++a;
				}
			}
		}
		REQUIRE(success);
	}

	template<typename KEY_TYPE, typename VALUE_TYPE, index_type SIZE>
	void MapTestOperatorFind(KEY_TYPE(IdxToKey)(index_type), VALUE_TYPE(IdxToVal)(index_type))
	{
		Map<KEY_TYPE, VALUE_TYPE> TestMap;

		for(index_type idx = 0; idx < SIZE; ++idx)
		{
			TestMap[IdxToKey(idx)] = IdxToVal(idx);
		}

		bool success = true;
		for(index_type idx = 0; idx < SIZE; ++idx)
		{
			auto Val = TestMap[IdxToKey(idx)];
			success &= Val == IdxToVal(idx);
		}
		REQUIRE(success);
	}

	template<typename KEY_TYPE, typename VALUE_TYPE, index_type SIZE>
	void MapTestOperatorErase(KEY_TYPE(IdxToKey)(index_type), VALUE_TYPE(IdxToVal)(index_type))
	{
		Map<KEY_TYPE, VALUE_TYPE> TestMap;

		for(index_type idx = 0; idx < SIZE; ++idx)
		{
			TestMap[IdxToKey(idx)] = IdxToVal(idx);
		}

		bool success = true;
		for(index_type idx = 0; idx < SIZE; ++idx)
		{
			auto Val = TestMap[IdxToKey(idx)];
			success &= Val == IdxToVal(idx);
			REQUIRE(success);

			if(idx == (SIZE / 2))
			{
				TestMap.erase(IdxToVal(idx));
			}
		}

		for(index_type idx = 0; idx < SIZE; ++idx)
		{
			auto val = TestMap.find(IdxToKey(idx));
			if(idx != (SIZE / 2))
			{
				success &= val != nullptr;
				if(val)
				{
					success &= *val == IdxToVal(idx);
				}
			}
			else
			{
				success &= (val == nullptr);
				REQUIRE(success);
			}
		}
	}

	index_type IdxToVal_index_type(index_type idx) { return idx; }

	Core::String IdxToVal_string(index_type idx)
	{
		Core::String str;
		str.Printf("%u", (int)idx);
		return str;
	}
}

TEST_CASE("map-tests-size")
{
	MapTestSize<index_type, index_type>(IdxToVal_index_type, IdxToVal_index_type);
}

TEST_CASE("map-tests-insert")
{
	SECTION("trivial")
	{
		MapTestInsert<index_type, index_type, 0x1>(IdxToVal_index_type, IdxToVal_index_type);
		MapTestInsert<index_type, index_type, 0x2>(IdxToVal_index_type, IdxToVal_index_type);
		MapTestInsert<index_type, index_type, 0xff>(IdxToVal_index_type, IdxToVal_index_type);
		MapTestInsert<index_type, index_type, 0x100>(IdxToVal_index_type, IdxToVal_index_type);
	}

	SECTION("non-trivial")
	{
		MapTestInsert<Core::String, Core::String, 0x1>(IdxToVal_string, IdxToVal_string);
		MapTestInsert<Core::String, Core::String, 0x2>(IdxToVal_string, IdxToVal_string);
		MapTestInsert<Core::String, Core::String, 0xff>(IdxToVal_string, IdxToVal_string);
		MapTestInsert<Core::String, Core::String, 0x100>(IdxToVal_string, IdxToVal_string);
	}
}


TEST_CASE("map-tests-find")
{
	SECTION("trivial")
	{
		MapTestFind<index_type, index_type, 0x1>(IdxToVal_index_type, IdxToVal_index_type);
		MapTestFind<index_type, index_type, 0x2>(IdxToVal_index_type, IdxToVal_index_type);
		MapTestFind<index_type, index_type, 0xff>(IdxToVal_index_type, IdxToVal_index_type);
		MapTestFind<index_type, index_type, 0x100>(IdxToVal_index_type, IdxToVal_index_type);
	}

	SECTION("non-trivial")
	{
		MapTestFind<Core::String, Core::String, 0x1>(IdxToVal_string, IdxToVal_string);
		MapTestFind<Core::String, Core::String, 0x2>(IdxToVal_string, IdxToVal_string);
		MapTestFind<Core::String, Core::String, 0xff>(IdxToVal_string, IdxToVal_string);
		MapTestFind<Core::String, Core::String, 0x100>(IdxToVal_string, IdxToVal_string);
	}
}

TEST_CASE("map-tests-operator-insert")
{
	SECTION("trivial")
	{
		MapTestOperatorInsert<index_type, index_type, 0x1>(IdxToVal_index_type, IdxToVal_index_type);
		MapTestOperatorInsert<index_type, index_type, 0x2>(IdxToVal_index_type, IdxToVal_index_type);
		MapTestOperatorInsert<index_type, index_type, 0xff>(IdxToVal_index_type, IdxToVal_index_type);
		MapTestOperatorInsert<index_type, index_type, 0x100>(IdxToVal_index_type, IdxToVal_index_type);
	}

	SECTION("non-trivial")
	{
		MapTestOperatorInsert<Core::String, Core::String, 0x1>(IdxToVal_string, IdxToVal_string);
		MapTestOperatorInsert<Core::String, Core::String, 0x2>(IdxToVal_string, IdxToVal_string);
		MapTestOperatorInsert<Core::String, Core::String, 0xff>(IdxToVal_string, IdxToVal_string);
		MapTestOperatorInsert<Core::String, Core::String, 0x100>(IdxToVal_string, IdxToVal_string);
	}
}


TEST_CASE("map-tests-operator-find")
{
	SECTION("trivial")
	{
		MapTestOperatorFind<index_type, index_type, 0x1>(IdxToVal_index_type, IdxToVal_index_type);
		MapTestOperatorFind<index_type, index_type, 0x2>(IdxToVal_index_type, IdxToVal_index_type);
		MapTestOperatorFind<index_type, index_type, 0xff>(IdxToVal_index_type, IdxToVal_index_type);
		MapTestOperatorFind<index_type, index_type, 0x100>(IdxToVal_index_type, IdxToVal_index_type);
	}

	SECTION("non-trivial")
	{
		MapTestOperatorFind<Core::String, Core::String, 0x1>(IdxToVal_string, IdxToVal_string);
		MapTestOperatorFind<Core::String, Core::String, 0x2>(IdxToVal_string, IdxToVal_string);
		MapTestOperatorFind<Core::String, Core::String, 0xff>(IdxToVal_string, IdxToVal_string);
		MapTestOperatorFind<Core::String, Core::String, 0x100>(IdxToVal_string, IdxToVal_string);
	}
}


TEST_CASE("map-tests-operator-erase")
{
	SECTION("trivial")
	{
		MapTestOperatorErase<index_type, index_type, 0x1>(IdxToVal_index_type, IdxToVal_index_type);
		MapTestOperatorErase<index_type, index_type, 0x2>(IdxToVal_index_type, IdxToVal_index_type);
		MapTestOperatorErase<index_type, index_type, 0xff>(IdxToVal_index_type, IdxToVal_index_type);
		MapTestOperatorErase<index_type, index_type, 0x100>(IdxToVal_index_type, IdxToVal_index_type);
	}

	SECTION("non-trivial")
	{
		MapTestOperatorErase<Core::String, Core::String, 0x1>(IdxToVal_string, IdxToVal_string);
		MapTestOperatorErase<Core::String, Core::String, 0x2>(IdxToVal_string, IdxToVal_string);
		MapTestOperatorErase<Core::String, Core::String, 0xff>(IdxToVal_string, IdxToVal_string);
		MapTestOperatorErase<Core::String, Core::String, 0x100>(IdxToVal_string, IdxToVal_string);
	}
}

TEST_CASE("map-tests-iterate")
{
	Core::Map<u32, i32> map;
	const i32 NUM_VALUES = 4096;
	for(i32 i = 0; i < NUM_VALUES; ++i)
	{
		i32 k = Core::HashCRC32(0, &i, sizeof(i));
		map[k] = i;
	}

	i32 total = 0;
	for(auto pair : map)
	{
		auto k = pair.key;
		auto v = pair.value;
		REQUIRE(map[k] == v);
		++total;
	}
	REQUIRE(total == NUM_VALUES);
}

TEST_CASE("map-tests-erase")
{
	Core::Map<u32, i32> map;
	const i32 NUM_VALUES = 4096;
	const i32 HALF_NUM_VALUES = NUM_VALUES / 2;

	for(i32 i = 0; i < NUM_VALUES; ++i)
	{
		map.insert(i, NUM_VALUES - i);
	}

	for(i32 i = 0; i < NUM_VALUES; ++i)
	{
		auto* found = map.find(i);
		REQUIRE(found);
		REQUIRE(*found == (NUM_VALUES - i));
	}

	for(i32 i = 0; i < NUM_VALUES; i += 4)
	{
		map.erase(i);
		REQUIRE(!map.find(i));
	}

	for(i32 i = 0; i < NUM_VALUES; i += 4)
	{
		REQUIRE(!map.find(i));
		map.insert(i, NUM_VALUES - i);

		auto* found = map.find(i);
		REQUIRE(found);
		REQUIRE(*found == (NUM_VALUES - i));
	}
}

TEST_CASE("map-tests-bad-hash")
{
	struct BadHasher
	{
		u64 operator()(u64 input, u32 data) const { return 7; };
	};

	Core::Map<u32, u32, BadHasher> map;

	for(u32 i = 0; i < 11; ++i)
	{
		map.insert(i, i);
	}

	for(u32 i = 0; i < 11; ++i)
	{
		REQUIRE(map.find(i));
		REQUIRE(*map.find(i) == i);
	}

	for(u32 i = 0; i < 11; i += 3)
	{
		map.erase(i);
	}

	for(u32 i = 0; i < 11; ++i)
	{
		map.insert(i, i);
	}

	for(u32 i = 0; i < 11; ++i)
	{
		REQUIRE(map.find(i));
		REQUIRE(*map.find(i) == i);
	}
}

TEST_CASE("map-tests-bench")
{
	const i32 NUM_ITERATIONS = 32;
	const i32 NUM_VALUES = 1024 * 32;

	SECTION("trivial")
	{
		Core::Log("<u32, i32>\n");
		Core::Map<u32, i32> mapA;
		std::unordered_map<u32, i32> mapB;

		// Insertion.
		{
			ScopedTimer timer("-           Core::Map insertion");

			for(i32 j = 0; j < NUM_ITERATIONS; ++j)
			{
				for(i32 i = 0; i < NUM_VALUES; ++i)
				{
					i32 k = Core::HashCRC32(0, &i, sizeof(i));
					mapA[k] = i;
				}
			}
		}

		{
			ScopedTimer timer("-  std::unordered_map insertion");

			for(i32 j = 0; j < NUM_ITERATIONS; ++j)
			{
				for(i32 i = 0; i < NUM_VALUES; ++i)
				{
					i32 k = Core::HashCRC32(0, &i, sizeof(i));
					mapB[k] = i;
				}
			}
		}

		Core::Log(" - - Average Probe Count: %f\n", mapA.AverageProbeCount());

		// Find.
		{
			ScopedTimer timer("-           Core::Map find");

			for(i32 j = 0; j < NUM_ITERATIONS; ++j)
			{
				for(i32 i = 0; i < NUM_VALUES; ++i)
				{
					i32 k = Core::HashCRC32(0, &i, sizeof(i));
					auto v = mapA.find(k);
					REQUIRE(*v == i);
				}
			}
		}

		{
			ScopedTimer timer("-  std::unordered_map find");

			for(i32 j = 0; j < NUM_ITERATIONS; ++j)
			{
				for(i32 i = 0; i < NUM_VALUES; ++i)
				{
					i32 k = Core::HashCRC32(0, &i, sizeof(i));
					auto it = mapB.find(k);
					REQUIRE(it->second == i);
				}
			}
		}

		// Verify.
		for(i32 i = 0; i < NUM_VALUES; ++i)
		{
			i32 k = Core::HashCRC32(0, &i, sizeof(i));
			bool match = mapA[k] == mapB[k];
			REQUIRE(match);
		}

		// Erase.
		{
			ScopedTimer timer("-           Core::Map erase");

			for(i32 i = 0; i < NUM_VALUES; ++i)
			{
				i32 k = Core::HashCRC32(0, &i, sizeof(i));
				mapA.erase(k);
			}
		}

		{
			ScopedTimer timer("-  std::unordered_map erase");

			for(i32 i = 0; i < NUM_VALUES; ++i)
			{
				i32 k = Core::HashCRC32(0, &i, sizeof(i));
				mapB.erase(k);
			}
		}
	}

	SECTION("non-trivial")
	{
		Core::Log("<Core::String, i32>\n");
		Core::Map<Core::String, i32> mapA;
		std::unordered_map<Core::String, i32> mapB;

		// Insertion.
		{
			ScopedTimer timer("-           Core::Map insertion");

			for(i32 j = 0; j < NUM_ITERATIONS; ++j)
			{
				for(i32 i = 0; i < NUM_VALUES; ++i)
				{
					auto k = IdxToVal_string(i);
					mapA[k] = i;
				}
			}
		}

		{
			ScopedTimer timer("-  std::unordered_map insertion");

			for(i32 j = 0; j < NUM_ITERATIONS; ++j)
			{
				for(i32 i = 0; i < NUM_VALUES; ++i)
				{
					auto k = IdxToVal_string(i);
					mapB[k] = i;
				}
			}
		}

		Core::Log(" - - Average Probe Count: %f\n", mapA.AverageProbeCount());

		// Find.
		{
			ScopedTimer timer("-           Core::Map find");

			for(i32 j = 0; j < NUM_ITERATIONS; ++j)
			{
				for(i32 i = 0; i < NUM_VALUES; ++i)
				{
					auto k = IdxToVal_string(i);
					auto v = mapA.find(k);
					CHECK(*v == i);
				}
			}
		}

		{
			ScopedTimer timer("-  std::unordered_map find");

			for(i32 j = 0; j < NUM_ITERATIONS; ++j)
			{
				for(i32 i = 0; i < NUM_VALUES; ++i)
				{
					auto k = IdxToVal_string(i);
					auto it = mapB.find(k);
					CHECK(it->second == i);
				}
			}
		}

		// Verify.
		for(i32 i = 0; i < NUM_VALUES; ++i)
		{
			auto k = IdxToVal_string(i);
			bool match = mapA[k] == mapB[k];
			REQUIRE(match);
		}

		// Erase.
		{
			ScopedTimer timer("-           Core::Map erase");

			for(i32 i = 0; i < NUM_VALUES; ++i)
			{
				auto k = IdxToVal_string(i);
				mapA.erase(k);
			}
		}

		{
			ScopedTimer timer("-  std::unordered_map erase");

			for(i32 i = 0; i < NUM_VALUES; ++i)
			{
				auto k = IdxToVal_string(i);
				mapB.erase(k);
			}
		}
	}
}