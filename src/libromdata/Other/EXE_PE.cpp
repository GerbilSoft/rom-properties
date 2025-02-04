/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * EXE_PE.cpp: DOS/Windows executable reader.                              *
 * 32-bit/64-bit Portable Executable format.                               *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * Copyright (c) 2022 by Egor.                                             *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "EXE_p.hpp"
#include "data/EXEData.hpp"
#include "disc/PEResourceReader.hpp"

// Other rom-properties libraries
using namespace LibRpBase;
using namespace LibRpText;

// for strnlen() if it's not available in <string.h>
#include "librptext/libc.h"

// C++ STL classes
using std::array;
using std::string;
using std::unique_ptr;
using std::vector;

namespace LibRomData {

/** EXEPrivate **/

/**
 * Load the PE section table.
 *
 * NOTE: If the table was read successfully, but no section
 * headers were found, -ENOENT is returned.
 *
 * @return 0 on success; negative POSIX error code on error.
 */
int EXEPrivate::loadPESectionTable(void)
{
	if (!pe_sections.empty()) {
		// Section table is already loaded.
		return 0;
	} else if (!file || !file->isOpen()) {
		// File isn't open.
		return -EBADF;
	} else if (!isValid) {
		// Unknown executable type.
		return -EIO;
	}

	uint32_t section_table_start = le32_to_cpu(mz.e_lfanew);
	uint32_t SizeOfHeaders;
	switch (exeType) {
		case ExeType::PE:
			section_table_start += sizeof(IMAGE_NT_HEADERS32);
			SizeOfHeaders = le32_to_cpu(hdr.pe.OptionalHeader.opt32.SizeOfHeaders);
			break;
		case ExeType::PE32PLUS:
			section_table_start += sizeof(IMAGE_NT_HEADERS64);
			SizeOfHeaders = le32_to_cpu(hdr.pe.OptionalHeader.opt64.SizeOfHeaders);
			break;
		default:
			// Not a PE executable.
			return -ENOTSUP;
	}

	// Read the section table, up to SizeOfHeaders.
	const uint32_t section_count = (SizeOfHeaders - section_table_start) / sizeof(IMAGE_SECTION_HEADER);
	assert(section_count <= 256);
	if (section_count > 256) {
		// Sanity check: Maximum of 256 sections.
		return -ENOMEM;
	}
	pe_sections.resize(section_count);
	uint32_t szToRead = static_cast<uint32_t>(section_count * sizeof(IMAGE_SECTION_HEADER));
	size_t size = file->seekAndRead(section_table_start, pe_sections.data(), szToRead);
	if (size != static_cast<size_t>(szToRead)) {
		// Seek and/or read error.
		pe_sections.clear();
		return -EIO;
	}

	// Not all sections may be in use.
	// Find the first section header with an empty name.
	int ret = 0;
	for (size_t i = 0; i < pe_sections.size(); i++) {
		if (pe_sections[i].Name[0] == 0) {
			// Found the first empty section.
			pe_sections.resize(i);
			break;
		}
	}

	// Section headers have been read.
	return ret;
}

/**
 * Convert a PE virtual address to a physical address.
 * pe_sections must be loaded.
 * @param vaddr Virtual address.
 * @param size Size of the virtual section.
 * @return Physical address, or 0 if not mappable.
 */
uint32_t EXEPrivate::pe_vaddr_to_paddr(uint32_t vaddr, uint32_t size)
{
	// Make sure the PE section table is loaded.
	if (pe_sections.empty()) {
		if (loadPESectionTable() != 0) {
			// Error loading the PE section table.
			return 0;
		}
	}

	// FIXME: How does XEX handle this?
	for (const IMAGE_SECTION_HEADER &p : pe_sections) {
		const uint32_t sect_vaddr = le32_to_cpu(p.VirtualAddress);
		if (sect_vaddr <= vaddr) {
			if ((sect_vaddr + le32_to_cpu(p.SizeOfRawData)) >= (vaddr+size)) {
				// Found the section. Adjust the address.
				return (vaddr - sect_vaddr) + le32_to_cpu(p.PointerToRawData);
			}
		}
	}

	// Not mappable.
	return 0;
}

/**
 * Load the top-level PE resource directory.
 * @return 0 on success; negative POSIX error code on error. (-ENOENT if not found)
 */
int EXEPrivate::loadPEResourceTypes(void)
{
	if (rsrcReader) {
		// Resource reader is already initialized.
		return 0;
	} else if (!file || !file->isOpen()) {
		// File isn't open.
		return -EBADF;
	} else if (!isValid) {
		// Unknown executable type.
		return -EIO;
	} else if (exeType != ExeType::PE && exeType != ExeType::PE32PLUS) {
		// Unsupported executable type.
		return -ENOTSUP;
	}

	// Make sure the section table is loaded.
	if (pe_sections.empty()) {
		int ret = loadPESectionTable();
		if (ret != 0) {
			// Unable to load the section table.
			return -EIO;
		}
	}

	// TODO: Find the section that matches the virtual address in
	// data directory entry IMAGE_DATA_DIRECTORY_RESOURCE_TABLE?

	// Find the .rsrc section.
	// .rsrc is usually closer to the end of the section list,
	// so search back to front.
	auto iter = std::find_if(pe_sections.crbegin(), pe_sections.crend(),
		[](const IMAGE_SECTION_HEADER &section) noexcept -> bool {
			return !strcmp(reinterpret_cast<const char*>(section.Name), ".rsrc");
		}
	);
	if (iter == pe_sections.crend()) {
		// No .rsrc section.
		return -ENOENT;
	}

	const IMAGE_SECTION_HEADER *rsrc = &(*iter);

	// Load the resources using PEResourceReader.
	// NOTE: .rsrc address and size are validated by PEResourceReader.
	rsrcReader = std::make_shared<PEResourceReader>(file,
		le32_to_cpu(rsrc->PointerToRawData),
		le32_to_cpu(rsrc->SizeOfRawData),
		le32_to_cpu(rsrc->VirtualAddress));
	if (!rsrcReader->isOpen()) {
		// Failed to open the .rsrc section.
		int err = rsrcReader->lastError();
		rsrcReader.reset();
		return (err != 0 ? err : -EIO);
	}

	// .rsrc section loaded.
	return 0;
}

/**
 * Read PE import/export directory
 * @param dataDir RVA/Size of directory will be stored here.
 * @param type One of IMAGE_DATA_DIRECTORY_{EXPORT,IMPORT}_TABLE
 * @param minSize Minimum direcrory size
 * @param maxSize Maximum directory size
 * @param dirTbl Vector into which the directory will be read.
 * @return 0 on success; negative POSIX error code on error.
 */
int EXEPrivate::readPEImpExpDir(IMAGE_DATA_DIRECTORY &dataDir, int type,
	size_t minSize, size_t maxSize, rp::uvector<uint8_t> &dirTbl)
{
	if (!file || !file->isOpen()) {
		// File isn't open.
		return -EBADF;
	} else if (!isValid) {
		// Unknown executable type.
		return -EIO;
	}

	// Check the import/export table.
	// NOTE: dataDir is 8 bytes, so we'll just copy it
	// instead of using a pointer.
	switch (exeType) {
		case ExeType::PE:
			dataDir = hdr.pe.OptionalHeader.opt32.DataDirectory[type];
			break;
		case ExeType::PE32PLUS:
			dataDir = hdr.pe.OptionalHeader.opt64.DataDirectory[type];
			break;
		default:
			// Not a PE executable.
			return -ENOTSUP;
	}

	if (dataDir.VirtualAddress == 0 || dataDir.Size == 0) {
		// No import/export table.
		return -ENOENT;
	}

	// FIXME: How does XEX handle this?
#if SYS_BYTEORDER == SYS_BIG_ENDIAN
	dataDir.VirtualAddress = le32_to_cpu(dataDir.VirtualAddress);
	dataDir.Size = le32_to_cpu(dataDir.Size);
#endif /* SYS_BYTEORDER == SYS_BIG_ENDIAN */

	// Get the import/export table's physical address.
	const uint32_t tbl_paddr = pe_vaddr_to_paddr(dataDir.VirtualAddress, dataDir.Size);
	if (tbl_paddr == 0) {
		// Not found.
		return -ENOENT;
	}

	// Found the section.
	if (dataDir.Size < minSize) {
		// Not enough space for the import table...
		return -ENOENT;
	} else if (dataDir.Size > maxSize) {
		// Import/export table is larger than 4 MB...
		// This might be data corruption.
		return -EIO;
	}

	// Load the import/export directory table.
	dirTbl.resize(dataDir.Size);
	size_t size = file->seekAndRead(tbl_paddr, dirTbl.data(), dirTbl.size());
	if (size != dirTbl.size()) {
		// Seek and/or read error.
		return -EIO;
	}
	return 0;
}

/**
 * Read a block of null-terminated strings, where the length of the
 * last one isn't known in advance.
 *
 * The amount that will be read is:
 * (high - low) + minExtra + maxExtra
 * Which must be in the range [minMax; minMax + maxExtra]
 *
 * @param low RVA of first string.
 * @param high RVA of last string.
 * @param minExtra Last string must be at least this long.
 * @param minMax The minimum size of the block must be smaller than this.
 * @param maxExtra How many extra bytes can be read.
 * @param outPtr Resulting array.
 * @param outSize How much data was read.
 * @return 0 on success; negative POSIX error code on error.
 */
int EXEPrivate::readPENullBlock(uint32_t low, uint32_t high, uint32_t minExtra,
	uint32_t minMax, uint32_t maxExtra, std::unique_ptr<char[]> &outPtr,
	size_t &outSize)
{
	const uint32_t sizeMin = high - low + minExtra;
	if (sizeMin > minMax) {
		return -EIO;
	}
	const uint32_t paddr = pe_vaddr_to_paddr(low, sizeMin);
	if (paddr == 0) {
		// Invalid VAs...
		return -ENOENT;
	}

	const uint32_t sizeMax = sizeMin + maxExtra;
	outPtr.reset(new char[sizeMax]);
	outSize = file->seekAndRead(paddr, reinterpret_cast<uint8_t*>(outPtr.get()), sizeMax);
	if (outSize < sizeMin || outSize > sizeMax) {
		// Seek and/or read error.
		return -EIO;
	}
	return 0;
}

/**
 * Read PE Import Directory (peImportDir) and DLL names (peImportNames).
 * @return 0 on success; negative POSIX error code on error.
 */
int EXEPrivate::readPEImportDir(void)
{
	if (peImportDirLoaded)
		return 0;

	// NOTE: There appears to be two copies of the DLL listing.
	// There's one in the file header before any sections, and
	// one in the import directory table area. This code uses
	// the import directory table area, though it might be
	// easier to use the first copy...

	// NOTE: The DLL filename strings may be included in the import
	// directory table area (MinGW), or they might be located before
	// the import directory table (MSVC 2017). The import directory
	// table size might not be an exact multiple of
	// IMAGE_IMPORT_DIRECTORY in the former case.

	IMAGE_DATA_DIRECTORY dataDir;
	rp::uvector<uint8_t> impDirTbl;
	int res = readPEImpExpDir(dataDir, IMAGE_DATA_DIRECTORY_IMPORT_TABLE,
		sizeof(IMAGE_IMPORT_DIRECTORY), 4*1024*1024, impDirTbl);
	if (res)
		return res;

	// Find the lowest and highest DLL name VAs in the import directory table.
	uint32_t dll_vaddr_low = ~0U;
	uint32_t dll_vaddr_high = 0U;
	const IMAGE_IMPORT_DIRECTORY *const pImpDirTbl = reinterpret_cast<const IMAGE_IMPORT_DIRECTORY*>(impDirTbl.data());
	const IMAGE_IMPORT_DIRECTORY *const pImpDirTblEnd = pImpDirTbl + (impDirTbl.size() / sizeof(IMAGE_IMPORT_DIRECTORY));
	const IMAGE_IMPORT_DIRECTORY *p = pImpDirTbl;
	for (; p < pImpDirTblEnd; p++) {
		if (p->rvaModuleName == 0) {
			// End of table.
			break;
		}

		const uint32_t rvaModuleName = le32_to_cpu(p->rvaModuleName);
		if (rvaModuleName < dll_vaddr_low) {
			dll_vaddr_low = rvaModuleName;
		}
		if (rvaModuleName > dll_vaddr_high) {
			dll_vaddr_high = rvaModuleName;
		}
	}
	if (dll_vaddr_low == ~0U || dll_vaddr_high == 0) {
		// No imports...
		return -ENOENT;
	}

	unique_ptr<char[]> dll_name_data;
	size_t dll_size_read;
	// NOTE: Since the DLL names are NULL-terminated, we'll have to guess
	// with the last one. It's unlikely that it'll be at EOF, but we'll
	// allow for 'short reads'.
	res = readPENullBlock(dll_vaddr_low, dll_vaddr_high,
		1, // minExtra
		// More than 1 MB of DLL names is highly unlikely.
		// There's probably some corruption in the import table.
		1*1024*1024, // minMax
		// MAX_PATH
		260, // maxExtra
		dll_name_data, dll_size_read);
	if (res)
		return res;

	// Ensure the end of the buffer is NULL-terminated.
	dll_name_data[dll_size_read-1] = '\0';

	// Copy to peImportDir
	peImportDir.clear();
	peImportDir.insert(peImportDir.begin(), pImpDirTbl, p);

	// Fill peImportNames
	peImportNames.clear();
	peImportNames.reserve(peImportDir.size());
	for(auto &ent : peImportDir) {
		const uint32_t vaddr = le32_to_cpu(ent.rvaModuleName);
		assert(vaddr >= dll_vaddr_low);
		assert(vaddr <= dll_vaddr_high);
		if (vaddr < dll_vaddr_low || vaddr > dll_vaddr_high) {
			// Out of bounds? This shouldn't have happened...
			return -ENOENT;
		}

		// Current DLL name from the import table.
		const char *const dll_name = &dll_name_data[vaddr - dll_vaddr_low];
		peImportNames.emplace_back(dll_name);
	}

	peImportDirLoaded = true;
	return 0;
}
/**
 * Find the runtime DLL. (PE version)
 * @param refDesc String to store the description.
 * @param refLink String to store the download link.
 * @return 0 on success; negative POSIX error code on error.
 */
int EXEPrivate::findPERuntimeDLL(string &refDesc, string &refLink)
{
	refDesc.clear();
	refLink.clear();

	const bool is64 = exeType == ExeType::PE32PLUS;

	int res = readPEImportDir();
	if (res)
		return res;

	// MSVC runtime DLL version to display version table.
	// Reference: https://matthew-brett.github.io/pydagogue/python_msvc.html
	// NOTE: MSVC debug runtimes are NOT redistributable.
	// TODO: Move somewhere else?
	struct msvc_dll_t {
		uint16_t dll_name_version;	// e.g. 140, 120
		const char display_version[6];
		const char *url_i386;	// i386 download link
		const char *url_amd64;	// amd64 download link
	};
	static const array<msvc_dll_t, 13> msvc_dll_tbl = {{
		{120,	"2013", "https://aka.ms/highdpimfc2013x86enu", "https://aka.ms/highdpimfc2013x64enu"},
		{110,	"2012", "https://download.microsoft.com/download/1/6/B/16B06F60-3B20-4FF2-B699-5E9B7962F9AE/VSU_4/vcredist_x86.exe", "https://download.microsoft.com/download/1/6/B/16B06F60-3B20-4FF2-B699-5E9B7962F9AE/VSU_4/vcredist_x64.exe"},
		{100,	"2010", "https://download.microsoft.com/download/1/6/5/165255E7-1014-4D0A-B094-B6A430A6BFFC/vcredist_x86.exe", "https://download.microsoft.com/download/1/6/5/165255E7-1014-4D0A-B094-B6A430A6BFFC/vcredist_x64.exe"},
		{ 90,	"2008", "https://download.microsoft.com/download/5/D/8/5D8C65CB-C849-4025-8E95-C3966CAFD8AE/vcredist_x86.exe", "https://download.microsoft.com/download/5/D/8/5D8C65CB-C849-4025-8E95-C3966CAFD8AE/vcredist_x64.exe"},
		{ 80,	"2005", "https://download.microsoft.com/download/8/B/4/8B42259F-5D70-43F4-AC2E-4B208FD8D66A/vcredist_x86.EXE", "https://download.microsoft.com/download/8/B/4/8B42259F-5D70-43F4-AC2E-4B208FD8D66A/vcredist_x64.EXE"},
		{ 71,	"2003", nullptr, nullptr},
		{ 70,	"2002", nullptr, nullptr},
		{ 60,	 "6.0", nullptr, nullptr},	// NOTE: MSVC 6.0 uses "msvcrt.dll".
		{ 50,	 "5.0", nullptr, nullptr},
		{ 42,	 "4.2", nullptr, nullptr},
		{ 40,	 "4.0", nullptr, nullptr},
		{ 20,	 "2.0", nullptr, nullptr},
		{ 10,	 "1.0", nullptr, nullptr},
	}};

	// Visual Basic DLL version to display version table.
	struct msvb_dll_t {
		uint8_t ver_major;
		uint8_t ver_minor;
		const char dll_name[14];
		const char *url;
	};
	static const array<msvb_dll_t, 4> msvb_dll_tbl = {{
		{6,0, "msvbvm60.dll", "https://download.microsoft.com/download/5/a/d/5ad868a0-8ecd-4bb0-a882-fe53eb7ef348/VB6.0-KB290887-X86.exe"},
		{5,0, "msvbvm50.dll", "https://download.microsoft.com/download/vb50pro/utility/1/win98/en-us/msvbvm50.exe"},

		// FIXME: Is it vbrun400.dll, vbrun432.dll, or both?
		// TODO: Find a download link.
		{4,0, "vbrun400.dll", nullptr},
		{4,0, "vbrun432.dll", nullptr},
	}};

	// Check all of the DLL names.
	bool found = false;
	for (const auto &s_dll_name : peImportNames) {
		// Current DLL name from the import table.
		const char *const dll_name = s_dll_name.c_str();

		// Check for MSVC 2015-2019. (vcruntime140.dll)
		if (!strcasecmp(dll_name, "vcruntime140.dll")) {
			// TODO: If host OS is Windows XP or earlier, limit it to 2017?
			refDesc = fmt::format(
				C_("EXE|Runtime", "Microsoft Visual C++ {:s} Runtime"), "2015-2022");
			switch (le16_to_cpu(hdr.pe.FileHeader.Machine)) {
				case IMAGE_FILE_MACHINE_I386:
					refLink = "https://aka.ms/vs/17/release/VC_redist.x86.exe";
					break;
				case IMAGE_FILE_MACHINE_AMD64:
					refLink = "https://aka.ms/vs/17/release/VC_redist.x64.exe";
					break;
				case IMAGE_FILE_MACHINE_ARM64:
					refLink = "https://aka.ms/vs/17/release/VC_redist.arm64.exe";
					break;
				default:
					break;
			}
			break;
		} else if (!strcasecmp(dll_name, "vcruntime140d.dll")) {
			refDesc = fmt::format(
				C_("EXE|Runtime", "Microsoft Visual C++ {:s} Debug Runtime"), "2015-2022");
			break;
		}

		// NOTE: MSVCP*.dll is usually listed first in executables
		// that use C++, so check for both P and R.

		if (!strncasecmp(dll_name, "msvcp", 5) || !strncasecmp(dll_name, "msvcr", 5)) {
			unsigned int dll_name_version = 0;
			char after_char = '\0';
			int n = sscanf(dll_name+5, "%u%c", &dll_name_version, &after_char);
			// Check for MSVCR debug build patterns.
			if (n == 2 && !strcasecmp(strchr(dll_name+5, after_char), "d.dll")) {
				// Found an MSVC debug DLL.
				for (const auto &p : msvc_dll_tbl) {
					if (p.dll_name_version == dll_name_version) {
						// Found a matching version.
						refDesc = fmt::format(
							C_("EXE|Runtime", "Microsoft Visual C++ {:s} Debug Runtime"),
							p.display_version);
						found = true;
						break;
					}
				}
			}
			if (found)
				break;

			// Check for MSVCR release build patterns.
			if (n == 2 && !strcasecmp(strchr(dll_name+5, after_char), ".dll")) {
				// Found an MSVC release DLL.
				for (const auto &p : msvc_dll_tbl) {
					if (p.dll_name_version == dll_name_version) {
						// Found a matching version.
						refDesc = fmt::format(
							C_("EXE|Runtime", "Microsoft Visual C++ {:s} Runtime"),
							p.display_version);
						if (is64) {
							if (p.url_amd64) {
								refLink = p.url_amd64;
							}
						} else {
							if (p.url_i386) {
								refLink = p.url_i386;
							}
						}
						found = true;
						break;
					}
				}
			}
		}


		// Check for MSVCRT.DLL.
		// FIXME: This is used by both 5.0 and 6.0.
		if (!strcasecmp(dll_name, "msvcrt.dll")) {
			// NOTE: Conflict between MSVC 6.0 and the "system" MSVCRT.
			// TODO: Other heuristics to figure this out. (Check for msvcp60.dll?)
			refDesc = C_("EXE|Runtime", "Microsoft System C++ Runtime");
			break;
		} else if (!strcasecmp(dll_name, "msvcrtd.dll")) {
			refDesc = fmt::format(
				C_("EXE|Runtime", "Microsoft Visual C++ {:s} Debug Runtime"), "6.0");
			break;
		}

		// Check for Visual Basic DLLs.
		// NOTE: There's only three 32-bit versions of Visual Basic,
		// and .NET versions don't count.
		for (const auto &p : msvb_dll_tbl) {
			if (!strcasecmp(dll_name, p.dll_name)) {
				// Found a matching version.
				refDesc = fmt::format(C_("EXE|Runtime", "Microsoft Visual Basic {:d}.{:d} Runtime"),
					p.ver_major, p.ver_minor);
				refLink = p.url;
				break;
			}
		}
	}

	return (!refDesc.empty() ? 0 : -ENOENT);
}

/**
 * Add fields for PE and PE32+ executables.
 */
void EXEPrivate::addFields_PE(void)
{
	// Up to 6 tabs.
	fields.reserveTabs(6);

	// PE Header
	fields.setTabName(0, "PE");
	fields.setTabIndex(0);

	const uint16_t machine = le16_to_cpu(hdr.pe.FileHeader.Machine);
	const uint16_t pe_flags = le16_to_cpu(hdr.pe.FileHeader.Characteristics);

	// Get the architecture-specific fields.
	uint16_t os_ver_major, os_ver_minor;
	uint16_t subsystem_ver_major, subsystem_ver_minor;
	uint16_t dll_flags;
	bool dotnet;
	if (exeType == EXEPrivate::ExeType::PE) {
		const auto &opthdr = hdr.pe.OptionalHeader.opt32;
		os_ver_major = le16_to_cpu(opthdr.MajorOperatingSystemVersion);
		os_ver_minor = le16_to_cpu(opthdr.MinorOperatingSystemVersion);
		subsystem_ver_major = le16_to_cpu(opthdr.MajorSubsystemVersion);
		subsystem_ver_minor = le16_to_cpu(opthdr.MinorSubsystemVersion);
		dll_flags = le16_to_cpu(opthdr.DllCharacteristics);
		// TODO: Check VirtualAddress, Size, or both?
		// 'file' checks VirtualAddress.
		dotnet = (opthdr.DataDirectory[IMAGE_DATA_DIRECTORY_CLR_HEADER].Size != 0);
	} else /*if (exeType == EXEPrivate::ExeType::PE32PLUS)*/ {
		const auto &opthdr = hdr.pe.OptionalHeader.opt64;
		os_ver_major = le16_to_cpu(opthdr.MajorOperatingSystemVersion);
		os_ver_minor = le16_to_cpu(opthdr.MinorOperatingSystemVersion);
		subsystem_ver_major = le16_to_cpu(opthdr.MajorSubsystemVersion);
		subsystem_ver_minor = le16_to_cpu(opthdr.MinorSubsystemVersion);
		dll_flags = le16_to_cpu(opthdr.DllCharacteristics);
		// TODO: Check VirtualAddress, Size, or both?
		// 'file' checks VirtualAddress.
		dotnet = (opthdr.DataDirectory[IMAGE_DATA_DIRECTORY_CLR_HEADER].Size != 0);
	}

	// CPU (Also .NET status)
	string s_cpu;

	// Check for a hybrid metadata pointer.
	// If present, this is an ARM64EC/ARM64X executable.
	uint64_t hybridMetadataPointer = getHybridMetadataPointer();
	if (hybridMetadataPointer != 0) {
		switch (machine) {
			case IMAGE_FILE_MACHINE_I386:
				s_cpu = "CHPE i386";
				break;
			case IMAGE_FILE_MACHINE_AMD64:
				s_cpu = "CHPEv2 ARM64EC";
				break;
			case IMAGE_FILE_MACHINE_ARM64:
				s_cpu = "CHPEv2 ARM64X";
				break;
			default:
				break;
		}
	}
	if (s_cpu.empty()) {
		const char *const cpu = EXEData::lookup_pe_cpu(machine);
		if (cpu != nullptr) {
			s_cpu = cpu;
		} else {
			s_cpu = fmt::format(C_("RomData", "Unknown (0x{:0>4X})"), machine);
		}
	}
	if (dotnet) {
		// .NET executable
		// TODO: How does this interact with CHPE?
		s_cpu += " (.NET)";
	}
	fields.addField_string(C_("EXE", "CPU"), s_cpu);

	// OS version
	fields.addField_string(C_("RomData", "OS Version"),
		fmt::format(FSTR("{:d}.{:d}"), os_ver_major, os_ver_minor));

	// Subsystem name and version
	string subsystem_display;
	const char *const s_subsystem = EXEData::lookup_pe_subsystem(pe_subsystem);
	if (s_subsystem) {
		subsystem_display = fmt::format(FSTR("{:s} {:d}.{:d}"), s_subsystem,
			subsystem_ver_major, subsystem_ver_minor);
	} else {
		const char *const s_unknown = C_("RomData", "Unknown");
		if (pe_subsystem == IMAGE_SUBSYSTEM_UNKNOWN) {
			subsystem_display = fmt::format(FSTR("{:s} {:d}.{:d}"),
				s_unknown, subsystem_ver_major, subsystem_ver_minor);
		} else {
			subsystem_display = fmt::format(FSTR("{:s} ({:d}) {:d}.{:d}"),
				s_unknown, pe_subsystem, subsystem_ver_major, subsystem_ver_minor);
		}
	}
	fields.addField_string(C_("EXE", "Subsystem"), subsystem_display);

	// PE flags (characteristics)
	// NOTE: Only important flags will be listed.
	static const array<const char*, 16> pe_flags_names = {{
		nullptr,
		NOP_C_("EXE|PEFlags", "Executable"),
		nullptr, nullptr, nullptr,
		NOP_C_("EXE|PEFlags", ">2GB addressing"),
		nullptr, nullptr, nullptr,
		nullptr, nullptr, nullptr,
		nullptr,
		NOP_C_("EXE|PEFlags", "DLL"),
		nullptr, nullptr,
	}};
	vector<string> *const v_pe_flags_names = RomFields::strArrayToVector_i18n("EXE|PEFlags", pe_flags_names);
	fields.addField_bitfield(C_("EXE", "PE Flags"),
		v_pe_flags_names, 3, pe_flags);

	// DLL flags (characteristics)
	static const array<const char*, 16> dll_flags_names = {{
		nullptr, nullptr, nullptr, nullptr, nullptr,
		NOP_C_("EXE|DLLFlags", "High Entropy VA"),
		NOP_C_("EXE|DLLFlags", "Dynamic Base"),
		NOP_C_("EXE|DLLFlags", "Force Integrity"),
		NOP_C_("EXE|DLLFlags", "NX Compatible"),
		NOP_C_("EXE|DLLFlags", "No Isolation"),
		NOP_C_("EXE|DLLFlags", "No SEH"),
		NOP_C_("EXE|DLLFlags", "No Bind"),
		NOP_C_("EXE|DLLFlags", "AppContainer"),
		NOP_C_("EXE|DLLFlags", "WDM Driver"),
		NOP_C_("EXE|DLLFlags", "Control Flow Guard"),
		NOP_C_("EXE|DLLFlags", "TS Aware"),
	}};
	vector<string> *const v_dll_flags_names = RomFields::strArrayToVector_i18n("EXE|DLLFlags", dll_flags_names);
	fields.addField_bitfield(C_("EXE", "DLL Flags"),
		v_dll_flags_names, 3, dll_flags);

	// Timestamp
	// TODO: Windows 10 modules have hashes here instead of timestamps.
	// We should detect that by checking for obviously out-of-range values.
	// TODO: time_t is signed, so values greater than 2^31-1 may be negative.
	const char *const timestamp_title = C_("EXE", "Timestamp");
	const uint32_t timestamp = le32_to_cpu(hdr.pe.FileHeader.TimeDateStamp);
	if (timestamp != 0) {
		fields.addField_dateTime(timestamp_title,
			static_cast<time_t>(timestamp),
			RomFields::RFT_DATETIME_HAS_DATE |
			RomFields::RFT_DATETIME_HAS_TIME);
	} else {
		fields.addField_string(timestamp_title, C_("EXE", "Not set"));
	}

	// Runtime DLL
	string runtime_dll, runtime_link;
	int ret = findPERuntimeDLL(runtime_dll, runtime_link);
	if (ret == 0 && !runtime_dll.empty()) {
		// TODO: Show the link?
		fields.addField_string(C_("EXE", "Runtime DLL"), runtime_dll);
	}

	// Load resources
	ret = loadPEResourceTypes();
	if (ret == 0 && rsrcReader != nullptr) {
		// Load the version resource.
		// NOTE: load_VS_VERSION_INFO loads it in host-endian.
		VS_FIXEDFILEINFO vsffi;
		IResourceReader::StringFileInfo vssfi;
		if (rsrcReader->load_VS_VERSION_INFO(VS_VERSION_INFO, -1, &vsffi, &vssfi) == 0) {
			// Add the version fields.
			fields.setTabName(1, C_("RomData", "Version"));
			fields.setTabIndex(1);
			addFields_VS_VERSION_INFO(&vsffi, &vssfi);
		}

#ifdef ENABLE_XML
		// Parse the manifest if it's present.
		// TODO: Support external manifests, e.g. program.exe.manifest?
		addFields_PE_Manifest();
#endif /* ENABLE_XML */
	}

	if (!dotnet) {
		// Add exports / imports
		// NOTE: .NET executables have a single import,
		// MSCOREE!_CorExeMain, so we're ignoring the
		// import/export tables for .NET.
		addFields_PE_Export();
		addFields_PE_Import();
	}
}

/**
 * Add fields for PE export table.
 * @return 0 on success; negative POSIX error code on error.
 */
int EXEPrivate::addFields_PE_Export(void)
{
	IMAGE_DATA_DIRECTORY dataDir;
	rp::uvector<uint8_t> expDirTbl;

	int res = readPEImpExpDir(dataDir, IMAGE_DATA_DIRECTORY_EXPORT_TABLE,
		sizeof(IMAGE_EXPORT_DIRECTORY), 4*1024*1024, expDirTbl);
	if (res)
		return res;

	const IMAGE_EXPORT_DIRECTORY *pExpDirTbl = reinterpret_cast<const IMAGE_EXPORT_DIRECTORY*>(expDirTbl.data());
	const uint32_t ordinalBase = le32_to_cpu(pExpDirTbl->Base);
	if (ordinalBase > 65535)
		return -ENOENT;

	// Bounds checking function to ensure that the tables are located inside
	// the export directory.
	const uint32_t rvaMin = dataDir.VirtualAddress;
	const uint32_t rvaMax = dataDir.VirtualAddress + dataDir.Size;
	auto checkBounds = [&](uint32_t rva, uint32_t size) -> void * {
		if (rva < rvaMin || rva > rvaMax)
			return nullptr;
		if (rva+size < rvaMin || rva+size > rvaMax)
			return nullptr;
		return expDirTbl.data() + (rva - rvaMin);
	};

	// Export Address Table
	const uint32_t rvaExpAddrTbl = le32_to_cpu(pExpDirTbl->AddressOfFunctions);
	uint32_t szExpAddrTbl = le32_to_cpu(pExpDirTbl->NumberOfFunctions);
	// Ignore ordinals greater than 65535
	szExpAddrTbl = std::min(65536-ordinalBase, szExpAddrTbl);
	const uint32_t *expAddrTbl = reinterpret_cast<const uint32_t*>(
		checkBounds(rvaExpAddrTbl, szExpAddrTbl*sizeof(uint32_t)));
	if (!expAddrTbl)
		return -ENOENT;

	// Export Name Table
	const uint32_t rvaExpNameTbl = le32_to_cpu(pExpDirTbl->AddressOfNames);
	const uint32_t szExpNameTbl = le32_to_cpu(pExpDirTbl->NumberOfNames);
	const uint32_t *expNameTbl = reinterpret_cast<const uint32_t*>(
		checkBounds(rvaExpNameTbl, szExpNameTbl*sizeof(uint32_t)));
	if (!expNameTbl)
		return -ENOENT;

	// Export Ordinal Table
	const uint32_t rvaExpOrdTbl = le32_to_cpu(pExpDirTbl->AddressOfNameOrdinals);
	const uint16_t *expOrdTbl = reinterpret_cast<const uint16_t*>(
		checkBounds(rvaExpOrdTbl, szExpNameTbl*sizeof(uint16_t)));
	if (!expOrdTbl)
		return -ENOENT;

	struct ExportEntry {
		int ordinal;	// index in the address table + ordinal base
		int hint;	// index in the name table (-1 if no name)
		uint32_t vaddr;	// RVA of the symbol
		uint32_t paddr; // corresponding file offset
		string name;
		string forwarder;
	};
	std::vector<ExportEntry> ents;
	ents.reserve(std::max(szExpAddrTbl, szExpNameTbl));

	// Read address table
	const uint32_t expDirTbl_base = le32_to_cpu(pExpDirTbl->Base);
	for (uint32_t i = 0; i < szExpAddrTbl; i++) {
		ExportEntry ent;
		ent.ordinal = expDirTbl_base + i;
		ent.hint = -1;
		ent.vaddr = le32_to_cpu(expAddrTbl[i]);
		ent.paddr = pe_vaddr_to_paddr(ent.vaddr, 0);
		if (ent.vaddr >= rvaMin && ent.vaddr < rvaMax) {
			/* RVA points inside export directory, so this is
			 * a forwarder RVA. It points to a NUL-terminated
			 * string that looks like "dllname.symbol" or
			 * "dllname.#123". */
			const char *fwd = reinterpret_cast<const char *>(expDirTbl.data() + (ent.vaddr - rvaMin));
			ent.forwarder.assign(fwd, strnlen(fwd, rvaMax - ent.vaddr));
		}
		ents.push_back(std::move(ent));
	}

	// Read name table
	for (uint32_t i = 0; i < szExpNameTbl; i++) {
		const uint32_t rvaName = le32_to_cpu(expNameTbl[i]);
		const uint16_t ord = le16_to_cpu(expOrdTbl[i]);
		if (ord >= szExpAddrTbl) // out of bounds ordinal
			continue;
		if (rvaName < rvaMin || rvaName >= rvaMax)
			continue;
		const char *pName = reinterpret_cast<const char*>(expDirTbl.data() + (rvaName - rvaMin));
		string name(pName, strnlen(pName, rvaMax - rvaName));
		if (ents[ord].hint != -1) {
			// This ordinal already has a name.
			// Temporarily move the old name out to avoid a copy.
			string oldname = std::move(ents[ord].name);
			// Duplicate the entry, replace name and hint in the copy.
			ExportEntry ent = ents[ord];
			ents[ord].name = std::move(oldname);
			ent.name = std::move(name);
			ent.hint = i;
			ents.push_back(std::move(ent));
		} else {
			ents[ord].name = std::move(name);
			ents[ord].hint = i;
		}
		/* NOTE: We don't have to filter out duplicate names, because
		 * they can be still accessed via hints. */
	}

	/* Sort by (hint, ordinal). This puts ordinal-only symbols at the top,
	 * in ordinal order, followed by named symbols in lexicographic order.
	 * ...unless your names aren't sorted, in which case your DLL is broken.
	 * NOTE: ExportEntry is 80 bytes on 64-bit, so we'll use a sort index map. */
	std::unique_ptr<unsigned int[]> sortIndexMap(new unsigned int[ents.size()]);
	const auto p_sortIndexMap_end = sortIndexMap.get() + ents.size();
	std::iota(sortIndexMap.get(), p_sortIndexMap_end, 0);

	std::sort(sortIndexMap.get(), p_sortIndexMap_end,
		[&ents](unsigned int idx_lhs, unsigned int idx_rhs) -> bool {
			const ExportEntry &lhs = ents[idx_lhs];
			const ExportEntry &rhs = ents[idx_rhs];

			if (lhs.hint < rhs.hint) return true;
			if (lhs.hint > rhs.hint) return false;

			if (lhs.ordinal < rhs.ordinal) return true;
			/*if (lhs.ordinal > rhs.ordinal)*/ return false;
		});

	// Convert to ListData
	auto *const vv_data = new RomFields::ListData_t();
	vv_data->reserve(ents.size());
	for (const ExportEntry &ent : ents) {
		// Filter out any unused ordinals.
		if (ent.hint == -1 && ent.vaddr == 0)
			continue;
		vv_data->emplace_back();
		auto &row = vv_data->back();
		row.reserve(5);
		row.push_back(ent.name);
		row.push_back(fmt::to_string(ent.ordinal));
		if (ent.hint != -1) {
			row.push_back(fmt::to_string(ent.hint));
		} else {
			row.emplace_back(C_("EXE|Exports", "None"));
		}
		if (ent.forwarder.size() != 0) {
			row.push_back(ent.forwarder);
			row.emplace_back();
		} else {
			row.push_back(fmt::format(FSTR("0x{:0>8X}"), ent.vaddr));
			if (ent.paddr)
				row.push_back(fmt::format("0x{:0>8X}", ent.paddr));
			else
				row.emplace_back(); // it's probably in the bss section
		}
	}

	// Create the tab if we have any exports.
	if (vv_data->size()) {
		fields.addTab(C_("EXE", "Exports"));
		fields.reserve(1);

		static const array<const char*, 5> field_names = {{
			NOP_C_("EXE|Exports", "Name"),
			NOP_C_("EXE|Exports", "Ordinal"),
			NOP_C_("EXE|Exports", "Hint"),
			NOP_C_("EXE|Exports", "Virtual Address"),
			NOP_C_("EXE|Exports", "File Offset"),
		}};
		vector<string> *const v_field_names = RomFields::strArrayToVector_i18n("EXE|Exports", field_names);

		RomFields::AFLD_PARAMS params;
		params.flags = RomFields::RFT_LISTDATA_SEPARATE_ROW;
		params.headers = v_field_names;
		params.data.single = vv_data;
		// TODO: Header alignment?
		params.col_attrs.align_data = AFLD_ALIGN5(TXA_D, TXA_R, TXA_D, TXA_D, TXA_D);
		params.col_attrs.sorting    = AFLD_ALIGN5(COLSORT_NC, COLSORT_NUM, COLSORT_NUM, COLSORT_STD, COLSORT_STD);
		params.col_attrs.sort_col   = 0;	// Name
		params.col_attrs.sort_dir   = RomFields::COLSORTORDER_ASCENDING;
		fields.addField_listData(C_("EXE", "Exports"), &params);
	} else {
		delete vv_data;
	}

	return 0;
}

/**
 * Add fields for PE import table.
 * @return 0 on success; negative POSIX error code on error.
 */
int EXEPrivate::addFields_PE_Import(void)
{
	const bool is64 = exeType == ExeType::PE32PLUS;

	int res = readPEImportDir();
	if (res)
		return res;

	unique_ptr<char[]> dll_ilt_data;
	uint32_t dll_ilt_base;
	size_t dll_ilt_read;
	// Read Import Lookup Table
	{
		// Find the lowest and highest ImportLookupTable RVAs
		if (peImportDir.empty()) {
			// No imports...
			return -ENOENT;
		}
		uint32_t dll_vaddr_low;
		uint32_t dll_vaddr_high;
		dll_vaddr_low = dll_vaddr_high = le32_to_cpu(peImportDir[0].rvaImportLookupTable);
		for (const auto &ent : peImportDir) {
			const uint32_t rvaImportLookupTable = le32_to_cpu(ent.rvaImportLookupTable);
			if ((rvaImportLookupTable - dll_vaddr_low) & (is64?7:3)) {
				/* Bad alignment. This check is mostly so that
				 * I don't have to think what is the correct
				 * "end" pointer after we type-pun to uint32.
				 * Relative alignment is enough, though. */
				return -ENOENT;
			}
			if (rvaImportLookupTable < dll_vaddr_low) {
				dll_vaddr_low = rvaImportLookupTable;
			}
			if (rvaImportLookupTable > dll_vaddr_high) {
				dll_vaddr_high = rvaImportLookupTable;
			}
		}
		res = readPENullBlock(dll_vaddr_low, dll_vaddr_high,
			// ILTs are terminated by NULL-pointer, so we need at
			// least size_t extra bytes for the last table.
			(is64?8:4), // minExtra
			// 1 MB is 128 K imports at 64-bits.
			1*1024*1024, // minMax
			// 128 KB is 16 K imports at 64-bits.
			128*1024, // maxExtra
			dll_ilt_data, dll_ilt_read);
		if (res)
			return res;
		dll_ilt_base = dll_vaddr_low;
	}

	// Iterator for import entries
	const uint32_t *ilt_buf = reinterpret_cast<const uint32_t*>(dll_ilt_data.get());
	const uint32_t *ilt_end;
	if (is64)
		ilt_end = ilt_buf + dll_ilt_read/8*2;
	else
		ilt_end = ilt_buf + dll_ilt_read/4;
	struct IltIterator {
		unsigned dir_index; // next DLL to read
		const uint32_t *ilt; // next ilt entry to read
		const string *dllname;
		bool is_ordinal;
		uint32_t value; // rva to hint/name or ordinal
		explicit IltIterator(const uint32_t *end)
			: dir_index(0)
			, ilt(end)
			, dllname(nullptr)
			, is_ordinal(false)
			, value(0)
		{ }
	};
	auto iltAdvance = [this, ilt_buf, ilt_end, dll_ilt_base, is64](IltIterator &it) -> bool {
		auto &dir_index = it.dir_index;
		auto &ilt = it.ilt;
		auto &dllname = it.dllname;
		auto &is_ordinal = it.is_ordinal;
		auto &value = it.value;
		while (ilt >= ilt_end) {
			// read next directory entry
			if (dir_index == peImportDir.size())
				return false;
			ilt = &ilt_buf[(le32_to_cpu(peImportDir[dir_index].rvaImportLookupTable) - dll_ilt_base)/4];
			dllname = &peImportNames[dir_index];
			dir_index++;
			// check for NULL entry
			if (!ilt[0] && (!is64 || !ilt[1]))
				ilt = ilt_end;
		}
		// parse ILT entry
		is_ordinal = le32_to_cpu(ilt[is64 ? 1 : 0]) & 0x80000000;
		value = le32_to_cpu(ilt[0]) & (is_ordinal ? 0xFFFF : 0x7FFFFFFF);
		// advance ILT pointer
		ilt += is64 ? 2 : 1;
		// check for NULL entry
		if (!ilt[0] && (!is64 || !ilt[1]))
			ilt = ilt_end;
		return true;
	};

	// Read Name/Hint Table
	unique_ptr<char[]> dll_hint_data;
	uint32_t dll_hint_base;
	size_t dll_hint_read;
	int import_count = 0;
	{
		// Find the lowest and highest hint/name RVAs
		uint32_t dll_vaddr_low = ~0U;
		uint32_t dll_vaddr_high = 0U;
		for (IltIterator it(ilt_end); iltAdvance(it); ) {
			import_count++;
			if (it.is_ordinal)
				continue;
			if (it.value < dll_vaddr_low)
				dll_vaddr_low = it.value;
			if (it.value > dll_vaddr_high)
				dll_vaddr_high = it.value;
		}
		if (dll_vaddr_low == ~0U || dll_vaddr_high == 0) {
			// No imports...
			return -ENOENT;
		}

		res = readPENullBlock(dll_vaddr_low, dll_vaddr_high,
			// 2-byte hint + NUL terminator
			3, // minExtra
			// 4 MB for all symbol names
			4*1024*1024, // minMax
			// Those C++ symbols can get quite long...
			8*1024, // maxExtra
			dll_hint_data, dll_hint_read);
		if (res)
			return res;
		dll_hint_data[dll_hint_read-1] = '\0';
		dll_hint_base = dll_vaddr_low;
	}

	// Generate list data
	auto *const vv_data = new RomFields::ListData_t();
	vv_data->reserve(import_count);
	for (IltIterator it(ilt_end); iltAdvance(it); ) {
		vv_data->emplace_back();
		auto &row = vv_data->back();
		row.reserve(3);
		if (it.is_ordinal) {
			row.push_back(fmt::format(C_("EXE|Exports", "Ordinal #{:d}"), it.value));
			row.emplace_back();
		} else {
			// RVA to hint number followed by NUL terminated name.
			// FIXME: How does XEX handle this?
			const char *const ent = &dll_hint_data[it.value - dll_hint_base];
			// FIXME: This may break on non-i386/amd64 systems...
			const uint16_t hint = le16_to_cpu(*reinterpret_cast<const uint16_t*>(ent));
			row.emplace_back(ent+2);
			row.push_back(fmt::to_string(hint));
		}
		row.push_back(*(it.dllname));
	}

	// Sort the list data by (module, name, hint).
	std::sort(vv_data->begin(), vv_data->end(),
		[](const vector<string> &lhs, const vector<string> &rhs) -> bool {
			// Vector index 0: Name
			// Vector index 1: Hint
			// Vector index 2: Module
			int res = strcasecmp(lhs[2].c_str(), rhs[2].c_str());
			if (res < 0) return true;
			if (res > 0) return false;

			res = strcasecmp(lhs[0].c_str(), rhs[0].c_str());
			if (res < 0) return true;
			if (res > 0) return false;

			// Hint is numeric, so convert it to a number first.
			unsigned long hint_lhs, hint_rhs;
			char *endptr_lhs, *endptr_rhs;
			hint_lhs = strtoul(lhs[1].c_str(), &endptr_lhs, 10);
			hint_rhs = strtoul(lhs[1].c_str(), &endptr_rhs, 10);
			if (!endptr_lhs || *endptr_lhs == '\0' || !endptr_rhs || *endptr_rhs == '\0') {
				// Invalid numeric value. Do a string comparison instead.
				return (strcasecmp(lhs[1].c_str(), rhs[1].c_str()) < 0);
			}

			// Numeric conversion succeeded.
			return (hint_lhs < hint_rhs);
		});

	// Add the tab
	fields.addTab(C_("EXE", "Imports"));
	fields.reserve(1);

	// Intentionally sharing the translation context with the exports tab.
	static const array<const char*, 3> field_names = {{
		NOP_C_("EXE|Exports", "Name"),
		NOP_C_("EXE|Exports", "Hint"),
		NOP_C_("EXE|Exports", "Module"),
	}};
	vector<string> *const v_field_names = RomFields::strArrayToVector_i18n("EXE|Exports", field_names);

	RomFields::AFLD_PARAMS params;
	params.flags = RomFields::RFT_LISTDATA_SEPARATE_ROW;
	params.headers = v_field_names;
	params.data.single = vv_data;
	// TODO: Header alignment?
	params.col_attrs.align_data = AFLD_ALIGN3(TXA_D, TXA_R, TXA_D);
	params.col_attrs.sorting    = AFLD_ALIGN3(COLSORT_NC, COLSORT_NUM, COLSORT_NC);
	params.col_attrs.sort_col   = 0;	// Name
	params.col_attrs.sort_dir   = RomFields::COLSORTORDER_ASCENDING;
	fields.addField_listData(C_("EXE", "Imports"), &params);
	return 0;
}

/**
 * Get the hybrid metadata pointer, if present.
 * @return Hybrid metadata pointer, or 0 if not present.
 */
uint64_t EXEPrivate::getHybridMetadataPointer(void)
{
	// Get the Load Config Table.
	if (exeType == EXEPrivate::ExeType::PE) {
		IMAGE_LOAD_CONFIG_DIRECTORY32 load_config;

		// FIXME: How does XEX handle this?
#if SYS_BYTEORDER == SYS_LIL_ENDIAN
		const auto &dirEntry = hdr.pe.OptionalHeader.opt32.DataDirectory[IMAGE_DATA_DIRECTORY_LOAD_CONFIG_TABLE];
#else /* SYS_BYTEORDER == SYS_BIG_ENDIAN */
		auto dirEntry = hdr.pe.OptionalHeader.opt32.DataDirectory[IMAGE_DATA_DIRECTORY_LOAD_CONFIG_TABLE];
		dirEntry.VirtualAddress = le32_to_cpu(dirEntry.VirtualAddress);
		dirEntry.Size = le32_to_cpu(dirEntry.Size);
#endif /* SYS_BYTEORDER == SYS_BIG_ENDIAN */

		if (dirEntry.VirtualAddress == 0 || dirEntry.Size == 0 ||
		    dirEntry.Size < (offsetof(IMAGE_LOAD_CONFIG_DIRECTORY32, CHPEMetadataPointer) + sizeof(uint32_t)) ||
		    dirEntry.Size > sizeof(load_config))
		{
			return 0;
		}

		uint32_t size = dirEntry.Size;
		const uint32_t paddr = pe_vaddr_to_paddr(dirEntry.VirtualAddress, size);
		if (paddr == 0)
			return 0;

		size_t sz_read = file->seekAndRead(paddr, &load_config, size);
		if (sz_read != size)
			return 0;

		// Verify the size of load_config.
		if (size != le32_to_cpu(load_config.Size))
			return 0;

		return le32_to_cpu(load_config.CHPEMetadataPointer);
	} else /*if (exeType == EXEPrivate::ExeType::PE32PLUS)*/ {
		IMAGE_LOAD_CONFIG_DIRECTORY64 load_config;

#if SYS_BYTEORDER == SYS_LIL_ENDIAN
		const auto &dirEntry = hdr.pe.OptionalHeader.opt64.DataDirectory[IMAGE_DATA_DIRECTORY_LOAD_CONFIG_TABLE];
#else /* SYS_BYTEORDER == SYS_BIG_ENDIAN */
		auto dirEntry = hdr.pe.OptionalHeader.opt64.DataDirectory[IMAGE_DATA_DIRECTORY_LOAD_CONFIG_TABLE];
		dirEntry.VirtualAddress = le32_to_cpu(dirEntry.VirtualAddress);
		dirEntry.Size = le32_to_cpu(dirEntry.Size);
#endif /* SYS_BYTEORDER == SYS_BIG_ENDIAN */

		if (dirEntry.VirtualAddress == 0 || dirEntry.Size == 0 ||
		    dirEntry.Size < (offsetof(IMAGE_LOAD_CONFIG_DIRECTORY64, CHPEMetadataPointer) + sizeof(uint64_t)) ||
		    dirEntry.Size > sizeof(load_config))
		{
			return 0;
		}

		uint32_t size = dirEntry.Size;
		const uint32_t paddr = pe_vaddr_to_paddr(dirEntry.VirtualAddress, size);
		if (paddr == 0)
			return 0;

		size_t sz_read = file->seekAndRead(paddr, &load_config, size);
		if (sz_read != size)
			return 0;

		// Verify the size of load_config.
		if (size != le32_to_cpu(load_config.Size))
			return 0;

		return le64_to_cpu(load_config.CHPEMetadataPointer);
	}

	// FIXME: Shouldn't get here...
	assert(!"Unreachable code!");
	return 0;
}

}
