/***************************************************************************
 * ROM Properties Page shell extension. (librptexture)                     *
 * ico_structs.h: Windows icon and cursor format data structures.          *
 *                                                                         *
 * Copyright (c) 2019-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include <stdint.h>
#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * References:
 * - http://justsolve.archiveteam.org/wiki/Windows_1.0_Icon
 * - http://justsolve.archiveteam.org/wiki/Windows_1.0_Cursor
 * - http://justsolve.archiveteam.org/wiki/ICO
 * - http://justsolve.archiveteam.org/wiki/CUR
 * - https://devblogs.microsoft.com/oldnewthing/20101018-00/?p=12513
 */

/**
 * Windows 1.0: Icon (and cursor)
 *
 * All fields are in little-endian.
 */
typedef struct _ICO_Win1_Header {
	uint16_t format;	// [0x000] See ICO_Win1_Format_e
	uint16_t hotX;		// [0x002] Cursor hotspot X (cursors only)
	uint16_t hotY;		// [0x004] Cursor hotspot Y (cursors only)
	uint16_t width;		// [0x006] Width, in pixels
	uint16_t height;	// [0x008] Height, in pixels
	uint16_t stride;	// [0x00A] Row stride, in bytes
	uint16_t color;		// [0x00C] Cursor color
} ICO_Win1_Header;
ASSERT_STRUCT(ICO_Win1_Header, 14);

/**
 * Windows 1.0: Icon format
 */
typedef enum {
	ICO_WIN1_FORMAT_MAYBE_WIN3	= 0x0000U,	// may be a Win3 icon/cursor

	ICO_WIN1_FORMAT_ICON_DIB	= 0x0001U,
	ICO_WIN1_FORMAT_ICON_DDB	= 0x0101U,
	ICO_WIN1_FORMAT_ICON_BOTH	= 0x0201U,

	ICO_WIN1_FORMAT_CURSOR_DIB	= 0x0003U,
	ICO_WIN1_FORMAT_CURSOR_DDB	= 0x0103U,
	ICO_WIN1_FORMAT_CURSOR_BOTH	= 0x0203U,
} ICO_Win1_Format_e;

/**
 * Windows 3.x: Icon header
 * Layout-compatible with the Windows 1.0 header.
 *
 * All fields are in little-endian.
 */
typedef struct _ICONDIR {
	uint16_t idReserved;	// [0x000] Zero for Win3.x icons
	uint16_t idType;	// [0x002] See ICO_Win3_idType_e
	uint16_t idCount;	// [0x004] Number of images
} ICONDIR, ICONHEADER;

/**
 * Windows 3.x: Icon types
 */
typedef enum {
	ICO_WIN3_TYPE_ICON	= 0x0001U,
	ICO_WIN3_TYPE_CURSOR	= 0x0002U,
} ICO_Win3_IdType_e;

#ifdef __cplusplus
}
#endif
