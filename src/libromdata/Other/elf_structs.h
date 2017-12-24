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
	uint8_t e_osabi;	// [0x007] OS ABI - usually 0 for System V
	uint8_t padding[8];	// [0x008]

	Elf32_Half e_type;	// [0x010] Executable type (see Elf_Type)
	Elf32_Half e_machine;	// [0x012] Machine type
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
 * ELF 32-bit header.
 * Contains Elf_PrimaryEhdr and fields for 32-bit executables.
 */
typedef struct PACKED _Elf32_Ehdr {
	// Primary header. (Same as Elf_PrimaryEhdr)
	char e_magic[4];	// [0x000] "\x7FELF"
	uint8_t e_class;	// [0x004] Bitness (see Elf_Bitness)
	uint8_t e_data;		// [0x005] Endianness (see Elf_Endianness)
	uint8_t e_elfversion;	// [0x006] ELF version
	uint8_t e_osabi;	// [0x007] OS ABI - usually 0 for System V
	uint8_t padding[8];	// [0x008]

	Elf32_Half e_type;	// [0x010] Executable type (see Elf_Type)
	Elf32_Half e_machine;	// [0x012] Machine type
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
	uint8_t e_osabi;	// [0x007] OS ABI - usually 0 for System V
	uint8_t padding[8];	// [0x008]

	Elf64_Half e_type;	// [0x010] Executable type (see Elf_Type)
	Elf64_Half e_machine;	// [0x012] Machine type
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

#pragma pack()

#ifdef __cplusplus
}
#endif

#endif /* __ROMPROPERTIES_LIBROMDATA_ELF_STRUCTS_H__ */
