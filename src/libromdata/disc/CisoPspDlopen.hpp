/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * CisoPspDlopen.cpp: PlayStation Portable CISO dlopen() handler.          *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "config.libromdata.h"

#ifdef _WIN32
#  include "libwin32common/RpWin32_sdk.h"
#  define dlsym(handle, symbol)	GetProcAddress((handle), (symbol))
#  define dlclose(handle)	FreeLibrary(handle)
typedef int pthread_once_t;
#else /* !_WIN32 */
#  include <dlfcn.h>	// for dlopen()
typedef void *HMODULE;
#endif /* !_WIN32 */

#ifdef HAVE_LZ4
#  if defined(USE_INTERNAL_LZ4) && !defined(USE_INTERNAL_LZ4_DLL)
     // Using a statically linked copy of LZ4.
#    define LZ4_DIRECT_LINKAGE 1
#  else /* !(USE_INTERNAL_LZ4 && USE_INTERNAL_LZ4_DLL) */
     // Using a shared library copy of LZ4.
#    define LZ4_SHARED_LINKAGE 1
#  endif /* USE_INTERNAL_LZ4 && USE_INTERNAL_LZ4_DLL */
#  include <lz4.h>
#endif /* HAVE_LZ4 */

// LZO (JISO)
// NOTE: The bundled version is MiniLZO.
#ifdef HAVE_LZO
#  if defined(USE_INTERNAL_LZO) && !defined(USE_INTERNAL_LZO_DLL)
     // Using a statically linked copy of LZO.
#    define LZO_DIRECT_LINKAGE 1
#  else /* !(USE_INTERNAL_LZO && USE_INTERNAL_LZO_DLL) */
     // Using a shared library copy of LZO.
#    define LZO_SHARED_LINKAGE 1
#  endif /* USE_INTERNAL_LZO && USE_INTERNAL_LZO_DLL */
#  ifdef USE_INTERNAL_LZO
#    include "minilzo.h"
#  else
#    include <lzo/lzo1x.h>
#  endif
#else /* !HAVE_LZO */
typedef uint8_t *lzo_bytep;
typedef size_t lzo_uint, *lzo_uintp;
typedef void *lzo_voidp;
#  define LZO_E_OK    0
#  define LZO_E_ERROR (-1)
#endif /* HAVE_LZO */

namespace LibRomData {

class CisoPspDlopen {
public:
	CisoPspDlopen();
	~CisoPspDlopen();

private:
	// dlopen()'d modules

#ifdef LZ4_SHARED_LINKAGE
	pthread_once_t m_once_lz4;
#endif /* LZ4_SHARED_LINKAGE */
#ifdef LZO_SHARED_LINKAGE
	pthread_once_t m_once_lzo;
#endif /* LZO_SHARED_LINKAGE */

#ifdef LZ4_SHARED_LINKAGE
	HMODULE m_liblz4;

	typedef __typeof__(::LZ4_decompress_safe) *pfn_LZ4_decompress_safe_t;
	pfn_LZ4_decompress_safe_t m_pfn_LZ4_decompress_safe;
#endif /* LZ4_SHARED_LINKAGE */

#ifdef LZO_SHARED_LINKAGE
	HMODULE m_liblzo2;

	typedef __typeof__(::lzo1x_decompress_safe) *pfn_lzo1x_decompress_safe_t;
	pfn_lzo1x_decompress_safe_t m_pfn_lzo1x_decompress_safe;
#elif defined(HAVE_LZO)
	bool m_lzoInit;
#endif /* LZO_SHARED_LINKAGE */

#ifdef LZ4_SHARED_LINKAGE
private:
	/**
	 * Initialize the LZ4 function pointers.
	 * (Internal version, called using pthread_once().)
	 */
	void init_pfn_LZ4_int(void);

public:
	/**
	 * Initialize the LZ4 function pointers.
	 * @return 0 on success; negative POSIX error code on error.
	 */
	int init_pfn_LZ4(void);

