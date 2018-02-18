#include "core/array.h"
#include "core/vector.h"

#include "catch.hpp"

using namespace Core;

namespace
{
	struct CtorDtorTest
	{
		static i32 numAllocs_;
		CtorDtorTest(i32 value = -1)
		    : value_(value)
		{
			numAllocs_++;
		}
		CtorDtorTest(const CtorDtorTest& other)
		    : value_(other.value_)
		{
			numAllocs_++;
		}
		CtorDtorTest(CtorDtorTest&& other)
		{
			using std::swap;
			swap(other.value_, value_);
			numAllocs_++;
		}
		~CtorDtorTest()
		{
			--numAllocs_;
			value_ = -2;
		}
		CtorDtorTest& operator=(CtorDtorTest&& other)
		{
			using std::swap;
			swap(other.value_, value_);
			return *this;
		}

		operator i32() const { return value_; }

		i32 value_ = -1;
	};

	i32 CtorDtorTest::numAllocs_ = 0;

	struct AllocatorTest : public Core::ContainerAllocator
	{
		static i64 numBytes_;

		void* Allocate(i64 bytes, i64 align)
		{
			numBytes_ += bytes;
			u64* header = (u64*)Core::ContainerAllocator::Allocate(bytes + sizeof(u64), align);
			*header = bytes;
			return header + 1;
		}

		void Deallocate(void* mem)
		{
			if(mem)
			{
				u64* header = (u64*)mem - 1;
				numBytes_ -= *header;
				Core::ContainerAllocator::Deallocate(header);
			}
		}
	};

	i64 AllocatorTest::numBytes_ = 0;


	typedef i32 index_type;

	template<typename TYPE, index_type ARRAY_SIZE>
	void VectorTestSize()
	{
		Vector<TYPE> TestArray;

		REQUIRE(TestArray.size() == 0);
		REQUIRE(TestArray.capacity() >= TestArray.size());

		TestArray.resize(ARRAY_SIZE);
		REQUIRE(TestArray.size() == ARRAY_SIZE);
		REQUIRE(TestArray.capacity() >= TestArray.size());
	}

	template<typename TYPE, index_type ARRAY_SIZE>
	void VectorTestFill(TYPE(IdxToVal)(index_type))
	{
		Vector<TYPE> TestArray;
		TestArray.resize(ARRAY_SIZE);

		bool Success = true;
		const index_type FILL_VAL = 123;
		TestArray.fill(IdxToVal(FILL_VAL));
		for(index_type Idx = 0; Idx < ARRAY_SIZE; ++Idx)
			Success &= (TestArray[Idx] == IdxToVal(FILL_VAL));
		REQUIRE(Success);
	}

	template<typename TYPE, index_type ARRAY_SIZE>
	void VectorTestPushBack(TYPE(IdxToVal)(index_type))
	{
		Vector<TYPE> TestArray;
		bool Success = true;
		for(index_type Idx = 0; Idx < ARRAY_SIZE; ++Idx)
			TestArray.push_back(IdxToVal(Idx));
		for(index_type Idx = 0; Idx < ARRAY_SIZE; ++Idx)
			Success &= (TestArray[Idx] == IdxToVal(Idx));
		REQUIRE(Success);
	}

	template<typename TYPE, index_type ARRAY_SIZE>
	void VectorTestPushBackReserve(TYPE(IdxToVal)(index_type))
	{
		Vector<TYPE> TestArray;
		TestArray.reserve(ARRAY_SIZE);

		REQUIRE(TestArray.capacity() >= ARRAY_SIZE);

		bool Success = true;
		for(index_type Idx = 0; Idx < ARRAY_SIZE; ++Idx)
			TestArray.push_back(IdxToVal(Idx));
		for(index_type Idx = 0; Idx < ARRAY_SIZE; ++Idx)
			Success &= (TestArray[Idx] == IdxToVal(Idx));
		REQUIRE(Success);
	}

