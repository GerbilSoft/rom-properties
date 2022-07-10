/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * EXEData.cpp: DOS/Windows executable data.                               *
 *                                                                         *
 * Copyright (c) 2016-2022 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "EXEData.hpp"
#include "Other/exe_structs.h"

namespace LibRomData { namespace EXEData {

struct MachineType {
	uint16_t cpu;
	const char8_t *name;
};

// PE machine types.
// NOTE: The cpu field *must* be sorted in ascending order.
static const MachineType machineTypes_PE[] = {
	{IMAGE_FILE_MACHINE_I386,	U8("Intel i386")},
	{IMAGE_FILE_MACHINE_R3000_BE,	U8("MIPS R3000 (big-endian)")},
	{IMAGE_FILE_MACHINE_R3000,	U8("MIPS R3000")},
	{IMAGE_FILE_MACHINE_R4000,	U8("MIPS R4000")},
	{IMAGE_FILE_MACHINE_R10000,	U8("MIPS R10000")},
	{IMAGE_FILE_MACHINE_WCEMIPSV2,	U8("MIPS (WCE v2)")},
	{IMAGE_FILE_MACHINE_ALPHA,	U8("DEC Alpha AXP")},
	{IMAGE_FILE_MACHINE_SH3,	U8("Hitachi SH3")},
	{IMAGE_FILE_MACHINE_SH3DSP,	U8("Hitachi SH3 DSP")},
	{IMAGE_FILE_MACHINE_SH3E,	U8("Hitachi SH3E")},
	{IMAGE_FILE_MACHINE_SH4,	U8("Hitachi SH4")},
	{IMAGE_FILE_MACHINE_SH5,	U8("Hitachi SH5")},
	{IMAGE_FILE_MACHINE_ARM,	U8("ARM")},
	{IMAGE_FILE_MACHINE_THUMB,	U8("ARM Thumb")},
	{IMAGE_FILE_MACHINE_ARMNT,	U8("ARM Thumb-2")},
	{IMAGE_FILE_MACHINE_AM33,	U8("Matsushita AM33")},
	{IMAGE_FILE_MACHINE_POWERPC,	U8("PowerPC")},
	{IMAGE_FILE_MACHINE_POWERPCFP,	U8("PowerPC with FPU")},
	{IMAGE_FILE_MACHINE_POWERPCBE,	U8("PowerPC (big-endian)")},
	{IMAGE_FILE_MACHINE_IA64,	U8("Intel Itanium")},
	{IMAGE_FILE_MACHINE_MIPS16,	U8("MIPS16")},
	{IMAGE_FILE_MACHINE_M68K,	U8("Motorola 68000")},
	{IMAGE_FILE_MACHINE_PA_RISC,	U8("PA-RISC")},
	{IMAGE_FILE_MACHINE_ALPHA64,	U8("DEC Alpha AXP (64-bit)")},
	{IMAGE_FILE_MACHINE_MIPSFPU,	U8("MIPS with FPU")},
	{IMAGE_FILE_MACHINE_MIPSFPU16,	U8("MIPS16 with FPU")},
	{IMAGE_FILE_MACHINE_TRICORE,	U8("Infineon TriCore")},
	{IMAGE_FILE_MACHINE_CEF,	U8("Common Executable Format")},
	{IMAGE_FILE_MACHINE_EBC,	U8("EFI Byte Code")},
	{IMAGE_FILE_MACHINE_RISCV32,	U8("RISC-V (32-bit address space)")},
	{IMAGE_FILE_MACHINE_RISCV64,	U8("RISC-V (64-bit address space)")},
	{IMAGE_FILE_MACHINE_RISCV128,	U8("RISC-V (128-bit address space)")},
	{IMAGE_FILE_MACHINE_AMD64,	U8("AMD64")},
	{IMAGE_FILE_MACHINE_M32R,	U8("Mitsubishi M32R")},
	{IMAGE_FILE_MACHINE_ARM64,	U8("ARM (64-bit)")},
	{IMAGE_FILE_MACHINE_CEE,	U8("MSIL")},

	{0, nullptr}
};

// LE machine types.
// NOTE: The cpu field *must* be sorted in ascending order.
static const MachineType machineTypes_LE[] = {
	{LE_CPU_80286,		U8("Intel i286")},
	{LE_CPU_80386,		U8("Intel i386")},
	{LE_CPU_80486,		U8("Intel i486")},
	{LE_CPU_80586,		U8("Intel Pentium")},
	{LE_CPU_i860_N10,	U8("Intel i860 XR (N10)")},
	{LE_CPU_i860_N11,	U8("Intel i860 XP (N11)")},
	{LE_CPU_MIPS_I,		U8("MIPS Mark I (R2000, R3000")},
	{LE_CPU_MIPS_II,	U8("MIPS Mark II (R6000)")},
	{LE_CPU_MIPS_III,	U8("MIPS Mark III (R4000)")},

	{0, nullptr}
};

/**
 * bsearch() comparison function for MachineType.
 * @param a
 * @param b
 * @return
 */
static int RP_C_API MachineType_compar(const void *a, const void *b)
{
	const uint16_t cpu1 = static_cast<const MachineType*>(a)->cpu;
	const uint16_t cpu2 = static_cast<const MachineType*>(b)->cpu;
	if (cpu1 < cpu2) return -1;
	if (cpu1 > cpu2) return 1;
	return 0;
}

/** Public functions **/

/**
 * Look up a PE machine type. (CPU)
 * @param cpu PE machine type.
 * @return Machine type name, or nullptr if not found.
 */
const char8_t *lookup_pe_cpu(uint16_t cpu)
{
	// Do a binary search.
	const MachineType key = {cpu, nullptr};
	const MachineType *res =
		static_cast<const MachineType*>(bsearch(&key,
			machineTypes_PE,
			ARRAY_SIZE(machineTypes_PE)-1,
			sizeof(MachineType),
			MachineType_compar));
	return (res) ? res->name : nullptr;
}

/**
 * Look up an LE machine type. (CPU)
 * @param cpu LE machine type.
 * @return Machine type name, or nullptr if not found.
 */
const char8_t *lookup_le_cpu(uint16_t cpu)
{
	// Do a binary search.
	const MachineType key = {cpu, nullptr};
	const MachineType *res =
		static_cast<const MachineType*>(bsearch(&key,
			machineTypes_LE,
			ARRAY_SIZE(machineTypes_LE)-1,
			sizeof(MachineType),
			MachineType_compar));
	return (res) ? res->name : nullptr;
}

} }
