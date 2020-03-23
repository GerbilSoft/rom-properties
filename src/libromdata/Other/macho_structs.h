/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * macho_structs.h: Mach-O executable structures.                          *
 *                                                                         *
 * Copyright (c) 2019-2020 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

// References:
// - https://lowlevelbits.org/parsing-mach-o-files/
// - https://developer.apple.com/documentation/kernel/mach_header?language=objc
// - https://opensource.apple.com/source/xnu/xnu-792/EXTERNAL_HEADERS/mach-o/fat.h.auto.html
// - https://opensource.apple.com/source/xnu/xnu-792/EXTERNAL_HEADERS/mach-o/loader.h.auto.html
// - https://opensource.apple.com/source/xnu/xnu-792/osfmk/mach/machine.h.auto.html
// - https://github.com/file/file/blob/master/magic/Magdir/mach
// - https://github.com/aidansteele/osx-abi-macho-file-format-reference
// - https://opensource.apple.com/source/xnu/xnu-344/EXTERNAL_HEADERS/mach-o/fat.h.auto.html

#ifndef __ROMPROPERTIES_LIBROMDATA_OTHER_MACHO_STRUCTS_H__
#define __ROMPROPERTIES_LIBROMDATA_OTHER_MACHO_STRUCTS_H__

#include <stdint.h>
#include "common.h"

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

// CPU subtype for CPU_TYPE_I386. (32-bit)
typedef enum {
	CPU_SUBTYPE_I386_ALL	= 3,
	CPU_SUBTYPE_386		= 3,
	CPU_SUBTYPE_486		= 4,
	CPU_SUBTYPE_486SX	= 4+128,
	CPU_SUBTYPE_586		= 5,
#define CPU_SUBTYPE_INTEL(f, m) ((f) + ((m) << 4))
	CPU_SUBTYPE_PENT	= CPU_SUBTYPE_INTEL(5, 0),
	CPU_SUBTYPE_PENTPRO	= CPU_SUBTYPE_INTEL(6, 1),
	CPU_SUBTYPE_PENTII_M2	= CPU_SUBTYPE_INTEL(6, 2),	// from `file`
	CPU_SUBTYPE_PENTII_M3	= CPU_SUBTYPE_INTEL(6, 3),
	CPU_SUBTYPE_PENTII_M4	= CPU_SUBTYPE_INTEL(6, 4),	// from `file`
	CPU_SUBTYPE_PENTII_M5	= CPU_SUBTYPE_INTEL(6, 5),

	// from `file`
	CPU_SUBTYPE_CELERON		= CPU_SUBTYPE_INTEL(7, 0),
	CPU_SUBTYPE_CELERON_MOBILE	= CPU_SUBTYPE_INTEL(7, 7),

	// from `file`
	CPU_SUBTYPE_PENTIII		= CPU_SUBTYPE_INTEL(8, 0),
	CPU_SUBTYPE_PENTIII_M		= CPU_SUBTYPE_INTEL(8, 1),
	CPU_SUBTYPE_PENTIII_XEON	= CPU_SUBTYPE_INTEL(8, 2),

	// from `file`
	CPU_SUBTYPE_PENTIUM_M		= CPU_SUBTYPE_INTEL(9, 0),
	CPU_SUBTYPE_PENTIUM_4		= CPU_SUBTYPE_INTEL(10, 0),
	CPU_SUBTYPE_ITANIUM		= CPU_SUBTYPE_INTEL(11, 0),
	CPU_SUBTYPE_ITANIUM_2		= CPU_SUBTYPE_INTEL(11, 1),
	CPU_SUBTYPE_XEON		= CPU_SUBTYPE_INTEL(12, 0),
	CPU_SUBTYPE_XEON_MP		= CPU_SUBTYPE_INTEL(12, 1),
} cpu_subtype_i386_t;

// CPU subtype for CPU_TYPE_I386. (32-bit)
typedef enum {
	CPU_SUBTYPE_AMD64_ARCH1		= 4,
	CPU_SUBTYPE_AMD64_HASWELL	= 8,
} cpu_subtype_amd64_t;

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

// CPU subtype for CPU_TYPE_ARM.
typedef enum {
	CPU_SUBTYPE_ARM_V4T	= 5,
	CPU_SUBTYPE_ARM_V6	= 6,
	CPU_SUBTYPE_ARM_V5TEJ	= 7,
	CPU_SUBTYPE_ARM_XSCALE	= 8,
	CPU_SUBTYPE_ARM_V7	= 9,
	CPU_SUBTYPE_ARM_V7F	= 10,
	CPU_SUBTYPE_ARM_V7S	= 11,
	CPU_SUBTYPE_ARM_V7K	= 12,
	CPU_SUBTYPE_ARM_V8	= 13,
	CPU_SUBTYPE_ARM_V6M	= 14,
	CPU_SUBTYPE_ARM_V7M	= 15,
	CPU_SUBTYPE_ARM_V7EM	= 16,
} cpu_subtype_arm;

// CPU subtype for CPU_TYPE_ARM. (64-bit)
typedef enum {
	CPU_SUBTYPE_ARM64_V8	= 1,
} cpu_subtype_arm64;

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
	MH_DYLIB_STUB	= 0x9,  // shared library stub for static
				//  linking only, no section contents
	MH_DSYM		= 0xa,	// companion file with only debug
				//   sections
	MH_KEXT_BUNDLE	= 0xb,	// x86_64 kexts
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

	// Flags from `file`'s magic listing.
	MH_SPLIT_SEGS			= 0x20,
	MH_LAZY_INIT			= 0x40,
	MH_TWOLEVEL			= 0x80,
	MH_FORCE_FLAT			= 0x100,
	MH_NOMULTIDEFS			= 0x200,
	MH_NOFIXPREBINDING		= 0x400,
	MH_PREBINDABLE			= 0x800,
	MH_ALLMODSBOUND			= 0x1000,
	MH_SUBSECTIONS_VIA_SYMBOLS	= 0x2000,
	MH_CANONICAL			= 0x4000,
	MH_WEAK_DEFINES			= 0x8000,
	MH_BINDS_TO_WEAK		= 0x10000,
	MH_ALLOW_STACK_EXECUTION	= 0x20000,
	MH_ROOT_SAFE			= 0x40000,
	MH_SETUID_SAFE			= 0x80000,
	MH_NO_REEXPORTED_DYLIBS		= 0x100000,
	MH_PIE				= 0x200000,
	MH_DEAD_STRIPPABLE_DYLIB	= 0x400000,
	MH_HAS_TLV_DESCRIPTORS		= 0x800000,
	MH_NO_HEAP_EXECUTION		= 0x1000000,
	MH_APP_EXTENSION_SAFE		= 0x2000000,
} mh_flags_t;

/**
 * Fat header for Universal Binaries.
 * NOTE: Universal Binary header is *always* in big-endian.
 */
#define FAT_MAGIC	0xCAFEBABE

typedef struct PACKED _fat_header {
	uint32_t magic;		/* FAT_MAGIC */
	uint32_t nfat_arch;	/* number of structs that follow */
} fat_header;

typedef struct PACKED _fat_arch {
	uint32_t cputype;	/* cpu specifier (int) */
	uint32_t cpusubtype;	/* machine specifier (int) */
	uint32_t offset;	/* file offset to this object file */
	uint32_t size;		/* size of this object file */
	uint32_t align;		/* alignment as a power of 2 */
} fat_arch;

#pragma pack()

#ifdef __cplusplus
}
#endif

#endif /* __ROMPROPERTIES_LIBROMDATA_OTHER_MACHO_STRUCTS_H__ */
