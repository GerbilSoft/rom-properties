/***************************************************************************
 * ROM Properties Page shell extension. (librpbyteswap)                    *
 * byteswap_rp.h: Byteswapping functions.                                  *
 *                                                                         *
 * Copyright (c) 2008-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

// C includes
#ifdef __cplusplus
#  include <cstdint>
#else /* !__cplusplus */
#  include <stdint.h>
#endif /* __cplusplus */

#include "librpcpuid/config.librpcpuid.h"
#include "librpcpuid/cpu_dispatch.h"

#include "librpbyteswap/config.byteswap.h"
#include "librpbyteswap/byteorder.h"
#include "dll-macros.h"
#include "force_inline.h"

#if defined(RP_CPU_I386) || defined(RP_CPU_AMD64)
#  include "librpcpuid/cpuflags_x86.h"
/* MSVC does not support MMX intrinsics in 64-bit builds. */
/* Reference: https://docs.microsoft.com/en-us/cpp/cpp/m64?view=msvc-160 */
/* In addition, amd64 CPUs all support SSE2 as a minimum, */
/* so there's no point in building MMX code for 64-bit. */
#  if (defined(_M_IX86) || defined(__i386__)) && \
      !(defined(_M_X64) || defined(_M_AMD64) || defined(__amd64__) || defined(__x86_64__))
#    define BYTESWAP_HAS_MMX 1
#  endif
#  define BYTESWAP_HAS_SSE2 1
#  define BYTESWAP_HAS_SSSE3 1
#endif
#ifdef RP_CPU_AMD64
#  define BYTESWAP_ALWAYS_HAS_SSE2 1
#endif /* RP_CPU_AMD64 */
#ifdef RP_CPU_ARM64
#  define BYTESWAP_HAS_NEON 1
#  define BYTESWAP_ALWAYS_HAS_NEON 1
#endif /* RP_CPU_ARM64 */

#if defined(_MSC_VER)

/* Use the MSVC byteswap intrinsics. */
#  ifdef __cplusplus
#    include <cstdlib>
#  else /* !__cplusplus */
#    include <stdlib.h>
#  endif /* __cplusplus */
#  define __swab16(x) _byteswap_ushort(x)
#  define __swab32(x) _byteswap_ulong(x)
#  define __swab64(x) _byteswap_uint64(x)

/* `inline` might not be defined in older versions. */
#  ifndef inline
#    define inline __inline
#  endif

#else /* !defined(_MSC_VER) */

/* Use gcc's byteswap intrinsics if available. */

#ifdef HAVE___BUILTIN_BSWAP16
#  define __swab16(x) ((uint16_t)__builtin_bswap16(x))
#else
#  define __swab16(x) ((uint16_t)((((uint16_t)x) << 8) | (((uint16_t)x) >> 8)))
#endif

#ifdef HAVE___BUILTIN_BSWAP32
#  define __swab32(x) ((uint32_t)__builtin_bswap32(x))
#else
#  define __swab32(x) \
	((uint32_t)((((uint32_t)x) << 24) | (((uint32_t)x) >> 24) | \
		((((uint32_t)x) & 0x0000FF00UL) << 8) | \
		((((uint32_t)x) & 0x00FF0000UL) >> 8)))
#endif

#ifdef HAVE___BUILTIN_BSWAP64
#  define __swab64(x) ((uint64_t)__builtin_bswap64(x))
#else
#  define __swab64(x) \
	((uint64_t)((((uint64_t)x) << 56) | (((uint64_t)x) >> 56) | \
		((((uint64_t)x) & 0x000000000000FF00ULL) << 40) | \
		((((uint64_t)x) & 0x0000000000FF0000ULL) << 24) | \
		((((uint64_t)x) & 0x00000000FF000000ULL) << 8) | \
		((((uint64_t)x) & 0x000000FF00000000ULL) >> 8) | \
		((((uint64_t)x) & 0x0000FF0000000000ULL) >> 24) | \
		((((uint64_t)x) & 0x00FF000000000000ULL) >> 40)))
#endif

#endif /* defined(_MSC_VER) */

/**
 * Byteswap a float.
 * @param f Float to byteswap
 * @return Byteswapped float
 */
static inline float __swabf(float f)
{
	union {
		uint32_t u32;
		float f;
	} u32_f;
	u32_f.f = f;
	u32_f.u32 = __swab32(u32_f.u32);
	return u32_f.f;
}

