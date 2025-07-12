/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * nes-plte-adjust.c: Mednafen NES palette adjustment tool.                *
 *                                                                         *
 * This tool adjusts PNG screenshots taken with Mednafen's default NES     *
 * palette to use TCRF's recommended NES palette.                          *
 * Reference: https://tcrf.net/Help:Contents/Taking_Screenshots#Palette    *
 *                                                                         *
 * Link with: -lz                                                          *
 *                                                                         *
 * Copyright (c) 2021-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

// Example bash command:
// for FILE in *.png; do if [ ! -L "${FILE}" ]; then ./nes-plte-adjust "${FILE}"; fi; done

#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <zlib.h>

// PACKED struct attribute.
// Use in conjunction with #pragma pack(1).
#ifdef __GNUC__
#  define PACKED __attribute__((packed))
#else
#  define PACKED
#endif

// NOTE: PNG stores all values in big-endian (network byte order) format.
// Use ntohl() and htonl() for portable conversion.

// NOTE: Chunk length does NOT include the length field, magic number, or
// CRC32. Hence, there's 12 extra bytes in each chunk.

// NOTE: CRC32 includes the magic number and data, but NOT the length field.

// Mednafen default NES palette
static const uint8_t mednafen_nes_palette[64][3] = {
        { 0x1D<<2, 0x1D<<2, 0x1D<<2 }, /* Value 0 */        { 0x09<<2, 0x06<<2, 0x23<<2 }, /* Value 1 */
        { 0x00<<2, 0x00<<2, 0x2A<<2 }, /* Value 2 */        { 0x11<<2, 0x00<<2, 0x27<<2 }, /* Value 3 */
        { 0x23<<2, 0x00<<2, 0x1D<<2 }, /* Value 4 */        { 0x2A<<2, 0x00<<2, 0x04<<2 }, /* Value 5 */
        { 0x29<<2, 0x00<<2, 0x00<<2 }, /* Value 6 */        { 0x1F<<2, 0x02<<2, 0x00<<2 }, /* Value 7 */
        { 0x10<<2, 0x0B<<2, 0x00<<2 }, /* Value 8 */        { 0x00<<2, 0x11<<2, 0x00<<2 }, /* Value 9 */
        { 0x00<<2, 0x14<<2, 0x00<<2 }, /* Value 10 */       { 0x00<<2, 0x0F<<2, 0x05<<2 }, /* Value 11 */
        { 0x06<<2, 0x0F<<2, 0x17<<2 }, /* Value 12 */       { 0x00<<2, 0x00<<2, 0x00<<2 }, /* Value 13 */
        { 0x00<<2, 0x00<<2, 0x00<<2 }, /* Value 14 */       { 0x00<<2, 0x00<<2, 0x00<<2 }, /* Value 15 */
        { 0x2F<<2, 0x2F<<2, 0x2F<<2 }, /* Value 16 */       { 0x00<<2, 0x1C<<2, 0x3B<<2 }, /* Value 17 */
        { 0x08<<2, 0x0E<<2, 0x3B<<2 }, /* Value 18 */       { 0x20<<2, 0x00<<2, 0x3C<<2 }, /* Value 19 */
        { 0x2F<<2, 0x00<<2, 0x2F<<2 }, /* Value 20 */       { 0x39<<2, 0x00<<2, 0x16<<2 }, /* Value 21 */
        { 0x36<<2, 0x0A<<2, 0x00<<2 }, /* Value 22 */       { 0x32<<2, 0x13<<2, 0x03<<2 }, /* Value 23 */
        { 0x22<<2, 0x1C<<2, 0x00<<2 }, /* Value 24 */       { 0x00<<2, 0x25<<2, 0x00<<2 }, /* Value 25 */
        { 0x00<<2, 0x2A<<2, 0x00<<2 }, /* Value 26 */       { 0x00<<2, 0x24<<2, 0x0E<<2 }, /* Value 27 */
        { 0x00<<2, 0x20<<2, 0x22<<2 }, /* Value 28 */       { 0x00<<2, 0x00<<2, 0x00<<2 }, /* Value 29 */
        { 0x00<<2, 0x00<<2, 0x00<<2 }, /* Value 30 */       { 0x00<<2, 0x00<<2, 0x00<<2 }, /* Value 31 */
        { 0x3F<<2, 0x3F<<2, 0x3F<<2 }, /* Value 32 */       { 0x0F<<2, 0x2F<<2, 0x3F<<2 }, /* Value 33 */
        { 0x17<<2, 0x25<<2, 0x3F<<2 }, /* Value 34 */       { 0x10<<2, 0x22<<2, 0x3F<<2 }, /* Value 35 */
        { 0x3D<<2, 0x1E<<2, 0x3F<<2 }, /* Value 36 */       { 0x3F<<2, 0x1D<<2, 0x2D<<2 }, /* Value 37 */
        { 0x3F<<2, 0x1D<<2, 0x18<<2 }, /* Value 38 */       { 0x3F<<2, 0x26<<2, 0x0E<<2 }, /* Value 39 */
        { 0x3C<<2, 0x2F<<2, 0x0F<<2 }, /* Value 40 */       { 0x20<<2, 0x34<<2, 0x04<<2 }, /* Value 41 */
        { 0x13<<2, 0x37<<2, 0x12<<2 }, /* Value 42 */       { 0x16<<2, 0x3E<<2, 0x26<<2 }, /* Value 43 */
        { 0x00<<2, 0x3A<<2, 0x36<<2 }, /* Value 44 */       { 0x1E<<2, 0x1E<<2, 0x1E<<2 }, /* Value 45 */
        { 0x00<<2, 0x00<<2, 0x00<<2 }, /* Value 46 */       { 0x00<<2, 0x00<<2, 0x00<<2 }, /* Value 47 */
        { 0x3F<<2, 0x3F<<2, 0x3F<<2 }, /* Value 48 */       { 0x2A<<2, 0x39<<2, 0x3F<<2 }, /* Value 49 */
        { 0x31<<2, 0x35<<2, 0x3F<<2 }, /* Value 50 */       { 0x35<<2, 0x32<<2, 0x3F<<2 }, /* Value 51 */
        { 0x3F<<2, 0x31<<2, 0x3F<<2 }, /* Value 52 */       { 0x3F<<2, 0x31<<2, 0x36<<2 }, /* Value 53 */
        { 0x3F<<2, 0x2F<<2, 0x2C<<2 }, /* Value 54 */       { 0x3F<<2, 0x36<<2, 0x2A<<2 }, /* Value 55 */
        { 0x3F<<2, 0x39<<2, 0x28<<2 }, /* Value 56 */       { 0x38<<2, 0x3F<<2, 0x28<<2 }, /* Value 57 */
        { 0x2A<<2, 0x3C<<2, 0x2F<<2 }, /* Value 58 */       { 0x2C<<2, 0x3F<<2, 0x33<<2 }, /* Value 59 */
        { 0x27<<2, 0x3F<<2, 0x3C<<2 }, /* Value 60 */       { 0x31<<2, 0x31<<2, 0x31<<2 }, /* Value 61 */
        { 0x00<<2, 0x00<<2, 0x00<<2 }, /* Value 62 */       { 0x00<<2, 0x00<<2, 0x00<<2 }, /* Value 63 */
};

