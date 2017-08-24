#include "core/array.h"
#include "core/vector.h"

#include "catch.hpp"

using namespace Core;

namespace
{
	typedef i32 index_type;

	template<typename TYPE, index_type ARRAY_SIZE>
	void ArrayTestSize()
	{
		Array<TYPE, ARRAY_SIZE> TestArray;

		REQUIRE(TestArray.size() == ARRAY_SIZE);
	}

	template<typename TYPE, index_type ARRAY_SIZE>
	void ArrayTestFill(TYPE(IdxToVal)(index_type))
	{
		Array<TYPE, ARRAY_SIZE> TestArray;

		bool Success = true;
		const index_type FILL_VAL = 123;
		TestArray.fill(IdxToVal(FILL_VAL));
		for(index_type Idx = 0; Idx < ARRAY_SIZE; ++Idx)
			Success &= (TestArray[Idx] == IdxToVal(FILL_VAL));
		REQUIRE(Success);
	}

	template<typename TYPE, index_type ARRAY_SIZE>
	void ArrayTestOperatorAssignment(TYPE(IdxToVal)(index_type))
	{
		Array<TYPE, ARRAY_SIZE> TestArray;

		bool Success = true;
		for(index_type Idx = 0; Idx < ARRAY_SIZE; ++Idx)
			TestArray[Idx] = IdxToVal(Idx);
		for(index_type Idx = 0; Idx < ARRAY_SIZE; ++Idx)
			Success &= (TestArray[Idx] == IdxToVal(Idx));
		REQUIRE(Success);
	}

	template<typename TYPE, index_type ARRAY_SIZE>
	void ArrayTestCopy(TYPE(IdxToVal)(index_type))
	{
		Array<TYPE, ARRAY_SIZE> TestArray = {};
		Array<TYPE, ARRAY_SIZE> TestArray2 = {};

		bool Success = true;
		for(index_type Idx = 0; Idx < ARRAY_SIZE; ++Idx)
			TestArray[Idx] = IdxToVal(Idx);
		TestArray2 = TestArray;
		for(index_type Idx = 0; Idx < ARRAY_SIZE; ++Idx)
			Success &= (TestArray2[Idx] == IdxToVal(Idx));
		REQUIRE(Success);
	}

	template<typename TYPE, index_type ARRAY_SIZE>
	void ArrayTestMove(TYPE(IdxToVal)(index_type))
	{
		Array<TYPE, ARRAY_SIZE> TestArray;
		Array<TYPE, ARRAY_SIZE> TestArray2;

		bool Success = true;
		for(index_type Idx = 0; Idx < ARRAY_SIZE; ++Idx)
			TestArray[Idx] = IdxToVal(Idx);
		TestArray2 = std::move(TestArray);
		for(index_type Idx = 0; Idx < ARRAY_SIZE; ++Idx)
			Success &= (TestArray2[Idx] == IdxToVal(Idx));
		REQUIRE(Success);
	}

	template<typename TYPE, index_type ARRAY_SIZE>
	void ArrayTestDataAssignment(TYPE(IdxToVal)(index_type))
	{
		Array<TYPE, ARRAY_SIZE> TestArray;

		bool Success = true;
		for(index_type Idx = 0; Idx < ARRAY_SIZE; ++Idx)
			TestArray.data()[Idx] = IdxToVal(Idx);
		for(index_type Idx = 0; Idx < ARRAY_SIZE; ++Idx)
			Success &= (TestArray[Idx] == IdxToVal(Idx));
		REQUIRE(Success);
	}

