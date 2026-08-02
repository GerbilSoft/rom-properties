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
#include "librpbyteswap/byteorder.h"

// CPU dispatch
#include "cpu_dispatch.h"
#if defined(RP_CPU_I386) || defined(RP_CPU_AMD64)
#  include "cpuflags_x86.h"
#elif defined(RP_CPU_ARM) || defined(RP_CPU_ARM64)
#  include "cpuflags_arm.h"
#endif

// C includes (C++ namespace)
#include <cassert>

// libfmt
#include "rp-libfmt.h"

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

#if defined(RP_CPU_I386) || defined(RP_CPU_AMD64)
	RP_CPU_Flags_x86_Init();

	// TODO: Trim the strings?
	if (RP_CPU_Info_x86.manufacturer_id.c[0] != '\0') {
		Gsvt::StdOut.fputs(fmt::format(FRUN(C_("rpcli", "Manufacturer ID: {:s}")),
			fmt::string_view(RP_CPU_Info_x86.manufacturer_id.c, sizeof(RP_CPU_Info_x86.manufacturer_id.c))));
		Gsvt::StdOut.newline();
	}
	if (RP_CPU_Info_x86.brand_string.c[0] != '\0') {
		Gsvt::StdOut.fputs(fmt::format(FRUN(C_("rpcli", "Brand String: {:s}")),
			fmt::string_view(RP_CPU_Info_x86.brand_string.c, sizeof(RP_CPU_Info_x86.brand_string.c))));
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
	// TODO: Colorization, maybe?
	Gsvt::StdOut.newline();
	Gsvt::StdOut.fputs(C_("rpcli", "CPU Flags:"));
	if (RP_CPU_Flags_x86 != 0) {
		Gsvt::StdOut.newline();
		// TODO: Check locale to see if we can print a Unicode bullet? (U+2022)
		// On Windows, check OS version?
		static const char s_bullet[] = "\xE2\x80\xA2 ";
#  define CHECK_CPUFLAG_x86(flag, desc) do { \
		if (RP_CPU_Flags_x86 & RP_CPUFLAG_x86_##flag) { \
			Gsvt::StdOut.fputs(s_bullet); \
			Gsvt::StdOut.fputs(fmt::format(FRUN(C_("rpcli", "{0:s}: {1:s}")), #flag, desc)); \
			Gsvt::StdOut.newline(); \
		} \
} while(0)

		// TODO: Column alignment?
		CHECK_CPUFLAG_x86(MMX, "MultiMedia Extensions");
		CHECK_CPUFLAG_x86(SSE, "Streaming SIMD Extensions");
		CHECK_CPUFLAG_x86(SSE2, "Streaming SIMD Extensions 2");
		CHECK_CPUFLAG_x86(SSE3, "Streaming SIMD Extensions 3");
		CHECK_CPUFLAG_x86(SSSE3, "Supplemental Streaming SIMD Extensions 3");
		CHECK_CPUFLAG_x86(SSE41, "Streaming SIMD Extensions 4.1");
		CHECK_CPUFLAG_x86(SSE42, "Streaming SIMD Extensions 4.2");
		CHECK_CPUFLAG_x86(AES, "Advanced Encryption Standard");
		CHECK_CPUFLAG_x86(AVX, "Advanced Vector Extensions");
		CHECK_CPUFLAG_x86(F16C, "Half-Precision Floating Point");
		CHECK_CPUFLAG_x86(FMA3, "Fused Multiply-Add, 3-operand");
		CHECK_CPUFLAG_x86(BMI1, "Bit Manipulation Instructions 1");
		CHECK_CPUFLAG_x86(AVX2, "Advanced Vector Extensions 2");
		CHECK_CPUFLAG_x86(BMI2, "Bit Manipulation Instructions 2");
		CHECK_CPUFLAG_x86(SHA, "Secure Hash Algorithm (SHA-1, SHA-256)");
		CHECK_CPUFLAG_x86(APX, "Advanced Performance Extensions");
	} else {
		Gsvt::StdOut.fputc(' ');
		Gsvt::StdOut.fputs(C_("rpcli", "(none)"));
		Gsvt::StdOut.newline();
	}
#else
	// Unsupported CPU architecture...
	Gsvt::StdOut.newline();
	Gsvt::StdOut.fputs(C_("rpcli", "CPU Flags:"));
	Gsvt::StdOut.fputc(' ');
	Gsvt::StdOut.fputs(C_("rpcli", "(none)"));
	Gsvt::StdOut.newline();
#endif

	Gsvt::StdOut.fflush();
	return 0;
}
