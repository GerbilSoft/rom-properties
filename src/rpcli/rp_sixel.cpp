/***************************************************************************
 * ROM Properties Page shell extension. (rpcli)                            *
 * rp_sixel.cpp: Sixel output for rpcli.                                   *
 *                                                                         *
 * Copyright (c) 2025 by David Korth.                                      *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "rp_sixel.hpp"
#include "ctypex.h"

// libsixel
#include <sixel.h>

// librpbase
using namespace LibRpBase;

// librptexture
#include "librptexture/img/rp_image.hpp"
using namespace LibRpTexture;

// Terminal I/O
#include <termios.h>

static int sixel_write(char *data, int size, void *priv)
{
	return fwrite(data, 1, size, static_cast<FILE*>(priv));
}

static int print_sixel_image(sixel_output_t *output, const rp_image_const_ptr &image)
{
	SIXELSTATUS status = SIXEL_FALSE;
	sixel_dither_t *dither = nullptr;

	const int width = image->width();
	const int height = image->height();

	switch (image->format()) {
		default:
			// Unsupported...
			assert(!"Invalid icon format.");
			sixel_output_destroy(output);
			return -EIO;
		case rp_image::Format::CI8: {
			const argb32_t *palette = reinterpret_cast<const argb32_t*>(image->palette());
			unsigned int palette_len = image->palette_len();

			status = sixel_dither_new(&dither, palette_len, nullptr);
			if (SIXEL_FAILED(status)) {
				return -EIO;
			}

			// Sixel uses 24-bit RGB palettes.
			uint8_t rgb_palette[256*3];
			memset(rgb_palette, 0, sizeof(rgb_palette));
			for (uint8_t *p = rgb_palette; palette_len > 0; palette_len--, palette++, p += 3) {
				p[0] = palette->r;
				p[1] = palette->g;
				p[2] = palette->b;

				// NOTE: We need to pre-multiply the alpha.
				// FIXME: This only works properly with a black background.
				if (palette->a != 255) {
					p[0] = ((p[0] * palette->a) / 256);
					p[1] = ((p[1] * palette->a) / 256);
					p[2] = ((p[2] * palette->a) / 256);
				}
			}

			sixel_dither_set_pixelformat(dither, SIXEL_PIXELFORMAT_PAL8);
			sixel_dither_set_palette(dither, rgb_palette);
			break;
		}
		case rp_image::Format::ARGB32:
			// FIXME: High Color mode (ncolors == -1) isn't working.
			// sixel_dither_initialize() seems to work decently enough, though.
			// from libsixel-1.10.3's encoder.c, load_image_callback_for_palette()
			status = sixel_dither_new(&dither, 256, nullptr);
			if (SIXEL_FAILED(status)) {
				// Failed to allocate a High Color dither.
				return -EIO;
			}
			status = sixel_dither_initialize(dither,
				(uint8_t*)image->bits(), width, height,
				SIXEL_PIXELFORMAT_BGRA8888,
				SIXEL_LARGE_NORM, SIXEL_REP_CENTER_BOX, SIXEL_QUALITY_HIGH);

			sixel_dither_set_pixelformat(dither, SIXEL_PIXELFORMAT_BGRA8888);
			break;
	}

	// FIXME: Handle stride?
	sixel_encode((uint8_t*)image->bits(), width, height, 0, dither, output);
	sixel_dither_destroy(dither);
	return 0;
}

static bool tty_supports_sixel(FILE *f)
{
	// TODO: Check that the terminal supports sixel.
	// To check, send "\x1B[c", then check input for "\x1B[".
	// Ignore question marks; terminate on 'c'.
	// If any value (separated by semicolons) is 4, sixels are supported.
	// For now, only checking if output is a tty.
	// TODO: libgsvt / Windows handling?
	return !!isatty(fileno(f));
}

// TODO: Move to libgsvt?

/**
 * Get the size of a single character cell on the terminal.
 * stdin/stdout must be TTY.
 * @param pWidth	[out] Character width
 * @param pHeight	[out] Character height
 * @return 0 on success; negative POSIX error code on error.
 */
