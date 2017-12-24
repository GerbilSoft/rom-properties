/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * elf_structs.h: Executable and Linkable Format structures.               *
 *                                                                         *
 * Copyright (c) 2017 by David Korth.                                      *
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

// References:
// - http://wiki.osdev.org/ELF
// - http://www.mcs.anl.gov/OpenAD/OpenADFortTkExtendedDox/elf_8h_source.html
// - https://github.com/file/file/blob/master/magic/Magdir/elf
// - http://www.sco.com/developers/gabi/latest/ch5.pheader.html

#ifndef __ROMPROPERTIES_LIBROMDATA_ELF_STRUCTS_H__
#define __ROMPROPERTIES_LIBROMDATA_ELF_STRUCTS_H__

#include "librpbase/common.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#pragma pack(1)

// Type for a 16-bit quantity.
typedef uint16_t Elf32_Half;
typedef uint16_t Elf64_Half;

// Types for signed and unsigned 32-bit quantities.
typedef uint32_t Elf32_Word;
typedef int32_t  Elf32_Sword;
typedef uint32_t Elf64_Word;
typedef int32_t  Elf64_Sword;

// Types for signed and unsigned 64-bit quantities.
typedef uint64_t Elf32_Xword;
typedef int64_t  Elf32_Sxword;
typedef uint64_t Elf64_Xword;
typedef int64_t  Elf64_Sxword;

// Type of addresses.
typedef uint32_t Elf32_Addr;
typedef uint64_t Elf64_Addr;

// Type of file offsets.
typedef uint32_t Elf32_Off;
typedef uint64_t Elf64_Off;

// Type for section indices, which are 16-bit quantities.
typedef uint16_t Elf32_Section;
typedef uint16_t Elf64_Section;

// Type of symbol indices.
typedef uint32_t Elf32_Symndx;
typedef uint64_t Elf64_Symndx;

/**
 * ELF primary header.
 * These fields are identical for both 32-bit and 64-bit.
 */
#define ELF_MAGIC "\177ELF"
typedef struct PACKED _Elf_PrimaryEhdr {
	char e_magic[4];	// [0x000] "\x7FELF"
	uint8_t e_class;	// [0x004] Bitness (see Elf_Bitness)
	uint8_t e_data;		// [0x005] Endianness (see Elf_Endianness)
	uint8_t e_elfversion;	// [0x006] ELF version
	uint8_t e_osabi;	// [0x007] OS ABI - usually 0 for System V (see Elf_OSABI)
	uint8_t padding[8];	// [0x008]

	Elf32_Half e_type;	// [0x010] Executable type (see Elf_Type)
	Elf32_Half e_machine;	// [0x012] Machine type (see Elf_Machine)
	Elf32_Word e_version;	// [0x014] Object file version
} Elf_PrimaryEhdr;
ASSERT_STRUCT(Elf_PrimaryEhdr, 24);

/**
 * Bitness.
 */
typedef enum {
	ELFCLASSNONE	= 0,	/* Invalid class */
	ELFCLASS32	= 1,	/* 32-bit objects */
	ELFCLASS64	= 2,	/* 64-bit objects */
	ELFCLASSNUM	= 3,
} Elf_Bitness;

/**
 * Endianness.
 */
typedef enum {
	ELFDATANONE	= 0,	/* Invalid data encoding */
	ELFDATA2LSB	= 1,	/* 2's complement, little endian */
	ELFDATA2MSB	= 2,	/* 2's complement, big endian */
	ELFDATANUM	= 3,
} Elf_Endianness;

/**
 * OS ABI.
 * This list isn't comprehensive; see ELFData.cpp for more.
 */
typedef enum {
	ELFOSABI_NONE		= 0,	/* UNIX System V ABI */
	ELFOSABI_SYSV		= 0,	/* Alias.  */
	ELFOSABI_HPUX		= 1,	/* HP-UX */
	ELFOSABI_NETBSD		= 2,	/* NetBSD.  */
	ELFOSABI_GNU		= 3,	/* Object uses GNU ELF extensions.  */
	ELFOSABI_LINUX		= ELFOSABI_GNU,	/* Compatibility alias.  */
	ELFOSABI_SOLARIS	= 6,	/* Sun Solaris.  */
	ELFOSABI_AIX		= 7,	/* IBM AIX.  */
	ELFOSABI_IRIX		= 8,	/* SGI Irix.  */
	ELFOSABI_FREEBSD	= 9,	/* FreeBSD.  */
	ELFOSABI_TRU64		= 10,	/* Compaq TRU64 UNIX.  */
	ELFOSABI_MODESTO	= 11,	/* Novell Modesto.  */
	ELFOSABI_OPENBSD	= 12,	/* OpenBSD.  */
	ELFOSABI_ARM_AEABI	= 64,	/* ARM EABI */
	ELFOSABI_ARM		= 97,	/* ARM */
	ELFOSABI_CAFEOS		= 202,	/* Nintendo Wii U */
	ELFOSABI_STANDALONE	= 255,	/* Standalone (embedded) application */
} Elf_OSABI;

