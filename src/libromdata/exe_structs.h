/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * exe_structs.h: DOS/Windows executable structures.                       *
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

// Based on w32api's winnt.h.
// References:
// - https://github.com/MaxKellermann/w32api/blob/440c229960e782831d01c6638661f1c40cadbeb5/include/winnt.h
// - https://github.com/MaxKellermann/w32api/blob/440c229960e782831d01c6638661f1c40cadbeb5/include/winver.h
// - http://www.brokenthorn.com/Resources/OSDevPE.html
// - https://msdn.microsoft.com/en-us/library/windows/desktop/ms648009(v=vs.85).aspx

#ifndef __ROMPROPERTIES_LIBROMDATA_EXE_STRUCTS_H__
#define __ROMPROPERTIES_LIBROMDATA_EXE_STRUCTS_H__

#include "common.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define IMAGE_DOS_SIGNATURE 0x5A4D
#define IMAGE_OS2_SIGNATURE 0x454E
#define IMAGE_OS2_SIGNATURE_LE 0x454C
#define IMAGE_VXD_SIGNATURE 0x454C
#define IMAGE_NT_SIGNATURE 0x00004550
#define IMAGE_NT_OPTIONAL_HDR32_MAGIC 0x10b
#define IMAGE_NT_OPTIONAL_HDR64_MAGIC 0x20b
#define IMAGE_ROM_OPTIONAL_HDR_MAGIC 0x107
#define IMAGE_SEPARATE_DEBUG_SIGNATURE 0x4944
#define IMAGE_NUMBEROF_DIRECTORY_ENTRIES 16
#define IMAGE_SIZEOF_ROM_OPTIONAL_HEADER 56
#define IMAGE_SIZEOF_STD_OPTIONAL_HEADER 28
#define IMAGE_SIZEOF_NT_OPTIONAL_HEADER 224
#define IMAGE_SIZEOF_SHORT_NAME 8
#define IMAGE_SIZEOF_SECTION_HEADER 40
#define IMAGE_SIZEOF_SYMBOL 18
#define IMAGE_SIZEOF_AUX_SYMBOL 18
#define IMAGE_SIZEOF_RELOCATION 10
#define IMAGE_SIZEOF_BASE_RELOCATION 8
#define IMAGE_SIZEOF_LINENUMBER 6
#define IMAGE_SIZEOF_ARCHIVE_MEMBER_HDR 60
#define SIZEOF_RFPO_DATA 16

#define IMAGE_SIZEOF_FILE_HEADER 20
typedef enum {
	IMAGE_FILE_RELOCS_STRIPPED		= 1,
	IMAGE_FILE_EXECUTABLE_IMAGE		= 2,
	IMAGE_FILE_LINE_NUMS_STRIPPED		= 4,
	IMAGE_FILE_LOCAL_SYMS_STRIPPED		= 8,
	IMAGE_FILE_AGGRESIVE_WS_TRIM 		= 16,
	IMAGE_FILE_LARGE_ADDRESS_AWARE		= 32,
	IMAGE_FILE_BYTES_REVERSED_LO		= 128,
	IMAGE_FILE_32BIT_MACHINE		= 256,
	IMAGE_FILE_DEBUG_STRIPPED		= 512,
	IMAGE_FILE_REMOVABLE_RUN_FROM_SWAP	= 1024,
	IMAGE_FILE_NET_RUN_FROM_SWAP		= 2048,
	IMAGE_FILE_SYSTEM			= 4096,
	IMAGE_FILE_DLL				= 8192,
	IMAGE_FILE_UP_SYSTEM_ONLY		= 16384,
	IMAGE_FILE_BYTES_REVERSED_HI		= 32768,
} PE_Characteristics;

