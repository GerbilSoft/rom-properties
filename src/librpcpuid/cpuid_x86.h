/***************************************************************************
 * ROM Properties Page shell extension. (librpcpuid)                       *
 * cpuid_x86.h: CPUID macros for x86.                                      *
 *                                                                         *
 * Copyright (c) 2017-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "cpuflags_x86.h"
#if !defined(RP_CPU_I386) && !defined(RP_CPU_AMD64)
#  error Do not compile cpuflags_x86.c on non-x86 CPUs!
#endif

#include "config.librpcpuid.h"
#ifdef HAVE_CPUID_H
#  include <cpuid.h>
#endif /* HAVE_CPUID_H */

#if defined(_MSC_VER) && _MSC_VER >= 1400
#  include <intrin.h>
#endif

#include "force_inline.h"

/**
 * Check if CPUID is supported on this CPU.
 * @return 0 if not supported; non-zero if supported.
 */
static FORCEINLINE int is_cpuid_supported(void)
{
#if defined(__GNUC__) && defined(RP_CPU_I386)
	// gcc, i386
	int __eax;
	__asm__ (
		"pushfl\n"
		"popl	%%eax\n"
		"movl	%%eax, %%edx\n"
		"xorl	$0x200000, %%eax\n"
		"pushl	%%eax\n"
		"popfl\n"
		"pushfl\n"
		"popl	%%eax\n"
		"xorl	%%edx, %%eax\n"
		"andl	$0x200000, %%eax"
		: "=a" (__eax)	// Output
		:		// Input
		: "edx", "cc"	// Clobber
		);
	return __eax;
#elif defined(_MSC_VER) && defined(RP_CPU_I386)
	// MSVC, i386
	int __eax;
	__asm {
		pushfd
		pop	eax
		mov	__eax, eax
		xor	eax, 0x200000
		push	eax
		popfd
		pushfd
		pop	eax
		xor	eax, __eax
		and	eax, 0x200000
		mov	__eax, eax
	}
	return __eax;
#elif !defined(__GNUC__) && !defined(_MSC_VER)
#  error Missing is_cpuid_supported() asm implementation for this compiler.
#else
	// AMD64. CPUID is always supported.
	return 1;
#endif
}

// gcc-5.0 no longer permanently reserves %ebx for PIC.
#if defined(__GNUC__) && !defined(__clang__) && defined(RP_CPU_I386)
#  if __GNUC__ < 5 && defined(__PIC__)
#    define ASM_RESERVE_EBX 1
#  endif
#endif

/**
 * Run the `cpuid` instruction.
 * @param level %eax
 * @param regs Registers (%eax, %ebx, %ecx, %edx)
 */
static FORCEINLINE void cpuid(unsigned int level, unsigned int regs[4])
{
#ifdef HAVE_CPUID_H
	// Use the compiler's __cpuid() macro.
	__cpuid(level, regs[0], regs[1], regs[2], regs[3]);
#elif defined(__GNUC__)
	// CPUID macro with PIC support.
	// See http://gcc.gnu.org/ml/gcc-patches/2007-09/msg00324.html
#  ifdef ASM_RESERVE_EBX
	__asm__ (
		"xchgl	%%ebx, %1\n"
		"cpuid\n"
		"xchgl	%%ebx, %1\n"
		: "=a" (regs[0]), "=r" (regs[1]), "=c" (regs[2]), "=d" (regs[3])
		: "0" (level)
		);
#  else /* !ASM_RESERVE_EBX */
	__asm__ (
		"cpuid\n"
		: "=a" (regs[0]), "=b" (regs[1]), "=c" (regs[2]), "=d" (regs[3])
		: "0" (level)
		);
#  endif
#elif defined(_MSC_VER)
#  if _MSC_VER >= 1400
	// CPUID for MSVC 2005+
	// Uses the __cpuid() intrinsic.
	__cpuid((int*)regs, level);
#  else /* _MSC_VER < 1400 */
	// CPUID for old MSVC that doesn't support intrinsics.
	// (TODO: Check MSVC 2002 and 2003?)
#    if defined(RP_CPU_AMD64)
#      error Cannot use inline assembly on 64-bit MSVC.
#    endif
	__asm {
		mov	eax, level
		cpuid
		mov	regs[0 * TYPE int], eax
		mov	regs[1 * TYPE int], ebx
		mov	regs[2 * TYPE int], ecx
		mov	regs[3 * TYPE int], edx
	}
#  endif
#else
#  error Missing 'cpuid' asm implementation for this compiler.
#endif
}

/**
 * Run the `cpuid` instruction, with 'count' parameter.
 * @param level %eax
 * @param count %ecx
 * @param regs Registers (%eax, %ebx, %ecx, %edx)
 */
