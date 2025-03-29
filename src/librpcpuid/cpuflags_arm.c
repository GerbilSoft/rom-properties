/***************************************************************************
 * ROM Properties Page shell extension. (librpcpuid)                       *
 * cpuflags_arm.c: ARM CPU flags detection.                                *
 *                                                                         *
 * Copyright (c) 2017-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "cpuflags_arm.h"
#if !defined(RP_CPU_ARM) && !defined(RP_CPU_ARM64)
#  error Do not compile cpuflags_arm.c on non-ARM CPUs!
#endif

// librpthreads
// FIXME: Cannot use librpthreads at the moment due to static linkage.
//#include "librpthreads/pthread_once.h"

uint32_t RP_CPU_Flags_arm = 0;
int RP_CPU_Flags_arm_IsInit = 0;	// 1 if RP_CPU_Flags_arm has been initialized.

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
	// TODO: Detect ARM NEON.
	// OS-specific stuff goes here.
	RP_CPU_Flags_arm = 0;
#endif

	// CPU flags initialized.
	RP_CPU_Flags_arm_IsInit = 1;
}

/**
 * Initialize RP_CPU_Flags.
 */
void RP_C_API RP_CPU_Flags_arm_Init(void)
{
	// FIXME: Cannot use librpthreads at the moment due to static linkage.
	//static pthread_once_t cpu_once_control = PTHREAD_ONCE_INIT;
	//pthread_once(&cpu_once_control, RP_CPU_Flags_arm_Init_int);
	static uint8_t cpu_once_control = 0;
	if (cpu_once_control == 0) {
		RP_CPU_Flags_arm_Init_int();
		cpu_once_control = 1;
	}
}