typedef enum {
	// TODO: Update from `file`.
	IMAGE_FILE_MACHINE_UNKNOWN	= 0x0000,
	IMAGE_FILE_MACHINE_I386		= 0x014C, /* Intel 386 or later processors 
						     and compatible processors */
	IMAGE_FILE_MACHINE_R3000_BE	= 0x0160, /* MIPS big endian */
	IMAGE_FILE_MACHINE_R3000	= 0x0162, /* MIPS little endian */
	IMAGE_FILE_MACHINE_R4000	= 0x0166, /* MIPS little endian */
	IMAGE_FILE_MACHINE_R10000	= 0x0168, /* MIPS little endian */
	IMAGE_FILE_MACHINE_WCEMIPSV2	= 0x0169, /* MIPS little-endian WCE v2 */
	IMAGE_FILE_MACHINE_ALPHA	= 0x0184, /* Alpha AXP */
	IMAGE_FILE_MACHINE_SH3		= 0x01A2, /* Hitachi SH3 */
	IMAGE_FILE_MACHINE_SH3DSP	= 0x01A3, /* Hitachi SH3 DSP */
	IMAGE_FILE_MACHINE_SH3E		= 0x01A4, /* Hitachi SH3E */
	IMAGE_FILE_MACHINE_SH4		= 0x01A6, /* Hitachi SH4 */
	IMAGE_FILE_MACHINE_SH5		= 0x01A8, /* Hitachi SH5 */
	IMAGE_FILE_MACHINE_ARM		= 0x01C0, /* ARM little endian */
	IMAGE_FILE_MACHINE_THUMB	= 0x01C2, /* Thumb */
	IMAGE_FILE_MACHINE_ARMNT	= 0x01C4, /* Thumb-2 */
	IMAGE_FILE_MACHINE_AM33		= 0x01D3, /* Matsushita AM33 */
	IMAGE_FILE_MACHINE_POWERPC	= 0x01F0, /* Power PC little endian */
	IMAGE_FILE_MACHINE_POWERPCFP	= 0x01F1, /* Power PC with floating point support */
	IMAGE_FILE_MACHINE_IA64		= 0x0200, /* Intel Itanium processor family */
	IMAGE_FILE_MACHINE_MIPS16	= 0x0266, /* MIPS16 */
	IMAGE_FILE_MACHINE_ALPHA64	= 0x0284, /* Alpha AXP (64-bit) */
	IMAGE_FILE_MACHINE_MIPSFPU	= 0x0366, /* MIPS with FPU */
	IMAGE_FILE_MACHINE_MIPSFPU16	= 0x0466, /* MIPS16 with FPU */
	IMAGE_FILE_MACHINE_AXP64	= IMAGE_FILE_MACHINE_ALPHA64, /* Alpha AXP (64-bit) */
	IMAGE_FILE_MACHINE_TRICORE	= 0x0520, /* Infinieon */
	IMAGE_FILE_MACHINE_CEF		= 0x0CEF, /* Common Executable Format (Windows CE) */
	IMAGE_FILE_MACHINE_EBC		= 0x0EBC, /* EFI byte code */
	IMAGE_FILE_MACHINE_AMD64	= 0x8664, /* x64 */
	IMAGE_FILE_MACHINE_M32R		= 0x9041, /* Mitsubishi M32R little endian */
	IMAGE_FILE_MACHINE_ARM64	= 0xAA64, /* ARM64 little-endian */
	IMAGE_FILE_MACHINE_CEE		= 0xC0EE, /* MSIL (.NET) */
} PE_Machine;

typedef enum {
	IMAGE_SUBSYSTEM_UNKNOWN			= 0,
	IMAGE_SUBSYSTEM_NATIVE			= 1,
	IMAGE_SUBSYSTEM_WINDOWS_GUI		= 2,
	IMAGE_SUBSYSTEM_WINDOWS_CUI		= 3,
	IMAGE_SUBSYSTEM_OS2_CUI			= 5,  /* Not in PECOFF v8 spec */
	IMAGE_SUBSYSTEM_POSIX_CUI		= 7,
	IMAGE_SUBSYSTEM_NATIVE_WINDOWS		= 8,  /* Not in PECOFF v8 spec */
	IMAGE_SUBSYSTEM_WINDOWS_CE_GUI		= 9,
	IMAGE_SUBSYSTEM_EFI_APPLICATION		= 10,
	IMAGE_SUBSYSTEM_EFI_BOOT_SERVICE_DRIVER	= 11,
	IMAGE_SUBSYSTEM_EFI_RUNTIME_DRIVER	= 12,
	IMAGE_SUBSYSTEM_EFI_ROM			= 13,
	IMAGE_SUBSYSTEM_XBOX			= 14,
} PE_Subsystem;

