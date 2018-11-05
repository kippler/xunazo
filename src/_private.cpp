

#include "stdafx.h"
#include "_private.h"


#define ERROR_RETURN {ASSERT(0);return false;}





// Karl Malbrain's compact CRC-32, with pre and post conditioning. 
// See "A compact CCITT crc16 and crc32 C implementation that balances processor cache usage against speed": 
// http://www.geocities.ws/malbrain/crc_c.html
static const uint32_t s_crc32[16] =
{
	0x00000000, 0x1db71064, 0x3b6e20c8, 0x26d930ac, 0x76dc4190, 0x6b6b51f4, 0x4db26158, 0x5005713c,
	0xedb88320, 0xf00f9344, 0xd6d6a3e8, 0xcb61b38c, 0x9b64c2b0, 0x86d3d2d4, 0xa00ae278, 0xbdbdf21c
};

uint32_t crc32(uint32_t crc, const uint8_t *ptr, size_t buf_len)
{
	if (!ptr)
		return 0;

	crc = ~crc;
	while (buf_len--)
	{
		const uint8_t b = *ptr++;
		crc = (crc >> 4) ^ s_crc32[(crc & 0xF) ^ (b & 0xF)];
		crc = (crc >> 4) ^ s_crc32[(crc & 0xF) ^ (b >> 4)];
	}
	return ~crc;
}





