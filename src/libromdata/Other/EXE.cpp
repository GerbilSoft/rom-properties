/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * EXE.cpp: DOS/Windows executable reader.                                 *
 *                                                                         *
 * Copyright (c) 2016-2018 by David Korth.                                 *
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

#include "EXE.hpp"

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

// EXE data.
#include "data/EXEData.hpp"

// Private class.
// NOTE: Must be #included after C++ headers due to uvector.h.
#include "EXE_p.hpp"

namespace LibRomData {

ROMDATA_IMPL(EXE)

/** EXEPrivate **/

// NE target OSes.
// Also used for LE.
const char *const EXEPrivate::NE_TargetOSes[] = {
	nullptr,			// NE_OS_UNKNOWN
	"IBM OS/2",			// NE_OS_OS2
	"Microsoft Windows",		// NE_OS_WIN
	"European MS-DOS 4.x",		// NE_OS_DOS4
	"Microsoft Windows (386)",	// NE_OS_WIN386 (TODO)
	"Borland Operating System Services",	// NE_OS_BOSS
};

EXEPrivate::EXEPrivate(EXE *q, IRpFile *file)
	: super(q, file)
	, exeType(EXE_TYPE_UNKNOWN)
	, rsrcReader(nullptr)
	, pe_subsystem(IMAGE_SUBSYSTEM_UNKNOWN)
{
	// Clear the structs.
	memset(&mz, 0, sizeof(mz));
	memset(&hdr, 0, sizeof(hdr));
}

EXEPrivate::~EXEPrivate()
{
	delete rsrcReader;
}

/**
 * Add VS_VERSION_INFO fields.
 *
 * NOTE: A subtab is NOT created here; if one is desired,
 * create and set it before calling this function.
 *
 * @param pVsFfi	[in] VS_FIXEDFILEINFO (host-endian)
 * @param pVsSfi	[in,opt] IResourceReader::StringFileInfo
 */
void EXEPrivate::addFields_VS_VERSION_INFO(const VS_FIXEDFILEINFO *pVsFfi, const IResourceReader::StringFileInfo *pVsSfi)
{
	// Reference: https://msdn.microsoft.com/en-us/library/windows/desktop/ms646997(v=vs.85).aspx
	assert(pVsFfi != nullptr);
	if (!pVsFfi)
		return;

	// File version.
	fields->addField_string(C_("EXE", "File Version"),
		rp_sprintf("%u.%u.%u.%u",
			pVsFfi->dwFileVersionMS >> 16,
			pVsFfi->dwFileVersionMS & 0xFFFF,
			pVsFfi->dwFileVersionLS >> 16,
			pVsFfi->dwFileVersionLS & 0xFFFF));

	// Product version.
	fields->addField_string(C_("EXE", "Product Version"),
		rp_sprintf("%u.%u.%u.%u",
			pVsFfi->dwProductVersionMS >> 16,
			pVsFfi->dwProductVersionMS & 0xFFFF,
			pVsFfi->dwProductVersionLS >> 16,
			pVsFfi->dwProductVersionLS & 0xFFFF));

	// File flags.
	static const char *const FileFlags_names[] = {
		NOP_C_("EXE|FileFlags", "Debug"),
		NOP_C_("EXE|FileFlags", "Prerelease"),
		NOP_C_("EXE|FileFlags", "Patched"),
		NOP_C_("EXE|FileFlags", "Private Build"),
		NOP_C_("EXE|FileFlags", "Info Inferred"),
		NOP_C_("EXE|FileFlags", "Special Build"),
	};
	vector<string> *v_FileFlags_names = RomFields::strArrayToVector_i18n(
		"EXE|FileFlags", FileFlags_names, ARRAY_SIZE(FileFlags_names));
	fields->addField_bitfield(C_("EXE", "File Flags"),
		v_FileFlags_names, 3, pVsFfi->dwFileFlags & pVsFfi->dwFileFlagsMask);

	// File OS.
	const char *file_os;
	switch (pVsFfi->dwFileOS) {
		case VOS_DOS:
			file_os = "MS-DOS";
			break;
		case VOS_NT:
			file_os = "Windows NT";
			break;
		case VOS__WINDOWS16:
			file_os = "Windows (16-bit)";
			break;
		case VOS__WINDOWS32:
			file_os = "Windows (32-bit)";
			break;
		case VOS_OS216:
			file_os = "OS/2 (16-bit)";
			break;
		case VOS_OS232:
			file_os = "OS/2 (32-bit)";
			break;
		case VOS__PM16:
			file_os = "Presentation Manager (16-bit)";
			break;
		case VOS__PM32:
			file_os = "Presentation Manager (32-bit)";
			break;

		case VOS_DOS_WINDOWS16:
			file_os = "Windows on MS-DOS (16-bit)";
			break;
		case VOS_DOS_WINDOWS32:
			file_os = "Windows 9x (32-bit)";
			break;
		case VOS_NT_WINDOWS32:
			file_os = "Windows NT";
			break;
		case VOS_OS216_PM16:
			file_os = "OS/2 with Presentation Manager (16-bit)";
			break;
		case VOS_OS232_PM32:
			file_os = "OS/2 with Presentation Manager (32-bit)";
			break;

		case VOS_UNKNOWN:
		default:
			file_os = nullptr;
			break;
	}

	if (file_os) {
		fields->addField_string(C_("EXE", "File OS"), file_os);
	} else {
		fields->addField_string(C_("EXE", "File OS"),
			rp_sprintf(C_("EXE", "Unknown (0x%08X)"), pVsFfi->dwFileOS));
	}

	// File type.
	static const char *const fileTypes[] = {
		// VFT_UNKNOWN
		nullptr,
		// tr: VFT_APP
		NOP_C_("EXE|FileType", "Application"),
		// tr: VFT_DLL
		NOP_C_("EXE|FileType", "DLL"),
		// tr: VFT_DRV
		NOP_C_("EXE|FileType", "Device Driver"),
		// tr: VFT_FONT
		NOP_C_("EXE|FileType", "Font"),
		// tr: VFT_VXD
		NOP_C_("EXE|FileType", "Virtual Device Driver"),
		nullptr,
		// tr: VFT_STATIC_LIB
		NOP_C_("EXE|FileType", "Static Library"),
	};
	const char *fileType = (pVsFfi->dwFileType < ARRAY_SIZE(fileTypes)
					? fileTypes[pVsFfi->dwFileType]
					: nullptr);
	if (fileType) {
		fields->addField_string(C_("EXE", "File Type"),
			dpgettext_expr(RP_I18N_DOMAIN, "EXE|FileType", fileType));
	} else {
		fields->addField_string(C_("EXE", "File Type"),
			rp_sprintf(C_("EXE", "Unknown (0x%08X)"), pVsFfi->dwFileType));
	}

	// File subtype.
	bool hasSubtype = false;
	const char *fileSubtype = nullptr;
	switch (pVsFfi->dwFileType) {
		case VFT_DRV: {
			hasSubtype = true;
			static const char *const fileSubtypes_DRV[] = {
				// VFT2_UNKNOWN
				nullptr,
				// tr: VFT2_DRV_PRINTER
				NOP_C_("EXE|FileSubType", "Printer"),
				// tr: VFT2_DRV_KEYBOARD
				NOP_C_("EXE|FileSubType", "Keyboard"),
				// tr: VFT2_DRV_LANGUAGE
				NOP_C_("EXE|FileSubType", "Language"),
				// tr: VFT2_DRV_DISPLAY
				NOP_C_("EXE|FileSubType", "Display"),
				// tr: VFT2_DRV_MOUSE
				NOP_C_("EXE|FileSubType", "Mouse"),
				// tr: VFT2_DRV_NETWORK
				NOP_C_("EXE|FileSubType", "Network"),
				// tr: VFT2_DRV_SYSTEM
				NOP_C_("EXE|FileSubType", "System"),
				// tr: VFT2_DRV_INSTALLABLE
				NOP_C_("EXE|FileSubType", "Installable"),
				// tr: VFT2_DRV_SOUND
				NOP_C_("EXE|FileSubType", "Sound"),
				// tr: VFT2_DRV_COMM
				NOP_C_("EXE|FileSubType", "Communications"),
				// tr: VFT2_DRV_INPUTMETHOD
				NOP_C_("EXE|FileSubType", "Input Method"),
				// tr: VFT2_DRV_VERSIONED_PRINTER
				NOP_C_("EXE|FileSubType", "Versioned Printer"),
			};
			fileSubtype = (pVsFfi->dwFileSubtype < ARRAY_SIZE(fileSubtypes_DRV)
						? fileSubtypes_DRV[pVsFfi->dwFileSubtype]
						: nullptr);
			break;
		};

		case VFT_FONT: {
			hasSubtype = true;
			static const char *const fileSubtypes_FONT[] = {
				// VFT2_UNKNOWN
				nullptr,
				// tr: VFT2_FONT_RASTER
				NOP_C_("EXE|FileSubType", "Raster"),
				// tr: VFT2_FONT_VECTOR
				NOP_C_("EXE|FileSubType", "Vector"),
				// tr: VFT2_FONT_TRUETYPE
				NOP_C_("EXE|FileSubType", "TrueType"),
			};
			fileSubtype = (pVsFfi->dwFileSubtype < ARRAY_SIZE(fileSubtypes_FONT)
						? fileSubtypes_FONT[pVsFfi->dwFileSubtype]
						: nullptr);
			break;
		};

		default:
			break;
	}

	if (hasSubtype) {
		if (fileSubtype) {
			fields->addField_string(C_("EXE", "File Subtype"),
				dpgettext_expr(RP_I18N_DOMAIN, "EXE|FileSubType", fileSubtype));
		} else {
			fields->addField_string(C_("EXE", "File Subtype"),
				rp_sprintf(C_("EXE", "Unknown (0x%02X)"), pVsFfi->dwFileSubtype));
		}
	}

	// File timestamp. (FILETIME format)
	// NOTE: This seems to be 0 in most EXEs and DLLs I've tested.
	uint64_t fileTime = ((uint64_t)pVsFfi->dwFileDateMS) << 32 |
			     (uint64_t)pVsFfi->dwFileDateLS;
	if (fileTime != 0) {
		// Convert to UNIX time.
#ifndef FILETIME_1970
		#define FILETIME_1970 116444736000000000LL	// Seconds between 1/1/1601 and 1/1/1970.
#endif
#ifndef HECTONANOSEC_PER_SEC
		#define HECTONANOSEC_PER_SEC 10000000LL
#endif
		time_t fileTimeUnix = (time_t)((fileTime - FILETIME_1970) / HECTONANOSEC_PER_SEC);
		fields->addField_dateTime(C_("EXE", "File Time"), fileTimeUnix,
			RomFields::RFT_DATETIME_HAS_DATE |
			RomFields::RFT_DATETIME_HAS_TIME
			);
	}

	// Was a StringFileInfo table loaded?
	if (!pVsSfi || pVsSfi->empty()) {
		// Not loaded.
		return;
	}

	// TODO: Show the language that most closely matches the system.
	// For now, only showing the "first" language, which may be
	// random due to unordered_map<>.
	// TODO: Show certain entries as their own fields?
	const auto &st = pVsSfi->begin()->second;
	auto data = new vector<vector<string> >();
	data->resize(st.size());
	for (unsigned int i = 0; i < (unsigned int)st.size(); i++) {
		const auto &st_row = st.at(i);
		auto &data_row = data->at(i);
		data_row.reserve(2);
		data_row.push_back(st_row.first);
		data_row.push_back(st_row.second);
	}

	// Fields.
	static const char *const field_names[] = {
		"Key", "Value"
	};
	vector<string> *v_field_names = RomFields::strArrayToVector(
		field_names, ARRAY_SIZE(field_names));

	// Add the StringFileInfo.
	fields->addField_listData("StringFileInfo", v_field_names, data);
}

/** MZ-specific **/

/**
 * Add fields for MZ executables.
 */
void EXEPrivate::addFields_MZ(void)
{
	// Program image size
	uint32_t program_size = le16_to_cpu(mz.e_cp) * 512;
	if (mz.e_cblp != 0) {
		program_size -= 512 - le16_to_cpu(mz.e_cblp);
	}
	fields->addField_string(C_("EXE", "Program Size"), formatFileSize(program_size));

	// Min/Max allocated memory
	fields->addField_string(C_("EXE", "Min. Memory"), formatFileSize(le16_to_cpu(mz.e_minalloc) * 16));
	fields->addField_string(C_("EXE", "Max. Memory"), formatFileSize(le16_to_cpu(mz.e_maxalloc) * 16));

	// Initial CS:IP/SS:SP
	fields->addField_string(C_("EXE", "Initial CS:IP"),
		rp_sprintf("%04X:%04X", le16_to_cpu(mz.e_cs), le16_to_cpu(mz.e_ip)), RomFields::STRF_MONOSPACE);
	fields->addField_string(C_("EXE", "Initial SS:SP"),
		rp_sprintf("%04X:%04X", le16_to_cpu(mz.e_ss), le16_to_cpu(mz.e_sp)), RomFields::STRF_MONOSPACE);
}

/** NE-specific **/

/**
 * Load the NE resource table.
 * @return 0 on success; negative POSIX error code on error. (-ENOENT if not found)
 */
int EXEPrivate::loadNEResourceTable(void)
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
	} else if (exeType != EXE_TYPE_NE) {
		// Unsupported executable type.
		return -ENOTSUP;
	}

	// NE resource table offset is relative to the start of the NE header.
	// It must be >= the size of the NE header.
	uint32_t ResTableOffset = le16_to_cpu(hdr.ne.ResTableOffset);
	if (ResTableOffset < sizeof(NE_Header)) {
		// Resource table cannot start in the middle of the NE header.
		return -EIO;
	}

	// Check for the lowest non-zero offset after ResTableOffset.
	// This will be used for the resource table size.
	uint32_t maxOffset = 0;
	uint32_t resTableSize = 0;
	{
		#define TABLE_CHECK(var) do { \
			const uint32_t var = le16_to_cpu(hdr.ne.var); \
			if (var != 0 && var > ResTableOffset && \
			    (maxOffset == 0 || var < maxOffset)) \
			{ \
				maxOffset = var; \
			} \
		} while (0)
		TABLE_CHECK(SegTableOffset);
		TABLE_CHECK(ResidNamTable);
		TABLE_CHECK(ModRefTable);
		TABLE_CHECK(ImportNameTable);
		// TODO: OffStartNonResTab is from the start of the file.
		// Not sure if we need to check it.

		if (maxOffset != 0 && maxOffset > ResTableOffset) {
			// Found a valid maximum.
			resTableSize = maxOffset - ResTableOffset;
		}
	}

	// Adjust ResTableOffset to make it an absolute address.
	ResTableOffset += le16_to_cpu(mz.e_lfanew);

	// Check if a size is known.
	if (resTableSize == 0) {
		// Not known. Go with the file size.
		// NOTE: Limited to 32-bit file sizes.
		resTableSize = (uint32_t)file->size() - ResTableOffset;
	}

	// Load the resources using NEResourceReader.
	rsrcReader = new NEResourceReader(file, ResTableOffset, resTableSize);
	if (!rsrcReader->isOpen()) {
		// Failed to open the resource table.
		int err = rsrcReader->lastError();
		delete rsrcReader;
		rsrcReader = nullptr;
		return (err != 0 ? err : -EIO);
	}

	// Resource table loaded.
	return 0;
}

