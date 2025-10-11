/** EXE PE Machine Types (generated from EXEPEMachineTypes_data.txt) **/
#pragma once

#include <stdint.h>

static const char EXEPEMachineTypes_strtbl[] =
	"\x00" "Intel i386" "\x00" "Intel i860" "\x00" "MIPS R3000 (big-e"
	"ndian)" "\x00" "MIPS R3000" "\x00" "MIPS R4000" "\x00" "MIPS R10"
	"000" "\x00" "MIPS (WCE v2)" "\x00" "DEC Alpha AXP" "\x00" "Hitac"
	"hi SH3" "\x00" "Hitachi SH3 DSP" "\x00" "Hitachi SH3E" "\x00" "H"
	"itachi SH4" "\x00" "Hitachi SH5" "\x00" "ARMv4" "\x00" "ARMv4T ("
	"Thumb)" "\x00" "ARMv7 (Thumb-2)" "\x00" "Matsushita AM33" "\x00" "R"
	"S/6000" "\x00" "PowerPC" "\x00" "PowerPC with FPU" "\x00" "Power"
	"PC (64-bit) (big-endian; Xenon)" "\x00" "Intel Itanium" "\x00" "M"
	"IPS16" "\x00" "Motorola 68000" "\x00" "DEC Alpha AXP (64-bit)" "\x00"
	"PA-RISC" "\x00" "MIPS with FPU" "\x00" "MIPS16 with FPU" "\x00" "H"
	"itachi SH (big-endian)" "\x00" "Infineon TriCore" "\x00" "Hitach"
	"i SH (little-endian)" "\x00" "PowerPC (big-endian)" "\x00" "Comm"
	"on Executable Format" "\x00" "EFI Byte Code" "\x00" "CHPEv1 i386"
	"\x00" "RISC-V (32-bit address space)" "\x00" "RISC-V (64-bit add"
	"ress space)" "\x00" "RISC-V (128-bit address space)" "\x00" "Loo"
	"ngArch (32-bit)" "\x00" "LoongArch (64-bit)" "\x00" "AMD64" "\x00"
	"Mitsubishi M32R" "\x00" "ARM64EC" "\x00" "ARM64X" "\x00" "ARM64" "\x00"
	"Microsoft OMNI VM (omniprox.dll)" "\x00" "COM+ Execution Engine" "\x00";

struct EXEPEMachineTypes_offtbl_t {
	uint16_t machineType;
	uint16_t offset;
};

static const EXEPEMachineTypes_offtbl_t EXEPEMachineTypes_offtbl[] = {
	{0x014c, 1},
	{0x014d, 12},
	{0x0160, 23},
	{0x0162, 47},
	{0x0166, 58},
	{0x0168, 69},
	{0x0169, 81},
	{0x0184, 95},
	{0x01a2, 109},
	{0x01a3, 121},

	{0x01a4, 137},
	{0x01a6, 150},
	{0x01a8, 162},
	{0x01c0, 174},
	{0x01c2, 180},
	{0x01c4, 195},
	{0x01d3, 211},
	{0x01df, 227},
	{0x01f0, 235},
	{0x01f1, 243},

	{0x01f2, 260},
	{0x0200, 297},
	{0x0266, 311},
	{0x0268, 318},
	{0x0284, 333},
	{0x0290, 356},
	{0x0366, 364},
	{0x0466, 378},
	{0x0500, 394},
	{0x0520, 418},

	{0x0550, 435},
	{0x0601, 462},
	{0x0cef, 483},
	{0x0ebc, 508},
	{0x3a64, 522},
	{0x5032, 534},
	{0x5064, 564},
	{0x5128, 594},
	{0x6232, 625},
	{0x6264, 644},

	{0x8664, 663},
	{0x9041, 669},
	{0xa641, 685},
	{0xa64e, 693},
	{0xaa64, 700},
	{0xace1, 706},
	{0xc0ee, 739},
};
