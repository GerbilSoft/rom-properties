/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * exe_mz_structs.h: DOS/Windows executable structures. (MZ)               *
 *                                                                         *
 * Copyright (c) 2017-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include <stdint.h>
#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef IMAGE_DOS_SIGNATURE

#define IMAGE_DOS_SIGNATURE 0x5A4D
#define IMAGE_OS2_SIGNATURE 0x454E
#define IMAGE_OS2_SIGNATURE_LE 0x454C
#define IMAGE_VXD_SIGNATURE 0x454C

/**
 * MS-DOS "MZ" header.
 * All other EXE-based headers have this one first.
 *
 * All fields are little-endian.
 */
typedef struct _IMAGE_DOS_HEADER {
	uint16_t e_magic;	// "MZ"
	uint16_t e_cblp;
	uint16_t e_cp;
	uint16_t e_crlc;
	uint16_t e_cparhdr;
	uint16_t e_minalloc;
	uint16_t e_maxalloc;
	uint16_t e_ss;
	uint16_t e_sp;
	uint16_t e_csum;
	uint16_t e_ip;
	uint16_t e_cs;
	uint16_t e_lfarlc;
	uint16_t e_ovno;
	uint16_t e_res[4];
	uint16_t e_oemid;
	uint16_t e_oeminfo;
	uint16_t e_res2[10];
	uint32_t e_lfanew;	// Pointer to NE/LE/LX/PE headers.
} IMAGE_DOS_HEADER;
ASSERT_STRUCT(IMAGE_DOS_HEADER, 64);

#else /* IMAGE_DOS_SIGNATURE */

// Windows headers are already included, and the various structs are defined.
// Don't re-define the structs, but ensure they have the correct sizes.
ASSERT_STRUCT(IMAGE_DOS_HEADER, 64);

#endif /* IMAGE_DOS_SIGNATURE */

#ifdef __cplusplus
}
#endif
