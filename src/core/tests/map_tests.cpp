#include "core/map.h"

#include "catch.hpp"

#include <string>
u32 Hash(u32 Input, const std::string& String)
{
	return Hash(Input, String.c_str());
}


namespace
{
	template<typename KEY_TYPE, typename VALUE_TYPE>
	void MapTestSize(KEY_TYPE(IdxToKey)(size_t), VALUE_TYPE(IdxToVal)(size_t))
	{
		Map<KEY_TYPE, VALUE_TYPE> TestMap;
		REQUIRE(TestMap.size() == 0);

		TestMap.insert(IdxToKey(0), IdxToVal(0));
		REQUIRE(TestMap.size() == 1);
	}

	template<typename KEY_TYPE, typename VALUE_TYPE, size_t SIZE>
	void MapTestInsert(KEY_TYPE(IdxToKey)(size_t), VALUE_TYPE(IdxToVal)(size_t))
	{
		Map<KEY_TYPE, VALUE_TYPE> TestMap;

		for(size_t Idx = 0; Idx < SIZE; ++Idx)
		{
			TestMap.insert(IdxToKey(Idx), IdxToVal(Idx));
		}
		REQUIRE(TestMap.size() == SIZE);

		for(size_t Idx = 0; Idx < SIZE; ++Idx)
		{
			TestMap.insert(IdxToKey(Idx), IdxToVal(Idx));
		}
		REQUIRE(TestMap.size() == SIZE);
	}

	template<typename KEY_TYPE, typename VALUE_TYPE, size_t SIZE>
	void MapTestFind(KEY_TYPE(IdxToKey)(size_t), VALUE_TYPE(IdxToVal)(size_t))
	{
		Map<KEY_TYPE, VALUE_TYPE> TestMap;

		for(size_t Idx = 0; Idx < SIZE; ++Idx)
		{
			TestMap.insert(IdxToKey(Idx), IdxToVal(Idx));
		}

		bool Success = true;
		for(size_t Idx = 0; Idx < SIZE; ++Idx)
		{
			auto Val = TestMap.find(IdxToKey(Idx));
			Success &= Val->First_ == IdxToKey(Idx);
			Success &= Val->Second_ == IdxToVal(Idx);
		}
		REQUIRE(Success);
	}

	template<typename KEY_TYPE, typename VALUE_TYPE, size_t SIZE>
	void MapTestOperatorInsert(KEY_TYPE(IdxToKey)(size_t), VALUE_TYPE(IdxToVal)(size_t))
	{
		Map<KEY_TYPE, VALUE_TYPE> TestMap;

		for(size_t Idx = 0; Idx < SIZE; ++Idx)
		{
			TestMap[IdxToKey(Idx)] = IdxToVal(Idx);
		}

		bool Success = true;
		for(size_t Idx = 0; Idx < SIZE; ++Idx)
		{
			auto Val = TestMap.find(IdxToKey(Idx));
			Success &= Val->First_ == IdxToKey(Idx);
			Success &= Val->Second_ == IdxToVal(Idx);
		}
		REQUIRE(Success);
	}

	template<typename KEY_TYPE, typename VALUE_TYPE, size_t SIZE>
	void MapTestOperatorFind(KEY_TYPE(IdxToKey)(size_t), VALUE_TYPE(IdxToVal)(size_t))
	{
		Map<KEY_TYPE, VALUE_TYPE> TestMap;

		for(size_t Idx = 0; Idx < SIZE; ++Idx)
		{
			TestMap[IdxToKey(Idx)] = IdxToVal(Idx);
		}

		bool Success = true;
		for(size_t Idx = 0; Idx < SIZE; ++Idx)
		{
			auto Val = TestMap[IdxToKey(Idx)];
			Success &= Val == IdxToVal(Idx);
		}
		REQUIRE(Success);
	}

	size_t IdxToVal_size_t(size_t Idx)
	{
		return Idx;
	}

	std::string IdxToVal_string(size_t Idx)
	{
		char Buffer[1024] = {0};
		sprintf_s(Buffer, sizeof(Buffer), "%u", Idx);
		return Buffer;
	}
}

TEST_CASE("map-tests-size")
{
	MapTestSize<size_t, size_t>(IdxToVal_size_t, IdxToVal_size_t);
}