typedef enum {
	IMAGE_DLLCHARACTERISTICS_HIGH_ENTROPY_VA	= 0x0020,	// Image can handle a high entropy 64-bit virtual address space.
	IMAGE_DLLCHARACTERISTICS_DYNAMIC_BASE		= 0x0040,	// DLL can move.
	IMAGE_DLLCHARACTERISTICS_FORCE_INTEGRITY	= 0x0080,	// Code Integrity Image
	IMAGE_DLLCHARACTERISTICS_NX_COMPAT		= 0x0100,	// Image is NX compatible.
	IMAGE_DLLCHARACTERISTICS_NO_ISOLATION		= 0x0200,	// Image understands isolation and doesn't want it.
	IMAGE_DLLCHARACTERISTICS_NO_SEH			= 0x0400,	// Image does not use SEH.  No SE handler may reside in this image.
	IMAGE_DLLCHARACTERISTICS_NO_BIND		= 0x0800,	// Do not bind this image.
	IMAGE_DLLCHARACTERISTICS_APPCONTAINER		= 0x1000,	// Image is an App(r).
	IMAGE_DLLCHARACTERISTICS_WDM_DRIVER		= 0x2000,	// Driver uses WDM model.
	IMAGE_DLLCHARACTERISTICS_GUARD_CF		= 0x4000,	// Image supports Control Flow Guard.
	IMAGE_DLLCHARACTERISTICS_TERMINAL_SERVER_AWARE	= 0x8000,
} PE_DLL_Characteristics;

/**
 * MS-DOS "MZ" header.
 * All other EXE-based headers have this one first.
 *
 * All fields are little-endian.
 */
#pragma pack(1)
typedef struct PACKED _IMAGE_DOS_HEADER {
	uint16_t e_magic;	// "MZ"
	uint16_t e_cblp;
	uint16_t e_cp;
	uint16_t e_crlc;
	uint16_t e_cparhdr;
	uint16_t e_minalloc;
	uint16_t e_maxalloc;
	uint16_t e_ss;
	uint16_t e_sp;
	uint16_t e_csum;
	uint16_t e_ip;
	uint16_t e_cs;
	uint16_t e_lfarlc;
	uint16_t e_ovno;
	uint16_t e_res[4];
	uint16_t e_oemid;
	uint16_t e_oeminfo;
	uint16_t e_res2[10];
	uint32_t e_lfanew;	// Pointer to NE/LE/LX/PE headers.
} IMAGE_DOS_HEADER;
#pragma pack()
ASSERT_STRUCT(IMAGE_DOS_HEADER, 64);

/**
 * Standard PE header.
 * All fields are little-endian.
 */
#pragma pack(1)
typedef struct PACKED _IMAGE_FILE_HEADER {
	uint16_t Machine;		// See PE_Machine.
	uint16_t NumberOfSections;
	uint32_t TimeDateStamp;		// UNIX timestamp.
	uint32_t PointerToSymbolTable;
	uint32_t NumberOfSymbols;
	uint16_t SizeOfOptionalHeader;
	uint16_t Characteristics;
} IMAGE_FILE_HEADER;
#pragma pack()
ASSERT_STRUCT(IMAGE_FILE_HEADER, IMAGE_SIZEOF_FILE_HEADER);

/**
 * PE image data directory indexes.
 * Reference: https://msdn.microsoft.com/en-us/library/windows/desktop/ms680305(v=vs.85).aspx
 */
#pragma pack(1)
typedef enum {
	IMAGE_DATA_DIRECTORY_EXPORT_TABLE		= 0,
	IMAGE_DATA_DIRECTORY_IMPORT_TABLE		= 1,
	IMAGE_DATA_DIRECTORY_RESOURCE_TABLE		= 2,
	IMAGE_DATA_DIRECTORY_EXCEPTION_TABLE		= 3,
	IMAGE_DATA_DIRECTORY_CERTIFICATE_TABLE		= 4,
	IMAGE_DATA_DIRECTORY_RELOCATION_TABLE		= 5,
	IMAGE_DATA_DIRECTORY_DEBUG_INFO			= 6,
	IMAGE_DATA_DIRECTORY_ARCH_SPECIFIC_DATA		= 7,
	IMAGE_DATA_DIRECTORY_GLOBAL_PTR_REG		= 8,
	IMAGE_DATA_DIRECTORY_TLS_TABLE			= 9,
	IMAGE_DATA_DIRECTORY_LOCAL_CFG_TABLE		= 10,
	IMAGE_DATA_DIRECTORY_BOUND_IMPORT_TABLE		= 11,
	IMAGE_DATA_DIRECTORY_IMPORT_ADDR_TABLE		= 12,
	IMAGE_DATA_DIRECTORY_DELAY_IMPORT_DESCRIPTOR	= 13,
	IMAGE_DATA_DIRECTORY_CLR_HEADER			= 14,
	IMAGE_DATA_DIRECTORY_RESERVED			= 15,
} ImageDataDirectoryIndex;

