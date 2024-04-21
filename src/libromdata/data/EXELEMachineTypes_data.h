/** EXE LE Machine Types (generated from EXELEMachineTypes_data.txt) **/
#pragma once

#include <stdint.h>

static const char EXELEMachineTypes_strtbl[] =
	"\x00" "Intel i286" "\x00" "Intel i386" "\x00" "Intel i486" "\x00"
	"Intel Pentium" "\x00" "Intel i860 XR (N10)" "\x00" "Intel i860 X"
	"P (N11)" "\x00" "MIPS Mark I (R2000, R3000" "\x00" "MIPS Mark II"
	" (R6000)" "\x00" "MIPS Mark III (R4000)" "\x00";

struct EXELEMachineTypes_offtbl_t {
	uint8_t machineType;
	uint8_t offset;
};

static const EXELEMachineTypes_offtbl_t EXELEMachineTypes_offtbl[] = {
	{0x01, 1},
	{0x02, 12},
	{0x03, 23},
	{0x04, 34},
	{0x20, 48},
	{0x21, 68},
	{0x40, 88},
	{0x41, 114},
	{0x42, 135},
};