// TCRF NES palette
static const uint8_t tcrf_nes_palette[64][3] = {
	{0x66, 0x66, 0x66},	{0x00, 0x2A, 0x88},	{0x14, 0x12, 0xA7},	{0x3B, 0x00, 0xA4},
	{0x5C, 0x00, 0x7E},	{0x6E, 0x00, 0x40},	{0x6C, 0x07, 0x00},	{0x56, 0x1D, 0x00},
	{0x33, 0x35, 0x00},	{0x0C, 0x48, 0x00},	{0x00, 0x52, 0x00},	{0x00, 0x4F, 0x08},
	{0x00, 0x40, 0x4D},	{0x00, 0x00, 0x00},	{0x00, 0x00, 0x00},	{0x00, 0x00, 0x00},
	{0xAD, 0xAD, 0xAD},	{0x15, 0x5F, 0xD9},	{0x42, 0x40, 0xFF},	{0x75, 0x27, 0xFE},
	{0xA0, 0x1A, 0xCC},	{0xB7, 0x1E, 0x7B},	{0xB5, 0x31, 0x20},	{0x99, 0x4E, 0x00},
	{0x6B, 0x6D, 0x00},	{0x38, 0x87, 0x00},	{0x0D, 0x93, 0x00},	{0x00, 0x8F, 0x32},
	{0x00, 0x7C, 0x8D},	{0x00, 0x00, 0x00},	{0x00, 0x00, 0x00},	{0x00, 0x00, 0x00},
	{0xFF, 0xFF, 0xFF},	{0x64, 0xB0, 0xFF},	{0x92, 0x90, 0xFF},	{0xC6, 0x76, 0xFF},
	{0xF2, 0x6A, 0xFF},	{0xFF, 0x6E, 0xCC},	{0xFF, 0x81, 0x70},	{0xEA, 0x9E, 0x22},
	{0xBC, 0xBE, 0x00},	{0x88, 0xD8, 0x00},	{0x5C, 0xE4, 0x30},	{0x45, 0xE0, 0x82},
	{0x48, 0xCD, 0xDE},	{0x4F, 0x4F, 0x4F},	{0x00, 0x00, 0x00},	{0x00, 0x00, 0x00},
	{0xFF, 0xFF, 0xFF},	{0xC0, 0xDF, 0xFF},	{0xD3, 0xD2, 0xFF},	{0xE8, 0xC8, 0xFF},
	{0xFA, 0xC2, 0xFF},	{0xFF, 0xC4, 0xEA},	{0xFF, 0xCC, 0xC5},	{0xF7, 0xD8, 0xA5},
	{0xE4, 0xE5, 0x94},	{0xCF, 0xEF, 0x96},	{0xBD, 0xF4, 0xAB},	{0xB3, 0xF3, 0xCC},
	{0xB5, 0xEB, 0xF2},	{0xB8, 0xB8, 0xB8},	{0x00, 0x00, 0x00},	{0x00, 0x00, 0x00},
};


