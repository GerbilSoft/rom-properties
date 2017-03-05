/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * APNG_dlopen.hpp: APNG dlopen()'d function pointers.                     *
 *                                                                         *
 * Copyright (c) 2014-2017 by David Korth.                                 *
 *                                                                         *
 * This program is free software; you can redistribute it and/or modify it *
 * under the terms of the GNU General Public License as published by the   *
 * Free Software Foundation; either version 2 of the License, or (at your  *
 * option) any later version.                                              *
 *                                                                         *
 * This program is distributed in the hope that it will be useful, but     *
 * WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 * GNU General Public License for more details.                            *
 *                                                                         *
 * You should have received a copy of the GNU General Public License along *
 * with this program; if not, write to the Free Software Foundation, Inc., *
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.           *
 ***************************************************************************/

#include "config.libromdata.h"
#include <png.h>

#include "APNG_dlopen.h"

// C includes.
#include <assert.h>
#include <stdio.h>

#ifndef _WIN32
// Unix dlopen()
#include <dlfcn.h>
#else
// Windows LoadLibrary()
#include <windows.h>
#define dlopen(filename, flag)	LoadLibraryA(filename)
#define dlsym(handle, symbol)	((void*)GetProcAddress(handle, symbol))
#define dlclose(handle)		FreeLibrary(handle)
#endif

// DLL handle.
static void *libpng_dll = NULL;

// APNG function pointers.
APNG_png_get_acTL_t APNG_png_get_acTL = NULL;
APNG_png_set_acTL_t APNG_png_set_acTL = NULL;
APNG_png_get_num_frames_t APNG_png_get_num_frames = NULL;
APNG_png_get_num_plays_t APNG_png_get_num_plays = NULL;
APNG_png_get_next_frame_fcTL_t APNG_png_get_next_frame_fcTL = NULL;
APNG_png_set_next_frame_fcTL_t APNG_png_set_next_frame_fcTL = NULL;
APNG_png_get_next_frame_width_t APNG_png_get_next_frame_width = NULL;
APNG_png_get_next_frame_height_t APNG_png_get_next_frame_height = NULL;
APNG_png_get_next_frame_x_offset_t APNG_png_get_next_frame_x_offset = NULL;
APNG_png_get_next_frame_y_offset_t APNG_png_get_next_frame_y_offset = NULL;
APNG_png_get_next_frame_delay_num_t APNG_png_get_next_frame_delay_num = NULL;
APNG_png_get_next_frame_delay_den_t APNG_png_get_next_frame_delay_den = NULL;
APNG_png_get_next_frame_dispose_op_t APNG_png_get_next_frame_dispose_op = NULL;
APNG_png_get_next_frame_blend_op_t APNG_png_get_next_frame_blend_op = NULL;
APNG_png_get_first_frame_is_hidden_t APNG_png_get_first_frame_is_hidden = NULL;
APNG_png_set_first_frame_is_hidden_t APNG_png_set_first_frame_is_hidden = NULL;
APNG_png_read_frame_head_t APNG_png_read_frame_head = NULL;
APNG_png_set_progressive_frame_fn_t APNG_png_set_progressive_frame_fn = NULL;
APNG_png_write_frame_head_t APNG_png_write_frame_head = NULL;
APNG_png_write_frame_tail_t APNG_png_write_frame_tail = NULL;

/**
 * APNG reference couner.
 */
static int ref_cnt = 0;

/**
 * Check if the PNG library supports APNG.
 * @return 0 if initialized; non-zero on error.
 */
