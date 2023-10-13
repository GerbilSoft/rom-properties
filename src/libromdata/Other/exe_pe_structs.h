/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * exe_pe_structs.h: DOS/Windows executable structures. (PE)               *
 *                                                                         *
 * Copyright (c) 2017-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include <stdint.h>
#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

// Based on w32api's winnt.h.
// References:
// - https://github.com/MaxKellermann/w32api/blob/440c229960e782831d01c6638661f1c40cadbeb5/include/winnt.h
// - https://github.com/MaxKellermann/w32api/blob/440c229960e782831d01c6638661f1c40cadbeb5/include/winver.h
// - http://www.brokenthorn.com/Resources/OSDevPE.html
// - https://docs.microsoft.com/en-us/windows/win32/menurc/resource-types
// - http://sandsprite.com/CodeStuff/Understanding_imports.html
// - https://docs.microsoft.com/en-us/windows/win32/debug/pe-format

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
	IMAGE_FILE_RELOCS_STRIPPED		=   0x01,
	IMAGE_FILE_EXECUTABLE_IMAGE		=   0x02,
	IMAGE_FILE_LINE_NUMS_STRIPPED		=   0x04,
	IMAGE_FILE_LOCAL_SYMS_STRIPPED		=   0x08,
	IMAGE_FILE_AGGRESIVE_WS_TRIM 		=   0x10,
	IMAGE_FILE_LARGE_ADDRESS_AWARE		=   0x20,
	IMAGE_FILE_BYTES_REVERSED_LO		=   0x80,
	IMAGE_FILE_32BIT_MACHINE		=  0x100,
	IMAGE_FILE_DEBUG_STRIPPED		=  0x200,
	IMAGE_FILE_REMOVABLE_RUN_FROM_SWAP	=  0x400,
	IMAGE_FILE_NET_RUN_FROM_SWAP		=  0x800,
	IMAGE_FILE_SYSTEM			= 0x1000,
	IMAGE_FILE_DLL				= 0x2000,
	IMAGE_FILE_UP_SYSTEM_ONLY		= 0x4000,
	IMAGE_FILE_BYTES_REVERSED_HI		= 0x8000,
} PE_Characteristics;

typedef enum {
	IMAGE_FILE_MACHINE_UNKNOWN	= 0x0000,
	IMAGE_FILE_MACHINE_I386		= 0x014C, /* Intel 386 or later processors 
						     and compatible processors */
	IMAGE_FILE_MACHINE_R3000_BE	= 0x0160, /* MIPS big-endian */
	IMAGE_FILE_MACHINE_R3000	= 0x0162, /* MIPS little-endian */
	IMAGE_FILE_MACHINE_R4000	= 0x0166, /* MIPS little-endian */
	IMAGE_FILE_MACHINE_R10000	= 0x0168, /* MIPS little-endian */
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
	IMAGE_FILE_MACHINE_POWERPC	= 0x01F0, /* PowerPC little-endian */
	IMAGE_FILE_MACHINE_POWERPCFP	= 0x01F1, /* PowerPC with floating point support */
	IMAGE_FILE_MACHINE_POWERPCBE	= 0x01F2, /* PowerPC big-endian (Xbox 360) */
	IMAGE_FILE_MACHINE_IA64		= 0x0200, /* Intel Itanium processor family */
	IMAGE_FILE_MACHINE_MIPS16	= 0x0266, /* MIPS16 */
	IMAGE_FILE_MACHINE_M68K		= 0x0268, /* Motorola 68000 */
	IMAGE_FILE_MACHINE_ALPHA64	= 0x0284, /* Alpha AXP (64-bit) */
	IMAGE_FILE_MACHINE_PA_RISC	= 0x0290, /* PA-RISC */
	IMAGE_FILE_MACHINE_MIPSFPU	= 0x0366, /* MIPS with FPU */
	IMAGE_FILE_MACHINE_MIPSFPU16	= 0x0466, /* MIPS16 with FPU */
	IMAGE_FILE_MACHINE_AXP64	= IMAGE_FILE_MACHINE_ALPHA64, /* Alpha AXP (64-bit) */
	IMAGE_FILE_MACHINE_TRICORE	= 0x0520, /* Infineon TriCore */
	IMAGE_FILE_MACHINE_MPPC_601	= 0x0601, /* PowerPC big-endian (MSVC for Mac) */
	IMAGE_FILE_MACHINE_CEF		= 0x0CEF, /* Common Executable Format (Windows CE) */
	IMAGE_FILE_MACHINE_EBC		= 0x0EBC, /* EFI byte code */
	IMAGE_FILE_MACHINE_RISCV32	= 0x5032, /* RISC-V 32-bit address space */
	IMAGE_FILE_MACHINE_RISCV64	= 0x5064, /* RISC-V 64-bit address space */
	IMAGE_FILE_MACHINE_RISCV128	= 0x5128, /* RISC-V 128-bit address space */
	IMAGE_FILE_MACHINE_LOONGARCH32	= 0x6232, /* LoongArch 32-bit */
	IMAGE_FILE_MACHINE_LOONGARCH64	= 0x6264, /* LoongArch 64-bit */
	IMAGE_FILE_MACHINE_AMD64	= 0x8664, /* x64 */
	IMAGE_FILE_MACHINE_M32R		= 0x9041, /* Mitsubishi M32R little endian */
	IMAGE_FILE_MACHINE_ARM64EC	= 0xA641, /* ARM64 ("emulation-compatible") */
	IMAGE_FILE_MACHINE_ARM64	= 0xAA64, /* ARM64 little-endian */
	IMAGE_FILE_MACHINE_CEE		= 0xC0EE, /* MSIL (.NET) */
} PE_Machine;