/**
 * Executable type.
 */
typedef enum {
	ET_NONE		= 0,	/* No file type */
	ET_REL		= 1,	/* Relocatable file */
	ET_EXEC		= 2,	/* Executable file */
	ET_DYN		= 3,	/* Shared object file */
	ET_CORE		= 4,	/* Core file */
	ET_NUM		= 5,	/* Number of defined types */

	ET_LOOS		= 0xFE00,	/* OS-specific range start */
	ET_HIOS		= 0xFEFF,	/* OS-specific range end */
	ET_LOPROC	= 0xFF00,	/* Processor-specific range start */
	ET_HIPROC	= 0xFFFF,	/* Processor-specific range end */
} Elf_Type;

/**
 * Machine type.
 * This list isn't comprehensive; see ELFData.cpp for more.
 */
typedef enum {
	EM_NONE		= 0,	/* No machine */
	EM_M32		= 1,	/* AT&T WE 32100 */
	EM_SPARC	= 2,	/* SUN SPARC */
	EM_386		= 3,	/* Intel 80386 */
	EM_68K		= 4,	/* Motorola m68k family */
	EM_88K		= 5,	/* Motorola m88k family */
	EM_IAMCU	= 6,	/* Intel MCU */
	EM_860		= 7,	/* Intel 80860 */
	EM_MIPS		= 8,	/* MIPS R3000 big-endian */
	EM_S370		= 9,	/* IBM System/370 */
	EM_MIPS_RS3_LE	= 10,	/* MIPS R3000 little-endian */
				/* reserved 11-14 */
	EM_PARISC	= 15,	/* HPPA */
				/* reserved 16 */
	EM_VPP500	= 17,	/* Fujitsu VPP500 */
	EM_SPARC32PLUS	= 18,	/* Sun's "v8plus" */
	EM_960		= 19,	/* Intel 80960 */
	EM_PPC		= 20,	/* PowerPC */
	EM_PPC64	= 21,	/* PowerPC 64-bit */
	EM_S390		= 22,	/* IBM S390 */
	EM_SPU		= 23,	/* IBM SPU/SPC */
} Elf_Machine;

/**
 * ELF 32-bit header.
 * Contains Elf_PrimaryEhdr and fields for 32-bit executables.
 */
typedef struct PACKED _Elf32_Ehdr {
	// Primary header. (Same as Elf_PrimaryEhdr)
	char e_magic[4];	// [0x000] "\x7FELF"
	uint8_t e_class;	// [0x004] Bitness (see Elf_Bitness)
	uint8_t e_data;		// [0x005] Endianness (see Elf_Endianness)
	uint8_t e_elfversion;	// [0x006] ELF version
	uint8_t e_osabi;	// [0x007] OS ABI - usually 0 for System V (see Elf_OSABI)
	uint8_t padding[8];	// [0x008]

	Elf32_Half e_type;	// [0x010] Executable type (see Elf_Type)
	Elf32_Half e_machine;	// [0x012] Machine type (see Elf_Machine)
	Elf32_Word e_version;	// [0x014] Object file version

	// 32-bit header.
	Elf32_Addr e_entry;	// [0x018] Entry point virtual address
	Elf32_Off e_phoff;	// [0x01C] Program header table file offset
	Elf32_Off e_shoff;	// [0x020] Section header table file offset
	Elf32_Word e_flags;	// [0x024] Processor-specific flags
	Elf32_Half e_ehsize;	// [0x028] ELF header size in bytes
	Elf32_Half e_phentsize;	// [0x02A] Program header table entry size
	Elf32_Half e_phnum;	// [0x02C] Program header table entry count
	Elf32_Half e_shentsize;	// [0x02E] Section header table entry size
	Elf32_Half e_shnum;	// [0x030] Section header table entry count
	Elf32_Half e_shstrndx;	// [0x032] Section header string table index
} Elf32_Ehdr;
ASSERT_STRUCT(Elf32_Ehdr, 52);

