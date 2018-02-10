#include "core/hash.h"

#include <cstring>

namespace Core
{
// MD5: https://github.com/B-Con/crypto-algorithms
#if PLATFORM_WINDOWS
#pragma warning(push)
#pragma warning(disable : 4244)
#endif
	namespace
	{
		struct MD5_CTX
		{
			u8 data[64];
			u16 datalen;
			u64 bitlen;
			u32 state[4];
		};

#define ROTLEFT(a, b) ((a << b) | (a >> (32 - b)))

#define F(x, y, z) ((x & y) | (~x & z))
#define G(x, y, z) ((x & z) | (y & ~z))
#define H(x, y, z) (x ^ y ^ z)
#define I(x, y, z) (y ^ (x | ~z))

#define FF(a, b, c, d, m, s, t)                                                                                        \
	{                                                                                                                  \
		a += F(b, c, d) + m + t;                                                                                       \
		a = b + ROTLEFT(a, s);                                                                                         \
	}
#define GG(a, b, c, d, m, s, t)                                                                                        \
	{                                                                                                                  \
		a += G(b, c, d) + m + t;                                                                                       \
		a = b + ROTLEFT(a, s);                                                                                         \
	}
#define HH(a, b, c, d, m, s, t)                                                                                        \
	{                                                                                                                  \
		a += H(b, c, d) + m + t;                                                                                       \
		a = b + ROTLEFT(a, s);                                                                                         \
	}
#define II(a, b, c, d, m, s, t)                                                                                        \
	{                                                                                                                  \
		a += I(b, c, d) + m + t;                                                                                       \
		a = b + ROTLEFT(a, s);                                                                                         \
	}

		void md5_transform(MD5_CTX* ctx, const u8 data[])
		{
			u32 a, b, c, d, m[16], i, j;

			// MD5 specifies big endian byte order, but this implementation assumes a little
			// endian byte order CPU. Reverse all the bytes upon input, and re-reverse them
			// on output (in md5_final()).
			for(i = 0, j = 0; i < 16; ++i, j += 4)
				m[i] = (data[j]) + (data[j + 1] << 8) + (data[j + 2] << 16) + (data[j + 3] << 24);

			a = ctx->state[0];
			b = ctx->state[1];
			c = ctx->state[2];
			d = ctx->state[3];

			FF(a, b, c, d, m[0], 7, 0xd76aa478);
			FF(d, a, b, c, m[1], 12, 0xe8c7b756);
			FF(c, d, a, b, m[2], 17, 0x242070db);
			FF(b, c, d, a, m[3], 22, 0xc1bdceee);
			FF(a, b, c, d, m[4], 7, 0xf57c0faf);
			FF(d, a, b, c, m[5], 12, 0x4787c62a);
			FF(c, d, a, b, m[6], 17, 0xa8304613);
			FF(b, c, d, a, m[7], 22, 0xfd469501);
			FF(a, b, c, d, m[8], 7, 0x698098d8);
			FF(d, a, b, c, m[9], 12, 0x8b44f7af);
			FF(c, d, a, b, m[10], 17, 0xffff5bb1);
			FF(b, c, d, a, m[11], 22, 0x895cd7be);
			FF(a, b, c, d, m[12], 7, 0x6b901122);
			FF(d, a, b, c, m[13], 12, 0xfd987193);
			FF(c, d, a, b, m[14], 17, 0xa679438e);
			FF(b, c, d, a, m[15], 22, 0x49b40821);

			GG(a, b, c, d, m[1], 5, 0xf61e2562);
			GG(d, a, b, c, m[6], 9, 0xc040b340);
			GG(c, d, a, b, m[11], 14, 0x265e5a51);
			GG(b, c, d, a, m[0], 20, 0xe9b6c7aa);
			GG(a, b, c, d, m[5], 5, 0xd62f105d);
			GG(d, a, b, c, m[10], 9, 0x02441453);
			GG(c, d, a, b, m[15], 14, 0xd8a1e681);
			GG(b, c, d, a, m[4], 20, 0xe7d3fbc8);
			GG(a, b, c, d, m[9], 5, 0x21e1cde6);
			GG(d, a, b, c, m[14], 9, 0xc33707d6);
			GG(c, d, a, b, m[3], 14, 0xf4d50d87);
			GG(b, c, d, a, m[8], 20, 0x455a14ed);
			GG(a, b, c, d, m[13], 5, 0xa9e3e905);
			GG(d, a, b, c, m[2], 9, 0xfcefa3f8);
			GG(c, d, a, b, m[7], 14, 0x676f02d9);
			GG(b, c, d, a, m[12], 20, 0x8d2a4c8a);

			HH(a, b, c, d, m[5], 4, 0xfffa3942);
			HH(d, a, b, c, m[8], 11, 0x8771f681);
			HH(c, d, a, b, m[11], 16, 0x6d9d6122);
			HH(b, c, d, a, m[14], 23, 0xfde5380c);
			HH(a, b, c, d, m[1], 4, 0xa4beea44);
			HH(d, a, b, c, m[4], 11, 0x4bdecfa9);
			HH(c, d, a, b, m[7], 16, 0xf6bb4b60);
			HH(b, c, d, a, m[10], 23, 0xbebfbc70);
			HH(a, b, c, d, m[13], 4, 0x289b7ec6);
			HH(d, a, b, c, m[0], 11, 0xeaa127fa);
			HH(c, d, a, b, m[3], 16, 0xd4ef3085);
			HH(b, c, d, a, m[6], 23, 0x04881d05);
			HH(a, b, c, d, m[9], 4, 0xd9d4d039);
			HH(d, a, b, c, m[12], 11, 0xe6db99e5);
			HH(c, d, a, b, m[15], 16, 0x1fa27cf8);
			HH(b, c, d, a, m[2], 23, 0xc4ac5665);

			II(a, b, c, d, m[0], 6, 0xf4292244);
			II(d, a, b, c, m[7], 10, 0x432aff97);
			II(c, d, a, b, m[14], 15, 0xab9423a7);
			II(b, c, d, a, m[5], 21, 0xfc93a039);
			II(a, b, c, d, m[12], 6, 0x655b59c3);
			II(d, a, b, c, m[3], 10, 0x8f0ccc92);
			II(c, d, a, b, m[10], 15, 0xffeff47d);
			II(b, c, d, a, m[1], 21, 0x85845dd1);
			II(a, b, c, d, m[8], 6, 0x6fa87e4f);
			II(d, a, b, c, m[15], 10, 0xfe2ce6e0);
			II(c, d, a, b, m[6], 15, 0xa3014314);
			II(b, c, d, a, m[13], 21, 0x4e0811a1);
			II(a, b, c, d, m[4], 6, 0xf7537e82);
			II(d, a, b, c, m[11], 10, 0xbd3af235);
			II(c, d, a, b, m[2], 15, 0x2ad7d2bb);
			II(b, c, d, a, m[9], 21, 0xeb86d391);

			ctx->state[0] += a;
			ctx->state[1] += b;
			ctx->state[2] += c;
			ctx->state[3] += d;
		}

