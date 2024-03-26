/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * ELFData.cpp: Executable and Linkable Format data.                       *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "ELFData.hpp"
#include "Other/elf_structs.h"

// C++ STL classes
using std::array;

namespace LibRomData { namespace ELFData {

#include "ELFMachineTypes_data.h"
#include "ELF_OSABI_data.h"

struct ELFMachineType {
	uint16_t cpu;
	const char *name;
};

// ELF machine types. (other IDs)
// Reference: https://github.com/file/file/blob/master/magic/Magdir/elf
static constexpr array<ELFMachineType, 30> ELFMachineTypes_other = {{
	// The following are unofficial and/or obsolete types.
	// TODO: Indicate unofficial/obsolete using a separate flag?
	{0x1057,	"AVR (unofficial)"},
	{0x1059,	"MSP430 (unofficial)"},
	{0x1223,	"Adapteva Epiphany (unofficial)"},
	{0x2530,	"Morpho MT (unofficial)"},
	{0x3330,	"Fujitsu FR30 (unofficial)"},
	{0x3426,	"OpenRISC (obsolete)"},
	{0x4157,	"WebAssembly (unofficial)"},
	{0x4688,	"Infineon C166 (unofficial)"},
	{0x4DEF,	"Freescale S12Z (unofficial)"},
	{0x5441,	"Fujitsu FR-V (unofficial)"},
	{0x5AA5,	"DLX (unofficial)"},
	{0x7650,	"Mitsubishi D10V (unofficial)"},
	{0x7676,	"Mitsubishi D30V (unofficial)"},
	{0x8217,	"Ubicom IP2xxx (unofficial)"},
	{0x8472,	"OpenRISC (obsolete)"},
	{0x9025,	"PowerPC (unofficial)"},
	{0x9026,	"DEC Alpha (unofficial)"},
	{0x9041,	"Renesas M32R (unofficial)"},	/* formerly Mitsubishi M32R */
	{0x9080,	"Renesas V850 (unofficial)"},
	{0xA390,	"IBM System/390 (obsolete)"},
	{0xABC7,	"Old Xtensa (unofficial)"},
	{0xAD45,	"xstormy16 (unofficial)"},
	{0xBAAB,	"Old MicroBlaze (unofficial)"},
	{0xBEEF,	"Matsushita MN10300 (unofficial)"},
	{0xDEAD,	"Matsushita MN10200 (unofficial)"},
	{0xF00D,	"Toshiba MeP (unofficial)"},
	{0xFEB0,	"Renesas M32C (unofficial)"},
	{0xFEBA,	"Vitesse IQ2000 (unofficial)"},
	{0xFEBB,	"NIOS (unofficial)"},
	{0xFEED,	"Moxie (unofficial)"},
}};

/** Public functions **/

/**
 * Look up an ELF machine type. (CPU)
 * @param cpu ELF machine type.
 * @return Machine type name, or nullptr if not found.
 */
const char *lookup_cpu(uint16_t cpu)
{
	if (likely(cpu < ARRAY_SIZE(ELFMachineTypes_offtbl))) {
		const unsigned int offset = ELFMachineTypes_offtbl[cpu];
		return (likely(offset != 0) ? &ELFMachineTypes_strtbl[offset] : nullptr);
	}

	// CPU ID is in the "other" IDs array.
	// Do a binary search.
	auto pELF = std::lower_bound(ELFMachineTypes_other.cbegin(), ELFMachineTypes_other.cend(), cpu,
		[](const ELFMachineType &elf, uint16_t cpu) noexcept -> bool {
			return (elf.cpu < cpu);
		});
	if (pELF == ELFMachineTypes_other.cend() || pELF->cpu != cpu) {
		return nullptr;
	}
	return pELF->name;
}

/**
 * Look up an ELF OS ABI.
 * @param osabi ELF OS ABI.
 * @return OS ABI name, or nullptr if not found.
 */
const char *lookup_osabi(uint8_t osabi)
{
	if (likely(osabi < ARRAY_SIZE(ELF_OSABI_offtbl))) {
		const unsigned int offset = ELF_OSABI_offtbl[osabi];
		return (likely(offset != 0) ? &ELF_OSABI_strtbl[offset] : nullptr);
	}

	switch (osabi) {
		case ELFOSABI_ARM_AEABI:	return "ARM EABI";
		case ELFOSABI_ARM:		return "ARM";
		case ELFOSABI_CELL_LV2:		return "Cell LV2";
		case ELFOSABI_CAFEOS:		return "Cafe OS";	// Wii U
		case ELFOSABI_STANDALONE:	return "Embedded";
		default:			break;
	}

	// Unknown OS ABI...
	return nullptr;
}

} }
