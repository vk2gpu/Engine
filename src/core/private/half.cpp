// Branch-free implementation of half-precision (16 bit) floating point
// Copyright 2006 Mike Acton <macton@gmail.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE
//
// Half-precision floating point format
// ------------------------------------
//
//   | Field    | Last | First | Note
//   |----------|------|-------|----------
//   | Sign     | 15   | 15    |
//   | Exponent | 14   | 10    | Bias = 15
//   | Mantissa | 9    | 0     |
//
// Compiling
// ---------
//
//  Preferred compile flags for GCC:
//     -O3 -fstrict-aliasing -std=c99 -pedantic -Wall -Wstrict-aliasing
//
//     This file is a C99 source file, intended to be compiled with a C99
//     compliant compiler. However, for the moment it remains combatible
//     with C++98. Therefore if you are using a compiler that poorly implements
//     C standards (e.g. MSVC), it may be compiled as C++. This is not
//     guaranteed for future versions.
//
// Features
// --------
//
//  * QNaN + <x>  = QNaN
//  * <x>  + +INF = +INF
//  * <x>  - -INF = -INF
//  * INF  - INF  = SNaN
//  * Denormalized values
//  * Difference of ZEROs is always +ZERO
//  * Sum round with guard + round + sticky bit (grs)
//  * And of course... no branching
//
// Precision of Sum
// ----------------
//
//  (SUM)        u16 z = half_add( x, y );
//  (DIFFERENCE) u16 z = half_add( x, -y );
//
//     Will have exactly (0 ulps difference) the same result as:
//     (For 32 bit IEEE 784 floating point and same rounding mode)
//
//     union FLOAT_32
//     {
//       float    f32;
//       u32 u32;
//     };
//
//     union FLOAT_32 fx = { .u32 = half_to_float( x ) };
//     union FLOAT_32 fy = { .u32 = half_to_float( y ) };
//     union FLOAT_32 fz = { .f32 = fx.f32 + fy.f32    };
//     u16       z  = float_to_half( fz );
//

#include "core/half.h"
#include <stdio.h>

#include <xmmintrin.h>

#if defined(COMPILER_MSVC)
#pragma warning(disable : 4146)
#pragma optimize("", on)
#endif

// Load immediate
static inline u32 _uint32_li(u32 a)
{
	return (a);
}

// Decrement
static inline u32 _uint32_dec(u32 a)
{
	return (a - 1);
}

// Complement
static inline u32 _uint32_not(u32 a)
{
	return (~a);
}

// Negate
static inline u32 _uint32_neg(u32 a)
{
	return (u32)(-((i32)a));
}

// Extend sign
static inline u32 _uint32_ext(u32 a)
{
	return (u32)(((i32)a) >> 31);
}

// And
static inline u32 _uint32_and(u32 a, u32 b)
{
	return (a & b);
}

// And with Complement
static inline u32 _uint32_andc(u32 a, u32 b)
{
	return (a & ~b);
}

// Or
static inline u32 _uint32_or(u32 a, u32 b)
{
	return (a | b);
}

// Shift Right Logical
static inline u32 _uint32_srl(u32 a, int sa)
{
	return (a >> sa);
}

// Shift Left Logical
static inline u32 _uint32_sll(u32 a, int sa)
{
	return (a << sa);
}

// Add
static inline u32 _uint32_add(u32 a, u32 b)
{
	return (a + b);
}

// Subtract
static inline u32 _uint32_sub(u32 a, u32 b)
{
	return (a - b);
}

// Select on Sign bit
static inline u32 _uint32_sels(u32 test, u32 a, u32 b)
{
	const u32 mask = _uint32_ext(test);
	const u32 sel_a = _uint32_and(a, mask);
	const u32 sel_b = _uint32_andc(b, mask);
	const u32 result = _uint32_or(sel_a, sel_b);

	return (result);
}

// Load Immediate
static inline u16 _uint16_li(u16 a)
{
	return (a);
}

// Extend sign
static inline u16 _uint16_ext(u16 a)
{
	return (u16)(((i16)a) >> 15);
}

// Negate
static inline u16 _uint16_neg(u16 a)
{
	return (u16)(-a);
}

// Complement
static inline u16 _uint16_not(u16 a)
{
	return (u16)(~a);
}

// Decrement
static inline u16 _uint16_dec(u16 a)
{
	return (u16)(a - 1);
}

