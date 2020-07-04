/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * EXE_PE.cpp: DOS/Windows executable reader.                              *
 * 32-bit/64-bit Portable Executable format.                               *
 *                                                                         *
 * Copyright (c) 2016-2019 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "EXE_p.hpp"
#include "data/EXEData.hpp"
#include "disc/PEResourceReader.hpp"

// librpbase
using namespace LibRpBase;

// C++ STL classes.
using std::string;
using std::unique_ptr;
using std::unordered_set;
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
	uint32_t section_count = (SizeOfHeaders - section_table_start) / sizeof(IMAGE_SECTION_HEADER);
	assert(section_count <= 128);
	if (section_count > 128) {
		// Sanity check: Maximum of 128 sections.
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

	for (auto iter = pe_sections.cbegin(); iter != pe_sections.cend(); ++iter) {
		if (iter->VirtualAddress <= vaddr) {
			if ((iter->VirtualAddress + iter->SizeOfRawData) >= (vaddr+size)) {
				// Found the section. Adjust the address.
				return (vaddr - iter->VirtualAddress) + iter->PointerToRawData;
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
		[](const IMAGE_SECTION_HEADER &section) -> bool {
			return !strcmp(section.Name, ".rsrc");
		}
	);
	if (iter == pe_sections.crend()) {
		// No .rsrc section.
		return -ENOENT;
	}

	const IMAGE_SECTION_HEADER *rsrc = &(*iter);

	// Load the resources using PEResourceReader.
	// NOTE: .rsrc address and size are validated by PEResourceReader.
	rsrcReader = new PEResourceReader(file,
		le32_to_cpu(rsrc->PointerToRawData),
		le32_to_cpu(rsrc->SizeOfRawData),
		le32_to_cpu(rsrc->VirtualAddress));
	if (!rsrcReader->isOpen()) {
		// Failed to open the .rsrc section.
		int err = rsrcReader->lastError();
		UNREF_AND_NULL_NOCHK(rsrcReader);
		return (err != 0 ? err : -EIO);
	}

	// .rsrc section loaded.
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

	if (!file || !file->isOpen()) {
		// File isn't open.
		return -EBADF;
	} else if (!isValid) {
		// Unknown executable type.
		return -EIO;
	}

	// Check the import table.
	// NOTE: dataDir is 8 bytes, so we'll just copy it
	// instead of using a pointer.
	IMAGE_DATA_DIRECTORY dataDir;
	bool is64 = false;
	switch (exeType) {
		case ExeType::PE:
			dataDir = hdr.pe.OptionalHeader.opt32.DataDirectory[IMAGE_DATA_DIRECTORY_IMPORT_TABLE];
			break;
		case ExeType::PE32PLUS:
			dataDir = hdr.pe.OptionalHeader.opt64.DataDirectory[IMAGE_DATA_DIRECTORY_IMPORT_TABLE];
			is64 = true;
			break;
		default:
			// Not a PE executable.
			return -ENOTSUP;
	}

	if (dataDir.VirtualAddress == 0 || dataDir.Size == 0) {
		// No import table.
		return -ENOENT;
	}

	// Get the import table's physical address.
	uint32_t imptbl_paddr = pe_vaddr_to_paddr(dataDir.VirtualAddress, dataDir.Size);
	if (imptbl_paddr == 0) {
		// Not found.
		return -ENOENT;
	}

	// Found the section.
	// NOTE: There appears to be two copies of the DLL listing.
	// There's one in the file header before any sections, and
	// one in the import directory table area. This code uses
	// the import directory table area, though it might be
	// easier to use the first copy...
	if (dataDir.Size < sizeof(IMAGE_IMPORT_DIRECTORY)) {
		// Not enough space for the import table...
		return -ENOENT;
	}

	// Load the import directory table.
	// NOTE: The DLL filename strings may be included in the import
	// directory table area (MinGW), or they might be located before
	// the import directory table (MSVC 2017). The import directory
	// table size might not be an exact multiple of
	// IMAGE_IMPORT_DIRECTORY in the former case.
	ao::uvector<uint8_t> impDirTbl;
	impDirTbl.resize(dataDir.Size);
	size_t size = file->seekAndRead(imptbl_paddr, impDirTbl.data(), impDirTbl.size());
	if (size != impDirTbl.size()) {
		// Seek and/or read error.
		return -EIO;
	}

	// Set containing all of the DLL name VAs.
	unordered_set<uint32_t> set_dll_vaddrs;

	// Find the lowest and highest DLL name VAs in the import directory table.
	uint32_t dll_vaddr_low = ~0U;
	uint32_t dll_vaddr_high = 0U;
	const IMAGE_IMPORT_DIRECTORY *pImpDirTbl = reinterpret_cast<const IMAGE_IMPORT_DIRECTORY*>(impDirTbl.data());
	const IMAGE_IMPORT_DIRECTORY *const pImpDirTblEnd = pImpDirTbl + (impDirTbl.size() / sizeof(IMAGE_IMPORT_DIRECTORY));
	for (; pImpDirTbl < pImpDirTblEnd; pImpDirTbl++) {
		if (pImpDirTbl->rvaImportLookupTable == 0 || pImpDirTbl->rvaModuleName == 0) {
			// End of table.
			break;
		}

		const uint32_t rvaModuleName = le32_to_cpu(pImpDirTbl->rvaModuleName);
		set_dll_vaddrs.insert(rvaModuleName);
		if (rvaModuleName < dll_vaddr_low) {
			dll_vaddr_low = rvaModuleName;
		}
		if (rvaModuleName > dll_vaddr_high) {
			dll_vaddr_high = rvaModuleName;
		}
	}

	// NOTE: Since the DLL names are NULL-terminated, we'll have to guess
	// with the last one. It's unlikely that it'll be at EOF, but we'll
	// allow for 'short reads'.
	const uint32_t dll_size_min = dll_vaddr_high - dll_vaddr_low + 1;
	uint32_t dll_paddr = pe_vaddr_to_paddr(dll_vaddr_low, dll_size_min);
	if (dll_paddr == 0) {
		// Invalid VAs...
		return -ENOENT;
	}

	const uint32_t dll_size_max = dll_size_min + 260;	// MAX_PATH
	unique_ptr<char[]> dll_name_data(new char[dll_size_max]);
	size_t dll_size_read = file->seekAndRead(dll_paddr, reinterpret_cast<uint8_t*>(dll_name_data.get()), dll_size_max);
	if (dll_size_read < dll_size_min || dll_size_read > dll_size_max) {
		// Seek and/or read error.
		return -EIO;
	}
	// Ensure the end of the buffer is NULL-terminated.
	dll_name_data[dll_size_max-1] = '\0';

	// Convert the entire buffer to lowercase. (ASCII characters only)
	{
		char *const p_end = &dll_name_data[dll_size_max];
		for (char *p = dll_name_data.get(); p < p_end; p++) {
			if (*p >= 'A' && *p <= 'Z') *p |= 0x20;
		}
	}

	// MSVC runtime DLL version to display version table.
	// Reference: https://matthew-brett.github.io/pydagogue/python_msvc.html
	// NOTE: MSVC debug runtimes are NOT redistributable.
	// TODO: Move somewhere else?
	static const struct {
		unsigned int dll_name_version;	// e.g. 140, 120
		const char display_version[8];
		const char *url_i386;	// i386 download link
		const char *url_amd64;	// amd64 download link
	} msvc_dll_tbl[] = {
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

		{  0,	    "", nullptr, nullptr}
	};

	// Check all of the DLL names.
	bool found = false;
	for (auto iter = set_dll_vaddrs.cbegin(); iter != set_dll_vaddrs.cend() && !found; ++iter) {
		uint32_t vaddr = *iter;
		assert(vaddr >= dll_vaddr_low);
		assert(vaddr <= dll_vaddr_high);
		if (vaddr < dll_vaddr_low || vaddr > dll_vaddr_high) {
			// Out of bounds? This shouldn't have happened...
			break;
		}

		// Current DLL name from the import table.
		const char *const dll_name = &dll_name_data[vaddr - dll_vaddr_low];

		// Check for MSVC 2015-2019. (vcruntime140.dll)
		if (!strcmp(dll_name, "vcruntime140.dll")) {
			refDesc = rp_sprintf(
				C_("EXE|Runtime", "Microsoft Visual C++ %s Runtime"), "2015-2019");
			if (is64) {
				refLink = "https://aka.ms/vs/16/release/vc_redist.x64.exe";
			} else {
				refLink = "https://aka.ms/vs/16/release/vc_redist.x86.exe";
			}
			break;
		} else if (!strcmp(dll_name, "vcruntime140d.dll")) {
			refDesc = rp_sprintf(
				C_("EXE|Runtime", "Microsoft Visual C++ %s Debug Runtime"), "2015-2019");
			break;
		}

		// NOTE: MSVCP*.dll is usually listed first in executables
		// that use C++, so check for both P and R.

		// Check for MSVCR debug build patterns.
		unsigned int dll_name_version = 0;
		char c_or_cpp = '\0';
		char is_debug = '\0';
		char last_char = '\0';
		int n = sscanf(dll_name, "msvc%c%u%c.dl%c",
			&c_or_cpp, &dll_name_version, &is_debug, &last_char);
		if (n == 4 && (c_or_cpp == 'p' || c_or_cpp == 'r') && is_debug == 'd' && last_char == 'l') {
			// Found an MSVC debug DLL.
			for (const auto *p = &msvc_dll_tbl[0]; p->dll_name_version != 0; p++) {
				if (p->dll_name_version == dll_name_version) {
					// Found a matching version.
					refDesc = rp_sprintf(
						C_("EXE|Runtime", "Microsoft Visual C++ %s Debug Runtime"), p->display_version);
					found = true;
					break;
				}
			}
		}
		if (found)
			break;

		// Check for MSVCR release build patterns.
		n = sscanf(dll_name, "msvc%c%u.dl%c", &c_or_cpp, &dll_name_version, &last_char);
		if (n == 3 && (c_or_cpp == 'p' || c_or_cpp == 'r') && last_char == 'l') {
			// Found an MSVC release DLL.
			for (auto *p = &msvc_dll_tbl[0]; p->dll_name_version != 0; p++) {
				if (p->dll_name_version == dll_name_version) {
					// Found a matching version.
					refDesc = rp_sprintf(
						C_("EXE|Runtime", "Microsoft Visual C++ %s Runtime"), p->display_version);
					if (is64) {
						if (p->url_amd64) {
							refLink = p->url_amd64;
						}
					} else {
						if (p->url_i386) {
							refLink = p->url_i386;
						}
					}
					found = true;
					break;
				}
			}
		}

		// Check for MSVCRT.DLL.
		// FIXME: This is used by both 5.0 and 6.0.
		if (!strcmp(dll_name, "msvcrt.dll")) {
			// NOTE: Conflict between MSVC 6.0 and the "system" MSVCRT.
			// TODO: Other heuristics to figure this out. (Check for msvcp60.dll?)
			refDesc = C_("EXE|Runtime", "Microsoft System C++ Runtime");
			break;
		} else if (!strcmp(dll_name, "msvcrtd.dll")) {
			refDesc = rp_sprintf(
				C_("EXE|Runtime", "Microsoft Visual C++ %s Debug Runtime"), "6.0");
			break;
		}

		// Check for Visual Basic DLLs.
		// NOTE: There's only three 32-bit versions of Visual Basic,
		// and .NET versions don't count.
		if (!strcmp(dll_name, "msvbvm60.dll")) {
			refDesc = rp_sprintf(
				C_("EXE|Runtime", "Microsoft Visual Basic %s Runtime"), "6.0");
			refLink = "https://download.microsoft.com/download/5/a/d/5ad868a0-8ecd-4bb0-a882-fe53eb7ef348/VB6.0-KB290887-X86.exe";
			break;
		} else if (!strcmp(dll_name, "msvbvm50.dll")) {
			refDesc = rp_sprintf(
				C_("EXE|Runtime", "Microsoft Visual Basic %s Runtime"), "5.0");
			refLink = "https://download.microsoft.com/download/vb50pro/utility/1/win98/en-us/msvbvm50.exe";
			break;
		} else if (!strcmp(dll_name, "vbrun400.dll") || !strcmp(dll_name, "vbrun432.dll")) {
			// FIXME: Is it vbrun400.dll, vbrun432.dll, or both?
			refDesc = rp_sprintf(
				C_("EXE|Runtime", "Microsoft Visual Basic %s Runtime"), "4.0");
			// TODO: Find a download link.
			break;
		}
	}

	return (!refDesc.empty() ? 0 : -ENOENT);
}

/**
 * Add fields for PE and PE32+ executables.
 */
void EXEPrivate::addFields_PE(void)
{
	// Up to 4 tabs.
	fields->reserveTabs(4);

	// PE Header
	fields->setTabName(0, "PE");
	fields->setTabIndex(0);

	const uint16_t machine = le16_to_cpu(hdr.pe.FileHeader.Machine);
	const uint16_t pe_flags = le16_to_cpu(hdr.pe.FileHeader.Characteristics);

	// Get the architecture-specific fields.
	uint16_t os_ver_major, os_ver_minor;
	uint16_t subsystem_ver_major, subsystem_ver_minor;
	uint16_t dll_flags;
	bool dotnet;
	if (exeType == EXEPrivate::ExeType::PE) {
		os_ver_major = le16_to_cpu(hdr.pe.OptionalHeader.opt32.MajorOperatingSystemVersion);
		os_ver_minor = le16_to_cpu(hdr.pe.OptionalHeader.opt32.MinorOperatingSystemVersion);
		subsystem_ver_major = le16_to_cpu(hdr.pe.OptionalHeader.opt32.MajorSubsystemVersion);
		subsystem_ver_minor = le16_to_cpu(hdr.pe.OptionalHeader.opt32.MinorSubsystemVersion);
		dll_flags = le16_to_cpu(hdr.pe.OptionalHeader.opt32.DllCharacteristics);
		// TODO: Check VirtualAddress, Size, or both?
		// 'file' checks VirtualAddress.
		dotnet = (hdr.pe.OptionalHeader.opt32.DataDirectory[IMAGE_DATA_DIRECTORY_CLR_HEADER].Size != 0);
	} else /*if (exeType == EXEPrivate::ExeType::PE32PLUS)*/ {
		os_ver_major = le16_to_cpu(hdr.pe.OptionalHeader.opt64.MajorOperatingSystemVersion);
		os_ver_minor = le16_to_cpu(hdr.pe.OptionalHeader.opt64.MinorOperatingSystemVersion);
		subsystem_ver_major = le16_to_cpu(hdr.pe.OptionalHeader.opt64.MajorSubsystemVersion);
		subsystem_ver_minor = le16_to_cpu(hdr.pe.OptionalHeader.opt64.MinorSubsystemVersion);
		dll_flags = le16_to_cpu(hdr.pe.OptionalHeader.opt64.DllCharacteristics);
		// TODO: Check VirtualAddress, Size, or both?
		// 'file' checks VirtualAddress.
		dotnet = (hdr.pe.OptionalHeader.opt64.DataDirectory[IMAGE_DATA_DIRECTORY_CLR_HEADER].Size != 0);
	}

	// CPU. (Also .NET status.)
	string s_cpu;
	const char *const cpu = EXEData::lookup_pe_cpu(machine);
	if (cpu != nullptr) {
		s_cpu = cpu;
	} else {
		s_cpu = rp_sprintf(C_("RomData", "Unknown (0x%04X)"), machine);
	}
	if (dotnet) {
		// .NET executable.
		s_cpu += " (.NET)";
	}
	fields->addField_string(C_("EXE", "CPU"), s_cpu);

	// OS version.
	fields->addField_string(C_("EXE", "OS Version"),
		rp_sprintf("%u.%u", os_ver_major, os_ver_minor));

	// Subsystem names.
	static const char *const subsysNames[IMAGE_SUBSYSTEM_XBOX+1] = {
		// IMAGE_SUBSYSTEM_UNKNOWN
		nullptr,
		// tr: IMAGE_SUBSYSTEM_NATIVE
		NOP_C_("EXE|Subsystem", "Native"),
		// tr: IMAGE_SUBSYSTEM_WINDOWS_GUI
		NOP_C_("EXE|Subsystem", "Windows"),
		// tr: IMAGE_SUBSYSTEM_WINDOWS_CUI
		NOP_C_("EXE|Subsystem", "Console"),
		// Unused...
		nullptr,
		// tr: IMAGE_SUBSYSTEM_OS2_CUI
		NOP_C_("EXE|Subsystem", "OS/2 Console"),
		// Unused...
		nullptr,
		// tr: IMAGE_SUBSYSTEM_POSIX_CUI
		NOP_C_("EXE|Subsystem", "POSIX Console"),
		// tr: IMAGE_SUBSYSTEM_NATIVE_WINDOWS
		NOP_C_("EXE|Subsystem", "Win9x Native Driver"),
		// tr: IMAGE_SUBSYSTEM_WINDOWS_CE_GUI
		NOP_C_("EXE|Subsystem", "Windows CE"),
		// tr: IMAGE_SUBSYSTEM_EFI_APPLICATION
		NOP_C_("EXE|Subsystem", "EFI Application"),
		// tr: IMAGE_SUBSYSTEM_EFI_BOOT_SERVICE_DRIVER
		NOP_C_("EXE|Subsystem", "EFI Boot Service Driver"),
		// tr: IMAGE_SUBSYSTEM_EFI_RUNTIME_DRIVER
		NOP_C_("EXE|Subsystem", "EFI Runtime Driver"),
		// tr: IMAGE_SUBSYSTEM_EFI_ROM
		NOP_C_("EXE|Subsystem", "EFI ROM Image"),
		// tr: IMAGE_SUBSYSTEM_XBOX
		NOP_C_("EXE|Subsystem", "Xbox"),
	};

	// Subsystem name and version.
	string subsystem_name;
	if (pe_subsystem < ARRAY_SIZE(subsysNames) && subsysNames[pe_subsystem] != nullptr) {
		subsystem_name = rp_sprintf("%s %u.%u",
			dpgettext_expr(RP_I18N_DOMAIN, "EXE|Subsystem", subsysNames[pe_subsystem]),
			subsystem_ver_major, subsystem_ver_minor);
	} else {
		const char *const s_unknown = C_("RomData", "Unknown");
		if (pe_subsystem == IMAGE_SUBSYSTEM_UNKNOWN) {
			subsystem_name = rp_sprintf("%s %u.%u",
				s_unknown,
				subsystem_ver_major, subsystem_ver_minor);
		} else {
			subsystem_name = rp_sprintf("%s (%u) %u.%u",
				s_unknown, pe_subsystem,
				subsystem_ver_major, subsystem_ver_minor);
		}
	}
	fields->addField_string(C_("EXE", "Subsystem"), subsystem_name);

	// PE flags. (characteristics)
	// NOTE: Only important flags will be listed.
	static const char *const pe_flags_names[] = {
		nullptr,
		NOP_C_("EXE|PEFlags", "Executable"),
		nullptr, nullptr, nullptr,
		NOP_C_("EXE|PEFlags", ">2GB addressing"),
		nullptr, nullptr, nullptr,
		nullptr, nullptr, nullptr,
		nullptr,
		NOP_C_("EXE|PEFlags", "DLL"),
		nullptr, nullptr,
	};
	vector<string> *const v_pe_flags_names = RomFields::strArrayToVector_i18n(
		"EXE|PEFlags", pe_flags_names, ARRAY_SIZE(pe_flags_names));
	fields->addField_bitfield(C_("EXE", "PE Flags"),
		v_pe_flags_names, 3, pe_flags);

	// DLL flags. (characteristics)
	static const char *const dll_flags_names[] = {
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
	};
	vector<string> *const v_dll_flags_names = RomFields::strArrayToVector_i18n(
		"EXE|DLLFlags", dll_flags_names, ARRAY_SIZE(dll_flags_names));
	fields->addField_bitfield(C_("EXE", "DLL Flags"),
		v_dll_flags_names, 3, dll_flags);

	// Timestamp.
	// TODO: Windows 10 modules have hashes here instead of timestamps.
	// We should detect that by checking for obviously out-of-range values.
	// TODO: time_t is signed, so values greater than 2^31-1 may be negative.
	const char *const timestamp_title = C_("EXE", "Timestamp");
	uint32_t timestamp = le32_to_cpu(hdr.pe.FileHeader.TimeDateStamp);
	if (timestamp != 0) {
		fields->addField_dateTime(timestamp_title,
			static_cast<time_t>(timestamp),
			RomFields::RFT_DATETIME_HAS_DATE |
			RomFields::RFT_DATETIME_HAS_TIME);
	} else {
		fields->addField_string(timestamp_title, C_("EXE", "Not set"));
	}

	// Runtime DLL.
	string runtime_dll, runtime_link;
	int ret = findPERuntimeDLL(runtime_dll, runtime_link);
	if (ret == 0 && !runtime_dll.empty()) {
		// TODO: Show the link?
		fields->addField_string(C_("EXE", "Runtime DLL"), runtime_dll);
	}

	// Load resources.
	ret = loadPEResourceTypes();
	if (ret != 0 || !rsrcReader) {
		// Unable to load resources.
		// We're done here.
		return;
	}

	// Load the version resource.
	// NOTE: load_VS_VERSION_INFO loads it in host-endian.
	VS_FIXEDFILEINFO vsffi;
	IResourceReader::StringFileInfo vssfi;
	ret = rsrcReader->load_VS_VERSION_INFO(VS_VERSION_INFO, -1, &vsffi, &vssfi);
	if (ret != 0) {
		// Unable to load the version resource.
		// We're done here.
		return;
	}

	// Add the version fields.
	fields->setTabName(1, C_("EXE", "Version"));
	fields->setTabIndex(1);
	addFields_VS_VERSION_INFO(&vsffi, &vssfi);

#ifdef ENABLE_XML
	// Parse the manifest if it's present.
	// TODO: Support external manifests, e.g. program.exe.manifest?
	addFields_PE_Manifest();
#endif /* ENABLE_XML */
}

}
