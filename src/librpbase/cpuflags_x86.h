/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * cpuflags_x86.h: x86 CPU flags detection.                                *
 *                                                                         *
 * Copyright (c) 2017 by David Korth.                                      *
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
 * You should have received a copy of the GNU General Public License along *
 * with this program; if not, write to the Free Software Foundation, Inc., *
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.           *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_LIBRPBASE_CPUFLAGS_X86_H__
#define __ROMPROPERTIES_LIBRPBASE_CPUFLAGS_X86_H__

#include <stdint.h>
#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

/* CPU flags (IA32/x86_64) */
#if defined(__i386__) || defined(__amd64__) || defined(__x86_64__) || \
    defined(_M_IX86) || defined(_M_X64)

// Set of CPU flags we check for right now.
// More flags will be added if needed.
#define RP_CPUFLAG_X86_MMX		((uint32_t)(1U << 0))
#define RP_CPUFLAG_X86_SSE		((uint32_t)(1U << 1))
#define RP_CPUFLAG_X86_SSE2		((uint32_t)(1U << 2))
#define RP_CPUFLAG_X86_SSE3		((uint32_t)(1U << 3))
#define RP_CPUFLAG_X86_SSSE3		((uint32_t)(1U << 4))
#define RP_CPUFLAG_X86_SSE41		((uint32_t)(1U << 5))
#define RP_CPUFLAG_X86_SSE42		((uint32_t)(1U << 6))

#endif /* defined(__i386__) || defined(__amd64__) || defined(__x86_64__) */

// Don't modify these!
extern uint32_t RP_CPU_Flags;
extern int RP_CPU_Flags_Init;	// 1 if RP_CPU_Flags has been initialized.

/**
 * Initialize RP_CPU_Flags.
 */
void RP_CPU_InitCPUFlags(void);

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
	return (RP_CPU_Flags & RP_CPUFLAG_X86_MMX);
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
	return (RP_CPU_Flags & RP_CPUFLAG_X86_SSE2);
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
	return (RP_CPU_Flags & RP_CPUFLAG_X86_SSSE3);
}

#ifdef __cplusplus
}
#endif

#endif /* __ROMPROPERTIES_LIBRPBASE_CPUFLAGS_X86_H__ */
