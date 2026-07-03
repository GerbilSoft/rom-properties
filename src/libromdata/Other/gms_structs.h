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

// GameMaker general information header
// for ease of file parsing, the "count" value was removed
// for a HACK to make GCC happy, MajorVersion etc was turned into the YYVersion struct
#define GEN7_HDR 'GEN7'
#define GEN8_HDR 'GEN8' // only one actually used by the runner
#define GENL_HDR 'GENL'
typedef struct RP_PACKED _YYHeader {
    int debug;
    uint32_t pName;
    uint32_t pConfig;
    int roomMaxId;
    int roomMaxTileId;
    int id;
    int buildNumber;
    int revisionNumber;
    int guid3;
    int guid4;
    uint32_t pGameName;
    YYVersion Version;
    int xscreensize;
    int yscreensize;
    int screenflags;
    int crc;
    uint8_t md5[0x10];
    int64_t datetimeUTC;
} YYHeader;
ASSERT_STRUCT(YYHeader, 0x64);

// Version v10
typedef struct RP_PACKED _YYHeader_000A {
    int debug;
    uint32_t pName;
    uint32_t pConfig;
    int roomMaxId;
    int roomMaxTileId;
    int id;
    int buildNumber;
    int revisionNumber;
    int guid3;
    int guid4;
    uint32_t pGameName;
    YYVersion Version;
    int xscreensize;
    int yscreensize;
    int screenflags;
    int crc;
    uint8_t md5[0x10];
    int64_t datetimeUTC;
    // 0xA
    uint32_t pDisplayName;
} YYHeader_000A;
ASSERT_STRUCT(YYHeader_000A, 0x68);

// Version v11
typedef struct RP_PACKED _YYHeader_000B {
    int debug;
    uint32_t pName;
    uint32_t pConfig;
    int roomMaxId;
    int roomMaxTileId;
    int id;
    int buildNumber;
    int revisionNumber;
    int guid3;
    int guid4;
    uint32_t pGameName;
    YYVersion Version;
    int xscreensize;
    int yscreensize;
    int screenflags;
    int crc;
    uint8_t md5[0x10];
    int64_t datetimeUTC;
    // 0xA
    uint32_t pDisplayName;
    // 0xB
    int64_t Licensed;
} YYHeader_000B;
ASSERT_STRUCT(YYHeader_000B, 0x70);

// Version v12
typedef struct RP_PACKED _YYHeader_000C {
    int debug;
    uint32_t pName;
    uint32_t pConfig;
    int roomMaxId;
    int roomMaxTileId;
    int id;
    int buildNumber;
    int revisionNumber;
    int guid3;
    int guid4;
    uint32_t pGameName;
    YYVersion Version;
    int xscreensize;
    int yscreensize;
    int screenflags;
    int crc;
    uint8_t md5[0x10];
    int64_t datetimeUTC;
    // 0xA
    uint32_t pDisplayName;
    // 0xB
    int64_t Licensed;
    // 0xC
    int64_t functionClasses;
} YYHeader_000C;
ASSERT_STRUCT(YYHeader_000C, 0x78);

// Version v13
typedef struct RP_PACKED _YYHeader_000D {
    int debug;
    uint32_t pName;
    uint32_t pConfig;
    int roomMaxId;
    int roomMaxTileId;
    int id;
    int buildNumber;
    int revisionNumber;
    int guid3;
    int guid4;
    uint32_t pGameName;
    YYVersion Version;
    int xscreensize;
    int yscreensize;
    int screenflags;
    int crc;
    uint8_t md5[0x10];
    int64_t datetimeUTC;
    // 0xA
    uint32_t pDisplayName;
    // 0xB
    int64_t Licensed;
    // 0xC
    int64_t functionClasses;
    // 0xD
    int steamAppId;
} YYHeader_000D;
ASSERT_STRUCT(YYHeader_000D, 0x7C);

// Version v14+
typedef struct RP_PACKED _YYHeader_000E {
    int debug;
    uint32_t pName;
    uint32_t pConfig;
    int roomMaxId;
    int roomMaxTileId;
    int id;
    int buildNumber;
    int revisionNumber;
    int guid3;
    int guid4;
    uint32_t pGameName;
    YYVersion Version;
    int xscreensize;
    int yscreensize;
    int screenflags;
    int crc;
    uint8_t md5[0x10];
    int64_t datetimeUTC;
    // 0xA
    uint32_t pDisplayName;
    // 0xB
    int64_t Licensed;
    // 0xC
    int64_t functionClasses;
    // 0xD
    int steamAppId;
    // 0xE
    int debuggerServerPort;
} YYHeader_000E;
ASSERT_STRUCT(YYHeader_000E, 0x80);

#pragma pack()

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
