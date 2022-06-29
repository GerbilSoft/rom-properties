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

struct MachineType {
	uint16_t cpu;
	const char *name;
};

// ELF machine types. (contiguous low IDs)
// Reference: https://github.com/file/file/blob/master/magic/Magdir/elf
static const char *const machineTypes_low[] = {
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
	"IBM System/370",

	// 10
	"MIPS R3000 LE (deprecated)",
	"SPARC v9 (deprecated)",
	nullptr, nullptr, nullptr,
	"HP PA-RISC",
	"nCUBE",
	"Fujitsu VPP500",
	"SPARC32PLUS",
	"Intel i960",

	// 20
	"PowerPC",		// or Cisco 4500?
	"64-bit PowerPC",	// or Cisco 7500?
	"IBM System/390",
	"Cell SPU",
	"Cisco SVIP",
	"Cisco 7200",
	nullptr, nullptr, nullptr, nullptr,

	// 30
	nullptr, nullptr, nullptr, nullptr,
	nullptr, nullptr,
	"NEC V800",		// or Cisco 12000?
	"Fujitsu FR20",
	"TRW RH-32",
	"Motorola M*Core",

	// 40
	"ARM",
	"DEC Alpha",
	"Renesas SuperH",
	"SPARC v9",
	"Siemens Tricore embedded processor",
	"Argonaut RISC Core",
	"Renesas H8/300",
	"Renesas H8/300H",
	"Renesas H8S",
	"Renesas H8/500",

	// 50
	"Intel Itanium",
	"Stanford MIPS-X",
	"Motorola Coldfire",
	"Motorola MC68HC12",
	"Fujitsu Multimedia Accelerator",
	"Siemens PCP",
	"Sony nCPU",
	"Denso NDR1",
	"Motorola Star*Core",
	"Toyota ME16",

	// 60
	"STMicroelectronics ST100",
	"Advanced Logic Corp. TinyJ",
	"AMD64",
	"Sony DSP",
	"DEC PDP-10",
	"DEC PDP-11",
	"Siemens FX66",
	"STMicroelectronics ST9+ 8/16-bit",
	"STMicroelectronics ST7 8-bit",
	"Motorola MC68HC16",

	// 70
	"Motorola MC68HC11",
	"Motorola MC68HC08",
	"Motorola MC68HC05",
	"SGI SVx or Cray NV1",
	"STMicroelectronics ST19 8-bit",
	"Digital VAX",
	"Axis cris",
	"Infineon Technologies 32-bit embedded CPU",
	"Element 14 64-bit DSP",
	"LSI Logic 16-bit DSP",

	// 80
	"Donald Knuth's 64-bit MMIX CPU",
	"Harvard machine-independent",
	"SiTera Prism",
	"Atmel AVR 8-bit",
	"Fujitsu FR30",
	"Mitsubishi D10V",
	"Mitsubishi D30V",
	"Renesas V850",		/* formerly NEC V850 */
	"Renesas M32R",		/* formerly Mitsubishi M32R */
	"Matsushita MN10300",

	// 90
	"Matsushita MN10200",
	"picoJava",
	"OpenRISC 1000",
	"ARCompact",
	"Tensilica Xtensa",
	"Alphamosaic VideoCore",
	"Thompson Multimedia GPP",
	"National Semiconductor 32000",
	"Tenor Network TPC",
	"Trebia SNP 1000",

	// 100
	"STMicroelectronics ST200",
	"Ubicom IP2022",
	"MAX Processor",
	"National Semiconductor CompactRISC",
	"Fujitsu F2MC16",
	"TI msp430",
	"ADI Blackfin",
	"S1C33 Family of Seiko Epson",
	"Sharp embedded",
	"Arca RISC",

	// 110
	"Unicore",
	"eXcess",
	"Icera Deep Execution Processor",
	"Altera Nios II",
	"National Semiconductor CRX",
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
	"ADI SHARC family",
	"Cyan Technology eCOG2",
	"Sunplus S+core7 RISC",
	"New Japan Radio (NJR) 24-bit DSP",
	"Broadcom VideoCore III",
	"Lattice Mico32",
	"Seiko Epson C17 family",

	// 140
	"TI TMS320C6000 DSP family",
	"TI TMS320C2000 DSP family",
	"TI TMS320C55x DSP family",
	"TI Application-Specific RISC",
	"TI Programmable Realtime Unit",
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
	"Intel 8051",
	"STMicroelectronics STxP7x family",
	"Andes Technology NDS32",
	"Cyan eCOG1X family",
	"Dallas MAXQ30",

	// 170
	"New Japan Radio (NJR) 16-bit DSP",
	"M2000 Reconfigurable RISC",
	"Cray NV2 vector architecture",
	"Renesas RX family",
	"Imagination Technologies Meta",
	"MCST Elbrus",
	"Cyan Technology eCOG16 family",
	"National Semiconductor CompactRISC (16-bit)",
	"Freescale Extended Time Processing Unit",
	"Infineon SLE9X",

	// 180
	"Intel L10M",
	"Intel K10M",
	"Intel (182)",
	"ARM AArch64",
	"ARM (184)",
	"Atmel AVR32",
	"STMicroelectronics STM8 8-bit",
	"Tilera TILE64",
	"Tilera TILEPro",
	"Xilinx MicroBlaze 32-bit RISC",

	// 190
	"NVIDIA CUDA",
	"Tilera TILE-Gx",
	"CloudShield",
	"KIPO-KAIST Core-A 1st gen.",
	"KIPO-KAIST Core-A 2nd gen.",
	"Synopsys ARCompact V2",
	"Open8 RISC",
	"Renesas RL78 family",
	"Broadcom VideoCore V",
	"Renesas 78K0R",

	// 200
	"Freescale 56800EX",
	"Beyond BA1",
	"Beyond BA2",
	"XMOS xCORE",
	"Micrchip 8-bit PIC(r)",
	"Intel Graphics Technology",
	"Intel (206)",
	"Intel (207)",
	"Intel (208)",
	"Intel (209)",

	// 210
	"KM211 KM32",
	"KM211 KMX32",
	"KM211 KMX16",
	"KM211 KMX8",
	"KM211 KVARC",
	"Paneve CDP",
	"Cognitive Smart Memory",
	"Bluechip Systems CoolEngine",
	"Nanoradio Optimized RISC",
	"CSR Kalimba",

	// 220
	"Zilog Z80",
	"Controls and Data Services VISIUMcore",
	"FTDI Chip FT32",
	"Moxie processor",
	"AMD GPU",

	nullptr
};

