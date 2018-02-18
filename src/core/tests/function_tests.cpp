#include "core/debug.h"
#include "core/function.h"
#include "core/vector.h"

#include "catch.hpp"

namespace
{
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
}

TEST_CASE("function-tests-basic")
{
	auto lambda = [](i32) {};

	Core::Function<void(i32)> func;
	REQUIRE(!func);
	func = lambda;
	REQUIRE(func);
}

TEST_CASE("function-tests-copy")
{
	SECTION("copy")
	{
		auto lambda = [](i32) {};
		Core::Function<void(i32)> func;
		REQUIRE(!func);
		func = lambda;
		REQUIRE(func);

		Core::Function<void(i32)> func2 = func;
		REQUIRE(func);
		REQUIRE(func2);
	}

	SECTION("copy-capture")
	{
		i32 a = 123;
		auto lambda = [a](i32 b) { return a * b; };
		REQUIRE(lambda(2) == (a * 2));
		Core::Function<i32(i32)> func;
		REQUIRE(!func);
		func = lambda;
		REQUIRE(func);
		REQUIRE(func(2) == (a * 2));

		Core::Function<i32(i32)> func2 = func;
		REQUIRE(func);
		REQUIRE(func(2) == (a * 2));
		REQUIRE(func2);
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

	i64 size0 = AllocatorTest::numBytes_;

	func = [vec](i32 idx) { return vec[idx]; };

	i64 size1 = AllocatorTest::numBytes_;
	REQUIRE(size1 > size0);

	for(i32 idx = 0; idx < 32; ++idx)
	{
		REQUIRE(func(idx) == vec[idx]);
		REQUIRE(func(idx) == vec[idx]);
	}

	vec.resize(0);

	i64 size2 = AllocatorTest::numBytes_;
	REQUIRE(size2 == size0);

	func = nullptr;

	i64 size3 = AllocatorTest::numBytes_;
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