#if SYS_BYTEORDER == SYS_LIL_ENDIAN
	#define be16_to_cpu(x)	__swab16(x)
	#define be32_to_cpu(x)	__swab32(x)
	#define be64_to_cpu(x)	__swab64(x)
	#define le16_to_cpu(x)	(x)
	#define le32_to_cpu(x)	(x)
	#define le64_to_cpu(x)	(x)

	#define cpu_to_be16(x)	__swab16(x)
	#define cpu_to_be32(x)	__swab32(x)
	#define cpu_to_be64(x)	__swab64(x)
	#define cpu_to_le16(x)	(x)
	#define cpu_to_le32(x)	(x)
	#define cpu_to_le64(x)	(x)

	#define bef32_to_cpu(x)	__swabf(x)
	#define cpu_to_bef32(x)	__swabf(x)

	#define lef32_to_cpu(x)	(x)
	#define cpu_to_lef32(x)	(x)
#else /* SYS_BYTEORDER == SYS_BIG_ENDIAN */
	#define be16_to_cpu(x)	(x)
	#define be32_to_cpu(x)	(x)
	#define be64_to_cpu(x)	(x)
	#define le16_to_cpu(x)	__swab16(x)
	#define le32_to_cpu(x)	__swab32(x)
	#define le64_to_cpu(x)	__swab64(x)

	#define cpu_to_be16(x)	(x)
	#define cpu_to_be32(x)	(x)
	#define cpu_to_be64(x)	(x)
	#define cpu_to_le16(x)	__swab16(x)
	#define cpu_to_le32(x)	__swab32(x)
	#define cpu_to_le64(x)	__swab64(x)

	#define bef32_to_cpu(x)	(x)
	#define cpu_to_bef32(x)	(x)

	#define lef32_to_cpu(x)	__swabf(x)
	#define cpu_to_lef32(x)	__swabf(x)
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * 16-bit byteswap function.
 * Standard version using regular C code.
 * @param ptr Pointer to array to swap. (MUST be 16-bit aligned!)
 * @param n Number of bytes to swap. (Must be divisible by 2; an extra odd byte will be ignored.)
 */
RP_LIBROMDATA_PUBLIC
void RP_C_API rp_byte_swap_16_array_c(uint16_t *ptr, size_t n);

/**
 * 32-bit byteswap function.
 * Standard version using regular C code.
 * @param ptr Pointer to array to swap. (MUST be 32-bit aligned!)
 * @param n Number of bytes to swap. (Must be divisible by 4; extra bytes will be ignored.)
 */
RP_LIBROMDATA_PUBLIC
void RP_C_API rp_byte_swap_32_array_c(uint32_t *ptr, size_t n);

#ifdef BYTESWAP_HAS_MMX
/**
 * 16-bit byteswap function.
 * MMX-optimized version.
 * @param ptr Pointer to array to swap. (MUST be 16-bit aligned!)
 * @param n Number of bytes to swap. (Must be divisible by 2; an extra odd byte will be ignored.)
 */
RP_LIBROMDATA_PUBLIC
void RP_C_API rp_byte_swap_16_array_mmx(uint16_t *ptr, size_t n);

/**
 * 32-bit byteswap function.
 * MMX-optimized version.
 * @param ptr Pointer to array to swap. (MUST be 32-bit aligned!)
 * @param n Number of bytes to swap. (Must be divisible by 4; extra bytes will be ignored.)
 */
RP_LIBROMDATA_PUBLIC
void RP_C_API rp_byte_swap_32_array_mmx(uint32_t *ptr, size_t n);
#endif /* BYTESWAP_HAS_MMX */

#ifdef BYTESWAP_HAS_SSE2
/**
 * 16-bit byteswap function.
 * SSE2-optimized version.
 * @param ptr Pointer to array to swap. (MUST be 16-bit aligned!)
 * @param n Number of bytes to swap. (Must be divisible by 2; an extra odd byte will be ignored.)
 */
RP_LIBROMDATA_PUBLIC
void RP_C_API rp_byte_swap_16_array_sse2(uint16_t *ptr, size_t n);

/**
 * 32-bit byteswap function.
 * SSE2-optimized version.
 * @param ptr Pointer to array to swap. (MUST be 32-bit aligned!)
 * @param n Number of bytes to swap. (Must be divisible by 4; extra bytes will be ignored.)
 */
RP_LIBROMDATA_PUBLIC
void RP_C_API rp_byte_swap_32_array_sse2(uint32_t *ptr, size_t n);
#endif /* BYTESWAP_HAS_SSE2 */

#ifdef BYTESWAP_HAS_SSSE3
/**
 * 16-bit byteswap function.
 * SSSE3-optimized version.
 * @param ptr Pointer to array to swap. (MUST be 16-bit aligned!)
 * @param n Number of bytes to swap. (Must be divisible by 2; an extra odd byte will be ignored.)
 */
