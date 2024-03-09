/***************************************************************************
 * ROM Properties Page shell extension. (librptext)                        *
 * fourCC.cpp: FourCC string conversion functions                          *
 *                                                                         *
 * Copyright (c) 2009-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "fourCC.hpp"

// C includes (C++ namespace)
#include <cassert>
#include <cerrno>

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
	if (size < 5)
		return -ENOMEM;

	// Convert from a FourCC to a string.
	// TODO: If system is big-endian, just do a memcpy()?
	buf[0] = (fourCC >> 24) & 0xFF;
	buf[1] = (fourCC >> 16) & 0xFF;
	buf[2] = (fourCC >>  8) & 0xFF;
	buf[3] =  fourCC        & 0xFF;
	buf[4] = '\0';

	return 0;
}

}
