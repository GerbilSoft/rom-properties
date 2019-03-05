/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * xbox360_common_structs.h: Microsoft Xbox 360 common data structures.    *
 *                                                                         *
 * Copyright (c) 2019 by David Korth.                                      *
 *                                                                         *
 * This program is free software; you can redistribute it and/or modify it *
 * under the terms of the GNU General Public License as published by the   *
 * Free Software Foundation; either version 2 of the License, or (at your  *
 * option) any later version.                                              *
 *                                                                         *
 * This program is distributed in the hope that it will be useful, but     *
 * WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 * GNU General Public License for more details.                            *
 *                                                                         *
 * You should have received a copy of the GNU General Public License       *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.   *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_LIBROMDATA_CONSOLE_XBOX360_COMMON_STRUCTS_H__
#define __ROMPROPERTIES_LIBROMDATA_CONSOLE_XBOX360_COMMON_STRUCTS_H__

#include "librpbase/common.h"
#include "librpbase/byteorder.h"
#include <stdint.h>

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
 * Contains two characters and a 16-bit number.
 * NOTE: Struct positioning only works with the original BE32 value.
 */
typedef union PACKED _XEX2_Title_ID {
	struct {
		char a;
		char b;
		uint16_t u16;
	};
	uint32_t u32;
} Xbox360_Title_ID;
ASSERT_STRUCT(Xbox360_Title_ID, sizeof(uint32_t));

#endif /* __ROMPROPERTIES_LIBROMDATA_CONSOLE_XBOX360_COMMON_STRUCTS_H__ */
