/***************************************************************************
 * ROM Properties Page shell extension. (librpcpuid)                       *
 * cpuflags_arm.c: ARM CPU flags detection.                                *
 *                                                                         *
 * Copyright (c) 2017-2026 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "cpuflags_arm.h"
#if !defined(RP_CPU_ARM) && !defined(RP_CPU_ARM64)
#  error Do not compile cpuflags_arm.c on non-ARM CPUs!
#endif
#include <stdio.h>
#ifdef __linux__
#  include <sys/auxv.h>
#  include <asm/hwcap.h>
#endif /* __linux__ */

// pthread_once()
#ifdef _WIN32
#  include "pthread_once_win32.h"
#else /* !_WIN32 */
#  include <pthread.h>
#endif /* _WIN32 */

uint32_t RP_CPU_Flags_arm = 0;
int RP_CPU_Flags_arm_IsInit = 0;	// 1 if RP_CPU_Flags_arm has been initialized.
static pthread_once_t cpu_once_control = PTHREAD_ONCE_INIT;

/**
 * Initialize RP_CPU_Flags. (internal function)
 * Called by pthread_once().
 */
static void RP_CPU_Flags_arm_Init_int(void)
{
#if defined(RP_CPU_ARM64) || (defined(RP_CPU_ARM) && defined(_WIN32))
	// ARM NEON is always available on arm64.
	// Windows on ARM also always has NEON. (desktop Windows, e.g. Windows RT [Win8])
	RP_CPU_Flags_arm = RP_CPUFLAG_ARM_NEON;
#elif defined(RP_CPU_ARM)
	// Initialize the CPU flags variable.
	RP_CPU_Flags_arm = 0;

	// Detect ARM NEON.
#  ifdef __linux__
	// Linux: Check HWCAP.
	// FIXME: Might not be correct (or available) on Android. Needs testing!
	const unsigned long hwcap = getauxval(AT_HWCAP);
	if (hwcap & HWCAP_NEON) {
		RP_CPU_Flags_arm |= RP_CPUFLAG_ARM_NEON;
	}
#  else
#    warning No CPU flag detection for this OS on ARM
#  endif
#endif

	// CPU flags initialized.
	RP_CPU_Flags_arm_IsInit = 1;
}

/**
 * Initialize RP_CPU_Flags.
 */
void RP_C_API RP_CPU_Flags_arm_Init(void)
{
	pthread_once(&cpu_once_control, RP_CPU_Flags_arm_Init_int);
}
