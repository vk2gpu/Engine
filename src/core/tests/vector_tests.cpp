#include "core/array.h"
#include "core/vector.h"

#include "catch.hpp"

namespace
{
	template<typename TYPE, size_t ARRAY_SIZE>
	void VectorTestSize()
	{
		Vector<TYPE> TestArray;

		REQUIRE(TestArray.size() == 0);
		REQUIRE(TestArray.capacity() >= TestArray.size());

		TestArray.resize(ARRAY_SIZE);
		REQUIRE(TestArray.size() == ARRAY_SIZE);
		REQUIRE(TestArray.capacity() >= TestArray.size());
	}

	template<typename TYPE, size_t ARRAY_SIZE>
	void VectorTestFill(TYPE(IdxToVal)(size_t))
	{
		Vector<TYPE> TestArray;
		TestArray.resize(ARRAY_SIZE);

		bool Success = true;
		const size_t FILL_VAL = 123;
		TestArray.fill(IdxToVal(FILL_VAL));
		for(size_t Idx = 0; Idx < ARRAY_SIZE; ++Idx)
			Success &= (TestArray[Idx] == IdxToVal(FILL_VAL));
		REQUIRE(Success);
	}

	template<typename TYPE, size_t ARRAY_SIZE>
	void VectorTestPushBack(TYPE(IdxToVal)(size_t))
	{
		Vector<TYPE> TestArray;
		bool Success = true;
		for(size_t Idx = 0; Idx < ARRAY_SIZE; ++Idx)
			TestArray.push_back(IdxToVal(Idx));
		for(size_t Idx = 0; Idx < ARRAY_SIZE; ++Idx)
			Success &= (TestArray[Idx] == IdxToVal(Idx));
		REQUIRE(Success);
	}

	template<typename TYPE, size_t ARRAY_SIZE>
	void VectorTestPushBackReserve(TYPE(IdxToVal)(size_t))
	{
		Vector<TYPE> TestArray;
		TestArray.reserve(ARRAY_SIZE);

		REQUIRE(TestArray.capacity() >= ARRAY_SIZE);

		bool Success = true;
		for(size_t Idx = 0; Idx < ARRAY_SIZE; ++Idx)
			TestArray.push_back(IdxToVal(Idx));
		for(size_t Idx = 0; Idx < ARRAY_SIZE; ++Idx)
			Success &= (TestArray[Idx] == IdxToVal(Idx));
		REQUIRE(Success);
	}

	template<typename TYPE, size_t ARRAY_SIZE>
	void VectorTestOperatorAssignment(TYPE(IdxToVal)(size_t))
	{
		Vector<TYPE> TestArray;
		TestArray.resize(ARRAY_SIZE);

		bool Success = true;
		for(size_t Idx = 0; Idx < ARRAY_SIZE; ++Idx)
			TestArray[Idx] = IdxToVal(Idx);
		for(size_t Idx = 0; Idx < ARRAY_SIZE; ++Idx)
			Success &= (TestArray[Idx] == IdxToVal(Idx));
		REQUIRE(Success);
	}

	template<typename TYPE, size_t ARRAY_SIZE>
	void VectorTestCopy(TYPE(IdxToVal)(size_t))
	{
		Vector<TYPE> TestArray;
		Vector<TYPE> TestArray2;
		TestArray.resize(ARRAY_SIZE);

		bool Success = true;
		for(size_t Idx = 0; Idx < ARRAY_SIZE; ++Idx)
			TestArray[Idx] = IdxToVal(Idx);
		TestArray2 = TestArray;
		REQUIRE(TestArray.size() == ARRAY_SIZE);
		REQUIRE(TestArray2.size() == ARRAY_SIZE);
		for(size_t Idx = 0; Idx < ARRAY_SIZE; ++Idx)
			Success &= (TestArray2[Idx] == IdxToVal(Idx));
		REQUIRE(Success);
	}

