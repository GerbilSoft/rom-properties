/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * gms_structs.h: GameMaker IFF/"data.win" data structures                 *
 *                                                                         *
 * Copyright (c) 2016-2026 by David Korth.                                 *
 * Copyright (c) 2026 by Emma / InvoxiPlayGames.                           *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include <stdint.h>
#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

// IFF section header
typedef struct _iff_sect_hdr {
	uint32_t magic;
	uint32_t length;
} iff_sect_hdr_t;
// IFF "FORM" magic
#define FORM_HDR 'FORM'

typedef struct _YYVersion {
	int Major;
	int Minor;
	int Release;
	int Build;
} YYVersion;

#pragma pack(1)

/**
 * GameMaker general information header
 * For ease of file parsing, the "count" value was removed.
 * For a HACK to make GCC happy, MajorVersion etc was turned into the YYVersion struct
 *
 * Pointers are absolute offsets into the file.
 *
 * Endianness depends on the overall file endianness,
 * which is determined by the "FORM" magic.
 */
#define GEN7_HDR 'GEN7'
#define GEN8_HDR 'GEN8' // only one actually used by the runner
#define GENL_HDR 'GENL'
typedef struct RP_PACKED _YYHeader {
	// Common fields for all versions
	int infoHeader;			// [0x000] Bits 8-15 contain the file format version
	uint32_t pName;			// [0x004] Pointer to project name
	uint32_t pConfig;		// [0x008] Pointer to project config
	int roomMaxId;			// [0x00C]
	int roomMaxTileId;		// [0x010]
	int id;				// [0x014]
	int buildNumber;		// [0x018]
	int revisionNumber;		// [0x01C]
	int guid3;			// [0x020]
	int guid4;			// [0x024]
	uint32_t pGameName;		// [0x028] Pointer to game name
	YYVersion Version;		// [0x02C] GameMaker IDE version
	int xscreensize;		// [0x03C] Screen size (width)
	int yscreensize;		// [0x040] Screen size (height)
	int screenflags;		// [0x044] Screen flags (see YYHeader_Screen_Flags_e)
	int crc;			// [0x048] CRC32 (FIXME: Should be unsigned?)
	uint8_t md5[0x10];		// [0x04C] MD5
	int64_t datetimeUTC;		// [0x05C] Build time (Unix timestamp)

	// v10 [0x000A] (starts at 0x064)
	struct {
		uint32_t pDisplayName;		// [0x064] Pointer to display name
	} v10;

	// v11 [0x000B] (starts at 0x068)
	struct {
		int64_t Licensed;		// [0x068]
	} v11;

	// v12 [0x000C] (starts at 0x070)
	struct {
		int64_t functionClasses;	// [0x070] Function classes used by this game
	} v12;

	// v13 [0x000D] (starts at 0x078)
	struct {
		int steamAppId;			// [0x078] Steam app ID
	} v13;

	// v14 [0x000E] (starts at 0x07C)
	struct {
		int debuggerServerPort;		// [0x07C]
	} v14;
} YYHeader;
ASSERT_STRUCT(YYHeader, 0x64+0x04+0x08+0x08+0x04+0x04);

#pragma pack()

/**
 * Screen flags
 */
typedef enum {
	YYHEADER_SCREEN_FLAG_FULLSCREEN		= (1U <<  0),
	YYHEADER_SCREEN_FLAG_VSYNC		= (1U <<  1),
	YYHEADER_SCREEN_FLAG_SW_VERTEXES	= (1U <<  2),
	YYHEADER_SCREEN_FLAG_ANTI_ALIASING	= (1U <<  3),
	YYHEADER_SCREEN_FLAG_KEEP_ASPECT_RATIO	= (1U <<  4),
	YYHEADER_SCREEN_FLAG_SHOW_CURSOR	= (1U <<  5),
	YYHEADER_SCREEN_FLAG_RESIZABLE		= (1U <<  6),
	YYHEADER_SCREEN_FLAG_ALLOW_FS_SWITCH	= (1U <<  7),
	YYHEADER_SCREEN_FLAG_ALLOW_DOCK_IN_FS	= (1U <<  8),
	YYHEADER_SCREEN_FLAG_FREE_IDE		= (1U <<  9),
	YYHEADER_SCREEN_FLAG_STANDARD_IDE	= (1U << 10),
	YYHEADER_SCREEN_FLAG_PRO_IDE		= (1U << 11),
	YYHEADER_SCREEN_FLAG_STEAM_WORKSHOP	= (1U << 12),
	YYHEADER_SCREEN_FLAG_ROAMING_APPDATA	= (1U << 13),
	YYHEADER_SCREEN_FLAG_BORDERLESS_FS	= (1U << 14),
	YYHEADER_SCREEN_FLAG_JS_ENABLED		= (1U << 15),
	// TODO: change these to match the specific IDE version as it can change
	YYHEADER_SCREEN_FLAG_IS_IDE_BUILD	= (1U << 16),	// prev "Show Hobby Splash"
	YYHEADER_SCREEN_FLAG_TRANSPARNET_BG	= (1U << 17),	// prev "Is IDE Build"
	YYHEADER_SCREEN_FLAG_D3D_SWAP_DISCARD	= (1U << 18),
} YYHeader_Screen_Flags_e;

// Struct sizes
#define YYHEADER_SIZE_COMMON (0x64)
#define YYHEADER_SIZE_V10 (0x64+0x04)
#define YYHEADER_SIZE_V11 (0x64+0x04+0x08)
#define YYHEADER_SIZE_V12 (0x64+0x04+0x08+0x08)
#define YYHEADER_SIZE_V13 (0x64+0x04+0x08+0x08+0x04)
#define YYHEADER_SIZE_V14 (0x64+0x04+0x08+0x08+0x04+0x04)

// Footer of the YYHeader for GMS2 titles
typedef struct RP_PACKED _YYGMS2HeaderData {
	float GameSpeed;
	int AllowStatistics;
	uint8_t GameGUID[0x10];
} YYGMS2HeaderData;
ASSERT_STRUCT(YYGMS2HeaderData, 0x18);

#define CODE_HDR 'CODE'

#ifdef __cplusplus
}
#endif
