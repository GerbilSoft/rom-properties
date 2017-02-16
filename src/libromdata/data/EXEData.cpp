/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * EXEData.cpp: DOS/Windows executable data.                               *
 *                                                                         *
 * Copyright (c) 2016-2017 by David Korth.                                 *
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
 * You should have received a copy of the GNU General Public License along *
 * with this program; if not, write to the Free Software Foundation, Inc., *
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.           *
 ***************************************************************************/

#include "EXEData.hpp"
#include "../exe_structs.h"

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
			const rp_char *name;
		};
		static const MachineType machineTypes[];

		/**
		 * bsearch() comparison function for MachineType.
		 * @param a
		 * @param b
		 * @return
		 */
		static int MachineType_compar(const void *a, const void *b);
};

// Machine type lookup.
// NOTE: The cpu field *must* be sorted in ascending order.
const EXEDataPrivate::MachineType EXEDataPrivate::machineTypes[] = {
	{IMAGE_FILE_MACHINE_I386,	_RP("Intel i386")},
	{IMAGE_FILE_MACHINE_R3000_BE,	_RP("MIPS R3000 (big-endian)")},
	{IMAGE_FILE_MACHINE_R3000,	_RP("MIPS R3000")},
	{IMAGE_FILE_MACHINE_R4000,	_RP("MIPS R4000")},
	{IMAGE_FILE_MACHINE_R10000,	_RP("MIPS R10000")},
	{IMAGE_FILE_MACHINE_WCEMIPSV2,	_RP("MIPS (WCE v2)")},
	{IMAGE_FILE_MACHINE_ALPHA,	_RP("DEC Alpha AXP")},
	{IMAGE_FILE_MACHINE_SH3,	_RP("Hitachi SH3")},
	{IMAGE_FILE_MACHINE_SH3DSP,	_RP("Hitachi SH3 DSP")},
	{IMAGE_FILE_MACHINE_SH3E,	_RP("Hitachi SH3E")},
	{IMAGE_FILE_MACHINE_SH4,	_RP("Hitachi SH4")},
	{IMAGE_FILE_MACHINE_SH5,	_RP("Hitachi SH5")},
	{IMAGE_FILE_MACHINE_ARM,	_RP("ARM")},
	{IMAGE_FILE_MACHINE_THUMB,	_RP("ARM Thumb")},
	{IMAGE_FILE_MACHINE_ARMNT,	_RP("ARM Thumb-2")},
	{IMAGE_FILE_MACHINE_AM33,	_RP("Matsushita AM33")},
	{IMAGE_FILE_MACHINE_POWERPC,	_RP("PowerPC")},
	{IMAGE_FILE_MACHINE_POWERPCFP,	_RP("PowerPC with FPU")},
	{IMAGE_FILE_MACHINE_IA64,	_RP("Intel Itanium")},
	{IMAGE_FILE_MACHINE_MIPS16,	_RP("MIPS16")},
	{IMAGE_FILE_MACHINE_ALPHA64,	_RP("DEC Alpha AXP (64-bit)")},
	{IMAGE_FILE_MACHINE_MIPSFPU,	_RP("MIPS with FPU")},
	{IMAGE_FILE_MACHINE_MIPSFPU16,	_RP("MIPS16 with FPU")},
	{IMAGE_FILE_MACHINE_TRICORE,	_RP("Infineon TriCore")},
	{IMAGE_FILE_MACHINE_CEF,	_RP("Common Executable Format")},
	{IMAGE_FILE_MACHINE_EBC,	_RP("EFI Byte Code")},
	{IMAGE_FILE_MACHINE_AMD64,	_RP("AMD64")},
	{IMAGE_FILE_MACHINE_M32R,	_RP("Mitsubishi M32R")},
	{IMAGE_FILE_MACHINE_ARM64,	_RP("ARM (64-bit)")},
	{IMAGE_FILE_MACHINE_CEE,	_RP("MSIL (.NET)")},

	{0, nullptr}
};

/**
 * bsearch() comparison function for MachineType.
 * @param a
 * @param b
 * @return
 */
int EXEDataPrivate::MachineType_compar(const void *a, const void *b)
{
	const uint16_t cpu1 = static_cast<const MachineType*>(a)->cpu;
	const uint16_t cpu2 = static_cast<const MachineType*>(b)->cpu;
	if (cpu1 < cpu2) return -1;
	if (cpu1 > cpu2) return 1;
	return 0;
}

/**
 * Look up a machine type. (CPU)
 * @param cpu Macine type.
 * @return Machine type name, or nullptr if not found.
 */
const rp_char *EXEData::lookup_cpu(uint16_t cpu)
{
	// Do a binary search.
	const EXEDataPrivate::MachineType key = {cpu, nullptr};
	const EXEDataPrivate::MachineType *res =
		static_cast<const EXEDataPrivate::MachineType*>(bsearch(&key,
			EXEDataPrivate::machineTypes,
			ARRAY_SIZE(EXEDataPrivate::machineTypes)-1,
			sizeof(EXEDataPrivate::MachineType),
			EXEDataPrivate::MachineType_compar));
	return (res ? res->name : nullptr);
}

}
