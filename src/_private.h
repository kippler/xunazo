

#pragma once


#define ERROR_RETURN {ASSERT(0);return false;}



uint32_t crc32(uint32_t crc, const uint8_t *ptr, size_t buf_len);



#undef min
#undef max

#include <algorithm>