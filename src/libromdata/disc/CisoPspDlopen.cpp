/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * CisoPspDlopen.cpp: PlayStation Portable CISO dlopen() handler.          *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "CisoPspDlopen.hpp"

#include "config.librpthreads.h"
#include "librpthreads/Atomics.h"
#ifdef _WIN32
#  include "libwin32common/RpWin32_sdk.h"
#  define sched_yield() SwitchToThread()
static constexpr pthread_once_t PTHREAD_ONCE_INIT = 0;
#endif

#ifdef _MSC_VER
// MSVC: rp_LoadLibrary()
#  include "libwin32common/DelayLoadHelper.h"
#endif /* _MSC_VER */

namespace LibRomData {

CisoPspDlopen::CisoPspDlopen()
{
#ifdef LZ4_SHARED_LINKAGE
	m_once_lz4 = PTHREAD_ONCE_INIT;
	m_liblz4 = nullptr;
	m_pfn_LZ4_decompress_safe = nullptr;
#endif /* LZ4_SHARED_LINKAGE */

#ifdef LZO_SHARED_LINKAGE
	m_once_lzo = PTHREAD_ONCE_INIT;
	m_liblzo2 = nullptr;
	m_pfn_lzo1x_decompress_safe = nullptr;
#elif defined(HAVE_LZO)
	m_lzoInit = false;
#endif /* LZO_SHARED_LINKAGE */
}

CisoPspDlopen::~CisoPspDlopen()
{
#ifdef LZ4_SHARED_LINKAGE
	if (m_liblz4) {
		dlclose(m_liblz4);
	}
#endif /* LZ4_SHARED_LINKAGE */
#ifdef LZO_SHARED_LINKAGE
	if (m_liblzo2) {
		dlclose(m_liblzo2);
	}
#endif /* LZO_SHARED_LINKAGE */
}

// NOTE: Cannot use pthread_once() with a member function,
// so we'll reimplement pthread_once() here.
#define pthread_once_inl(once_control, init_routine) do { \
	if (*(once_control) != 1) { \
		while (1) { \
			/* Check if once_control is 0. If it is, set it to 2. */ \
			/* NOTE: ATOMIC_CMPXCHG() returns the initial value,  */ \
			/* so it will return 0 if once_control was 0, though  */ \
			/* once_control will now be 2.                        */ \
			switch (ATOMIC_CMPXCHG(once_control, 0, 2)) { \
				case 0: \
					/* NOTE: pthread_once() doesn't have a way to */ \
					/* indicate that initialization failed.       */ \
					init_routine(); \
					ATOMIC_EXCHANGE(once_control, 1); \
					return 0; \
				case 1: \
					/* The initializer has already been executed. */ \
					return 0; \
				default: \
					/* The initializer is being processed by another thread. */ \
					sched_yield(); \
					break; \
			} \
		} \
	} \
} while (0)

#ifdef LZ4_SHARED_LINKAGE
/**
 * Initialize the LZ4 function pointers.
 * (Internal version, called using pthread_once().)
 */
void CisoPspDlopen::init_pfn_LZ4_int(void)
{
#ifdef _WIN32
#  ifndef NDEBUG
#    define LZ4_DLL_FILENAME "lz4d.dll"
#  else /* NDEBUG */
#    define LZ4_DLL_FILENAME "lz4.dll"
#  endif /* NDEBUG */
	HMODULE lib = rp_LoadLibrary(LZ4_DLL_FILENAME);
#else /* !_WIN32 */
	HMODULE lib = dlopen("liblz4.so.1", RTLD_LOCAL|RTLD_NOW);
#endif /* _WIN32 */
	if (!lib) {
		// NOTE: dlopen() does not set errno, but it does have dlerror().
		return;	// TODO: Better error code.
	}

	// Attempt to load the function pointers.
	m_pfn_LZ4_decompress_safe = reinterpret_cast<pfn_LZ4_decompress_safe_t>(dlsym(lib, "LZ4_decompress_safe"));
	if (!m_pfn_LZ4_decompress_safe) {
		// Failed to load the function pointers.
		dlclose(lib);
		return;	// TODO: Better error code.
	}

	// Function pointers loaded.
	m_liblz4 = lib;
}