/**
 * PE image data directory.
 * All fields are little-endian.
 */
#pragma pack(1)
typedef struct PACKED _IMAGE_DATA_DIRECTORY {
	uint32_t VirtualAddress;
	uint32_t Size;
} IMAGE_DATA_DIRECTORY;
#pragma pack()
ASSERT_STRUCT(IMAGE_DATA_DIRECTORY, 8);

/**
 * "Optional" 32-bit PE header.
 * All fields are little-endian.
 */
#pragma pack(1)
typedef struct PACKED _IMAGE_OPTIONAL_HEADER32 {
	uint16_t Magic;
	uint8_t MajorLinkerVersion;
	uint8_t MinorLinkerVersion;
	uint32_t SizeOfCode;
	uint32_t SizeOfInitializedData;
	uint32_t SizeOfUninitializedData;
	uint32_t AddressOfEntryPoint;
	uint32_t BaseOfCode;
	uint32_t BaseOfData;
	uint32_t ImageBase;
	uint32_t SectionAlignment;
	uint32_t FileAlignment;
	uint16_t MajorOperatingSystemVersion;
	uint16_t MinorOperatingSystemVersion;
	uint16_t MajorImageVersion;
	uint16_t MinorImageVersion;
	uint16_t MajorSubsystemVersion;
	uint16_t MinorSubsystemVersion;
	uint32_t Win32VersionValue;
	uint32_t SizeOfImage;
	uint32_t SizeOfHeaders;
	uint32_t CheckSum;
	uint16_t Subsystem;		// See PE_Subsystem.
	uint16_t DllCharacteristics;
	uint32_t SizeOfStackReserve;
	uint32_t SizeOfStackCommit;
	uint32_t SizeOfHeapReserve;
	uint32_t SizeOfHeapCommit;
	uint32_t LoaderFlags;
	uint32_t NumberOfRvaAndSizes;
	IMAGE_DATA_DIRECTORY DataDirectory[IMAGE_NUMBEROF_DIRECTORY_ENTRIES];
} IMAGE_OPTIONAL_HEADER32;
#pragma pack()
ASSERT_STRUCT(IMAGE_OPTIONAL_HEADER32, 224);

/**
 * "Optional" 64-bit PE header.
 * All fields are little-endian.
 */
#pragma pack(1)
typedef struct PACKED _IMAGE_OPTIONAL_HEADER64 {
	uint16_t Magic;
	uint8_t MajorLinkerVersion;
	uint8_t MinorLinkerVersion;
	uint32_t SizeOfCode;
	uint32_t SizeOfInitializedData;
	uint32_t SizeOfUninitializedData;
	uint32_t AddressOfEntryPoint;
	uint32_t BaseOfCode;
	uint64_t ImageBase;
	uint32_t SectionAlignment;
	uint32_t FileAlignment;
	uint16_t MajorOperatingSystemVersion;
	uint16_t MinorOperatingSystemVersion;
	uint16_t MajorImageVersion;
	uint16_t MinorImageVersion;
	uint16_t MajorSubsystemVersion;
	uint16_t MinorSubsystemVersion;
	uint32_t Win32VersionValue;
	uint32_t SizeOfImage;
	uint32_t SizeOfHeaders;
	uint32_t CheckSum;
	uint16_t Subsystem;		// See PE_Subsystem.
	uint16_t DllCharacteristics;
	uint64_t SizeOfStackReserve;
	uint64_t SizeOfStackCommit;
	uint64_t SizeOfHeapReserve;
	uint64_t SizeOfHeapCommit;
	uint32_t LoaderFlags;
	uint32_t NumberOfRvaAndSizes;
	IMAGE_DATA_DIRECTORY DataDirectory[IMAGE_NUMBEROF_DIRECTORY_ENTRIES];
} IMAGE_OPTIONAL_HEADER64;
#pragma pack()
ASSERT_STRUCT(IMAGE_OPTIONAL_HEADER64, 240);

/**
 * 32-bit PE headers.
 * All fields are little-endian.
 */
#pragma pack(1)
typedef struct PACKED _IMAGE_NT_HEADERS32 {
	uint32_t Signature;
	IMAGE_FILE_HEADER FileHeader;
	IMAGE_OPTIONAL_HEADER32 OptionalHeader;
} IMAGE_NT_HEADERS32;
#pragma pack()
ASSERT_STRUCT(IMAGE_NT_HEADERS32, 248);

