#include "core/array.h"
#include "core/vector.h"

#include "catch.hpp"

namespace
{
	template<typename TYPE, size_t ARRAY_SIZE>
	void ArrayTestSize()
	{
		Array<TYPE, ARRAY_SIZE> TestArray;

		REQUIRE(TestArray.size() == ARRAY_SIZE);
	}

	template<typename TYPE, size_t ARRAY_SIZE>
	void ArrayTestFill(TYPE(IdxToVal)(size_t))
	{
		Array<TYPE, ARRAY_SIZE> TestArray;

		bool Success = true;
		const size_t FILL_VAL = 123;
		TestArray.fill(IdxToVal(FILL_VAL));
		for(size_t Idx = 0; Idx < ARRAY_SIZE; ++Idx)
			Success &= (TestArray[Idx] == IdxToVal(FILL_VAL));
		REQUIRE(Success);
	}

	template<typename TYPE, size_t ARRAY_SIZE>
	void ArrayTestOperatorAssignment(TYPE(IdxToVal)(size_t))
	{
		Array<TYPE, ARRAY_SIZE> TestArray;

		bool Success = true;
		for(size_t Idx = 0; Idx < ARRAY_SIZE; ++Idx)
			TestArray[Idx] = IdxToVal(Idx);
		for(size_t Idx = 0; Idx < ARRAY_SIZE; ++Idx)
			Success &= (TestArray[Idx] == IdxToVal(Idx));
		REQUIRE(Success);
	}

	template<typename TYPE, size_t ARRAY_SIZE>
	void ArrayTestCopy(TYPE(IdxToVal)(size_t))
	{
		Array<TYPE, ARRAY_SIZE> TestArray;
		Array<TYPE, ARRAY_SIZE> TestArray2;

		bool Success = true;
		for(size_t Idx = 0; Idx < ARRAY_SIZE; ++Idx)
			TestArray[Idx] = IdxToVal(Idx);
		TestArray2 = TestArray;
		for(size_t Idx = 0; Idx < ARRAY_SIZE; ++Idx)
			Success &= (TestArray2[Idx] == IdxToVal(Idx));
		REQUIRE(Success);
	}

	template<typename TYPE, size_t ARRAY_SIZE>
	void ArrayTestMove(TYPE(IdxToVal)(size_t))
	{
		Array<TYPE, ARRAY_SIZE> TestArray;
		Array<TYPE, ARRAY_SIZE> TestArray2;

		bool Success = true;
		for(size_t Idx = 0; Idx < ARRAY_SIZE; ++Idx)
			TestArray[Idx] = IdxToVal(Idx);
		TestArray2 = std::move(TestArray);
		for(size_t Idx = 0; Idx < ARRAY_SIZE; ++Idx)
			Success &= (TestArray2[Idx] == IdxToVal(Idx));
		REQUIRE(Success);
	}

	template<typename TYPE, size_t ARRAY_SIZE>
	void ArrayTestDataAssignment(TYPE(IdxToVal)(size_t))
	{
		Array<TYPE, ARRAY_SIZE> TestArray;

		bool Success = true;
		for(size_t Idx = 0; Idx < ARRAY_SIZE; ++Idx)
			TestArray.data()[Idx] = IdxToVal(Idx);
		for(size_t Idx = 0; Idx < ARRAY_SIZE; ++Idx)
			Success &= (TestArray[Idx] == IdxToVal(Idx));
		REQUIRE(Success);
	}

	template<typename TYPE, size_t ARRAY_SIZE>
	void ArrayTestIteratorAssignment(TYPE(IdxToVal)(size_t))
	{
		Array<TYPE, ARRAY_SIZE> TestArray;

		bool Success = true;
		size_t Idx = 0;
		for(auto& It : TestArray)
			It = IdxToVal(Idx++);
		for(size_t Idx = 0; Idx < ARRAY_SIZE; ++Idx)
			Success &= (TestArray[Idx] == IdxToVal(Idx));
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

TEST_CASE("array-tests-size")
{
	ArrayTestSize<size_t, 0x1>();
	ArrayTestSize<size_t, 0x2>();
	ArrayTestSize<size_t, 0xff>();
	ArrayTestSize<size_t, 0x100>();
	ArrayTestSize<size_t, 0xffff>();
	ArrayTestSize<size_t, 0x10000>();
}

TEST_CASE("array-tests-fill")
{
	SECTION("trivial")
	{
		ArrayTestFill<size_t, 0x1>(IdxToVal_size_t);
		ArrayTestFill<size_t, 0x2>(IdxToVal_size_t);
		ArrayTestFill<size_t, 0xff>(IdxToVal_size_t);
		ArrayTestFill<size_t, 0x100>(IdxToVal_size_t);
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
		ArrayTestOperatorAssignment<size_t, 0x1>(IdxToVal_size_t);
		ArrayTestOperatorAssignment<size_t, 0x2>(IdxToVal_size_t);
		ArrayTestOperatorAssignment<size_t, 0xff>(IdxToVal_size_t);
		ArrayTestOperatorAssignment<size_t, 0x100>(IdxToVal_size_t);
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
		ArrayTestCopy<size_t, 0x1>(IdxToVal_size_t);
		ArrayTestCopy<size_t, 0x2>(IdxToVal_size_t);
		ArrayTestCopy<size_t, 0xff>(IdxToVal_size_t);
		ArrayTestCopy<size_t, 0x100>(IdxToVal_size_t);
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
		ArrayTestMove<size_t, 0x1>(IdxToVal_size_t);
		ArrayTestMove<size_t, 0x2>(IdxToVal_size_t);
		ArrayTestMove<size_t, 0xff>(IdxToVal_size_t);
		ArrayTestMove<size_t, 0x100>(IdxToVal_size_t);
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
		ArrayTestDataAssignment<size_t, 0x1>(IdxToVal_size_t);
		ArrayTestDataAssignment<size_t, 0x2>(IdxToVal_size_t);
		ArrayTestDataAssignment<size_t, 0xff>(IdxToVal_size_t);
		ArrayTestDataAssignment<size_t, 0x100>(IdxToVal_size_t);
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
		ArrayTestIteratorAssignment<size_t, 0x1>(IdxToVal_size_t);
		ArrayTestIteratorAssignment<size_t, 0x2>(IdxToVal_size_t);
		ArrayTestIteratorAssignment<size_t, 0xff>(IdxToVal_size_t);
		ArrayTestIteratorAssignment<size_t, 0x100>(IdxToVal_size_t);
	}

	SECTION("non-trivial")
	{
		ArrayTestIteratorAssignment<std::string, 0x1>(IdxToVal_string);
		ArrayTestIteratorAssignment<std::string, 0x2>(IdxToVal_string);
		ArrayTestIteratorAssignment<std::string, 0xff>(IdxToVal_string);
		ArrayTestIteratorAssignment<std::string, 0x100>(IdxToVal_string);
	}
}
