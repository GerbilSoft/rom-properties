/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * EXE.cpp: DOS/Windows executable reader.                                 *
 *                                                                         *
 * Copyright (c) 2016-2017 by David Korth.                                 *
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

#include "EXE.hpp"
#include "RomData_p.hpp"

#include "data/EXEData.hpp"
#include "exe_structs.h"

#include "common.h"
#include "byteswap.h"
#include "TextFuncs.hpp"
#include "file/IRpFile.hpp"

// IResourceReader
#include "disc/NEResourceReader.hpp"
#include "disc/PEResourceReader.hpp"

// C includes. (C++ namespace)
#include <cassert>
#include <cctype>
#include <cstdio>
#include <cstring>

// C++ includes.
#include <algorithm>
#include <string>
#include <vector>
using std::string;
using std::vector;

// Uninitialized vector class.
// Reference: http://andreoffringa.org/?q=uvector
#include "uvector.h"

namespace LibRomData {

class EXEPrivate : public RomDataPrivate
{
	public:
		EXEPrivate(EXE *q, IRpFile *file);
		~EXEPrivate();

	private:
		typedef RomDataPrivate super;
		RP_DISABLE_COPY(EXEPrivate)

	public:
		// Executable type.
		enum ExeType {
			EXE_TYPE_UNKNOWN = -1,	// Unknown EXE type.

			EXE_TYPE_MZ = 0,	// DOS MZ
			EXE_TYPE_NE,		// 16-bit New Executable
			EXE_TYPE_LE,		// Mixed 16/32-bit Linear Executable
			EXE_TYPE_W3,		// Collection of LE executables (WIN386.EXE)
			EXE_TYPE_LX,		// 32-bit Linear Executable
			EXE_TYPE_PE,		// 32-bit Portable Executable
			EXE_TYPE_PE32PLUS,	// 64-bit Portable Executable

			EXE_TYPE_LAST
		};
		int exeType;

	public:
		// DOS MZ header.
		IMAGE_DOS_HEADER mz;

		// Secondary header.
		#pragma pack(1)
		union PACKED {
			uint32_t sig32;
			uint16_t sig16;
			struct PACKED {
				uint32_t Signature;
				IMAGE_FILE_HEADER FileHeader;
				union {
					uint16_t Magic;
					IMAGE_OPTIONAL_HEADER32 opt32;
					IMAGE_OPTIONAL_HEADER64 opt64;
				} OptionalHeader;
			} pe;
			NE_Header ne;
			LE_Header le;
		} hdr;
		#pragma pack()

		// Resource reader.
		IResourceReader *rsrcReader;

		/**
		 * Add VS_VERSION_INFO fields.
		 *
		 * NOTE: A subtab is NOT created here; if one is desired,
		 * create and set it before calling this function.
		 *
		 * @param pVsFfi	[in] VS_FIXEDFILEINFO
		 * @param pVsSfi	[in,opt] IResourceReader::StringFileInfo
		 */
		void addFields_VS_VERSION_INFO(const VS_FIXEDFILEINFO *pVsFfi, const IResourceReader::StringFileInfo *pVsSfi);

		/** NE-specific **/

		// NE target OSes.
		// Also used for LE.
		static const rp_char *const NE_TargetOSes[];

		/**
		 * Load the NE resource table.
		 * @return 0 on success; negative POSIX error code on error. (-ENOENT if not found)
		 */
		int loadNEResourceTable(void);

		/**
		 * Add fields for NE executables.
		 */
		void addFields_NE(void);

		/** LE/LX-specific **/

		/**
		 * Add fields for LE/LX executables.
		 */
		void addFields_LE(void);

		/** PE-specific **/

		// PE subsystem.
		uint16_t pe_subsystem;

		// PE section headers.
		ao::uvector<IMAGE_SECTION_HEADER> pe_sections;

		/**
		 * Load the PE section table.
		 *
		 * NOTE: If the table was read successfully, but no section
		 * headers were found, -ENOENT is returned.
		 *
		 * @return 0 on success; negative POSIX error code on error.
		 */
		int loadPESectionTable(void);

		/**
		 * Load the top-level PE resource directory.
		 * @return 0 on success; negative POSIX error code on error. (-ENOENT if not found)
		 */
		int loadPEResourceTypes(void);

