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
	Gsvt::StdOut.fputs(C_("rpcli", "Flags:"));
	uint32_t cpu_flags = 0;

	// Flags
	// TODO: Colorization, maybe?
#if defined(RP_CPU_I386) || defined(RP_CPU_AMD64)
	RP_CPU_Flags_x86_Init();
	cpu_flags = RP_CPU_Flags_x86;

#  define CHECK_CPUFLAG_x86(flag) do { \
	if (cpu_flags & RP_CPUFLAG_x86_##flag) { \
		Gsvt::StdOut.fputs(" " #flag); \
	} \
} while(0)

	CHECK_CPUFLAG_x86(MMX);
	CHECK_CPUFLAG_x86(SSE);
	CHECK_CPUFLAG_x86(SSE2);
	CHECK_CPUFLAG_x86(SSE3);
	CHECK_CPUFLAG_x86(SSSE3);
	CHECK_CPUFLAG_x86(SSE41);
	CHECK_CPUFLAG_x86(SSE42);
	CHECK_CPUFLAG_x86(AES);
	CHECK_CPUFLAG_x86(AVX);
	CHECK_CPUFLAG_x86(F16C);
	CHECK_CPUFLAG_x86(FMA3);
	CHECK_CPUFLAG_x86(BMI1);
	CHECK_CPUFLAG_x86(AVX2);
	CHECK_CPUFLAG_x86(BMI2);
	CHECK_CPUFLAG_x86(SHA);
	CHECK_CPUFLAG_x86(APX);
#endif

	if (cpu_flags == 0) {
		Gsvt::StdOut.fputc(' ');
		Gsvt::StdOut.fputs(C_("rpcli", "(none)"));
	}

	Gsvt::StdOut.newline();
	Gsvt::StdOut.fflush();

	return 0;
}
