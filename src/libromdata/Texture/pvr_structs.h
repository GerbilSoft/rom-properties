/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * pvr_structs.h: Sega PVR image format data structures.                   *
 *                                                                         *
 * Copyright (c) 2017 by David Korth.                                      *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_LIBROMDATA_PVR_STRUCTS_H__
#define __ROMPROPERTIES_LIBROMDATA_PVR_STRUCTS_H__

#include "librpbase/common.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#pragma pack(1)

/**
 * References:
 * - http://fabiensanglard.net/Mykaruga/tools/segaPVRFormat.txt
 * - https://github.com/yevgeniy-logachev/spvr2png/blob/master/SegaPVRImage.c
 */

/**
 * Global Index header for all PVR formats.
 * Index endianness depends on PVR format:
 * - PVR:  Little-endian.
 * - PVRX: Little-endian.
 * - GVR:  Big-endian.
 */
#define PVR_MAGIC_GBIX 'GBIX'
#define PVR_MAGIC_GCIX 'GCIX'
typedef struct PACKED _PVR_GBIX_Header {
	uint32_t magic;		// [0x000] 'GBIX' (or 'GCIX' in Wii games)
	uint32_t length;	// [0x004] Length of GBIX header. (***ALWAYS*** little-endian!)
	uint32_t index;		// [0x008] Global index.

	// NOTE: GBIX may or may not have an extra 4 bytes of padding.
	// It usually does, so length == 8.
	// Otherwise, length == 4.
} PVR_GBIX_Header;
ASSERT_STRUCT(PVR_GBIX_Header, 12);

/**
 * Common PVR header.
 * - Dreamcast PVR: All fields are little-endian.
 * - GameCube GVR: All fields are big-endian.
 */
#define PVR_MAGIC_PVRT 'PVRT'
#define PVR_MAGIC_GVRT 'GVRT'
#define PVR_MAGIC_PVRX 'PVRX'
typedef struct PACKED _PVR_Header {
	uint32_t magic;		// [0x000] 'PVRT', 'GVRT', or 'PVRX'
	uint32_t length;	// [0x004] Length of the file, starting at px_format.
	union {
		struct {
			uint8_t px_format;	// [0x008] Pixel format.
			uint8_t img_data_type;	// [0x009] Image data type.
			uint8_t reserved[2];	// [0x00A] 0x0000
		} pvr;
		struct {
			uint8_t reserved[2];	// [0x008] 0x0000
			uint8_t px_format;	// [0x00A] Pixel format.
			uint8_t img_data_type;	// [0x00B] Image data type.
		} gvr;
	};
	uint16_t width;		// [0x00C] Width
	uint16_t height;	// [0x00E] Height
} PVR_Header;
ASSERT_STRUCT(PVR_Header, 16);

/**
 * PVR pixel formats.
 */
typedef enum {
	// Sega Dreamcast (PVR)
	PVR_PX_ARGB1555	= 0x00,
	PVR_PX_RGB565	= 0x01,
	PVR_PX_ARGB4444	= 0x02,
	PVR_PX_YUV422	= 0x03,
	PVR_PX_BUMP	= 0x04,
	PVR_PX_4BIT	= 0x05,
	PVR_PX_8BIT	= 0x06,

	// Sony PlayStation 2 (SVR)
	SVR_PX_BGR5A3		= 0x08,
	SVR_PX_BGR888_ABGR7888	= 0x09,

	SVR_PX_MIN = SVR_PX_BGR5A3,
	SVR_PX_MAX = SVR_PX_BGR888_ABGR7888,
} PVR_Pixel_Format_t;

/**
 * PVR image data types.
 */
typedef enum {
	// Sega Dreamcast (PVR)
	PVR_IMG_SQUARE_TWIDDLED			= 0x01,
	PVR_IMG_SQUARE_TWIDDLED_MIPMAP		= 0x02,
	PVR_IMG_VQ				= 0x03,
	PVR_IMG_VQ_MIPMAP			= 0x04,
	PVR_IMG_CI8_TWIDDLED			= 0x05,
	PVR_IMG_CI4_TWIDDLED			= 0x06,
	PVR_IMG_P8_TWIDDLED			= 0x07,
	PVR_IMG_P4_TWIDDLED			= 0x08,
	PVR_IMG_RECTANGLE			= 0x09,
	PVR_IMG_RECTANGULAR_STRIDE		= 0x0B,
	PVR_IMG_RECTANGULAR_TWIDDLED		= 0x0D,
	PVR_IMG_SMALL_VQ			= 0x10,
	PVR_IMG_SMALL_VQ_MIPMAP			= 0x11,
	PVR_IMG_SQUARE_TWIDDLED_MIPMAP_ALT	= 0x12,

	// Sony PlayStation 2 (SVR)
	SVR_IMG_RECTANGLE			= 0x60,
	SVR_IMG_RECTANGLE_SWIZZLED		= 0x61,
	SVR_IMG_INDEX4_EXT_PALETTE		= 0x62,
	SVR_IMG_INDEX8_EXT_PALETTE		= 0x64,
	SVR_IMG_INDEX4_BGR5A3_RECTANGLE		= 0x66,
	SVR_IMG_INDEX4_BGR5A3_SQUARE		= 0x67,
	SVR_IMG_INDEX4_ABGR8_RECTANGLE		= 0x68,
	SVR_IMG_INDEX4_ABGR8_SQUARE		= 0x69,
	SVR_IMG_INDEX8_BGR5A3_RECTANGLE		= 0x6A,
	SVR_IMG_INDEX8_BGR5A3_SQUARE		= 0x6B,
	SVR_IMG_INDEX8_ABGR8_RECTANGLE		= 0x6C,
	SVR_IMG_INDEX8_ABGR8_SQUARE		= 0x6D,

	SVR_IMG_MIN = SVR_IMG_RECTANGLE,
	SVR_IMG_MAX = SVR_IMG_INDEX8_ABGR8_SQUARE,
} PVR_Image_Data_Type_t;

/**
 * GVR pixel formats.
 * FIXME: Not sure if this is valid for GVR.
 */
typedef enum {
	GVR_PX_IA8	= 0x00,
	GVR_PX_RGB565	= 0x01,
	GVR_PX_RGB5A3	= 0x02,
	GVR_PX_UNKNOWN	= 0xFF,
} GVR_Pixel_Format_t;

/**
 * GVR image data types.
 */
typedef enum {
	GVR_IMG_I4		= 0x00,
	GVR_IMG_I8		= 0x01,
	GVR_IMG_IA4		= 0x02,
	GVR_IMG_IA8		= 0x03,
	GVR_IMG_RGB565		= 0x04,
	GVR_IMG_RGB5A3		= 0x05,
	GVR_IMG_ARGB8888	= 0x06,
	GVR_IMG_CI4		= 0x08,
	GVR_IMG_CI8		= 0x09,
	GVR_IMG_DXT1		= 0x0E,
} GVR_Image_Data_Type_t;

#pragma pack()

#ifdef __cplusplus
}
#endif

#endif /* __ROMPROPERTIES_LIBROMDATA_PVR_STRUCTS_H__ */
