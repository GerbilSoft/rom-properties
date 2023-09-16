/***************************************************************************
 * ROM Properties Page shell extension. (librpcpu)                         *
 * byteswap_ifunc.c: Byteswapping functions. (IFUNC)                       *
 *                                                                         *
 * Copyright (c) 2016-2022 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "byteswap_rp.h"
#include "cpu_dispatch.h"

#ifdef HAVE_IFUNC

// NOTE: llvm/clang 14.0.0 fails to detect the resolver functions
// if they're marked static, even though the docs say this is okay.
// In C code, it merely warns that the resolvers aren't used, but
// in C++ code, the IFUNC_ATTR() attribute fails.

/**
 * IFUNC resolver function for rp_byte_swap_16_array().
 * @return Function pointer.
 */
NO_SANITIZE_ADDRESS
__typeof__(&rp_byte_swap_16_array_c) rp_byte_swap_16_array_resolve(void)
{
#if defined(BYTESWAP_HAS_SSSE3) || defined(BYTESWAP_HAS_SSE2) || defined(BYTESWAP_HAS_MMX)
	__builtin_cpu_init();
#endif

#ifdef BYTESWAP_HAS_SSSE3
	if (__builtin_cpu_supports("ssse3")) {
		return &rp_byte_swap_16_array_ssse3;
	} else
#endif /* BYTESWAP_HAS_SSSE3 */
#ifdef BYTESWAP_ALWAYS_HAS_SSE2
	{
		return &rp_byte_swap_16_array_sse2;
	}
#else /* !BYTESWAP_ALWAYS_HAS_SSE2 */
# ifdef BYTESWAP_HAS_SSE2
	if (__builtin_cpu_supports("sse2")) {
		return &rp_byte_swap_16_array_sse2;
	} else
# endif /* BYTESWAP_HAS_SSE2 */
# ifdef BYTESWAP_HAS_MMX
	if (__builtin_cpu_supports("mmx")) {
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
NO_SANITIZE_ADDRESS
__typeof__(&rp_byte_swap_32_array_c) rp_byte_swap_32_array_resolve(void)
{
	// NOTE: Since libromdata is a shared library now, IFUNC resolvers
	// cannot call PLT functions. Otherwise, it will crash.
	// We'll use gcc's built-in CPU ID functions instead.
	// Requires gcc-4.8 or later, or clang-6.0 or later.

#if defined(BYTESWAP_HAS_SSSE3) || defined(BYTESWAP_HAS_SSE2) || defined(BYTESWAP_HAS_MMX)
	__builtin_cpu_init();
#endif

#ifdef BYTESWAP_HAS_SSSE3
	if (__builtin_cpu_supports("ssse3")) {
		return &rp_byte_swap_32_array_ssse3;
	} else
#endif /* BYTESWAP_HAS_SSSE3 */
#ifdef BYTESWAP_ALWAYS_HAS_SSE2
	{
		return &rp_byte_swap_32_array_sse2;
	}
#else /* !BYTESWAP_ALWAYS_HAS_SSE2 */
# ifdef BYTESWAP_HAS_SSE2
	if (__builtin_cpu_supports("sse2")) {
		return &rp_byte_swap_32_array_sse2;
	} else
# endif /* BYTESWAP_HAS_SSE2 */
# if 0 /* FIXME: The MMX version is actually *slower* than the C version. */
# ifdef BYTESWAP_HAS_MMX
	if (__builtin_cpu_supports("mmx")) {
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
