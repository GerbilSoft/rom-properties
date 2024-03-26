/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * MachOData.cpp: Mach-O executable format data.                           *
 *                                                                         *
 * Copyright (c) 2019-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "MachOData.hpp"
#include "Other/macho_structs.h"

// C++ STL classes
using std::array;

namespace LibRomData { namespace MachOData {

/**
 * Look up a Mach-O CPU type.
 * @param cputype Mach-O CPU type.
 * @return CPU type name, or nullptr if not found.
 */
const char *lookup_cpu_type(uint32_t cputype)
{
	const unsigned int abi = (cputype >> 24);
	const unsigned int cpu = (cputype & 0xFFFFFF);

	static constexpr char cpu_tbl_32[19][8] = {
		// 32-bit CPUs
		"", "VAX", "", "ROMP",
		"NS32032", "NS32332", "MC680x0", "i386",
		"MIPS", "NS32532", "MC98000", "HPPA",
		"ARM", "MC88000", "SPARC", "i860",
		"Alpha", "RS/6000", "PowerPC"
	};

	const char *s_cpu = nullptr;
	switch (abi) {
		case 0:
			// 32-bit
			if (cpu < ARRAY_SIZE(cpu_tbl_32)) {
				s_cpu = cpu_tbl_32[cpu];
			}
			break;

		case 1:
			// 64-bit
			switch (cpu) {
				case CPU_TYPE_I386:
					s_cpu = "amd64";
					break;
				case CPU_TYPE_ARM:
					s_cpu = "arm64";
					break;
				case CPU_TYPE_POWERPC:
					s_cpu = "PowerPC 64";
					break;
				default:
					break;
			}
			break;

		case 2:
			if (cpu == CPU_TYPE_ARM) {
				s_cpu = "arm64_32";
			}
			break;

		default:
			break;
	};

	if (s_cpu && s_cpu[0] == '\0') {
		s_cpu = nullptr;
	}
	return s_cpu;
}

/**
 * Look up a Mach-O CPU subtype.
 * @param cputype Mach-O CPU type.
 * @param cpusubtype Mach-O CPU subtype.
 * @return OS ABI name, or nullptr if not found.
 */
const char *lookup_cpu_subtype(uint32_t cputype, uint32_t cpusubtype)
{
	const unsigned int abi = (cputype >> 24) & 1;
	cpusubtype &= 0xFFFFFF;

	const char *s_cpu_subtype = nullptr;
	switch (cputype & 0xFFFFFF) {
		default:
			break;

		case CPU_TYPE_VAX: {
			static constexpr array<const char*, 12+1> cpu_subtype_vax_tbl = {{
				nullptr, "VAX-11/780", "VAX-11/785", "VAX-11/750",
				"VAX-11/730", "MicroVAX I", "MicroVAX II", "VAX 8200",
				"VAX 8500", "VAX 8600", "VAX 8650", "VAX 8800",
				"MicroVAX III"
			}};
			if (cpusubtype < cpu_subtype_vax_tbl.size()) {
				s_cpu_subtype = cpu_subtype_vax_tbl[cpusubtype];
			}
			break;
		}

		case CPU_TYPE_MC680x0: {
			static constexpr char cpu_subtype_m68k_tbl[][8] = {
				"", "", "MC68040", "MC68030"
			};
			if (cpusubtype < ARRAY_SIZE(cpu_subtype_m68k_tbl)) {
				s_cpu_subtype = cpu_subtype_m68k_tbl[cpusubtype];
			}
			break;
		}

		case CPU_TYPE_I386: {
			// 32-bit
			if (abi == 0) {
				switch (cpusubtype & 0xF) {
					default:
						break;
					case CPU_SUBTYPE_386:
						s_cpu_subtype = "i386";
						break;
					case CPU_SUBTYPE_486:
						s_cpu_subtype = (cpusubtype == CPU_SUBTYPE_486SX ? "i486SX" : "i486");
						break;
					case CPU_SUBTYPE_PENT:
						s_cpu_subtype = "Pentium";
						break;

					case CPU_SUBTYPE_INTEL(6, 0): {
						// i686 class
						static constexpr array<const char*, 6> i686_cpu_tbl = {{
							"i686", "Pentium Pro",
							"Pentium II (M2)", "Pentium II (M3)",
							"Pentium II (M4)", "Pentium II (M5)"
						}};
						const uint8_t i686_subtype = (cpusubtype >> 4);
						if (i686_subtype < i686_cpu_tbl.size()) {
							s_cpu_subtype = i686_cpu_tbl[i686_subtype];
						} else {
							s_cpu_subtype = i686_cpu_tbl[0];
						}
						break;
					}

					case CPU_SUBTYPE_CELERON:
						// Celeron
						if (cpusubtype != CPU_SUBTYPE_CELERON_MOBILE) {
							s_cpu_subtype = "Celeron";
						} else {
							s_cpu_subtype = "Celeron (Mobile)";
						}
						break;

					case CPU_SUBTYPE_PENTIII: {
						// Pentium III
						static constexpr array<const char*, 3> p3_cpu_tbl = {{
							"Pentium III", "Pentium III-M",
							"Pentium III Xeon"
						}};
						const uint8_t p3_subtype = (cpusubtype >> 4);
						if (p3_subtype < p3_cpu_tbl.size()) {
							s_cpu_subtype = p3_cpu_tbl[p3_subtype];
						} else {
							s_cpu_subtype = p3_cpu_tbl[0];
						}
						break;
					}

					case CPU_SUBTYPE_PENTIUM_M:
						s_cpu_subtype = "Pentium M";
						break;

					case CPU_SUBTYPE_PENTIUM_4:
						s_cpu_subtype = "Pentium 4";
						break;

					case CPU_SUBTYPE_ITANIUM:
						if (cpusubtype == CPU_SUBTYPE_ITANIUM_2) {
							s_cpu_subtype = "Itanium 2";
						} else {
							s_cpu_subtype = "Itanium";
						}
						break;

					case CPU_SUBTYPE_XEON:
						if (cpusubtype == CPU_SUBTYPE_XEON_MP) {
							s_cpu_subtype = "Xeon MP";
						}
						break;
				}
			} else {
				// 64-bit
				switch (cpusubtype) {
					default:
						break;
					case CPU_SUBTYPE_AMD64_ARCH1:
						s_cpu_subtype = "arch1";
						break;
					case CPU_SUBTYPE_AMD64_HASWELL:
						s_cpu_subtype = "Haswell";
						break;
				}
			}

			break;
		}

		case CPU_TYPE_MIPS: {
			static constexpr char cpu_subtype_mips_tbl[][8] = {
				"", "R2300", "R2600", "R2800",
				"R2000a", "R2000", "R3000a", "R3000"
			};
			if (cpusubtype < ARRAY_SIZE(cpu_subtype_mips_tbl)) {
				s_cpu_subtype = cpu_subtype_mips_tbl[cpusubtype];
			}
			break;
		}

		case CPU_TYPE_MC98000:
			if (cpusubtype == CPU_SUBTYPE_MC98601) {
				s_cpu_subtype = "MC98601";
			}
			break;

		case CPU_TYPE_HPPA: {
			static constexpr array<const char*, 2+1> cpu_subtype_hppa_tbl = {{
				nullptr, "HP/PA 7100", "HP/PA 7100LC"
			}};
			if (cpusubtype < cpu_subtype_hppa_tbl.size()) {
				s_cpu_subtype = cpu_subtype_hppa_tbl[cpusubtype];
			}
			break;
		}

		case CPU_TYPE_MC88000: {
			static constexpr char cpu_subtype_m88k_tbl[][8] = {
				"", "MC88100", "MC88110"
			};
			if (cpusubtype < ARRAY_SIZE(cpu_subtype_m88k_tbl)) {
				s_cpu_subtype = cpu_subtype_m88k_tbl[cpusubtype];
			}
			break;
		}

		case CPU_TYPE_ARM: {
			if (abi && cpusubtype == CPU_SUBTYPE_ARM64_V8) {
				s_cpu_subtype = "ARMv8";
			} else if (!abi) {
				static constexpr array<const char*, 16+1> cpu_subtype_arm_tbl = {{
					nullptr, nullptr, nullptr, nullptr,
					nullptr, "ARMv4T", "ARMv6", "ARMv5TEJ",
					"XScale", "ARMv7", "ARMv7f", "ARMv7s",
					"ARMv7k", "ARMv8", "ARMv6-M", "ARMv7-M",
					"ARMv7E-M"
				}};
				if (cpusubtype < cpu_subtype_arm_tbl.size()) {
					s_cpu_subtype = cpu_subtype_arm_tbl[cpusubtype];
				}
			}
			break;
		}

		case CPU_TYPE_POWERPC: {
			static constexpr char cpu_subtype_ppc_tbl[][8] = {
				"", "601", "602", "603",
				"603e", "603ev", "604", "604e",
				"620", "750", "7400", "7450"
			};
			if (cpusubtype < ARRAY_SIZE(cpu_subtype_ppc_tbl)) {
				s_cpu_subtype = cpu_subtype_ppc_tbl[cpusubtype];
			} else if (cpusubtype == CPU_SUBTYPE_POWERPC_970) {
				s_cpu_subtype = "970";
			}
			break;
		}
	}

	if (s_cpu_subtype && s_cpu_subtype[0] == '\0') {
		// Empty string in a no-pointer array.
		// Change it to nullptr.
		s_cpu_subtype = nullptr;
	}

	return s_cpu_subtype;
}

} }
