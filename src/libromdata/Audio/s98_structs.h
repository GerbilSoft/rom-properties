/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * s98_structs.h: S98 audio data structures.                               *
 *                                                                         *
 * Copyright (c) 2018-2026 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

// References:
// - https://vgmrips.net/wiki/S98_File_Format
// - https://www.purose.net/befis/download/kmp/old/s98spec2.txt
// - https://vgmrips.net/mirror/s98spec3.txt

#pragma once

#include "common.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * S98 sound log format.
 * All fields are little-endian.
 */
#define S98_MAGIC "S98"
#define S98_TAG_MAGIC "[S98]"
#define S98_TIMER_INFO_DEFAULT 10U
#define S98_TIMER_INFO2_DEFAULT 1000U
typedef struct _S98_Header {
	char magic[3];			// [0x000] "S98"
	char version;			// [0x003] Version (as an ASCII digit). Latest is 3; 0, 1, and 2 also exist.
	uint32_t timer_info;		// [0x004] Timer info, numerator. (If 0, default is 10.)
	uint32_t timer_info2;		// [0x008] Timer info, denominator. (If 0, default is 1000.)
	uint32_t compressing;		// [0x00C] v3: always 0; earlier versions, 'inflate' size in bytes if not 0.
	uint32_t tag_offset;		// [0x010] If non-zero, offset to tag structure (v3) or song name (v2).
	uint32_t dump_data_offset;	// [0x014] Offset to dump data
	uint32_t loop_point_offset;	// [0x018] Offset to loop point if present; otherwise, 0.

	union {
		uint32_t v2_compressed_data_offset;	// [0x01C] v2: Offset to compressed data. (If 0, use dump_data_offset.)
		uint32_t v3_device_count;		// [0x01C] v3: Device count. (If 0, assume one OPNA with 7,987,200 Hz.)
	};

	// For v2 and v3, a list of device info data follows.
	// v2 uses an implicit count; it ends when device type 0 (none) is found.
	// v3 has an explicit count; if 0, assume one OPNA with 7,987,200 Hz.
} S98_Header;
ASSERT_STRUCT(S98_Header, 0x20);

/**
 * S98: Device info struct.
 */
typedef struct _S98_Device_Info_t {
	uint32_t type;		// [0x000] Device type (see S98_Device_Type_e)
	uint32_t clock;		// [0x004] Clock rate, in Hz
	uint32_t v3_pan;	// [0x008] v3 only: Panning (for mono devices only; device-specific interpretation)
	uint32_t v2_data;	// [0x00C] v2 only: Offset to application-dependent data
} S98_Device_Info_t;
ASSERT_STRUCT(S98_Device_Info_t, 16);

/**
 * S98: Device type
 */
typedef enum {
	// All versions
	S98_DEVICE_TYPE_NONE		= 0,
	S98_DEVICE_TYPE_PSG_YM2149	= 1,
	S98_DEVICE_TYPE_OPN_YM2203	= 2,
	S98_DEVICE_TYPE_OPN2_YM2612	= 3,
	S98_DEVICE_TYPE_OPNA_YM2608	= 4,
	S98_DEVICE_TYPE_OPM_YM2151	= 5,

	// v3 only
	S98_DEVICE_TYPE_OPLL_YM2413	= 6,
	S98_DEVICE_TYPE_OPL_YM3526	= 7,
	S98_DEVICE_TYPE_OPL2_YM3812	= 8,
	S98_DEVICE_TYPE_OPL3_YMF262	= 9,
	S98_DEVICE_TYPE_PSG_AY_3_8910	= 15,
	S98_DEVICE_TYPE_DCSG_SN76489	= 16,
} S98_Device_Type_e;

#ifdef __cplusplus
}
#endif