		void md5_init(MD5_CTX* ctx)
		{
			ctx->datalen = 0;
			ctx->bitlen = 0;
			ctx->state[0] = 0x67452301;
			ctx->state[1] = 0xEFCDAB89;
			ctx->state[2] = 0x98BADCFE;
			ctx->state[3] = 0x10325476;
		}

		void md5_update(MD5_CTX* ctx, const u8* data, size_t len)
		{
			size_t i;

			for(i = 0; i < len; ++i)
			{
				ctx->data[ctx->datalen] = data[i];
				ctx->datalen++;
				if(ctx->datalen == 64)
				{
					md5_transform(ctx, ctx->data);
					ctx->bitlen += 512;
					ctx->datalen = 0;
				}
			}
		}

		void md5_final(MD5_CTX* ctx, u8* hash)
		{
			size_t i;

			i = ctx->datalen;

			// Pad whatever data is left in the buffer.
			if(ctx->datalen < 56)
			{
				ctx->data[i++] = 0x80;
				while(i < 56)
					ctx->data[i++] = 0x00;
			}
			else if(ctx->datalen >= 56)
			{
				ctx->data[i++] = 0x80;
				while(i < 64)
					ctx->data[i++] = 0x00;
				md5_transform(ctx, ctx->data);
				memset(ctx->data, 0, 56);
			}

			// Append to the padding the total message's length in bits and transform.
			ctx->bitlen += ctx->datalen * 8;
			ctx->data[56] = ctx->bitlen;
			ctx->data[57] = ctx->bitlen >> 8;
			ctx->data[58] = ctx->bitlen >> 16;
			ctx->data[59] = ctx->bitlen >> 24;
			ctx->data[60] = ctx->bitlen >> 32;
			ctx->data[61] = ctx->bitlen >> 40;
			ctx->data[62] = ctx->bitlen >> 48;
			ctx->data[63] = ctx->bitlen >> 56;
			md5_transform(ctx, ctx->data);

			// Since this implementation uses little endian byte ordering and MD uses big endian,
			// reverse all the bytes when copying the final state to the output hash.
			for(i = 0; i < 4; ++i)
			{
				hash[i] = (ctx->state[0] >> (i * 8)) & 0x000000ff;
				hash[i + 4] = (ctx->state[1] >> (i * 8)) & 0x000000ff;
				hash[i + 8] = (ctx->state[2] >> (i * 8)) & 0x000000ff;
				hash[i + 12] = (ctx->state[3] >> (i * 8)) & 0x000000ff;
			}
		}
	}
#if PLATFORM_WINDOWS
#pragma warning(pop)
#endif