// PNG header
static const uint8_t PNG_HDR[8] = {0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A};

// IHDR
#pragma pack(1)
typedef struct PACKED _IHDR_t {
	uint32_t length;	// sizeof(IHDR_t)-12 (length, magic, CRC32)
	char magic[4];		// "IHDR"
	uint32_t width;
	uint32_t height;
	uint8_t bit_depth;
	uint8_t color_type;
	uint8_t compression_method;
	uint8_t filter_method;
	uint8_t interlace_method;
	uint32_t crc32;
} IHDR_t;
#pragma pack()

// PLTE (header)
#pragma pack(1)
typedef struct PACKED _PLTE_t {
	uint32_t length;	// sizeof(PLTE_t)-12 (length, magic, CRC32), plus 3*entries
	char magic[4];		// "PLTE"
} PLTE_t;
#pragma pack()

/* These describe the color_type field in png_info. */
/* color type masks */
#define PNG_COLOR_MASK_PALETTE    1
#define PNG_COLOR_MASK_COLOR      2
#define PNG_COLOR_MASK_ALPHA      4

/* color types.  Note that not all combinations are legal */
#define PNG_COLOR_TYPE_GRAY 0
#define PNG_COLOR_TYPE_PALETTE  (PNG_COLOR_MASK_COLOR | PNG_COLOR_MASK_PALETTE)
#define PNG_COLOR_TYPE_RGB        (PNG_COLOR_MASK_COLOR)
#define PNG_COLOR_TYPE_RGB_ALPHA  (PNG_COLOR_MASK_COLOR | PNG_COLOR_MASK_ALPHA)
#define PNG_COLOR_TYPE_GRAY_ALPHA (PNG_COLOR_MASK_ALPHA)
/* aliases */
#define PNG_COLOR_TYPE_RGBA  PNG_COLOR_TYPE_RGB_ALPHA
#define PNG_COLOR_TYPE_GA  PNG_COLOR_TYPE_GRAY_ALPHA