	template<typename TYPE, index_type ARRAY_SIZE>
	void ArrayTestIteratorAssignment(TYPE(IdxToVal)(index_type))
	{
		Array<TYPE, ARRAY_SIZE> TestArray;

		bool Success = true;
		index_type Idx = 0;
		for(auto& It : TestArray)
			It = IdxToVal(Idx++);
		for(Idx = 0; Idx < ARRAY_SIZE; ++Idx)
			Success &= (TestArray[Idx] == IdxToVal(Idx));
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

TEST_CASE("array-tests-size")
{
	ArrayTestSize<index_type, 0x1>();
	ArrayTestSize<index_type, 0x2>();
	ArrayTestSize<index_type, 0xff>();
	ArrayTestSize<index_type, 0x100>();
	ArrayTestSize<index_type, 0xffff>();
	ArrayTestSize<index_type, 0x10000>();
}

TEST_CASE("array-tests-fill")
{
	SECTION("trivial")
	{
		ArrayTestFill<index_type, 0x1>(IdxToVal_index_type);
		ArrayTestFill<index_type, 0x2>(IdxToVal_index_type);
		ArrayTestFill<index_type, 0xff>(IdxToVal_index_type);
		ArrayTestFill<index_type, 0x100>(IdxToVal_index_type);
	}

	SECTION("non-trivial")
	{
		ArrayTestFill<std::string, 0x1>(IdxToVal_string);
		ArrayTestFill<std::string, 0x2>(IdxToVal_string);
		ArrayTestFill<std::string, 0xff>(IdxToVal_string);
		ArrayTestFill<std::string, 0x100>(IdxToVal_string);
	}
}

TEST_CASE("array-tests-operator-assignment")
{
	SECTION("trivial")
	{
		ArrayTestOperatorAssignment<index_type, 0x1>(IdxToVal_index_type);
		ArrayTestOperatorAssignment<index_type, 0x2>(IdxToVal_index_type);
		ArrayTestOperatorAssignment<index_type, 0xff>(IdxToVal_index_type);
		ArrayTestOperatorAssignment<index_type, 0x100>(IdxToVal_index_type);
	}

	SECTION("non-trivial")
	{
		ArrayTestOperatorAssignment<std::string, 0x1>(IdxToVal_string);
		ArrayTestOperatorAssignment<std::string, 0x2>(IdxToVal_string);
		ArrayTestOperatorAssignment<std::string, 0xff>(IdxToVal_string);
		ArrayTestOperatorAssignment<std::string, 0x100>(IdxToVal_string);
	}
}

TEST_CASE("array-tests-copy")
{
	SECTION("trivial")
	{
		ArrayTestCopy<index_type, 0x1>(IdxToVal_index_type);
		ArrayTestCopy<index_type, 0x2>(IdxToVal_index_type);
		ArrayTestCopy<index_type, 0xff>(IdxToVal_index_type);
		ArrayTestCopy<index_type, 0x100>(IdxToVal_index_type);
	}

	SECTION("non-trivial")
	{
		ArrayTestCopy<std::string, 0x1>(IdxToVal_string);
		ArrayTestCopy<std::string, 0x2>(IdxToVal_string);
		ArrayTestCopy<std::string, 0xff>(IdxToVal_string);
		ArrayTestCopy<std::string, 0x100>(IdxToVal_string);
	}
}

TEST_CASE("array-tests-move")
{
	SECTION("trivial")
	{
		ArrayTestMove<index_type, 0x1>(IdxToVal_index_type);
		ArrayTestMove<index_type, 0x2>(IdxToVal_index_type);
		ArrayTestMove<index_type, 0xff>(IdxToVal_index_type);
		ArrayTestMove<index_type, 0x100>(IdxToVal_index_type);
	}

	SECTION("non-trivial")
	{
		ArrayTestMove<std::string, 0x1>(IdxToVal_string);
		ArrayTestMove<std::string, 0x2>(IdxToVal_string);
		ArrayTestMove<std::string, 0xff>(IdxToVal_string);
		ArrayTestMove<std::string, 0x100>(IdxToVal_string);
	}
}

TEST_CASE("array-tests-data-assignment")
{
	SECTION("trivial")
	{
		ArrayTestDataAssignment<index_type, 0x1>(IdxToVal_index_type);
		ArrayTestDataAssignment<index_type, 0x2>(IdxToVal_index_type);
		ArrayTestDataAssignment<index_type, 0xff>(IdxToVal_index_type);
		ArrayTestDataAssignment<index_type, 0x100>(IdxToVal_index_type);
	}

	SECTION("non-trivial")
	{
		ArrayTestDataAssignment<std::string, 0x1>(IdxToVal_string);
		ArrayTestDataAssignment<std::string, 0x2>(IdxToVal_string);
		ArrayTestDataAssignment<std::string, 0xff>(IdxToVal_string);
		ArrayTestDataAssignment<std::string, 0x100>(IdxToVal_string);
	}
}

TEST_CASE("array-tests-iterator-assignment")
{
	SECTION("trivial")
	{
		ArrayTestIteratorAssignment<index_type, 0x1>(IdxToVal_index_type);
		ArrayTestIteratorAssignment<index_type, 0x2>(IdxToVal_index_type);
		ArrayTestIteratorAssignment<index_type, 0xff>(IdxToVal_index_type);
		ArrayTestIteratorAssignment<index_type, 0x100>(IdxToVal_index_type);
	}

	SECTION("non-trivial")
	{
		ArrayTestIteratorAssignment<std::string, 0x1>(IdxToVal_string);
		ArrayTestIteratorAssignment<std::string, 0x2>(IdxToVal_string);
		ArrayTestIteratorAssignment<std::string, 0xff>(IdxToVal_string);
		ArrayTestIteratorAssignment<std::string, 0x100>(IdxToVal_string);
	}
}
