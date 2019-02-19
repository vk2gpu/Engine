#pragma once

#include "core/portability.h"

#include <cstdint>

typedef std::uint64_t u64;
typedef std::uint32_t u32;
typedef std::uint16_t u16;
typedef std::uint8_t u8;

typedef std::int64_t i64;
typedef std::int32_t i32;
typedef std::int16_t i16;
typedef std::int8_t i8;

typedef std::size_t size_t;

typedef float f32;
typedef double f64;

typedef wchar_t wchar;

#define DEFINE_ENUM_CLASS_FLAG_OPERATOR(_Type, _Operator)                                                              \
	inline _Type operator _Operator##=(_Type& A, _Type B)                                                              \
	{                                                                                                                  \
		A = (_Type)((int)A _Operator(int) B);                                                                          \
		return A;                                                                                                      \
	}                                                                                                                  \
	inline _Type operator _Operator(_Type A, _Type B) { return (_Type)((int)A _Operator(int) B); }

#define DEFINE_ENUM_CLASS_UNARY_FLAG_OPERATOR(_Type, _Operator)                                                        \
	inline _Type operator _Operator(_Type A) { return (_Type)(_Operator(int) A); }

namespace Core
{
}