	template<typename TYPE, index_type ARRAY_SIZE>
	void VectorTestOperatorAssignment(TYPE(IdxToVal)(index_type))
	{
		Vector<TYPE> TestArray;
		TestArray.resize(ARRAY_SIZE);

		bool Success = true;
		for(index_type Idx = 0; Idx < ARRAY_SIZE; ++Idx)
			TestArray[Idx] = IdxToVal(Idx);
		for(index_type Idx = 0; Idx < ARRAY_SIZE; ++Idx)
			Success &= (TestArray[Idx] == IdxToVal(Idx));
		REQUIRE(Success);
	}

	template<typename TYPE, index_type ARRAY_SIZE>
	void VectorTestCopy(TYPE(IdxToVal)(index_type))
	{
		Vector<TYPE> TestArray;
		Vector<TYPE> TestArray2;
		TestArray.resize(ARRAY_SIZE);

		bool Success = true;
		for(index_type Idx = 0; Idx < ARRAY_SIZE; ++Idx)
			TestArray[Idx] = IdxToVal(Idx);
		TestArray2 = TestArray;
		REQUIRE(TestArray.size() == ARRAY_SIZE);
		REQUIRE(TestArray2.size() == ARRAY_SIZE);
		for(index_type Idx = 0; Idx < ARRAY_SIZE; ++Idx)
			Success &= (TestArray2[Idx] == IdxToVal(Idx));
		REQUIRE(Success);
	}

	template<typename TYPE, index_type ARRAY_SIZE>
	void VectorTestMove(TYPE(IdxToVal)(index_type))
	{
		Vector<TYPE> TestArray;
		Vector<TYPE> TestArray2;
		TestArray.resize(ARRAY_SIZE);

		bool Success = true;
		for(index_type Idx = 0; Idx < ARRAY_SIZE; ++Idx)
			TestArray[Idx] = IdxToVal(Idx);
		TestArray2 = std::move(TestArray);
		REQUIRE(TestArray.size() == 0);
		REQUIRE(TestArray2.size() == ARRAY_SIZE);
		for(index_type Idx = 0; Idx < ARRAY_SIZE; ++Idx)
			Success &= (TestArray2[Idx] == IdxToVal(Idx));
		REQUIRE(Success);
	}

	template<typename TYPE, index_type ARRAY_SIZE>
	void VectorTestDataAssignment(TYPE(IdxToVal)(index_type))
	{
		Vector<TYPE> TestArray;
		TestArray.resize(ARRAY_SIZE);

		bool Success = true;
		for(index_type Idx = 0; Idx < ARRAY_SIZE; ++Idx)
			TestArray.data()[Idx] = IdxToVal(Idx);
		for(index_type Idx = 0; Idx < ARRAY_SIZE; ++Idx)
			Success &= (TestArray[Idx] == IdxToVal(Idx));
		REQUIRE(Success);
	}

	template<typename TYPE, index_type ARRAY_SIZE>
	void VectorTestIteratorAssignment(TYPE(IdxToVal)(index_type))
	{
		Vector<TYPE> TestArray;
		TestArray.resize(ARRAY_SIZE);

		bool Success = true;
		index_type Idx = 0;
		for(auto& It : TestArray)
			It = IdxToVal(Idx++);
		for(Idx = 0; Idx < ARRAY_SIZE; ++Idx)
			Success &= (TestArray[Idx] == IdxToVal(Idx));
		REQUIRE(Success);
	}

	template<typename TYPE, index_type ARRAY_SIZE>
	void VectorTestShrinkToFit(TYPE(IdxToVal)(index_type))
	{
		Vector<TYPE> TestArray;
		TestArray.resize(ARRAY_SIZE);
		TestArray.push_back(TYPE());

		TestArray.shrink_to_fit();

		REQUIRE(TestArray.size() == TestArray.capacity());
	}

