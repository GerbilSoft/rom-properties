/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * byteswap.h: Byteswapping functions.                                     *
 *                                                                         *
 * Copyright (c) 2008-2017 by David Korth.                                 *
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

#ifndef __ROMPROPERTIES_LIBRPBASE_BYTESWAP_H__
#define __ROMPROPERTIES_LIBRPBASE_BYTESWAP_H__

// C includes.
#include <stdint.h>

/* Get the system byte order. */
#include "byteorder.h"
#include "cpu_dispatch.h"

#if defined(RP_CPU_I386) || defined(RP_CPU_AMD64)
# include "librpbase/cpuflags_x86.h"
# define BYTESWAP_HAS_SSE2 1
#endif
#ifdef RP_CPU_AMD64
# define BYTESWAP_ALWAYS_HAS_SSE2 1
#endif

#if defined(_MSC_VER)

/* Use the MSVC byteswap intrinsics. */
#include <stdlib.h>
#define __swab16(x) _byteswap_ushort(x)
#define __swab32(x) _byteswap_ulong(x)
#define __swab64(x) _byteswap_uint64(x)

/* `inline` might not be defined in older versions. */
#ifndef inline
# define inline __inline
#endif

#elif defined(__GNUC__)

/* Use the gcc byteswap intrinsics. */
#define __swab16(x) __builtin_bswap16(x)
#define __swab32(x) __builtin_bswap32(x)
#define __swab64(x) __builtin_bswap64(x)

#else

/* Use the macros. */
#warning No intrinsics defined for this compiler. Byteswapping may be slow.

#define __swab16(x) ((uint16_t)(((x) << 8) | ((x) >> 8)))

#define __swab32(x) \
	((uint32_t)(((x) << 24) | ((x) >> 24) | \
		(((x) & 0x0000FF00UL) << 8) | \
		(((x) & 0x00FF0000UL) >> 8)))

#define __swab64(x) \
	((uint64_t)(((x) << 56) | ((x) >> 56) | \
		(((x) & 0x000000000000FF00ULL) << 40) | \
		(((x) & 0x0000000000FF0000ULL) << 24) | \
		(((x) & 0x00000000FF000000ULL) << 8) | \
		(((x) & 0x000000FF00000000ULL) >> 8) | \
		(((x) & 0x0000FF0000000000ULL) >> 24) | \
		(((x) & 0x00FF000000000000ULL) >> 40)))
#endif

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
void __byte_swap_16_array_c(uint16_t *ptr, unsigned int n);

/**
 * 32-bit byteswap function.
 * Standard version using regular C code.
 * @param ptr Pointer to array to swap. (MUST be 32-bit aligned!)
 * @param n Number of bytes to swap. (Must be divisible by 4; extra bytes will be ignored.)
 */
void __byte_swap_32_array_c(uint32_t *ptr, unsigned int n);

#ifdef BYTESWAP_HAS_SSE2
/**
 * 16-bit byteswap function.
 * SSE2-optimized version.
 * @param ptr Pointer to array to swap. (MUST be 16-bit aligned!)
 * @param n Number of bytes to swap. (Must be divisible by 2; an extra odd byte will be ignored.)
 */
void __byte_swap_16_array_sse2(uint16_t *ptr, unsigned int n);
#endif /* BYTESWAP_HAS_SSE2 */

#ifdef BYTESWAP_ALWAYS_HAS_SSE2
/* System always has SSE2. No IFUNC necessary. */

/**
 * 16-bit byteswap function.
 * @param ptr Pointer to array to swap. (MUST be 16-bit aligned!)
 * @param n Number of bytes to swap. (Must be divisible by 2; an extra odd byte will be ignored.)
 */
static inline void __byte_swap_16_array(uint16_t *ptr, unsigned int n)
{
	__byte_swap_16_array_sse2(ptr, n);
}

#else /* !BYTESWAP_ALWAYS_HAS_SSE2 */

# if defined(RP_HAS_IFUNC)
/* System has IFUNC. Use it for dispatching. */

/**
 * 16-bit byteswap function.
 * @param ptr Pointer to array to swap. (MUST be 16-bit aligned!)
 * @param n Number of bytes to swap. (Must be divisible by 2; an extra odd byte will be ignored.)
 */
void __byte_swap_16_array(uint16_t *ptr, unsigned int n);
# else /* !RP_HAS_IFUNC */
/* System does not have IFUNC. Use inline dispatch functions. */

/**
 * 16-bit byteswap function.
 * @param ptr Pointer to array to swap. (MUST be 16-bit aligned!)
 * @param n Number of bytes to swap. (Must be divisible by 2; an extra odd byte will be ignored.)
 */
static inline void __byte_swap_16_array(uint16_t *ptr, unsigned int n)
{
#  ifdef BYTESWAP_HAS_SSE2
	if (RP_CPU_HasSSE2()) {
		__byte_swap_16_array_sse2(ptr, n);
	} else
#  endif /* BYTESWAP_HAS_SSE2 */
	{
		__byte_swap_16_array_c(ptr, n);
	}
}
# endif /* RP_HAS_IFUNC */

#endif /* BYTESWAP_ALWAYS_HAS_SSE2 */

// TODO: SSSE3-optimized 32-bit byteswap.
// Also MMX versions?
/**
 * 16-bit byteswap function.
 * @param ptr Pointer to array to swap. (MUST be 16-bit aligned!)
 * @param n Number of bytes to swap. (Must be divisible by 2; an extra odd byte will be ignored.)
 */
static inline void __byte_swap_32_array(uint32_t *ptr, unsigned int n)
{
	__byte_swap_32_array_c(ptr, n);
}

#ifdef __cplusplus
}
#endif

#endif /* __ROMPROPERTIES_LIBRPBASE_BYTESWAP_H__ */
