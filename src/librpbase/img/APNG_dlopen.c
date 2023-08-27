/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * APNG_dlopen.c: APNG dlopen()'d function pointers.                       *
 *                                                                         *
 * Copyright (c) 2014-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"

#include <png.h>
#include "APNG_dlopen.h"

#if !defined(USE_INTERNAL_PNG) || defined(USE_INTERNAL_PNG_DLL)

// librpthreads
#include "librpthreads/Atomics.h"

#ifndef _WIN32
// Unix dlopen()
#  include <dlfcn.h>
#else
// Windows LoadLibrary()
#  include "libwin32common/RpWin32_sdk.h"
#  include "tcharx.h"
#  define dlsym(handle, symbol)	((void*)GetProcAddress(handle, symbol))
#  define dlclose(handle)	FreeLibrary(handle)
#endif

// DLL handle.
#ifdef _WIN32
static HMODULE libpng_dll = NULL;
#else /* _WIN32 */
static void *libpng_dll = NULL;
#endif

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

// Internal PNG (statically-linked) always has APNG.
// System PNG (or bundled DLL) might not.

/**
 * APNG reference couner.
 */
static volatile int APNG_ref_cnt = 0;

/**
 * Check if the PNG library supports APNG.
 * @return 0 if initialized; non-zero on error.
 */
static int init_apng(void)
{
#ifdef _WIN32
	BOOL bRet;
#endif /* _WIN32 */

	if (libpng_dll) {
		// APNG is already initialized.
		return 0;
	}

#define xstr(a) str(a)
#define str(a) #a

#ifdef _WIN32
	// Get the handle of the already-opened libpng.
	// Using GetModuleHandleEx() to increase the refcount.
	// NOTE: If libpng is set for delay-load, the caller *must*
	// ensure that it's loaded before calling this function!
	// Otherwise, this will fail.
#ifndef NDEBUG
	static const TCHAR libpng_dll_filename[] = _T("libpng") xstr(PNG_LIBPNG_VER_DLLNUM) _T("d.dll");
#else /* !NDEBUG */
	static const TCHAR libpng_dll_filename[] = _T("libpng") xstr(PNG_LIBPNG_VER_DLLNUM) _T(".dll");
#endif /* NDEBUG */
	bRet = GetModuleHandleEx(0, libpng_dll_filename, &libpng_dll);
	assert(bRet != FALSE);
	if (!bRet) {
		libpng_dll = NULL;
		return -1;
	}
#else /* !_WIN32 */
	// TODO: Get path of already-opened libpng?
	// TODO: On Linux, __USE_GNU and RTLD_DEFAULT.
	static const char libpng_so_filename[] = "libpng" xstr(PNG_LIBPNG_VER_SONUM) ".so";
	libpng_dll = dlopen(libpng_so_filename, RTLD_LOCAL|RTLD_NOW);
	if (!libpng_dll)
		return -1;
#endif

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
#endif /* !USE_INTERNAL_PNG || USE_INTERNAL_PNG_DLL */

/**
 * Load APNG and increment the reference counter.
 *
 * NOTE: On Windows, if libpng is set for delay-load, the caller
 * *must* ensure that it's loaded before calling this function!
 * Otherwise, this function will fail.
 *
 * @return 0 on success; non-zero on error.
 */
int RP_C_API APNG_ref(void)
{
#if !defined(USE_INTERNAL_PNG) || defined(USE_INTERNAL_PNG_DLL)
	// Internal PNG (statically-linked) always has APNG.
	// System PNG (or bundled DLL) might not.
	assert(APNG_ref_cnt >= 0);
	if (ATOMIC_INC_FETCH(&APNG_ref_cnt) == 1) {
		// First APNG reference.
		// Attempt to load APNG.
		if (init_apng() != 0) {
			// Error loading APNG.
			// NOTE: Not resetting the reference count here.
			// Caller must call APNG_unref() regardless of
			// initialization success or failure.
			return -1;
		}
	}
#endif /* !USE_INTERNAL_PNG || USE_INTERNAL_PNG_DLL */
	return 0;
}

/**
 * Decrement the APNG reference counter.
 * @return Non-zero if APNG is supported, 0 if not supported.
 */
void RP_C_API APNG_unref(void)
{
#if !defined(USE_INTERNAL_PNG) || defined(USE_INTERNAL_PNG_DLL)
	// Internal PNG (statically-linked) always has APNG.
	// System PNG (or bundled DLL) might not.
	assert(APNG_ref_cnt > 0);
	if (ATOMIC_DEC_FETCH(&APNG_ref_cnt) == 0) {
		// Unload APNG.
		// TODO: Clear the function pointers?
		if (libpng_dll) {
			dlclose(libpng_dll);
			libpng_dll = NULL;
		}
	}
#endif /* !USE_INTERNAL_PNG || USE_INTERNAL_PNG_DLL */
}

/**
 * Force the APNG library to be unloaded.
 * This decrements the reference count to 0.
 */
void APNG_force_unload(void)
{
#if !defined(USE_INTERNAL_PNG) || defined(USE_INTERNAL_PNG_DLL)
	// Internal PNG (statically-linked) always has APNG.
	// System PNG (or bundled DLL) might not.
	if (APNG_ref_cnt > 0) {
		// Unload APNG.
		// TODO: Clear the function pointers?
		if (libpng_dll) {
			dlclose(libpng_dll);
			libpng_dll = NULL;
		}
		APNG_ref_cnt = 0;
	}
#endif /* !USE_INTERNAL_PNG || USE_INTERNAL_PNG_DLL */
}
