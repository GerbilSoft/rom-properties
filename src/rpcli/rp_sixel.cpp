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

void print_sixel_icon_banner(const RomDataPtr &romData)
{
	// Make sure the terminal supports sixel.
	if (!gsvt_supports_sixel()) {
		// Sixel is not supported.
		return;
	}

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
		const int icon_width = icon->width() + 8;
		int icon_cols = icon_width / cell_size_w;
		if (icon_width % cell_size_w != 0) {
			icon_cols++;
		}
		cur_col += icon_cols;
		printf("\x1B[u\x1B[%dC", cur_col);	// restore cursor position; move right `cur_col` columns
	}

	// Move back down to after the icon/banner.
	printf("\x1B[%dE", rows);
	fflush(stdout);

	SixelDll::psixel_output_destroy(output);
}