static int init_apng(void)
{
	// External PNG library.
	char png_dll_filename[32];
#ifdef _WIN32
	// The Windows build uses a debug suffix for libpng.
#ifndef NDEBUG
	snprintf(png_dll_filename, sizeof(png_dll_filename),
		 "libpng%dd.dll", PNG_LIBPNG_VER_DLLNUM);
#else
	snprintf(png_dll_filename, sizeof(png_dll_filename),
		 "libpng%d.dll", PNG_LIBPNG_VER_DLLNUM);
#endif
#else /* !_WIN32 */
	// TODO: dylib on Mac OS X?
	snprintf(png_dll_filename, sizeof(png_dll_filename),
		 "libpng%d.so", PNG_LIBPNG_VER_DLLNUM);
#endif

	// TODO: Get path of already-opened libpng?
	// TODO: On Linux, __USE_GNU and RTLD_DEFAULT.
	libpng_dll = dlopen(png_dll_filename, RTLD_LOCAL|RTLD_NOW);
	if (!libpng_dll)
		return -1;

	// Check for APNG support.
	APNG_png_get_acTL = dlsym(libpng_dll, "png_get_acTL");
	APNG_png_set_acTL = dlsym(libpng_dll, "png_set_acTL");
	if (!APNG_png_get_acTL || !APNG_png_set_acTL) {
		// APNG support not found.
		APNG_png_get_acTL = NULL;
		APNG_png_set_acTL = NULL;
		dlclose(libpng_dll);
		libpng_dll = NULL;
		return -2;
	}

	// Load the rest of the symbols.
	// TODO: Check the rest for NULL as well?
	APNG_png_get_num_frames = dlsym(libpng_dll, "png_get_num_frames");
	APNG_png_get_num_plays = dlsym(libpng_dll, "png_get_num_plays");
	APNG_png_get_next_frame_fcTL = dlsym(libpng_dll, "png_get_next_frame_fcTL");
	APNG_png_set_next_frame_fcTL = dlsym(libpng_dll, "png_set_next_frame_fcTL");
	APNG_png_get_next_frame_width = dlsym(libpng_dll, "png_get_next_frame_width");
	APNG_png_get_next_frame_height = dlsym(libpng_dll, "png_get_next_frame_height");
	APNG_png_get_next_frame_x_offset = dlsym(libpng_dll, "png_get_next_frame_x_offset");
	APNG_png_get_next_frame_y_offset = dlsym(libpng_dll, "png_get_next_frame_y_offset");
	APNG_png_get_next_frame_delay_num = dlsym(libpng_dll, "png_get_next_frame_delay_num");
	APNG_png_get_next_frame_delay_den = dlsym(libpng_dll, "png_get_next_frame_delay_den");
	APNG_png_get_next_frame_dispose_op = dlsym(libpng_dll, "png_get_next_frame_dispose_op");
	APNG_png_get_next_frame_blend_op = dlsym(libpng_dll, "png_get_next_frame_blend_op");
	APNG_png_get_first_frame_is_hidden = dlsym(libpng_dll, "png_get_first_frame_is_hidden");
	APNG_png_set_first_frame_is_hidden = dlsym(libpng_dll, "png_set_first_frame_is_hidden");
	APNG_png_read_frame_head = dlsym(libpng_dll, "png_read_frame_head");
	APNG_png_set_progressive_frame_fn = dlsym(libpng_dll, "png_set_progressive_frame_fn");
	APNG_png_write_frame_head = dlsym(libpng_dll, "png_write_frame_head");
	APNG_png_write_frame_tail = dlsym(libpng_dll, "png_write_frame_tail");
	return 0;
}

/**
 * Load APNG and increment the reference counter.
 * @return 0 on success; non-zero on error.
 */
int APNG_ref(void)
{
	assert(ref_cnt >= 0);
	if (ref_cnt <= 0) {
		// Attempt to load APNG.
		if (init_apng() != 0) {
			// Error loading APNG.
			return -1;
		}
		if (ref_cnt < 0) {
			// Invalid reference count...
			ref_cnt = 0;
		}
	}

	// Increment the reference counter.
	// TODO: Atomic increment?
	ref_cnt++;
	return 0;
}

/**
 * Decrement the APNG reference counter.
 * @return Non-zero if APNG is supported, 0 if not supported.
 */
void APNG_unref(void)
{
	assert(ref_cnt > 0);
	if (ref_cnt <= 0)
		return;

	if (--ref_cnt == 0) {
		// Unload APNG.
		// TODO: Clear the function pointers?
		dlclose(libpng_dll);
		libpng_dll = NULL;
	}
}

/**
 * Force the APNG library to be unloaded.
 * This decrements the reference count to 0.
 */
void APNG_force_unload(void)
{
	if (ref_cnt > 0) {
		// Unload APNG.
		// TODO: Clear the function pointers?
		dlclose(libpng_dll);
		libpng_dll = NULL;
		ref_cnt = 0;
	}
}
