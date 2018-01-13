#pragma once

#include "core/dll.h"
#include "core/types.h"

namespace Core
{
	using ConvertFn = void(void*, const void*, int);

	enum class DataType : i32
	{
		INVALID = -1,
		FLOAT = 0,
		UNORM,
		SNORM,
		UINT,
		SINT,
		TYPELESS,
	};

	struct StreamDesc
	{
		StreamDesc() = default;

		StreamDesc(void* data, DataType dataType, i32 numBits, i32 stride)
		    : data_(data)
		    , dataType_(dataType)
		    , numBits_(numBits)
		    , stride_(stride)
		{
		}

		StreamDesc operator()(void* data)
		{
			StreamDesc out = *this;
			out.data_ = data;
			return out;
		}

		void* data_ = nullptr;
		DataType dataType_ = DataType::FLOAT;
		i32 numBits_ = 32;
		i32 stride_ = sizeof(f32);
	};

	/**
	 * Convert a stream of data from one type to another.
	 * @param outStream Output stream.
	 * @param inStream Input stream.
	 * @param num Number of elements.
	 * @param components Number of components per element.
	 * @return true is conversion successful.
	 */
	CORE_DLL bool Convert(StreamDesc outStream, StreamDesc inStream, i32 num, i32 components);


} // namespace Core
