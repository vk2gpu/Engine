#pragma once

#include "core/dll.h"
#include "core/types.h"

namespace Core
{
	/**
	 * Convert from UTF-16 to UTF-8.
	 * @param src Source string.
	 * @param srcLength Source string length.
	 * @param dst Destination string.
	 * @param dstLength Destination length including null terminator.
	 * @return Success.
	 * @pre src != nullptr.
	 * @pre dst != nullptr.
	 * @pre srcLength > 0.
	 * @pre dstLength > 1.
	 */
	CORE_DLL bool StringConvertUTF16toUTF8(const wchar* src, i32 srcLength, char* dst, i32 dstLength);

	/**
	 * Convert from UTF-8 to UTF-16.
	 * @param src Source string.
	 * @param srcLength Source string length.
	 * @param dst Destination string.
	 * @param dstLength Destination length including null terminator.
	 * @return Success.
	 * @pre src != nullptr.
	 * @pre dst != nullptr.
	 * @pre srcLength > 0.
	 * @pre dstLength > 1.
	 */
	CORE_DLL bool StringConvertUTF8toUTF16(const char* src, i32 srcLength, wchar* dst, i32 dstLength);

} // end namespace Core
