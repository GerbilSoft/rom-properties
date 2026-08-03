/***************************************************************************
 * ROM Properties Page shell extension. (rpcli)                            *
 * printcpufeatures.cpp: Print CPU features.                               *
 *                                                                         *
 * Copyright (c) 2016-2026 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "config.rpcli.h"
#include "printcpufeatures.hpp"

// libgsvt for VT handling
#include "gsvtpp.hpp"

// Other rom-properties libraries
#include "libi18n/i18n.hpp"
#include "librpbase/SystemRegion.hpp"
#include "librpbyteswap/byteorder.h"
using namespace LibRpBase;

// CPU dispatch
#include "cpu_dispatch.h"
#if defined(RP_CPU_I386) || defined(RP_CPU_AMD64)
#  include "cpuflags_x86.h"
#elif defined(RP_CPU_ARM) || defined(RP_CPU_ARM64)
#  include "cpuflags_arm.h"
#endif

// C includes (C++ namespace)
#include <cassert>

// C++ STL classes
#include <array>
using std::array;

// libfmt
#include "rp-libfmt.h"

/**
 * Determine the actual length of a fixed-length string.
 * This checks for the last non-whitespace character in the string.
 * @param str String
 * @param len Fixed length
 * @return Actual length
 */
static size_t get_str_actual_len(const char *str, size_t len)
{
	for (; len > 0; len--) {
		const char c = str[len-1];
		if (c != ' ' && c != '\0') {
			break;
		}
	}

	return len;
}

/**
 * Print CPU features.
 * @return 0 on success; non-zero on error.
 */