/**
 * Add fields for NE executables.
 */
void EXEPrivate::addFields_NE(void)
{
	// Up to 3 tabs.
	fields->reserveTabs(3);

	// NE Header
	fields->setTabName(0, C_("EXE", "NE Header"));
	fields->setTabIndex(0);

	// Target OS.
	const char *targetOS = (hdr.ne.targOS < ARRAY_SIZE(NE_TargetOSes))
					? NE_TargetOSes[hdr.ne.targOS]
					: nullptr;
	if (!targetOS) {
		// Check for Phar Lap extenders.
		if (hdr.ne.targOS == NE_OS_PHARLAP_286_OS2) {
			targetOS = NE_TargetOSes[NE_OS_OS2];
		} else if (hdr.ne.targOS == NE_OS_PHARLAP_286_WIN) {
			targetOS = NE_TargetOSes[NE_OS_WIN];
		}
	}

	if (targetOS) {
		fields->addField_string(C_("EXE", "Target OS"), targetOS);
	} else {
		fields->addField_string(C_("EXE", "Target OS"),
			rp_sprintf(C_("EXE", "Unknown (0x%02X)"), hdr.ne.targOS));
	}

	// DGroup type.
	static const char *const dgroupTypes[] = {
		NOP_C_("EXE|DGroupType", "None"),
		NOP_C_("EXE|DGroupType", "Single Shared"),
		NOP_C_("EXE|DGroupType", "Multiple"),
		NOP_C_("EXE|DGroupType", "(null)"),
	};
	fields->addField_string(C_("EXE", "DGroup Type"),
		dpgettext_expr(RP_I18N_DOMAIN, "EXE|DGroupType", dgroupTypes[hdr.ne.ProgFlags & 3]));

	// Program flags.
	static const char *const ProgFlags_names[] = {
		nullptr, nullptr,	// DGroup Type
		NOP_C_("EXE|ProgFlags", "Global Init"),
		NOP_C_("EXE|ProgFlags", "Protected Mode Only"),
		NOP_C_("EXE|ProgFlags", "8086 insns"),
		NOP_C_("EXE|ProgFlags", "80286 insns"),
		NOP_C_("EXE|ProgFlags", "80386 insns"),
		NOP_C_("EXE|ProgFlags", "FPU insns"),
	};
	vector<string> *v_ProgFlags_names = RomFields::strArrayToVector_i18n(
		"EXE|ProgFlags", ProgFlags_names, ARRAY_SIZE(ProgFlags_names));
	fields->addField_bitfield("Program Flags",
		v_ProgFlags_names, 2, hdr.ne.ProgFlags);

	// Application type.
	const char *applType;
	if (hdr.ne.targOS == NE_OS_OS2) {
		// Only mentioning Presentation Manager for OS/2 executables.
		static const char *const applTypes_OS2[] = {
			NOP_C_("EXE|ApplType", "None"),
			NOP_C_("EXE|ApplType", "Full Screen (not aware of Presentation Manager)"),
			NOP_C_("EXE|ApplType", "Presentation Manager compatible"),
			NOP_C_("EXE|ApplType", "Presentation Manager application"),
		};
		applType = applTypes_OS2[hdr.ne.ApplFlags & 3];
	} else {
		// Assume Windows for everything else.
		static const char *const applTypes_Win[] = {
			NOP_C_("EXE|ApplType", "None"),
			NOP_C_("EXE|ApplType", "Full Screen (not aware of Windows)"),
			NOP_C_("EXE|ApplType", "Windows compatible"),
			NOP_C_("EXE|ApplType", "Windows application"),
		};
		applType = applTypes_Win[hdr.ne.ApplFlags & 3];
	}
	fields->addField_string(C_("EXE", "Application Type"),
		dpgettext_expr(RP_I18N_DOMAIN, "EXE|ApplType", applType));

	// Application flags.
	static const char *const ApplFlags_names[] = {
		nullptr, nullptr,	// Application type
		nullptr,
		NOP_C_("EXE|ApplFlags", "OS/2 Application"),
		nullptr,
		NOP_C_("EXE|ApplFlags", "Image Error"),
		NOP_C_("EXE|ApplFlags", "Non-Conforming"),
		NOP_C_("EXE|ApplFlags", "DLL"),
	};
	vector<string> *v_ApplFlags_names = RomFields::strArrayToVector_i18n(
		"EXE|ApplFlags", ApplFlags_names, ARRAY_SIZE(ApplFlags_names));
	fields->addField_bitfield(C_("EXE", "Application Flags"),
		v_ApplFlags_names, 2, hdr.ne.ApplFlags);

	// Other flags.
	// NOTE: Indicated as OS/2 flags by OSDev Wiki,
	// but may be set on Windows programs too.
	// References:
	// - http://wiki.osdev.org/NE
	// - http://www.program-transformation.org/Transform/PcExeFormat
	static const char *const OtherFlags_names[] = {
		NOP_C_("EXE|OtherFlags", "Long File Names"),
		NOP_C_("EXE|OtherFlags", "Protected Mode"),
		NOP_C_("EXE|OtherFlags", "Proportional Fonts"),
		NOP_C_("EXE|OtherFlags", "Gangload Area"),
	};
	vector<string> *v_OtherFlags_names = RomFields::strArrayToVector_i18n(
		"EXE|OtherFlags", OtherFlags_names, ARRAY_SIZE(OtherFlags_names));
	fields->addField_bitfield(C_("EXE", "Other Flags"),
		v_OtherFlags_names, 2, hdr.ne.OS2EXEFlags);

	// Expected Windows version.
	// TODO: Is this used in OS/2 executables?
	if (hdr.ne.targOS == NE_OS_WIN || hdr.ne.targOS == NE_OS_WIN386) {
		fields->addField_string(C_("EXE", "Windows Version"),
			rp_sprintf("%u.%u", hdr.ne.expctwinver[1], hdr.ne.expctwinver[0]));
	}

	// Load resources.
	int ret = loadNEResourceTable();
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
}

