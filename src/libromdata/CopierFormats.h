/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * CopierFormats.h: Various ROM copier formats.                            *
 *                                                                         *
 * Copyright (c) 2016-2020 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_LIBROMDATA_COPIERFORMATS_H__
#define __ROMPROPERTIES_LIBROMDATA_COPIERFORMATS_H__

#include <stdint.h>
#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Common header format for Super Magic Drive,
 * Super Magicom / Super Wild Card, and Magic Griffin.
 *
 * References:
 * - http://cgfm2.emuviews.com/txt/smdtech.txt
 * - http://wiki.superfamicom.org/snes/show/Super+Wild+Card
 */
typedef struct SMD_Header {
	union {
		// Console-specific parameters.
		struct {
			uint8_t pages;		// Number of 16 KB pages.
			uint8_t file_data_type;	// File data type. (See SMD_FileDataType)
			uint8_t status_flags;	// Status flags.
		} smd;
		struct {
			uint16_t pages;		// Number of 8 KB pages. (LE16)
			uint8_t emulation_mode;	// Emulation mode. (See SMC_EmulationMode)
			uint8_t unused;		// Unused
		} smc;
		// TODO: Magic Griffin parameters.
	};
	uint8_t reserved1[4];		// Reserved. (Should be 0.)
	uint8_t id[2];			// Should be {0xAA 0xBB}.
	// NOTE: file_type is only valid if id is correct.
	uint8_t file_type;		// File type. (See SMD_FileType)
	uint8_t reserved2[501];		// Reserved. (Should be 0.)
} SMD_Header;
ASSERT_STRUCT(SMD_Header, 512);

/**
 * SMD file data types.
 * Reference: http://cgfm2.emuviews.com/txt/smdtech.txt
 */
typedef enum {
	SMD_FDT_SRAM_DATA = 0,		// 32 KB SRAM data
	SMD_FDT_Z80_PROGRAM = 1,	// Z80 program
	SMD_FDT_BIOS_PROGRAM = 2,	// BIOS program
	SMD_FDT_68K_PROGRAM = 3,	// 68K program (MD ROM image)
} SMD_FileDataType;

/**
 * SMD status flags. (bitfield)
 * Reference: http://cgfm2.emuviews.com/txt/smdtech.txt
 */
typedef enum {
	// If 1, this file is part of a multi-file set and
	// it isn't the last file in the set.
	SMD_SF_MULTI_FILE = (1U << 6),
} SMD_StatusFlags;

/**
 * SMC/SWC emulation mode. (bitfield)
 * Reference: http://wiki.superfamicom.org/snes/show/Super+Wild+Card
 */
typedef enum {
	// If 1, enable external cartridge memory image at
	// bank $20-$5F, $A0-$DF in System Mode 2, 3.
	SMC_EM_EXT_CART_MEMORY = (1U << 0),

	// 0 == run in Mode 3; 1 == run in Mode 2.
	SMC_EM_B1_MODE_3 = (0U << 1),
	SMC_EM_B1_MODE_2 = (1U << 1),

	// 00 = SRAM off; 01 == 16 KB; 10 == 64 KB; 11 == 256 KB
	SMC_EM_SRAM_OFF   = (0U << 2),
	SMC_EM_SRAM_16KB  = (1U << 2),
	SMC_EM_SRAM_64KB  = (2U << 2),
	SMC_EM_SRAM_256KB = (3U << 2),
	SMC_EM_SRAM_MASK  = (3U << 2),

	// 0 == Mode 20; 1 == Mode 21 (DRAM mapping)
	SMC_EM_MODE_20 = (0U << 4),
	SMC_EM_MODE_21 = (1U << 4),

	// 0 == Mode 1; 1 == Mode 2 (SRAM mapping)
	SMC_EM_B5_MODE_1 = (0U << 5),
	SMC_EM_B5_MODE_2 = (1U << 5),

	// If 1, this file is part of a multi-file set and
	// it isn't the last file in the set.
	SMC_EM_MULTI_FILE = (1U << 6),

	// 1 == Mode 0 (jump to $8000)
	SMC_EM_MODE_0 = (1U << 7),
} SMC_EmulationMode;

/**
 * SMD/SMC/SWC/MG file types.
 * This indicates the platform and if the file is a
 * ROM file or an SRAM file.
 */
typedef enum {
	SMD_FT_MG_GAME_FILE = 2,	// Magic Griffin game file. (PC Engine)
	SMD_FT_MG_SRAM_FILE = 3,	// Magic Griffin SRAM file.
	SMD_FT_SMC_GAME_FILE = 4,	// SMC/SWC game file. (Super NES)
	SMD_FT_SMC_SRAM_FILE = 5,	// SMC/SWC SRAM file.
	SMD_FT_SMD_GAME_FILE = 6,	// SMD game file. (Mega Drive)
	SMD_FT_SMD_SRAM_FILE = 7,	// SMD SRAM file.
} SMD_FileType;

#ifdef __cplusplus
}
#endif

#endif /* __ROMPROPERTIES_LIBROMDATA_COPIERFORMATS_H__ */
