/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * EXEData.cpp: DOS/Windows executable data.                               *
 *                                                                         *
 * Copyright (c) 2016-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "EXEData.hpp"

#include "Other/exe_pe_structs.h"
#include "Other/exe_le_structs.h"

#include "libi18n/i18n.h"

// C++ STL classes
using std::array;

namespace LibRomData { namespace EXEData {

struct MachineType {
	uint16_t cpu;
	const char *name;
};

// PE machine types.
// NOTE: The cpu field *must* be sorted in ascending order.
static constexpr array<MachineType, 40> machineTypes_PE = {{
	{IMAGE_FILE_MACHINE_I386,	"Intel i386"},
	{IMAGE_FILE_MACHINE_R3000_BE,	"MIPS R3000 (big-endian)"},
	{IMAGE_FILE_MACHINE_R3000,	"MIPS R3000"},
	{IMAGE_FILE_MACHINE_R4000,	"MIPS R4000"},
	{IMAGE_FILE_MACHINE_R10000,	"MIPS R10000"},
	{IMAGE_FILE_MACHINE_WCEMIPSV2,	"MIPS (WCE v2)"},
	{IMAGE_FILE_MACHINE_ALPHA,	"DEC Alpha AXP"},
	{IMAGE_FILE_MACHINE_SH3,	"Hitachi SH3"},
	{IMAGE_FILE_MACHINE_SH3DSP,	"Hitachi SH3 DSP"},
	{IMAGE_FILE_MACHINE_SH3E,	"Hitachi SH3E"},
	{IMAGE_FILE_MACHINE_SH4,	"Hitachi SH4"},
	{IMAGE_FILE_MACHINE_SH5,	"Hitachi SH5"},
	{IMAGE_FILE_MACHINE_ARM,	"ARM"},
	{IMAGE_FILE_MACHINE_THUMB,	"ARM Thumb"},
	{IMAGE_FILE_MACHINE_ARMNT,	"ARM Thumb-2"},
	{IMAGE_FILE_MACHINE_AM33,	"Matsushita AM33"},
	{IMAGE_FILE_MACHINE_POWERPC,	"PowerPC"},
	{IMAGE_FILE_MACHINE_POWERPCFP,	"PowerPC with FPU"},
	{IMAGE_FILE_MACHINE_POWERPCBE,	"PowerPC (big-endian; Xenon)"},
	{IMAGE_FILE_MACHINE_IA64,	"Intel Itanium"},
	{IMAGE_FILE_MACHINE_MIPS16,	"MIPS16"},
	{IMAGE_FILE_MACHINE_M68K,	"Motorola 68000"},
	{IMAGE_FILE_MACHINE_ALPHA64,	"DEC Alpha AXP (64-bit)"},
	{IMAGE_FILE_MACHINE_PA_RISC,	"PA-RISC"},
	{IMAGE_FILE_MACHINE_MIPSFPU,	"MIPS with FPU"},
	{IMAGE_FILE_MACHINE_MIPSFPU16,	"MIPS16 with FPU"},
	{IMAGE_FILE_MACHINE_TRICORE,	"Infineon TriCore"},
	{IMAGE_FILE_MACHINE_MPPC_601,	"PowerPC (big-endian)"},
	{IMAGE_FILE_MACHINE_CEF,	"Common Executable Format"},
	{IMAGE_FILE_MACHINE_EBC,	"EFI Byte Code"},
	{IMAGE_FILE_MACHINE_RISCV32,	"RISC-V (32-bit address space)"},
	{IMAGE_FILE_MACHINE_RISCV64,	"RISC-V (64-bit address space)"},
	{IMAGE_FILE_MACHINE_RISCV128,	"RISC-V (128-bit address space)"},
	{IMAGE_FILE_MACHINE_LOONGARCH32, "LoongArch (32-bit)"},
	{IMAGE_FILE_MACHINE_LOONGARCH64, "LoongArch (64-bit)"},
	{IMAGE_FILE_MACHINE_AMD64,	"AMD64"},
	{IMAGE_FILE_MACHINE_M32R,	"Mitsubishi M32R"},
	{IMAGE_FILE_MACHINE_ARM64EC,	"ARM (64-bit) (emulation-compatible)"},
	{IMAGE_FILE_MACHINE_ARM64,	"ARM (64-bit)"},
	{IMAGE_FILE_MACHINE_CEE,	"MSIL"},
}};

// LE machine types.
// NOTE: The cpu field *must* be sorted in ascending order.
static constexpr array<MachineType, 9> machineTypes_LE = {{
	{LE_CPU_80286,		"Intel i286"},
	{LE_CPU_80386,		"Intel i386"},
	{LE_CPU_80486,		"Intel i486"},
	{LE_CPU_80586,		"Intel Pentium"},
	{LE_CPU_i860_N10,	"Intel i860 XR (N10)"},
	{LE_CPU_i860_N11,	"Intel i860 XP (N11)"},
	{LE_CPU_MIPS_I,		"MIPS Mark I (R2000, R3000"},
	{LE_CPU_MIPS_II,	"MIPS Mark II (R6000)"},
	{LE_CPU_MIPS_III,	"MIPS Mark III (R4000)"},
}};

// Subsystem names
static constexpr array<const char*, IMAGE_SUBSYSTEM_WINDOWS_BOOT_APPLICATION+1> subsystemNames = {{
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
	auto pPE = std::lower_bound(machineTypes_PE.cbegin(), machineTypes_PE.cend(), cpu,
		[](const MachineType &pe, uint16_t cpu) noexcept -> bool {
			return (pe.cpu < cpu);
		});
	if (pPE == machineTypes_PE.cend() || pPE->cpu != cpu) {
		return nullptr;
	}
	return pPE->name;
}

/**
 * Look up an LE machine type. (CPU)
 * @param cpu LE machine type.
 * @return Machine type name, or nullptr if not found.
 */
const char *lookup_le_cpu(uint16_t cpu)
{
	// Do a binary search.
	auto pLE = std::lower_bound(machineTypes_LE.cbegin(), machineTypes_LE.cend(), cpu,
		[](const MachineType &pe, uint16_t cpu) noexcept -> bool {
			return (pe.cpu < cpu);
		});
	if (pLE == machineTypes_LE.cend() || pLE->cpu != cpu) {
		return nullptr;
	}
	return pLE->name;
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

	return pgettext_expr("EXE|Subsystem", subsystemNames[subsystem]);
}

} }