	template<typename TYPE, index_type ARRAY_SIZE>
	void VectorTestErase(TYPE(IdxToVal)(index_type))
	{
		Vector<TYPE> TestArray;

		// Erase from beginning.
		{
			TestArray.resize(ARRAY_SIZE);
			index_type Idx = 0;
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
			index_type Idx = 0;
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
			index_type Idx = 0;
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

	index_type IdxToVal_index_type(index_type Idx) { return Idx; }

	std::string IdxToVal_string(index_type Idx)
	{
		char Buffer[1024] = {0};
		sprintf_s(Buffer, sizeof(Buffer), "%u", (int)Idx);
		return Buffer;
	}
}


TEST_CASE("vector-tests-size")
{
	VectorTestSize<index_type, 0x1>();
	VectorTestSize<index_type, 0x2>();
	VectorTestSize<index_type, 0xff>();
	VectorTestSize<index_type, 0x100>();
	VectorTestSize<index_type, 0xffff>();
	VectorTestSize<index_type, 0x10000>();
}

TEST_CASE("vector-tests-fill")
{
	SECTION("trivial")
	{
		VectorTestFill<index_type, 0x1>(IdxToVal_index_type);
		VectorTestFill<index_type, 0x2>(IdxToVal_index_type);
		VectorTestFill<index_type, 0xff>(IdxToVal_index_type);
		VectorTestFill<index_type, 0x100>(IdxToVal_index_type);
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
		VectorTestPushBack<index_type, 0x1>(IdxToVal_index_type);
		VectorTestPushBack<index_type, 0x2>(IdxToVal_index_type);
		VectorTestPushBack<index_type, 0xff>(IdxToVal_index_type);
		VectorTestPushBack<index_type, 0x100>(IdxToVal_index_type);
	}

	SECTION("non-trivial")
	{
		VectorTestPushBack<std::string, 0x1>(IdxToVal_string);
		VectorTestPushBack<std::string, 0x2>(IdxToVal_string);
		VectorTestPushBack<std::string, 0xff>(IdxToVal_string);
		VectorTestOperatorAssignment<std::string, 0x100>(IdxToVal_string);
	}
}

TEST_CASE("vector-tests-emplace-back")
{
	SECTION("trivial")
	{
		Core::Vector<i32> vec;
		vec.emplace_back(0);
		vec.emplace_back(1);
		vec.emplace_back(2);
		REQUIRE(vec[0] == 0);
		REQUIRE(vec[1] == 1);
		REQUIRE(vec[2] == 2);
	}

	SECTION("trivial-params")
	{
		struct TestType
		{
			TestType() = default;
			TestType(int a, int b)
			    : a_(a)
			    , b_(b)
			{
			}

			bool operator==(const TestType& other) { return a_ == other.a_ && b_ == other.b_; }

			int a_ = 0;
			int b_ = 0;
		};

		Core::Vector<TestType> vec;
		vec.emplace_back(0, 1);
		vec.emplace_back(1, 2);
		vec.emplace_back(2, 4);
		REQUIRE(vec[0] == TestType(0, 1));
		REQUIRE(vec[1] == TestType(1, 2));
		REQUIRE(vec[2] == TestType(2, 4));
	}

	SECTION("non-trivial")
	{
		Core::Vector<std::string> vec;
		vec.emplace_back("0");
		vec.emplace_back("1");
		vec.emplace_back("2");
		REQUIRE(vec[0] == "0");
		REQUIRE(vec[1] == "1");
		REQUIRE(vec[2] == "2");
	}
}

TEST_CASE("vector-tests-push-back-reserve")
{
	SECTION("trivial")
	{
		VectorTestPushBackReserve<index_type, 0x1>(IdxToVal_index_type);
		VectorTestPushBackReserve<index_type, 0x2>(IdxToVal_index_type);
		VectorTestPushBackReserve<index_type, 0xff>(IdxToVal_index_type);
		VectorTestPushBackReserve<index_type, 0x100>(IdxToVal_index_type);
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
		VectorTestOperatorAssignment<index_type, 0x1>(IdxToVal_index_type);
		VectorTestOperatorAssignment<index_type, 0x2>(IdxToVal_index_type);
		VectorTestOperatorAssignment<index_type, 0xff>(IdxToVal_index_type);
		VectorTestOperatorAssignment<index_type, 0x100>(IdxToVal_index_type);
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
		VectorTestCopy<index_type, 0x1>(IdxToVal_index_type);
		VectorTestCopy<index_type, 0x2>(IdxToVal_index_type);
		VectorTestCopy<index_type, 0xff>(IdxToVal_index_type);
		VectorTestCopy<index_type, 0x100>(IdxToVal_index_type);
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
		VectorTestMove<index_type, 0x1>(IdxToVal_index_type);
		VectorTestMove<index_type, 0x2>(IdxToVal_index_type);
		VectorTestMove<index_type, 0xff>(IdxToVal_index_type);
		VectorTestMove<index_type, 0x100>(IdxToVal_index_type);
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
		VectorTestDataAssignment<index_type, 0x1>(IdxToVal_index_type);
		VectorTestDataAssignment<index_type, 0x2>(IdxToVal_index_type);
		VectorTestDataAssignment<index_type, 0xff>(IdxToVal_index_type);
		VectorTestDataAssignment<index_type, 0x100>(IdxToVal_index_type);
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
		VectorTestIteratorAssignment<index_type, 0x1>(IdxToVal_index_type);
		VectorTestIteratorAssignment<index_type, 0x2>(IdxToVal_index_type);
		VectorTestIteratorAssignment<index_type, 0xff>(IdxToVal_index_type);
		VectorTestIteratorAssignment<index_type, 0x100>(IdxToVal_index_type);
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
		VectorTestErase<index_type, 0x1>(IdxToVal_index_type);
		VectorTestErase<index_type, 0x2>(IdxToVal_index_type);
		VectorTestErase<index_type, 0xff>(IdxToVal_index_type);
		VectorTestErase<index_type, 0x100>(IdxToVal_index_type);
	}

	SECTION("non-trivial")
	{
		VectorTestErase<std::string, 0x1>(IdxToVal_string);
		VectorTestErase<std::string, 0x2>(IdxToVal_string);
		VectorTestErase<std::string, 0xff>(IdxToVal_string);
		VectorTestErase<std::string, 0x100>(IdxToVal_string);
	}
}

TEST_CASE("vector-tests-ctor-dtor-test")
{
	using TestVector = Core::Vector<CtorDtorTest>;

	SECTION("resize")
	{
		{
			TestVector vec;
			vec.resize(1);
			REQUIRE(CtorDtorTest::numAllocs_ == 1);
		}
		REQUIRE(CtorDtorTest::numAllocs_ == 0);

		{
			TestVector vec;
			vec.resize(100);
			REQUIRE(CtorDtorTest::numAllocs_ == 100);
			vec.resize(200);
			REQUIRE(CtorDtorTest::numAllocs_ == 200);
		}
		REQUIRE(CtorDtorTest::numAllocs_ == 0);

		{
			TestVector vec;
			vec.resize(200);
			REQUIRE(CtorDtorTest::numAllocs_ == 200);
			vec.resize(100);
			REQUIRE(CtorDtorTest::numAllocs_ == 100);
		}
		REQUIRE(CtorDtorTest::numAllocs_ == 0);
	}

	SECTION("copy")
	{
		{
			TestVector vec;
			vec.resize(100);
			REQUIRE(CtorDtorTest::numAllocs_ == 100);

			TestVector vec2;
			vec2 = vec;
			REQUIRE(CtorDtorTest::numAllocs_ == 200);
		}
		REQUIRE(CtorDtorTest::numAllocs_ == 0);
	}

	SECTION("push_back")
	{
		{
			TestVector vec;
			for(i32 i = 0; i < 1; ++i)
			{
				vec.push_back(i);
				REQUIRE(CtorDtorTest::numAllocs_ == i + 1);
			}
		}
		REQUIRE(CtorDtorTest::numAllocs_ == 0);

		{
			TestVector vec;
			for(i32 i = 0; i < 100; ++i)
			{
				vec.push_back(i);
				REQUIRE(CtorDtorTest::numAllocs_ == i + 1);
			}
		}
		REQUIRE(CtorDtorTest::numAllocs_ == 0);

		{
			TestVector vec;
			for(i32 i = 0; i < 200; ++i)
			{
				vec.push_back(i);
				REQUIRE(CtorDtorTest::numAllocs_ == i + 1);
			}
		}
		REQUIRE(CtorDtorTest::numAllocs_ == 0);
	}

	SECTION("emplace_back")
	{
		{
			TestVector vec;
			for(i32 i = 0; i < 1; ++i)
			{
				vec.emplace_back(i);
				REQUIRE(CtorDtorTest::numAllocs_ == i + 1);
			}
		}
		REQUIRE(CtorDtorTest::numAllocs_ == 0);

		{
			TestVector vec;
			for(i32 i = 0; i < 100; ++i)
			{
				vec.emplace_back(i);
				REQUIRE(CtorDtorTest::numAllocs_ == i + 1);
			}
		}
		REQUIRE(CtorDtorTest::numAllocs_ == 0);

		{
			TestVector vec;
			for(i32 i = 0; i < 200; ++i)
			{
				vec.emplace_back(i);
				REQUIRE(CtorDtorTest::numAllocs_ == i + 1);
			}
		}
		REQUIRE(CtorDtorTest::numAllocs_ == 0);
	}

	SECTION("erase")
	{
		{
			TestVector vec;
			vec.emplace_back(0);
			REQUIRE(CtorDtorTest::numAllocs_ == 1);

			vec.emplace_back(1);
			REQUIRE(CtorDtorTest::numAllocs_ == 2);

			vec.emplace_back(2);
			REQUIRE(CtorDtorTest::numAllocs_ == 3);

			REQUIRE(vec[0] == 0);
			REQUIRE(vec[1] == 1);
			REQUIRE(vec[2] == 2);

			vec.erase(vec.begin() + 1);
			REQUIRE(CtorDtorTest::numAllocs_ == 2);

			REQUIRE(vec[0] == 0);
			REQUIRE(vec[1] == 2);
		}
		REQUIRE(CtorDtorTest::numAllocs_ == 0);
	}

	SECTION("fill")
	{
		{
			TestVector vec;
			vec.resize(100);
			REQUIRE(CtorDtorTest::numAllocs_ == 100);

			vec.fill(10);
			REQUIRE(CtorDtorTest::numAllocs_ == 100);
		}
		REQUIRE(CtorDtorTest::numAllocs_ == 0);
	}
}

TEST_CASE("vector-tests-allocator-test")
{
	using TestVector = Core::Vector<i32, AllocatorTest>;

	SECTION("resize")
	{
		{
			TestVector vec;
			vec.resize(1);
			REQUIRE(AllocatorTest::numBytes_ == sizeof(i32) * 1);
		}
		REQUIRE(AllocatorTest::numBytes_ == 0);

		{
			TestVector vec;
			vec.resize(100);
			REQUIRE(AllocatorTest::numBytes_ == sizeof(i32) * 100);
			vec.resize(200);
			REQUIRE(AllocatorTest::numBytes_ == sizeof(i32) * 200);
		}
		REQUIRE(AllocatorTest::numBytes_ == 0);

		{
			TestVector vec;
			vec.resize(200);
			REQUIRE(AllocatorTest::numBytes_ == sizeof(i32) * 200);
			vec.resize(100);
			REQUIRE(AllocatorTest::numBytes_ == sizeof(i32) * 100);
		}
		REQUIRE(AllocatorTest::numBytes_ == 0);
	}

	SECTION("copy")
	{
		{
			TestVector vec;
			vec.resize(100);
			REQUIRE(AllocatorTest::numBytes_ == sizeof(i32) * 100);

			TestVector vec2;
			vec2 = vec;
			REQUIRE(AllocatorTest::numBytes_ == sizeof(i32) * 200);
		}
		REQUIRE(AllocatorTest::numBytes_ == 0);
	}

	SECTION("push_back")
	{
		{
			TestVector vec;
			for(i32 i = 0; i < 1; ++i)
			{
				vec.push_back(i);
				REQUIRE(AllocatorTest::numBytes_ >= (i64)sizeof(i32) * (i + 1));
			}
		}
		REQUIRE(AllocatorTest::numBytes_ == 0);

		{
			TestVector vec;
			for(i32 i = 0; i < 100; ++i)
			{
				vec.push_back(i);
				REQUIRE(AllocatorTest::numBytes_ >= (i64)sizeof(i32) * (i + 1));
			}
		}
		REQUIRE(AllocatorTest::numBytes_ == 0);

		{
			TestVector vec;
			for(i32 i = 0; i < 200; ++i)
			{
				vec.push_back(i);
				REQUIRE(AllocatorTest::numBytes_ >= (i64)sizeof(i32) * (i + 1));
			}
		}
		REQUIRE(AllocatorTest::numBytes_ == 0);
	}

	SECTION("emplace_back")
	{
		{
			TestVector vec;
			for(i32 i = 0; i < 1; ++i)
			{
				vec.emplace_back(i);
				REQUIRE(AllocatorTest::numBytes_ >= (i64)sizeof(i32) * (i + 1));
			}
		}
		REQUIRE(AllocatorTest::numBytes_ == 0);

		{
			TestVector vec;
			for(i32 i = 0; i < 100; ++i)
			{
				vec.emplace_back(i);
				REQUIRE(AllocatorTest::numBytes_ >= (i64)sizeof(i32) * (i + 1));
			}
		}
		REQUIRE(AllocatorTest::numBytes_ == 0);

		{
			TestVector vec;
			for(i32 i = 0; i < 200; ++i)
			{
				vec.emplace_back(i);
				REQUIRE(AllocatorTest::numBytes_ >= (i64)sizeof(i32) * (i + 1));
			}
		}
		REQUIRE(AllocatorTest::numBytes_ == 0);
	}

	SECTION("erase")
	{
		{
			TestVector vec;
			vec.emplace_back(0);
			REQUIRE(AllocatorTest::numBytes_ >= sizeof(i32) * 1);

			vec.emplace_back(1);
			REQUIRE(AllocatorTest::numBytes_ >= sizeof(i32) * 2);

			vec.emplace_back(2);
			REQUIRE(AllocatorTest::numBytes_ >= sizeof(i32) * 3);
			auto totalBytes = AllocatorTest::numBytes_;

			REQUIRE(vec[0] == 0);
			REQUIRE(vec[1] == 1);
			REQUIRE(vec[2] == 2);

			vec.erase(vec.begin() + 1);
			REQUIRE(AllocatorTest::numBytes_ == totalBytes);

			REQUIRE(vec[0] == 0);
			REQUIRE(vec[1] == 2);
		}
		REQUIRE(AllocatorTest::numBytes_ == 0);
	}

	SECTION("fill")
	{
		{
			TestVector vec;
			vec.resize(100);
			REQUIRE(AllocatorTest::numBytes_ == sizeof(i32) * 100);

			vec.fill(10);
			REQUIRE(AllocatorTest::numBytes_ == sizeof(i32) * 100);
		}
		REQUIRE(AllocatorTest::numBytes_ == 0);
	}
}