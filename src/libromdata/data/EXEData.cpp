/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * EXEData.cpp: DOS/Windows executable data.                               *
 *                                                                         *
 * Copyright (c) 2016-2018 by David Korth.                                 *
 *                                                                         *
 * This program is free software; you can redistribute it and/or modify it *
 * under the terms of the GNU General Public License as published by the   *
 * Free Software Foundation; either version 2 of the License, or (at your  *
 * option) any later version.                                              *
 *                                                                         *
 * This program is distributed in the hope that it will be useful, but     *
 * WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 * GNU General Public License for more details.                            *
 *                                                                         *
 * You should have received a copy of the GNU General Public License       *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.   *
 ***************************************************************************/

#include "EXEData.hpp"
#include "Other/exe_structs.h"

// C includes.
#include <stdlib.h>

namespace LibRomData {

class EXEDataPrivate {
	private:
		// Static class.
		EXEDataPrivate();
		~EXEDataPrivate();
		RP_DISABLE_COPY(EXEDataPrivate)

	public:
		struct MachineType {
			uint16_t cpu;
			const char *name;
		};
		static const MachineType machineTypes_PE[];
		static const MachineType machineTypes_LE[];

		/**
		 * bsearch() comparison function for MachineType.
		 * @param a
		 * @param b
		 * @return
		 */
		static int RP_C_API MachineType_compar(const void *a, const void *b);
};

// PE machine types.
// NOTE: The cpu field *must* be sorted in ascending order.
const EXEDataPrivate::MachineType EXEDataPrivate::machineTypes_PE[] = {
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
	{IMAGE_FILE_MACHINE_IA64,	"Intel Itanium"},
	{IMAGE_FILE_MACHINE_MIPS16,	"MIPS16"},
	{IMAGE_FILE_MACHINE_ALPHA64,	"DEC Alpha AXP (64-bit)"},
	{IMAGE_FILE_MACHINE_MIPSFPU,	"MIPS with FPU"},
	{IMAGE_FILE_MACHINE_MIPSFPU16,	"MIPS16 with FPU"},
	{IMAGE_FILE_MACHINE_TRICORE,	"Infineon TriCore"},
	{IMAGE_FILE_MACHINE_CEF,	"Common Executable Format"},
	{IMAGE_FILE_MACHINE_EBC,	"EFI Byte Code"},
	{IMAGE_FILE_MACHINE_AMD64,	"AMD64"},
	{IMAGE_FILE_MACHINE_M32R,	"Mitsubishi M32R"},
	{IMAGE_FILE_MACHINE_ARM64,	"ARM (64-bit)"},
	{IMAGE_FILE_MACHINE_CEE,	"MSIL"},

	{0, nullptr}
};

// LE machine types.
// NOTE: The cpu field *must* be sorted in ascending order.
const EXEDataPrivate::MachineType EXEDataPrivate::machineTypes_LE[] = {
	{LE_CPU_80286,		"Intel i286"},
	{LE_CPU_80386,		"Intel i386"},
	{LE_CPU_80486,		"Intel i486"},
	{LE_CPU_80586,		"Intel Pentium"},
	{LE_CPU_i860_N10,	"Intel i860 XR (N10)"},
	{LE_CPU_i860_N11,	"Intel i860 XP (N11)"},
	{LE_CPU_MIPS_I,		"MIPS Mark I (R2000, R3000"},
	{LE_CPU_MIPS_II,	"MIPS Mark II (R6000)"},
	{LE_CPU_MIPS_III,	"MIPS Mark III (R4000)"},

	{0, nullptr}
};

/**
 * bsearch() comparison function for MachineType.
 * @param a
 * @param b
 * @return
 */
int RP_C_API EXEDataPrivate::MachineType_compar(const void *a, const void *b)
{
	const uint16_t cpu1 = static_cast<const MachineType*>(a)->cpu;
	const uint16_t cpu2 = static_cast<const MachineType*>(b)->cpu;
	if (cpu1 < cpu2) return -1;
	if (cpu1 > cpu2) return 1;
	return 0;
}

/**
 * Look up a PE machine type. (CPU)
 * @param cpu PE machine type.
 * @return Machine type name, or nullptr if not found.
 */
const char *EXEData::lookup_pe_cpu(uint16_t cpu)
{
	// Do a binary search.
	const EXEDataPrivate::MachineType key = {cpu, nullptr};
	const EXEDataPrivate::MachineType *res =
		static_cast<const EXEDataPrivate::MachineType*>(bsearch(&key,
			EXEDataPrivate::machineTypes_PE,
			ARRAY_SIZE(EXEDataPrivate::machineTypes_PE)-1,
			sizeof(EXEDataPrivate::MachineType),
			EXEDataPrivate::MachineType_compar));
	return (res ? res->name : nullptr);
}

/**
 * Look up an LE machine type. (CPU)
 * @param cpu LE machine type.
 * @return Machine type name, or nullptr if not found.
 */
const char *EXEData::lookup_le_cpu(uint16_t cpu)
{
	// Do a binary search.
	const EXEDataPrivate::MachineType key = {cpu, nullptr};
	const EXEDataPrivate::MachineType *res =
		static_cast<const EXEDataPrivate::MachineType*>(bsearch(&key,
			EXEDataPrivate::machineTypes_LE,
			ARRAY_SIZE(EXEDataPrivate::machineTypes_LE)-1,
			sizeof(EXEDataPrivate::MachineType),
			EXEDataPrivate::MachineType_compar));
	return (res ? res->name : nullptr);
}

}
