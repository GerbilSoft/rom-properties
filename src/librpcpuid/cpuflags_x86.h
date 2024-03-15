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

#ifdef __cplusplus
extern "C" {
#endif

/* CPU flags (IA32/x86_64) */
#if defined(_M_IX86) || defined(__i386__) || \
    defined(_M_X64) || defined(_M_AMD64) || defined(__amd64__) || defined(__x86_64__)

// Set of CPU flags we check for right now.
// More flags will be added if needed.
#define RP_CPUFLAG_X86_MMX		((uint32_t)(1U << 0))
#define RP_CPUFLAG_X86_SSE		((uint32_t)(1U << 1))
#define RP_CPUFLAG_X86_SSE2		((uint32_t)(1U << 2))
#define RP_CPUFLAG_X86_SSE3		((uint32_t)(1U << 3))
#define RP_CPUFLAG_X86_SSSE3		((uint32_t)(1U << 4))
#define RP_CPUFLAG_X86_SSE41		((uint32_t)(1U << 5))
#define RP_CPUFLAG_X86_SSE42		((uint32_t)(1U << 6))

#endif /* _M_IX86) || __i386__ || _M_X64 || _M_AMD64 || __amd64__ || __x86_64__ */

// Don't modify these!
extern uint32_t RP_CPU_Flags;
extern int RP_CPU_Flags_Init;	// 1 if RP_CPU_Flags has been initialized.

/**
 * Initialize RP_CPU_Flags.
 */
void RP_C_API RP_CPU_InitCPUFlags(void);

/**
 * Check if the CPU supports MMX.
 * @return Non-zero if MMX is supported; 0 if not.
 */
static FORCEINLINE int RP_CPU_HasMMX(void)
{
#if defined(__amd64__) || defined(__x86_64__) || defined(_M_X64)
	// 64-bit always has MMX.
	return 1;
#else
	if (unlikely(!RP_CPU_Flags_Init)) {
		RP_CPU_InitCPUFlags();
	}
	return (int)(RP_CPU_Flags & RP_CPUFLAG_X86_MMX);
#endif
}

/**
 * Check if the CPU supports SSE2.
 * @return Non-zero if SSE2 is supported; 0 if not.
 */
static FORCEINLINE int RP_CPU_HasSSE2(void)
{
#if defined(__amd64__) || defined(__x86_64__) || defined(_M_X64)
	// 64-bit always has SSE2.
	return 1;
#else
	if (unlikely(!RP_CPU_Flags_Init)) {
		RP_CPU_InitCPUFlags();
	}
	return (int)(RP_CPU_Flags & RP_CPUFLAG_X86_SSE2);
#endif
}

/**
 * Check if the CPU supports SSSE3.
 * @return Non-zero if SSSE3 is supported; 0 if not.
 */
static FORCEINLINE int RP_CPU_HasSSSE3(void)
{
	if (unlikely(!RP_CPU_Flags_Init)) {
		RP_CPU_InitCPUFlags();
	}
	return (int)(RP_CPU_Flags & RP_CPUFLAG_X86_SSSE3);
}

/**
 * Check if the CPU supports SSE4.1.
 * @return Non-zero if SSE4.1 is supported; 0 if not.
 */
static FORCEINLINE int RP_CPU_HasSSE41(void)
{
	if (unlikely(!RP_CPU_Flags_Init)) {
		RP_CPU_InitCPUFlags();
	}
	return (int)(RP_CPU_Flags & RP_CPUFLAG_X86_SSE41);
}

#ifdef __cplusplus
}
#endif
