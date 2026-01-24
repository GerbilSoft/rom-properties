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

// libgsvt for VT handling
#include "gsvt.h"

// libsixel
#include <sixel.h>

// librpbase
using namespace LibRpBase;

// librptexture
#include "librptexture/img/rp_image.hpp"
using namespace LibRpTexture;

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

void print_sixel_icon_banner(const RomDataPtr &romData)
{
	// Make sure the terminal supports sixel.
	if (!gsvt_supports_sixel()) {
		// Sixel is not supported.
		return;
	}

	// Get the character cell size.
	int cell_size_w = 0, cell_size_h = 0;
	int ret = gsvt_get_cell_size(&cell_size_w, &cell_size_h);
	if (ret != 0 || cell_size_w <= 0 || cell_size_h <= 0) {
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
	int rows = max_height / cell_size_h;
	if (max_height % cell_size_h != 0) {
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
		int icon_cols = icon_width / cell_size_w;
		if (icon_width % cell_size_w != 0) {
			icon_cols++;
		}
		cur_col += icon_cols;
		printf("\x1B[u\x1B[%dC", cur_col);	// restore cursor position; move right `cur_col` columns
		fflush(stdout);
	}
	if (banner) {
		print_sixel_image(output, banner);
		const int icon_width = icon->width() + 8;
		int icon_cols = icon_width / cell_size_w;
		if (icon_width % cell_size_w != 0) {
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
