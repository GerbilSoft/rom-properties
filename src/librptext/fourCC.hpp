/***************************************************************************
 * ROM Properties Page shell extension. (librptext)                        *
 * fourCC.hpp: FourCC string conversion functions                          *
 *                                                                         *
 * Copyright (c) 2009-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "common.h"	// for ATTR_PRINTF()
#include "dll-macros.h"	// for RP_LIBROMDATA_PUBLIC

// C includes
#include <stdint.h>

namespace LibRpText {

/**
 * Convert a host-endian FourCC to a string.
 * @param buf		[out] Output buffer
 * @param size		[in] Size of buf (should be >= 5)
 * @param fourCC	[in] FourCC
 * @return 0 on success; negative POSIX error code on error.
 */
int fourCCtoString(char *buf, size_t size, uint32_t fourCC);

}