/**
 * 64-bit PE32+ headers.
 * All fields are little-endian.
 */
#pragma pack(1)
typedef struct PACKED _IMAGE_NT_HEADERS64 {
	uint32_t Signature;
	IMAGE_FILE_HEADER FileHeader;
	IMAGE_OPTIONAL_HEADER64 OptionalHeader;
} IMAGE_NT_HEADERS64;
#pragma pack()
ASSERT_STRUCT(IMAGE_NT_HEADERS64, 264);

/**
 * Section header.
 */
#pragma pack(1)
typedef struct _IMAGE_SECTION_HEADER {
	char Name[IMAGE_SIZEOF_SHORT_NAME];
	union {
		uint32_t PhysicalAddress;
		uint32_t VirtualSize;
	} Misc;
	uint32_t VirtualAddress;
	uint32_t SizeOfRawData;
	uint32_t PointerToRawData;
	uint32_t PointerToRelocations;
	uint32_t PointerToLinenumbers;
	uint16_t NumberOfRelocations;
	uint16_t NumberOfLinenumbers;
	uint32_t Characteristics;
} IMAGE_SECTION_HEADER;
#pragma pack()
ASSERT_STRUCT(IMAGE_SECTION_HEADER, IMAGE_SIZEOF_SECTION_HEADER);

/** Win32 resources. **/

// Resource types.
typedef enum {
	RT_CURSOR	= 1,
	RT_BITMAP	= 2,
	RT_ICON		= 3,
	RT_MENU		= 4,
	RT_DIALOG	= 5,
	RT_STRING	= 6,
	RT_FONTDIR	= 7,
	RT_FONT		= 8,
	RT_ACCELERATOR	= 9,
	RT_RCDATA	= 10,
	RT_MESSAGETABLE	= 11,
	RT_GROUP_CURSOR	= 12,	// (RT_CURSOR+11)
	RT_GROUP_ICON	= 14,	// (RT_ICON+11)
	RT_VERSION	= 16,
	RT_DLGINCLUDE	= 17,
	RT_PLUGPLAY	= 19,
	RT_VXD		= 20,
	RT_ANICURSOR	= 21,
	RT_ANIICON	= 22,
	RT_HTML		= 23,
	RT_MANIFEST	= 24,

	// MFC resources
	RT_DLGINIT	= 240,
	RT_TOOLBAR	= 241,
} ResourceType;

// Resource directory.
#pragma pack(1)
typedef struct PACKED _IMAGE_RESOURCE_DIRECTORY {
	uint32_t Characteristics;
	uint32_t TimeDateStamp;
	uint16_t MajorVersion;
	uint16_t MinorVersion;
	uint16_t NumberOfNamedEntries;
	uint16_t NumberOfIdEntries;
	// following this struct are named entries, then ID entries
} IMAGE_RESOURCE_DIRECTORY;
#pragma pack()

// Resource directory entry.
#pragma pack(1)
typedef struct PACKED _IMAGE_RESOURCE_DIRECTORY_ENTRY {
	// Name/ID field.
	// If bit 31 is set, this is an offset into the string table.
	// Otherwise, it's a 16-bit ID.
	uint32_t Name;

	// Offset to the resource data.
	// If bit 31 is set, this points to a subdirectory.
	// Otherwise, it's an actual resource.
	uint32_t OffsetToData;
} IMAGE_RESOURCE_DIRECTORY_ENTRY;
#pragma pack()

// Resource data entry.
#pragma pack(1)
typedef struct PACKED _IMAGE_RESOURCE_DATA_ENTRY {
	uint32_t OffsetToData;
	uint32_t Size;
	uint32_t CodePage;
	uint32_t Reserved;
} IMAGE_RESOURCE_DATA_ENTRY;
#pragma pack()

// Version flags.
//#define VS_FILE_INFO RT_VERSION	// TODO
#define VS_VERSION_INFO 1
#define VS_USER_DEFINED 100
#define VS_FFI_SIGNATURE 0xFEEF04BD
#define VS_FFI_STRUCVERSION 0x10000
#define VS_FFI_FILEFLAGSMASK 0x3F
typedef enum {
	VS_FF_DEBUG = 1,
	VS_FF_PRERELEASE = 2,
	VS_FF_PATCHED = 4,
	VS_FF_PRIVATEBUILD = 8,
	VS_FF_INFOINFERRED = 16,
	VS_FF_SPECIALBUILD = 32,
} VS_FileFlags;

