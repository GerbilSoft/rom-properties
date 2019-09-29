/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * EXE_PE.cpp: DOS/Windows executable reader.                              *
 * 32-bit/64-bit Portable Executable format.                               *
 *                                                                         *
 * Copyright (c) 2016-2019 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "EXE_p.hpp"

#include "data/EXEData.hpp"
#include "disc/PEResourceReader.hpp"

// librpbase
#include "librpbase/common.h"
#include "librpbase/byteswap.h"
#include "librpbase/TextFuncs.hpp"
#include "librpbase/file/IRpFile.hpp"
#include "libi18n/i18n.h"
using namespace LibRpBase;

// C includes. (C++ namespace)
#include <cassert>
#include <cerrno>
#include <cstring>

// C++ includes.
#include <string>
#include <vector>
using std::string;
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
		case EXE_TYPE_PE:
			section_table_start += sizeof(IMAGE_NT_HEADERS32);
			SizeOfHeaders = le32_to_cpu(hdr.pe.OptionalHeader.opt32.SizeOfHeaders);
			break;
		case EXE_TYPE_PE32PLUS:
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
	for (unsigned int i = 0; i < static_cast<unsigned int>(pe_sections.size()); i++) {
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
	} else if (exeType != EXE_TYPE_PE && exeType != EXE_TYPE_PE32PLUS) {
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
		delete rsrcReader;
		rsrcReader = nullptr;
		return (err != 0 ? err : -EIO);
	}

	// .rsrc section loaded.
	return 0;
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
	if (exeType == EXEPrivate::EXE_TYPE_PE) {
		os_ver_major = le16_to_cpu(hdr.pe.OptionalHeader.opt32.MajorOperatingSystemVersion);
		os_ver_minor = le16_to_cpu(hdr.pe.OptionalHeader.opt32.MinorOperatingSystemVersion);
		subsystem_ver_major = le16_to_cpu(hdr.pe.OptionalHeader.opt32.MajorSubsystemVersion);
		subsystem_ver_minor = le16_to_cpu(hdr.pe.OptionalHeader.opt32.MinorSubsystemVersion);
		dll_flags = le16_to_cpu(hdr.pe.OptionalHeader.opt32.DllCharacteristics);
		// TODO: Check VirtualAddress, Size, or both?
		// 'file' checks VirtualAddress.
		dotnet = (hdr.pe.OptionalHeader.opt32.DataDirectory[IMAGE_DATA_DIRECTORY_CLR_HEADER].Size != 0);
	} else /*if (exeType == EXEPrivate::EXE_TYPE_PE32PLUS)*/ {
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
		nullptr,
		// tr: IMAGE_SUBSYSTEM_OS2_CUI
		NOP_C_("EXE|Subsystem", "OS/2 Console"),
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
	fields->addField_string(C_("EXE", "Subsystem"),
		rp_sprintf("%s %u.%u",
			(pe_subsystem < ARRAY_SIZE(subsysNames)
				? dpgettext_expr(RP_I18N_DOMAIN, "EXE|Subsystem", subsysNames[pe_subsystem])
				: C_("RomData", "Unknown")),
			subsystem_ver_major, subsystem_ver_minor));

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

	// Load resources.
	int ret = loadPEResourceTypes();
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
