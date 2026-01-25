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

// We're opening libsixel dynamically, so we need to define all
// libsixel function prototypes and definitions here.
#include "sixel-mini.h"

#ifndef _WIN32
// Unix dlopen()
#  include <dlfcn.h>
typedef void *HMODULE;
// T2U8() is a no-op.
#  define T2U8(str) (str)
#else /* _WIN32 */
// Windows LoadLibrary()
#  include "libwin32common/RpWin32_sdk.h"
#  include "libwin32common/MiniU82T.hpp"
#  include "tcharx.h"
#  define dlsym(handle, symbol)	((void*)GetProcAddress(handle, symbol))
#  define dlclose(handle)	FreeLibrary(handle)
using LibWin32Common::T2U8;
#endif /* _WIN32 */

// librpbase
using namespace LibRpBase;

// librptexture
#include "librptexture/img/rp_image.hpp"
using namespace LibRpTexture;

// C++ STL classes
#include <mutex>
using std::array;
using std::unique_ptr;

namespace SixelDll {

class HMODULE_deleter
{
public:
	typedef HMODULE pointer;

	void operator()(HMODULE hModule)
	{
		if (hModule) {
			dlclose(hModule);
		}
	}
};
static unique_ptr<HMODULE, HMODULE_deleter> libsixel_dll;
static std::once_flag sixel_once_flag;

// Function pointer macro
#define DEF_STATIC_FUNCPTR(f) static __typeof__(f) * p##f = nullptr
#define DEF_FUNCPTR(f) __typeof__(f) * p##f = nullptr
#define LOAD_FUNCPTR(f) p##f = (__typeof__(f)*)dlsym(libsixel_dll.get(), #f)

DEF_STATIC_FUNCPTR(sixel_output_new);
DEF_STATIC_FUNCPTR(sixel_output_destroy);

DEF_STATIC_FUNCPTR(sixel_dither_new);
DEF_STATIC_FUNCPTR(sixel_dither_destroy);
DEF_STATIC_FUNCPTR(sixel_dither_initialize);
DEF_STATIC_FUNCPTR(sixel_dither_set_palette);
DEF_STATIC_FUNCPTR(sixel_dither_set_pixelformat);

DEF_STATIC_FUNCPTR(sixel_encode);

/**
 * Initialize libsixel.
 * Called by std::call_once().
 *
 * Check if libsixel_dll is nullptr afterwards.
 */
static void init_sixel_once(void)
{
	// Open libsixel.
#ifdef _WIN32
	// TODO: Also check arch-specific subdirectory?
	libsixel_dll.reset(LoadLibraryEx(_T("libsixel-1.dll"), nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32 | LOAD_LIBRARY_SEARCH_APPLICATION_DIR));
#else /* !_WIN32 */
	// TODO: Consistently use either RTLD_NOW or RTLD_LAZY.
	// Maybe make it a CMake option?
	// TODO: Check for ABI version 3 too?
	// Version 4 was introduced with cURL v7.16.0 (October 2006),
	// but Debian arbitrarily kept it at version 3.
	// Reference: https://daniel.haxx.se/blog/2024/10/30/eighteen-years-of-abi-stability/
	libsixel_dll.reset(dlopen("libsixel.so.1", RTLD_LOCAL | RTLD_NOW));
#endif /* _WIN32 */

	if (!libsixel_dll) {
		// Could not open libcurl.
		return;
	}

	// Load all of the function pointers.
	LOAD_FUNCPTR(sixel_output_new);
	LOAD_FUNCPTR(sixel_output_destroy);

	LOAD_FUNCPTR(sixel_dither_new);
	LOAD_FUNCPTR(sixel_dither_destroy);
	LOAD_FUNCPTR(sixel_dither_initialize);
	LOAD_FUNCPTR(sixel_dither_set_palette);
	LOAD_FUNCPTR(sixel_dither_set_pixelformat);

	LOAD_FUNCPTR(sixel_encode);

	if (!psixel_output_new ||
	    !psixel_output_destroy ||
	    !psixel_dither_new ||
	    !psixel_dither_destroy ||
	    !psixel_dither_initialize ||
	    !psixel_dither_set_palette ||
	    !psixel_dither_set_pixelformat ||
	    !psixel_encode)
	{
		// At least one symbol is missing.
		libsixel_dll.reset();
		return;
	}
}

}

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
			SixelDll::psixel_output_destroy(output);
			return -EIO;
		case rp_image::Format::CI8: {
			const argb32_t *palette = reinterpret_cast<const argb32_t*>(image->palette());
			unsigned int palette_len = image->palette_len();

			status = SixelDll::psixel_dither_new(&dither, palette_len, nullptr);
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

			SixelDll::psixel_dither_set_pixelformat(dither, SIXEL_PIXELFORMAT_PAL8);
			SixelDll::psixel_dither_set_palette(dither, rgb_palette);
			break;
		}
		case rp_image::Format::ARGB32:
			// FIXME: High Color mode (ncolors == -1) isn't working.
			// sixel_dither_initialize() seems to work decently enough, though.
			// from libsixel-1.10.3's encoder.c, load_image_callback_for_palette()
			status = SixelDll::psixel_dither_new(&dither, 256, nullptr);
			if (SIXEL_FAILED(status)) {
				// Failed to allocate a High Color dither.
				return -EIO;
			}
			status = SixelDll::psixel_dither_initialize(dither,
				(uint8_t*)image->bits(), width, height,
				SIXEL_PIXELFORMAT_BGRA8888,
				SIXEL_LARGE_NORM, SIXEL_REP_CENTER_BOX, SIXEL_QUALITY_HIGH);

			SixelDll::psixel_dither_set_pixelformat(dither, SIXEL_PIXELFORMAT_BGRA8888);
			break;
	}

	// FIXME: Handle stride?
	SixelDll::psixel_encode((uint8_t*)image->bits(), width, height, 0, dither, output);
	SixelDll::psixel_dither_destroy(dither);
	return 0;
}

