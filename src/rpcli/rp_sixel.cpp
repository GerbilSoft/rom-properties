/***************************************************************************
 * ROM Properties Page shell extension. (rpcli)                            *
 * rp_sixel.cpp: Sixel output for rpcli.                                   *
 *                                                                         *
 * Copyright (c) 2025 by David Korth.                                      *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "rp_sixel.hpp"

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
			// FIXME: High Color dithering isn't working...
			dither = sixel_dither_get(SIXEL_BUILTIN_XTERM256);
#if 0
			status = sixel_dither_new(&dither, -1, nullptr);
			if (SIXEL_FAILED(status)) {
				// Failed to allocate a High Color dither.
				return -EIO;
			}
#endif
			sixel_dither_set_pixelformat(dither, SIXEL_PIXELFORMAT_BGRA8888);
			break;
	}

	// FIXME: Handle stride?
	sixel_encode((uint8_t*)image->bits(), image->width(), image->height(), 0, dither, output);
	sixel_dither_destroy(dither);
	return 0;
}

void print_sixel_icon_banner(const RomDataPtr &romData)
{
	// TODO: Check that the terminal supports sixel.
	sixel_output_t *output = nullptr;
	SIXELSTATUS status = sixel_output_new(&output, sixel_write, stdout, nullptr);
	if (SIXEL_FAILED(status)) {
		// Failed to initialize sixel output.
		return;
	}

	// TODO: Print both the icon and banner on the same row?

	// Check for an icon first.
	const unsigned int imgbf = romData->supportedImageTypes();
	if (imgbf & RomData::IMGBF_INT_ICON) {
		rp_image_const_ptr icon = romData->image(RomData::IMG_INT_ICON);
		if (icon) {
			print_sixel_image(output, icon);
		}
	}
	if (imgbf & RomData::IMGBF_INT_BANNER) {
		rp_image_const_ptr banner = romData->image(RomData::IMG_INT_BANNER);
		if (banner) {
			print_sixel_image(output, banner);
		}
	}

	sixel_output_destroy(output);
}
