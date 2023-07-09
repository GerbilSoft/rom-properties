/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * exe_ne_structs.h: DOS/Windows executable structures. (NE)               *
 *                                                                         *
 * Copyright (c) 2017-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include <stdint.h>
#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

/** New Executable (Win16) structs **/
// References:
// - http://wiki.osdev.org/NE
// - http://www.fileformat.info/format/exe/corion-ne.htm

typedef struct _NE_Header {
	// 0x00
	uint16_t sig;			// "NE" (0x4E45)
	uint8_t MajLinkerVersion;	// The major linker version
	uint8_t MinLinkerVersion;	// The minor linker version
	uint16_t EntryTableOffset;	// Offset of entry table, see below
	uint16_t EntryTableLength;	// Length of entry table in bytes
	uint32_t FileLoadCRC;		// 32-bit CRC of entire contents of file
	uint8_t ProgFlags;		// Program flags, bitmapped
	uint8_t ApplFlags;		// Application flags, bitmapped
	uint8_t AutoDataSegIndex;	// The automatic data segment index
	uint8_t reserved;
	// 0x10
	uint16_t InitHeapSize;		// The intial local heap size
	uint16_t InitStackSize;		// The inital stack size
	uint32_t EntryPoint;		// CS:IP entry point, CS is index into segment table
	uint32_t InitStack;		// SS:SP inital stack pointer, SS is index into segment table
	uint16_t SegCount;		// Number of segments in segment table
	uint16_t ModRefs;		// Number of module references (DLLs)
	// 0x20
	uint16_t NoResNamesTabSiz;	// Size of non-resident names table, in bytes (Please clarify non-resident names table)
	uint16_t SegTableOffset;	// Offset of Segment table
	uint16_t ResTableOffset;	// Offset of resources table
	uint16_t ResidNamTable;		// Offset of resident names table
	uint16_t ModRefTable;		// Offset of module reference table
					// (points to entries in ImportNameTable)
	uint16_t ImportNameTable;	// Offset of imported names table (array of counted strings)
	uint32_t OffStartNonResTab;	// Offset from start of file to non-resident names table
	// 0x30
	uint16_t MovEntryCount;		// Count of moveable entry point listed in entry table
	uint16_t FileAlnSzShftCnt;	// File alignment size shift count (0=9(default 512 byte pages))
	uint16_t nResTabEntries;	// Number of resource table entries
	uint8_t targOS;			// Target OS
	uint8_t OS2EXEFlags;		// Other OS/2 flags
	uint16_t retThunkOffset;	// Offset to return thunks or start of gangload area - what is gangload?
	uint16_t segrefthunksoff;	// Offset to segment reference thunks or size of gangload area
	uint16_t mincodeswap;		// Minimum code swap area size
	uint8_t expctwinver[2];		// Expected windows version (minor first)
} NE_Header;
ASSERT_STRUCT(NE_Header, 64);

// Program flags (ProgFlags)

// DGroup type (bits 0-1)
typedef enum {
	NE_DGT_NONE = 0,	// None
	NE_DGT_SINSHARED,	// Single shared
	NE_DGT_MULTIPLE,	// Multiple
	NE_DGT_NULL,		// (null)
} NE_DGroupType;

typedef enum {
	NE_GLOBINIT	= (1U << 2),	// Global initialization
	NE_PMODEONLY	= (1U << 3),	// Protected mode only
	NE_INSTRUC86	= (1U << 4),	// 8086 instructions
	NE_INSTRU286	= (1U << 5),	// 80286 instructions
	NE_INSTRU386	= (1U << 6),	// 80386 instructions
	NE_INSTRUx87	= (1U << 7),	// 80x87 (FPU) instructions
} NE_ProgFlags;

// Application flags (ApplFlags)

// Application type (bits 0-1)
typedef enum {
	NE_APP_NONE = 0,
	NE_APP_FULLSCREEN,	// Fullscreen (not aware of Windows/P.M. API)
	NE_APP_WINPMCOMPAT,	// Compatible with Windows/P.M. API
	NE_APP_WINPMUSES,	// Uses Windows/P.M. API
} NE_AppType;