	/**
	 * Are the LZ4 function pointers loaded?
	 * @return True if loaded; false if not.
	 */
	inline bool is_LZ4_loaded(void) const
	{
		return (m_liblz4 != nullptr);
	}

	inline int LZ4_decompress_safe(const char* src, char* dst, int compressedSize, int dstCapacity)
	{
		return m_pfn_LZ4_decompress_safe(src, dst, compressedSize, dstCapacity);
	}
#else /* LZ4_SHARED_LINKAGE */
public:
	/**
	 * Initialize the LZ4 function pointers.
	 * @return 0 on success; negative POSIX error code on error.
	 */
	static inline int init_pfn_LZ4(void)
	{
#ifdef HAVE_LZ4
		return 0;
#else /* !HAVE_LZ4 */
		return -ENOTSUP;
#endif /* HAVE_LZ4 */
	}

	/**
	 * Are the LZ4 function pointers loaded?
	 * @return True if loaded; false if not.
	 */
	static inline bool is_LZ4_loaded(void)
	{
#ifdef HAVE_LZ4
		return true;
#else /* !HAVE_LZ4 */
		return false;
#endif /* HAVE_LZ4 */
	}

	static inline int LZ4_decompress_safe(const char* src, char* dst, int compressedSize, int dstCapacity)
	{
#ifdef HAVE_LZ4
		return ::LZ4_decompress_safe(src, dst, compressedSize, dstCapacity);
#else /* !HAVE_LZ4 */
		RP_UNUSED(src);
		RP_UNUSED(dst);
		RP_UNUSED(compressedSize);
		RP_UNUSED(dstCapacity);
		return -1;
#endif /* HAVE_LZ4 */
	}
#endif /* LZ4_SHARED_LINKAGE */

#ifdef LZO_SHARED_LINKAGE
private:
	/**
	 * Initialize the LZO function pointers.
	 * (Internal version, called using pthread_once().)
	 */
	void init_pfn_LZO_int(void);

public:
	/**
	 * Initialize the LZO function pointers.
	 * @return 0 on success; negative POSIX error code on error.
	 */
	int init_pfn_LZO(void);

	/**
	 * Are the LZ4 function pointers loaded?
	 * @return True if loaded; false if not.
	 */
	inline bool is_LZO_loaded(void) const
	{
		return (m_liblzo2 != nullptr);
	}

	inline int lzo1x_decompress_safe(const lzo_bytep src, lzo_uint  src_len,
	                                       lzo_bytep dst, lzo_uintp dst_len,
	                                       lzo_voidp wrkmem /* NOT USED */)
	{
		return m_pfn_lzo1x_decompress_safe(src, src_len, dst, dst_len, wrkmem);
	}
#else /* !LZO_SHARED_LINKAGE */
public:
	/**
	 * Initialize the LZO function pointers.
	 * @return 0 on success; negative POSIX error code on error.
	 */
	int init_pfn_LZO(void);

	/**
	 * Are the LZO function pointers loaded?
	 * @return True if loaded; false if not.
	 */
#ifdef HAVE_LZO
	inline bool is_LZO_loaded(void) const
	{
		return m_lzoInit;
	}
#else /* !HAVE_LZO */
	static inline bool is_LZO_loaded(void)
	{
		return false;
	}
#endif /* HAVE_LZO */

	static inline int lzo1x_decompress_safe(const lzo_bytep src, lzo_uint  src_len,
	                                              lzo_bytep dst, lzo_uintp dst_len,
	                                              lzo_voidp wrkmem /* NOT USED */)
	{
#ifdef HAVE_LZO
		return ::lzo1x_decompress_safe(src, src_len, dst, dst_len, wrkmem);
#else /* !HAVE_LZO */
		RP_UNUSED(src);
		RP_UNUSED(src_len);
		RP_UNUSED(dst);
		RP_UNUSED(dst_len);
		RP_UNUSED(wrkmem);
		return LZO_E_ERROR;
#endif /* HAVE_LZO */
	}
#endif /* LZO_SHARED_LINKAGE */
};

} // namespace LibRomData
