
/* intel_init.c - SSE2 optimized filter functions
 *
 * Copyright (c) 2016-2017 Glenn Randers-Pehrson
 * Written by Mike Klein and Matt Sarett, Google, Inc.
 * Derived from arm/arm_init.c
 *
 * Last changed in libpng 1.6.29 [March 16, 2017]
 *
 * This code is released under the libpng license.
 * For conditions of distribution and use, see the disclaimer
 * and license in png.h
 */

#include "../pngpriv.h"

#ifdef PNG_READ_SUPPORTED
#if PNG_INTEL_SSE_IMPLEMENTATION > 0

#ifdef _MSC_VER
// __cpuid() intrinsic is present as of MSVC 2005.
#include <intrin.h>
#elif defined(__GNUC__)
// gcc-4.3 has cpuid.h.
#include <cpuid.h>
#else
#error Unsupported compiler.
#endif

/**
 * Check if SSE2 is supported.
 * @return 0 if not supported; non-zero if supported.
 */
static __inline int
is_sse2_supported(void)
{
#if defined(_M_X64) || defined(__amd64__) || defined(__x86_64__)
	/* AMD64 always supports SSE2. */
	return 1;
#elif defined(_M_IX86) || defined(__i386__)
	/* 32-bit x86. */
	static int has_checked = 0;
	static int is_sse2 = 0;
#ifdef _MSC_VER
	int regs[4];	/* eax, ebx, ecx, edx */
#else /* __GNUC__ */
	unsigned int eax, ebx, ecx, edx;
	int ret;
#endif

	if (has_checked) {
		return is_sse2;
	}
	has_checked = 1;

#ifdef _MSC_VER
	/* Check if the CPUID instruction is supported. */
	__asm {
		pushfd
		pop	eax
		mov	regs[0], eax
		xor	eax, 0x200000
		push	eax
		popfd
		pushfd
		pop	eax
		xor	eax, regs[0]
		and	eax, 0x200000
		mov	regs[0], eax
	}

	if (regs[0] == 0) {
		/* CPUID is not supported. */
		return 0;
	}

	/* Get the maximum number of functions. */
	__cpuid(regs, 0);
	if (regs[0] < 1) {
		/* CPUID level 1 is not supported. */
		return 0;
	}

	/* Check for SSE2. */
	__cpuid(regs, 1);
	is_sse2 = !!(regs[3] & (1 << 26));
#else /* __GNUC__ */
	/* Check if CPUID is supported and if *
	 * the CPU supports SSE2.             */
	ret = __get_cpuid(1, &eax, &ebx, &ecx, &edx);
	if (ret == 0) {
		/* CPUID level 1 is not supported. */
		return 0;
	}

	is_sse2 = !!(edx & bit_SSE2);
#endif

	return is_sse2;
#else
	/* Not an x86 CPU. */
	return 0;
#endif
}

void
png_init_filter_functions_sse2(png_structp pp, unsigned int bpp)
{
   /* Check if SSE2 is supported. */
   if (!is_sse2_supported()) {
      /* SSE2 is not supported. */
      return;
   }

   /* The techniques used to implement each of these filters in SSE operate on
    * one pixel at a time.
    * So they generally speed up 3bpp images about 3x, 4bpp images about 4x.
    * They can scale up to 6 and 8 bpp images and down to 2 bpp images,
    * but they'd not likely have any benefit for 1bpp images.
    * Most of these can be implemented using only MMX and 64-bit registers,
    * but they end up a bit slower than using the equally-ubiquitous SSE2.
   */
   png_debug(1, "in png_init_filter_functions_sse2");
   if (bpp == 3)
   {
      pp->read_filter[PNG_FILTER_VALUE_SUB-1] = png_read_filter_row_sub3_sse2;
      pp->read_filter[PNG_FILTER_VALUE_AVG-1] = png_read_filter_row_avg3_sse2;
      pp->read_filter[PNG_FILTER_VALUE_PAETH-1] =
         png_read_filter_row_paeth3_sse2;
   }
   else if (bpp == 4)
   {
      pp->read_filter[PNG_FILTER_VALUE_SUB-1] = png_read_filter_row_sub4_sse2;
      pp->read_filter[PNG_FILTER_VALUE_AVG-1] = png_read_filter_row_avg4_sse2;
      pp->read_filter[PNG_FILTER_VALUE_PAETH-1] =
          png_read_filter_row_paeth4_sse2;
   }

   /* No need optimize PNG_FILTER_VALUE_UP.  The compiler should
    * autovectorize.
    */
}

#endif /* PNG_INTEL_SSE_IMPLEMENTATION > 0 */
#endif /* PNG_READ_SUPPORTED */
