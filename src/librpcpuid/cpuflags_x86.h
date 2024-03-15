/***************************************************************************
 * ROM Properties Page shell extension. (librpcpuid)                         *
 * cpuflags_x86.h: x86 CPU flags detection.                                *
 *                                                                         *
 * Copyright (c) 2017-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include <stdint.h>
#include "common.h"
#include "dll-macros.h"	// for RP_C_API

// RP_CPU_* macros
#include "cpu_dispatch.h"

#ifdef __cplusplus
extern "C" {
#endif

/* CPU flags (IA32/x86_64) */
#if defined(RP_CPU_I386) || defined(RP_CPU_AMD64)

// Set of CPU flags we check for right now.
// More flags will be added if needed.
#define RP_CPUFLAG_X86_MMX		((uint32_t)(1U <<  0))
#define RP_CPUFLAG_X86_SSE		((uint32_t)(1U <<  1))
#define RP_CPUFLAG_X86_SSE2		((uint32_t)(1U <<  2))
#define RP_CPUFLAG_X86_SSE3		((uint32_t)(1U <<  3))
#define RP_CPUFLAG_X86_SSSE3		((uint32_t)(1U <<  4))
#define RP_CPUFLAG_X86_SSE41		((uint32_t)(1U <<  5))
#define RP_CPUFLAG_X86_SSE42		((uint32_t)(1U <<  6))
#define RP_CPUFLAG_X86_AVX		((uint32_t)(1U <<  7))
#define RP_CPUFLAG_X86_AVX2		((uint32_t)(1U <<  8))
#define RP_CPUFLAG_X86_F16C		((uint32_t)(1U <<  9))
#define RP_CPUFLAG_X86_FMA3		((uint32_t)(1U << 10))

#endif /* RP_CPU_I386 || RP_CPU_AMD64 */

// Don't modify these!
extern uint32_t RP_CPU_Flags;
extern int RP_CPU_Flags_Init;	// 1 if RP_CPU_Flags has been initialized.

/**
 * Initialize RP_CPU_Flags.
 */
void RP_C_API RP_CPU_InitCPUFlags(void);

// Convenience macros to determine if the CPU supports a certain flag.

// Macro for flags that need to be tested on both i386 and amd64 CPUs.
#define CPU_FLAG_X86_CHECK(flag) \
static FORCEINLINE int RP_CPU_Has##flag(void) \
{ \
	if (unlikely(!RP_CPU_Flags_Init)) { \
		RP_CPU_InitCPUFlags(); \
	} \
	return (int)(RP_CPU_Flags & RP_CPUFLAG_X86_##flag); \
}

// Macro for flags that always exist on amd64 and only need to be tested on i386.
#ifdef RP_CPU_AMD64
#  define CPU_FLAG_X86_CHECK_i386only(flag) \
static FORCEINLINE int RP_CPU_Has##flag(void) \
{ \
	return 1; \
}
#else /* !RP_CPU_AMD64 */
#  define CPU_FLAG_X86_CHECK_i386only(flag) CPU_FLAG_X86_CHECK(flag)
#endif /* RP_CPU_AMD64 */

CPU_FLAG_X86_CHECK_i386only(MMX)
CPU_FLAG_X86_CHECK_i386only(SSE)
CPU_FLAG_X86_CHECK_i386only(SSE2)
CPU_FLAG_X86_CHECK(SSE3)
CPU_FLAG_X86_CHECK(SSSE3)
CPU_FLAG_X86_CHECK(SSE41)
CPU_FLAG_X86_CHECK(SSE42)
CPU_FLAG_X86_CHECK(AVX)
CPU_FLAG_X86_CHECK(AVX2)
CPU_FLAG_X86_CHECK(F16C)
CPU_FLAG_X86_CHECK(FMA3)

#ifdef __cplusplus
}
#endif