	template<typename TYPE, size_t ARRAY_SIZE>
	void VectorTestMove(TYPE(IdxToVal)(size_t))
	{
		Vector<TYPE> TestArray;
		Vector<TYPE> TestArray2;
		TestArray.resize(ARRAY_SIZE);

		bool Success = true;
		for(size_t Idx = 0; Idx < ARRAY_SIZE; ++Idx)
			TestArray[Idx] = IdxToVal(Idx);
		TestArray2 = std::move(TestArray);
		REQUIRE(TestArray.size() == 0);
		REQUIRE(TestArray2.size() == ARRAY_SIZE);
		for(size_t Idx = 0; Idx < ARRAY_SIZE; ++Idx)
			Success &= (TestArray2[Idx] == IdxToVal(Idx));
		REQUIRE(Success);
	}

	template<typename TYPE, size_t ARRAY_SIZE>
	void VectorTestDataAssignment(TYPE(IdxToVal)(size_t))
	{
		Vector<TYPE> TestArray;
		TestArray.resize(ARRAY_SIZE);

		bool Success = true;
		for(size_t Idx = 0; Idx < ARRAY_SIZE; ++Idx)
			TestArray.data()[Idx] = IdxToVal(Idx);
		for(size_t Idx = 0; Idx < ARRAY_SIZE; ++Idx)
			Success &= (TestArray[Idx] == IdxToVal(Idx));
		REQUIRE(Success);
	}

	template<typename TYPE, size_t ARRAY_SIZE>
	void VectorTestIteratorAssignment(TYPE(IdxToVal)(size_t))
	{
		Vector<TYPE> TestArray;
		TestArray.resize(ARRAY_SIZE);

		bool Success = true;
		size_t Idx = 0;
		for(auto& It : TestArray)
			It = IdxToVal(Idx++);
		for(size_t Idx = 0; Idx < ARRAY_SIZE; ++Idx)
			Success &= (TestArray[Idx] == IdxToVal(Idx));
		REQUIRE(Success);
	}

	template<typename TYPE, size_t ARRAY_SIZE>
	void VectorTestShrinkToFit(TYPE(IdxToVal)(size_t))
	{
		Vector<TYPE> TestArray;
		TestArray.resize(ARRAY_SIZE);
		TestArray.push_back(TYPE());

		TestArray.shrink_to_fit();

		REQUIRE(TestArray.size() == TestArray.capacity());
	}

