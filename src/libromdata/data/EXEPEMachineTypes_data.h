/** EXE PE Machine Types (generated from EXEPEMachineTypes_data.txt) **/
#pragma once

#include <stdint.h>

static const char EXEPEMachineTypes_strtbl[] =
	"\x00" "Intel i386" "\x00" "MIPS R3000 (big-endian)" "\x00" "MIPS"
	" R3000" "\x00" "MIPS R4000" "\x00" "MIPS R10000" "\x00" "MIPS (W"
	"CE v2)" "\x00" "DEC Alpha AXP" "\x00" "Hitachi SH3" "\x00" "Hita"
	"chi SH3 DSP" "\x00" "Hitachi SH3E" "\x00" "Hitachi SH4" "\x00" "H"
	"itachi SH5" "\x00" "ARM" "\x00" "ARM Thumb" "\x00" "ARM Thumb-2" "\x00"
	"Matsushita AM33" "\x00" "PowerPC" "\x00" "PowerPC with FPU" "\x00"
	"PowerPC (big-endian; Xenon)" "\x00" "Intel Itanium" "\x00" "MIPS"
	"16" "\x00" "Motorola 68000" "\x00" "DEC Alpha AXP (64-bit)" "\x00"
	"PA-RISC" "\x00" "MIPS with FPU" "\x00" "MIPS16 with FPU" "\x00" "I"
	"nfineon TriCore" "\x00" "PowerPC (big-endian)" "\x00" "Common Ex"
	"ecutable Format" "\x00" "EFI Byte Code" "\x00" "RISC-V (32-bit a"
	"ddress space)" "\x00" "RISC-V (64-bit address space)" "\x00" "RI"
	"SC-V (128-bit address space)" "\x00" "LoongArch (32-bit)" "\x00" "L"
	"oongArch (64-bit)" "\x00" "AMD64" "\x00" "Mitsubishi M32R" "\x00"
	"ARM (64-bit) (emulation-compatible)" "\x00" "ARM (64-bit)" "\x00"
	"MSIL" "\x00";

struct EXEPEMachineTypes_offtbl_t {
	uint16_t machineType;
	uint16_t offset;
};

static const EXEPEMachineTypes_offtbl_t EXEPEMachineTypes_offtbl[] = {
	{0x014c, 1},
	{0x0160, 12},
	{0x0162, 36},
	{0x0166, 47},
	{0x0168, 58},
	{0x0169, 70},
	{0x0184, 84},
	{0x01a2, 98},
	{0x01a3, 110},
	{0x01a4, 126},

	{0x01a6, 139},
	{0x01a8, 151},
	{0x01c0, 163},
	{0x01c2, 167},
	{0x01c4, 177},
	{0x01d3, 189},
	{0x01f0, 205},
	{0x01f1, 213},
	{0x01f2, 230},
	{0x0200, 258},

	{0x0266, 272},
	{0x0268, 279},
	{0x0284, 294},
	{0x0290, 317},
	{0x0366, 325},
	{0x0466, 339},
	{0x0520, 355},
	{0x0601, 372},
	{0x0cef, 393},
	{0x0ebc, 418},

	{0x5032, 432},
	{0x5064, 462},
	{0x5128, 492},
	{0x6232, 523},
	{0x6264, 542},
	{0x8664, 561},
	{0x9041, 567},
	{0xa641, 583},
	{0xaa64, 619},
	{0xc0ee, 632},
};
