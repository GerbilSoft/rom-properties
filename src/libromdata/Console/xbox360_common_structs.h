/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * xbox360_common_structs.h: Microsoft Xbox 360 common data structures.    *
 *                                                                         *
 * Copyright (c) 2019-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include <stdint.h>

#include "common.h"
#include "librpbyteswap/byteorder.h"

/**
 * Xbox 360: Vesion number
 * All fields are in big-endian.
 * NOTE: Bitfields are in HOST-endian. u32 value
 * must be converted before using the bitfields.
 * TODO: Verify behavior on various platforms!
 * TODO: Static assertions?
 */
typedef union _Xbox360_Version_t {
	uint32_t u32;
	struct {
#if SYS_BYTEORDER == SYS_BIG_ENDIAN
		uint32_t major : 4;
		uint32_t minor : 4;
		uint32_t build : 16;
		uint32_t qfe : 8;
#else /* SYS_BYTEORDER == SYS_LIL_ENDIAN */
		uint32_t qfe : 8;
		uint32_t build : 16;
		uint32_t minor : 4;
		uint32_t major : 4;
#endif
	};
} Xbox360_Version_t;
ASSERT_STRUCT(Xbox360_Version_t, sizeof(uint32_t));

/**
 * Xbox 360: Title ID
 * Contains a two-character company ID and a 16-bit game ID.
 * NOTE: Struct positioning only works with the original BE32 value.
 */
typedef union _XEX2_Title_ID {
	struct {
		char a;
		char b;
		uint16_t u16;
	};
	uint32_t u32;
} Xbox360_Title_ID;
ASSERT_STRUCT(Xbox360_Title_ID, sizeof(uint32_t));