typedef enum {
	VOS_UNKNOWN = 0,
	VOS_DOS = 0x10000,
	VOS_OS216 = 0x20000,
	VOS_OS232 = 0x30000,
	VOS_NT = 0x40000,
	VOS__BASE = 0,
	VOS__WINDOWS16 = 1,
	VOS__PM16 = 2,
	VOS__PM32 = 3,
	VOS__WINDOWS32 = 4,
	VOS_DOS_WINDOWS16 = 0x10001,
	VOS_DOS_WINDOWS32 = 0x10004,
	VOS_OS216_PM16 = 0x20002,
	VOS_OS232_PM32 = 0x30003,
	VOS_NT_WINDOWS32 = 0x40004,
} VS_OperatingSystem;

typedef enum {
	VFT_UNKNOWN = 0,
	VFT_APP = 1,
	VFT_DLL = 2,
	VFT_DRV = 3,
	VFT_FONT = 4,
	VFT_VXD = 5,
	VFT_STATIC_LIB = 7,
} VS_FileType;

typedef enum {
	VFT2_UNKNOWN = 0,
	VFT2_DRV_PRINTER = 1,
	VFT2_DRV_KEYBOARD = 2,
	VFT2_DRV_LANGUAGE = 3,
	VFT2_DRV_DISPLAY = 4,
	VFT2_DRV_MOUSE = 5,
	VFT2_DRV_NETWORK = 6,
	VFT2_DRV_SYSTEM = 7,
	VFT2_DRV_INSTALLABLE = 8,
	VFT2_DRV_SOUND = 9,
	VFT2_DRV_COMM = 10,
	VFT2_DRV_INPUTMETHOD = 11,
	VFT2_DRV_VERSIONED_PRINTER = 12,
	VFT2_FONT_RASTER = 1,
	VFT2_FONT_VECTOR = 2,
	VFT2_FONT_TRUETYPE = 3,
} VS_FileSubtype;

/**
 * Version info resource. (fixed-size data section)
 */
#pragma pack(1)
typedef struct PACKED _VS_FIXEDFILEINFO {
	uint32_t dwSignature;
	uint32_t dwStrucVersion;
	uint32_t dwFileVersionMS;
	uint32_t dwFileVersionLS;
	uint32_t dwProductVersionMS;
	uint32_t dwProductVersionLS;
	uint32_t dwFileFlagsMask;
	uint32_t dwFileFlags;
	uint32_t dwFileOS;
	uint32_t dwFileType;
	uint32_t dwFileSubtype;
	uint32_t dwFileDateMS;
	uint32_t dwFileDateLS;
} VS_FIXEDFILEINFO;
#pragma pack()
ASSERT_STRUCT(VS_FIXEDFILEINFO, 13*4);

/** New Executable (Win16) structs. **/
// References:
// - http://wiki.osdev.org/NE
// - http://www.fileformat.info/format/exe/corion-ne.htm

#pragma pack(1)
typedef struct PACKED _NE_Header {
	// 0x00
	uint16_t sig;			// "NE" (0x4E45)
	uint8_t MajLinkerVersion;	// The major linker version
	uint8_t MinLinkerVersion;	// The minor linker version
	uint16_t EntryTableOffset;	// Offset of entry table, see below
	uint16_t EntryTableLength;	// Length of entry table in bytes
	uint32_t FileLoadCRC;		// 32-bit CRC of entire contents of file
	uint8_t ProgFlags;		// Program flags, bitmapped
	uint8_t ApplFlags;		// Application flags, bitmapped
	uint8_t AutoDataSegIndex;	// The automatic data segment index
	uint8_t reserved;
	// 0x10
	uint16_t InitHeapSize;		// The intial local heap size
	uint16_t InitStackSize;		// The inital stack size
	uint32_t EntryPoint;		// CS:IP entry point, CS is index into segment table
	uint32_t InitStack;		// SS:SP inital stack pointer, SS is index into segment table
	uint16_t SegCount;		// Number of segments in segment table
	uint16_t ModRefs;		// Number of module references (DLLs)
	// 0x20
	uint16_t NoResNamesTabSiz;	// Size of non-resident names table, in bytes (Please clarify non-resident names table)
	uint16_t SegTableOffset;	// Offset of Segment table
	uint16_t ResTableOffset;	// Offset of resources table
	uint16_t ResidNamTable;		// Offset of resident names table
	uint16_t ModRefTable;		// Offset of module reference table
	uint16_t ImportNameTable;	// Offset of imported names table (array of counted strings, terminated with string of length 00h)
	uint32_t OffStartNonResTab;	// Offset from start of file to non-resident names table
	// 0x30
	uint16_t MovEntryCount;		// Count of moveable entry point listed in entry table
	uint16_t FileAlnSzShftCnt;	// File alignment size shift count (0=9(default 512 byte pages))
	uint16_t nResTabEntries;	// Number of resource table entries
	uint8_t targOS;			// Target OS
	uint8_t OS2EXEFlags;		// Other OS/2 flags
	uint16_t retThunkOffset;	// Offset to return thunks or start of gangload area - what is gangload?
	uint16_t segrefthunksoff;	// Offset to segment reference thunks or size of gangload area
	uint16_t mincodeswap;		// Minimum code swap area size
	uint8_t expctwinver[2];		// Expected windows version (minor first)
} NE_Header;
#pragma pack()
ASSERT_STRUCT(NE_Header, 64);