	HashMD5Digest HashMD5(const void* data, size_t size)
	{
		HashMD5Digest digest;
		MD5_CTX ctx;
		md5_init(&ctx);
		md5_update(&ctx, (const u8*)data, size);
		md5_final(&ctx, digest.data8_);
		return digest;
	}


// SHA-1: https://github.com/B-Con/crypto-algorithms
#if PLATFORM_WINDOWS
#pragma warning(push)
#pragma warning(disable : 4244)
#endif
	namespace
	{
		struct SHA1_CTX
		{
			u8 data[64];
			u16 datalen;
			u64 bitlen;
			u32 state[5];
			u32 k[4];
		};
#define ROTLEFT(a, b) ((a << b) | (a >> (32 - b)))

		/*********************** FUNCTION DEFINITIONS ***********************/
		void sha1_transform(SHA1_CTX* ctx, const u8 data[])
		{
			u32 a, b, c, d, e, i, j, t, m[80];

			for(i = 0, j = 0; i < 16; ++i, j += 4)
				m[i] = (data[j] << 24) + (data[j + 1] << 16) + (data[j + 2] << 8) + (data[j + 3]);
			for(; i < 80; ++i)
			{
				m[i] = (m[i - 3] ^ m[i - 8] ^ m[i - 14] ^ m[i - 16]);
				m[i] = (m[i] << 1) | (m[i] >> 31);
			}

			a = ctx->state[0];
			b = ctx->state[1];
			c = ctx->state[2];
			d = ctx->state[3];
			e = ctx->state[4];

			for(i = 0; i < 20; ++i)
			{
				t = ROTLEFT(a, 5) + ((b & c) ^ (~b & d)) + e + ctx->k[0] + m[i];
				e = d;
				d = c;
				c = ROTLEFT(b, 30);
				b = a;
				a = t;
			}
			for(; i < 40; ++i)
			{
				t = ROTLEFT(a, 5) + (b ^ c ^ d) + e + ctx->k[1] + m[i];
				e = d;
				d = c;
				c = ROTLEFT(b, 30);
				b = a;
				a = t;
			}
			for(; i < 60; ++i)
			{
				t = ROTLEFT(a, 5) + ((b & c) ^ (b & d) ^ (c & d)) + e + ctx->k[2] + m[i];
				e = d;
				d = c;
				c = ROTLEFT(b, 30);
				b = a;
				a = t;
			}
			for(; i < 80; ++i)
			{
				t = ROTLEFT(a, 5) + (b ^ c ^ d) + e + ctx->k[3] + m[i];
				e = d;
				d = c;
				c = ROTLEFT(b, 30);
				b = a;
				a = t;
			}

			ctx->state[0] += a;
			ctx->state[1] += b;
			ctx->state[2] += c;
			ctx->state[3] += d;
			ctx->state[4] += e;
		}

		void sha1_init(SHA1_CTX* ctx)
		{
			ctx->datalen = 0;
			ctx->bitlen = 0;
			ctx->state[0] = 0x67452301;
			ctx->state[1] = 0xEFCDAB89;
			ctx->state[2] = 0x98BADCFE;
			ctx->state[3] = 0x10325476;
			ctx->state[4] = 0xc3d2e1f0;
			ctx->k[0] = 0x5a827999;
			ctx->k[1] = 0x6ed9eba1;
			ctx->k[2] = 0x8f1bbcdc;
			ctx->k[3] = 0xca62c1d6;
		}