	template<typename TYPE, size_t ARRAY_SIZE>
	void VectorTestErase(TYPE(IdxToVal)(size_t))
	{
		Vector<TYPE> TestArray;

		// Erase from beginning.
		{
			TestArray.resize(ARRAY_SIZE);
			size_t Idx = 0;
			for(auto& It : TestArray)
				It = IdxToVal(Idx++);

			auto It = TestArray.begin();
			Idx = 0;
			bool Success = true;
			while(It != TestArray.end())
			{
				Success &= (TestArray[0] == IdxToVal(Idx));
				Success &= (*It == IdxToVal(Idx));
				It = TestArray.erase(It);
				++Idx;
			}
			REQUIRE(Success);
		}

		// Erase from end.
		{
			TestArray.resize(ARRAY_SIZE);
			size_t Idx = 0;
			for(auto& It : TestArray)
				It = IdxToVal(Idx++);

			Idx = TestArray.size() - 1;
			auto It = TestArray.begin() + Idx;
			bool Success = true;

			Success &= (TestArray.back() == IdxToVal(Idx));
			Success &= (*It == IdxToVal(Idx));
			It = TestArray.erase(It);
			Success &= It == TestArray.end();

			REQUIRE(Success);
		}

		// Erase from middle.
		{
			TestArray.resize(ARRAY_SIZE);
			size_t Idx = 0;
			for(auto& It : TestArray)
				It = IdxToVal(Idx++);

			Idx = TestArray.size() / 2;
			auto It = TestArray.begin() + Idx;
			bool Success = true;
			while(It != TestArray.end())
			{
				Success &= (*It == IdxToVal(Idx));
				It = TestArray.erase(It);
				++Idx;
			}
			REQUIRE(Success);
		}
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


TEST_CASE("vector-tests-size")
{
	VectorTestSize<size_t, 0x1>();
	VectorTestSize<size_t, 0x2>();
	VectorTestSize<size_t, 0xff>();
	VectorTestSize<size_t, 0x100>();
	VectorTestSize<size_t, 0xffff>();
	VectorTestSize<size_t, 0x10000>();
}

TEST_CASE("vector-tests-fill")
{
	SECTION("trivial")
	{
		VectorTestFill<size_t, 0x1>(IdxToVal_size_t);
		VectorTestFill<size_t, 0x2>(IdxToVal_size_t);
		VectorTestFill<size_t, 0xff>(IdxToVal_size_t);
		VectorTestFill<size_t, 0x100>(IdxToVal_size_t);
	}

	SECTION("non-trivial")
	{
		VectorTestFill<std::string, 0x1>(IdxToVal_string);
		VectorTestFill<std::string, 0x2>(IdxToVal_string);
		VectorTestFill<std::string, 0xff>(IdxToVal_string);
		VectorTestFill<std::string, 0x100>(IdxToVal_string);
	}
}

TEST_CASE("vector-tests-push-back")
{
	SECTION("trivial")
	{
		VectorTestPushBack<size_t, 0x1>(IdxToVal_size_t);
		VectorTestPushBack<size_t, 0x2>(IdxToVal_size_t);
		VectorTestPushBack<size_t, 0xff>(IdxToVal_size_t);
		VectorTestPushBack<size_t, 0x100>(IdxToVal_size_t);
	}

	SECTION("non-trivial")
	{
		VectorTestPushBack<std::string, 0x1>(IdxToVal_string);
		VectorTestPushBack<std::string, 0x2>(IdxToVal_string);
		VectorTestPushBack<std::string, 0xff>(IdxToVal_string);
		VectorTestOperatorAssignment<std::string, 0x100>(IdxToVal_string);
	}
}

TEST_CASE("vector-tests-push-back-reserve")
{
	SECTION("trivial")
	{
		VectorTestPushBackReserve<size_t, 0x1>(IdxToVal_size_t);
		VectorTestPushBackReserve<size_t, 0x2>(IdxToVal_size_t);
		VectorTestPushBackReserve<size_t, 0xff>(IdxToVal_size_t);
		VectorTestPushBackReserve<size_t, 0x100>(IdxToVal_size_t);
	}

	SECTION("non-trivial")
	{
		VectorTestPushBackReserve<std::string, 0x1>(IdxToVal_string);
		VectorTestPushBackReserve<std::string, 0x2>(IdxToVal_string);
		VectorTestPushBackReserve<std::string, 0xff>(IdxToVal_string);
		VectorTestPushBackReserve<std::string, 0x100>(IdxToVal_string);
	}
}

TEST_CASE("vector-tests-operator-assignment")
{
	SECTION("trivial")
	{
		VectorTestOperatorAssignment<size_t, 0x1>(IdxToVal_size_t);
		VectorTestOperatorAssignment<size_t, 0x2>(IdxToVal_size_t);
		VectorTestOperatorAssignment<size_t, 0xff>(IdxToVal_size_t);
		VectorTestOperatorAssignment<size_t, 0x100>(IdxToVal_size_t);
	}

	SECTION("non-trivial")
	{
		VectorTestOperatorAssignment<std::string, 0x1>(IdxToVal_string);
		VectorTestOperatorAssignment<std::string, 0x2>(IdxToVal_string);
		VectorTestOperatorAssignment<std::string, 0xff>(IdxToVal_string);
		VectorTestOperatorAssignment<std::string, 0x100>(IdxToVal_string);
	}
}

TEST_CASE("vector-tests-copy")
{
	SECTION("trivial")
	{
		VectorTestCopy<size_t, 0x1>(IdxToVal_size_t);
		VectorTestCopy<size_t, 0x2>(IdxToVal_size_t);
		VectorTestCopy<size_t, 0xff>(IdxToVal_size_t);
		VectorTestCopy<size_t, 0x100>(IdxToVal_size_t);
	}

	SECTION("non-trivial")
	{
		VectorTestCopy<std::string, 0x1>(IdxToVal_string);
		VectorTestCopy<std::string, 0x2>(IdxToVal_string);
		VectorTestCopy<std::string, 0xff>(IdxToVal_string);
		VectorTestCopy<std::string, 0x100>(IdxToVal_string);
	}
}

TEST_CASE("vector-tests-move")
{
	SECTION("trivial")
	{
		VectorTestMove<size_t, 0x1>(IdxToVal_size_t);
		VectorTestMove<size_t, 0x2>(IdxToVal_size_t);
		VectorTestMove<size_t, 0xff>(IdxToVal_size_t);
		VectorTestMove<size_t, 0x100>(IdxToVal_size_t);
	}

	SECTION("non-trivial")
	{
		VectorTestMove<std::string, 0x1>(IdxToVal_string);
		VectorTestMove<std::string, 0x2>(IdxToVal_string);
		VectorTestMove<std::string, 0xff>(IdxToVal_string);
		VectorTestMove<std::string, 0x100>(IdxToVal_string);
	}
}

TEST_CASE("vector-tests-data-assignment")
{
	SECTION("trivial")
	{
		VectorTestDataAssignment<size_t, 0x1>(IdxToVal_size_t);
		VectorTestDataAssignment<size_t, 0x2>(IdxToVal_size_t);
		VectorTestDataAssignment<size_t, 0xff>(IdxToVal_size_t);
		VectorTestDataAssignment<size_t, 0x100>(IdxToVal_size_t);
	}

	SECTION("non-trivial")
	{
		VectorTestDataAssignment<std::string, 0x1>(IdxToVal_string);
		VectorTestDataAssignment<std::string, 0x2>(IdxToVal_string);
		VectorTestDataAssignment<std::string, 0xff>(IdxToVal_string);
		VectorTestDataAssignment<std::string, 0x100>(IdxToVal_string);
	}
}

TEST_CASE("vector-tests-iterator-assignment")
{
	SECTION("trivial")
	{
		VectorTestIteratorAssignment<size_t, 0x1>(IdxToVal_size_t);
		VectorTestIteratorAssignment<size_t, 0x2>(IdxToVal_size_t);
		VectorTestIteratorAssignment<size_t, 0xff>(IdxToVal_size_t);
		VectorTestIteratorAssignment<size_t, 0x100>(IdxToVal_size_t);
	}

	SECTION("non-trivial")
	{
		VectorTestIteratorAssignment<std::string, 0x1>(IdxToVal_string);
		VectorTestIteratorAssignment<std::string, 0x2>(IdxToVal_string);
		VectorTestIteratorAssignment<std::string, 0xff>(IdxToVal_string);
		VectorTestIteratorAssignment<std::string, 0x100>(IdxToVal_string);
	}
}

TEST_CASE("vector-tests-erase")
{
	SECTION("trivial")
	{
		VectorTestErase<size_t, 0x1>(IdxToVal_size_t);
		VectorTestErase<size_t, 0x2>(IdxToVal_size_t);
		VectorTestErase<size_t, 0xff>(IdxToVal_size_t);
		VectorTestErase<size_t, 0x100>(IdxToVal_size_t);
	}

	SECTION("non-trivial")
	{
		VectorTestErase<std::string, 0x1>(IdxToVal_string);
		VectorTestErase<std::string, 0x2>(IdxToVal_string);
		VectorTestErase<std::string, 0xff>(IdxToVal_string);
		VectorTestErase<std::string, 0x100>(IdxToVal_string);
	}
}