// Program flags (ProgFlags)

// DGroup type (bits 0-1)
typedef enum {
	NE_DGT_NONE = 0,	// None
	NE_DGT_SINSHARED,	// Single shared
	NE_DGT_MULTIPLE,	// Multiple
	NE_DGT_NULL,		// (null)
} NE_DGroupType;

typedef enum {
	NE_GLOBINIT	= (1 << 2),	// Global initialization
	NE_PMODEONLY	= (1 << 3),	// Protected mode only
	NE_INSTRUC86	= (1 << 4),	// 8086 instructions
	NE_INSTRU286	= (1 << 5),	// 80286 instructions
	NE_INSTRU386	= (1 << 6),	// 80386 instructions
	NE_INSTRUx87	= (1 << 7),	// 80x87 (FPU) instructions
} NE_ProgFlags;

// Application flags (ApplFlags)

// Application type (bits 0-1)
typedef enum {
	NE_APP_NONE = 0,
	NE_APP_FULLSCREEN,	// Fullscreen (not aware of Windows/P.M. API)
	NE_APP_WINPMCOMPAT,	// Compatible with Windows/P.M. API
	NE_APP_WINPMUSES,	// Uses Windows/P.M. API
} NE_AppType;

typedef enum {
	NE_OS2APP	= (1 << 3),	// OS/2 family application
	// bit 4 reserved?
	NE_IMAGEERROR	= (1 << 5),	// Errors in image/executable
	NE_ONCONFORM	= (1 << 6),	// Non-conforming program?
	NE_DLL		= (1 << 7),	// DLL or driver (SS:SP invalid, CS:IP -> Far INIT routine)
					// AX=HMODULE, returns AX==0 success, AX!=0 fail
} NE_ApplFlags;

// Target OS (targOS)
// Used for NE and LE.
// NOTE: Phar Lap is NE only.
typedef enum {
	NE_OS_UNKNOWN = 0,
	NE_OS_OS2,		// IBM OS/2
	NE_OS_WIN,		// Windows (16-bit)
	NE_OS_DOS4,		// European DOS 4.x
	NE_OS_WIN386,		// Windows for the 80386. (Win32s?) 32-bit code.
	NE_OS_BOSS,		// Borland Operating System Services

	NE_OS_PHARLAP_286_OS2 = 0x81,	// Phar Lap 286|DOS Extender, OS/2
	NE_OS_PHARLAP_286_WIN = 0x82,	// Phar Lap 286|DOS Extender, Windows
} NE_TargetOS;

// Other OS/2 flags.
typedef enum {
	NE_OS2_LFN	= (1 << 0),	// OS/2 Long File Names
	NE_OS2_PMODE	= (1 << 1),	// OS/2 2.x Protected Mode executable
	NE_OS2_PFONT	= (1 << 2),	// OS/2 2.x Proportional Fonts
	NE_OS2_GANGL	= (1 << 3),	// OS/2 Gangload area
} NE_OS2EXEFlags;

// 16-bit resource structs.

#pragma pack(1)
typedef struct PACKED _NE_NAMEINFO {
	uint16_t rnOffset;
	uint16_t rnLength;
	uint16_t rnFlags;
	uint16_t rnID;
	uint16_t rnHandle;
	uint16_t rnUsage;
} NE_NAMEINFO;
#pragma pack()

