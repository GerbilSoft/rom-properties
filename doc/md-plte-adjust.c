/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * md-plte-adjust.c: Mega Drive palette adjustment tool.                   *
 *                                                                         *
 * This tool is used to adjust a screenshot taken from a Mega Drive        *
 * emulator that uses unscaled RGB, e.g. white == RGB(224,224,224), to the *
 * non-linear values as measured on SpritesMind:                           *
 * https://gendev.spritesmind.net/forum/viewtopic.php?t=2188               *
 *                                                                         *
 * Link with: -lz                                                          *
 *                                                                         *
 * Copyright (c) 2021 by David Korth.                                      *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

// Example bash command:
// for FILE in *.png; do if [ ! -L "${FILE}" ]; then ./md-plte-adjust "${FILE}"; fi; done

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

// VDP color lookup table. (non-S/H)
// Reference: https://gendev.spritesmind.net/forum/viewtopic.php?t=2188
static const uint8_t vdp_colors[8] = {
	0, 52, 87, 116, 144, 172, 206, 255
};

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

	// Adjust the RGB triplets.
	// NOTE: A failure will occur if any value has bits other than the
	// high 3 bits set, since this indicates one of the following:
	// - Shadow/Highlight is enabled.
	// - Image was *not* taken using the "raw" palette.
	// - Image is from a 32X game.
	for (i = 0; i < palette_len; i += 3) {
		uint8_t r, g, b;
		r = p_pal_data[i+0];
		g = p_pal_data[i+1];
		b = p_pal_data[i+2];
		if ((r & 0x1F) || (g & 0x1F) || (b & 0x1F)) {
			// Low bit is set.
			// NOTE: Last palette index might be RGB(255,255,255) for some reason,
			// even though it's not used. (not full optimization)
			if ((i + 3) == palette_len && (r == 0xFF) && (g == 0xFF) && (b == 0xFF)) {
				// We'll allow it.
			} else {
				// Low bits are set...
				fclose(f_png);
				fprintf(stderr, "*** ERROR updating PNG file '%s': Palette index %u has low RGB bits set.\n",
					png_filename, i / 3);
				return EXIT_FAILURE;
			}
		}

		p_pal_data[i+0] = vdp_colors[p_pal_data[i+0] >> 5];
		p_pal_data[i+1] = vdp_colors[p_pal_data[i+1] >> 5];
		p_pal_data[i+2] = vdp_colors[p_pal_data[i+2] >> 5];
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

	fprintf(stderr, "'%s': MD palette updated successfully.\n", png_filename);
	return EXIT_SUCCESS;
}
