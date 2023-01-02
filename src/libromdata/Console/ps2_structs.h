/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * ps2_structs.h: Sony PlayStation 2 data structures.                      *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "common.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Sony PlayStation 2: CDVDGEN struct.
 * This is commonly found on PS2 prototype discs.
 *
 * Encoding is assumed to be cp1252.
 *
 * NOTE: Strings are NOT null-terminated!
 * Most string fields are space-padded.
 */
// Stored at LBA 14 (with duplicate at LBA 15).
#define PS2_CDVDGEN_LBA 14
typedef struct _PS2_CDVDGEN_t {
	char disc_name[32];		// [0x000] Disc name (usually the product code)
	char producer_name[32];		// [0x020] Producer name
	char copyright_holder[32];	// [0x040] Copyright holder
	char creation_date[8];		// [0x060] Creation date (format: 20200624)
	char master_disc_id[25];	// [0x068] "PlayStation Master Disc 2"
	uint8_t unknown1[31];		// [0x081] Unknown data, space-padded
	char padding1[96];		// [0x0A0] Space padding

	uint8_t unknown2[64];		// [0x100] Unknown data
	uint8_t padding2[192];		// [0x140] NULL padding

	uint8_t padding3[256];		// [0x200] NULL padding

	struct {
		char vendor[8];		// [0x300] Disc drive vendor
		char model[16];		// [0x308] Disc drive model
		char revision[4];	// [0x318] Disc drive firmware version

		// Notes field is different depending on the drive model.
		// - DSR-8000dp: Contains "DVD-R   DVR-S2012.14"
		// - DVR-S201: Contains "2000/07/14"
		char notes[20];
	} drive;

	char sw_version[32];		// [0x330] CDVDGEN version.
	uint8_t padding4[176];		// [0x340] Space padding.
} PS2_CDVDGEN_t;
ASSERT_STRUCT(PS2_CDVDGEN_t, 0x400);

#ifdef __cplusplus
}
#endif