/** LE/LX-specific **/

/**
 * Add fields for LE/LX executables.
 */
void EXEPrivate::addFields_LE(void)
{
	// TODO: Handle fields that indicate byteorder.
	// Currently assuming little-endian.

	// Up to 2 tabs.
	fields->reserveTabs(2);

	// LE Header
	fields->setTabName(0, C_("EXE", "LE Header"));
	fields->setTabIndex(0);

	// CPU.
	const uint16_t cpu_type = le16_to_cpu(hdr.le.cpu_type);
	const char *const cpu = EXEData::lookup_le_cpu(cpu_type);
	if (cpu) {
		fields->addField_string(C_("EXE", "CPU"), cpu);
	} else {
		fields->addField_string(C_("EXE", "CPU"),
			rp_sprintf(C_("EXE", "Unknown (0x%04X)"), cpu_type));
	}

	// Target OS.
	// NOTE: Same as NE.
	const uint16_t targOS = le16_to_cpu(hdr.le.targOS);
	const char *const targetOS = (targOS < ARRAY_SIZE(NE_TargetOSes))
					? NE_TargetOSes[targOS]
					: nullptr;
	if (targetOS) {
		fields->addField_string(C_("EXE", "Target OS"), targetOS);
	} else {
		fields->addField_string(C_("EXE", "Target OS"),
			rp_sprintf(C_("EXE", "Unknown (0x%02X)"), targOS));
	}
}

