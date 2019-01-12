/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * macho_structs.h: Mach-O executable structures.                          *
 *                                                                         *
 * Copyright (c) 2019 by David Korth.                                      *
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

// References:
// - https://lowlevelbits.org/parsing-mach-o-files/
// - https://developer.apple.com/documentation/kernel/mach_header?language=objc
// - https://opensource.apple.com/source/xnu/xnu-792/EXTERNAL_HEADERS/mach-o/fat.h.auto.html
// - https://opensource.apple.com/source/xnu/xnu-792/EXTERNAL_HEADERS/mach-o/loader.h.auto.html
// - https://opensource.apple.com/source/xnu/xnu-792/osfmk/mach/machine.h.auto.html
// - https://github.com/file/file/blob/master/magic/Magdir/mach

#ifndef __ROMPROPERTIES_LIBROMDATA_OTHER_MACHO_STRUCTS_H__
#define __ROMPROPERTIES_LIBROMDATA_OTHER_MACHO_STRUCTS_H__

#include "librpbase/common.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#pragma pack(1)

// Capability bits used in the CPU type.
#define CPU_ARCH_MASK	0xFF000000	/* mask for architecture bits */
#define CPU_ARCH_ABI64	0x01000000	/* 64-bit ABI */

// CPU type.
typedef enum {
	CPU_TYPE_ANY		= -1,
	CPU_TYPE_VAX		= 1,
	CPU_TYPE_ROMP		= 2,
	CPU_TYPE_NS32032	= 4,
	CPU_TYPE_NS32332	= 5,
	CPU_TYPE_MC680x0	= 6,
	CPU_TYPE_I386		= 7,
	CPU_TYPE_MIPS		= 8,
	CPU_TYPE_NS32532	= 9,
	CPU_TYPE_MC98000	= 10,
	CPU_TYPE_HPPA		= 11,
	CPU_TYPE_ARM		= 12,
	CPU_TYPE_MC88000	= 13,
	CPU_TYPE_SPARC		= 14,
	CPU_TYPE_I860		= 15,
	CPU_TYPE_ALPHA		= 16,
	CPU_TYPE_RS6000		= 17,
	CPU_TYPE_POWERPC	= 18,
	CPU_TYPE_POWERPC64	= (CPU_TYPE_POWERPC | CPU_ARCH_ABI64),
} cpu_type_t;

// CPU subtype for CPU_TYPE_ANY.
typedef enum {
	CPU_SUBTYPE_MULTIPLE		= -1,
	CPU_SUBTYPE_LITTLE_ENDIAN	= 0,
	CPU_SUBTYPE_BIG_ENDIAN		= 1,
} cpu_subtype_any_t;

// CPU subtype for CPU_TYPE_VAX.
// NOTE: These do *not* necessarily conform to the actual
// CPU ID assigned by DEC available via the SID register.
typedef enum {
	CPU_SUBTYPE_VAX_ALL	= 0,
	CPU_SUBTYPE_VAX780	= 1,
	CPU_SUBTYPE_VAX785	= 2,
	CPU_SUBTYPE_VAX750	= 3,
	CPU_SUBTYPE_VAX730	= 4,
	CPU_SUBTYPE_UVAXI	= 5,
	CPU_SUBTYPE_UVAXII	= 6,
	CPU_SUBTYPE_VAX8200	= 7,
	CPU_SUBTYPE_VAX8500	= 8,
	CPU_SUBTYPE_VAX8600	= 9,
	CPU_SUBTYPE_VAX8650	= 10,
	CPU_SUBTYPE_VAX8800	= 11,
	CPU_SUBTYPE_UVAXIII	= 12,
} cpu_subtype_vax_t;

// CPU subtype for CPU_TYPE_MC680x0.
// Definitions are a bit unusual because NeXT considered
// 68030 code as generic 68000 code. MC68030 is kept for
// compatibility purposes; for 68030-specific instructions,
// use MC68030_ONLY.
typedef enum {
	CPU_SUBTYPE_MC680x0_ALL		= 1,
	CPU_SUBTYPE_MC68030		= 1,	/* compat */
	CPU_SUBTYPE_MC68040		= 2,
	CPU_SUBTYPE_MC68030_ONLY	= 3,
} cpu_subtype_mc680x0_t;

// CPU subtype for CPU_TYPE_I386.
typedef enum {
	CPU_SUBTYPE_I386_ALL	= 3,
	CPU_SUBTYPE_386		= 3,
	CPU_SUBTYPE_486		= 4,
	CPU_SUBTYPE_486SX	= 4+128,
	CPU_SUBTYPE_586		= 5,
#define CPU_SUBTYPE_INTEL(f, m) ((f) + ((m) << 4))
	CPU_SUBTYPE_PENT	= CPU_SUBTYPE_INTEL(5, 0),
	CPU_SUBTYPE_PENTPRO	= CPU_SUBTYPE_INTEL(6, 1),
	CPU_SUBTYPE_PENTII_M3	= CPU_SUBTYPE_INTEL(6, 3),
	CPU_SUBTYPE_PENTII_M5	= CPU_SUBTYPE_INTEL(6, 5),
} cpu_subtype_i386_t;

#define CPU_SUBTYPE_INTEL_FAMILY(x)	((x) & 15)
#define CPU_SUBTYPE_INTEL_FAMILY_MAX	15

#define CPU_SUBTYPE_INTEL_MODEL(x)	((x) >> 4)
#define CPU_SUBTYPE_INTEL_MODEL_ALL	0

// CPU threadtype for CPU_TPYE_I386.
typedef enum {
	CPU_THREADTYPE_INTEL_HTT = 1,
} cpu_threadtype_i386_t;