		void sha1_update(SHA1_CTX* ctx, const u8 data[], size_t len)
		{
			size_t i;

			for(i = 0; i < len; ++i)
			{
				ctx->data[ctx->datalen] = data[i];
				ctx->datalen++;
				if(ctx->datalen == 64)
				{
					sha1_transform(ctx, ctx->data);
					ctx->bitlen += 512;
					ctx->datalen = 0;
				}
			}
		}

		void sha1_final(SHA1_CTX* ctx, u8 hash[])
		{
			u32 i;

			i = ctx->datalen;

			// Pad whatever data is left in the buffer.
			if(ctx->datalen < 56)
			{
				ctx->data[i++] = 0x80;
				while(i < 56)
					ctx->data[i++] = 0x00;
			}
			else
			{
				ctx->data[i++] = 0x80;
				while(i < 64)
					ctx->data[i++] = 0x00;
				sha1_transform(ctx, ctx->data);
				memset(ctx->data, 0, 56);
			}

			// Append to the padding the total message's length in bits and transform.
			ctx->bitlen += ctx->datalen * 8;
			ctx->data[63] = ctx->bitlen;
			ctx->data[62] = ctx->bitlen >> 8;
			ctx->data[61] = ctx->bitlen >> 16;
			ctx->data[60] = ctx->bitlen >> 24;
			ctx->data[59] = ctx->bitlen >> 32;
			ctx->data[58] = ctx->bitlen >> 40;
			ctx->data[57] = ctx->bitlen >> 48;
			ctx->data[56] = ctx->bitlen >> 56;
			sha1_transform(ctx, ctx->data);

			// Since this implementation uses little endian byte ordering and MD uses big endian,
			// reverse all the bytes when copying the final state to the output hash.
			for(i = 0; i < 4; ++i)
			{
				hash[i] = (ctx->state[0] >> (24 - i * 8)) & 0x000000ff;
				hash[i + 4] = (ctx->state[1] >> (24 - i * 8)) & 0x000000ff;
				hash[i + 8] = (ctx->state[2] >> (24 - i * 8)) & 0x000000ff;
				hash[i + 12] = (ctx->state[3] >> (24 - i * 8)) & 0x000000ff;
				hash[i + 16] = (ctx->state[4] >> (24 - i * 8)) & 0x000000ff;
			}
		}
	}
#if PLATFORM_WINDOWS
#pragma warning(pop)
#endif
	HashSHA1Digest HashSHA1(const void* data, size_t size)
	{
		HashSHA1Digest digest;
		SHA1_CTX ctx;
		sha1_init(&ctx);
		sha1_update(&ctx, (const u8*)data, size);
		sha1_final(&ctx, digest.data8_);
		return digest;
	}