typedef enum {
	NE_OS2APP	= (1U << 3),	// OS/2 family application
	// bit 4 reserved?
	NE_IMAGEERROR	= (1U << 5),	// Errors in image/executable
	NE_ONCONFORM	= (1U << 6),	// Non-conforming program?
	NE_DLL		= (1U << 7),	// DLL or driver (SS:SP invalid, CS:IP -> Far INIT routine)
					// AX=HMODULE, returns AX==0 success, AX!=0 fail
} NE_ApplFlags;

// Target OS (targOS)
// Used for NE and LE.
// NOTE: Phar Lap is NE only.
typedef enum {
	NE_OS_UNKNOWN = 0,
	NE_OS_OS2,		// IBM OS/2
	NE_OS_WIN,		// Windows (16-bit)
	NE_OS_DOS4,		// European DOS 4.x
	NE_OS_WIN386,		// Windows for the 80386. (Win32s?) 32-bit code.
	NE_OS_BOSS,		// Borland Operating System Services

	NE_OS_PHARLAP_286_OS2 = 0x81,	// Phar Lap 286|DOS Extender, OS/2
	NE_OS_PHARLAP_286_WIN = 0x82,	// Phar Lap 286|DOS Extender, Windows
} NE_TargetOS;

// Other OS/2 flags.
typedef enum {
	NE_OS2_LFN	= (1U << 0),	// OS/2 Long File Names
	NE_OS2_PMODE	= (1U << 1),	// OS/2 2.x Protected Mode executable
	NE_OS2_PFONT	= (1U << 2),	// OS/2 2.x Proportional Fonts
	NE_OS2_GANGL	= (1U << 3),	// OS/2 Gangload area
} NE_OS2EXEFlags;

// 16-bit resource structs.

typedef struct _NE_NAMEINFO {
	uint16_t rnOffset;
	uint16_t rnLength;
	uint16_t rnFlags;
	uint16_t rnID;
	uint16_t rnHandle;
	uint16_t rnUsage;
} NE_NAMEINFO;
ASSERT_STRUCT(NE_NAMEINFO, 6*sizeof(uint16_t));

typedef struct _NE_TYPEINFO {
	uint16_t rtTypeID;
	uint16_t rtResourceCount;
	uint32_t rtReserved;
	// followed by NE_NAMEINFO[]
} NE_TYPEINFO;
ASSERT_STRUCT(NE_TYPEINFO, 8);

typedef struct _NE_Segment {
	uint16_t offset;	// in sectors as defined by FileAlnSzShftCnt
	uint16_t filesz;	// 0 = 64K
	uint16_t flags;
	uint16_t memsz;		// 0 = 64K
} NE_Segment;
ASSERT_STRUCT(NE_Segment, 8);

typedef enum {
	NE_SEG_DATA		= 0x0001,
	NE_SEG_ALLOCATED	= 0x0002,
	NE_SEG_LOADED		= 0x0004,
	NE_SEG_MOVABLE		= 0x0010,
	NE_SEG_SHAREABLE	= 0x0020,
	NE_SEG_PRELOAD		= 0x0040,
	NE_SEG_EXECUTEONLY	= 0x0080,
	NE_SEG_READONLY		= 0x0080,
	NE_SEG_RELOCINFO	= 0x0100,
	NE_SEG_DISCARD		= 0x1000,
} NE_SegFlags;

typedef struct _NE_Reloc {
	uint8_t source_type;
	uint8_t flags;
	uint16_t offset;
	uint16_t target1;
	uint16_t target2;
} NE_Reloc;
ASSERT_STRUCT(NE_Reloc, 8);

typedef enum {
	NE_REL_TARGET_MASK	= 0x03,
	NE_REL_INTERNALREF	= 0x00,
	NE_REL_IMPORTORDINAL	= 0x01,
	NE_REL_IMPORTNAME	= 0x02,
	NE_REL_OSFIXUP		= 0x03,
	NE_REL_ADDITIVE		= 0x04,
} NE_RelocFlags;

#ifdef __cplusplus
}
#endif