// CPU subtype for CPU_TYPE_MIPS.
typedef enum {
	CPU_SUBTYPE_MIPS_ALL	= 0,
	CPU_SUBTYPE_MIPS_R2300	= 1,
	CPU_SUBTYPE_MIPS_R2600	= 2,
	CPU_SUBTYPE_MIPS_R2800	= 3,
	CPU_SUBTYPE_MIPS_R2000a	= 4,	/* pmax */
	CPU_SUBTYPE_MIPS_R2000	= 5,
	CPU_SUBTYPE_MIPS_R3000a	= 6,	/* 3max */
	CPU_SUBTYPE_MIPS_R3000	= 7,
} cpu_subtype_mips_t;

// CPU subtype for CPU_TYPE_MC98000. (PowerPC)
typedef enum {
	CPU_SUBTYPE_MC98000_ALL	= 0,
	CPU_SUBTYPE_MC98601	= 1,
} cpu_subtype_mc98000_t;

// CPU subtype for CPU_TYPE_HPPA.
typedef enum {
	CPU_SUBTYPE_HPPA_ALL	= 0,
	CPU_SUBTYPE_HPPA_7100	= 0,	/* compat */
	CPU_SUBTYPE_HPPA_7100LC	= 1,
} cpu_subtype_hppa_t;

// CPU subtype for CPU_TYPE_MC88000.
typedef enum {
	CPU_SUBTYPE_MC88000_ALL	= 0,
	CPU_SUBTYPE_MC88100	= 1,
	CPU_SUBTYPE_MC88110	= 2,
} cpu_subtype_mc88000_t;

// CPU subtype for CPU_TYPE_SPARC.
typedef enum {
	CPU_SUBTYPE_SPARC_ALL	= 0,
} cpu_subtype_sparc_t;

// CPU subtype for CPU_TYPE_I860.
typedef enum {
	CPU_SUBTYPE_I860_ALL	= 0,
	CPU_SUBTYPE_I860_860	= 1,
} cpu_subtype_i860_t;

// CPU subtype for CPU_TYPE_POWERPC.
typedef enum {
	CPU_SUBTYPE_POWERPC_ALL		= 0,
	CPU_SUBTYPE_POWERPC_601		= 1,
	CPU_SUBTYPE_POWERPC_602		= 2,
	CPU_SUBTYPE_POWERPC_603		= 3,
	CPU_SUBTYPE_POWERPC_603e	= 4,
	CPU_SUBTYPE_POWERPC_603ev	= 5,
	CPU_SUBTYPE_POWERPC_604		= 6,
	CPU_SUBTYPE_POWERPC_604e	= 7,
	CPU_SUBTYPE_POWERPC_620		= 8,
	CPU_SUBTYPE_POWERPC_750		= 9,
	CPU_SUBTYPE_POWERPC_7400	= 10,
	CPU_SUBTYPE_POWERPC_7450	= 11,
	CPU_SUBTYPE_POWERPC_970		= 100,
} cpu_subtype_powerpc_t;

/**
 * Mach-O header.
 * These fields are identical for both 32-bit and 64-bit.
 * The magic number is slightly different, though.
 */
#define MH_MAGIC	0xFEEDFACE	/* 32-bit, host-endian */
#define MH_CIGAM	0xCEFAEDFE	/* 32-bit, byteswapped */
#define MH_MAGIC_64	0xFEEDFACF	/* 64-bit, host-endian */
#define MH_CIGAM_64	0xCFFAEDFE	/* 64-bit, byteswapped */
typedef struct PACKED _mach_header {
	uint32_t magic;		// [0x000] mach magic number identifier
	uint32_t cputype;	// [0x004] cpu specifier (see cpu_type_t)
	uint32_t cpusubtype;	// [0x008] machine specifier (see cpu_subtype_*_t)
	uint32_t filetype;	// [0x00C] type of file
	uint32_t ncmds;		// [0x010] number of load commands
	uint32_t sizeofcmds;	// [0x014] the size of all the load commands
	uint32_t flags;		// [0x018] flags
	//uint32_t reserved;	// [0x01C] reserved (64-bit only)
} mach_header;
ASSERT_STRUCT(mach_header, 28);

// Filetype field.
typedef enum {
	MH_OBJECT	= 0x1,	// relocatable object file
	MH_EXECUTE	= 0x2,	// demand paged executable file
	MH_FVMLIB	= 0x3,	// fixed VM shared library file
	MH_CORE		= 0x4,	// core file
	MH_PRELOAD	= 0x5,	// preloaded executable file
	MH_DYLIB	= 0x6,	// dynamically bound shared library file
	MH_DYLINKER	= 0x7,	// dynamic link editor
	MH_BUNDLE	= 0x8,	// dynamically bound bundle file
} mh_filetype_t;

// Flags field. (bitfield)
typedef enum {
	MH_NOUNDEFS	= 0x1,	// the object file has no undefined
				// references, can be executed
	MH_INCRLINK	= 0x2,	// the object file is the output of an
				// incremental link against a base file
				// and can't be link edited again
	MH_DYLDLINK	= 0x4,	// the object file is input for the
				// dynamic linker and can't be statically
				// link edited again
	MH_BINDATLOAD	= 0x8,	// the object file's undefined
				// references are bound by the dynamic
				// linker when loaded
	MH_PREBOUND	= 0x10,	// the file has its dynamic undefined
				// references prebound
} mh_flags_t;

#pragma pack()

#ifdef __cplusplus
}
#endif

#endif /* __ROMPROPERTIES_LIBROMDATA_OTHER_MACHO_STRUCTS_H__ */