	namespace
	{
		// clang-format off
	const u32 gCRC32Table_[256] =
	{
		0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419,
		0x706af48f, 0xe963a535, 0x9e6495a3, 0x0edb8832, 0x79dcb8a4,
		0xe0d5e91e, 0x97d2d988, 0x09b64c2b, 0x7eb17cbd, 0xe7b82d07,
		0x90bf1d91, 0x1db71064, 0x6ab020f2, 0xf3b97148, 0x84be41de,
		0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7, 0x136c9856,
		0x646ba8c0, 0xfd62f97a, 0x8a65c9ec, 0x14015c4f, 0x63066cd9,
		0xfa0f3d63, 0x8d080df5, 0x3b6e20c8, 0x4c69105e, 0xd56041e4,
		0xa2677172, 0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b,
		0x35b5a8fa, 0x42b2986c, 0xdbbbc9d6, 0xacbcf940, 0x32d86ce3,
		0x45df5c75, 0xdcd60dcf, 0xabd13d59, 0x26d930ac, 0x51de003a,
		0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423, 0xcfba9599,
		0xb8bda50f, 0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
		0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d, 0x76dc4190,
		0x01db7106, 0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f,
		0x9fbfe4a5, 0xe8b8d433, 0x7807c9a2, 0x0f00f934, 0x9609a88e,
		0xe10e9818, 0x7f6a0dbb, 0x086d3d2d, 0x91646c97, 0xe6635c01,
		0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e, 0x6c0695ed,
		0x1b01a57b, 0x8208f4c1, 0xf50fc457, 0x65b0d9c6, 0x12b7e950,
		0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3,
		0xfbd44c65, 0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2,
		0x4adfa541, 0x3dd895d7, 0xa4d1c46d, 0xd3d6f4fb, 0x4369e96a,
		0x346ed9fc, 0xad678846, 0xda60b8d0, 0x44042d73, 0x33031de5,
		0xaa0a4c5f, 0xdd0d7cc9, 0x5005713c, 0x270241aa, 0xbe0b1010,
		0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
		0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17,
		0x2eb40d81, 0xb7bd5c3b, 0xc0ba6cad, 0xedb88320, 0x9abfb3b6,
		0x03b6e20c, 0x74b1d29a, 0xead54739, 0x9dd277af, 0x04db2615,
		0x73dc1683, 0xe3630b12, 0x94643b84, 0x0d6d6a3e, 0x7a6a5aa8,
		0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1, 0xf00f9344,
		0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb,
		0x196c3671, 0x6e6b06e7, 0xfed41b76, 0x89d32be0, 0x10da7a5a,
		0x67dd4acc, 0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5,
		0xd6d6a3e8, 0xa1d1937e, 0x38d8c2c4, 0x4fdff252, 0xd1bb67f1,
		0xa6bc5767, 0x3fb506dd, 0x48b2364b, 0xd80d2bda, 0xaf0a1b4c,
		0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55, 0x316e8eef,
		0x4669be79, 0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
		0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f, 0xc5ba3bbe,
		0xb2bd0b28, 0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31,
		0x2cd99e8b, 0x5bdeae1d, 0x9b64c2b0, 0xec63f226, 0x756aa39c,
		0x026d930a, 0x9c0906a9, 0xeb0e363f, 0x72076785, 0x05005713,
		0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38, 0x92d28e9b,
		0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21, 0x86d3d2d4, 0xf1d4e242,
		0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1,
		0x18b74777, 0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c,
		0x8f659eff, 0xf862ae69, 0x616bffd3, 0x166ccf45, 0xa00ae278,
		0xd70dd2ee, 0x4e048354, 0x3903b3c2, 0xa7672661, 0xd06016f7,
		0x4969474d, 0x3e6e77db, 0xaed16a4a, 0xd9d65adc, 0x40df0b66,
		0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
		0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605,
		0xcdd70693, 0x54de5729, 0x23d967bf, 0xb3667a2e, 0xc4614ab8,
		0x5d681b02, 0x2a6f2b94, 0xb40bbe37, 0xc30c8ea1, 0x5a05df1b,
		0x2d02ef8d
	};
		// clang-format on
	} // namespace

	u32 HashCRC32(u32 Input, const void* pInData, size_t Size)
	{
		const u8* Data = reinterpret_cast<const u8*>(pInData);
		u32 Hash = Input;

		Hash = ~Hash;

		while(Size--)
		{
			Hash = gCRC32Table_[(Hash ^ *Data++) & 0xff] ^ (Hash >> 8);
		}

		return ~Hash;
	}

	u32 HashSDBM(u32 Input, const void* pInData, size_t Size)
	{
		const u8* Data = reinterpret_cast<const u8*>(pInData);
		u32 Hash = Input;

		while(Size--)
		{
			Hash = (u32)*Data++ + (Hash << 6) + (Hash << 16) - Hash;
		}

		return Hash;
	}

	namespace
	{
	// clang-format off
		/*
		 * hash_64 - 64 bit Fowler/Noll/Vo-0 FNV-1a hash code
		 *
		 * @(#) $Revision: 5.1 $
		 * @(#) $Id: hash_64a.c,v 5.1 2009/06/30 09:01:38 chongo Exp $
		 * @(#) $Source: /usr/local/src/cmd/fnv/RCS/hash_64a.c,v $
		 *
		 ***
		 *
		 * Fowler/Noll/Vo hash
		 *
		 * The basis of this hash algorithm was taken from an idea sent
		 * as reviewer comments to the IEEE POSIX P1003.2 committee by:
		 *
		 *      Phong Vo (http://www.research.att.com/info/kpv/)
		 *      Glenn Fowler (http://www.research.att.com/~gsf/)
		 *
		 * In a subsequent ballot round:
		 *
		 *      Landon Curt Noll (http://www.isthe.com/chongo/)
		 *
		 * improved on their algorithm.  Some people tried this hash
		 * and found that it worked rather well.  In an EMail message
		 * to Landon, they named it the ``Fowler/Noll/Vo'' or FNV hash.
		 *
		 * FNV hashes are designed to be fast while maintaining a low
		 * collision rate. The FNV speed allows one to quickly hash lots
		 * of data while maintaining a reasonable collision rate.  See:
		 *
		 *      http://www.isthe.com/chongo/tech/comp/fnv/index.html
		 *
		 * for more details as well as other forms of the FNV hash.
		 *
		 ***
		 *
		 * To use the recommended 64 bit FNV-1a hash, pass FNV1A_64_INIT as the
		 * Fnv64_t hashval argument to fnv_64a_buf() or fnv_64a_str().
		 *
		 ***
		 *
		 * Please do not copyright this code.  This code is in the public domain.
		 *
		 * LANDON CURT NOLL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
		 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO
		 * EVENT SHALL LANDON CURT NOLL BE LIABLE FOR ANY SPECIAL, INDIRECT OR
		 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF
		 * USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
		 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
		 * PERFORMANCE OF THIS SOFTWARE.
		 *
		 * By:
		 *	chongo <Landon Curt Noll> /\oo/\
		 *      http://www.isthe.com/chongo/
		 *
		 * Share and Enjoy!	:-)
		 */

