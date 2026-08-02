/***************************************************************************
 * ROM Properties Page shell extension. (librpcpuid)                       *
 * cpuflags_x86.c: x86 CPU flags detection.                                *
 *                                                                         *
 * Copyright (c) 2017-2026 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "cpuflags_x86.h"
#if !defined(RP_CPU_I386) && !defined(RP_CPU_AMD64)
#  error Do not compile cpuflags_x86.c on non-x86 CPUs!
#endif

// cpuid() macros
#include "cpuid_x86.h"

// pthread_once()
#ifdef _WIN32
#  include "pthread_once_win32.h"
#else /* !_WIN32 */
#  include <pthread.h>
#endif /* _WIN32 */

// C includes
#include <string.h>

uint32_t RP_CPU_Flags_x86 = 0;
int RP_CPU_Flags_x86_IsInit = 0;	// 1 if RP_CPU_Flags_x86 has been initialized.
RP_CPU_Info_x86_t RP_CPU_Info_x86;
static pthread_once_t cpu_once_control = PTHREAD_ONCE_INIT;

/**
 * Initialize RP_CPU_Flags. (internal function)
 * Called by pthread_once().
 */
static void RP_CPU_Flags_x86_Init_int(void)
{
	unsigned int regs[4];	// %eax, %ebx, %ecx, %edx
	uint8_t can_XSAVE = 0;

	// CPU info struct, to be filled in later.
	memset(&RP_CPU_Info_x86, 0, sizeof(RP_CPU_Info_x86));

#ifdef RP_CPU_I386
	// i386 is not guaranteed to support FXSAVE. (required for SSE)
	uint8_t can_FXSAVE = 0;

	// Check if cpuid is supported.
	if (!is_cpuid_supported()) {
		// CPUID is not supported.
		// This CPU must be an early 486 or older.
		RP_CPU_Flags_x86_IsInit = 1;
		return;
	}

	// Initialize the CPU flags variable.
	RP_CPU_Flags_x86 = 0;
#else /* !RP_CPU_I386 */
	// amd64 *is* guaranteed to support FXSAVE.
	static const uint8_t can_FXSAVE = 1;

	// Initialize the CPU flags variable.
	// amd64 is guaranteed to support MMX, SSE, and SSE2.
	RP_CPU_Flags_x86 = (RP_CPUFLAG_x86_MMX | RP_CPUFLAG_x86_SSE | RP_CPUFLAG_x86_SSE2);
#endif /* RP_CPU_I386 */

	// CPUID is supported.
	// Check if the CPUID Features function (Function 1) is supported.
	// This also retrieves the CPU vendor string.
	cpuid(CPUID_MAX_FUNCTIONS, regs);
	RP_CPU_Info_x86.highest_fn = regs[REG_EAX];
	RP_CPU_Info_x86.manufacturer_id.u32[0] = regs[REG_EBX];
	RP_CPU_Info_x86.manufacturer_id.u32[1] = regs[REG_EDX];
	RP_CPU_Info_x86.manufacturer_id.u32[2] = regs[REG_ECX];

	if (RP_CPU_Info_x86.highest_fn < CPUID_PROC_INFO_FEATURE_BITS) {
		// No CPUID functions are supported.
		// NOTE: Skipping extended functions. (>= 0x80000000)
		RP_CPU_Flags_x86_IsInit = 1;
		return;
	}

	// Get the processor info and feature bits.
	cpuid(CPUID_PROC_INFO_FEATURE_BITS, regs);
	RP_CPU_Info_x86.version.u32 = regs[REG_EAX];

#ifdef RP_CPU_I386
	if (regs[REG_EDX] & CPUFLAG_IA32_EDX_MMX) {
		// MMX is supported.
		// NOTE: Not officially supported on amd64 in 64-bit,
		// but all known implementations support it.
		RP_CPU_Flags_x86 |= RP_CPUFLAG_x86_MMX;
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
#endif /* RP_CPU_I386 */

	// Check for other SSE instruction sets.
	if (can_FXSAVE) {
#ifdef RP_CPU_I386
		if (regs[REG_EDX] & CPUFLAG_IA32_EDX_SSE) {
			// this check is *probably* not needed?
			RP_CPU_Flags_x86 |= CPUFLAG_IA32_EDX_SSE;
		}
		if (regs[REG_EDX] & CPUFLAG_IA32_EDX_SSE2) {
			RP_CPU_Flags_x86 |= RP_CPUFLAG_x86_SSE2;
		}
#endif /* RP_CPU_I386 */
		if (regs[REG_ECX] & CPUFLAG_IA32_ECX_SSE3) {
			RP_CPU_Flags_x86 |= RP_CPUFLAG_x86_SSE3;
		}
		if (regs[REG_ECX] & CPUFLAG_IA32_ECX_SSSE3) {
			RP_CPU_Flags_x86 |= RP_CPUFLAG_x86_SSSE3;
		}
		if (regs[REG_ECX] & CPUFLAG_IA32_ECX_SSE41) {
			RP_CPU_Flags_x86 |= RP_CPUFLAG_x86_SSE41;
		}
		if (regs[REG_ECX] & CPUFLAG_IA32_ECX_SSE42) {
			RP_CPU_Flags_x86 |= RP_CPUFLAG_x86_SSE42;
		}
		if (regs[REG_ECX] & CPUFLAG_IA32_ECX_AES) {
			RP_CPU_Flags_x86 |= RP_CPUFLAG_x86_AES;
		}
		if (regs[REG_ECX] & CPUFLAG_IA32_ECX_F16C) {
			RP_CPU_Flags_x86 |= RP_CPUFLAG_x86_F16C;
		}
		if (regs[REG_ECX] & CPUFLAG_IA32_ECX_FMA3) {
			RP_CPU_Flags_x86 |= RP_CPUFLAG_x86_FMA3;
		}
	}

	// Check for XSAVE and OSXSAVE.
	// Required for AVX, AVX2, and APX.
	can_XSAVE = (regs[REG_ECX] & (CPUFLAG_IA32_ECX_XSAVE | CPUFLAG_IA32_ECX_OSXSAVE)) ==
	                             (CPUFLAG_IA32_ECX_XSAVE | CPUFLAG_IA32_ECX_OSXSAVE);
	if (can_XSAVE) {
		// XSAVE and OSXSAVE are set.
		if (regs[REG_ECX] & CPUFLAG_IA32_ECX_AVX) {
			RP_CPU_Flags_x86 |= RP_CPUFLAG_x86_AVX;
		}
	}

	// Get extended features, including AVX2.
	// NOTE: AVX2 and APX both require XSAVE.
	if (RP_CPU_Info_x86.highest_fn >= CPUID_EXT_FEATURES) {
		cpuid_count(CPUID_EXT_FEATURES, 0, regs);

		if (regs[REG_EBX] & CPUFLAG_IA32_FN7p0_EBX_BMI1) {
			RP_CPU_Flags_x86 |= RP_CPUFLAG_x86_BMI1;
		}
		if (regs[REG_EBX] & CPUFLAG_IA32_FN7p0_EBX_BMI2) {
			RP_CPU_Flags_x86 |= RP_CPUFLAG_x86_BMI2;
		}
		if (regs[REG_EBX] & CPUFLAG_IA32_FN7p0_EBX_SHA) {
			RP_CPU_Flags_x86 |= RP_CPUFLAG_x86_SHA;
		}

		if (can_XSAVE) {
			if (can_XSAVE && (regs[REG_EBX] & CPUFLAG_IA32_FN7p0_EBX_AVX2)) {
				RP_CPU_Flags_x86 |= RP_CPUFLAG_x86_AVX2;
			}

			// NOTE: APX is 64-bit only.
#ifdef RP_CPU_AMD64
			cpuid_count(CPUID_EXT_FEATURES, 1, regs);
			if (regs[REG_EDX] & CPUFLAG_IA32_FN7p1_EDX_APX) {
				RP_CPU_Flags_x86 |= RP_CPUFLAG_x86_APX;
			}
#endif /* RP_CPU_AMD64 */
		}
	}

	// Check the maximum number of Extended Functions.
	cpuid(CPUID_MAX_EXT_FUNCTIONS, regs);
	RP_CPU_Info_x86.highest_ext_fn = regs[REG_EAX];

	if (RP_CPU_Info_x86.highest_ext_fn >= CPUID_EXT_PROC_BRAND_STRING_3) {
		// Get the brand string.
		cpuid(CPUID_EXT_PROC_BRAND_STRING_1, regs);
		memcpy(&RP_CPU_Info_x86.brand_string.u32[0], regs, sizeof(regs));
		cpuid(CPUID_EXT_PROC_BRAND_STRING_2, regs);
		memcpy(&RP_CPU_Info_x86.brand_string.u32[4], regs, sizeof(regs));
		cpuid(CPUID_EXT_PROC_BRAND_STRING_3, regs);
		memcpy(&RP_CPU_Info_x86.brand_string.u32[8], regs, sizeof(regs));
	}

	// CPU flags initialized.
	RP_CPU_Flags_x86_IsInit = 1;
}

/**
 * Initialize RP_CPU_Flags.
 */
void RP_C_API RP_CPU_Flags_x86_Init(void)
{
	pthread_once(&cpu_once_control, RP_CPU_Flags_x86_Init_int);
}
