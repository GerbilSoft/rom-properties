/***************************************************************************
 * ROM Properties Page shell extension. (librptext)                        *
 * fourCC.cpp: FourCC string conversion functions                          *
 *                                                                         *
 * Copyright (c) 2009-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "fourCC.hpp"
#include "librpbyteswap/byteorder.h"

// C includes (C++ namespace)
#include <cassert>
#include <cerrno>
#include <cstring>

// C++ STL classes
using std::string;

namespace LibRpText {

/**
 * Convert a host-endian FourCC to a string.
 * @param buf		[out] Output buffer
 * @param size		[in] Size of buf (should be >= 5)
 * @param fourCC	[in] FourCC
 * @return 0 on success; negative POSIX error code on error.
 */
int fourCCtoString(char *buf, size_t size, uint32_t fourCC)
{
	assert(size >= 5);
	if (size < 5) {
		return -ENOMEM;
	}

	// Convert from a FourCC to a string.
#if SYS_BYTEORDER == SYS_LIL_ENDIAN
	union {
		char c[4];
		uint32_t u32;
	} conv;
	conv.u32 = fourCC;
	buf[0] = conv.c[3];
	buf[1] = conv.c[2];
	buf[2] = conv.c[1];
	buf[3] = conv.c[0];
	buf[4] = '\0';
#else /* SYS_BYTEORDER == SYS_BIG_ENDIAN */
	memcpy(buf, &fourCC, 4);
	buf[4] = '\0';
#endif

	return 0;
}

/**
 * Convert a host-endian FourCC to a string.
 * @param fourCC	[in] FourCC
 * @return FourCC as a string on success; empty string on error.
 */
string fourCCtoString(uint32_t fourCC)
{
	union {
		char c[4];
		uint32_t u32;
	} conv;
	conv.u32 = fourCC;
#if SYS_BYTEORDER == SYS_LIL_ENDIAN
	return {conv.c[3], conv.c[2], conv.c[1], conv.c[0]};
#else /* SYS_BYTEORDER == SYS_BIG_ENDIAN */
	return {conv.c[0], conv.c[1], conv.c[2], conv.c[3]};
#endif
}

}
