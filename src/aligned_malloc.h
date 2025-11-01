/***************************************************************************
 * ROM Properties Page shell extension.                                    *
 * aligned_malloc.h: Aligned memory allocation compatibility header.       *
 *                                                                         *
 * Copyright (c) 2015-2025 by David Korth                                  *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

// References:
// - http://www.gnu.org/software/libc/manual/html_node/Aligned-Memory-Blocks.html
// - https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/aligned-malloc?view=msvc-160
// - http://linux.die.net/man/3/aligned_alloc (needs _ISOC11_SOURCE ?)

#include "config.libc.h"
#include "force_inline.h"

/**
 * This header defines two functions for aligned memory allocation:
 * - aligned_malloc(): Allocate aligned memory.
 *   - Same signature as C11 aligned_alloc().
 *   - Returns NULL and sets errno on error.
 *   - NOTE: C aligned_alloc() requires size to be a multiple of
 *     alignment. glibc does not enforce this requirement.
 *   - NOTE: When using posix_memalign(), alignment must be a
 *     power of two multiple of sizeof(void*).
 * - aligned_free(): Free aligned memory.
 *   - Required for MSVC and custom implementations.
 *
 * These functions are wrappers around system-provided functions.
 */

#include <errno.h>

// Alignment macros
#include "alignment_macros.h"

#if defined(HAVE_MSVC_ALIGNED_MALLOC)

// MSVC _aligned_malloc()
#include <malloc.h>

static FORCEINLINE void *aligned_malloc(size_t alignment, size_t size)
{
	return _aligned_malloc(ALIGN_BYTES(alignment, size), alignment);
}

static FORCEINLINE void aligned_free(void *memptr)
{
	_aligned_free(memptr);
}

#elif defined(HAVE_ALIGNED_ALLOC) && !defined(__APPLE__)
// FIXME: Not working properly on Mac OS. (only provided as of 10.15?)

// C11 aligned_alloc()
#include <stdlib.h>

static FORCEINLINE void *aligned_malloc(size_t alignment, size_t size)
{
	return aligned_alloc(alignment, ALIGN_BYTES(alignment, size));
}

static FORCEINLINE void aligned_free(void *memptr)
{
	free(memptr);
}

#elif defined(HAVE_POSIX_MEMALIGN)

// posix_memalign()
#include <stdlib.h>

static FORCEINLINE void *aligned_malloc(size_t alignment, size_t size)
{
	void *ptr;
	const int ret = posix_memalign(&ptr, alignment, ALIGN_BYTES(alignment, size));
	if (ret != 0) {
		// posix_memalign() returns errno instead of setting it.
		errno = ret;
		return NULL;
	}
	return ptr;
}

static FORCEINLINE void aligned_free(void *memptr)
{
	free(memptr);
}

#elif defined(HAVE_MEMALIGN)

// memalign()
#include <malloc.h>

static FORCEINLINE void *aligned_malloc(size_t alignment, size_t size)
{
	return memalign(alignment, ALIGN_BYTES(alignment, size));
}

static FORCEINLINE void aligned_free(void *memptr)
{
	free(memptr);
}

#else
#  error Missing aligned malloc() function for this system.
#endif

#ifdef __cplusplus

// std::unique_ptr<> wrapper for aligned_malloc().
// Reference: https://embeddedartistry.com/blog/2017/2/23/c-smart-pointers-with-aligned-mallocfree
#include <memory>

// NOTE: MSVC 2010 doesn't support "template using".
// TODO: Check 2012; assuming 2013+ for now.
#if !defined(_MSC_VER) || _MSC_VER >= 1800
template<class T>
using unique_ptr_aligned = std::unique_ptr<T, decltype(&aligned_free)>;
#  define UNIQUE_PTR_ALIGNED(T) unique_ptr_aligned<T>
#else /* _MSC_VER < 1900 */
#  define UNIQUE_PTR_ALIGNED(T) std::unique_ptr<T, decltype(&aligned_free)>
#endif

/**
 * Aligned unique_ptr() helper.
 * @param align Alignment, in bytes.
 * @param size Size, in sizeof(T) units.
 */
template<class T>
static inline UNIQUE_PTR_ALIGNED(T) aligned_uptr(size_t align, size_t size)
{
	return UNIQUE_PTR_ALIGNED(T)(static_cast<T*>(aligned_malloc(align, size * sizeof(T))), &aligned_free);
}

#endif /* __cplusplus */
