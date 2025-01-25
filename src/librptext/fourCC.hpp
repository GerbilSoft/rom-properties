/***************************************************************************
 * ROM Properties Page shell extension. (librptext)                        *
 * fourCC.hpp: FourCC string conversion functions                          *
 *                                                                         *
 * Copyright (c) 2009-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "common.h"	// for ATTR_PRINTF()
#include "dll-macros.h"	// for RP_LIBROMDATA_PUBLIC

// C includes (C++ namespace)
#include <cstddef>	/* size_t */
#include <cstdint>

// C++ includes
#include <string>

namespace LibRpText {

/**
 * Convert a host-endian FourCC to a string.
 * @param buf		[out] Output buffer
 * @param size		[in] Size of buf (should be >= 5)
 * @param fourCC	[in] FourCC
 * @return 0 on success; negative POSIX error code on error.
 */
ATTR_ACCESS_SIZE(write_only, 1, 2)
int fourCCtoString(char *buf, size_t size, uint32_t fourCC);

/**
 * Convert a host-endian FourCC to a string.
 * @param fourCC	[in] FourCC
 * @return FourCC as a string on success; empty string on error.
 */
std::string fourCCtoString(uint32_t fourCC);

}
