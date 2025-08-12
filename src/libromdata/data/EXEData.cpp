/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * EXEData.cpp: DOS/Windows executable data.                               *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "EXEData.hpp"

#include "Other/exe_pe_structs.h"
#include "Other/exe_le_structs.h"

// C++ STL classes
using std::array;

namespace LibRomData { namespace EXEData {

/**
 * EXE machine type data is generated using EXEMachineTypes_parser.py.
 * This file is *not* automatically updated by the build system.
 * The parser script should be run manually when the source file
 * is updated to add new mappers.
 *
 * - Source file: EXE(LE|PE)MachineTypes_data.txt
 * - Source file: EXE(LE|PE)MachineTypes_data.h
 */
#include "EXELEMachineTypes_data.h"
#include "EXEPEMachineTypes_data.h"

// Subsystem names
static const array<const char*, IMAGE_SUBSYSTEM_WINDOWS_BOOT_APPLICATION+1> subsystemNames = {{
	// IMAGE_SUBSYSTEM_UNKNOWN
	nullptr,
	// tr: IMAGE_SUBSYSTEM_NATIVE
	NOP_C_("EXE|Subsystem", "Native"),
	// tr: IMAGE_SUBSYSTEM_WINDOWS_GUI
	NOP_C_("EXE|Subsystem", "Windows"),
	// tr: IMAGE_SUBSYSTEM_WINDOWS_CUI
	NOP_C_("EXE|Subsystem", "Console"),
	// Unused...
	nullptr,
	// tr: IMAGE_SUBSYSTEM_OS2_CUI
	NOP_C_("EXE|Subsystem", "OS/2 Console"),
	// Unused...
	nullptr,
	// tr: IMAGE_SUBSYSTEM_POSIX_CUI
	NOP_C_("EXE|Subsystem", "POSIX Console"),
	// tr: IMAGE_SUBSYSTEM_NATIVE_WINDOWS
	NOP_C_("EXE|Subsystem", "Win9x Native Driver"),
	// tr: IMAGE_SUBSYSTEM_WINDOWS_CE_GUI
	NOP_C_("EXE|Subsystem", "Windows CE"),
	// tr: IMAGE_SUBSYSTEM_EFI_APPLICATION
	NOP_C_("EXE|Subsystem", "EFI Application"),
	// tr: IMAGE_SUBSYSTEM_EFI_BOOT_SERVICE_DRIVER
	NOP_C_("EXE|Subsystem", "EFI Boot Service Driver"),
	// tr: IMAGE_SUBSYSTEM_EFI_RUNTIME_DRIVER
	NOP_C_("EXE|Subsystem", "EFI Runtime Driver"),
	// tr: IMAGE_SUBSYSTEM_EFI_ROM
	NOP_C_("EXE|Subsystem", "EFI ROM Image"),
	// tr: IMAGE_SUBSYSTEM_XBOX
	NOP_C_("EXE|Subsystem", "Xbox"),
	// Unused...
	nullptr,
	// tr: IMAGE_SUBSYSTEM_WINDOWS_BOOT_APPLICATION
	NOP_C_("EXE|Subsystem", "Boot Application"),
}};

/** Public functions **/

/**
 * Look up a PE machine type. (CPU)
 * @param cpu PE machine type.
 * @return Machine type name, or nullptr if not found.
 */
const char *lookup_pe_cpu(uint16_t cpu)
{
	// Do a binary search.
	static const auto *const p_EXEPEMachineTypes_offtbl_end = &EXEPEMachineTypes_offtbl[ARRAY_SIZE(EXEPEMachineTypes_offtbl)];
	auto pPE = std::lower_bound(EXEPEMachineTypes_offtbl, p_EXEPEMachineTypes_offtbl_end, cpu,
		[](const EXEPEMachineTypes_offtbl_t &pe, uint16_t cpu) noexcept -> bool {
			return (pe.machineType < cpu);
		});
	if (pPE == p_EXEPEMachineTypes_offtbl_end || pPE->machineType != cpu) {
		return nullptr;
	}
	return &EXEPEMachineTypes_strtbl[pPE->offset];
}

/**
 * Look up an LE machine type. (CPU)
 * @param cpu LE machine type.
 * @return Machine type name, or nullptr if not found.
 */
const char *lookup_le_cpu(uint16_t cpu)
{
	// Do a binary search.
	static const auto *const p_EXELEMachineTypes_offtbl_end = &EXELEMachineTypes_offtbl[ARRAY_SIZE(EXELEMachineTypes_offtbl)];
	auto pLE = std::lower_bound(EXELEMachineTypes_offtbl, p_EXELEMachineTypes_offtbl_end, cpu,
		[](const EXELEMachineTypes_offtbl_t &pe, uint16_t cpu) noexcept -> bool {
			return (pe.machineType < cpu);
		});
	if (pLE == p_EXELEMachineTypes_offtbl_end || pLE->machineType != cpu) {
		return nullptr;
	}
	return &EXELEMachineTypes_strtbl[pLE->offset];
}

/**
 * Look up a PE subsystem name.
 * NOTE: This function returns localized subsystem names.
 * @param subsystem PE subsystem
 * @return PE subsystem name, or nullptr if invalid.
 */
const char *lookup_pe_subsystem(uint16_t subsystem)
{
	if (subsystem >= subsystemNames.size())
		return nullptr;

	const char *const name = subsystemNames[subsystem];
	return (name) ? pgettext_expr("EXE|Subsystem", name) : nullptr;
}

} }
