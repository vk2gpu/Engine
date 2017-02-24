#include "core/map.h"

#include "catch.hpp"

#include <string>
u32 Hash(u32 Input, const std::string& String)
{
	return Hash(Input, String.c_str());
}


namespace
{
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

		for(index_type Idx = 0; Idx < SIZE; ++Idx)
		{
			TestMap.insert(IdxToKey(Idx), IdxToVal(Idx));
		}
		REQUIRE(TestMap.size() == SIZE);

		for(index_type Idx = 0; Idx < SIZE; ++Idx)
		{
			TestMap.insert(IdxToKey(Idx), IdxToVal(Idx));
		}
		REQUIRE(TestMap.size() == SIZE);
	}

	template<typename KEY_TYPE, typename VALUE_TYPE, index_type SIZE>
	void MapTestFind(KEY_TYPE(IdxToKey)(index_type), VALUE_TYPE(IdxToVal)(index_type))
	{
		Map<KEY_TYPE, VALUE_TYPE> TestMap;

		for(index_type Idx = 0; Idx < SIZE; ++Idx)
		{
			TestMap.insert(IdxToKey(Idx), IdxToVal(Idx));
		}

		bool Success = true;
		for(index_type Idx = 0; Idx < SIZE; ++Idx)
		{
			auto Val = TestMap.find(IdxToKey(Idx));
			Success &= Val->First_ == IdxToKey(Idx);
			Success &= Val->Second_ == IdxToVal(Idx);
		}
		REQUIRE(Success);
	}

	template<typename KEY_TYPE, typename VALUE_TYPE, index_type SIZE>
	void MapTestOperatorInsert(KEY_TYPE(IdxToKey)(index_type), VALUE_TYPE(IdxToVal)(index_type))
	{
		Map<KEY_TYPE, VALUE_TYPE> TestMap;

		for(index_type Idx = 0; Idx < SIZE; ++Idx)
		{
			TestMap[IdxToKey(Idx)] = IdxToVal(Idx);
		}

		bool Success = true;
		for(index_type Idx = 0; Idx < SIZE; ++Idx)
		{
			auto Val = TestMap.find(IdxToKey(Idx));
			Success &= Val->First_ == IdxToKey(Idx);
			Success &= Val->Second_ == IdxToVal(Idx);
		}
		REQUIRE(Success);
	}

	template<typename KEY_TYPE, typename VALUE_TYPE, index_type SIZE>
	void MapTestOperatorFind(KEY_TYPE(IdxToKey)(index_type), VALUE_TYPE(IdxToVal)(index_type))
	{
		Map<KEY_TYPE, VALUE_TYPE> TestMap;

		for(index_type Idx = 0; Idx < SIZE; ++Idx)
		{
			TestMap[IdxToKey(Idx)] = IdxToVal(Idx);
		}

		bool Success = true;
		for(index_type Idx = 0; Idx < SIZE; ++Idx)
		{
			auto Val = TestMap[IdxToKey(Idx)];
			Success &= Val == IdxToVal(Idx);
		}
		REQUIRE(Success);
	}

	index_type IdxToVal_index_type(index_type Idx) { return Idx; }

	std::string IdxToVal_string(index_type Idx)
	{
		char Buffer[1024] = {0};
		sprintf_s(Buffer, sizeof(Buffer), "%u", (int)Idx);
		return Buffer;
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
		MapTestFind<index_type, index_type, 0x1>(IdxToVal_index_type, IdxToVal_index_type);
		MapTestFind<index_type, index_type, 0x2>(IdxToVal_index_type, IdxToVal_index_type);
		MapTestFind<index_type, index_type, 0xff>(IdxToVal_index_type, IdxToVal_index_type);
		MapTestFind<index_type, index_type, 0x100>(IdxToVal_index_type, IdxToVal_index_type);
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
		MapTestOperatorInsert<index_type, index_type, 0x1>(IdxToVal_index_type, IdxToVal_index_type);
		MapTestOperatorInsert<index_type, index_type, 0x2>(IdxToVal_index_type, IdxToVal_index_type);
		MapTestOperatorInsert<index_type, index_type, 0xff>(IdxToVal_index_type, IdxToVal_index_type);
		MapTestOperatorInsert<index_type, index_type, 0x100>(IdxToVal_index_type, IdxToVal_index_type);
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
		MapTestOperatorFind<index_type, index_type, 0x1>(IdxToVal_index_type, IdxToVal_index_type);
		MapTestOperatorFind<index_type, index_type, 0x2>(IdxToVal_index_type, IdxToVal_index_type);
		MapTestOperatorFind<index_type, index_type, 0xff>(IdxToVal_index_type, IdxToVal_index_type);
		MapTestOperatorFind<index_type, index_type, 0x100>(IdxToVal_index_type, IdxToVal_index_type);
	}

	SECTION("non-trivial")
	{
		MapTestOperatorFind<std::string, std::string, 0x1>(IdxToVal_string, IdxToVal_string);
		MapTestOperatorFind<std::string, std::string, 0x2>(IdxToVal_string, IdxToVal_string);
		MapTestOperatorFind<std::string, std::string, 0xff>(IdxToVal_string, IdxToVal_string);
		MapTestOperatorFind<std::string, std::string, 0x100>(IdxToVal_string, IdxToVal_string);
	}
}
