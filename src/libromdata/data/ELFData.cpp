/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * ELFData.cpp: Executable and Linkable Format data.                       *
 *                                                                         *
 * Copyright (c) 2016-2022 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "ELFData.hpp"
#include "Other/elf_structs.h"

namespace LibRomData { namespace ELFData {

#include "ELFMachineTypes_data.h"
#include "ELF_OSABI_data.h"

struct ELFMachineType {
	uint16_t cpu;
	const char8_t *name;
};

// ELF machine types. (other IDs)
// Reference: https://github.com/file/file/blob/master/magic/Magdir/elf
static const ELFMachineType ELFMachineTypes_other[] = {
	// The following are unofficial and/or obsolete types.
	// TODO: Indicate unofficial/obsolete using a separate flag?
	{0x1057,	U8("AVR (unofficial)")},
	{0x1059,	U8("MSP430 (unofficial)")},
	{0x1223,	U8("Adapteva Epiphany (unofficial)")},
	{0x2530,	U8("Morpho MT (unofficial)")},
	{0x3330,	U8("Fujitsu FR30 (unofficial)")},
	{0x3426,	U8("OpenRISC (obsolete)")},
	{0x4157,	U8("WebAssembly (unofficial)")},
	{0x4688,	U8("Infineon C166 (unofficial)")},
	{0x4DEF,	U8("Freescale S12Z (unofficial)")},
	{0x5441,	U8("Fujitsu FR-V (unofficial)")},
	{0x5AA5,	U8("DLX (unofficial)")},
	{0x7650,	U8("Mitsubishi D10V (unofficial)")},
	{0x7676,	U8("Mitsubishi D30V (unofficial)")},
	{0x8217,	U8("Ubicom IP2xxx (unofficial)")},
	{0x8472,	U8("OpenRISC (obsolete)")},
	{0x9025,	U8("PowerPC (unofficial)")},
	{0x9026,	U8("DEC Alpha (unofficial)")},
	{0x9041,	U8("Renesas M32R (unofficial)")},	/* formerly Mitsubishi M32R */
	{0x9080,	U8("Renesas V850 (unofficial)")},
	{0xA390,	U8("IBM System/390 (obsolete)")},
	{0xABC7,	U8("Old Xtensa (unofficial)")},
	{0xAD45,	U8("xstormy16 (unofficial)")},
	{0xBAAB,	U8("Old MicroBlaze (unofficial)")},
	{0xBEEF,	U8("Matsushita MN10300 (unofficial)")},
	{0xDEAD,	U8("Matsushita MN10200 (unofficial)")},
	{0xF00D,	U8("Toshiba MeP (unofficial)")},
	{0xFEB0,	U8("Renesas M32C (unofficial)")},
	{0xFEBA,	U8("Vitesse IQ2000 (unofficial)")},
	{0xFEBB,	U8("NIOS (unofficial)")},
	{0xFEED,	U8("Moxie (unofficial)")},

	{0, nullptr}
};

/**
 * bsearch() comparison function for ELFMachineType.
 * @param a
 * @param b
 * @return
 */
static int RP_C_API ELFMachineType_compar(const void *a, const void *b)
{
	const uint16_t cpu1 = static_cast<const ELFMachineType*>(a)->cpu;
	const uint16_t cpu2 = static_cast<const ELFMachineType*>(b)->cpu;
	if (cpu1 < cpu2) return -1;
	if (cpu1 > cpu2) return 1;
	return 0;
}

/** Public functions **/

/**
 * Look up an ELF machine type. (CPU)
 * @param cpu ELF machine type.
 * @return Machine type name, or nullptr if not found.
 */
const char8_t *lookup_cpu(uint16_t cpu)
{
	if (likely(cpu < ARRAY_SIZE(ELFMachineTypes_offtbl))) {
		const unsigned int offset = ELFMachineTypes_offtbl[cpu];
		return (likely(offset != 0) ? &ELFMachineTypes_strtbl[offset] : nullptr);
	}

	// CPU ID is in the "other" IDs array.
	// Do a binary search.
	const ELFMachineType key = {cpu, nullptr};
	const ELFMachineType *res =
		static_cast<const ELFMachineType*>(bsearch(&key,
			ELFMachineTypes_other,
			ARRAY_SIZE(ELFMachineTypes_other)-1,
			sizeof(ELFMachineType),
			ELFMachineType_compar));
	return (res) ? res->name : nullptr;
}

/**
 * Look up an ELF OS ABI.
 * @param osabi ELF OS ABI.
 * @return OS ABI name, or nullptr if not found.
 */
const char8_t *lookup_osabi(uint8_t osabi)
{
	if (likely(osabi < ARRAY_SIZE(ELF_OSABI_offtbl))) {
		const unsigned int offset = ELF_OSABI_offtbl[osabi];
		return (likely(offset != 0) ? &ELF_OSABI_strtbl[offset] : nullptr);
	}

	switch (osabi) {
		case ELFOSABI_ARM_AEABI:	return U8("ARM EABI");
		case ELFOSABI_ARM:		return U8("ARM");
		case ELFOSABI_CELL_LV2:		return U8("Cell LV2");
		case ELFOSABI_CAFEOS:		return U8("Cafe OS");	// Wii U
		case ELFOSABI_STANDALONE:	return U8("Embedded");
		default:			break;
	}

	// Unknown OS ABI...
	return nullptr;
}

} }