typedef enum {
	IMAGE_SUBSYSTEM_UNKNOWN				= 0,
	IMAGE_SUBSYSTEM_NATIVE				= 1,
	IMAGE_SUBSYSTEM_WINDOWS_GUI			= 2,
	IMAGE_SUBSYSTEM_WINDOWS_CUI			= 3,
	IMAGE_SUBSYSTEM_OS2_CUI				= 5,  /* Not in PECOFF v8 spec */
	IMAGE_SUBSYSTEM_POSIX_CUI			= 7,
	IMAGE_SUBSYSTEM_NATIVE_WINDOWS			= 8,  /* Not in PECOFF v8 spec */
	IMAGE_SUBSYSTEM_WINDOWS_CE_GUI			= 9,
	IMAGE_SUBSYSTEM_EFI_APPLICATION			= 10,
	IMAGE_SUBSYSTEM_EFI_BOOT_SERVICE_DRIVER		= 11,
	IMAGE_SUBSYSTEM_EFI_RUNTIME_DRIVER		= 12,
	IMAGE_SUBSYSTEM_EFI_ROM				= 13,
	IMAGE_SUBSYSTEM_XBOX				= 14,
	IMAGE_SUBSYSTEM_WINDOWS_BOOT_APPLICATION	= 16,
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
 * Standard PE header.
 * All fields are little-endian.
 */
typedef struct _IMAGE_FILE_HEADER {
	uint16_t Machine;		// See PE_Machine
	uint16_t NumberOfSections;
	uint32_t TimeDateStamp;		// UNIX timestamp
	uint32_t PointerToSymbolTable;
	uint32_t NumberOfSymbols;
	uint16_t SizeOfOptionalHeader;
	uint16_t Characteristics;	// See PE_Characteristics
} IMAGE_FILE_HEADER;
ASSERT_STRUCT(IMAGE_FILE_HEADER, IMAGE_SIZEOF_FILE_HEADER);

/**
 * PE image data directory indexes.
 * Reference: https://docs.microsoft.com/en-us/windows/win32/api/winnt/ns-winnt-image_data_directory
 */
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
	IMAGE_DATA_DIRECTORY_LOAD_CONFIG_TABLE		= 10,
	IMAGE_DATA_DIRECTORY_BOUND_IMPORT_TABLE		= 11,
	IMAGE_DATA_DIRECTORY_IMPORT_ADDR_TABLE		= 12,
	IMAGE_DATA_DIRECTORY_DELAY_IMPORT_DESCRIPTOR	= 13,
	IMAGE_DATA_DIRECTORY_CLR_HEADER			= 14,
	IMAGE_DATA_DIRECTORY_RESERVED			= 15,
} ImageDataDirectoryIndex;

