/***************************************************************************
 * ROM Properties Page shell extension. (librpcpuid)                       *
 * cpuflags_arm.h: ARM CPU flags detection.                                *
 *                                                                         *
 * Copyright (c) 2017-2025 by David Korth.                                 *
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

/* CPU flags (arm/arm64) */
#if defined(RP_CPU_ARM) || defined(RP_CPU_ARM64)

// Set of CPU flags we check for right now.
// More flags will be added if needed.
#define RP_CPUFLAG_ARM_NEON		((uint32_t)(1U <<  0))

// Don't modify these!
extern uint32_t RP_CPU_Flags_arm;
extern int RP_CPU_Flags_arm_IsInit;	// 1 if RP_CPU_Flags_arm has been initialized.

/**
 * Initialize RP_CPU_Flags_arm.
 */
void RP_C_API RP_CPU_Flags_arm_Init(void);

// Convenience macros to determine if the CPU supports a certain flag.

// Macro for flags that need to be tested on both i386 and amd64 CPUs.
#define CPU_FLAG_ARM_CHECK(flag) \
static FORCEINLINE int RP_CPU_arm_Has##flag(void) \
{ \
	if (unlikely(!RP_CPU_Flags_arm_IsInit)) { \
		RP_CPU_Flags_arm_Init(); \
	} \
	return (int)(RP_CPU_Flags_arm & RP_CPUFLAG_ARM_##flag); \
}

// Macro for flags that always exist on amd64 and only need to be tested on i386.
#ifdef RP_CPU_ARM64
#  define CPU_FLAG_ARM_CHECK_arm32only(flag) \
static FORCEINLINE int RP_CPU_arm_Has##flag(void) \
{ \
	return 1; \
}
#else /* !RP_CPU_ARM64 */
#  define CPU_FLAG_ARM_CHECK_arm32only(flag) CPU_FLAG_ARM_CHECK(flag)
#endif /* RP_CPU_ARM64 */

CPU_FLAG_ARM_CHECK_arm32only(NEON)

#endif /* RP_CPU_ARM || RP_CPU_ARM64 */

#ifdef __cplusplus
}
#endif
