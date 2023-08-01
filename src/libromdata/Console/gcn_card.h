/***************************************************************************
 * GameCube Memory Card Recovery Program.                                  *
 * card.h: Memory Card definitions.                                        *
 * Derived from libogc's card.c and card.h.                                *
 **************************************************************************/

/**
 * References:
 * - http://devkitpro.svn.sourceforge.net/viewvc/devkitpro/trunk/libogc/libogc/card.c?revision=4732&view=markup
 * - http://hitmen.c02.at/files/yagcd/yagcd/chap12.html
 */

/*-------------------------------------------------------------

card.c -- Memory card subsystem

Copyright (C) 2004
Michael Wiedenbauer (shagkur)
Dave Murphy (WinterMute)

This software is provided 'as-is', without any express or implied
warranty.  In no event will the authors be held liable for any
damages arising from the use of this software.

Permission is granted to anyone to use this software for any
purpose, including commercial applications, and to alter it and
redistribute it freely, subject to the following restrictions:

1.	The origin of this software must not be misrepresented; you
must not claim that you wrote the original software. If you use
this software in a product, an acknowledgment in the product
documentation would be appreciated but is not required.

2.	Altered source versions must be plainly marked as such, and
must not be misrepresented as being the original software.

3.	This notice may not be removed or altered from any source
distribution.

-------------------------------------------------------------*/

#pragma once

#include <stdint.h>
#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Memory card system locations.
 */
#define CARD_SYSAREA		5
#define CARD_SYSDIR		0x2000
#define CARD_SYSDIR_BACK	0x4000
#define CARD_SYSBAT		0x6000
#define CARD_SYSBAT_BACK	0x8000

#define CARD_FILENAMELEN	32	/* Filename length. */
#define CARD_MAXFILES		127	/* Maximum number of files. */

/**
 * Memory card header.
 * Reference for first 32 bytes: Dolphin
 * - Revision bef3d7229eca9a7f9568abf72de6b4d467feee9f
 * - File: Source/Core/Core/Src/HW/GCMemcard.h
 */
typedef struct _card_header {
	// The following is uint32_t serial[8] in libogc.
	// It's used as an 8-word key for F-Zero GX and PSO "encryption".
	union {
		uint32_t serial_full[8];
#pragma pack(1)
		struct PACKED {
			uint8_t serial[12];	// Serial number. (TODO: Should be 8...)
			uint64_t formatTime;	// Format time. (OSTime value; 1 tick == 1/40,500,000 sec)
			uint32_t sramBias;	// SRAM bias at time of format.
			uint32_t sramLang;	// SRAM language.
			uint8_t reserved1[4];	// usually 0
		};
#pragma pack()
	};

	uint16_t device_id;	// 0 if formatted in slot A; 1 if formatted in slot B
	uint16_t size;		// size of card, in Mbits
	uint16_t encoding;	// 0 == cp1252; 1 == Shift-JIS

	uint8_t padding[0x1D6];
	uint16_t chksum1;	// Checksum.
	uint16_t chksum2;	// Inverted checksum.
} card_header;
ASSERT_STRUCT(card_header, 512);

/**
 * Directory control block.
 */
typedef struct _card_dircntrl {
	uint8_t pad[58];
	uint16_t updated;	// Update counter.
	uint16_t chksum1;	// Checksum 1.
	uint16_t chksum2;	// Checksum 2.
} card_dircntrl;
ASSERT_STRUCT(card_dircntrl, 64);

/**
 * Directory entry.
 * Addresses are relative to the start of the file.
 */
typedef struct _card_direntry {
	union {
		struct {
			char gamecode[4];	// Game code.
			char company[2];	// Company code.
		};
		char id6[6];
	};
	uint8_t pad_00;		// Padding. (0xFF)
	uint8_t bannerfmt;	// Banner format.
	char filename[CARD_FILENAMELEN];	// Filename.
	uint32_t lastmodified;	// Last modified time. (seconds since 2000/01/01)
	uint32_t iconaddr;	// Icon address.
	uint16_t iconfmt;	// Icon format.
	uint16_t iconspeed;	// Icon speed.
	uint8_t permission;	// File permissions.
	uint8_t copytimes;	// Copy counter.
	uint16_t block;		// Starting block address.
	uint16_t length;	// File length, in blocks.
	uint16_t pad_01;	// Padding. (0xFFFF)
	uint32_t commentaddr;	// Comment address.
} card_direntry;
ASSERT_STRUCT(card_direntry, 64);

/**
 * Directory table.
 */
typedef struct _card_dat {
	struct _card_direntry entries[CARD_MAXFILES];
	struct _card_dircntrl dircntrl;
} card_dat;
ASSERT_STRUCT(card_dat, 8192);

/**
 * Block allocation table.
 */
typedef struct _card_bat {
	uint16_t chksum1;	// Checksum 1.
	uint16_t chksum2;	// Checksum 2.
	uint16_t updated;	// Update counter.
	uint16_t freeblocks;	// Number of free blocks.
	uint16_t lastalloc;	// Last block allocated.

	// NOTE: Subtract 5 from the block address
	// before looking it up in the FAT!
	uint16_t fat[0xFFB];	// File allocation table.
} card_bat;
ASSERT_STRUCT(card_bat, 8192);

// File attributes.
#define CARD_ATTRIB_PUBLIC	0x04
#define CARD_ATTRIB_NOCOPY	0x08
#define CARD_ATTRIB_NOMOVE	0x10
#define CARD_ATTRIB_GLOBAL	0x20

// Banner size.
#define CARD_BANNER_W		96
#define CARD_BANNER_H		32

// Banner format.
#define CARD_BANNER_NONE	0x00	/* No banner. */
#define CARD_BANNER_CI		0x01	/* CI8 (256-color) */
#define CARD_BANNER_RGB		0x02	/* RGB5A3 */
#define CARD_BANNER_MASK	0x03

// Icon size.
#define CARD_MAXICONS		8	/* Maximum 8 icons per file. */
#define CARD_ICON_W		32
#define CARD_ICON_H		32

// Icon format.
#define CARD_ICON_NONE		0x00	/* No icon. */
#define CARD_ICON_CI_SHARED	0x01	/* CI8 (256-color; shared palette) */
#define CARD_ICON_RGB		0x02	/* RGB5A3 */
#define CARD_ICON_CI_UNIQUE	0x03	/* CI8 (256-color; unique palette) */
#define CARD_ICON_MASK		0x03

// Icon animation style.
// (Stored in card_direntry.bannerfmt.)
#define CARD_ANIM_LOOP		0x00
#define CARD_ANIM_BOUNCE	0x04
#define CARD_ANIM_MASK		0x04

// Icon animation speed.
#define CARD_SPEED_END		0x00	/* No icon. (End of animation) */
#define CARD_SPEED_FAST		0x01	/* Icon lasts for 4 frames. */
#define CARD_SPEED_MIDDLE	0x02	/* Icon lasts for 8 frames. */
#define CARD_SPEED_SLOW		0x03	/* Icon lasts for 12 frames. */
#define CARD_SPEED_MASK		0x03

// System font encoding.
#define SYS_FONT_ENCODING_ANSI	0x00	/* ANSI text. (ISO-8859-1; possibly cp1252?) */
#define SYS_FONT_ENCODING_SJIS	0x01	/* Shift-JIS text. */
#define SYS_FONT_ENCODING_MASK	0x01

// Difference between GameCube timebase and Unix timebase.
// (GameCube starts at 2000/01/01; Unix starts at 1970/01/01.)
#define GC_UNIX_TIME_DIFF	0x386D4380

#ifdef __cplusplus
}
#endif
