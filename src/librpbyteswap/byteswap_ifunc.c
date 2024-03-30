/***************************************************************************
 * ROM Properties Page shell extension. (librpbyteswap)                    *
 * byteswap_ifunc.c: Byteswapping functions. (IFUNC)                       *
 *                                                                         *
 * Copyright (c) 2016-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "config.librpcpuid.h"

#ifdef HAVE_IFUNC

#include "byteswap_rp.h"
#include "cpu_dispatch.h"

// NOTE: llvm/clang 14.0.0 fails to detect the resolver functions
// if they're marked static, even though the docs say this is okay.
// In C code, it merely warns that the resolvers aren't used, but
// in C++ code, the IFUNC_ATTR() attribute fails.

/**
 * IFUNC resolver function for rp_byte_swap_16_array().
 * @return Function pointer.
 */
__typeof__(&rp_byte_swap_16_array_c) rp_byte_swap_16_array_resolve(void)
{
#ifdef BYTESWAP_HAS_SSSE3
	if (RP_CPU_HasSSSE3()) {
		return &rp_byte_swap_16_array_ssse3;
	} else
#endif /* BYTESWAP_HAS_SSSE3 */
#ifdef BYTESWAP_ALWAYS_HAS_SSE2
	{
		return &rp_byte_swap_16_array_sse2;
	}
#else /* !BYTESWAP_ALWAYS_HAS_SSE2 */
# ifdef BYTESWAP_HAS_SSE2
	if (RP_CPU_HasSSE2()) {
		return &rp_byte_swap_16_array_sse2;
	} else
# endif /* BYTESWAP_HAS_SSE2 */
# ifdef BYTESWAP_HAS_MMX
	if (RP_CPU_HasMMX()) {
		return &rp_byte_swap_16_array_mmx;
	} else
# endif /* BYTESWAP_HAS_MMX */
	{
		return &rp_byte_swap_16_array_c;
	}
#endif /* BYTESWAP_ALWAYS_HAS_SSE2 */
}

/**
 * IFUNC resolver function for rp_byte_swap_32_array().
 * @return Function pointer.
 */
__typeof__(&rp_byte_swap_32_array_c) rp_byte_swap_32_array_resolve(void)
{
#ifdef BYTESWAP_HAS_SSSE3
	if (RP_CPU_HasSSSE3()) {
		return &rp_byte_swap_32_array_ssse3;
	} else
#endif /* BYTESWAP_HAS_SSSE3 */
#ifdef BYTESWAP_ALWAYS_HAS_SSE2
	{
		return &rp_byte_swap_32_array_sse2;
	}
#else /* !BYTESWAP_ALWAYS_HAS_SSE2 */
# ifdef BYTESWAP_HAS_SSE2
	if (RP_CPU_HasSSE2()) {
		return &rp_byte_swap_32_array_sse2;
	} else
# endif /* BYTESWAP_HAS_SSE2 */
# if 0 /* FIXME: The MMX version is actually *slower* than the C version. */
# ifdef BYTESWAP_HAS_MMX
	if (RP_CPU_HasMMX()) {
		return &rp_byte_swap_32_array_mmx;
	} else
# endif /* BYTESWAP_HAS_MMX */
# endif /* 0 */
	{
		return &rp_byte_swap_32_array_c;
	}
#endif /* !BYTESWAP_ALWAYS_HAS_SSE2 */
}

void RP_C_API rp_byte_swap_16_array(uint16_t *ptr, size_t n)
	IFUNC_ATTR(rp_byte_swap_16_array_resolve);
void RP_C_API rp_byte_swap_32_array(uint32_t *ptr, size_t n)
	IFUNC_ATTR(rp_byte_swap_32_array_resolve);

#endif /* HAVE_IFUNC */
