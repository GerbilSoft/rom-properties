/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * EXEData.cpp: DOS/Windows executable data.                               *
 *                                                                         *
 * Copyright (c) 2016-2026 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "EXEData.hpp"

#include "Other/exe_pe_structs.h"
#include "Other/exe_le_structs.h"

// Other rom-properties libraries
#include "libi18n/i18n.hpp"

// C++ STL classes
#include <array>
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
	const EXEPEMachineTypes_offtbl_t key = {cpu, 0};
	void *ptr = bsearch(&key, EXEPEMachineTypes_offtbl,
		ARRAY_SIZE(EXEPEMachineTypes_offtbl), sizeof(EXEPEMachineTypes_offtbl[0]),
		[](const void *a, const void *b) -> int
		{
			const EXEPEMachineTypes_offtbl_t *const pa = static_cast<const EXEPEMachineTypes_offtbl_t*>(a);
			const EXEPEMachineTypes_offtbl_t *const pb = static_cast<const EXEPEMachineTypes_offtbl_t*>(b);
			return (static_cast<int>(pa->machineType) - static_cast<int>(pb->machineType));
		});
	if (!ptr) {
		return nullptr;
	}

	const EXEPEMachineTypes_offtbl_t *const pPE = static_cast<const EXEPEMachineTypes_offtbl_t*>(ptr);
	return &EXEPEMachineTypes_strtbl[pPE->offset];
}

/**
 * Look up an LE machine type. (CPU)
 * @param cpu LE machine type.
 * @return Machine type name, or nullptr if not found.
 */
const char *lookup_le_cpu(uint16_t cpu)
{
	// The table uses uint8_t as keys, so we have to make sure the
	// CPU value is in range.
	static constexpr uint16_t EXELE_MachineType_Max = 0x42;
	assert(EXELEMachineTypes_offtbl[ARRAY_SIZE(EXELEMachineTypes_offtbl)-1].machineType == EXELE_MachineType_Max);
	if (cpu > EXELE_MachineType_Max) {
		// Out of range.
		return nullptr;
	}

	// Do a binary search.
	const EXELEMachineTypes_offtbl_t key = {static_cast<uint8_t>(cpu), 0};
	void *ptr = bsearch(&key, EXELEMachineTypes_offtbl,
		ARRAY_SIZE(EXELEMachineTypes_offtbl), sizeof(EXELEMachineTypes_offtbl[0]),
		[](const void *a, const void *b) -> int
		{
			const EXELEMachineTypes_offtbl_t *const pa = static_cast<const EXELEMachineTypes_offtbl_t*>(a);
			const EXELEMachineTypes_offtbl_t *const pb = static_cast<const EXELEMachineTypes_offtbl_t*>(b);
			return (static_cast<int>(pa->machineType) - static_cast<int>(pb->machineType));
		});
	if (!ptr) {
		return nullptr;
	}

	const EXELEMachineTypes_offtbl_t *const pLE = static_cast<const EXELEMachineTypes_offtbl_t*>(ptr);
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
	if (subsystem >= subsystemNames.size()) {
		return nullptr;
	}

	const char *const name = subsystemNames[subsystem];
	return (name) ? pgettext_expr("EXE|Subsystem", name) : nullptr;
}

} } // namespace LibRomData::EXEData
