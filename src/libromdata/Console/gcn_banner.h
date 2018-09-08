/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * gcn_banner.h: Nintendo GameCube banner structures.                      *
 *                                                                         *
 * Copyright (c) 2016-2018 by David Korth.                                 *
 *                                                                         *
 * This program is free software; you can redistribute it and/or modify it *
 * under the terms of the GNU General Public License as published by the   *
 * Free Software Foundation; either version 2 of the License, or (at your  *
 * option) any later version.                                              *
 *                                                                         *
 * This program is distributed in the hope that it will be useful, but     *
 * WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 * GNU General Public License for more details.                            *
 *                                                                         *
 * You should have received a copy of the GNU General Public License       *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.   *
 ***************************************************************************/

/**
 * Reference:
 * - http://hitmen.c02.at/files/yagcd/yagcd/chap14.html
 */

#ifndef __ROMPROPERTIES_LIBROMDATA_GCN_BANNER_H__
#define __ROMPROPERTIES_LIBROMDATA_GCN_BANNER_H__

#include <stdint.h>
#include "librpbase/common.h"

#ifdef __cplusplus
extern "C" {
#endif

#pragma pack(1)

// Magic numbers.
#define GCN_BANNER_MAGIC_BNR1 'BNR1'
#define GCN_BANNER_MAGIC_BNR2 'BNR2'

// Banner size.
#define GCN_BANNER_IMAGE_W 96
#define GCN_BANNER_IMAGE_H 32
#define GCN_BANNER_IMAGE_SIZE ((GCN_BANNER_IMAGE_W * GCN_BANNER_IMAGE_H) * 2)

// NOTE: Strings are encoded in either cp1252 or Shift-JIS,
// depending on the game region.

// Banner comment.
typedef struct PACKED _gcn_banner_comment_t
{
	char gamename[0x20];
	char company[0x20];
	char gamename_full[0x40];
	char company_full[0x40];
	char gamedesc[0x80];
} gcn_banner_comment_t;
ASSERT_STRUCT(gcn_banner_comment_t, 0x140);

// BNR1
typedef struct PACKED _gcn_banner_bnr1_t
{
	uint32_t magic;					// BANNER_MAGIC_BNR1
	uint8_t reserved[0x1C];
	uint16_t banner[GCN_BANNER_IMAGE_SIZE/2];	// Banner image. (96x32, RGB5A3)
	gcn_banner_comment_t comment;
} gcn_banner_bnr1_t;
ASSERT_STRUCT(gcn_banner_bnr1_t, 0x1820 + 0x140);

// BNR2
typedef struct PACKED _gcn_banner_bnr2_t
{
	uint32_t magic;					// BANNER_MAGIC_BNR2
	uint8_t reserved[0x1C];
	uint16_t banner[GCN_BANNER_IMAGE_SIZE/2];	// Banner image. (96x32, RGB5A3)
	gcn_banner_comment_t comments[6];
} gcn_banner_bnr2_t;
ASSERT_STRUCT(gcn_banner_bnr2_t, 0x1820 + (0x140 * 6));

// BNR2 languages. (Maps to GameCube language setting.)
typedef enum {
	GCN_PAL_LANG_ENGLISH	= 0,
	GCN_PAL_LANG_GERMAN	= 1,
	GCN_PAL_LANG_FRENCH	= 2,
	GCN_PAL_LANG_SPANISH	= 3,
	GCN_PAL_LANG_ITALIAN	= 4,
	GCN_PAL_LANG_DUTCH	= 5,
} GCN_PAL_Language_ID;

#pragma pack()

#ifdef __cplusplus
}
#endif

#endif /* __ROMPROPERTIES_LIBROMDATA_GCN_BANNER_H__ */
