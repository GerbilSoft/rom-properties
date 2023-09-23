/***************************************************************************
 * ROM Properties Page shell extension. (librptexture)                     *
 * tga_structs.h: TrueVision TGA texture format data structures.           *
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

// References:
// - https://en.wikipedia.org/wiki/Truevision_TGA
// - https://www.ludorg.net/amnesia/TGA_File_Format_Spec.html
// - http://www.paulbourke.net/dataformats/tga/
// - https://www.dca.fee.unicamp.br/~martino/disciplinas/ea978/tgaffs.pdf

// NOTE: 16-bit color is 15-bit RGB + 1-bit transparency.

// Maximum supported TGA file size is 16 MB.
#define TGA_MAX_SIZE (16U*1024*1024)

/**
 * TrueVision TGA: Main header.
 * All fields are little-endian.
 */
#pragma pack(1)
typedef struct PACKED _TGA_Header {
	uint8_t id_length;		// [0x000] Length of ID field. (0 if not present)
	uint8_t color_map_type;		// [0x001] Color map type: 0 if none, 1 if present.
	uint8_t image_type;		// [0x002] Image type. (See TGA_ImageType_e.)
	struct {
		uint16_t idx0;		// [0x003] Index of first color map entry.
		uint16_t len;		// [0x005] Number of entries in color map.
		uint8_t bpp;		// [0x007] Bits per pixel in color map. (15/16/24/32)
	} cmap;
	struct {
		uint16_t x_origin;	// [0x008] Lower-left corner for displays where
		uint16_t y_origin;	// [0x00A]     the origin is at the lower left.
		uint16_t width;		// [0x00C]
		uint16_t height;	// [0x00E]
		uint8_t bpp;		// [0x010] Color depth (8/15/16/24/32)
		uint8_t attr_dir;	// [0x011] Bits 3-0 == attribute bits (usually alpha)
					//         Bits 5-4 == orientation (see TGA_Orientation_e)
	} img;
} TGA_Header;
ASSERT_STRUCT(TGA_Header, 18);
#pragma pack()

/**
 * TGA image type.
 */
typedef enum {
	TGA_IMAGETYPE_NONE	= 0,	// No image data
	TGA_IMAGETYPE_COLORMAP	= 1,	// Color-mapped image (8/15/16 bpp)
	TGA_IMAGETYPE_TRUECOLOR	= 2,	// True color image
	TGA_IMAGETYPE_GRAYSCALE	= 3,	// Grayscale image (8 bpp)

	TGA_IMAGETYPE_RLE_FLAG		= 8,	// Flag indicates image is compressed using RLE.
	TGA_IMAGETYPE_RLE_COLORMAP	= (TGA_IMAGETYPE_COLORMAP | TGA_IMAGETYPE_RLE_FLAG),
	TGA_IMAGETYPE_RLE_TRUECOLOR	= (TGA_IMAGETYPE_TRUECOLOR | TGA_IMAGETYPE_RLE_FLAG),
	TGA_IMAGETYPE_RLE_GRAYSCALE	= (TGA_IMAGETYPE_GRAYSCALE | TGA_IMAGETYPE_RLE_FLAG),

	// Huffman+Delta compressed
	TGA_IMAGETYPE_HUFFMAN_FLAG		= 32,
	TGA_IMAGETYPE_HUFFMAN_COLORMAP		= (0 | TGA_IMAGETYPE_HUFFMAN_FLAG),
	TGA_IMAGETYPE_HUFFMAN_4PASS_COLORMAP	= (1 | TGA_IMAGETYPE_HUFFMAN_FLAG),
} TGA_ImageType_e;

/**
 * TGA image orientation.
 */
typedef enum {
	TGA_ORIENTATION_X_LTR	= (0U << 4),	// X: left-to-right
	TGA_ORIENTATION_X_RTL	= (1U << 4),	// X: right-to-left
	TGA_ORIENTATION_X_MASK	= (1U << 4),

	TGA_ORIENTATION_Y_UP	= (0U << 5),	// Y: bottom-to-top
	TGA_ORIENTATION_Y_DOWN	= (1U << 5),	// Y: top-to-bottom
	TGA_ORIENTATION_Y_MASK	= (1U << 5),
} TGA_Orientation_e;

/**
 * TrueVision TGA: Date stamp.
 * All fields are little-endian.
 */