int tty_get_cell_size(int *pWidth, int *pHeight)
{
	static const char query_cmd[] = "\x1B[16t";

	// Adjust TTY handling for the query.
	struct termios term, initial_term;
	fflush(stdin);
	tcgetattr(STDIN_FILENO, &initial_term);
	term = initial_term;
	term.c_lflag &= ~ICANON;
	term.c_lflag &= ~ECHO;
	tcsetattr(STDIN_FILENO, TCSANOW, &term);

	fwrite(query_cmd, 1, sizeof(query_cmd)-1, stdout);
	fflush(stdout);

	// Wait 300ms for a response from the terminal.
	// Reference: https://stackoverflow.com/questions/16026858/reading-the-device-status-report-ansi-escape-sequence-reply
	fd_set readset;
	struct timeval time;
	FD_ZERO(&readset);
	FD_SET(STDIN_FILENO, &readset);
	time.tv_sec = 0;
	time.tv_usec = 300000;

	errno = 0;
	if (select(STDIN_FILENO + 1, &readset, nullptr, nullptr, &time) != 1) {
		// Failed to select() the data...
		int err = -errno;
		if (err == 0) {
			err = -EIO;
		}
		tcsetattr(STDIN_FILENO, TCSADRAIN, &initial_term);
		return err;
	}

	// Data should be in the format: "\x1B[6;_;_t"
	// - Param 1: height
	// - Param 2: width
	// Keep reading until we find a lowercase letter.
	char buf[16];
	size_t n = 0;
	for (; n < sizeof(buf); n++) {
		buf[n] = getc(stdin);
		if (islower_ascii(buf[n])) {
			n++;
			if (n < sizeof(buf)) {
				buf[n] = '\0';
			}
			break;
		}
	}
	if (n >= sizeof(buf) || buf[n] != '\0') {
		// Not a valid sequence?
		return -EIO;
	}

	// Use sscanf() to verify the string.
	int start_code = 0;
	char end_code = '\0';
	int s = sscanf(buf, "\x1B[%d;%d;%d%c", &start_code, pHeight, pWidth, &end_code);
	if (s != 4 || start_code != 6 || end_code != 't') {
		// Not valid...
		tcsetattr(STDIN_FILENO, TCSADRAIN, &initial_term);
		return -EIO;
	}

	// Retrieved the width and height.
	tcsetattr(STDIN_FILENO, TCSADRAIN, &initial_term);
	return 0;
}

void print_sixel_icon_banner(const RomDataPtr &romData)
{
	// Make sure the terminal supports sixel.
	if (!tty_supports_sixel(stdout)) {
		// Sixel is not supported.
		return;
	}

	// Get the character cell size.
	int cell_w = 0, cell_h = 0;
	int ret = tty_get_cell_size(&cell_w, &cell_h);
	if (ret != 0 || cell_w <= 0 || cell_h <= 0) {
		// Unable to get the character cell size...
		return;
	}

	// Get the icon and banner, and determine the maximum height.
	// NOTE: No scaling here...
	int max_height = 0;
	rp_image_const_ptr icon, banner;
	// Check for an icon first.
	const unsigned int imgbf = romData->supportedImageTypes();
	if (imgbf & RomData::IMGBF_INT_ICON) {
		icon = romData->image(RomData::IMG_INT_ICON);
		if (icon) {
			max_height = icon->height();
		}
	}
	if (imgbf & RomData::IMGBF_INT_BANNER) {
		banner = romData->image(RomData::IMG_INT_BANNER);
		if (banner) {
			max_height = std::max(max_height, banner->height());
		}
	}
	if (!icon && !banner) {
		// No images...
		return;
	}

	sixel_output_t *output = nullptr;
	SIXELSTATUS status = sixel_output_new(&output, sixel_write, stdout, nullptr);
	if (SIXEL_FAILED(status)) {
		// Failed to initialize sixel output.
		return;
	}

	// Determine how many rows are needed.
	int rows = max_height / cell_h;
	if (max_height % cell_h != 0) {
		rows++;
	}
	for (int i = 0; i < rows; i++) {
		putchar('\n');
	}
	printf("\x1B[%dA\x1B[s", rows);	// move up `rows` rows, save cursor position
	fflush(stdout);

	int cur_col = 0;
	if (icon) {
		print_sixel_image(output, icon);
		const int icon_width = icon->width() + 8;
		int icon_cols = icon_width / cell_w;
		if (icon_width % cell_w != 0) {
			icon_cols++;
		}
		cur_col += icon_cols;
		printf("\x1B[u\x1B[%dC", cur_col);	// restore cursor position; move right `cur_col` columns
		fflush(stdout);
	}
	if (banner) {
		print_sixel_image(output, banner);
		const int icon_width = icon->width() + 8;
		int icon_cols = icon_width / cell_w;
		if (icon_width % cell_w != 0) {
			icon_cols++;
		}
		cur_col += icon_cols;
		printf("\x1B[u\x1B[%dC", cur_col);	// restore cursor position; move right `cur_col` columns
		fflush(stdout);
	}

	// Move back down to after the icon/banner.
	printf("\x1B[%dE", rows);
	fflush(stdout);

	sixel_output_destroy(output);
}
