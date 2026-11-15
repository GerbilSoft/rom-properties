/***************************************************************************
 * ROM Properties Page shell extension. (librptext)                        *
 * fourCC.cpp: FourCC string conversion functions                          *
 *                                                                         *
 * Copyright (c) 2009-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "fourCC.hpp"
#include "librpbyteswap/byteswap_rp.h"

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
	union {
		char c[4];
		uint32_t u32;
	} conv;
	conv.u32 = cpu_to_be32(fourCC);

	memcpy(buf, &conv.c, 4);
	buf[4] = '\0';
	return 0;
}

/**
 * Convert a host-endian FourCC to a string.
 * @param fourCC	[in] FourCC
 * @return FourCC as a string on success; empty string on error.
 */
string fourCCtoString(uint32_t fourCC)
{
	string s_ret;
	s_ret.resize(4);

	union {
		char c[4];
		uint32_t u32;
	} conv;
	conv.u32 = cpu_to_be32(fourCC);

	memcpy(&s_ret[0], conv.c, sizeof(conv.c));
	return s_ret;
}

}
