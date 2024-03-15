/***************************************************************************
 * ROM Properties Page shell extension. (librptexture)                     *
 * argb32_t.hpp: ARGB32 value with byte accessors.                         *
 *                                                                         *
 * Copyright (c) 2016-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "common.h"
#include "librpbyteswap/byteorder.h"

// C includes (C++ namespace)
#include <cassert>

namespace LibRpTexture {

// ARGB32 value with byte accessors.
union argb32_t {
	struct {
#if SYS_BYTEORDER == SYS_LIL_ENDIAN
		uint8_t b;
		uint8_t g;
		uint8_t r;
		uint8_t a;
#else /* SYS_BYTEORDER == SYS_BIG_ENDIAN */
		uint8_t a;
		uint8_t r;
		uint8_t g;
		uint8_t b;
#endif
	};

	// for YCoCg swizzling
	struct {
#if SYS_BYTEORDER == SYS_LIL_ENDIAN
		uint8_t a;
		uint8_t cg;
		uint8_t co;
		uint8_t y;
#else /* SYS_BYTEORDER == SYS_BIG_ENDIAN */
		uint8_t y;
		uint8_t co;
		uint8_t cg;
		uint8_t a;
#endif
	} YCoCg;

	uint32_t u32;
};
ASSERT_STRUCT(argb32_t, 4);

// Byte offsets for certain functions.
#if SYS_BYTEORDER == SYS_LIL_ENDIAN
#  define ARGB32_BYTE_OFFSET_B 0
#  define ARGB32_BYTE_OFFSET_G 1
#  define ARGB32_BYTE_OFFSET_R 2
#  define ARGB32_BYTE_OFFSET_A 3
#else /* SYS_BYTEORDER == SYS_BIG_ENDIAN */
#  define ARGB32_BYTE_OFFSET_A 0
#  define ARGB32_BYTE_OFFSET_R 1
#  define ARGB32_BYTE_OFFSET_G 2
#  define ARGB32_BYTE_OFFSET_B 3
#endif

}
