/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * APNG_dlopen.cpp: APNG dlopen()'d function pointers.                     *
 *                                                                         *
 * Copyright (c) 2014-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"

#include <png.h>
#include "APNG_dlopen.hpp"

#if !defined(USE_INTERNAL_PNG) || defined(USE_INTERNAL_PNG_DLL)

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

// C++ STL classes
#include <mutex>
using std::unique_ptr;

// DLL handle.
#ifndef _WIN32
typedef void *HMODULE;
#endif

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
static unique_ptr<HMODULE, HMODULE_deleter> libpng_dll;
static std::once_flag apng_once_flag;

// APNG function pointers.
APNG_png_get_acTL_t APNG_png_get_acTL = nullptr;
APNG_png_set_acTL_t APNG_png_set_acTL = nullptr;
APNG_png_get_num_frames_t APNG_png_get_num_frames = nullptr;
APNG_png_get_num_plays_t APNG_png_get_num_plays = nullptr;
APNG_png_get_next_frame_fcTL_t APNG_png_get_next_frame_fcTL = nullptr;
APNG_png_set_next_frame_fcTL_t APNG_png_set_next_frame_fcTL = nullptr;
APNG_png_get_next_frame_width_t APNG_png_get_next_frame_width = nullptr;
APNG_png_get_next_frame_height_t APNG_png_get_next_frame_height = nullptr;
APNG_png_get_next_frame_x_offset_t APNG_png_get_next_frame_x_offset = nullptr;
APNG_png_get_next_frame_y_offset_t APNG_png_get_next_frame_y_offset = nullptr;
APNG_png_get_next_frame_delay_num_t APNG_png_get_next_frame_delay_num = nullptr;
APNG_png_get_next_frame_delay_den_t APNG_png_get_next_frame_delay_den = nullptr;
APNG_png_get_next_frame_dispose_op_t APNG_png_get_next_frame_dispose_op = nullptr;
APNG_png_get_next_frame_blend_op_t APNG_png_get_next_frame_blend_op = nullptr;
APNG_png_get_first_frame_is_hidden_t APNG_png_get_first_frame_is_hidden = nullptr;
APNG_png_set_first_frame_is_hidden_t APNG_png_set_first_frame_is_hidden = nullptr;
APNG_png_read_frame_head_t APNG_png_read_frame_head = nullptr;
APNG_png_set_progressive_frame_fn_t APNG_png_set_progressive_frame_fn = nullptr;
APNG_png_write_frame_head_t APNG_png_write_frame_head = nullptr;
APNG_png_write_frame_tail_t APNG_png_write_frame_tail = nullptr;

// Internal PNG (statically-linked) always has APNG.
// System PNG (or bundled DLL) might not.

/**
 * Check if the PNG library supports APNG.
 * Called by std::call_once().
 *
 * Check if libpng_dll is nullptr afterwards.
 */
static void init_apng(void)
{
#ifdef _WIN32
	BOOL bRet;
#endif /* _WIN32 */

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
	HMODULE hTmp = nullptr;
	bRet = GetModuleHandleEx(0, libpng_dll_filename, &hTmp);
	assert(bRet != FALSE);
	if (!bRet || !hTmp) {
		return;
	}
	libpng_dll.reset(hTmp);
#else /* !_WIN32 */
	// TODO: Get path of already-opened libpng?
	// TODO: On Linux, __USE_GNU and RTLD_DEFAULT.
	static const char libpng_so_filename[] = "libpng" xstr(PNG_LIBPNG_VER_SONUM) ".so";
	libpng_dll.reset(dlopen(libpng_so_filename, RTLD_LOCAL|RTLD_NOW));
	if (!libpng_dll) {
		return;
	}
#endif

	// Check for APNG support.
#define DLSYM(sym) APNG_##sym = reinterpret_cast<__typeof__(APNG_##sym)>(dlsym(libpng_dll.get(), #sym))
	DLSYM(png_get_acTL);
	DLSYM(png_set_acTL);
	if (!APNG_png_get_acTL || !APNG_png_set_acTL) {
		// APNG support not found.
		APNG_png_get_acTL = nullptr;
		APNG_png_set_acTL = nullptr;
		libpng_dll.reset();
		return;
	}

	// Load the rest of the symbols.
	// TODO: Check the rest for nullptr as well?
	DLSYM(png_get_num_frames);
	DLSYM(png_get_num_plays);
	DLSYM(png_get_next_frame_fcTL);
	DLSYM(png_set_next_frame_fcTL);
	DLSYM(png_get_next_frame_width);
	DLSYM(png_get_next_frame_height);
	DLSYM(png_get_next_frame_x_offset);
	DLSYM(png_get_next_frame_y_offset);
	DLSYM(png_get_next_frame_delay_num);
	DLSYM(png_get_next_frame_delay_den);
	DLSYM(png_get_next_frame_dispose_op);
	DLSYM(png_get_next_frame_blend_op);
	DLSYM(png_get_first_frame_is_hidden);
	DLSYM(png_set_first_frame_is_hidden);
	DLSYM(png_read_frame_head);
	DLSYM(png_set_progressive_frame_fn);
	DLSYM(png_write_frame_head);
	DLSYM(png_write_frame_tail);
}
#endif /* !USE_INTERNAL_PNG || USE_INTERNAL_PNG_DLL */

/**
 * Ensure APNG symbols are loaded.
 *
 * NOTE: On Windows, if libpng is set for delay-load, the caller
 * *must* ensure that it's loaded before calling this function!
 * Otherwise, this function will fail.
 *
 * NOTE 2: APNG is automatically unloaded when this library is unloaded.
 *
 * @return 0 on success; non-zero on error.
 */
int RP_C_API APNG_load(void)
{
#if !defined(USE_INTERNAL_PNG) || defined(USE_INTERNAL_PNG_DLL)
	// Internal PNG (statically-linked) always has APNG.
	// System PNG (or bundled DLL) might not.
	std::call_once(apng_once_flag, init_apng);
	return ((bool)libpng_dll) ? 0 : -1;
#endif /* !USE_INTERNAL_PNG || USE_INTERNAL_PNG_DLL */
	return 0;
}