int main(int argc, char *argv[])
{
	const char *png_filename;
	FILE *f_png;
	size_t size;
	long palette_pos;		// PLTE position in the file.
	unsigned int palette_len;	// PLTE length, in bytes.

	uint32_t crc32_orig, crc32_calc;	// CRC32s
	uint8_t *p_pal_data;	// Pointer to PLTE palette data in the buffer.
	uint8_t *p_pal_crc32;	// Poitnter to PLTE CRC32 in the buffer. (4 bytes)

	unsigned int i;

	union {
		uint8_t   u8[1024];
		uint16_t u16[1024>>1];
		uint32_t u32[1024>>2];
		IHDR_t ihdr;
		PLTE_t plte;
	} buf;

	if (argc != 2) {
		fprintf(stderr, "Syntax: %s file.png\n", argv[0]);
		return EXIT_FAILURE;
	}

	png_filename = argv[1];
	f_png = fopen(png_filename, "rb+");
	if (!f_png) {
		fprintf(stderr, "*** ERROR opening PNG file '%s': %s\n", png_filename, strerror(errno));
		return EXIT_FAILURE;
	}

	// Verify the PNG header.
	size = fread(buf.u8, 1, sizeof(PNG_HDR), f_png);
	if (size != sizeof(PNG_HDR) || memcmp(buf.u8, PNG_HDR, sizeof(PNG_HDR)) != 0) {
		fclose(f_png);
		fprintf(stderr, "*** ERROR reading PNG file '%s': PNG header is invalid.\n", png_filename);
		return EXIT_FAILURE;
	}

	// Read IHDR.
	size = fread(&buf.ihdr, 1, sizeof(buf.ihdr), f_png);
	if (size != sizeof(buf.ihdr) ||
	    ntohl(buf.ihdr.length) != sizeof(buf.ihdr)-12 ||
	    memcmp(buf.ihdr.magic, "IHDR", sizeof(buf.ihdr.magic)) != 0)
	{
		fclose(f_png);
		fprintf(stderr, "*** ERROR reading PNG file '%s': IHDR chunk is invalid or missing.\n",
			png_filename);
		return EXIT_FAILURE;
	}

	// Image must be color type 3 (paletted).
	// TODO: Verify IHDR CRC32?
	if (buf.ihdr.color_type != PNG_COLOR_TYPE_PALETTE) {
		fclose(f_png);
		fprintf(stderr, "*** ERROR reading PNG file '%s': Color type is not 3 (paletted).\n", png_filename);
		return EXIT_FAILURE;
	}

	// Read the PLTE header. (Must be the first chunk after IHDR.)
	size = fread(&buf.plte, 1, sizeof(buf.plte), f_png);
	if (size == sizeof(buf.plte) && !memcmp(buf.plte.magic, "acTL", 4)) {
		// acTL. This is an APNG.
		// PLTE should be immediately after it.
		// FIXME: First color seems to be unused and has low bits set...
		fseek(f_png, 8+4, SEEK_CUR);
		size = fread(&buf.plte, 1, sizeof(buf.plte), f_png);
	}
	if (size != sizeof(buf.plte) || memcmp(buf.plte.magic, "PLTE", 4) != 0) {
		fclose(f_png);
		fprintf(stderr, "*** ERROR reading PNG file '%s': PLTE chunk is invalid or missing.\n",
			png_filename);
		return EXIT_FAILURE;
	}

	// Read the rest of the palette and the CRC32.
	palette_len = ntohl(buf.plte.length);
	if (palette_len == 0) {
		fclose(f_png);
		fprintf(stderr, "*** ERROR reading PNG file '%s': PLTE chunk has size 0.\n", png_filename);
		return EXIT_FAILURE;
	} else if (palette_len > 3*256) {
		fclose(f_png);
		fprintf(stderr, "*** ERROR reading PNG file '%s': PLTE chunk has more than 256 entries.\n",
			png_filename);
		return EXIT_FAILURE;
	} else if (palette_len % 3 != 0) {
		fclose(f_png);
		fprintf(stderr, "*** ERROR reading PNG file '%s': PLTE chunk size %u is not a multiple of 3.\n",
			png_filename, palette_len);
		return EXIT_FAILURE;
	}

	palette_pos = ftell(f_png);
	p_pal_data = &buf.u8[sizeof(buf.plte)];
	size = fread(p_pal_data, 1, palette_len + sizeof(uint32_t), f_png);
	if (size != (palette_len + sizeof(uint32_t))) {
		fclose(f_png);
		fprintf(stderr, "*** ERROR reading PNG file '%s': Unable to read PLTE data.\n", png_filename);
		return EXIT_FAILURE;
	}

	// Verify the PLTE CRC32.
	p_pal_crc32 = p_pal_data + palette_len;
	crc32_calc = crc32(0, buf.plte.magic, sizeof(buf.plte.magic) + palette_len);
	crc32_orig = (p_pal_crc32[0] << 24) |
	             (p_pal_crc32[1] << 16) |
	             (p_pal_crc32[2] <<  8) |
	              p_pal_crc32[3];
	if (crc32_calc != crc32_orig) {
		fclose(f_png);
		fprintf(stderr, "*** ERROR reading PNG file '%s': Existing PLTE CRC32 is incorrect.\n", png_filename);
		return EXIT_FAILURE;
	}

	// Convert from Mednafen NES palette to TCRF NES palette.
	unsigned int index = 0;
	uint8_t *const p_end = p_pal_data + palette_len;
	for (uint8_t *p = p_pal_data; p < p_end; p += 3, index++) {
		bool found_match = false;

		for (unsigned int i = 0; i < 64; i++) {
			if (!memcmp(mednafen_nes_palette[i], p, 3)) {
				// Found a match.
				memcpy(p, tcrf_nes_palette[i], 3);
				found_match = true;
				break;
			}
		}

		if (!found_match) {
			// No match!
			fclose(f_png);
			fprintf(stderr, "*** ERROR updating PNG file '%s': Palette index %u is not in the Mednafen NES palette.\n",
				png_filename, index);
			return EXIT_FAILURE;
		}
	}

	// Update the CRC32.
	crc32_calc = crc32(0, buf.plte.magic, sizeof(buf.plte.magic) + palette_len);
	p_pal_crc32[0] = (crc32_calc >> 24);
	p_pal_crc32[1] = (crc32_calc >> 16) & 0xFF;
	p_pal_crc32[2] = (crc32_calc >>  8) & 0xFF;
	p_pal_crc32[3] = (crc32_calc)       & 0xFF;

	// Write the updated palette data.
	fseek(f_png, palette_pos, SEEK_SET);
	size = fwrite(p_pal_data, 1, palette_len + sizeof(uint32_t), f_png);
	fclose(f_png);
	if (size != (palette_len + sizeof(uint32_t))) {
		fprintf(stderr, "*** ERROR writing PNG file '%s': Write failed...\n", png_filename);
		return EXIT_FAILURE;
	}

	fprintf(stderr, "'%s': NES palette updated successfully.\n", png_filename);
	return EXIT_SUCCESS;
}