#pragma pack(1)
typedef struct PACKED _NE_TYPEINFO {
	uint16_t rtTypeID;
	uint16_t rtResourceCount;
	uint32_t rtReserved;
	// followed by NE_NAMEINFO[]
} NE_TYPEINFO;
#pragma pack()

/** Linear Executable structs. **/
// NOTE: The header format is the same for LE (Win16 drivers)
// and LX (32-bit OS/2 executables).
// References:
// - http://fileformats.archiveteam.org/wiki/Linear_Executable
// - http://faydoc.tripod.com/formats/exe-LE.htm
// - http://www.textfiles.com/programming/FORMATS/lxexe.txt

#pragma pack(1)
typedef struct PACKED _LE_Header {
	// 0x00
	uint16_t sig;		// 'LE' (0x4C45)
	uint8_t byte_order;	// 0 == little-endian; other == big-endian
	uint8_t word_order;	// 0 == little-endian; other == big-endian
	uint32_t format_level;	// Executable format level.
	uint16_t cpu_type;	// See LE_CPU_Type.
	uint16_t targOS;	// See NE_Target_OS.
	uint32_t module_version;
	// 0x10
	uint32_t module_type_flags;	// See LE_Module_Type_Flags.
	uint32_t module_page_count;	// Number of memory pages.
	uint32_t initial_cs_number;	// Initial object CS number.
	uint32_t initial_eip;		// Initial EIP.
	// 0x20
	uint32_t initial_ss_number;	// Initial object SS number.
	uint32_t initial_esp;		// Initial ESP.
	uint32_t page_size;
	uint32_t bytes_on_last_page;	// or page offset shift?
	// 0x30
	uint32_t fixup_section_size;
	uint32_t fixup_section_checksum;
	uint32_t loader_section_size;
	uint32_t loader_section_checksum;
	// 0x40
	uint32_t object_table_offset;
	uint32_t object_table_count;	// Number of entries in the object table.
	uint32_t object_page_map_offset;
	uint32_t object_iterate_data_map_offset;
	// 0x50
	uint32_t resource_table_offset;
	uint32_t resource_table_count;	// Number of entries in the resource table.
	uint32_t resident_names_table_offset;
	uint32_t entry_table_offset;
	// 0x60
	// TODO more
	uint8_t filler[0xA8-0x60];
} LE_Header;
#pragma pack()
ASSERT_STRUCT(LE_Header, 0xA8);

// CPU type.
typedef enum {
	// TODO add to 'file'
	LE_CPU_UNKNOWN		= 0x00,
	LE_CPU_80286		= 0x01,
	LE_CPU_80386		= 0x02,
	LE_CPU_80486		= 0x03,
	LE_CPU_80586		= 0x04,
	LE_CPU_i860_N10		= 0x20,	// i860 XR
	LE_CPU_i860_N11		= 0x21,	// i860 XP
	LE_CPU_MIPS_I		= 0x40,	// MIPS Mark I (R2000, R3000)
	LE_CPU_MIPS_II		= 0x41,	// MIPS Mark II (R6000)
	LE_CPU_MIPS_III		= 0x42,	// MIPS Mark III (R4000)
} LE_CPU_Type;

// Module type flags.
typedef enum {
	LE_DLL_INIT_GLOBAL		= (0 << 2),
	LE_DLL_INIT_PER_PROCESS		= (1 << 2),
	LE_DLL_INIT_MASK		= (1 << 2),

	LE_EXE_NO_INTERNAL_FIXUP	= (1 << 4),
	LE_EXE_NO_EXTERNAL_FIXUP	= (1 << 5),

	// Same sa NE_AppType.
	LE_WINDOW_TYPE_UNKNOWN		= (0 << 8),
	LE_WINDOW_TYPE_INCOMPATIBLE	= (1 << 8),
	LE_WINDOW_TYPE_COMPATIBLE	= (2 << 8),
	LE_WINDOW_TYPE_USES		= (3 << 8),
	LE_WINDOW_TYPE_MASK		= (3 << 8),

	LE_MODULE_NOT_LOADABLE		= (1 << 13),
	LE_MODULE_IS_DLL		= (1 << 15),
} LE_Module_Type_Flags;

#ifdef __cplusplus
}
#endif

#endif /* __ROMPROPERTIES_LIBROMDATA_EXE_STRUCTS_H__ */
