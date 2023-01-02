/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * dmg_structs.h: Nintendo Virtual Boy data structures.                    *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * Copyright (c) 2016-2018 by Egor.                                        *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include <stdint.h>
#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Virtual Boy ROM footer.
 * This matches the ROM footer format exactly.
 * References:
 * - http://www.goliathindustries.com/vb/download/vbprog.pdf page 9
 * 
 * NOTE: Strings are NOT null-terminated!
 */
typedef struct _VB_RomFooter {
	char title[21];
	uint8_t reserved[4];
	char publisher[2];
	char gameid[4];
	uint8_t version;
} VB_RomFooter;
ASSERT_STRUCT(VB_RomFooter, 32);

#ifdef __cplusplus
}
#endif