static void print_sixel_icon_banner_int(const RomDataPtr &romData)
{
	// Load libsixel.
	std::call_once(SixelDll::sixel_once_flag, SixelDll::init_sixel_once);
	if (!SixelDll::libsixel_dll) {
		// libsixel could not be loaded...
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
	SIXELSTATUS status = SixelDll::psixel_output_new(&output, sixel_write, stdout, nullptr);
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
	}
	if (banner) {
		print_sixel_image(output, banner);
		const int banner_width = banner->width() + 8;
		int banner_cols = banner_width / cell_size_w;
		if (banner_width % cell_size_w != 0) {
			banner_cols++;
		}
		cur_col += banner_cols;
		printf("\x1B[u\x1B[%dC", cur_col);	// restore cursor position; move right `cur_col` columns
	}

	// Move back down to after the icon/banner.
	printf("\x1B[%dE", rows);
	fflush(stdout);

	SixelDll::psixel_output_destroy(output);
}

static int print_kitty_image(rp_image_const_ptr image_src)
{
	// Kitty requires R/B to be swapped.
	rp_image_ptr image = image_src->dup_ARGB32();
	if (!image->isValid()) {
		// Failed to dup() and/or convert to ARGB32...
		return -EINVAL;
	}

	// Swap the R/B channels.
	{
		argb32_t *bits = reinterpret_cast<argb32_t*>(image->bits());
		const int strideDiff = image->row_bytes() - (image->width() * sizeof(uint32_t));
		for (unsigned int y = (unsigned int)image->height(); y > 0; y--) {
			unsigned int x;
			for (x = (unsigned int)image->width(); x > 1; x -= 2) {
				// Swap the R and B channels
				std::swap(bits[0].r, bits[0].b);
				std::swap(bits[1].r, bits[1].b);
				bits += 2;
			}
			if (x == 1) {
				// Last pixel
				std::swap(bits->r, bits->b);
				bits++;
			}

			// Next line.
			bits += strideDiff;
		}
	}

	const int width = image->width();
	const int height = image->height();

	// Print the Kitty header.
	printf("\x1B_Gf=32,s=%d,v=%d,a=T;", width, height);

	// Allocate a line buffer for base64 conversion.
	// 3 input bytes == 4 output bytes
	// NOTE: Console output must be printed in 4 KB chunks.
	// Extra 8 bytes to handle scanline shenanigans.
	size_t linebuf_size = ((static_cast<size_t>(width) * sizeof(uint32_t)) * 4 / 3) + 8;
	// Round up to 4 bytes.
	size_t rem = linebuf_size % 4;
	if (rem != 0) {
		rem = (4 - rem);
		linebuf_size += rem;
	}

	unique_ptr<char[]> linebuf(new char[linebuf_size]);

	static const array<char, 64> base64_encoding_table = {{
		'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
		'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
		'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
		'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
		'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
		'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
		'w', 'x', 'y', 'z', '0', '1', '2', '3',
		'4', '5', '6', '7', '8', '9', '+', '/'
	}};

	const uint8_t *bits = static_cast<const uint8_t*>(image->bits());
	const size_t row_bytes = static_cast<size_t>(image->row_bytes());
	const size_t stride_adj = (static_cast<size_t>(image->stride()) - row_bytes);
	for (int y = height; y > 0; y--, bits += stride_adj) {
		// Process 3 input bytes at a time.
		size_t n;
		char *p = linebuf.get();
		for (n = row_bytes; n >= 3; n -= 3, bits += 3, p += 4) {
			uint32_t data = (bits[0] << 16) | (bits[1] << 8) | bits[2];
			p[0] = base64_encoding_table[(data >> (3*6)) & 0x3F];
			p[1] = base64_encoding_table[(data >> (2*6)) & 0x3F];
			p[2] = base64_encoding_table[(data >> (1*6)) & 0x3F];
			p[3] = base64_encoding_table[(data >> (0*6)) & 0x3F];
		}

		if (n > 0) {
			// Encode the remaining bytes using equal signs to indicate
			// that some bytes are missing.
			uint32_t data = 0;
			for (size_t o = n; o > 0; o--, bits++) {
				data <<= 8;
				data |= *bits;
			}
			data <<= ((3 - n) * 8);

			p[0] = base64_encoding_table[(data >> (3*6)) & 0x3F];
			p[1] = base64_encoding_table[(data >> (2*6)) & 0x3F];
			p[2] = base64_encoding_table[(data >> (1*6)) & 0x3F];
			p[3] = base64_encoding_table[(data >> (0*6)) & 0x3F];

			// Number of bytes to overwrite depends on number of source bytes remaining:
			// - n = 1: overwrite 2, 3
			// - n = 2: overwrite    3
			switch (n) {
				default:
					break;
				case 1:
					p[2] = '=';
					// fall-through
				case 2:
					p[3] = '=';
					break;
			}
			p += 4;
		}

		// Write the encoded data in 4 KB chunks.
		static constexpr size_t BASE64_CHUNK_SIZE = 4096U;
		size_t linebuf_used = static_cast<size_t>(p - linebuf.get());
		p = linebuf.get();
		while (linebuf_used > 0) {
			size_t bytes_to_print = std::min(linebuf_used, BASE64_CHUNK_SIZE);
			fwrite(p, 1, bytes_to_print, stdout);
			p += bytes_to_print;
			linebuf_used -= bytes_to_print;
		}
	}

	// End of data.
	fwrite("\x1B\\", 1, 2, stdout);
	return 0;
}

