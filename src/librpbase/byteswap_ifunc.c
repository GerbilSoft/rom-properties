/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * byteswap_ifunc.c: Byteswapping functions. (IFUNC)                       *
 *                                                                         *
 * Copyright (c) 2016-2018 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "cpu_dispatch.h"

#ifdef RP_HAS_IFUNC

#include "byteswap.h"

/**
 * IFUNC resolver function for __byte_swap_16_array().
 * @return Function pointer.
 */
static __typeof__(&__byte_swap_16_array_c) __byte_swap_16_array_resolve(void)
{
#ifdef BYTESWAP_HAS_SSSE3
	if (RP_CPU_HasSSSE3()) {
		return &__byte_swap_16_array_ssse3;
	} else
#endif /* BYTESWAP_HAS_SSSE3 */
#ifdef BYTESWAP_ALWAYS_HAS_SSE2
	{
		return &__byte_swap_16_array_sse2;
	}
#else /* !BYTESWAP_ALWAYS_HAS_SSE2 */
# ifdef BYTESWAP_HAS_SSE2
	if (RP_CPU_HasSSE2()) {
		return &__byte_swap_16_array_sse2;
	} else
# endif /* BYTESWAP_HAS_SSE2 */
# ifdef BYTESWAP_HAS_MMX
	if (RP_CPU_HasMMX()) {
		return &__byte_swap_16_array_mmx;
	} else
# endif /* BYTESWAP_HAS_MMX */
	{
		return &__byte_swap_16_array_c;
	}
#endif /* BYTESWAP_ALWAYS_HAS_SSE2 */
}

/**
 * IFUNC resolver function for __byte_swap_32_array().
 * @return Function pointer.
 */
static __typeof__(&__byte_swap_32_array_c) __byte_swap_32_array_resolve(void)
{
#ifdef BYTESWAP_HAS_SSSE3
	if (RP_CPU_HasSSSE3()) {
		return &__byte_swap_32_array_ssse3;
	} else
#endif /* BYTESWAP_HAS_SSSE3 */
#ifdef BYTESWAP_ALWAYS_HAS_SSE2
	{
		return &__byte_swap_32_array_sse2;
	}
#else /* !BYTESWAP_ALWAYS_HAS_SSE2 */
# ifdef BYTESWAP_HAS_SSE2
	if (RP_CPU_HasSSE2()) {
		return &__byte_swap_32_array_sse2;
	} else
# endif /* BYTESWAP_HAS_SSE2 */
# if 0 /* FIXME: The MMX version is actually *slower* than the C version. */
# ifdef BYTESWAP_HAS_MMX
	if (RP_CPU_HasMMX()) {
		return &__byte_swap_32_array_mmx;
	} else
# endif /* BYTESWAP_HAS_MMX */
# endif /* 0 */
	{
		return &__byte_swap_32_array_c;
	}
#endif /* !BYTESWAP_ALWAYS_HAS_SSE2 */
}

void __byte_swap_16_array(uint16_t *ptr, unsigned int n)
	IFUNC_ATTR(__byte_swap_16_array_resolve);
void __byte_swap_32_array(uint32_t *ptr, unsigned int n)
	IFUNC_ATTR(__byte_swap_32_array_resolve);

#endif /* RP_HAS_IFUNC */