/** PE-specific **/

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
	uint32_t szToRead = (uint32_t)(section_count * sizeof(IMAGE_SECTION_HEADER));
	size_t size = file->seekAndRead(section_table_start, pe_sections.data(), szToRead);
	if (size != (size_t)szToRead) {
		// Seek and/or read error.
		pe_sections.clear();
		return -EIO;
	}

	// Not all sections may be in use.
	// Find the first section header with an empty name.
	int ret = 0;
	for (unsigned int i = 0; i < (unsigned int)pe_sections.size(); i++) {
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
	const IMAGE_SECTION_HEADER *rsrc = nullptr;
	for (int i = (int)pe_sections.size()-1; i >= 0; i--) {
		const IMAGE_SECTION_HEADER *section = &pe_sections[i];
		if (!strcmp(section->Name, ".rsrc")) {
			// Found the .rsrc section.
			rsrc = section;
			break;
		}
	}

	if (!rsrc) {
		// No .rsrc section.
		return -ENOENT;
	}

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
	fields->setTabName(0, C_("EXE", "PE Header"));
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
		s_cpu = rp_sprintf(C_("EXE", "Unknown (0x%04X)"), machine);
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
				: C_("EXE", "Unknown")),
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
	vector<string> *v_pe_flags_names = RomFields::strArrayToVector_i18n(
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
	vector<string> *v_dll_flags_names = RomFields::strArrayToVector_i18n(
		"EXE|DLLFlags", dll_flags_names, ARRAY_SIZE(dll_flags_names));
	fields->addField_bitfield(C_("EXE", "DLL Flags"),
		v_dll_flags_names, 3, dll_flags);

	// Timestamp.
	// TODO: Windows 10 modules have hashes here instead of timestamps.
	// We should detect that by checking for obviously out-of-range values.
	// TODO: time_t is signed, so values greater than 2^31-1 may be negative.
	uint32_t timestamp = le32_to_cpu(hdr.pe.FileHeader.TimeDateStamp);
	if (timestamp != 0) {
		fields->addField_dateTime(C_("EXE", "Timestamp"),
			(time_t)timestamp,
			RomFields::RFT_DATETIME_HAS_DATE |
			RomFields::RFT_DATETIME_HAS_TIME);
	} else {
		fields->addField_string(C_("EXE", "Timestamp"), C_("EXE", "Not set"));
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

/** EXE **/

/**
 * Read a DOS/Windows executable.
 *
 * A ROM file must be opened by the caller. The file handle
 * will be dup()'d and must be kept open in order to load
 * data from the ROM.
 *
 * To close the file, either delete this object or call close().
 *
 * NOTE: Check isValid() to determine if this is a valid ROM.
 *
 * @param file Open ROM image.
 */
EXE::EXE(IRpFile *file)
	: super(new EXEPrivate(this, file))
{
	// This class handles different types of files.
	// d->fileType will be set later.
	RP_D(EXE);
	d->className = "EXE";
	d->fileType = FTYPE_UNKNOWN;

	if (!d->file) {
		// Could not dup() the file handle.
		return;
	}

	// Read the DOS MZ header.
	d->file->rewind();
	size_t size = d->file->read(&d->mz, sizeof(d->mz));
	if (size != sizeof(d->mz))
		return;

	// Check if this executable is supported.
	DetectInfo info;
	info.header.addr = 0;
	info.header.size = sizeof(d->mz);
	info.header.pData = reinterpret_cast<const uint8_t*>(&d->mz);
	info.ext = nullptr;	// Not needed for EXE.
	info.szFile = 0;	// Not needed for EXE.
	d->isValid = (isRomSupported_static(&info) >= 0);

	if (!d->isValid) {
		// Not an MZ executable.
		return;
	}

	// NOTE: isRomSupported_static() only determines if the
	// file has a DOS MZ executable stub. The actual executable
	// type is determined here.

	// Check for MS-DOS executables:
	// - Relocation table address less than 0x40
	// - Magic number is 'ZM' (Windows only accepts 'MZ')
	if (le16_to_cpu(d->mz.e_lfarlc) < 0x40 ||
	    d->mz.e_magic == cpu_to_be16('ZM')) {
		// MS-DOS executable.
		d->exeType = EXEPrivate::EXE_TYPE_MZ;
		// TODO: Check for MS-DOS device drivers?
		d->fileType = FTYPE_EXECUTABLE;
		return;
	}

	// Load the secondary header. (NE/LE/LX/PE)
	// TODO: LE/LX.
	// NOTE: NE and PE secondary headers are both 64 bytes.
	uint32_t hdr_addr = le32_to_cpu(d->mz.e_lfanew);
	if (hdr_addr < sizeof(d->mz) || hdr_addr >= (d->file->size() - sizeof(d->hdr))) {
		// PE header address is out of range.
		d->exeType = EXEPrivate::EXE_TYPE_MZ;
		return;
	}

	size = d->file->seekAndRead(hdr_addr, &d->hdr, sizeof(d->hdr));
	if (size != sizeof(d->hdr)) {
		// Seek and/or read error.
		// TODO: Check the signature first instead of
		// depending on the full union being available?
		d->exeType = EXEPrivate::EXE_TYPE_UNKNOWN;
		d->isValid = false;
		return;
	}

	// Check the signature.
	// FIXME: MSVC handles 'PE\0\0' as 0x00504500,
	// probably due to the embedded NULL bytes.
	if (d->hdr.pe.Signature == cpu_to_be32(0x50450000) /*'PE\0\0'*/) {
		// This is a PE executable.
		// Check if it's PE or PE32+.
		// (.NET is checked in loadFieldData().)
		switch (le16_to_cpu(d->hdr.pe.OptionalHeader.Magic)) {
			case IMAGE_NT_OPTIONAL_HDR32_MAGIC:
				d->exeType = EXEPrivate::EXE_TYPE_PE;
				d->pe_subsystem = le16_to_cpu(d->hdr.pe.OptionalHeader.opt32.Subsystem);
				break;
			case IMAGE_NT_OPTIONAL_HDR64_MAGIC:
				d->exeType = EXEPrivate::EXE_TYPE_PE32PLUS;
				d->pe_subsystem = le16_to_cpu(d->hdr.pe.OptionalHeader.opt64.Subsystem);
				break;
			default:
				// Unsupported PE executable.
				d->exeType = EXEPrivate::EXE_TYPE_UNKNOWN;
				d->isValid = false;
				return;
		}

		// Check the file type.
		const uint16_t pe_flags = le16_to_cpu(d->hdr.pe.FileHeader.Characteristics);
		if (pe_flags & IMAGE_FILE_DLL) {
			// DLL file.
			d->fileType = FTYPE_DLL;
		} else {
			switch (d->pe_subsystem) {
				case IMAGE_SUBSYSTEM_NATIVE:
					// TODO: IMAGE_SUBSYSTEM_NATIVE may be either a
					// device driver or boot-time executable.
					// Need to check some other flag...
					d->fileType = FTYPE_EXECUTABLE;
					break;
				case IMAGE_SUBSYSTEM_EFI_BOOT_SERVICE_DRIVER:
				case IMAGE_SUBSYSTEM_EFI_RUNTIME_DRIVER:
					d->fileType = FTYPE_DEVICE_DRIVER;
					break;
				case IMAGE_SUBSYSTEM_EFI_ROM:
					d->fileType = FTYPE_ROM_IMAGE;
					break;
				default:
					d->fileType = FTYPE_EXECUTABLE;
					break;
			}
		}
	} else if (d->hdr.ne.sig == cpu_to_be16('NE') /* 'NE' */) {
		// New Executable.
		d->exeType = EXEPrivate::EXE_TYPE_NE;

		// Check if this is a resource library.
		// (All segment size values are 0.)
		// NOTE: AutoDataSegIndex is 0 for .FON, but 1 for MORICONS.DLL.
		// FIXME: ULFONT.FON has non-zero values.

		// Check 0x10-0x1F for all 0s.
		const uint32_t *res0chk = reinterpret_cast<const uint32_t*>(&d->hdr.ne.InitHeapSize);
		if (res0chk[0] == 0 && res0chk[1] == 0 &&
		    res0chk[2] == 0 && res0chk[3] == 0)
		{
			// This is a resource library.
			// May be a font (.FON) or an icon library (.ICL, moricons.dll).
			// TODO: Check the version resource if it's present?
			d->fileType = FTYPE_RESOURCE_LIBRARY;
			return;
		}

		// TODO: Distinguish between DLL and driver?
		if (d->hdr.ne.ApplFlags & NE_DLL) {
			d->fileType = FTYPE_DLL;
		} else {
			d->fileType = FTYPE_EXECUTABLE;
		}
	} else if (d->hdr.le.sig == cpu_to_be16('LE') ||
		   d->hdr.le.sig == cpu_to_be16('LX'))
	{
		// Linear Executable.
		if (d->hdr.le.sig == cpu_to_be16('LE')) {
			d->exeType = EXEPrivate::EXE_TYPE_LE;
		} else /*if (d->hdr.le.sig == cpu_to_be16('LX'))*/ {
			d->exeType = EXEPrivate::EXE_TYPE_LX;
		}

		// TODO: Check byteorder flags and adjust as necessary.
		if (d->hdr.le.targOS == cpu_to_le16(NE_OS_WIN386)) {
			// LE VxD
			d->fileType = FTYPE_DEVICE_DRIVER;
		} else if (d->hdr.le.module_type_flags & cpu_to_le32(LE_MODULE_IS_DLL)) {
			// LE DLL
			d->fileType = FTYPE_DLL;
		} else {
			// LE EXE
			d->fileType = FTYPE_EXECUTABLE;
		}
	} else if (d->hdr.sig16 == cpu_to_be16('W3') /* 'W3' */) {
		// W3 executable. (Collection of LE executables.)
		// Only used by WIN386.EXE.
		// TODO: Check for W4. (Compressed version of W3 used by Win9x.)
		d->exeType = EXEPrivate::EXE_TYPE_W3;
		d->fileType = FTYPE_EXECUTABLE;
	} else {
		// Unrecognized secondary header.
		d->exeType = EXEPrivate::EXE_TYPE_MZ;
		d->fileType = FTYPE_EXECUTABLE;
		return;
	}
}

/** ROM detection functions. **/

/**
 * Is a ROM image supported by this class?
 * @param info DetectInfo containing ROM detection information.
 * @return Class-specific system ID (>= 0) if supported; -1 if not.
 */
int EXE::isRomSupported_static(const DetectInfo *info)
{
	assert(info != nullptr);
	assert(info->header.pData != nullptr);
	assert(info->header.addr == 0);
	if (!info || !info->header.pData ||
	    info->header.addr != 0 ||
	    info->header.size < sizeof(IMAGE_DOS_HEADER))
	{
		// Either no detection information was specified,
		// or the header is too small.
		return -1;
	}

	const IMAGE_DOS_HEADER *const pMZ =
		reinterpret_cast<const IMAGE_DOS_HEADER*>(info->header.pData);

	// Check the magic number.
	// This may be either 'MZ' or 'ZM'. ('ZM' is less common.)
	// NOTE: 'ZM' can only be used for MS-DOS executables.
	// NOTE: Checking for little-endian host byteorder first.
	if (pMZ->e_magic == 'ZM' || pMZ->e_magic == 'MZ') {
		// This is a DOS "MZ" executable.
		// Specific subtypes are checked later.
		return EXEPrivate::EXE_TYPE_MZ;
	}

	// Not supported.
	return -1;
}

/**
 * Get the name of the system the loaded ROM is designed for.
 * @param type System name type. (See the SystemName enum.)
 * @return System name, or nullptr if type is invalid.
 */
const char *EXE::systemName(unsigned int type) const
{
	RP_D(const EXE);
	if (!d->isValid || !isSystemNameTypeValid(type))
		return nullptr;

	static const char *const sysNames_Windows[4] = {
		"Microsoft Windows", "Windows", "Windows", nullptr
	};

	// New Executable (and Linear Executable) operating systems.
	static const char *const sysNames_NE[6][4] = {
		{nullptr, nullptr, nullptr, nullptr},				// NE_OS_UNKNOWN
		{"IBM OS/2", "OS/2", "OS/2", nullptr},				// NE_OS_OS2
		{"Microsoft Windows", "Windows", "Windows", nullptr},		// NE_OS_WIN
		{"European MS-DOS 4.x", "EuroDOS 4.x", "EuroDOS 4.x", nullptr},	// NE_OS_DOS4
		{"Microsoft Windows", "Windows", "Windows", nullptr},		// NE_OS_WIN386 (TODO)
		{"Borland Operating System Services", "BOSS", "BOSS", nullptr},	// NE_OS_BOSS
	};

	switch (d->exeType) {
		case EXEPrivate::EXE_TYPE_MZ: {
			// DOS executable.
			static const char *const sysNames_DOS[4] = {
				"Microsoft MS-DOS", "MS-DOS", "DOS", nullptr
			};
			return sysNames_DOS[type & SYSNAME_TYPE_MASK];
		}

		case EXEPrivate::EXE_TYPE_NE: {
			// New Executable.
			if (d->hdr.ne.targOS > NE_OS_BOSS) {
				// Check for Phar Lap 286 extenders.
				// Reference: https://github.com/weheartwebsites/exeinfo/blob/master/exeinfo.cpp
				static const char *const sysNames_NE_PharLap[2][4] = {
					{"Phar Lap 286|DOS Extender, OS/2", "Phar Lap 286 OS/2", "Phar Lap 286 OS/2", nullptr},	// 0x81
					{"Phar Lap 286|DOS Extender, Windows", "Phar Lap 286 Windows", "Phar Lap 286 Windows", nullptr},	// 0x82
				};
				if (d->hdr.ne.targOS == 0x81) {
					return sysNames_NE_PharLap[0][type & SYSNAME_TYPE_MASK];
				} else if (d->hdr.ne.targOS == 0x82) {
					return sysNames_NE_PharLap[1][type & SYSNAME_TYPE_MASK];
				} else {
					// Not Phar-Lap.
					return nullptr;
				}
			}
			return sysNames_NE[d->hdr.ne.targOS][type & SYSNAME_TYPE_MASK];
		}

		case EXEPrivate::EXE_TYPE_LE:
		case EXEPrivate::EXE_TYPE_LX: {
			// Linear Executable.
			// TODO: Some DOS extenders have the target OS set to OS/2.
			// Check 'file' msdos magic.
			// TODO: Byteswapping.
			const uint16_t targOS = le16_to_cpu(d->hdr.le.targOS);
			if (targOS <= NE_OS_WIN386) {
				return sysNames_NE[targOS][type & SYSNAME_TYPE_MASK];
			}
			return nullptr;
		}

		case EXEPrivate::EXE_TYPE_W3: {
			// W3 executable. (Collection of LE executables.)
			// Only used by WIN386.EXE.
			return sysNames_Windows[type & SYSNAME_TYPE_MASK];
		}

		case EXEPrivate::EXE_TYPE_PE:
		case EXEPrivate::EXE_TYPE_PE32PLUS: {
			// Portable Executable.
			// TODO: Also used by older SkyOS and BeOS, and HX for DOS.
			switch (d->pe_subsystem) {
				case IMAGE_SUBSYSTEM_EFI_APPLICATION:
				case IMAGE_SUBSYSTEM_EFI_BOOT_SERVICE_DRIVER:
				case IMAGE_SUBSYSTEM_EFI_RUNTIME_DRIVER:
				case IMAGE_SUBSYSTEM_EFI_ROM: {
					// EFI executable.
					static const char *const sysNames_EFI[4] = {
						"Extensible Firmware Interface", "EFI", "EFI", nullptr
					};
					return sysNames_EFI[type & SYSNAME_TYPE_MASK];
				}

				case IMAGE_SUBSYSTEM_XBOX: {
					// TODO: Which Xbox?
					static const char *const sysNames_Xbox[4] = {
						"Microsoft Xbox", "Xbox", "Xbox", nullptr
					};
					return sysNames_Xbox[type & SYSNAME_TYPE_MASK];
				}

				default:
					return sysNames_Windows[type & SYSNAME_TYPE_MASK];
			}
			break;
		}

		default:
			break;
	};

	// Should not get here...
	assert(!"Unknown EXE type.");
	return "Unknown EXE type";
}

/**
 * Get a list of all supported file extensions.
 * This is to be used for file type registration;
 * subclasses don't explicitly check the extension.
 *
 * NOTE: The extensions do not include the leading dot,
 * e.g. "bin" instead of ".bin".
 *
 * NOTE 2: The array and the strings in the array should
 * *not* be freed by the caller.
 *
 * @return NULL-terminated array of all supported file extensions, or nullptr on error.
 */
const char *const *EXE::supportedFileExtensions_static(void)
{
	// References:
	// - https://en.wikipedia.org/wiki/Portable_Executable
	static const char *const exts[] = {
		// PE extensions
		".exe", ".dll",
		".acm", ".ax",
		".cpl", ".drv",
		".efi", ".mui",
		".ocx", ".scr",
		".sys", ".tsp",

		// NE extensions
		".fon", ".icl",

		// LE extensions
		".vxd", ".386",

		nullptr
	};
	return exts;
}

/**
 * Load field data.
 * Called by RomData::fields() if the field data hasn't been loaded yet.
 * @return Number of fields read on success; negative POSIX error code on error.
 */
int EXE::loadFieldData(void)
{
	RP_D(EXE);
	if (d->fields->isDataLoaded()) {
		// Field data *has* been loaded...
		return 0;
	} else if (!d->file || !d->file->isOpen()) {
		// File isn't open.
		return -EBADF;
	} else if (!d->isValid || d->exeType < 0) {
		// Unknown ROM image type.
		return -EIO;
	}

	// Maximum number of fields:
	// - NE: 6
	// - PE: 7
	//   - PE Version: +6
	//   - PE Manifest: +12
	d->fields->reserve(25);

	// Executable type.
	// NOTE: Not translatable.
	static const char *const exeTypes[EXEPrivate::EXE_TYPE_LAST] = {
		"MS-DOS Executable",		// EXE_TYPE_MZ
		"16-bit New Executable",	// EXE_TYPE_NE
		"Mixed-Mode Linear Executable",	// EXE_TYPE_LE
		"Windows/386 Kernel",		// EXE_TYPE_W3
		"32-bit Linear Executable",	// EXE_TYPE_LX
		"32-bit Portable Executable",	// EXE_TYPE_PE
		"64-bit Portable Executable",	// EXE_TYPE_PE32PLUS
	};
	if (d->exeType >= EXEPrivate::EXE_TYPE_MZ && d->exeType < EXEPrivate::EXE_TYPE_LAST) {
		d->fields->addField_string(C_("EXE", "Type"), exeTypes[d->exeType]);
	} else {
		d->fields->addField_string(C_("EXE", "Type"), C_("EXE", "Unknown"));
	}

	switch (d->exeType) {
		case EXEPrivate::EXE_TYPE_MZ:
			d->addFields_MZ();
			break;

		case EXEPrivate::EXE_TYPE_NE:
			d->addFields_NE();
			break;

		case EXEPrivate::EXE_TYPE_LE:
		case EXEPrivate::EXE_TYPE_LX:
			d->addFields_LE();
			break;

		case EXEPrivate::EXE_TYPE_PE:
		case EXEPrivate::EXE_TYPE_PE32PLUS:
			d->addFields_PE();
			break;

		default:
			// TODO: Other executable types.
			break;
	}

	// Add MZ tab for non-MZ executables
	if (d->exeType != EXEPrivate::EXE_TYPE_MZ) {
		d->fields->addTab(C_("EXE", "MZ Header")); // NOTE: doesn't actually create a separate tab for non implemented types.
		d->addFields_MZ();
	}

	// Finished reading the field data.
	return (int)d->fields->count();
}

}
