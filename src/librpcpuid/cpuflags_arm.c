/***************************************************************************
 * ROM Properties Page shell extension. (librpcpuid)                       *
 * cpuflags_arm.c: ARM CPU flags detection.                                *
 *                                                                         *
 * Copyright (c) 2017-2026 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "config.librpcpuid.h"

#include "cpuflags_arm.h"
#if !defined(RP_CPU_ARM) && !defined(RP_CPU_ARM64)
#  error Do not compile cpuflags_arm.c on non-ARM CPUs!
#endif
#include <stdio.h>

#ifdef HAVE_GETAUXVAL
#  include <sys/auxv.h>
#  include <asm/hwcap.h>

// ARM64 HWCAPs defined after 2016 which may not be present in Ubuntu 16.04.
#  ifdef RP_CPU_ARM64
#    ifndef HWCAP_SHA3
#      define HWCAP_SHA3	(1 << 17)
#    endif
#    ifndef HWCAP_SHA512
#      define HWCAP_SHA512	(1 << 21)
#    endif
#    ifndef HWCAP_SVE
#      define HWCAP_SVE		(1 << 22)
#    endif
#    ifndef HWCAP_SVE2P2
#      define HWCAP_SVE2P2	(1UL << 41)
#    endif
#    ifndef HWCAP2_SVE2
#      define HWCAP2_SVE2	(1 << 1)
#    endif
#    ifndef HWCAP2_SVE2P1
#      define HWCAP2_SVE2P1	(1UL << 36)
#    endif
#  endif /* RP_CPU_ARM64 */
#endif /* HAVE_GETAUXVAL */

// Windows headers for IsProcessorFeaturePresent()
#ifdef _WIN32
#  define WIN32_LEAN_AND_MEAN 1
#  include <windows.h>

// Processor features that might not be defined in older Windows SDKs.
#  ifndef PF_ARM_NEON_INSTRUCTIONS_AVAILABLE
#    define PF_ARM_NEON_INSTRUCTIONS_AVAILABLE 19
#  endif
#  ifndef PF_ARM_V8_CRYPTO_INSTRUCTIONS_AVAILABLE
#    define PF_ARM_V8_CRYPTO_INSTRUCTIONS_AVAILABLE 30
#  endif
#  ifndef PF_ARM_V8_CRC32_INSTRUCTIONS_AVAILABLE
#    define PF_ARM_V8_CRC32_INSTRUCTIONS_AVAILABLE 31
#  endif
#  ifndef PF_ARM_SHA3_INSTRUCTIONS_AVAILABLE
#    define PF_ARM_SHA3_INSTRUCTIONS_AVAILABLE 64
#  endif
#  ifndef PF_ARM_SHA512_INSTRUCTIONS_AVAILABLE
#    define PF_ARM_SHA512_INSTRUCTIONS_AVAILABLE 65
#  endif
#  ifndef PF_ARM_SVE_INSTRUCTIONS_AVAILABLE
#    define PF_ARM_SVE_INSTRUCTIONS_AVAILABLE 46
#  endif
#  ifndef PF_ARM_SVE2_INSTRUCTIONS_AVAILABLE
#    define PF_ARM_SVE2_INSTRUCTIONS_AVAILABLE 47
#  endif
#  ifndef PF_ARM_SVE2_1_INSTRUCTIONS_AVAILABLE
#    define PF_ARM_SVE2_1_INSTRUCTIONS_AVAILABLE 48
#  endif
#endif

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
	RP_CPU_Flags_arm = 0;

#if defined(HAVE_GETAUXVAL)
	// glibc: Use getauxval() to get CPU information.

	// Check HWCAP.
	const unsigned long hwcap = getauxval(AT_HWCAP);
	const unsigned long hwcap2 = getauxval(AT_HWCAP2);

#  ifdef RP_CPU_ARM64
	// 64-bit ARM: HWCAP_ASIMD (NEON) must *always* be set.
	assert(hwcap & HWCAP_ASIMD);

	if (hwcap & HWCAP_ASIMD) {
		RP_CPU_Flags_arm |= RP_CPUFLAG_ARM_NEON;
	}
	if (hwcap & HWCAP_AES) {
		RP_CPU_Flags_arm |= RP_CPUFLAG_ARM_AES;
	}
	if (hwcap & HWCAP_SHA1) {
		RP_CPU_Flags_arm |= RP_CPUFLAG_ARM_SHA1;
	}
	if (hwcap & HWCAP_SHA2) {
		RP_CPU_Flags_arm |= RP_CPUFLAG_ARM_SHA2;
	}
	if (hwcap & HWCAP_CRC32) {
		RP_CPU_Flags_arm |= RP_CPUFLAG_ARM_CRC32;
	}
	if (hwcap & HWCAP_SHA3) {
		RP_CPU_Flags_arm |= RP_CPUFLAG_ARM_SHA3;
	}
	if (hwcap & HWCAP_SHA512) {
		RP_CPU_Flags_arm |= RP_CPUFLAG_ARM_SHA512;
	}
	if (hwcap & HWCAP_SVE) {
		RP_CPU_Flags_arm |= RP_CPUFLAG_ARM_SVE;
	}
	if (hwcap2 & HWCAP2_SVE2) {
		RP_CPU_Flags_arm |= RP_CPUFLAG_ARM_SVE2;
	}
	if (hwcap2 & HWCAP2_SVE2P1) {
		RP_CPU_Flags_arm |= RP_CPUFLAG_ARM_SVE2P1;
	}
	if (hwcap & HWCAP_SVE2P2) {
		RP_CPU_Flags_arm |= RP_CPUFLAG_ARM_SVE2P2;
	}