// ELF machine types. (other IDs)
// Reference: https://github.com/file/file/blob/master/magic/Magdir/elf
static const MachineType machineTypes_other[] = {
	{243,		"RISC-V"},
	{244,		"Lanai"},
	{247,		"Linux eBPF"},
	{250,		"Netronome Flow Processor"},
	{251,		"NEC VE"},
	{252,		"C-SKY"},

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

	{0, nullptr}
};

// ELF OS ABI names.
// Reference: https://github.com/file/file/blob/master/magic/Magdir/elf
static const char *const osabi_names[] = {
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
 * Look up an ELF machine type. (CPU)
 * @param cpu ELF machine type.
 * @return Machine type name, or nullptr if not found.
 */
const char *lookup_cpu(uint16_t cpu)
{
	static_assert(ARRAY_SIZE(machineTypes_low) == 224+2,
		"ELFData::machineTypes_low[] is missing entries.");
	if (cpu < ARRAY_SIZE(machineTypes_low)-1) {
		// CPU ID is in the contiguous low IDs array.
		return machineTypes_low[cpu];
	}

	// CPU ID is in the "other" IDs array.
	// Do a binary search.
	const MachineType key = {cpu, nullptr};
	const MachineType *res =
		static_cast<const MachineType*>(bsearch(&key,
			machineTypes_other,
			ARRAY_SIZE(machineTypes_other)-1,
			sizeof(MachineType),
			MachineType_compar));
	return (res ? res->name : nullptr);
}

/**
 * Look up an ELF OS ABI.
 * @param osabi ELF OS ABI.
 * @return OS ABI name, or nullptr if not found.
 */
const char *lookup_osabi(uint8_t osabi)
{
	if (osabi < ARRAY_SIZE(osabi_names)-1) {
		// OS ABI ID is in the array.
		return osabi_names[osabi];
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

} }
