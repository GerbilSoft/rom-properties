/***************************************************************************
 * ROM Properties Page shell extension. (librpcpuid)                       *
 * cpuflags_x86.c: x86 CPU flags detection.                                *
 *                                                                         *
 * Copyright (c) 2017-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "cpuflags_x86.h"
#if !defined(RP_CPU_I386) && !defined(RP_CPU_AMD64)
#  error Do not compile cpuflags_x86.c on non-x86 CPUs!
#endif

// cpuid() macros
#include "cpuid_x86.h"

// librpthreads
// FIXME: Cannot use librpthreads at the moment due to static linkage.
//#include "librpthreads/pthread_once.h"

uint32_t RP_CPU_Flags = 0;
int RP_CPU_Flags_Init = 0;	// 1 if RP_CPU_Flags has been initialized.

/**
 * Initialize RP_CPU_Flags. (internal function)
 * Called by pthread_once().
 */
static void RP_CPU_InitCPUFlags_int(void)
{
	unsigned int regs[4];	// %eax, %ebx, %ecx, %edx
	unsigned int maxFunc;
#ifdef RP_CPU_I386
	// i386 is not guaranteed to support FXSAVE. (required for SSE)
	uint8_t can_FXSAVE = 0;
#else /* !RP_CPU_I386 */
	// amd64 *is* guaranteed to support FXSAVE.
	static const uint8_t can_FXSAVE = 1;
#endif /* RP_CPU_I386 */
	uint8_t can_XSAVE = 0;

	// Make sure the CPU flags variable is empty.
	RP_CPU_Flags = 0;

	// Check if cpuid is supported.
	if (!is_cpuid_supported()) {
		// CPUID is not supported.
		// This CPU must be an early 486 or older.
		RP_CPU_Flags_Init = 1;
		return;
	}

	// CPUID is supported.
	// Check if the CPUID Features function (Function 1) is supported.
	// This also retrieves the CPU vendor string. (currently unused)
	cpuid(CPUID_MAX_FUNCTIONS, regs);
	maxFunc = regs[0];
	if (maxFunc < CPUID_PROC_INFO_FEATURE_BITS) {
		// No CPUID functions are supported.
		RP_CPU_Flags_Init = 1;
		return;
	}

	// Get the processor info and feature bits.
	cpuid(CPUID_PROC_INFO_FEATURE_BITS, regs);

#ifdef RP_CPU_I386
	if (regs[REG_EDX] & CPUFLAG_IA32_EDX_MMX) {
		// MMX is supported.
		// NOTE: Not officially supported on amd64 in 64-bit,
		// but all known implementations support it.
		RP_CPU_Flags |= RP_CPUFLAG_X86_MMX;
	}

	if (regs[REG_EDX] & CPUFLAG_IA32_EDX_SSE) {
		// CPU reports that it supports SSE, but the OS
		// might not support FXSAVE.

		// Check if this CPU supports FXSAVE with SSE.
		if (regs[REG_EDX] & CPUFLAG_IA32_EDX_FXSAVE) {
			// CPU supports FXSAVE.

#  ifdef _WIN32
			// Windows 95 does not support SSE.
			// Windows NT 4.0 supports SSE if the
			// appropriate driver is installed.

			// Check if CR0.EM == 0.
			unsigned int __smsw;
#    if defined(__GNUC__)
			__asm__ (
				"smsw	%0"
				: "=r" (__smsw)
				);
#    elif defined(_MSC_VER)
			// TODO: Optimize the MSVC version to
			// not use the stack?
			__asm	smsw	__smsw
#    else
#      error Missing 'smsw' asm implementation for this compiler.
#    endif
			if (!(__smsw & IA32_CR0_EM)) {
				// FPU emulation is disabled.
				// SSE is enabled by the OS.
				can_FXSAVE = 1;
			}
#  else /* !_WIN32 */
			// For non-Windows operating systems, we'll assume
			// the OS supports SSE. Valgrind doesn't like the
			// 'smsw' instruction, so we can't do memory debugging
			// with Valgrind if we use 'smsw'.
			can_FXSAVE = 1;
#  endif /* _WIN32 */
		}
	}
#else /* !RP_CPU_I386 */
	// amd64: MMX *does* function, but use is not recommended.
	// Use SSE or SSE2 instead on 64-bit.
	// Also, SSE and SSE2 are always supported on amd64.
	RP_CPU_Flags |= (RP_CPUFLAG_X86_MMX | RP_CPUFLAG_X86_SSE | RP_CPUFLAG_X86_SSE2);
#endif /* RP_CPU_I386 */

	// Check for other SSE instruction sets.
	if (can_FXSAVE) {
#ifdef RP_CPU_I386
		if (regs[REG_EDX] & CPUFLAG_IA32_EDX_SSE)	// this check is *probably* not needed?
			RP_CPU_Flags |= CPUFLAG_IA32_EDX_SSE;
		if (regs[REG_EDX] & CPUFLAG_IA32_EDX_SSE2)
			RP_CPU_Flags |= RP_CPUFLAG_X86_SSE2;
#endif /* RP_CPU_I386 */
		if (regs[REG_ECX] & CPUFLAG_IA32_ECX_SSE3)
			RP_CPU_Flags |= RP_CPUFLAG_X86_SSE3;
		if (regs[REG_ECX] & CPUFLAG_IA32_ECX_SSSE3)
			RP_CPU_Flags |= RP_CPUFLAG_X86_SSSE3;
		if (regs[REG_ECX] & CPUFLAG_IA32_ECX_SSE41)
			RP_CPU_Flags |= RP_CPUFLAG_X86_SSE41;
		if (regs[REG_ECX] & CPUFLAG_IA32_ECX_SSE42)
			RP_CPU_Flags |= RP_CPUFLAG_X86_SSE42;
		if (regs[REG_ECX] & CPUFLAG_IA32_ECX_F16C)
			RP_CPU_Flags |= RP_CPUFLAG_X86_F16C;
		if (regs[REG_ECX] & CPUFLAG_IA32_ECX_FMA3)
			RP_CPU_Flags |= RP_CPUFLAG_X86_FMA3;
	}

	// Check for XSAVE and OSXSAVE.
	// Required for AVX and AVX2.
	can_XSAVE = (regs[REG_ECX] & (CPUFLAG_IA32_ECX_XSAVE | CPUFLAG_IA32_ECX_OSXSAVE)) ==
	                             (CPUFLAG_IA32_ECX_XSAVE | CPUFLAG_IA32_ECX_OSXSAVE);
	if (can_XSAVE) {
		// XSAVE and OSXSAVE are set.
		if (regs[REG_ECX] & CPUFLAG_IA32_ECX_AVX)
			RP_CPU_Flags |= RP_CPUFLAG_X86_AVX;
	}

	// Get extended features, including AVX2.
	// NOTE: AVX2 requires XSAVE.
	if (can_XSAVE && maxFunc >= CPUID_EXT_FEATURES) {
		cpuid_count(CPUID_EXT_FEATURES, 0, regs);

		if (regs[REG_EBX] & CPUFLAG_IA32_FN7p0_EBX_AVX2)
			RP_CPU_Flags |= RP_CPUFLAG_X86_AVX2;
	}

	// CPU flags initialized.
	RP_CPU_Flags_Init = 1;
}

/**
 * Initialize RP_CPU_Flags.
 */
void RP_C_API RP_CPU_InitCPUFlags(void)
{
	// FIXME: Cannot use librpthreads at the moment due to static linkage.
	//static pthread_once_t cpu_once_control = PTHREAD_ONCE_INIT;
	//pthread_once(&cpu_once_control, RP_CPU_InitCPUFlags_int);
	static uint8_t cpu_once_control = 0;
	if (cpu_once_control == 0) {
		RP_CPU_InitCPUFlags_int();
		cpu_once_control = 1;
	}
}