/**
 * PE image data directory
 * All fields are little-endian.
 */
typedef struct _IMAGE_DATA_DIRECTORY {
	uint32_t VirtualAddress;
	uint32_t Size;
} IMAGE_DATA_DIRECTORY;
ASSERT_STRUCT(IMAGE_DATA_DIRECTORY, 2*sizeof(uint32_t));

/**
 * "Optional" 32-bit PE header
 * All fields are little-endian.
 */
typedef struct _IMAGE_OPTIONAL_HEADER32 {
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
ASSERT_STRUCT(IMAGE_OPTIONAL_HEADER32, 224);

/**
 * "Optional" 64-bit PE header
 * All fields are little-endian.
 */
typedef struct _IMAGE_OPTIONAL_HEADER64 {
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
ASSERT_STRUCT(IMAGE_OPTIONAL_HEADER64, 240);

/**
 * Load Config: Code integrity
 * TODO: Implement this.
 */
typedef struct _IMAGE_LOAD_CONFIG_CODE_INTEGRITY {
	uint8_t code_integrity[12];
} IMAGE_LOAD_CONFIG_CODE_INTEGRITY;
ASSERT_STRUCT(IMAGE_LOAD_CONFIG_CODE_INTEGRITY, 12);

/**
 * Load Config table (32-bit)
 * All fields are little-endian.
 * Reference: https://learn.microsoft.com/en-us/windows/win32/api/winnt/ns-winnt-image_load_config_directory32
 */
typedef struct _IMAGE_LOAD_CONFIG_DIRECTORY32 {
	uint32_t Size;
	uint32_t TimeDateStamp;
	uint16_t MajorVersion;
	uint16_t MinorVersion;
	uint32_t GlobalFlagsClear;
	uint32_t GlobalFlagsSet;
	uint32_t CriticalSectionDefaultTimeout;
	uint32_t DeCommitFreeBlockThreshold;
	uint32_t DeCommitTotalFreeThreshold;
	uint32_t LockPrefixTable;
	uint32_t MaximumAllocationSize;
	uint32_t VirtualMemoryThreshold;
	uint32_t ProcessHeapFlags;
	uint32_t ProcessAffinityMask;
	uint16_t CSDVersion;
	uint16_t DependentLoadFlags;
	uint32_t EditList;
	uint32_t SecurityCookie;
	uint32_t SEHandlerTable;
	uint32_t SEHandlerCount;
	// For Windows XP, this struct ends here.

	uint32_t GuardCFCheckFunctionPointer;
	uint32_t GuardCFDispatchFunctionPointer;
	uint32_t GuardCFFunctionTable;
	uint32_t GuardCFFunctionCount;
	uint32_t GuardFlags;
	IMAGE_LOAD_CONFIG_CODE_INTEGRITY CodeIntegrity;
	uint32_t GuardAddressTakenIatEntryTable;
	uint32_t GuardAddressTakenIatEntryCount;
	uint32_t GuardLongJumpTargetTable;
	uint32_t GuardLongJumpTargetCount;
	uint32_t DynamicValueRelocTable;
	uint32_t CHPEMetadataPointer;
	uint32_t GuardRFFailureRoutine;
	uint32_t GuardRFFailureRoutineFunctionPointer;
	uint32_t DynamicValueRelocTableOffset;
	uint16_t DynamicValueRelocTableSection;
	uint16_t Reserved2;
	uint32_t GuardRFVerifyStackPointerFunctionPointer;
	uint32_t HotPatchTableOffset;
	uint32_t Reserved3;
	uint32_t EnclaveConfigurationPointer;
	uint32_t VolatileMetadataPointer;
	uint32_t GuardEHContinuationTable;
	uint32_t GuardEHContinuationCount;
	uint32_t GuardXFGCheckFunctionPointer;
	uint32_t GuardXFGDispatchFunctionPointer;
	uint32_t GuardXFGTableDispatchFunctionPointer;
	uint32_t CastGuardOsDeterminedFailureMode;
	uint32_t GuardMemcpyFunctionPointer;
	// End of main struct for i386
} IMAGE_LOAD_CONFIG_DIRECTORY32;
ASSERT_STRUCT(IMAGE_LOAD_CONFIG_DIRECTORY32, 192);

/**
 * Load Config table (64-bit)
 * All fields are little-endian.
 * Reference: https://learn.microsoft.com/en-us/windows/win32/api/winnt/ns-winnt-image_load_config_directory64
 */
typedef struct _IMAGE_LOAD_CONFIG_DIRECTORY64 {
	uint32_t Size;
	uint32_t TimeDateStamp;
	uint16_t MajorVersion;
	uint16_t MinorVersion;
	uint32_t GlobalFlagsClear;
	uint32_t GlobalFlagsSet;
	uint32_t CriticalSectionDefaultTimeout;
	uint64_t DeCommitFreeBlockThreshold;
	uint64_t DeCommitTotalFreeThreshold;
	uint64_t LockPrefixTable;
	uint64_t MaximumAllocationSize;
	uint64_t VirtualMemoryThreshold;
	uint64_t ProcessAffinityMask;
	uint32_t ProcessHeapFlags;
	uint16_t CSDVersion;
	uint16_t DependentLoadFlags;
	uint64_t EditList;
	uint64_t SecurityCookie;
	uint64_t SEHandlerTable;
	uint64_t SEHandlerCount;
	// For Windows XP, this struct ends here.

	uint64_t GuardCFCheckFunctionPointer;
	uint64_t GuardCFDispatchFunctionPointer;
	uint64_t GuardCFFunctionTable;
	uint64_t GuardCFFunctionCount;
	uint32_t GuardFlags;
	IMAGE_LOAD_CONFIG_CODE_INTEGRITY CodeIntegrity;
	uint64_t GuardAddressTakenIatEntryTable;
	uint64_t GuardAddressTakenIatEntryCount;
	uint64_t GuardLongJumpTargetTable;
	uint64_t GuardLongJumpTargetCount;
	uint64_t DynamicValueRelocTable;
	uint64_t CHPEMetadataPointer;
	uint64_t GuardRFFailureRoutine;
	uint64_t GuardRFFailureRoutineFunctionPointer;
	uint32_t DynamicValueRelocTableOffset;
	uint16_t DynamicValueRelocTableSection;
	uint16_t Reserved2;
	uint64_t GuardRFVerifyStackPointerFunctionPointer;
	uint32_t HotPatchTableOffset;
	uint32_t Reserved3;
	uint64_t EnclaveConfigurationPointer;
	uint64_t VolatileMetadataPointer;
	uint64_t GuardEHContinuationTable;
	uint64_t GuardEHContinuationCount;
	uint64_t GuardXFGCheckFunctionPointer;
	uint64_t GuardXFGDispatchFunctionPointer;
	uint64_t GuardXFGTableDispatchFunctionPointer;
	uint64_t CastGuardOsDeterminedFailureMode;
	uint64_t GuardMemcpyFunctionPointer;
	// End of main struct for amd64
} IMAGE_LOAD_CONFIG_DIRECTORY64;
ASSERT_STRUCT(IMAGE_LOAD_CONFIG_DIRECTORY64, 320);

/**
 * 32-bit PE headers
 * All fields are little-endian.
 */
typedef struct _IMAGE_NT_HEADERS32 {
	uint32_t Signature;
	IMAGE_FILE_HEADER FileHeader;
	IMAGE_OPTIONAL_HEADER32 OptionalHeader;
} IMAGE_NT_HEADERS32;
ASSERT_STRUCT(IMAGE_NT_HEADERS32, 248);

/**
 * 64-bit PE32+ headers
 * All fields are little-endian.
 */
typedef struct _IMAGE_NT_HEADERS64 {
	uint32_t Signature;
	IMAGE_FILE_HEADER FileHeader;
	IMAGE_OPTIONAL_HEADER64 OptionalHeader;
} IMAGE_NT_HEADERS64;
ASSERT_STRUCT(IMAGE_NT_HEADERS64, 264);

/**
 * Section header.
 */
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
ASSERT_STRUCT(IMAGE_SECTION_HEADER, IMAGE_SIZEOF_SECTION_HEADER);

/**
 * Export directory.
 * Reference: https://docs.microsoft.com/en-us/windows/win32/debug/pe-format#export-directory-table
 */
typedef struct _IMAGE_EXPORT_DIRECTORY {
	uint32_t Characteristics;	// Reserved
	uint32_t TimeDateStamp;		// UNIX timestamp
	uint16_t MajorVersion;
	uint16_t MinorVersion;
	uint32_t Name;			// DLL name
	uint32_t Base;			// The starting ordinal number
	uint32_t NumberOfFunctions;	// Size of address table
	uint32_t NumberOfNames;		// Size of name table
	uint32_t AddressOfFunctions;	// RVA of address table
	uint32_t AddressOfNames;	// RVA of name table
	uint32_t AddressOfNameOrdinals;	// RVA of name-to-ordinal table
} IMAGE_EXPORT_DIRECTORY;
ASSERT_STRUCT(IMAGE_EXPORT_DIRECTORY, 10*sizeof(uint32_t));

/** Import table. **/
// Reference: http://sandsprite.com/CodeStuff/Understanding_imports.html

/**
 * Import directory.
 */
typedef struct _IMAGE_IMPORT_DIRECTORY {
	uint32_t rvaImportLookupTable;	// RVA of the import lookup table
	uint32_t TimeDateStamp;		// UNIX timestamp
	uint32_t ForwarderChain;
	uint32_t rvaModuleName;		// RVA of the DLL filename
	uint32_t rvaImportAddressTable;	// RVA of the thunk table
} IMAGE_IMPORT_DIRECTORY;
ASSERT_STRUCT(IMAGE_IMPORT_DIRECTORY, 5*sizeof(uint32_t));

// Import lookup table consists of 32-bit values which are either
// RVAs to the function name or, if the low(?) bit is set, an ordinal.

/** Win32 resources **/

// Resource directory
typedef struct _IMAGE_RESOURCE_DIRECTORY {
	uint32_t Characteristics;
	uint32_t TimeDateStamp;
	uint16_t MajorVersion;
	uint16_t MinorVersion;
	uint16_t NumberOfNamedEntries;
	uint16_t NumberOfIdEntries;
	// following this struct are named entries, then ID entries
} IMAGE_RESOURCE_DIRECTORY;
ASSERT_STRUCT(IMAGE_RESOURCE_DIRECTORY, 16);

// Resource directory entry
typedef struct _IMAGE_RESOURCE_DIRECTORY_ENTRY {
	// Name/ID field.
	// If bit 31 is set, this is an offset into the string table.
	// Otherwise, it's a 16-bit ID.
	uint32_t Name;

	// Offset to the resource data.
	// If bit 31 is set, this points to a subdirectory.
	// Otherwise, it's an actual resource.
	uint32_t OffsetToData;
} IMAGE_RESOURCE_DIRECTORY_ENTRY;
ASSERT_STRUCT(IMAGE_RESOURCE_DIRECTORY_ENTRY, 2*sizeof(uint32_t));
// Resource data entry.
typedef struct _IMAGE_RESOURCE_DATA_ENTRY {
	uint32_t OffsetToData;
	uint32_t Size;
	uint32_t CodePage;
	uint32_t Reserved;
} IMAGE_RESOURCE_DATA_ENTRY;
ASSERT_STRUCT(IMAGE_RESOURCE_DATA_ENTRY, 4*sizeof(uint32_t));

// Manifest IDs
typedef enum {
	CREATEPROCESS_MANIFEST_RESOURCE_ID = 1,
	ISOLATIONAWARE_MANIFEST_RESOURCE_ID = 2,
	ISOLATIONAWARE_NOSTATICIMPORT_MANIFEST_RESOURCE_ID = 3,

	// Windows XP's explorer.exe uses resource ID 123.
	// Reference: https://docs.microsoft.com/en-us/windows/win32/controls/cookbook-overview
	XP_VISUAL_STYLE_MANIFEST_RESOURCE_ID = 123,
} PE_ManifestID;

#ifdef __cplusplus
}
#endif
