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
	libsixel_dll.reset(dlopen("libsixel.so.1", RTLD_LOCAL | RTLD_NOW));
#endif /* _WIN32 */

	if (!libsixel_dll) {
		// Could not open libsixel.
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

/**
 * Encode 3 source bytes as 4 base64 bytes.
 * @param dest	[out] Destination buffer
 * @param src	[in] Source buffer
 */
static inline void encode_base64_3to4(char *dest, const uint8_t *src)
{
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

	const uint32_t data = (src[0] << 16) | (src[1] << 8) | src[2];
	dest[0] = base64_encoding_table[(data >> (3*6)) & 0x3F];
	dest[1] = base64_encoding_table[(data >> (2*6)) & 0x3F];
	dest[2] = base64_encoding_table[(data >> (1*6)) & 0x3F];
	dest[3] = base64_encoding_table[(data >> (0*6)) & 0x3F];
}

/**
 * Print an rp_image in base64 format for the Kitty graphics protocol.
 *
 * This function only prints the base64 data.
 * The appropriate Kitty control codes must have been sent prior to
 * calling this function.
 *
 * @param image_src	[in] Image source
 * @param is_animated	[in,opt] If true, this is part of an animated image.
 * @param anim_frame_0	[in,opt] Set to true for the first frame of an animated image.
 * @param image_number	[in,opt] Set to the application-specific Kitty image number.
 * @param ms		[in,opt] Animation delay, in milliseconds.
 * @return 0 on success; negative POSIX error code on error.
 */
static int print_kitty_image(const rp_image *image_src, bool is_animated = false, bool anim_frame_0 = false, int image_number = 0, int ms = 0)
{
	if (!image_src->isValid()) {
		return -EINVAL;
	}

	// Kitty requires R/B to be swapped.
	rp_image_ptr image = image_src->dup_ARGB32();
	if (!image->isValid()) {
		// Failed to dup() and/or convert to ARGB32...
		return -EINVAL;
	}

	// Swap the R/B channels.
	image->swizzle("bgra");

	const int width = image->width();
	const int height = image->height();

	// FIXME: Chunked operation (m=1) isn't working.
	// Kitty says it's needed for escape sequences larger than 4 KB,
	// but it seems to be working regardless...

	// Print the image header.
	if (likely(!is_animated)) {
		printf("\x1B_Ga=T,q=2,f=32,s=%d,v=%d,m=1;", width, height);
	} else {
		printf("\x1B_Ga=%c,q=2,f=32,s=%d,v=%d,I=%d,z=%d;" /*,m=1;"*/,
			(anim_frame_0 ? 'T' : 'f'),
			width, height, image_number, ms);
	}

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
	const uint8_t *bits = static_cast<const uint8_t*>(image->bits());
	const size_t row_bytes = static_cast<size_t>(image->row_bytes());
	const size_t stride_adj = (static_cast<size_t>(image->stride()) - row_bytes);

	// Temporary buffer for source lines that aren't multiples of 3 bytes.
	uint8_t tmpbuf[3];
	uint8_t tmpbuf_size = 0;
	int chunk_number = 0;	// FIXME: Not working for animated images.

	for (int y = height; y > 0; y--, bits += stride_adj) {
		// Process 3 input bytes at a time.
		size_t n = row_bytes;
		char *p = linebuf.get();

		if (tmpbuf_size > 0) {
			// Need to encode bytes from the end of the previous line.
			for (; tmpbuf_size < 3; tmpbuf_size++, bits++, n--) {
				tmpbuf[tmpbuf_size] = *bits;
			}

			encode_base64_3to4(p, tmpbuf);
			p += 4;
			tmpbuf_size = 0;
		}

		// Encode the line.
		for (; n >= 3; n -= 3, bits += 3, p += 4) {
			encode_base64_3to4(p, bits);
		}

		if (n > 0) {
			// Need to encode the remaining bytes at the beginning of the next line.
			memcpy(tmpbuf, bits, n);
			tmpbuf_size = static_cast<uint8_t>(n);
			bits += n;
		}

		// Write the encoded data in 4 KB chunks.
		static constexpr size_t BASE64_CHUNK_SIZE = 4096U - 32U;
		size_t linebuf_used = static_cast<size_t>(p - linebuf.get());
		p = linebuf.get();
		while (linebuf_used > 0) {
			size_t bytes_to_print = std::min(linebuf_used, BASE64_CHUNK_SIZE);
			if (likely(!is_animated) && chunk_number > 0) {
				// First chunk is preceded by an escape sequence indicating
				// we're starting pixel data. Remaining chunks need an escape
				// sequence indicating we're continuing to print.
				const int final_chunk = !(y == 1 && linebuf_used <= BASE64_CHUNK_SIZE && tmpbuf_size == 0);
				printf("\x1B_Gq=2,m=%d;", final_chunk);
			}

			fwrite(p, 1, bytes_to_print, stdout);

			if (likely(!is_animated)) {
				fwrite("\x1B\\", 1, 2, stdout);
				chunk_number++;
			}

			p += bytes_to_print;
			linebuf_used -= bytes_to_print;
		}
	}

	if (tmpbuf_size > 0) {
		// Encode the remaining bytes using equal signs to indicate
		// that some bytes are missing.
		for (uint8_t n = tmpbuf_size; n < 3; n++) {
			tmpbuf[n] = 0;
		}

		char *p = linebuf.get();
		encode_base64_3to4(p, tmpbuf);
		p[4] = '\0';

		// Number of bytes to overwrite depends on number of source bytes remaining:
		// - n = 1: overwrite 2, 3
		// - n = 2: overwrite    3
		switch (tmpbuf_size) {
			default:
				break;
			case 1:
				p[2] = '=';
				// fall-through
			case 2:
				p[3] = '=';
				break;
		}

		// Write the last 4 bytes.
		if (likely(!is_animated)) {
			printf("\x1B_Gq=2,m=0;%s\x1B\\", p);
			chunk_number += 2;
		} else {
			fwrite(p, 1, 4, stdout);
		}
	}

	if (likely(!is_animated)) {
		// No chunks, or only one chunk (with m=1), were written.
		// Need to write a dummy final chunk.
		printf("\x1B_Gq=2,m=0;\x1B\\");
	} else if (chunk_number <= 1) {
		fwrite("\x1B\\", 1, 2, stdout);
	}

	return 0;
}

/**
 * Print an rp_image in base64 format for the Kitty graphics protocol.
 *
 * This function only prints the base64 data.
 * The appropriate Kitty control codes must have been sent prior to
 * calling this function.
 *
 * @param image_src	[in] Image source
 * @param is_animated	[in,opt] If true, this is part of an animated image.
 * @param anim_frame_0	[in,opt] Set to true for the first frame of an animated image.
 * @param image_number	[in,opt] Set to the application-specific Kitty image number.
 * @param ms		[in,opt] Animation delay, in milliseconds.
 * @return 0 on success; negative POSIX error code on error.
 */
static inline int print_kitty_image(const rp_image_const_ptr &image_src, bool is_animated = false, bool anim_frame_0 = false, int image_number = 0, int ms = 0)
{
	return print_kitty_image(image_src.get(), is_animated, anim_frame_0, image_number, ms);
}

/**
 * Print an animated image using the Kitty graphics protocol.
 * @param iconAnimData IconAnimData
 * @return 0 on success; negative POSIX error code on error.
 */
static int print_kitty_animated_image(const IconAnimDataConstPtr &iconAnimData)
{
	// Print all of the animated icon frames.
	// Using image numbers, which are unique per animation but not per session.
	// NOTE: The frame grap is specified per frame, not as a separate sequence,
	// so we have to go by the sequence. This may result in frames being sent
	// multiple times.

	// Last image number. ("I=?")
	// NOTE: Using image number instead of image ID to prevent conflicts
	// with other programs that may have printed images.
	// NOTE 2: Must start at 1. Image number 0 won't animate for some reason.
	static int image_number = 1;

	bool first = true;
	for (int i = 0; i < iconAnimData->seq_count; i++) {
		const rp_image *const this_frame = iconAnimData->frames[iconAnimData->seq_index[i]].get();
		if (!this_frame || !this_frame->isValid()) {
			// Empty frame.
			continue;
		}

		int ms = iconAnimData->delays[i].ms;

		// Check if the following frames have NULL images.
		for (int j = i + 1; j < iconAnimData->seq_count; j++) {
			const rp_image *next_frame = iconAnimData->frames[iconAnimData->seq_index[j]].get();
			if (next_frame) {
				// Next frame is OK.
				break;
			}

			// Found a NULL frame. Add its delay to the current frame.
			ms += iconAnimData->delays[j].ms;
			// Skip this frame.
			i++;
		}

		print_kitty_image(this_frame, true, first, image_number, ms);
		first = false;
	}

	// Start the animation.
	printf("\x1B_Ga=a,I=%d,s=3,v=1\x1B\\", image_number);

	// Increment the image number for the next invocation.
	image_number++;
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
		// Do we have an animated icon?
		bool ok = false;
		if (romData->imgpf(RomData::IMG_INT_ICON) & RomData::IMGPF_ICON_ANIMATED) {
			// Try to load the animated icon.
			IconAnimDataConstPtr iconAnimData = romData->iconAnimData();
			if (iconAnimData) {
				print_kitty_animated_image(iconAnimData);
				ok = true;
			}
		}
		if (!ok) {
			// No animated icon. Print the regular icon.
			print_kitty_image(icon);
		}
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
