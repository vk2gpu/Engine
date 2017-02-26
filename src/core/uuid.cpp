#include "core/uuid.h"
#include "core/hash.h"

#include <string.h>

namespace Core
{
	UUID::UUID(const char* string, u8 variant)
	{
		HashSHA1Digest digest = HashSHA1(string, strlen(string));
		SetInternal(digest.data32_[0], digest.data32_[1], digest.data32_[2], digest.data32_[3], 5, variant);
	}

	void UUID::AsString(char* outStr)
	{
		auto writeByte = [](char*& outStr, u8 byte) {
			char base0 = '0';
			char base10 = 'a' - 10;
			u8 b0 = (byte >> 4) & 0x0f;
			u8 b1 = byte & 0x0f;
			if(b0 < 10)
				*outStr++ = base0 + b0;
			else
				*outStr++ = base10 + b0;
			if(b1 < 10)
				*outStr++ = base0 + b1;
			else
				*outStr++ = base10 + b1;
		};

		for(i32 i = 0; i < 4; ++i)
			writeByte(outStr, data8_[i]);
		*outStr++ = '-';
		for(i32 i = 4; i < 6; ++i)
			writeByte(outStr, data8_[i]);
		*outStr++ = '-';
		for(i32 i = 6; i < 8; ++i)
			writeByte(outStr, data8_[i]);
		*outStr++ = '-';
		for(i32 i = 8; i < 10; ++i)
			writeByte(outStr, data8_[i]);
		*outStr++ = '-';
		for(i32 i = 10; i < 16; ++i)
			writeByte(outStr, data8_[i]);
		*outStr++ = '\0';
	}

	void UUID::SetInternal(u32 data0, u32 data1, u32 data2, u32 data3, u8 version, u8 variant)
	{
		data32_[0] = data0;
		data32_[1] = data1;
		data32_[2] = data2;
		data32_[3] = data3;
		data8_[6] = (data8_[6] & 0x0f) | (version << 4);
		data8_[8] = (data8_[8] & 0x1f) | (variant << 6);
	}

	u32 Hash(u32 input, const UUID& data)
	{ //
		return HashCRC32(input, &data, sizeof(data));
	}
} // namespace core
