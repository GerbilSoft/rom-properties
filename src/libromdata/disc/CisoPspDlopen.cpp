/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * CisoPspDlopen.cpp: PlayStation Portable CISO dlopen() handler.          *
 *                                                                         *
 * Copyright (c) 2016-2026 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "CisoPspDlopen.hpp"

/** LZO types required for __lzo_init_v2() **/
typedef uint32_t lzo_uint32_t;
typedef lzo_uint lzo_xint;

#ifdef _MSC_VER
#  define __LZO_CDECL __cdecl
#else /* !_MSC_VER */
#  define __LZO_CDECL
#endif /* _MSC_VER */

// Flat memory model
#define __LZO_MMODEL

#define lzo_sizeof_dict_t ((unsigned)sizeof(lzo_bytep))

/* Callback interface. Currently only the progress indicator ("nprogress")
 * is used, but this may change in a future release. */

struct lzo_callback_t;
typedef struct lzo_callback_t lzo_callback_t;
#define lzo_callback_p lzo_callback_t __LZO_MMODEL *

/* malloc & free function types */
typedef lzo_voidp (__LZO_CDECL *lzo_alloc_func_t)
    (lzo_callback_p self, lzo_uint items, lzo_uint size);
typedef void      (__LZO_CDECL *lzo_free_func_t)
    (lzo_callback_p self, lzo_voidp ptr);

/* a progress indicator callback function */
typedef void (__LZO_CDECL *lzo_progress_func_t)
    (lzo_callback_p, lzo_uint, lzo_uint, int);

struct lzo_callback_t
{
	/* custom allocators (set to 0 to disable) */
	lzo_alloc_func_t nalloc;                /* [not used right now] */
	lzo_free_func_t nfree;                  /* [not used right now] */

	/* a progress indicator callback function (set to 0 to disable) */
	lzo_progress_func_t nprogress;

	/* INFO: the first parameter "self" of the nalloc/nfree/nprogress
	* callbacks points back to this struct, so you are free to store
	* some extra info in the following variables. */
	lzo_voidp user1;
	lzo_xint user2;
	lzo_xint user3;
};

#ifdef _WIN32
// rp_LoadLibrary()
// NOTE: Delay-load is not supported with MinGW, but we still need
// access to the rp_LoadLibrary() function.
#  include "libwin32common/DelayLoadHelper.h"
#endif /* _MSC_VER */

namespace LibRomData {

CisoPspDlopen::CisoPspDlopen()
{
#ifdef LZ4_SHARED_LINKAGE
	m_liblz4 = nullptr;
	m_pfn_LZ4_decompress_safe = nullptr;
#endif /* LZ4_SHARED_LINKAGE */

#ifdef LZO_SHARED_LINKAGE
	m_liblzo2 = nullptr;
	m_pfn_lzo1x_decompress_safe = nullptr;
#elif defined(HAVE_LZO)
	m_lzoInit = false;
#endif /* LZO_SHARED_LINKAGE */
}

#ifdef LZ4_SHARED_LINKAGE
/**
 * Initialize the LZ4 function pointers.
 * (Internal version, called using std::call_once().)
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
	m_liblz4.reset(lib);
}

/**
 * Initialize the LZ4 function pointers.
 * @return 0 on success; negative POSIX error code on error.
 */
int CisoPspDlopen::init_pfn_LZ4(void)
{
	// TODO: Better error code.
	std::call_once(m_once_lz4, &CisoPspDlopen::init_pfn_LZ4_int, this);
	return ((bool)m_liblz4) ? 0 : -EIO;
}
#endif /* LZ4_SHARED_LINKAGE */

#ifdef LZO_SHARED_LINKAGE
/**
 * Initialize the LZO function pointers.
 * (Internal version, called using std::call_once().)
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
	// NOTE: __lzo_init_v2() is based on the version from LZO v2.10.
	typedef int (*pfn___lzo_init_v2_t)(unsigned,int,int,int,int,int,int,int,int,int);
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
	m_liblzo2.reset(lib);
}

/**
 * Initialize the LZO function pointers.
 * @return 0 on success; negative POSIX error code on error.
 */
int CisoPspDlopen::init_pfn_LZO(void)
{
	// TODO: Better error code.
	std::call_once(m_once_lzo, &CisoPspDlopen::init_pfn_LZO_int, this);
	return ((bool)m_liblzo2) ? 0 : -EIO;
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