TEST_CASE("map-tests-insert")
{
	SECTION("trivial")
	{
		MapTestInsert<size_t, size_t, 0x1>(IdxToVal_size_t, IdxToVal_size_t);
		MapTestInsert<size_t, size_t, 0x2>(IdxToVal_size_t, IdxToVal_size_t);
		MapTestInsert<size_t, size_t, 0xff>(IdxToVal_size_t, IdxToVal_size_t);
		MapTestInsert<size_t, size_t, 0x100>(IdxToVal_size_t, IdxToVal_size_t);
	}

	SECTION("non-trivial")
	{
		MapTestInsert<std::string, std::string, 0x1>(IdxToVal_string, IdxToVal_string);
		MapTestInsert<std::string, std::string, 0x2>(IdxToVal_string, IdxToVal_string);
		MapTestInsert<std::string, std::string, 0xff>(IdxToVal_string, IdxToVal_string);
		MapTestInsert<std::string, std::string, 0x100>(IdxToVal_string, IdxToVal_string);
	}
}


TEST_CASE("map-tests-find")
{
	SECTION("trivial")
	{
		MapTestFind<size_t, size_t, 0x1>(IdxToVal_size_t, IdxToVal_size_t);
		MapTestFind<size_t, size_t, 0x2>(IdxToVal_size_t, IdxToVal_size_t);
		MapTestFind<size_t, size_t, 0xff>(IdxToVal_size_t, IdxToVal_size_t);
		MapTestFind<size_t, size_t, 0x100>(IdxToVal_size_t, IdxToVal_size_t);
	}

	SECTION("non-trivial")
	{
		MapTestFind<std::string, std::string, 0x1>(IdxToVal_string, IdxToVal_string);
		MapTestFind<std::string, std::string, 0x2>(IdxToVal_string, IdxToVal_string);
		MapTestFind<std::string, std::string, 0xff>(IdxToVal_string, IdxToVal_string);
		MapTestFind<std::string, std::string, 0x100>(IdxToVal_string, IdxToVal_string);
	}
}

TEST_CASE("map-tests-operator-insert")
{
	SECTION("trivial")
	{
		MapTestOperatorInsert<size_t, size_t, 0x1>(IdxToVal_size_t, IdxToVal_size_t);
		MapTestOperatorInsert<size_t, size_t, 0x2>(IdxToVal_size_t, IdxToVal_size_t);
		MapTestOperatorInsert<size_t, size_t, 0xff>(IdxToVal_size_t, IdxToVal_size_t);
		MapTestOperatorInsert<size_t, size_t, 0x100>(IdxToVal_size_t, IdxToVal_size_t);
	}

	SECTION("non-trivial")
	{
		MapTestOperatorInsert<std::string, std::string, 0x1>(IdxToVal_string, IdxToVal_string);
		MapTestOperatorInsert<std::string, std::string, 0x2>(IdxToVal_string, IdxToVal_string);
		MapTestOperatorInsert<std::string, std::string, 0xff>(IdxToVal_string, IdxToVal_string);
		MapTestOperatorInsert<std::string, std::string, 0x100>(IdxToVal_string, IdxToVal_string);
	}
}


TEST_CASE("map-tests-operator-find")
{
	SECTION("trivial")
	{
		MapTestOperatorFind<size_t, size_t, 0x1>(IdxToVal_size_t, IdxToVal_size_t);
		MapTestOperatorFind<size_t, size_t, 0x2>(IdxToVal_size_t, IdxToVal_size_t);
		MapTestOperatorFind<size_t, size_t, 0xff>(IdxToVal_size_t, IdxToVal_size_t);
		MapTestOperatorFind<size_t, size_t, 0x100>(IdxToVal_size_t, IdxToVal_size_t);
	}

	SECTION("non-trivial")
	{
		MapTestOperatorFind<std::string, std::string, 0x1>(IdxToVal_string, IdxToVal_string);
		MapTestOperatorFind<std::string, std::string, 0x2>(IdxToVal_string, IdxToVal_string);
		MapTestOperatorFind<std::string, std::string, 0xff>(IdxToVal_string, IdxToVal_string);
		MapTestOperatorFind<std::string, std::string, 0x100>(IdxToVal_string, IdxToVal_string);
	}
}




