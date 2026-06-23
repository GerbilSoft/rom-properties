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

#pragma pack(1)

// IFF section header
typedef struct _iff_sect_hdr {
    uint32_t magic;
    uint32_t length;
} iff_sect_hdr_t;
// "FORM" magic
#define FORM_HDR 0x4D524F46

// GameMaker general information header
// for ease of file parsing, the "count" value was removed
#define GEN7_HDR 0x374E4547
#define GEN8_HDR 0x384E4547 // only one actually used by the runner
#define GENL_HDR 0x4C4E4547
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
    int MajorVersion;
    int MinorVersion;
    int ReleaseVersion;
    int BuildVersion;
    int xscreensize;
    int yscreensize;
    int screenflags;
    int crc;
    uint8_t md5[0x10];
    int64_t datetimeUTC;
} YYHeader;

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
    int MajorVersion;
    int MinorVersion;
    int ReleaseVersion;
    int BuildVersion;
    int xscreensize;
    int yscreensize;
    int screenflags;
    int crc;
    uint8_t md5[0x10];
    int64_t datetimeUTC;
    // 0xA
    uint32_t pDisplayName;
} YYHeader_000A;

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
    int MajorVersion;
    int MinorVersion;
    int ReleaseVersion;
    int BuildVersion;
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
    int MajorVersion;
    int MinorVersion;
    int ReleaseVersion;
    int BuildVersion;
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
    int MajorVersion;
    int MinorVersion;
    int ReleaseVersion;
    int BuildVersion;
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
    int MajorVersion;
    int MinorVersion;
    int ReleaseVersion;
    int BuildVersion;
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

// Footer of the YYHeader for GMS2 titles
typedef struct RP_PACKED _YYGMS2HeaderData {
    float GameSpeed;
    int AllowStatistics;
    uint8_t GameGUID[0x10];
} YYGMS2HeaderData;

#pragma pack()

#ifdef __cplusplus
}
#endif