int PrintCPUFeatures(void)
{
	Gsvt::StdOut.fputs(fmt::format(FRUN(C_("rpcli", "CPU architecture: {:s}")), RP_CPU_ARCH_NAME));
	Gsvt::StdOut.newline();

	const char *const endianness =
#if SYS_BYTEORDER == SYS_BIG_ENDIAN
		C_("RomData", "Big-Endian");
#elif SYS_BYTEORDER == SYS_LIL_ENDIAN
		C_("RomData", "Little-Endian");
#elif SYS_BYTEORDER == SYS_PDP_ENDIAN
		// shouldn't happen...
		"PDP-Endian????";
#else
#  error SYS_BYTEORDER is invalid!
#endif

	Gsvt::StdOut.fputs(fmt::format(FRUN(C_("rpcli", "Endianness: {:s}")), endianness));
	Gsvt::StdOut.newline();

	// Bullet character.
	// If the system supports Unicode, use U+2022.
	// Otherwise, use '*'.
	const char *s_bullet;
	if (SystemRegion::doesSystemSupportUnicode()) {
		s_bullet = "\xE2\x80\xA2 ";
	} else {
		s_bullet = "* ";
	}

	// CPU flag table
	struct cpu_flag_tbl_t {
		uint32_t value;
		const char *name;
		const char *desc;
	};
	uint32_t cpu_flags = 0;

#if defined(RP_CPU_I386) || defined(RP_CPU_AMD64)
	RP_CPU_Flags_x86_Init();

	// TODO: Trim the strings?
	if (RP_CPU_Info_x86.manufacturer_id.c[0] != '\0') {
		const size_t len = get_str_actual_len(RP_CPU_Info_x86.manufacturer_id.c, sizeof(RP_CPU_Info_x86.manufacturer_id.c));
		if (len > 0) {
			Gsvt::StdOut.fputs(fmt::format(FRUN(C_("rpcli", "Manufacturer ID: {:s}")),
				fmt::string_view(RP_CPU_Info_x86.manufacturer_id.c, len)));
		}
		Gsvt::StdOut.newline();
	}
	if (RP_CPU_Info_x86.brand_string.c[0] != '\0') {
		const size_t len = get_str_actual_len(RP_CPU_Info_x86.brand_string.c, sizeof(RP_CPU_Info_x86.brand_string.c));
		if (len > 0) {
			Gsvt::StdOut.fputs(fmt::format(FRUN(C_("rpcli", "Brand String: {:s}")),
				fmt::string_view(RP_CPU_Info_x86.brand_string.c, len)));
		}
		Gsvt::StdOut.newline();
	}

	// Family/Model/Stepping
	Gsvt::StdOut.fputs(fmt::format(FRUN(C_("rpcli", "Family/Model/Stepping: {0:d}/{1:d}/{2:d} ({3:0>2X}h/{4:0>2X}h/{5:0>2X}h)")),
		RP_CPU_Info_x86.version.family_id,
		RP_CPU_Info_x86.version.model_id,
		RP_CPU_Info_x86.version.stepping_id,
		RP_CPU_Info_x86.version.family_id,
		RP_CPU_Info_x86.version.model_id,
		RP_CPU_Info_x86.version.stepping_id));
	Gsvt::StdOut.newline();

	// x86 CPU flags
	cpu_flags = RP_CPU_Flags_x86;
	static const array<cpu_flag_tbl_t, 17> cpu_flag_tbl = {{
		{RP_CPUFLAG_x86_MMX,    "MMX",    "MultiMedia Extensions"},
		{RP_CPUFLAG_x86_SSE,    "SSE",    "Streaming SIMD Extensions"},
		{RP_CPUFLAG_x86_SSE2,   "SSE2",   "Streaming SIMD Extensions 2"},
		{RP_CPUFLAG_x86_SSE3,   "SSE3",   "Streaming SIMD Extensions 3"},
		{RP_CPUFLAG_x86_SSSE3,  "SSSE3",  "Supplemental Streaming SIMD Extensions 3"},
		{RP_CPUFLAG_x86_SSE41,  "SSE41",  "Streaming SIMD Extensions 4.1"},
		{RP_CPUFLAG_x86_SSE42,  "SSE42",  "Streaming SIMD Extensions 4.2"},
		{RP_CPUFLAG_x86_AES,    "AES",    "Advanced Encryption Standard"},
		{RP_CPUFLAG_x86_AVX,    "AVX",    "Advanced Vector Extensions"},
		{RP_CPUFLAG_x86_F16C,   "F16C",   "Half-Precision Floating Point"},
		{RP_CPUFLAG_x86_FMA3,   "FMA3",   "Fused Multiply-Add, 3-operand"},
		{RP_CPUFLAG_x86_BMI1,   "BMI1",   "Bit Manipulation Instructions 1"},
		{RP_CPUFLAG_x86_AVX2,   "AVX2",   "Advanced Vector Extensions 2"},
		{RP_CPUFLAG_x86_BMI2,   "BMI2",   "Bit Manipulation Instructions 2"},
		{RP_CPUFLAG_x86_SHA,    "SHA",    "Secure Hash Algorithm (SHA-1, SHA-256)"},
		{RP_CPUFLAG_x86_SHA512, "SHA512", "Secure Hash Algorithm (SHA-512)"},
		{RP_CPUFLAG_x86_APX,    "APX",    "Advanced Performance Extensions"},
	}};
#elif defined(RP_CPU_ARM) || defined(RP_CPU_ARM64)
	// ARM
	RP_CPU_Flags_arm_Init();
	cpu_flags = RP_CPU_Flags_arm;
	static const array<cpu_flag_tbl_t, 8> cpu_flag_tbl = {{
		{RP_CPUFLAG_ARM_NEON,   "NEON",   "NEON SIMD Extensions"},
		{RP_CPUFLAG_ARM_AES,    "AES",    "Advanced Encryption Standard"},
		{RP_CPUFLAG_ARM_SHA1,   "SHA1",   "Secure Hash Algorithm (SHA-1)"},
		{RP_CPUFLAG_ARM_SHA2,   "SHA2",   "Secure Hash Algorithm (SHA-256)"},
		{RP_CPUFLAG_ARM_CRC32,  "CRC32",  "Cyclic Redundancy Check (32-bit)"},
		{RP_CPUFLAG_ARM_SHA3,   "SHA3",   "Secure Hash Algorithm (SHA3-256)"},
		{RP_CPUFLAG_ARM_SHA512, "SHA512", "Secure Hash Algorithm (SHA-512)"},
		{RP_CPUFLAG_ARM_SVE,    "SVE",    "Scalable Vector Extensions"},
	}};
#else
	// No CPU flags for this architecture...
	cpu_flags = 0;
	static const array<cpu_flag_tbl_t, 0> cpu_flag_tbl = {{ }};
#endif

	// TODO: Colorization, maybe?
	Gsvt::StdOut.newline();
	Gsvt::StdOut.fputs(C_("rpcli", "CPU flags:"));
	if (cpu_flags != 0) {
		Gsvt::StdOut.newline();

		// TODO: Column alignment?
		for (const cpu_flag_tbl_t &flag : cpu_flag_tbl) {
			if (cpu_flags & flag.value) {
				Gsvt::StdOut.fputs(s_bullet);
				Gsvt::StdOut.fputs(fmt::format(FRUN(C_("rpcli", "{0:s}: {1:s}")), flag.name, flag.desc));
				Gsvt::StdOut.newline();
			}
		}
	} else {
		Gsvt::StdOut.fputc(' ');
		Gsvt::StdOut.fputs(C_("rpcli", "(none)"));
		Gsvt::StdOut.newline();
	}

	Gsvt::StdOut.fflush();
	return 0;
}