// Shift Left Logical
static inline u16 _uint16_sll(u16 a, int sa)
{
	return (u16)(a << sa);
}

// Shift Right Logical
static inline u16 _uint16_srl(u16 a, int sa)
{
	return (u16)(a >> sa);
}

// Add
static inline u16 _uint16_add(u16 a, u16 b)
{
	return (u16)(a + b);
}

// Subtract
static inline u16 _uint16_sub(u16 a, u16 b)
{
	return (u16)(a - b);
}

// And
static inline u16 _uint16_and(u16 a, u16 b)
{
	return (a & b);
}

// Or
static inline u16 _uint16_or(u16 a, u16 b)
{
	return (a | b);
}

// Exclusive Or
static inline u16 _uint16_xor(u16 a, u16 b)
{
	return (a ^ b);
}

// And with Complement
static inline u16 _uint16_andc(u16 a, u16 b)
{
	return (a & ~b);
}

// And then Shift Right Logical
static inline u16 _uint16_andsrl(u16 a, u16 b, int sa)
{
	return (u16)((a & b) >> sa);
}

// Shift Right Logical then Mask
static inline u16 _uint16_srlm(u16 a, int sa, u16 mask)
{
	return (u16)((a >> sa) & mask);
}

// Add then Mask
static inline u16 _uint16_addm(u16 a, u16 b, u16 mask)
{
	return (u16)((a + b) & mask);
}


// Select on Sign bit
static inline u16 _uint16_sels(u16 test, u16 a, u16 b)
{
	const u16 mask = _uint16_ext(test);
	const u16 sel_a = _uint16_and(a, mask);
	const u16 sel_b = _uint16_andc(b, mask);
	const u16 result = _uint16_or(sel_a, sel_b);

	return (result);
}

// Count Leading Zeros
static inline u32 _uint32_cntlz(u32 x)
{
	const u32 x0 = _uint32_srl(x, 1);
	const u32 x1 = _uint32_or(x, x0);
	const u32 x2 = _uint32_srl(x1, 2);
	const u32 x3 = _uint32_or(x1, x2);
	const u32 x4 = _uint32_srl(x3, 4);
	const u32 x5 = _uint32_or(x3, x4);
	const u32 x6 = _uint32_srl(x5, 8);
	const u32 x7 = _uint32_or(x5, x6);
	const u32 x8 = _uint32_srl(x7, 16);
	const u32 x9 = _uint32_or(x7, x8);
	const u32 xA = _uint32_not(x9);
	const u32 xB = _uint32_srl(xA, 1);
	const u32 xC = _uint32_and(xB, 0x55555555);
	const u32 xD = _uint32_sub(xA, xC);
	const u32 xE = _uint32_and(xD, 0x33333333);
	const u32 xF = _uint32_srl(xD, 2);
	const u32 x10 = _uint32_and(xF, 0x33333333);
	const u32 x11 = _uint32_add(xE, x10);
	const u32 x12 = _uint32_srl(x11, 4);
	const u32 x13 = _uint32_add(x11, x12);
	const u32 x14 = _uint32_and(x13, 0x0f0f0f0f);
	const u32 x15 = _uint32_srl(x14, 8);
	const u32 x16 = _uint32_add(x14, x15);
	const u32 x17 = _uint32_srl(x16, 16);
	const u32 x18 = _uint32_add(x16, x17);
	const u32 x19 = _uint32_and(x18, 0x0000003f);
	return (x19);
}

// Count Leading Zeros
static inline u16 _uint16_cntlz(u16 x)
{
	const u16 x0 = _uint16_srl(x, 1);
	const u16 x1 = _uint16_or(x, x0);
	const u16 x2 = _uint16_srl(x1, 2);
	const u16 x3 = _uint16_or(x1, x2);
	const u16 x4 = _uint16_srl(x3, 4);
	const u16 x5 = _uint16_or(x3, x4);
	const u16 x6 = _uint16_srl(x5, 8);
	const u16 x7 = _uint16_or(x5, x6);
	const u16 x8 = _uint16_not(x7);
	const u16 x9 = _uint16_srlm(x8, 1, 0x5555);
	const u16 xA = _uint16_sub(x8, x9);
	const u16 xB = _uint16_and(xA, 0x3333);
	const u16 xC = _uint16_srlm(xA, 2, 0x3333);
	const u16 xD = _uint16_add(xB, xC);
	const u16 xE = _uint16_srl(xD, 4);
	const u16 xF = _uint16_addm(xD, xE, 0x0f0f);
	const u16 x10 = _uint16_srl(xF, 8);
	const u16 x11 = _uint16_addm(xF, x10, 0x001f);
	return (x11);
}