		/**
		 * Add fields for PE and PE32+ executables.
		 */
		void addFields_PE(void);
};

/** EXEPrivate **/

// NE target OSes.
// Also used for LE.
const rp_char *const EXEPrivate::NE_TargetOSes[] = {
	nullptr,			// NE_OS_UNKNOWN
	_RP("IBM OS/2"),		// NE_OS_OS2
	_RP("Microsoft Windows"),	// NE_OS_WIN
	_RP("European MS-DOS 4.x"),	// NE_OS_DOS4
	_RP("Microsoft Windows (386)"),	// NE_OS_WIN386 (TODO)
	_RP("Borland Operating System Services"),	// NE_OS_BOSS
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
 * @param pVsFfi	[in] VS_FIXEDFILEINFO
 * @param pVsSfi	[in,opt] IResourceReader::StringFileInfo
 */
void EXEPrivate::addFields_VS_VERSION_INFO(const VS_FIXEDFILEINFO *pVsFfi, const IResourceReader::StringFileInfo *pVsSfi)
{
	// Reference: https://msdn.microsoft.com/en-us/library/windows/desktop/ms646997(v=vs.85).aspx
	assert(pVsFfi != nullptr);
	if (!pVsFfi)
		return;

	// Temporary buffer for snprintf().
	char buf[64];
	int len;

	// File version.
	len = snprintf(buf, sizeof(buf), "%u.%u.%u.%u",
		pVsFfi->dwFileVersionMS >> 16,
		pVsFfi->dwFileVersionMS & 0xFFFF,
		pVsFfi->dwFileVersionLS >> 16,
		pVsFfi->dwFileVersionLS & 0xFFFF);
	if (len > (int)sizeof(buf))
		len = (int)sizeof(buf);
	fields->addField_string(_RP("File Version"),
		len > 0 ? latin1_to_rp_string(buf, len) : _RP("Unknown"));

	// Product version.
	len = snprintf(buf, sizeof(buf), "%u.%u.%u.%u",
		pVsFfi->dwProductVersionMS >> 16,
		pVsFfi->dwProductVersionMS & 0xFFFF,
		pVsFfi->dwProductVersionLS >> 16,
		pVsFfi->dwProductVersionLS & 0xFFFF);
	if (len > (int)sizeof(buf))
		len = (int)sizeof(buf);
	fields->addField_string(_RP("Product Version"),
		len > 0 ? latin1_to_rp_string(buf, len) : _RP("Unknown"));

	// File flags.
	static const rp_char *const file_flags_names[] = {
		_RP("Debug"), _RP("Prerelease"), _RP("Patched"),
		_RP("Private Build"), _RP("Info Inferred"), _RP("Special Build")
	};
	vector<rp_string> *v_file_flags_names = RomFields::strArrayToVector(
		file_flags_names, ARRAY_SIZE(file_flags_names));
	fields->addField_bitfield(_RP("File Flags"),
		v_file_flags_names, 3, pVsFfi->dwFileFlags & pVsFfi->dwFileFlagsMask);

	// File OS.
	const rp_char *file_os;
	switch (pVsFfi->dwFileOS) {
		case VOS_DOS:
			file_os = _RP("MS-DOS");
			break;
		case VOS_NT:
			file_os = _RP("Windows NT");
			break;
		case VOS__WINDOWS16:
			file_os = _RP("Windows (16-bit)");
			break;
		case VOS__WINDOWS32:
			file_os = _RP("Windows (32-bit)");
			break;
		case VOS_OS216:
			file_os = _RP("OS/2 (16-bit)");
			break;
		case VOS_OS232:
			file_os = _RP("OS/2 (32-bit)");
			break;
		case VOS__PM16:
			file_os = _RP("Presentation Manager (16-bit)");
			break;
		case VOS__PM32:
			file_os = _RP("Presentation Manager (32-bit)");
			break;

		case VOS_DOS_WINDOWS16:
			file_os = _RP("Windows on MS-DOS (16-bit)");
			break;
		case VOS_DOS_WINDOWS32:
			file_os = _RP("Windows 9x (32-bit)");
			break;
		case VOS_NT_WINDOWS32:
			file_os = _RP("Windows NT");
			break;
		case VOS_OS216_PM16:
			file_os = _RP("OS/2 with Presentation Manager (16-bit)");
			break;
		case VOS_OS232_PM32:
			file_os = _RP("OS/2 with Presentation Manager (32-bit)");
			break;

		case VOS_UNKNOWN:
		default:
			file_os = nullptr;
			break;
	}

	if (file_os) {
		fields->addField_string(_RP("File OS"), file_os);
	} else {
		len = snprintf(buf, sizeof(buf), "Unknown (0x%08X)", pVsFfi->dwFileOS);
		if (len > (int)sizeof(buf))
			len = (int)sizeof(buf);
		fields->addField_string(_RP("File OS"),
			len > 0 ? latin1_to_rp_string(buf, len) : _RP("Unknown"));
	}

	// File type.
	static const rp_char *const fileTypes[] = {
		nullptr,			// VFT_UNKNOWN
		_RP("Application"),		// VFT_APP
		_RP("DLL"),			// VFT_DLL
		_RP("Device Driver"),		// VFT_DRV
		_RP("Font"),			// VFT_FONT,
		_RP("Virtual Device Driver"),	// VFT_VXD
		nullptr,
		_RP("Static Library"),		// VFT_STATIC_LIB
	};
	const rp_char *fileType = (pVsFfi->dwFileType < ARRAY_SIZE(fileTypes)
					? fileTypes[pVsFfi->dwFileType]
					: nullptr);
	if (fileType) {
		fields->addField_string(_RP("File Type"), fileType);
	} else {
		len = snprintf(buf, sizeof(buf), "Unknown (0x%08X)", pVsFfi->dwFileType);
		if (len > (int)sizeof(buf))
			len = (int)sizeof(buf);
		fields->addField_string(_RP("File Type"),
			len > 0 ? latin1_to_rp_string(buf, len) : _RP("Unknown"));
	}

	// File subtype.
	bool hasSubtype = false;
	const rp_char *fileSubtype = nullptr;
	switch (pVsFfi->dwFileType) {
		case VFT_DRV: {
			hasSubtype = true;
			static const rp_char *const fileSubtypes_DRV[] = {
				nullptr,			// VFT2_UNKNOWN
				_RP("Printer"),			// VFT2_DRV_PRINTER
				_RP("Keyboard"),		// VFT2_DRV_KEYBOARD
				_RP("Language"),		// VFT2_DRV_LANGUAGE
				_RP("Display"),			// VFT2_DRV_DISPLAY
				_RP("Mouse"),			// VFT2_DRV_MOUSE
				_RP("Network"),			// VFT2_DRV_NETWORK
				_RP("System"),			// VFT2_DRV_SYSTEM
				_RP("Installable"),		// VFT2_DRV_INSTALLABLE
				_RP("Sound"),			// VFT2_DRV_SOUND
				_RP("Communications"),		// VFT2_DRV_COMM
				_RP("Input Method"),		// VFT2_DRV_INPUTMETHOD
				_RP("Versioned Printer"),	// VFT2_DRV_VERSIONED_PRINTER
			};
			fileSubtype = (pVsFfi->dwFileSubtype < ARRAY_SIZE(fileSubtypes_DRV)
						? fileSubtypes_DRV[pVsFfi->dwFileSubtype]
						: nullptr);
			break;
		};

		case VFT_FONT: {
			hasSubtype = true;
			static const rp_char *const fileSubtypes_FONT[] = {
				nullptr,		// VFT2_UNKNOWN
				_RP("Raster"),		// VFT2_FONT_RASTER
				_RP("Vector"),		// VFT2_FONT_VECTOR
				_RP("TrueType"),	// VFT2_FONT_TRUETYPE
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
			fields->addField_string(_RP("File Subtype"), fileSubtype);
		} else {
			len = snprintf(buf, sizeof(buf), "Unknown (%08X)", pVsFfi->dwFileSubtype);
			if (len > (int)sizeof(buf))
				len = (int)sizeof(buf);
			fields->addField_string(_RP("File Subtype"),
				len > 0 ? latin1_to_rp_string(buf, len) : _RP("Unknown"));
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
		int64_t fileTimeUnix = (fileTime - FILETIME_1970) / HECTONANOSEC_PER_SEC;
		fields->addField_dateTime(_RP("File Time"), fileTimeUnix,
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
	auto data = new vector<vector<rp_string> >();
	data->resize(st.size());
	for (unsigned int i = 0; i < (unsigned int)st.size(); i++) {
		const auto &st_row = st.at(i);
		auto &data_row = data->at(i);
		data_row.reserve(2);
		data_row.push_back(st_row.first);
		data_row.push_back(st_row.second);
	}

	// Fields.
	static const rp_char *const field_names[] = {
		_RP("Key"), _RP("Value")
	};
	vector<rp_string> *v_field_names = RomFields::strArrayToVector(
		field_names, ARRAY_SIZE(field_names));

	// Add the StringFileInfo.
	fields->addField_listData(_RP("StringFileInfo"), v_field_names, data);
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
	// Up to 2 tabs.
	fields->reserveTabs(2);

	// NE Header
	fields->setTabName(0, _RP("NE Header"));
	fields->setTabIndex(0);

	// Temporary buffer for snprintf().
	char buf[64];
	int len;

	// Target OS.
	const rp_char *targetOS = (hdr.ne.targOS < ARRAY_SIZE(NE_TargetOSes))
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
		fields->addField_string(_RP("Target OS"), targetOS);
	} else {
		len = snprintf(buf, sizeof(buf), "Unknown (0x%02X)", hdr.ne.targOS);
		if (len > (int)sizeof(buf))
			len = (int)sizeof(buf);
		fields->addField_string(_RP("Target OS"),
			len > 0 ? latin1_to_rp_string(buf, len) : _RP("Unknown"));
	}

	// DGroup type.
	static const rp_char *const dgroupTypes[] = {
		_RP("None"), _RP("Single Shared"), _RP("Multiple"), _RP("(null)")
	};
	fields->addField_string(_RP("DGroup Type"), dgroupTypes[hdr.ne.ProgFlags & 3]);

	// Program flags.
	static const rp_char *const ProgFlags_names[] = {
		nullptr, nullptr,	// DGroup Type
		_RP("Global Init"), _RP("Protected Mode Only"),
		_RP("8086 insns"), _RP("80286 insns"),
		_RP("80386 insns"), _RP("FPU insns")
	};
	vector<rp_string> *v_ProgFlags_names = RomFields::strArrayToVector(
		ProgFlags_names, ARRAY_SIZE(ProgFlags_names));
	fields->addField_bitfield(_RP("Program Flags"),
		v_ProgFlags_names, 2, hdr.ne.ProgFlags);

	// Application type.
	if (hdr.ne.targOS == NE_OS_OS2) {
		// Only mentioning Presentation Manager for OS/2 executables.
		static const rp_char *const applTypes_OS2[] = {
			_RP("None"),
			_RP("Full Screen (not aware of Presentation Manager)"),
			_RP("Presentation Manager compatible"),
			_RP("Presentation Manager application")
		};
		fields->addField_string(_RP("Application Type"),
			applTypes_OS2[hdr.ne.ApplFlags & 3]);
	} else {
		// Assume Windows for everything else.
		static const rp_char *const applTypes_Win[] = {
			_RP("None"),
			_RP("Full Screen (not aware of Windows)"),
			_RP("Windows compatible"),
			_RP("Windows application")
		};
		fields->addField_string(_RP("Application Type"),
			applTypes_Win[hdr.ne.ApplFlags & 3]);
	}

	// Application flags.
	static const rp_char *const ApplFlags_names[] = {
		nullptr, nullptr,	// Application type
		nullptr, _RP("OS/2 Application"),
		nullptr, _RP("Image Error"),
		_RP("Non-Conforming"), _RP("DLL")
	};
	vector<rp_string> *v_ApplFlags_names = RomFields::strArrayToVector(
		ApplFlags_names, ARRAY_SIZE(ApplFlags_names));
	fields->addField_bitfield(_RP("Application Flags"),
		v_ApplFlags_names, 2, hdr.ne.ApplFlags);

	// Other flags.
	// NOTE: Indicated as OS/2 flags by OSDev Wiki,
	// but may be set on Windows programs too.
	// References:
	// - http://wiki.osdev.org/NE
	// - http://www.program-transformation.org/Transform/PcExeFormat
	static const rp_char *const other_flags_names[] = {
		_RP("Long File Names"), _RP("Protected Mode"),
		_RP("Proportional Fonts"), _RP("Gangload Area"),
	};
	vector<rp_string> *v_other_flags_names = RomFields::strArrayToVector(
		other_flags_names, ARRAY_SIZE(other_flags_names));
	fields->addField_bitfield(_RP("Other Flags"),
		v_other_flags_names, 2, hdr.ne.OS2EXEFlags);

	// Expected Windows version.
	// TODO: Is this used in OS/2 executables?
	if (hdr.ne.targOS == NE_OS_WIN || hdr.ne.targOS == NE_OS_WIN386) {
		len = snprintf(buf, sizeof(buf), "%u.%u",
			hdr.ne.expctwinver[1], hdr.ne.expctwinver[0]);
		if (len > (int)sizeof(buf))
			len = (int)sizeof(buf);
		fields->addField_string(_RP("Windows Version"),
			len > 0 ? latin1_to_rp_string(buf, len) : _RP("Unknown"));
	}

	// Load resources.
	int ret = loadNEResourceTable();
	if (ret != 0 || !rsrcReader) {
		// Unable to load resources.
		// We're done here.
		return;
	}

	// Load the version resource.
	VS_FIXEDFILEINFO vsffi;
	IResourceReader::StringFileInfo vssfi;
	ret = rsrcReader->load_VS_VERSION_INFO(VS_VERSION_INFO, -1, &vsffi, &vssfi);
	if (ret != 0) {
		// Unable to load the version resource.
		// We're done here.
		return;
	}

	// Add the version fields.
	fields->setTabName(1, _RP("Version"));
	fields->setTabIndex(1);
	addFields_VS_VERSION_INFO(&vsffi, &vssfi);
}

/** LE/LX-specific **/

/**
 * Add fields for LE/LX executables.
 */
void EXEPrivate::addFields_LE(void)
{
	// TODO: Byteswapping values.

	// Temporary buffer for snprintf().
	char buf[64];
	int len;

	// CPU.
	const uint16_t cpu_type = le16_to_cpu(hdr.le.cpu_type);
	const rp_char *const cpu = EXEData::lookup_le_cpu(cpu_type);
	if (cpu) {
		fields->addField_string(_RP("CPU"), cpu);
	} else {
		len = snprintf(buf, sizeof(buf), "Unknown (0x%04X)", cpu_type);
		if (len > (int)sizeof(buf))
			len = (int)sizeof(buf);
		fields->addField_string(_RP("CPU"),
			len > 0 ? latin1_to_rp_string(buf, len) : _RP("Unknown"));
	}

	// Target OS.
	// NOTE: Same as NE.
	const uint16_t targOS = le16_to_cpu(hdr.le.targOS);
	const rp_char *const targetOS = (targOS < ARRAY_SIZE(NE_TargetOSes))
					? NE_TargetOSes[targOS]
					: nullptr;
	if (targetOS) {
		fields->addField_string(_RP("Target OS"), targetOS);
	} else {
		len = snprintf(buf, sizeof(buf), "Unknown (0x%02X)", targOS);
		if (len > (int)sizeof(buf))
			len = (int)sizeof(buf);
		fields->addField_string(_RP("Target OS"),
			len > 0 ? latin1_to_rp_string(buf, len) : _RP("Unknown"));
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
	int ret = file->seek(section_table_start);
	if (ret != 0) {
		// Seek error.
		return -EIO;
	}
	pe_sections.resize(section_count);
	uint32_t szToRead = (uint32_t)(section_count * sizeof(IMAGE_SECTION_HEADER));
	size_t size = file->read(pe_sections.data(), szToRead);
	if (size != (size_t)szToRead) {
		// Read error.
		pe_sections.clear();
		return -EIO;
	}

	// Not all sections may be in use.
	// Find the first section header with an empty name.
	ret = 0;
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
	// Up to 3 tabs.
	fields->reserveTabs(3);

	// PE Header
	fields->setTabName(0, _RP("PE Header"));
	fields->setTabIndex(0);

	// Temporary buffer for snprintf().
	char buf[64];
	int len;

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
	rp_string s_cpu;
	const rp_char *const cpu = EXEData::lookup_pe_cpu(machine);
	if (cpu != nullptr) {
		s_cpu = cpu;
	} else {
		len = snprintf(buf, sizeof(buf), "Unknown (0x%04X)%s",
			machine, (dotnet ? " (.NET)" : ""));
		if (len > (int)sizeof(buf))
			len = (int)sizeof(buf);
		s_cpu = (len > 0 ? latin1_to_rp_string(buf, len) : _RP("Unknown"));
	}
	if (dotnet) {
		// .NET executable.
		s_cpu += _RP(" (.NET)");
	}
	fields->addField_string(_RP("CPU"), s_cpu);

	// OS version.
	len = snprintf(buf, sizeof(buf), "%u.%u", os_ver_major, os_ver_minor);
	if (len > (int)sizeof(buf))
		len = (int)sizeof(buf);
	fields->addField_string(_RP("OS Version"),
		len > 0 ? latin1_to_rp_string(buf, len) : _RP("Unknown"));

	// Subsystem names.
	static const char *const subsysNames[IMAGE_SUBSYSTEM_XBOX+1] = {
		nullptr,			// IMAGE_SUBSYSTEM_UNKNOWN
		"Native",			// IMAGE_SUBSYSTEM_NATIVE
		"Windows",			// IMAGE_SUBSYSTEM_WINDOWS_GUI
		"Console",			// IMAGE_SUBSYSTEM_WINDOWS_CUI
		nullptr,
		"OS/2 Console",			// IMAGE_SUBSYSTEM_OS2_CUI
		nullptr,
		"POSIX Console",		// IMAGE_SUBSYSTEM_POSIX_CUI
		"Win9x Native Driver",		// IMAGE_SUBSYSTEM_NATIVE_WINDOWS
		"Windows CE",			// IMAGE_SUBSYSTEM_WINDOWS_CE_GUI
		"EFI Application",		// IMAGE_SUBSYSTEM_EFI_APPLICATION
		"EFI Boot Service Driver",	// IMAGE_SUBSYSTEM_EFI_BOOT_SERVICE_DRIVER
		"EFI Runtime Driver",		// IMAGE_SUBSYSTEM_EFI_RUNTIME_DRIVER
		"EFI ROM Image",		// IMAGE_SUBSYSTEM_EFI_ROM
		"Xbox",				// IMAGE_SUBSYSTEM_XBOX
	};

	// Subsystem name and version.
	len = snprintf(buf, sizeof(buf), "%s %u.%u",
		(pe_subsystem < ARRAY_SIZE(subsysNames)
			? subsysNames[pe_subsystem]
			: "Unknown"),
		subsystem_ver_major, subsystem_ver_minor);
	if (len > (int)sizeof(buf))
		len = (int)sizeof(buf);
	fields->addField_string(_RP("Subsystem"),
		len > 0 ? latin1_to_rp_string(buf, len) : _RP("Unknown"));

	// PE flags. (characteristics)
	// NOTE: Only important flags will be listed.
	static const rp_char *const pe_flags_names[] = {
		nullptr, _RP("Executable"), nullptr,
		nullptr, nullptr, _RP(">2GB addressing"),
		nullptr, nullptr, nullptr,
		nullptr, nullptr, nullptr,
		nullptr, _RP("DLL"), nullptr,
		nullptr
	};
	vector<rp_string> *v_pe_flags_names = RomFields::strArrayToVector(
		pe_flags_names, ARRAY_SIZE(pe_flags_names));
	fields->addField_bitfield(_RP("PE Flags"),
		v_pe_flags_names, 3, pe_flags);

	// DLL flags. (characteristics)
	static const rp_char *const dll_flags_names[] = {
		nullptr, nullptr, nullptr,
		nullptr, nullptr, _RP("High Entropy VA"),
		_RP("Dynamic Base"), _RP("Force Integrity"), _RP("NX Compatible"),
		_RP("No Isolation"), _RP("No SEH"), _RP("No Bind"),
		_RP("AppContainer"), _RP("WDM Driver"), _RP("Control Flow Guard"),
		_RP("TS Aware"),
	};
	vector<rp_string> *v_dll_flags_names = RomFields::strArrayToVector(
		dll_flags_names, ARRAY_SIZE(dll_flags_names));
	fields->addField_bitfield(_RP("DLL Flags"),
		v_dll_flags_names, 3, dll_flags);

	// Load resources.
	int ret = loadPEResourceTypes();
	if (ret != 0 || !rsrcReader) {
		// Unable to load resources.
		// We're done here.
		return;
	}

	// Load the version resource.
	VS_FIXEDFILEINFO vsffi;
	IResourceReader::StringFileInfo vssfi;
	ret = rsrcReader->load_VS_VERSION_INFO(VS_VERSION_INFO, -1, &vsffi, &vssfi);
	if (ret != 0) {
		// Unable to load the version resource.
		// We're done here.
		return;
	}

	// Add the version fields.
	fields->setTabName(1, _RP("Version"));
	fields->setTabIndex(1);
	addFields_VS_VERSION_INFO(&vsffi, &vssfi);
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
	    be16_to_cpu(d->mz.e_magic) == 'ZM') {
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

	int ret = d->file->seek(hdr_addr);
	if (ret != 0) {
		// Seek error.
		d->exeType = EXEPrivate::EXE_TYPE_UNKNOWN;
		d->isValid = false;
		return;
	}
	size = d->file->read(&d->hdr, sizeof(d->hdr));
	if (size != sizeof(d->hdr)) {
		// Read error.
		// TODO: Check the signature first instead of
		// depending on the full union being available?
		d->exeType = EXEPrivate::EXE_TYPE_UNKNOWN;
		d->isValid = false;
		return;
	}

	// Check the signature.
	// FIXME: MSVC handles 'PE\0\0' as 0x00504500,
	// probably due to the embedded NULL bytes.
	if (be32_to_cpu(d->hdr.pe.Signature) == 0x50450000 /*'PE\0\0'*/) {
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
	} else if (be16_to_cpu(d->hdr.ne.sig) == 'NE' /* 'NE' */) {
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
	} else if (be16_to_cpu(d->hdr.le.sig) == 'LE' ||
		   be16_to_cpu(d->hdr.le.sig) == 'LX')
	{
		// Linear Executable.
		if (be16_to_cpu(d->hdr.le.sig) == 'LE') {
			d->exeType = EXEPrivate::EXE_TYPE_LE;
		} else /*if (be16_to_cpu(d->hdr.le.sig) == 'LX')*/ {
			d->exeType = EXEPrivate::EXE_TYPE_LX;
		}

		// TODO: Check byteorder flags and adjust as necessary.
		if (le16_to_cpu(d->hdr.le.targOS) == NE_OS_WIN386) {
			// LE VxD
			d->fileType = FTYPE_DEVICE_DRIVER;
		} else if (le32_to_cpu(d->hdr.le.module_type_flags) & LE_MODULE_IS_DLL) {
			// LE DLL
			d->fileType = FTYPE_DLL;
		} else {
			// LE EXE
			d->fileType = FTYPE_EXECUTABLE;
		}
	} else if (be16_to_cpu(d->hdr.sig16) == 'W3' /* 'W3' */) {
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
 * Is a ROM image supported by this object?
 * @param info DetectInfo containing ROM detection information.
 * @return Class-specific system ID (>= 0) if supported; -1 if not.
 */
int EXE::isRomSupported(const DetectInfo *info) const
{
	return isRomSupported_static(info);
}

/**
 * Get the name of the system the loaded ROM is designed for.
 * @param type System name type. (See the SystemName enum.)
 * @return System name, or nullptr if type is invalid.
 */
const rp_char *EXE::systemName(uint32_t type) const
{
	RP_D(const EXE);
	if (!d->isValid || !isSystemNameTypeValid(type))
		return nullptr;

	static const rp_char *const sysNames_Windows[4] = {
		_RP("Microsoft Windows"), _RP("Windows"), _RP("Windows"), nullptr
	};

	// New Executable (and Linear Executable) operating systems.
	static const rp_char *const sysNames_NE[6][4] = {
		{nullptr, nullptr, nullptr, nullptr},						// NE_OS_UNKNOWN
		{_RP("IBM OS/2"), _RP("OS/2"), _RP("OS/2"), nullptr},				// NE_OS_OS2
		{_RP("Microsoft Windows"), _RP("Windows"), _RP("Windows"), nullptr},		// NE_OS_WIN
		{_RP("European MS-DOS 4.x"), _RP("EuroDOS 4.x"), _RP("EuroDOS 4.x"), nullptr},	// NE_OS_DOS4
		{_RP("Microsoft Windows"), _RP("Windows"), _RP("Windows"), nullptr},		// NE_OS_WIN386 (TODO)
		{_RP("Borland Operating System Services"), _RP("BOSS"), _RP("BOSS"), nullptr},	// NE_OS_BOSS
	};

	switch (d->exeType) {
		case EXEPrivate::EXE_TYPE_MZ: {
			// DOS executable.
			static const rp_char *const sysNames_DOS[4] = {
				_RP("Microsoft MS-DOS"), _RP("MS-DOS"), _RP("DOS"), nullptr
			};
			return sysNames_DOS[type & SYSNAME_TYPE_MASK];
		}

		case EXEPrivate::EXE_TYPE_NE: {
			// New Executable.
			if (d->hdr.ne.targOS > NE_OS_BOSS) {
				// Check for Phar Lap 286 extenders.
				// Reference: https://github.com/weheartwebsites/exeinfo/blob/master/exeinfo.cpp
				static const rp_char *const sysNames_NE_PharLap[2][4] = {
					{_RP("Phar Lap 286|DOS Extender, OS/2"), _RP("Phar Lap 286 OS/2"), _RP("Phar Lap 286 OS/2"), nullptr},	// 0x81
					{_RP("Phar Lap 286|DOS Extender, Windows"), _RP("Phar Lap 286 Windows"), _RP("Phar Lap 286 Windows"), nullptr},	// 0x82
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
					static const rp_char *const sysNames_EFI[4] = {
						_RP("Extensible Firmware Interface"), _RP("EFI"), _RP("EFI"), nullptr
					};
					return sysNames_EFI[type & SYSNAME_TYPE_MASK];
				}

				case IMAGE_SUBSYSTEM_XBOX: {
					// TODO: Which Xbox?
					static const rp_char *const sysNames_Xbox[4] = {
						_RP("Microsoft Xbox"), _RP("Xbox"), _RP("Xbox"), nullptr
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
	return _RP("Unknown EXE type");
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
const rp_char *const *EXE::supportedFileExtensions_static(void)
{
	// References:
	// - https://en.wikipedia.org/wiki/Portable_Executable
	static const rp_char *const exts[] = {
		// PE extensions
		_RP(".exe"), _RP(".dll"),
		_RP(".acm"), _RP(".ax"),
		_RP(".cpl"), _RP(".drv"),
		_RP(".efi"), _RP(".mui"),
		_RP(".ocx"), _RP(".scr"),
		_RP(".sys"), _RP(".tsp"),

		// NE extensions
		_RP(".fon"), _RP(".icl"),

		// LE extensions
		_RP("*.vxd"), _RP(".386"),

		nullptr
	};
	return exts;
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
const rp_char *const *EXE::supportedFileExtensions(void) const
{
	return supportedFileExtensions_static();
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
	// - PE: 6
	//   - PE Version: +6
	d->fields->reserve(12);

	// Executable type.
	static const rp_char *const exeTypes[EXEPrivate::EXE_TYPE_LAST] = {
		_RP("MS-DOS Executable"),		// EXE_TYPE_MZ
		_RP("16-bit New Executable"),		// EXE_TYPE_NE
		_RP("Mixed-Mode Linear Executable"),	// EXE_TYPE_LE
		_RP("Windows/386 Kernel"),		// EXE_TYPE_W3
		_RP("32-bit Linear Executable"),	// EXE_TYPE_LX
		_RP("32-bit Portable Executable"),	// EXE_TYPE_PE
		_RP("64-bit Portable Executable"),	// EXE_TYPE_PE32PLUS
	};
	if (d->exeType >= EXEPrivate::EXE_TYPE_MZ && d->exeType < EXEPrivate::EXE_TYPE_LAST) {
		d->fields->addField_string(_RP("Type"), exeTypes[d->exeType]);
	} else {
		d->fields->addField_string(_RP("Type"), _RP("Unknown"));
	}

	switch (d->exeType) {
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

	// Finished reading the field data.
	return (int)d->fields->count();
}

}
