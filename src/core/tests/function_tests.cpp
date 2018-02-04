#include "core/debug.h"
#include "core/function.h"
#include "core/vector.h"

#include "catch.hpp"

namespace
{
	struct AllocatorTest : public Core::Allocator
	{
		static i32 numBytes_;

		void* allocate(index_type count, index_type size)
		{
			numBytes_ += count * size;
			return Core::Allocator::allocate(count, size);
		}

		void deallocate(void* mem, index_type count, index_type size)
		{
			numBytes_ -= count * size;
			Core::Allocator::deallocate(mem, count, size);
		}
	};

	i32 AllocatorTest::numBytes_ = 0;
}

TEST_CASE("function-tests-basic")
{
	auto lambda = [](i32) {};

	Core::Function<void(i32)> func;
	REQUIRE(func == false);
	func = lambda;
	REQUIRE(func == true);
}

TEST_CASE("function-tests-copy")
{
	SECTION("copy")
	{
		auto lambda = [](i32) {};
		Core::Function<void(i32)> func;
		REQUIRE(func == false);
		func = lambda;
		REQUIRE(func == true);

		Core::Function<void(i32)> func2 = func;
		REQUIRE(func == true);
		REQUIRE(func2 == true);
	}

	SECTION("copy-capture")
	{
		i32 a = 123;
		auto lambda = [a](i32 b) { return a * b; };
		REQUIRE(lambda(2) == (a * 2));
		Core::Function<i32(i32)> func;
		REQUIRE(func == false);
		func = lambda;
		REQUIRE(func == true);
		REQUIRE(func(2) == (a * 2));

		Core::Function<i32(i32)> func2 = func;
		REQUIRE(func == true);
		REQUIRE(func(2) == (a * 2));
		REQUIRE(func2 == true);
		REQUIRE(func2(2) == (a * 2));
	}

	SECTION("copy-capture-ctor-dtor")
	{
		struct CaptureCount
		{
			CaptureCount(i32& count)
				: count_(count)
			{
				++count_;
			}

			CaptureCount(const CaptureCount& other)
				: count_(other.count_)
			{
				++count_;
			}

			~CaptureCount()
			{
				if(count_)
					--count_;
			}
			i32& count_;
		};

		i32 count = 0;
		{
			auto lambda = [cap = CaptureCount(count)]() { return cap.count_; };
			auto lambda2 = [cap = CaptureCount(count)]() { return cap.count_; };
			CHECK(count == 2);

			Core::Function<i32()> func;

			func = lambda;
			CHECK(count == 3);
			func = nullptr;
			CHECK(count == 2);

			func = lambda2;
			CHECK(count == 3);
			func = lambda;
			CHECK(count == 3);
		}
		CHECK(count == 0);
	}
}


TEST_CASE("function-tests-alloc")
{
	using Vector = Core::Vector<i32, AllocatorTest>;
	Core::Function<i32(i32)> func;

	Vector vec;
	vec.reserve(32);
	for(i32 idx = 0; idx < 32; ++idx)
	{
		vec.push_back(idx);
	}

	i32 size0 = AllocatorTest::numBytes_;

	func = [vec](i32 idx) { return vec[idx]; };

	i32 size1 = AllocatorTest::numBytes_;
	REQUIRE(size1 > size0);

	for(i32 idx = 0; idx < 32; ++idx)
	{
		REQUIRE(func(idx) == vec[idx]);
		REQUIRE(func(idx) == vec[idx]);
	}

	vec.resize(0);

	i32 size2 = AllocatorTest::numBytes_;
	REQUIRE(size2 == size0);

	func = nullptr;

	i32 size3 = AllocatorTest::numBytes_;
	REQUIRE(size3 == 0);
}


TEST_CASE("function-tests-vector")
{
	Core::Vector<Core::Function<i32()>> funcs;

	for(i32 idx = 0; idx < 32; ++idx)
	{
		funcs.push_back([idx]() { return idx * 2; });
	}

	for(i32 idx = 0; idx < 32; ++idx)
	{
		const auto& func = funcs[idx];
		REQUIRE(func() == (idx * 2));
	}

	Core::Vector<Core::Function<i32()>> funcs2 = funcs;

	for(i32 idx = 0; idx < 32; ++idx)
	{
		const auto& func = funcs[idx];
		const auto& func2 = funcs2[idx];
		REQUIRE(func() == (idx * 2));
		REQUIRE(func() == func2());
	}
}