static void print_kitty_icon_banner_int(const RomDataPtr &romData)
{
	// Kitty protocol is very simple: it accepts base64-encoded
	// PNG or raw graphics data.
	// TODO: Stride handling for raw graphics data.
	// TODO: Pass-through PNG data?
	// TODO: Also supports animated icons.

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

	// Determine how many rows are needed.
	int rows = max_height / cell_size_h;
	if (max_height % cell_size_h != 0) {
		rows++;
	}
	for (int i = 0; i < rows; i++) {
		putchar('\n');
	}
	printf("\x1B[%dA\x1B[s", rows);	// move up `rows` rows, save cursor position

	int cur_col = 0;
	if (icon) {
		print_kitty_image(icon);
		const int icon_width = icon->width() + 8;
		int icon_cols = icon_width / cell_size_w;
		if (icon_width % cell_size_w != 0) {
			icon_cols++;
		}
		cur_col += icon_cols;
		printf("\x1B[u\x1B[%dC", cur_col);	// restore cursor position; move right `cur_col` columns
	}
	if (banner) {
		print_kitty_image(banner);
		const int banner_width = banner->width() + 8;
		int banner_cols = banner_width / cell_size_w;
		if (banner_width % cell_size_w != 0) {
			banner_cols++;
		}
		cur_col += banner_cols;
		printf("\x1B[u\x1B[%dC", cur_col);	// restore cursor position; move right `cur_col` columns
	}

	// Move back down to after the icon/banner.
	printf("\x1B[%dE", rows);
	fflush(stdout);
}

void print_sixel_icon_banner(const RomDataPtr &romData)
{
	// Check if the terminal supports Kitty.
	if (gsvt_supports_kitty()) {
		// Use the Kitty graphics protocol.
		print_kitty_icon_banner_int(romData);
	} else if (gsvt_supports_sixel()) {
		// Use the Sixel graphics protocol.
		print_sixel_icon_banner_int(romData);
	}
}