		#define FNV_VERSION "5.0.2"	/* @(#) FNV Version */

		#define HAVE_64BIT_LONG_LONG

		/*
		 * 32 bit FNV-0 hash type
		 */
		typedef u32 Fnv32_t;


		/*
		 * 32 bit FNV-0 zero initial basis
		 *
		 * This historic hash is not recommended.  One should use
		 * the FNV-1 hash and initial basis instead.
		 */
		#define FNV0_32_INIT ((Fnv32_t)0)


		/*
		 * 32 bit FNV-1 and FNV-1a non-zero initial basis
		 *
		 * The FNV-1 initial basis is the FNV-0 hash of the following 32 octets:
		 *
		 *              chongo <Landon Curt Noll> /\../\
		 *
		 * NOTE: The \'s above are not back-slashing escape characters.
		 * They are literal ASCII  backslash 0x5c characters.
		 *
		 * NOTE: The FNV-1a initial basis is the same value as FNV-1 by definition.
		 */
		#define FNV1_32_INIT ((Fnv32_t)0x811c9dc5)
		#define FNV1_32A_INIT FNV1_32_INIT


		/*
		 * 64 bit FNV-0 hash
		 */
		#if defined(HAVE_64BIT_LONG_LONG)
		typedef u64 Fnv64_t;
		#else /* HAVE_64BIT_LONG_LONG */
		typedef struct {
			u32 w32[2]; /* w32[0] is low order, w32[1] is high order word */
		} Fnv64_t;
		#endif /* HAVE_64BIT_LONG_LONG */


		/*
		 * 64 bit FNV-0 zero initial basis
		 *
		 * This historic hash is not recommended.  One should use
		 * the FNV-1 hash and initial basis instead.
		 */
		#if defined(HAVE_64BIT_LONG_LONG)
		#define FNV0_64_INIT ((Fnv64_t)0)
		#else /* HAVE_64BIT_LONG_LONG */
		extern const Fnv64_t fnv0_64_init;
		#define FNV0_64_INIT (fnv0_64_init)
		#endif /* HAVE_64BIT_LONG_LONG */


		/*
		 * 64 bit FNV-1 non-zero initial basis
		 *
		 * The FNV-1 initial basis is the FNV-0 hash of the following 32 octets:
		 *
		 *              chongo <Landon Curt Noll> /\../\
		 *
		 * NOTE: The \'s above are not back-slashing escape characters.
		 * They are literal ASCII  backslash 0x5c characters.
		 *
		 * NOTE: The FNV-1a initial basis is the same value as FNV-1 by definition.
		 */
		#if defined(HAVE_64BIT_LONG_LONG)
		#define FNV1_64_INIT ((Fnv64_t)0xcbf29ce484222325ULL)
		#define FNV1A_64_INIT FNV1_64_INIT
		#else /* HAVE_64BIT_LONG_LONG */
		extern const fnv1_64_init;
		extern const Fnv64_t fnv1a_64_init;
		#define FNV1_64_INIT (fnv1_64_init)
		#define FNV1A_64_INIT (fnv1a_64_init)
		#endif /* HAVE_64BIT_LONG_LONG */

		/*
		 * FNV-1a defines the initial basis to be non-zero
		 */
		#if !defined(HAVE_64BIT_LONG_LONG)
		const Fnv64_t fnv1a_64_init = { 0x84222325, 0xcbf29ce4 };
		#endif /* ! HAVE_64BIT_LONG_LONG */


