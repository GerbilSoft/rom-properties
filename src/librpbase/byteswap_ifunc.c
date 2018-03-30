/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * byteswap_ifunc.c: Byteswapping functions. (IFUNC)                       *
 *                                                                         *
 * Copyright (c) 2016-2018 by David Korth.                                 *
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
 * You should have received a copy of the GNU General Public License       *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.   *
 ***************************************************************************/

#include "cpu_dispatch.h"

#ifdef RP_HAS_IFUNC

#include "byteswap.h"

/**
 * IFUNC resolver function for __byte_swap_16_array().
 * @return Function pointer.
 */
static RP_IFUNC_ptr_t __byte_swap_16_array_resolve(void)
{
#ifdef BYTESWAP_HAS_SSSE3
	if (RP_CPU_HasSSSE3()) {
		return (RP_IFUNC_ptr_t)&__byte_swap_16_array_ssse3;
	} else
#endif /* BYTESWAP_HAS_SSSE3 */
#ifdef BYTESWAP_ALWAYS_HAS_SSE2
	{
		return (RP_IFUNC_ptr_t)&__byte_swap_16_array_sse2;
	}
#else /* !BYTESWAP_ALWAYS_HAS_SSE2 */
# ifdef BYTESWAP_HAS_SSE2
	if (RP_CPU_HasSSE2()) {
		return (RP_IFUNC_ptr_t)&__byte_swap_16_array_sse2;
	} else
# endif /* BYTESWAP_HAS_SSE2 */
# ifdef BYTESWAP_HAS_MMX
	if (RP_CPU_HasMMX()) {
		return (RP_IFUNC_ptr_t)&__byte_swap_16_array_mmx;
	} else
# endif /* BYTESWAP_HAS_MMX */
	{
		return (RP_IFUNC_ptr_t)&__byte_swap_16_array_c;
	}
#endif /* BYTESWAP_ALWAYS_HAS_SSE2 */
}

/**
 * IFUNC resolver function for __byte_swap_32_array().
 * @return Function pointer.
 */
static RP_IFUNC_ptr_t __byte_swap_32_array_resolve(void)
{
#ifdef BYTESWAP_HAS_SSSE3
	if (RP_CPU_HasSSSE3()) {
		return (RP_IFUNC_ptr_t)&__byte_swap_32_array_ssse3;
	} else
#endif /* BYTESWAP_HAS_SSSE3 */
#ifdef BYTESWAP_ALWAYS_HAS_SSE2
	{
		return (RP_IFUNC_ptr_t)&__byte_swap_32_array_sse2;
	}
#else /* !BYTESWAP_ALWAYS_HAS_SSE2 */
# ifdef BYTESWAP_HAS_SSE2
	if (RP_CPU_HasSSE2()) {
		return (RP_IFUNC_ptr_t)&__byte_swap_32_array_sse2;
	} else
# endif /* BYTESWAP_HAS_SSE2 */
# if 0 /* FIXME: The MMX version is actually *slower* than the C version. */
# ifdef BYTESWAP_HAS_MMX
	if (RP_CPU_HasMMX()) {
		return (RP_IFUNC_ptr_t)&__byte_swap_32_array_mmx;
	} else
# endif /* BYTESWAP_HAS_MMX */
# endif /* 0 */
	{
		return (RP_IFUNC_ptr_t)&__byte_swap_32_array_c;
	}
#endif /* !BYTESWAP_ALWAYS_HAS_SSE2 */
}

void __byte_swap_16_array(uint16_t *ptr, unsigned int n)
	IFUNC_ATTR(__byte_swap_16_array_resolve);
void __byte_swap_32_array(uint32_t *ptr, unsigned int n)
	IFUNC_ATTR(__byte_swap_32_array_resolve);

#endif /* RP_HAS_IFUNC */