static u16 float_to_half(u32 f)
{
	const u32 one = _uint32_li(0x00000001);
	const u32 f_e_mask = _uint32_li(0x7f800000);
	const u32 f_m_mask = _uint32_li(0x007fffff);
	const u32 f_s_mask = _uint32_li(0x80000000);
	const u32 h_e_mask = _uint32_li(0x00007c00);
	const u32 f_e_pos = _uint32_li(0x00000017);
	const u32 f_m_round_bit = _uint32_li(0x00001000);
	const u32 h_nan_em_min = _uint32_li(0x00007c01);
	const u32 f_h_s_pos_offset = _uint32_li(0x00000010);
	const u32 f_m_hidden_bit = _uint32_li(0x00800000);
	const u32 f_h_m_pos_offset = _uint32_li(0x0000000d);
	const u32 f_h_bias_offset = _uint32_li(0x38000000);
	const u32 f_m_snan_mask = _uint32_li(0x003fffff);
	const u16 h_snan_mask = (u16)_uint32_li(0x00007e00);
	const u32 f_e = _uint32_and(f, f_e_mask);
	const u32 f_m = _uint32_and(f, f_m_mask);
	const u32 f_s = _uint32_and(f, f_s_mask);
	const u32 f_e_h_bias = _uint32_sub(f_e, f_h_bias_offset);
	const u32 f_e_h_bias_amount = _uint32_srl(f_e_h_bias, f_e_pos);
	const u32 f_m_round_mask = _uint32_and(f_m, f_m_round_bit);
	const u32 f_m_round_offset = _uint32_sll(f_m_round_mask, one);
	const u32 f_m_rounded = _uint32_add(f_m, f_m_round_offset);
	const u32 f_m_rounded_overflow = _uint32_and(f_m_rounded, f_m_hidden_bit);
	const u32 f_m_denorm_sa = _uint32_sub(one, f_e_h_bias_amount);
	const u32 f_m_with_hidden = _uint32_or(f_m_rounded, f_m_hidden_bit);
	const u32 f_m_denorm = _uint32_srl(f_m_with_hidden, f_m_denorm_sa);
	const u32 f_em_norm_packed = _uint32_or(f_e_h_bias, f_m_rounded);
	const u32 f_e_overflow = _uint32_add(f_e_h_bias, f_m_hidden_bit);
	const u32 h_s = _uint32_srl(f_s, f_h_s_pos_offset);
	const u32 h_m_nan = _uint32_srl(f_m, f_h_m_pos_offset);
	const u32 h_m_denorm = _uint32_srl(f_m_denorm, f_h_m_pos_offset);
	const u32 h_em_norm = _uint32_srl(f_em_norm_packed, f_h_m_pos_offset);
	const u32 h_em_overflow = _uint32_srl(f_e_overflow, f_h_m_pos_offset);
	const u32 is_e_eqz_msb = _uint32_dec(f_e);
	const u32 is_m_nez_msb = _uint32_neg(f_m);
	const u32 is_h_m_nan_nez_msb = _uint32_neg(h_m_nan);
	const u32 is_e_nflagged_msb = _uint32_sub(f_e, f_e_mask);
	const u32 is_ninf_msb = _uint32_or(is_e_nflagged_msb, is_m_nez_msb);
	const u32 is_underflow_msb = _uint32_sub(is_e_eqz_msb, f_h_bias_offset);
	const u32 is_nan_nunderflow_msb = _uint32_or(is_h_m_nan_nez_msb, is_e_nflagged_msb);
	const u32 is_m_snan_msb = _uint32_sub(f_m_snan_mask, f_m);
	const u32 is_snan_msb = _uint32_andc(is_m_snan_msb, is_e_nflagged_msb);
	const u32 is_overflow_msb = _uint32_neg(f_m_rounded_overflow);
	const u32 h_nan_underflow_result = _uint32_sels(is_nan_nunderflow_msb, h_em_norm, h_nan_em_min);
	const u32 h_inf_result = _uint32_sels(is_ninf_msb, h_nan_underflow_result, h_e_mask);
	const u32 h_underflow_result = _uint32_sels(is_underflow_msb, h_m_denorm, h_inf_result);
	const u32 h_overflow_result = _uint32_sels(is_overflow_msb, h_em_overflow, h_underflow_result);
	const u32 h_em_result = _uint32_sels(is_snan_msb, h_snan_mask, h_overflow_result);
	const u32 h_result = _uint32_or(h_em_result, h_s);

	return (u16)(h_result);
}

