/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * MachOData.cpp: Mach-O executable format data.                           *
 *                                                                         *
 * Copyright (c) 2019-2022 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "MachOData.hpp"
#include "Other/macho_structs.h"

namespace LibRomData { namespace MachOData {

/**
 * Look up a Mach-O CPU type.
 * @param cputype Mach-O CPU type.
 * @return CPU type name, or nullptr if not found.
 */
const char8_t *lookup_cpu_type(uint32_t cputype)
{
	const unsigned int abi = (cputype >> 24);
	const unsigned int cpu = (cputype & 0xFFFFFF);

	static const char8_t cpu_tbl_32[19][8] = {
		// 32-bit CPUs
		U8(""), U8("VAX"), U8(""), U8("ROMP"),
		U8("NS32032"), U8("NS32332"), U8("MC680x0"), U8("i386"),
		U8("MIPS"), U8("NS32532"), U8("MC98000"), U8("HPPA"),
		U8("ARM"), U8("MC88000"), U8("SPARC"), U8("i860"),
		U8("Alpha"), U8("RS/6000"), U8("PowerPC")
	};

	const char8_t *s_cpu = nullptr;
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
					s_cpu = U8("amd64");
					break;
				case CPU_TYPE_ARM:
					s_cpu = U8("arm64");
					break;
				case CPU_TYPE_POWERPC:
					s_cpu = U8("PowerPC 64");
					break;
				default:
					break;
			}
			break;

