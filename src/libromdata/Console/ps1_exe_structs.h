/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * ps1_exe_structs.h: Sony PlayStation PS-X executable data structures.    *
 *                                                                         *
 * Copyright (c) 2016-2020 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

// References:
// - https://problemkaputt.de/psx-spx.htm#cdromfileformats

#ifndef __ROMPROPERTIES_LIBROMDATA_PS1_EXE_STRUCTS_H__
#define __ROMPROPERTIES_LIBROMDATA_PS1_EXE_STRUCTS_H__

#include "common.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * "SC" magic struct.
 * Found at 0x84 in PSV save files.
 *
 * All fields are little-endian.
 */
#define PS1_EXE_MAGIC "PS-X EXE"
typedef struct _PS1_EXE_Header {
	char magic[8];			// [0x000] Magic ("PS-X EXE")
	uint8_t padding1[8];		// [0x008] Zero-filled
	uint32_t initial_pc;		// [0x010] Initial PC
	uint32_t initial_gp;		// [0x014] Initial GP/R28
	uint32_t ram_addr;		// [0x018] Destination RAM address
	uint32_t filesize;		// [0x01C] Filesize, minus header. (Must be a multiple of 0x800)
	uint32_t unknown1[2];		// [0x020]

	uint32_t memfill_start;		// [0x028] Memfill start address (0=None)
	uint32_t memfill_size;		// [0x02C] Memfill size (in bytes) (0=None)
	uint32_t initial_sp;		// [0x030] Initial SP/R29 and FP/R29 base
	uint32_t initial_sp_off;	// [0x034] Initial SP offset
	uint8_t reserved_43h[20];	// [0x038] Reserved for A(43h) function

	char region_id[128];		// [0x04C] Region ID (ignored by BIOS)

	uint8_t padding2[0x734];	// [0x0CC] Zero-filled.
} PS1_EXE_Header;
ASSERT_STRUCT(PS1_EXE_Header, 2048);

#ifdef __cplusplus
}
#endif

#endif /* __ROMPROPERTIES_LIBROMDATA_PS1_EXE_STRUCTS_H__ */