static u32 half_to_float(u16 h)
{
	const u32 h_e_mask = _uint32_li(0x00007c00);
	const u32 h_m_mask = _uint32_li(0x000003ff);
	const u32 h_s_mask = _uint32_li(0x00008000);
	const u32 h_f_s_pos_offset = _uint32_li(0x00000010);
	const u32 h_f_e_pos_offset = _uint32_li(0x0000000d);
	const u32 h_f_bias_offset = _uint32_li(0x0001c000);
	const u32 f_e_mask = _uint32_li(0x7f800000);
	const u32 f_m_mask = _uint32_li(0x007fffff);
	const u32 h_f_e_denorm_bias = _uint32_li(0x0000007e);
	const u32 h_f_m_denorm_sa_bias = _uint32_li(0x00000008);
	const u32 f_e_pos = _uint32_li(0x00000017);
	const u32 h_e_mask_minus_one = _uint32_li(0x00007bff);
	const u32 h_e = _uint32_and(h, h_e_mask);
	const u32 h_m = _uint32_and(h, h_m_mask);
	const u32 h_s = _uint32_and(h, h_s_mask);
	const u32 h_e_f_bias = _uint32_add(h_e, h_f_bias_offset);
	const u32 h_m_nlz = _uint32_cntlz(h_m);
	const u32 f_s = _uint32_sll(h_s, h_f_s_pos_offset);
	const u32 f_e = _uint32_sll(h_e_f_bias, h_f_e_pos_offset);
	const u32 f_m = _uint32_sll(h_m, h_f_e_pos_offset);
	const u32 f_em = _uint32_or(f_e, f_m);
	const u32 h_f_m_sa = _uint32_sub(h_m_nlz, h_f_m_denorm_sa_bias);
	const u32 f_e_denorm_unpacked = _uint32_sub(h_f_e_denorm_bias, h_f_m_sa);
	const u32 h_f_m = _uint32_sll(h_m, h_f_m_sa);
	const u32 f_m_denorm = _uint32_and(h_f_m, f_m_mask);
	const u32 f_e_denorm = _uint32_sll(f_e_denorm_unpacked, f_e_pos);
	const u32 f_em_denorm = _uint32_or(f_e_denorm, f_m_denorm);
	const u32 f_em_nan = _uint32_or(f_e_mask, f_m);
	const u32 is_e_eqz_msb = _uint32_dec(h_e);
	const u32 is_m_nez_msb = _uint32_neg(h_m);
	const u32 is_e_flagged_msb = _uint32_sub(h_e_mask_minus_one, h_e);
	const u32 is_zero_msb = _uint32_andc(is_e_eqz_msb, is_m_nez_msb);
	const u32 is_inf_msb = _uint32_andc(is_e_flagged_msb, is_m_nez_msb);
	const u32 is_denorm_msb = _uint32_and(is_m_nez_msb, is_e_eqz_msb);
	const u32 is_nan_msb = _uint32_and(is_e_flagged_msb, is_m_nez_msb);
	const u32 is_zero = _uint32_ext(is_zero_msb);
	const u32 f_zero_result = _uint32_andc(f_em, is_zero);
	const u32 f_denorm_result = _uint32_sels(is_denorm_msb, f_em_denorm, f_zero_result);
	const u32 f_inf_result = _uint32_sels(is_inf_msb, f_e_mask, f_denorm_result);
	const u32 f_nan_result = _uint32_sels(is_nan_msb, f_em_nan, f_inf_result);
	const u32 f_result = _uint32_or(f_s, f_nan_result);

	return (f_result);
}

namespace Core
{
	void HalfToFloat(const u16* in, f32* out, i32 num)
	{
		for(i32 i = 0; i < num; ++i)
			*reinterpret_cast<u32*>(out++) = half_to_float(*in++);
	}

	void FloatToHalf(const f32* in, u16* out, i32 num)
	{
		for(i32 i = 0; i < num; ++i)
			*out++ = float_to_half(*reinterpret_cast<const u32*>(in++));
	}
} // namespace Core