/**
 * Initialize the LZ4 function pointers.
 * @return 0 on success; negative POSIX error code on error.
 */
int CisoPspDlopen::init_pfn_LZ4(void)
{
	// TODO: Better error code.
	pthread_once_inl(&m_once_lz4, init_pfn_LZ4_int);
	return (m_liblz4 != nullptr) ? 0 : -EIO;
}
#endif /* LZ4_SHARED_LINKAGE */

#ifdef LZO_SHARED_LINKAGE
/**
 * Initialize the LZ4 function pointers.
 * (Internal version, called using pthread_once().)
 */
void CisoPspDlopen::init_pfn_LZO_int(void)
{
#ifdef _WIN32
#  ifndef NDEBUG
#    define LZO_DLL_FILENAME "minilzod.dll"
#  else /* NDEBUG */
#    define LZO_DLL_FILENAME "minilzo.dll"
#  endif /* NDEBUG */
	HMODULE lib = rp_LoadLibrary(LZO_DLL_FILENAME);
#else /* !_WIN32 */
	HMODULE lib = dlopen("liblzo2.so.2", RTLD_LOCAL|RTLD_NOW);
#endif /* _WIN32 */
	if (!lib) {
		// NOTE: dlopen() does not set errno, but it does have dlerror().
		return;
	}

	// Load the __lzo_init_v2 function pointer first and initialize LZO.
	typedef __typeof__(__lzo_init_v2) *pfn___lzo_init_v2_t;
	pfn___lzo_init_v2_t pfn___lzo_init_v2 = reinterpret_cast<pfn___lzo_init_v2_t>(dlsym(lib, "__lzo_init_v2"));
	if (!pfn___lzo_init_v2) {
		// Failed to load the LZO initialization function pointer.
		dlclose(lib);
		return;
	}

	// Initialize the LZO library.
	// Based on the lzo_init() macro from lzoconf.h.
	int ret = pfn___lzo_init_v2(LZO_VERSION, (int)sizeof(short), (int)sizeof(int),
		(int)sizeof(long), (int)sizeof(lzo_uint32_t), (int)sizeof(lzo_uint),
		(int)lzo_sizeof_dict_t, (int)sizeof(char *), (int)sizeof(lzo_voidp),
		(int)sizeof(lzo_callback_t));
	if (ret != LZO_E_OK) {
		// Failed to initialize LZO.
		dlclose(lib);
		return;
	}

	// Attempt to load the remaining function pointers.
	m_pfn_lzo1x_decompress_safe = reinterpret_cast<pfn_lzo1x_decompress_safe_t>(dlsym(lib, "lzo1x_decompress_safe"));
	if (!m_pfn_lzo1x_decompress_safe) {
		// Failed to load the function pointers.
		dlclose(lib);
		return;
	}

	// Function pointers loaded.
	m_liblzo2 = lib;
}

/**
 * Initialize the LZO function pointers.
 * @return 0 on success; negative POSIX error code on error.
 */
int CisoPspDlopen::init_pfn_LZO(void)
{
	// TODO: Better error code.
	pthread_once_inl(&m_once_lzo, init_pfn_LZO_int);
	return (m_liblzo2 != nullptr) ? 0 : -EIO;
}
#elif defined(HAVE_LZO)
/**
 * Initialize the LZO function pointers.
 * @return 0 on success; negative POSIX error code on error.
 */
int CisoPspDlopen::init_pfn_LZO(void)
{
	// Need to call lzo_init(), even in static library builds.
	if (m_lzoInit) {
		return LZO_E_OK;
	}

	int ret = lzo_init();
	m_lzoInit = (ret == LZO_E_OK);
	return ret;
}
#endif /* LZO_SHARED_LINKAGE */

} // namespace LibRomData