		case 2:
			if (cpu == CPU_TYPE_ARM) {
				s_cpu = U8("arm64_32");
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
const char8_t *lookup_cpu_subtype(uint32_t cputype, uint32_t cpusubtype)
{
	const unsigned int abi = (cputype >> 24) & 1;
	cpusubtype &= 0xFFFFFF;

	const char8_t *s_cpu_subtype = nullptr;
	switch (cputype & 0xFFFFFF) {
		default:
			break;

		case CPU_TYPE_VAX: {
			static const char8_t *const cpu_subtype_vax_tbl[] = {
				nullptr, U8("VAX-11/780"), U8("VAX-11/785"), U8("VAX-11/750"),
				U8("VAX-11/730"), U8("MicroVAX I"), U8("MicroVAX II"), U8("VAX 8200"),
				U8("VAX 8500"), U8("VAX 8600"), U8("VAX 8650"), U8("VAX 8800"),
				U8("MicroVAX III")
			};
			if (cpusubtype < ARRAY_SIZE(cpu_subtype_vax_tbl)) {
				s_cpu_subtype = cpu_subtype_vax_tbl[cpusubtype];
			}
			break;
		}

		case CPU_TYPE_MC680x0: {
			static const char8_t cpu_subtype_m68k_tbl[][8] = {
				U8(""), U8(""), U8("MC68040"), U8("MC68030")
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
						s_cpu_subtype = U8("i386");
						break;
					case CPU_SUBTYPE_486:
						s_cpu_subtype = (cpusubtype == CPU_SUBTYPE_486SX) ? U8("i486SX") : U8("i486");
						break;
					case CPU_SUBTYPE_PENT:
						s_cpu_subtype = U8("Pentium");
						break;

					case CPU_SUBTYPE_INTEL(6, 0): {
						// i686 class
						static const char8_t *const i686_cpu_tbl[] = {
							U8("i686"), U8("Pentium Pro"),
							U8("Pentium II (M2)"), U8("Pentium II (M3)"),
							U8("Pentium II (M4)"), U8("Pentium II (M5)")
						};
						const uint8_t i686_subtype = (cpusubtype >> 4);
						if (i686_subtype < ARRAY_SIZE(i686_cpu_tbl)) {
							s_cpu_subtype = i686_cpu_tbl[i686_subtype];
						} else {
							s_cpu_subtype = i686_cpu_tbl[0];
						}
						break;
					}

					case CPU_SUBTYPE_CELERON:
						// Celeron
						if (cpusubtype != CPU_SUBTYPE_CELERON_MOBILE) {
							s_cpu_subtype = U8("Celeron");
						} else {
							s_cpu_subtype = U8("Celeron (Mobile)");
						}
						break;

					case CPU_SUBTYPE_PENTIII: {
						// Pentium III
						static const char8_t *const p3_cpu_tbl[] = {
							U8("Pentium III"), U8("Pentium III-M"),
							U8("Pentium III Xeon")
						};
						const uint8_t p3_subtype = (cpusubtype >> 4);
						if (p3_subtype < ARRAY_SIZE(p3_cpu_tbl)) {
							s_cpu_subtype = p3_cpu_tbl[p3_subtype];
						} else {
							s_cpu_subtype = p3_cpu_tbl[0];
						}
						break;
					}

					case CPU_SUBTYPE_PENTIUM_M:
						s_cpu_subtype = U8("Pentium M");
						break;

					case CPU_SUBTYPE_PENTIUM_4:
						s_cpu_subtype = U8("Pentium 4");
						break;

					case CPU_SUBTYPE_ITANIUM:
						if (cpusubtype == CPU_SUBTYPE_ITANIUM_2) {
							s_cpu_subtype = U8("Itanium 2");
						} else {
							s_cpu_subtype = U8("Itanium");
						}
						break;

					case CPU_SUBTYPE_XEON:
						if (cpusubtype == CPU_SUBTYPE_XEON_MP) {
							s_cpu_subtype = U8("Xeon MP");
						}
						break;
				}
			} else {
				// 64-bit
				switch (cpusubtype) {
					default:
						break;
					case CPU_SUBTYPE_AMD64_ARCH1:
						s_cpu_subtype = U8("arch1");
						break;
					case CPU_SUBTYPE_AMD64_HASWELL:
						s_cpu_subtype = U8("Haswell");
						break;
				}
			}

			break;
		}

		case CPU_TYPE_MIPS: {
			static const char8_t cpu_subtype_mips_tbl[][8] = {
				U8(""), U8("R2300"), U8("R2600"), U8("R2800"),
				U8("R2000a"), U8("R2000"), U8("R3000a"), U8("R3000")
			};
			if (cpusubtype < ARRAY_SIZE(cpu_subtype_mips_tbl)) {
				s_cpu_subtype = cpu_subtype_mips_tbl[cpusubtype];
			}
			break;
		}

		case CPU_TYPE_MC98000:
			if (cpusubtype == CPU_SUBTYPE_MC98601) {
				s_cpu_subtype = U8("MC98601");
			}
			break;

		case CPU_TYPE_HPPA: {
			static const char8_t *const cpu_subtype_hppa_tbl[] = {
				nullptr, U8("HP/PA 7100"), U8("HP/PA 7100LC")
			};
			if (cpusubtype < ARRAY_SIZE(cpu_subtype_hppa_tbl)) {
				s_cpu_subtype = cpu_subtype_hppa_tbl[cpusubtype];
			}
			break;
		}

		case CPU_TYPE_MC88000: {
			static const char8_t cpu_subtype_m88k_tbl[][8] = {
				U8(""), U8("MC88100"), U8("MC88110")
			};
			if (cpusubtype < ARRAY_SIZE(cpu_subtype_m88k_tbl)) {
				s_cpu_subtype = cpu_subtype_m88k_tbl[cpusubtype];
			}
			break;
		}

		case CPU_TYPE_ARM: {
			if (abi && cpusubtype == CPU_SUBTYPE_ARM64_V8) {
				s_cpu_subtype = U8("ARMv8");
			} else if (!abi) {
				static const char8_t *const cpu_subtype_arm_tbl[] = {
					nullptr, nullptr, nullptr, nullptr,
					nullptr, U8("ARMv4T"), U8("ARMv6"), U8("ARMv5TEJ"),
					U8("XScale"), U8("ARMv7"), U8("ARMv7f"), U8("ARMv7s"),
					U8("ARMv7k"), U8("ARMv8"), U8("ARMv6-M"), U8("ARMv7-M"),
					U8("ARMv7E-M")
				};
				if (cpusubtype < ARRAY_SIZE(cpu_subtype_arm_tbl)) {
					s_cpu_subtype = cpu_subtype_arm_tbl[cpusubtype];
				}
			}
			break;
		}

		case CPU_TYPE_POWERPC: {
			static const char8_t cpu_subtype_ppc_tbl[][8] = {
				U8(""), U8("601"), U8("602"), U8("603"),
				U8("603e"), U8("603ev"), U8("604"), U8("604e"),
				U8("620"), U8("750"), U8("7400"), U8("7450")
			};
			if (cpusubtype < ARRAY_SIZE(cpu_subtype_ppc_tbl)) {
				s_cpu_subtype = cpu_subtype_ppc_tbl[cpusubtype];
			} else if (cpusubtype == CPU_SUBTYPE_POWERPC_970) {
				s_cpu_subtype = U8("970");
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