static FORCEINLINE void cpuid_count(unsigned int level, unsigned int count, unsigned int regs[4])
{
#ifdef HAVE_CPUID_H
	// Use the compiler's __cpuid_count() macro.
	__cpuid_count(level, count, regs[0], regs[1], regs[2], regs[3]);
#elif defined(__GNUC__)
	// CPUID macro with PIC support.
	// See http://gcc.gnu.org/ml/gcc-patches/2007-09/msg00324.html
#  ifdef ASM_RESERVE_EBX
	__asm__ (
		"xchgl	%%ebx, %1\n"
		"cpuid\n"
		"xchgl	%%ebx, %1\n"
		: "=a" (regs[0]), "=r" (regs[1]), "=c" (regs[2]), "=d" (regs[3])
		: "0" (level), "2" (count)
		);
#  else /* !ASM_RESERVE_EBX */
	__asm__ (
		"cpuid\n"
		: "=a" (regs[0]), "=b" (regs[1]), "=c" (regs[2]), "=d" (regs[3])
		: "0" (level), "2" (count)
		);
#  endif
#elif defined(_MSC_VER)
#  if _MSC_VER >= 1400
	// CPUID for MSVC 2005+
	// Uses the __cpuid() intrinsic.
	__cpuidex((int*)regs, level, count);
#  else /* _MSC_VER < 1400 */
	// CPUID for old MSVC that doesn't support intrinsics.
	// (TODO: Check MSVC 2002 and 2003?)
#    if defined(RP_CPU_AMD64)
#      error Cannot use inline assembly on 64-bit MSVC.
#    endif
	__asm {
		mov	eax, level
		mov	ecx, count
		cpuid
		mov	regs[0 * TYPE int], eax
		mov	regs[1 * TYPE int], ebx
		mov	regs[2 * TYPE int], ecx
		mov	regs[3 * TYPE int], edx
	}
#  endif
#else
#  error Missing 'cpuid' asm implementation for this compiler.
#endif
}

// Register indexes
#define REG_EAX 0
#define REG_EBX 1
#define REG_ECX 2
#define REG_EDX 3

// IA32 CPU flags
// References:
// - Intel: http://download.intel.com/design/processor/applnots/24161832.pdf
// - AMD: http://www.amd.com/us-en/assets/content_type/white_papers_and_tech_docs/25481.pdf
// - Wikipedia:
//   - https://en.wikipedia.org/wiki/CPUID
//   - https://en.wikipedia.org/wiki/Control_register

// CR0.EM: FPU emulation.
#define IA32_CR0_EM		(1U << 2)

// CPUID function 1: Processor Info and Feature Bits

// Flags stored in the %edx register.
#define CPUFLAG_IA32_EDX_MMX		((uint32_t)(1U << 23))
#define CPUFLAG_IA32_EDX_FXSAVE		((uint32_t)(1U << 24))
#define CPUFLAG_IA32_EDX_SSE		((uint32_t)(1U << 25))
#define CPUFLAG_IA32_EDX_SSE2		((uint32_t)(1U << 26))

// Flags stored in the %ecx register.
#define CPUFLAG_IA32_ECX_SSE3		((uint32_t)(1U << 0))
#define CPUFLAG_IA32_ECX_SSSE3		((uint32_t)(1U << 9))
#define CPUFLAG_IA32_ECX_FMA3		((uint32_t)(1U << 12))
#define CPUFLAG_IA32_ECX_SSE41		((uint32_t)(1U << 19))
#define CPUFLAG_IA32_ECX_SSE42		((uint32_t)(1U << 20))
#define CPUFLAG_IA32_ECX_XSAVE		((uint32_t)(1U << 26))
#define CPUFLAG_IA32_ECX_OSXSAVE	((uint32_t)(1U << 27))
#define CPUFLAG_IA32_ECX_AVX		((uint32_t)(1U << 28))
#define CPUFLAG_IA32_ECX_F16C		((uint32_t)(1U << 29))

// CPUID function 7, %ecx=0: Extended Features

// Flags stored in the %ebx register.
#define CPUFLAG_IA32_FN7p0_EBX_AVX2	((uint32_t)(1U << 5))

// CPUID function 0x80000001: Extended Processor Info and Feature Bits

// Flags stored in the %edx register.
#define CPUFLAG_IA32_EXT_EDX_MMXEXT	((uint32_t)(1U << 22))
#define CPUFLAG_IA32_EXT_EDX_3DNOW	((uint32_t)(1U << 31))
#define CPUFLAG_IA32_EXT_EDX_3DNOWEXT	((uint32_t)(1U << 30))

// Flags stored in the %ecx register.
#define CPUFLAG_IA32_EXT_ECX_SSE4A	((uint32_t)(1U << 6))
#define CPUFLAG_IA32_EXT_ECX_XOP	((uint32_t)(1U << 11))
#define CPUFLAG_IA32_EXT_ECX_FMA4	((uint32_t)(1U << 16))

// CPUID functions.
#define CPUID_MAX_FUNCTIONS			((uint32_t)(0x00000000U))
#define CPUID_PROC_INFO_FEATURE_BITS		((uint32_t)(0x00000001U))
#define CPUID_EXT_FEATURES			((uint32_t)(0x00000007U))
#define CPUID_MAX_EXT_FUNCTIONS			((uint32_t)(0x80000000U))
#define CPUID_EXT_PROC_INFO_FEATURE_BITS	((uint32_t)(0x80000001U))
#define CPUID_EXT_PROC_BRAND_STRING_1		((uint32_t)(0x80000002U))
#define CPUID_EXT_PROC_BRAND_STRING_2		((uint32_t)(0x80000003U))
#define CPUID_EXT_PROC_BRAND_STRING_3		((uint32_t)(0x80000004U))
