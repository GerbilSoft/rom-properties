/***************************************************************************
 * ROM Properties Page shell extension. (librpcpuid)                       *
 * cpuflags_x86.h: x86 CPU flags detection.                                *
 *                                                                         *
 * Copyright (c) 2017-2026 by David Korth.                                 *
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
#define RP_CPUFLAG_x86_MMX		((uint32_t)(1U <<  0))
#define RP_CPUFLAG_x86_SSE		((uint32_t)(1U <<  1))
#define RP_CPUFLAG_x86_SSE2		((uint32_t)(1U <<  2))
#define RP_CPUFLAG_x86_SSE3		((uint32_t)(1U <<  3))
#define RP_CPUFLAG_x86_SSSE3		((uint32_t)(1U <<  4))
#define RP_CPUFLAG_x86_SSE41		((uint32_t)(1U <<  5))
#define RP_CPUFLAG_x86_SSE42		((uint32_t)(1U <<  6))
#define RP_CPUFLAG_x86_AES		((uint32_t)(1U <<  7))
#define RP_CPUFLAG_x86_AVX		((uint32_t)(1U <<  8))
#define RP_CPUFLAG_x86_F16C		((uint32_t)(1U <<  9))
#define RP_CPUFLAG_x86_FMA3		((uint32_t)(1U << 10))
#define RP_CPUFLAG_x86_BMI1		((uint32_t)(1U << 11))
#define RP_CPUFLAG_x86_AVX2		((uint32_t)(1U << 12))
#define RP_CPUFLAG_x86_BMI2		((uint32_t)(1U << 13))
#define RP_CPUFLAG_x86_SHA		((uint32_t)(1U << 14))
#define RP_CPUFLAG_x86_APX		((uint32_t)(1U << 15))

// Don't modify these!
extern uint32_t RP_CPU_Flags_x86;
extern int RP_CPU_Flags_x86_IsInit;	// 1 if RP_CPU_Flags_x86 has been initialized.

// x86 CPU information
typedef struct _RP_CPU_Info_x86_t {
	// EAX=0
	uint32_t highest_fn;		// Highest regular function parameter
	// EAX=0x80000000
	uint32_t highest_ext_fn;	// Highest extended function parameter (>= 0x80000000)

	union {
		char c[12];
		uint32_t u32[3];
	} manufacturer_id; // Manufacturer ID (*not* NULL-terminated)

	// EAX=1
	union {
		struct {
			uint32_t stepping	: 4;
			uint32_t model		: 4;
			uint32_t family_id	: 4;
			uint32_t processor_type	: 2;
			uint32_t reserved1	: 2;
			uint32_t ext_model_id	: 4;
			uint32_t ext_family_id	: 8;
			uint32_t reserved2	: 4;
		};
		uint32_t u32;
	} version;

	// EAX=0x80000002, 0x80000003, 0x80000004
	union {
		char c[48];
		uint32_t u32[12];
	} brand_string; // Processor brand string (*not necessarily* NULL-terminated)
} RP_CPU_Info_x86_t;
extern RP_CPU_Info_x86_t RP_CPU_Info_x86;

/**
 * Initialize RP_CPU_Flags_x86.
 */
void RP_C_API RP_CPU_Flags_x86_Init(void);

// Convenience macros to determine if the CPU supports a certain flag.

// Macro for flags that need to be tested on both i386 and amd64 CPUs.
#define CPU_FLAG_x86_CHECK(flag) \
static RP_FORCEINLINE int RP_CPU_x86_Has##flag(void) \
{ \
	if (unlikely(!RP_CPU_Flags_x86_IsInit)) { \
		RP_CPU_Flags_x86_Init(); \
	} \
	return (int)(RP_CPU_Flags_x86 & RP_CPUFLAG_x86_##flag); \
}

// Macro for flags that always exist on amd64 and only need to be tested on i386.
#ifdef RP_CPU_AMD64
#  define CPU_FLAG_x86_CHECK_i386only(flag) \
static RP_FORCEINLINE int RP_CPU_x86_Has##flag(void) \
{ \
	return 1; \
}
#  define CPU_FLAG_x86_CHECK_amd64only(flag) CPU_FLAG_x86_CHECK(flag)
#else /* !RP_CPU_AMD64 */
#  define CPU_FLAG_x86_CHECK_i386only(flag) CPU_FLAG_x86_CHECK(flag)
#  define CPU_FLAG_x86_CHECK_amd64only(flag) \
static RP_FORCEINLINE int RP_CPU_x86_Has##flag(void) \
{ \
	return 0; \
}
#endif /* RP_CPU_AMD64 */

CPU_FLAG_x86_CHECK_i386only(MMX)
CPU_FLAG_x86_CHECK_i386only(SSE)
CPU_FLAG_x86_CHECK_i386only(SSE2)
CPU_FLAG_x86_CHECK(SSE3)
CPU_FLAG_x86_CHECK(SSSE3)
CPU_FLAG_x86_CHECK(SSE41)
CPU_FLAG_x86_CHECK(SSE42)
CPU_FLAG_x86_CHECK(AES)
CPU_FLAG_x86_CHECK(AVX)
CPU_FLAG_x86_CHECK(F16C)
CPU_FLAG_x86_CHECK(FMA3)
CPU_FLAG_x86_CHECK(BMI1)
CPU_FLAG_x86_CHECK(AVX2)
CPU_FLAG_x86_CHECK(BMI2)
CPU_FLAG_x86_CHECK(SHA)
CPU_FLAG_x86_CHECK_amd64only(APX)

#endif /* RP_CPU_I386 || RP_CPU_AMD64 */

#ifdef __cplusplus
}
#endif