/**
 * ELF 64-bit header.
 * Contains Elf_PrimaryEhdr and fields for 64-bit executables.
 */
typedef struct PACKED _Elf64_Ehdr {
	// Primary header. (Same as Elf_PrimaryEhdr)
	char e_magic[4];	// [0x000] "\x7FELF"
	uint8_t e_class;	// [0x004] Bitness (see Elf_Bitness)
	uint8_t e_data;		// [0x005] Endianness (see Elf_Endianness)
	uint8_t e_elfversion;	// [0x006] ELF version
	uint8_t e_osabi;	// [0x007] OS ABI - usually 0 for System V (see Elf_OSABI)
	uint8_t padding[8];	// [0x008]

	Elf64_Half e_type;	// [0x010] Executable type (see Elf_Type)
	Elf64_Half e_machine;	// [0x012] Machine type (see Elf_Machine)
	Elf64_Word e_version;	// [0x014] Object file version

	// 64-bit header.
	Elf64_Addr e_entry;	// [0x018] Entry point virtual address
	Elf64_Off e_phoff;	// [0x020] Program header table file offset
	Elf64_Off e_shoff;	// [0x028] Section header table file offset
	Elf64_Word e_flags;	// [0x030] Processor-specific flags
	Elf64_Half e_ehsize;	// [0x034] ELF header size in bytes
	Elf64_Half e_phentsize;	// [0x036] Program header table entry size
	Elf64_Half e_phnum;	// [0x038] Program header table entry count
	Elf64_Half e_shentsize;	// [0x03A] Section header table entry size
	Elf64_Half e_shnum;	// [0x03C] Section header table entry count
	Elf64_Half e_shstrndx;	// [0x03E] Section header string table index
} Elf64_Ehdr;
ASSERT_STRUCT(Elf64_Ehdr, 64);

/**
 * ELF 32-bit program header.
 */
typedef struct PACKED _Elf32_Phdr {
	Elf32_Word p_type;	// [0x000] Program header type (see Elf_Phdr_Type)
	Elf32_Off p_offset;	// [0x004] Offset of segment from the beginning of the file
	Elf32_Addr p_vaddr;	// [0x008] Virtual address
	Elf32_Addr p_paddr;	// [0x00C] Physical address
	Elf32_Word p_filesz;	// [0x010] Size of file image, in bytes
	Elf32_Word p_memsz;	// [0x014] Size of memory image, in bytes
	Elf32_Word p_flags;	// [0x018] Flags
	Elf32_Word p_align;	// [0x01C] Alignment value
} Elf32_Phdr;
ASSERT_STRUCT(Elf32_Phdr, 32);

/**
 * ELF 64-bit program header.
 */
typedef struct PACKED _Elf64_Phdr {
	Elf64_Word p_type;	// [0x000] Program header type (see Elf_Phdr_Type)
	Elf64_Word p_flags;	// [0x004] Flags
	Elf64_Off p_offset;	// [0x008] Offset of segment from the beginning of the file
	Elf64_Addr p_vaddr;	// [0x010] Virtual address
	Elf64_Addr p_paddr;	// [0x018] Physical address
	Elf64_Xword p_filesz;	// [0x020] Size of file image, in bytes
	Elf64_Xword p_memsz;	// [0x028] Size of memory image, in bytes
	Elf64_Xword p_align;	// [0x030] Alignment value
} Elf64_Phdr;
ASSERT_STRUCT(Elf64_Phdr, 56);

/**
 * ELF program header types.
 */
typedef enum {
	PT_NULL		= 0,
	PT_LOAD		= 1,
	PT_DYNAMIC	= 2,
	PT_INTERP	= 3,
	PT_NOTE		= 4,
	PT_SHLIB	= 5,
	PT_PHDR		= 6,
	PT_TLS		= 7,

	// OS-specific
	PT_LOOS		= 0x60000000,
	PT_HIOS		= 0x6FFFFFFF,
	// CPU-specific
	PT_LOPROC	= 0x70000000,
	PT_HIPROC	= 0x7FFFFFFF,
} Elf_Phdr_Type;

#pragma pack()

#ifdef __cplusplus
}
#endif

#endif /* __ROMPROPERTIES_LIBROMDATA_ELF_STRUCTS_H__ */