		/*
		 * 64 bit magic FNV-1a prime
		 */
		#define FNV_64_PRIME ((Fnv64_t)0x100000001b3ULL)


		/*
		 * fnv_64a_buf - perform a 64 bit Fowler/Noll/Vo FNV-1a hash on a buffer
		 *
		 * input:
		 *	buf	- start of buffer to hash
		 *	len	- length of buffer in octets
		 *	hval	- previous hash value or 0 if first call
		 *
		 * returns:
		 *	64 bit hash as a static hash type
		 *
		 * NOTE: To use the recommended 64 bit FNV-1a hash, use FNV1A_64_INIT as the
		 * 	 hval arg on the first call to either fnv_64a_buf() or fnv_64a_str().
		 */
		Fnv64_t
		fnv_64a_buf(const void *buf, size_t len, Fnv64_t hval)
		{
			unsigned char *bp = (unsigned char *)buf;	/* start of buffer */
			unsigned char *be = bp + len;		/* beyond end of buffer */

		#if defined(HAVE_64BIT_LONG_LONG)
			/*
			 * FNV-1a hash each octet of the buffer
			 */
			while (bp < be) {

			/* xor the bottom with the current octet */
			hval ^= (Fnv64_t)*bp++;

			/* multiply by the 64 bit FNV magic prime mod 2^64 */
		#if defined(NO_FNV_GCC_OPTIMIZATION)
			hval *= FNV_64_PRIME;
		#else /* NO_FNV_GCC_OPTIMIZATION */
			hval += (hval << 1) + (hval << 4) + (hval << 5) +
				(hval << 7) + (hval << 8) + (hval << 40);
		#endif /* NO_FNV_GCC_OPTIMIZATION */
			}

		#else /* HAVE_64BIT_LONG_LONG */

			unsigned long val[4];			/* hash value in base 2^16 */
			unsigned long tmp[4];			/* tmp 64 bit value */

			/*
			 * Convert Fnv64_t hval into a base 2^16 array
			 */
			val[0] = hval.w32[0];
			val[1] = (val[0] >> 16);
			val[0] &= 0xffff;
			val[2] = hval.w32[1];
			val[3] = (val[2] >> 16);
			val[2] &= 0xffff;

			/*
			 * FNV-1a hash each octet of the buffer
			 */
			while (bp < be) {

			/* xor the bottom with the current octet */
			val[0] ^= (unsigned long)*bp++;

			/*
			 * multiply by the 64 bit FNV magic prime mod 2^64
			 *
			 * Using 0x100000001b3 we have the following digits base 2^16:
			 *
			 *	0x0	0x100	0x0	0x1b3
			 *
			 * which is the same as:
			 *
			 *	0x0	1<<FNV_64_PRIME_SHIFT	0x0	FNV_64_PRIME_LOW
			 */
			/* multiply by the lowest order digit base 2^16 */
			tmp[0] = val[0] * FNV_64_PRIME_LOW;
			tmp[1] = val[1] * FNV_64_PRIME_LOW;
			tmp[2] = val[2] * FNV_64_PRIME_LOW;
			tmp[3] = val[3] * FNV_64_PRIME_LOW;
			/* multiply by the other non-zero digit */
			tmp[2] += val[0] << FNV_64_PRIME_SHIFT;	/* tmp[2] += val[0] * 0x100 */
			tmp[3] += val[1] << FNV_64_PRIME_SHIFT;	/* tmp[3] += val[1] * 0x100 */
			/* propagate carries */
			tmp[1] += (tmp[0] >> 16);
			val[0] = tmp[0] & 0xffff;
			tmp[2] += (tmp[1] >> 16);
			val[1] = tmp[1] & 0xffff;
			val[3] = tmp[3] + (tmp[2] >> 16);
			val[2] = tmp[2] & 0xffff;
			/*
			 * Doing a val[3] &= 0xffff; is not really needed since it simply
			 * removes multiples of 2^64.  We can discard these excess bits
			 * outside of the loop when we convert to Fnv64_t.
			 */
			}

			/*
			 * Convert base 2^16 array back into an Fnv64_t
			 */
			hval.w32[1] = ((val[3]<<16) | val[2]);
			hval.w32[0] = ((val[1]<<16) | val[0]);

		#endif /* HAVE_64BIT_LONG_LONG */

			/* return our new hash value */
			return hval;
		}