RP_LIBROMDATA_PUBLIC
void RP_C_API rp_byte_swap_16_array_ssse3(uint16_t *ptr, size_t n);

/**
 * 32-bit byteswap function.
 * SSSE3-optimized version.
 * @param ptr Pointer to array to swap. (MUST be 32-bit aligned!)
 * @param n Number of bytes to swap. (Must be divisible by 4; extra bytes will be ignored.)
 */
RP_LIBROMDATA_PUBLIC
void RP_C_API rp_byte_swap_32_array_ssse3(uint32_t *ptr, size_t n);
#endif /* BYTESWAP_HAS_SSSE3 */

#ifdef BYTESWAP_HAS_NEON
/**
 * 16-bit byteswap function.
 * NEON-optimized version.
 * @param ptr Pointer to array to swap. (MUST be 16-bit aligned!)
 * @param n Number of bytes to swap. (Must be divisible by 2; an extra odd byte will be ignored.)
 */
RP_LIBROMDATA_PUBLIC
void RP_C_API rp_byte_swap_16_array_neon(uint16_t *ptr, size_t n);

/**
 * 32-bit byteswap function.
 * NEON-optimized version.
 * @param ptr Pointer to array to swap. (MUST be 32-bit aligned!)
 * @param n Number of bytes to swap. (Must be divisible by 4; extra bytes will be ignored.)
 */
RP_LIBROMDATA_PUBLIC
void RP_C_API rp_byte_swap_32_array_neon(uint32_t *ptr, size_t n);
#endif /* BYTESWAP_HAS_ARM64 */

/**
 * 16-bit byteswap function.
 * @param ptr Pointer to array to swap. (MUST be 16-bit aligned!)
 * @param n Number of bytes to swap. (Must be divisible by 2; an extra odd byte will be ignored.)
 */
static FORCEINLINE void rp_byte_swap_16_array(uint16_t *ptr, size_t n)
{
#ifdef BYTESWAP_HAS_SSSE3
	if (RP_CPU_HasSSSE3()) {
		rp_byte_swap_16_array_ssse3(ptr, n);
	} else
#endif /* BYTESWAP_HAS_SSSE3 */
#ifdef BYTESWAP_ALWAYS_HAS_SSE2
	{
		rp_byte_swap_16_array_sse2(ptr, n);
	}
#else /* !BYTESWAP_ALWAYS_HAS_SSE2 */
#  ifdef BYTESWAP_HAS_SSE2
	if (RP_CPU_HasSSE2()) {
		rp_byte_swap_16_array_sse2(ptr, n);
	} else
#  endif /* BYTESWAP_HAS_SSE2 */
#  ifdef BYTESWAP_HAS_MMX
	if (RP_CPU_HasMMX()) {
		rp_byte_swap_16_array_mmx(ptr, n);
	} else
#  endif /* BYTESWAP_HAS_MMX */
	// TODO: MMX-optimized version?
	{
		rp_byte_swap_16_array_c(ptr, n);
	}
#endif /* BYTESWAP_ALWAYS_HAS_SSE2 */
}

/**
 * 32-bit byteswap function.
 * @param ptr Pointer to array to swap. (MUST be 32-bit aligned!)
 * @param n Number of bytes to swap. (Must be divisible by 4; extra bytes will be ignored.)
 */
static FORCEINLINE void rp_byte_swap_32_array(uint32_t *ptr, size_t n)
{
#ifdef BYTESWAP_HAS_SSSE3
	if (RP_CPU_HasSSSE3()) {
		rp_byte_swap_32_array_ssse3(ptr, n);
	} else
#endif /* BYTESWAP_HAS_SSSE3 */
#ifdef BYTESWAP_ALWAYS_HAS_SSE2
	{
		rp_byte_swap_32_array_sse2(ptr, n);
	}
#else /* !BYTESWAP_ALWAYS_HAS_SSE2 */
#  ifdef BYTESWAP_HAS_SSE2
	if (RP_CPU_HasSSE2()) {
		rp_byte_swap_32_array_sse2(ptr, n);
	} else
#  endif /* BYTESWAP_HAS_SSE2 */
#  if 0 /* FIXME: The MMX version is actually *slower* than the C version. */
#  ifdef BYTESWAP_HAS_MMX
	if (RP_CPU_HasMMX()) {
		rp_byte_swap_32_array_mmx(ptr, n);
	} else
#  endif /* BYTESWAP_HAS_MMX */
#  endif /* 0 */
	{
		rp_byte_swap_32_array_c(ptr, n);
	}
#endif /* !BYTESWAP_ALWAYS_HAS_SSE2 */
}

#ifdef __cplusplus
}
#endif
