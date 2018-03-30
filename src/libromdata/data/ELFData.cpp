/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * ELFData.cpp: Executable and Linkable Format data.                       *
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

#include "ELFData.hpp"
#include "Other/elf_structs.h"

// C includes.
#include <stdlib.h>

namespace LibRomData {

class ELFDataPrivate {
	private:
		// Static class.
		ELFDataPrivate();
		~ELFDataPrivate();
		RP_DISABLE_COPY(ELFDataPrivate)

	public:
		// CPUs
		struct MachineType {
			uint16_t cpu;
			const char *name;
		};
		static const char *const machineTypes_low[];
		static const MachineType machineTypes_other[];

		// OS ABIs
		static const char *const osabi_names[];

		/**
		 * bsearch() comparison function for MachineType.
		 * @param a
		 * @param b
		 * @return
		 */
		static int RP_C_API MachineType_compar(const void *a, const void *b);
};

// ELF machine types. (contiguous low IDs)
// Reference: https://github.com/file/file/blob/master/magic/Magdir/elf
const char *const ELFDataPrivate::machineTypes_low[] = {
	// 0
	"No machine",
	"AT&T WE 32100 (M32)",
	"Sun/Oracle SPARC",
	"Intel i386",
	"Motorola M68K",
	"Motorola M88K",
	"Intel i486",
	"Intel i860",
	"MIPS",
	"Amdahl",

	// 10
	"MIPS (deprecated)",
	"IBM RS/6000",
	nullptr, nullptr, nullptr,
	"HP PA-RISC",
	"nCUBE",
	"Fujitsu VPP500",
	"SPARC32PLUS",
	"Intel i960",

	// 20
	"PowerPC or Cisco 4500",
	"64-bit PowerPC or Cisco 7500",
	"IBM System/390",
	"Cell SPU",
	"Cisco SVIP",
	"Cisco 7200",
	nullptr, nullptr, nullptr, nullptr,

	// 30
	nullptr, nullptr, nullptr, nullptr,
	nullptr, nullptr,
	"NEC V800 or Cisco 12000",
	"Fujitsu FR20",
	"TRW RH-32",
	"Motorola RCE",

	// 40
	"ARM",
	"DEC Alpha",
	"Hitachi/Renesas SuperH",
	"Sun/Oracle SPARC V9",
	"Siemens Tricore Embedded Processor",
	"Argonaut RISC Core",
	"Renesas H8/300",
	"Renesas H8/300H",
	"Renesas H8S",
	"Renesas H8/500",

	// 50
	"Intel Itanium",
	"Stanford MIPS-X",
	"Motorola Coldfire",
	"Motorola M68HC12",
	"Fujitsu MMA",
	"Siemens PCP",
	"Sony nCPU",
	"Denso NDR1",
	"Start*Core",
	"Toyota ME16",

	// 60
	"ST100",
	"Tinyj emb.",
	"AMD64",
	"Sony DSP",
	"DEC PDP-10",
	"DEC PDP-11",
	"FX66",
	"ST9+ 8/16-bit",
	"ST7 8-bit",
	"Motorola MC68HC16",

	// 70
	"Motorola MC68HC11",
	"Motorola MC68HC08",
	"Motorola MC68HC05",
	"SGI SVx or Cray NV1",
	"ST19 8-bit",
	"Digital VAX",
	"Axis cris",
	"Infineon 32-bit embedded",
	"Element 14 64-bit DSP",
	"LSI Logic 16-bit DSP",

	// 80
	"MMIX",
	"Harvard machine-independent",
	"SiTera Prism",
	"Atmel AVR 8-bit",
	"Fujitsu FR-30",
	"Mitsubishi D10V",
	"Mitsubishi D30V",
	"NEC V850",
	"Renesas M32R",
	"Matsushita MN10300",

	// 90
	"Matsushita MN10200",
	"picoJava",
	"OpenRISC",
	"ARC Cores Tangent-A5",
	"Tensilica Xtensa",
	"Alphamosaic VideoCore",
	"Thompson Multimedia",
	"NatSemi 32k",
	"Tenor Network TPC",
	"Trebia SNP 1000",

	// 100
	"STMicroelectronics ST200",
	"Ubicom IP2022",
	"MAX Processor",
	"NatSemi CompactRISC",
	"Fujitsu F2MC16",
	"TI msp430",
	"Analog Devices Blackfin",
	"S1C33 Family of Seiko Epson",
	"Sharp embedded",
	"Arca RISC",

	// 110
	"PKU-Unity Ltd.",
	"eXcess: 16/32/64-bit",
	"Icera Deep Execution Processor",
	"Altera Nios II",
	"NatSemi CRX",
	"Motorola XGATE",
	"Infineon C16x/XC16x",
	"Renesas M16C series",
	"Microchip dsPIC30F",
	"Freescale RISC core",

	// 120
	"Renesas M32C series",
	nullptr, nullptr, nullptr, nullptr,
	nullptr, nullptr, nullptr, nullptr, nullptr,

	// 130
	nullptr,
	"Altium TSK3000 core",
	"Freescale RS08",
	nullptr,
	"Cyan Technology eCOG2",
	"Sunplus S+core7 RISC",
	"New Japan Radio (NJR) 24-bit DSP",
	"Broadcom VideoCore III",
	"LatticeMico32",
	"Seiko Epson C17 family",

	// 140
	"TI TMS320C6000 DSP family",
	"TI TMS320C2000 DSP family",
	"TI TMS320C55x DSP family",
	nullptr, nullptr,
	nullptr, nullptr, nullptr, nullptr, nullptr,

	// 150
	nullptr, nullptr, nullptr, nullptr, nullptr,
	nullptr, nullptr, nullptr, nullptr, nullptr,

	// 160
	"STMicroelectronics 64-bit VLIW DSP",
	"Cypress M8C",
	"Renesas R32C series",
	"NXP TriMedia family",
	"Qualcomm DSP6",
	"Intel 8051 and variants",
	"STMicroelectronics STxP7x family",
	"Andes embedded RISC",
	"Cyan eCOG1X family",
	"Dallas MAXQ30",

	// 170
	"New Japan Radio (NJR) 16-bit DSP",
	"M2000 Reconfigurable RISC",
	"Cray NV2 vector architecture",
	"Renesas RX family",
	"META",
	"MCST Elbrus",
	"Cyan Technology eCOG16 family",
	"NatSemi CompactRISC",
	"Freescale Extended Time Processing Unit",
	"Infineon SLE9X",

	// 180
	"Intel L10M",
	"Intel K10M",
	nullptr,
	"ARM AArch64",
	nullptr,
	"Atmel 32-bit family",
	"STMicroelectronics STM8 8-bit",
	"Tilera TILE64",
	"Tilera TILEPro",
	"Xilinx MicroBlaze 32-bit RISC",

	// 190
	"NVIDIA CUDA architecture",
	"Tilera TILE-Gx",
	nullptr, nullptr, nullptr, nullptr, nullptr,
	"Renesas RL78 family",
	nullptr,
	"Renesas 68K0R",

	// 200
	"Freescale 56800EX",
	"Beyond BA1",
	"Beyond BA2",
	"XMOS xCORE",
	"Micrchip 8-bit PIC(r)",
	nullptr, nullptr, nullptr, nullptr, nullptr,

	// 210
	"KM211 KM32",
	"KM211 KMX32",
	"KM211 KMX16",
	"KM211 KMX8",
	"KM211 KVARC",
	"Paneve CDP",
	"Cognitive Smart Memory",
	"iCelero CoolEngine",
	"Nanoradio Optimized RISC",

	nullptr
};

// ELF machine types. (other IDs)
// Reference: https://github.com/file/file/blob/master/magic/Magdir/elf
const ELFDataPrivate::MachineType ELFDataPrivate::machineTypes_other[] = {
	{243,		"UCB RISC-V"},

	// The following are unofficial and/or obsolete types.
	// TODO: Indicate unofficial/obsolete using a separate flag?
	{0x1057,	"AVR (unofficial)"},
	{0x1059,	"MSP430 (unofficial)"},
	{0x1223,	"Adapteva Epiphany (unofficial)"},
	{0x2530,	"Morpho MT (unofficial)"},
	{0x3330,	"FR30 (unofficial)"},
	{0x3426,	"OpenRISC (obsolete)"},
	{0x4688,	"Infineon C166 (unofficial)"},
	{0x5441,	"Cygnus FRV (unofficial)"},
	{0x5aa5,	"DLX (unofficial)"},
	{0x7650,	"Cygnus D10V (unofficial)"},
	{0x7676,	"Cygnus D30V (unofficial)"},
	{0x8217,	"Ubicom IP2xxx (unofficial)"},
	{0x8472,	"OpenRISC (obsolete)"},
	{0x9025,	"Cygnus PowerPC (unofficial)"},
	{0x9026,	"DEC Alpha (unofficial)"},
	{0x9041,	"Cygnus M32R (unofficial)"},
	{0x9080,	"Cygnus V850 (unofficial)"},
	{0xA390,	"IBM System/390 (obsolete)"},
	{0xABC7,	"Old Xtensa (unofficial)"},
	{0xAD45,	"xstormy16 (unofficial)"},
	{0xBAAB,	"Old MicroBlaze (unofficial)"},
	{0xBEEF,	"Cygnus MN10300 (unofficial)"},
	{0xDEAD,	"Cygnus MN10200 (unofficial)"},
	{0xF00D,	"Toshiba MeP (unofficial)"},
	{0xFEB0,	"Renesas M32C (unofficial)"},
	{0xFEBA,	"Vitesse IQ2000 (unofficial)"},
	{0xFEBB,	"NIOS (unofficial)"},
	{0xFEED,	"Moxie (unofficial)"},

	{0, nullptr}
};

// ELF OS ABI names.
// Reference: https://github.com/file/file/blob/master/magic/Magdir/elf
const char *const ELFDataPrivate::osabi_names[] = {
	// 0
	"UNIX System V",
	"HP-UX",
	"NetBSD",
	"GNU/Linux",
	"GNU/Hurd",
	"86Open",
	"Solaris",
	"Monterey",
	"IRIX",
	"FreeBSD",

	// 10
	"Tru64",
	"Novell Modesto",
	"OpenBSD",
	"OpenVMS",
	"HP NonStop Kernel",
	"AROS Research Operating System",
	"FenixOS",
	"Nuxi CloudABI",

	nullptr
};

/**
 * bsearch() comparison function for MachineType.
 * @param a
 * @param b
 * @return
 */
int RP_C_API ELFDataPrivate::MachineType_compar(const void *a, const void *b)
{
	const uint16_t cpu1 = static_cast<const MachineType*>(a)->cpu;
	const uint16_t cpu2 = static_cast<const MachineType*>(b)->cpu;
	if (cpu1 < cpu2) return -1;
	if (cpu1 > cpu2) return 1;
	return 0;
}

/**
 * Look up an ELF machine type. (CPU)
 * @param cpu ELF machine type.
 * @return Machine type name, or nullptr if not found.
 */
const char *ELFData::lookup_cpu(uint16_t cpu)
{
	static_assert(ARRAY_SIZE(ELFDataPrivate::machineTypes_low) == 218+2,
		"ELFDataPrivate::machineTypes_low[] is missing entries.");
	if (cpu < ARRAY_SIZE(ELFDataPrivate::machineTypes_low)-1) {
		// CPU ID is in the contiguous low IDs array.
		return ELFDataPrivate::machineTypes_low[cpu];
	}

	// CPU ID is in the "other" IDs array.
	// Do a binary search.
	const ELFDataPrivate::MachineType key = {cpu, nullptr};
	const ELFDataPrivate::MachineType *res =
		static_cast<const ELFDataPrivate::MachineType*>(bsearch(&key,
			ELFDataPrivate::machineTypes_other,
			ARRAY_SIZE(ELFDataPrivate::machineTypes_other)-1,
			sizeof(ELFDataPrivate::MachineType),
			ELFDataPrivate::MachineType_compar));
	return (res ? res->name : nullptr);
}

/**
 * Look up an ELF OS ABI.
 * @param cpu ELF OS ABI.
 * @return OS ABI name, or nullptr if not found.
 */
const char *ELFData::lookup_osabi(uint8_t osabi)
{
	if (osabi < ARRAY_SIZE(ELFDataPrivate::osabi_names)-1) {
		// OS ABI ID is in the array.
		return ELFDataPrivate::osabi_names[osabi];
	}

	switch (osabi) {
		case ELFOSABI_ARM_AEABI:	return "ARM EABI";
		case ELFOSABI_ARM:		return "ARM";
		case ELFOSABI_CAFEOS:		return "Cafe OS";	// Wii U
		case ELFOSABI_STANDALONE:	return "Embedded";
		default:			break;
	}

	// Unknown OS ABI...
	return nullptr;
}

}