		/*
		 * fnv_64a_str - perform a 64 bit Fowler/Noll/Vo FNV-1a hash on a buffer
		 *
		 * input:
		 *	buf	- start of buffer to hash
		 *	hval	- previous hash value or 0 if first call
		 *
		 * returns:
		 *	64 bit hash as a static hash type
		 *
		 * NOTE: To use the recommended 64 bit FNV-1a hash, use FNV1A_64_INIT as the
		 * 	 hval arg on the first call to either fnv_64a_buf() or fnv_64a_str().
		 */
		Fnv64_t
		fnv_64a_str(const char *str, Fnv64_t hval)
		{
			unsigned char *s = (unsigned char *)str;	/* unsigned string */

		#if defined(HAVE_64BIT_LONG_LONG)

			/*
			 * FNV-1a hash each octet of the string
			 */
			while (*s) {

			/* xor the bottom with the current octet */
			hval ^= (Fnv64_t)*s++;

			/* multiply by the 64 bit FNV magic prime mod 2^64 */
		#if defined(NO_FNV_GCC_OPTIMIZATION)
			hval *= FNV_64_PRIME;
		#else /* NO_FNV_GCC_OPTIMIZATION */
			hval += (hval << 1) + (hval << 4) + (hval << 5) +
				(hval << 7) + (hval << 8) + (hval << 40);
		#endif /* NO_FNV_GCC_OPTIMIZATION */
			}

		#else /* !HAVE_64BIT_LONG_LONG */

			unsigned long val[4];	/* hash value in base 2^16 */
			unsigned long tmp[4];	/* tmp 64 bit value */

			/*
			 * Convert Fnv64_t hval into a base 2^16 array
			 */
			val[0] = hval.w32[0];
			val[1] = (val[0] >> 16);
			val[0] &= 0xffff;
			val[2] = hval.w32[1];
			val[3] = (val[2] >> 16);
			val[2] &= 0xffff;

			/*
			 * FNV-1a hash each octet of the string
			 */
			while (*s) {

			/* xor the bottom with the current octet */

			/*
			 * multiply by the 64 bit FNV magic prime mod 2^64
			 *
			 * Using 1099511628211, we have the following digits base 2^16:
			 *
			 *	0x0	0x100	0x0	0x1b3
			 *
			 * which is the same as:
			 *
			 *	0x0	1<<FNV_64_PRIME_SHIFT	0x0	FNV_64_PRIME_LOW
			 */
			/* multiply by the lowest order digit base 2^16 */
			tmp[0] = val[0] * FNV_64_PRIME_LOW;
			tmp[1] = val[1] * FNV_64_PRIME_LOW;
			tmp[2] = val[2] * FNV_64_PRIME_LOW;
			tmp[3] = val[3] * FNV_64_PRIME_LOW;
			/* multiply by the other non-zero digit */
			tmp[2] += val[0] << FNV_64_PRIME_SHIFT;	/* tmp[2] += val[0] * 0x100 */
			tmp[3] += val[1] << FNV_64_PRIME_SHIFT;	/* tmp[3] += val[1] * 0x100 */
			/* propagate carries */
			tmp[1] += (tmp[0] >> 16);
			val[0] = tmp[0] & 0xffff;
			tmp[2] += (tmp[1] >> 16);
			val[1] = tmp[1] & 0xffff;
			val[3] = tmp[3] + (tmp[2] >> 16);
			val[2] = tmp[2] & 0xffff;
			/*
			 * Doing a val[3] &= 0xffff; is not really needed since it simply
			 * removes multiples of 2^64.  We can discard these excess bits
			 * outside of the loop when we convert to Fnv64_t.
			 */
			val[0] ^= (unsigned long)(*s++);
			}

			/*
			 * Convert base 2^16 array back into an Fnv64_t
			 */
			hval.w32[1] = ((val[3]<<16) | val[2]);
			hval.w32[0] = ((val[1]<<16) | val[0]);

		#endif /* !HAVE_64BIT_LONG_LONG */

			/* return our new hash value */
			return hval;
		}
		// clang-format on
	}

	u64 HashFNV1a(u64 input, const void* pInData, size_t size) { return fnv_64a_buf(pInData, size, input); }

	u64 Hash(u64 Input, const char* Data)
	{ //
		return fnv_64a_str(Data, Input);
	}
} // namespace Core