typedef struct _TGA_DateStamp {
	uint16_t month;	// [0x000] 1-12
	uint16_t day;	// [0x002] 1-31
	uint16_t year;	// [0x004] 4-digit year, e.g. 1989
	uint16_t hour;	// [0x006] 0-23
	uint16_t min;	// [0x008] 0-59
	uint16_t sec;	// [0x00A] 0-59
} TGA_DateStamp;
ASSERT_STRUCT(TGA_DateStamp, 6*sizeof(uint16_t));

/**
 * TrueVision TGA: Elapsed time.
 * All fields are little-endian.
 */
// Some compilers pad this structure to a multiple of 4 bytes
#pragma pack(1)
typedef struct PACKED _TGA_ElapsedTime {
	uint16_t hours;	// [0x000] 0-65535
	uint16_t mins;	// [0x002] 0-59
	uint16_t secs;	// [0x004] 0-59
} TGA_ElapsedTime;
ASSERT_STRUCT(TGA_ElapsedTime, 3*sizeof(uint16_t));
#pragma pack()

/**
 * TrueVision TGA: Software version.
 * All fields are little-endian.
 */
#pragma pack(1)
typedef struct PACKED _TGA_SW_Version {
	uint16_t number;	// [0x000] Version number * 100 (0 for unused)
				//             Example: 213 for version 2.13
	char letter;		// [0x002] Version letter suffix (' ' for unused)
				//             Example: 'b' for version 2.13b
} TGA_SW_Version;
#pragma pack()
ASSERT_STRUCT(TGA_SW_Version, 3);

/**
 * TrueVision TGA: Ratio values.
 * Used for pixel aspect ratio and gamma values.
 * All fields are little-endian.
 */
typedef struct _TGA_Ratio {
	uint16_t numerator;	// [0x000] Numerator
	uint16_t denominator;	// [0x002] Denominator
} TGA_Ratio;
ASSERT_STRUCT(TGA_Ratio, 2*sizeof(uint16_t));

/**
 * TrueVision TGA: Extension area.
 * Present if the size field is 495.
 * All fields are little-endian.
 */
#pragma pack(1)
typedef struct PACKED _TGA_ExtArea {
	uint16_t size;			// [0x000] Extension area size. (Always 495)
	char author_name[41];		// [0x002] Author name, NULL-terminated.
	char author_comment[4][81];	// [0x02B] Comment lines, NULL-terminated.
	TGA_DateStamp timestamp;	// [0x16F] Timestamp
	char job_id[41];		// [0x17B] Job ID
	TGA_ElapsedTime job_time;	// [0x1A4] Time taken to create the job
	char software_id[41];		// [0x1AA] Application that created the file
	TGA_SW_Version sw_version;	// [0x1D3] Software version
	uint32_t key_color;		// [0x1D6] Key color (ARGB)
	TGA_Ratio pixel_aspect_ratio;	// [0x1DA] Num=W, Denom=H
	TGA_Ratio gamma_value;		// [0x1DE]

	// Data offsets if non-zero.
	uint32_t color_correction_offset;	// [0x1E2]
	uint32_t postage_stamp_offset;		// [0x1E6]
	uint32_t scan_line_offset;		// [0x1EA]

	uint8_t attributes_type;	// [0x1EE] Alpha channel type (see TGA_AlphaType_e)
} TGA_ExtArea;
ASSERT_STRUCT(TGA_ExtArea, 495);
#pragma pack()

/**
 * TGA alpha channel type.
 */
typedef enum {
	TGA_ALPHATYPE_NONE		= 0,
	TGA_ALPHATYPE_UNDEFINED_IGNORE	= 1,
	TGA_ALPHATYPE_UNDEFINED_RETAIN	= 2,
	TGA_ALPHATYPE_PRESENT		= 3,	// Standard alpha
	TGA_ALPHATYPE_PREMULTIPLIED	= 4,	// Premultiplied alpha
} TGA_AlphaType_e;

/**
 * TrueVision TGA: Footer.
 * Optional, but present in most TGA v2.0 files.
 * All fields are little-endian.
 */
#define TGA_SIGNATURE "TRUEVISION-XFILE"
#pragma pack(1)
typedef struct PACKED _TGA_Footer {
	uint32_t ext_offset;		// [0x000] Offset from the beginning of the file.
	uint32_t dev_area_offset;	// [0x004] Offset from the beginning of the file.
	char signature[16];		// [0x008]
	char dot;			// [0x018] '.'
	char null_byte;			// [0x019] '\0'
} TGA_Footer;
#pragma pack()
ASSERT_STRUCT(TGA_Footer, 26);

#ifdef __cplusplus
}
#endif