#  else /* RP_CPU_ARM */
	if (hwcap & HWCAP_NEON) {
		RP_CPU_Flags_arm |= RP_CPUFLAG_ARM_NEON;
	}
	if (hwcap2 & HWCAP2_AES) {
		RP_CPU_Flags_arm |= RP_CPUFLAG_ARM_AES;
	}
	if (hwcap2 & HWCAP2_SHA1) {
		RP_CPU_Flags_arm |= RP_CPUFLAG_ARM_SHA1;
	}
	if (hwcap2 & HWCAP2_SHA2) {
		RP_CPU_Flags_arm |= RP_CPUFLAG_ARM_SHA2;
	}
	if (hwcap2 & HWCAP2_CRC32) {
		RP_CPU_Flags_arm |= RP_CPUFLAG_ARM_CRC32;
	}

	// SHA-3, SHA-512, SVE, and SVE2 are not available on 32-bit.
#  endif
#elif defined(_WIN32)
	// Windows: Use IsProcessorFeaturePresent().
	// NOTE: Some of these processor features have SVE and standard versions available.
	// We're only checking standard versions.

	// NEON instructions *must* be available on desktop Windows for ARM.
	assert(IsProcessorFeaturePresent(PF_ARM_NEON_INSTRUCTIONS_AVAILABLE));

	if (IsProcessorFeaturePresent(PF_ARM_NEON_INSTRUCTIONS_AVAILABLE)) {
		RP_CPU_Flags_arm |= RP_CPUFLAG_ARM_NEON;
	}
	if (IsProcessorFeaturePresent(PF_ARM_V8_CRYPTO_INSTRUCTIONS_AVAILABLE)) {
		// NOTE: This covers AES, SHA-1, and SHA-2.
		RP_CPU_Flags_arm |= RP_CPUFLAG_ARM_AES |
		                    RP_CPUFLAG_ARM_SHA1 |
		                    RP_CPUFLAG_ARM_SHA2;
	}
	if (IsProcessorFeaturePresent(PF_ARM_V8_CRC32_INSTRUCTIONS_AVAILABLE)) {
		RP_CPU_Flags_arm |= RP_CPUFLAG_ARM_CRC32;
	}
	if (IsProcessorFeaturePresent(PF_ARM_SHA3_INSTRUCTIONS_AVAILABLE)) {
		RP_CPU_Flags_arm |= RP_CPUFLAG_ARM_SHA3;
	}
	if (IsProcessorFeaturePresent(PF_ARM_SHA512_INSTRUCTIONS_AVAILABLE)) {
		RP_CPU_Flags_arm |= RP_CPUFLAG_ARM_SHA512;
	}
	if (IsProcessorFeaturePresent(PF_ARM_SVE_INSTRUCTIONS_AVAILABLE)) {
		RP_CPU_Flags_arm |= RP_CPUFLAG_ARM_SVE;
	}
	if (IsProcessorFeaturePresent(PF_ARM_SVE2_INSTRUCTIONS_AVAILABLE)) {
		RP_CPU_Flags_arm |= RP_CPUFLAG_ARM_SVE2;
	}
	if (IsProcessorFeaturePresent(PF_ARM_SVE2_1_INSTRUCTIONS_AVAILABLE)) {
		RP_CPU_Flags_arm |= RP_CPUFLAG_ARM_SVE2P1;
	}
#else
	// TODO: FreeBSD elf_aux_info()?
#  if defined(RP_CPU_ARM64) || (defined(RP_CPU_ARM) && defined(_WIN32))
	// ARM NEON is always available on arm64.
	// Windows on ARM also always has NEON. (desktop Windows, e.g. Windows RT [Win8])
	RP_CPU_Flags_arm = RP_CPUFLAG_ARM_NEON;
#  else
	RP_CPU_Flags_arm = 0;
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
