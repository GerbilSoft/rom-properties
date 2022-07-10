/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * EXE.cpp: DOS/Windows executable reader.                                 *
 *                                                                         *
 * Copyright (c) 2016-2022 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "EXE.hpp"

// librpbase, librpfile
#include "librpbase/Achievements.hpp"
using namespace LibRpBase;
using LibRpFile::IRpFile;

// C++ STL classes.
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

/* RomDataInfo */
const char8_t *const EXEPrivate::exts[] = {
	// References:
	// - https://en.wikipedia.org/wiki/Portable_Executable

	// PE extensions
	U8(".exe"), U8(".dll"),
	U8(".acm"), U8(".ax"),
	U8(".cpl"), U8(".drv"),
	U8(".efi"), U8(".mui"),
	U8(".ocx"), U8(".scr"),
	U8(".sys"), U8(".tsp"),

	// NE extensions
	U8(".fon"), U8(".icl"),

	// LE extensions
	U8(".vxd"), U8(".386"),

	nullptr
};
const char *const EXEPrivate::mimeTypes[] = {
	// Unofficial MIME types from FreeDesktop.org.
	"application/x-ms-dos-executable",

	// Unofficial MIME types from Microsoft.
	// Reference: https://technet.microsoft.com/en-us/library/cc995276.aspx?f=255&MSPPError=-2147217396
	"application/x-msdownload",

	nullptr
};
const RomDataInfo EXEPrivate::romDataInfo = {
	"EXE", exts, mimeTypes
};

// NE target OSes.
// Also used for LE.
const char *const EXEPrivate::NE_TargetOSes[6] = {
	nullptr,			// NE_OS_UNKNOWN
	"IBM OS/2",			// NE_OS_OS2
	"Microsoft Windows",		// NE_OS_WIN
	"European MS-DOS 4.x",		// NE_OS_DOS4
	"Microsoft Windows (386)",	// NE_OS_WIN386 (TODO)
	"Borland Operating System Services",	// NE_OS_BOSS
};

EXEPrivate::EXEPrivate(EXE *q, IRpFile *file)
	: super(q, file, &romDataInfo)
	, exeType(ExeType::Unknown)
	, rsrcReader(nullptr)
	, pe_subsystem(IMAGE_SUBSYSTEM_UNKNOWN)
{
	// Clear the structs.
	memset(&mz, 0, sizeof(mz));
	memset(&hdr, 0, sizeof(hdr));
}

EXEPrivate::~EXEPrivate()
{
	UNREF(rsrcReader);
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
	// Reference: https://docs.microsoft.com/en-us/windows/win32/api/verrsrc/ns-verrsrc-vs_fixedfileinfo
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
	vector<string> *const v_FileFlags_names = RomFields::strArrayToVector_i18n(
		"EXE|FileFlags", FileFlags_names, ARRAY_SIZE(FileFlags_names));
	fields->addField_bitfield(C_("EXE", "File Flags"),
		v_FileFlags_names, 3, pVsFfi->dwFileFlags & pVsFfi->dwFileFlagsMask);

	// File OS.
	// NOTE: Not translatable.
	static const struct {
		uint32_t dwFileOS;
		const char *s_fileOS;
	} fileOS_lkup_tbl[] = {
		// TODO: Reorder based on how common each OS is?
		// VOS_NT_WINDOWS32 is probably the most common nowadays.

		{VOS_DOS,		"MS-DOS"},
		{VOS_OS216,		"OS/2 (16-bit)"},
		{VOS_OS232,		"OS/2 (32-bit)"},
		{VOS_NT,		"Windows NT"},
		{VOS_WINCE,		"Windows CE"},

		{VOS__WINDOWS16,	"Windows (16-bit)"},
		{VOS__WINDOWS32,	"Windows (32-bit)"},
		{VOS__PM16,		"Presentation Manager (16-bit)"},
		{VOS__PM32,		"Presentation Manager (32-bit)"},

		{VOS_DOS_WINDOWS16,	"Windows on MS-DOS (16-bit)"},
		{VOS_DOS_WINDOWS32,	"Windows 9x (32-bit)"},
		{VOS_OS216_PM16,	"OS/2 with Presentation Manager (16-bit)"},
		{VOS_OS232_PM32,	"OS/2 with Presentation Manager (32-bit)"},
		{VOS_NT_WINDOWS32,	"Windows NT"},
	};

	const uint32_t dwFileOS = pVsFfi->dwFileOS;
	const char *s_fileOS = nullptr;
	for (const auto &p : fileOS_lkup_tbl) {
		if (p.dwFileOS == dwFileOS) {
			// Found a match.
			s_fileOS = p.s_fileOS;
			break;
		}
	}

	const char *const fileOS_title = C_("EXE", "File OS");
	if (s_fileOS) {
		fields->addField_string(fileOS_title, s_fileOS);
	} else {
		fields->addField_string(fileOS_title,
			rp_sprintf(C_("RomData", "Unknown (0x%08X)"), dwFileOS));
	}

	// File type.
	static const char *const fileTypes_tbl[] = {
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
		// Type 6 is unknown...
		nullptr,
		// tr: VFT_STATIC_LIB
		NOP_C_("EXE|FileType", "Static Library"),
	};
	const char *const fileType_title = C_("EXE", "File Type");
	if (pVsFfi->dwFileType < ARRAY_SIZE(fileTypes_tbl) &&
	    fileTypes_tbl[pVsFfi->dwFileType] != nullptr)
	{
		fields->addField_string(fileType_title,
			dpgettext_expr(RP_I18N_DOMAIN, "EXE|FileType", fileTypes_tbl[pVsFfi->dwFileType]));
	} else {
		if (pVsFfi->dwFileType == VFT_UNKNOWN) {
			fields->addField_string(fileType_title, C_("RomData", "Unknown"));
		} else {
			fields->addField_string(fileType_title,
				rp_sprintf(C_("RomData", "Unknown (0x%08X)"), pVsFfi->dwFileType));
		}
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
			if (pVsFfi->dwFileSubtype < ARRAY_SIZE(fileSubtypes_DRV)) {
				fileSubtype = fileSubtypes_DRV[pVsFfi->dwFileSubtype];
			}
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
			if (pVsFfi->dwFileSubtype < ARRAY_SIZE(fileSubtypes_FONT)) {
				fileSubtype = fileSubtypes_FONT[pVsFfi->dwFileSubtype];
			}
			break;
		};

		default:
			break;
	}

	if (hasSubtype) {
		const char *const fileSubType_title = C_("EXE", "File Subtype");
		if (fileSubtype) {
			fields->addField_string(fileSubType_title,
				dpgettext_expr(RP_I18N_DOMAIN, "EXE|FileSubType", fileSubtype));
		} else {
			fields->addField_string(fileSubType_title,
				rp_sprintf(C_("RomData", "Unknown (0x%02X)"), pVsFfi->dwFileSubtype));
		}
	}

	// File timestamp. (FILETIME format)
	// NOTE: This seems to be 0 in most EXEs and DLLs I've tested.
	uint64_t fileTime = (static_cast<uint64_t>(pVsFfi->dwFileDateMS)) << 32 |
			     static_cast<uint64_t>(pVsFfi->dwFileDateLS);
	if (fileTime != 0) {
		// Convert to UNIX time.
#ifndef FILETIME_1970
		#define FILETIME_1970 116444736000000000LL	// Seconds between 1/1/1601 and 1/1/1970.
#endif
#ifndef HECTONANOSEC_PER_SEC
		#define HECTONANOSEC_PER_SEC 10000000LL
#endif
		time_t fileTimeUnix = static_cast<time_t>((fileTime - FILETIME_1970) / HECTONANOSEC_PER_SEC);
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
	auto vv_data = new RomFields::ListData_t(st.size());
	for (size_t i = 0; i < st.size(); i++) {
		const auto &st_row = st.at(i);
		auto &data_row = vv_data->at(i);
		data_row.reserve(2);
		data_row.emplace_back(st_row.first);
		data_row.emplace_back(st_row.second);
	}

	// Fields.
	static const char *const field_names[] = {
		NOP_C_("EXE|StringFileInfo", "Key"),
		NOP_C_("EXE|StringFileInfo", "Value"),
	};
	vector<string> *const v_field_names = RomFields::strArrayToVector_i18n(
		"EXE|StringFileInfo", field_names, ARRAY_SIZE(field_names));

	// Add the StringFileInfo.
	RomFields::AFLD_PARAMS params;
	params.headers = v_field_names;
	params.data.single = vv_data;
	fields->addField_listData("StringFileInfo", &params);
}

/** MZ-specific **/

/**
 * Add fields for MZ executables.
 */
void EXEPrivate::addFields_MZ(void)
{
	/* MS-DOS allocation algorithm (all sizes in paragraphs):
	 *     progsize = e_cp*512/16 - e_cparhdr
	 *     if maxfreeblock < 0x10 + progsize:
	 *         error
	 *     if e_maxalloc == 0:
	 *         allocate(maxfreeblock)
	 *         load_high
	 *         return
	 *     if maxfreeblock < 0x10 + progsize + e_minalloc:
	 *         error
	 *     allocate(min(0x10 + progsize + e_maxalloc, maxfreeblock))
	 *     load_low
	 *
	 * e_cblp doesn't seem to be used by MS-DOS at all. The documentation says
	 * that its meant for overlays. However, overlay or not, the loader just
	 * keeps reading until it reads progsize paragraphs, or gets a short read.
	 * With the above in mind, progsize seems to be the most useful metric
	 * to display, since e_cp/e_cblp is supposed to be the same as the filesize.
	 */

	// Header and program size
	fields->addField_string(C_("EXE", "Header Size"), formatFileSize(le16_to_cpu(mz.e_cparhdr) * 16));
	uint32_t program_size = le16_to_cpu(mz.e_cp) * 512 - le16_to_cpu(mz.e_cparhdr) * 16;
	fields->addField_string(C_("EXE", "Program Size"), formatFileSize(program_size));

	// File size warnings
	// Only show them if it's an MZ-only executable and if e_cblp is sane
	bool shownWarning = false;
	if (exeType == EXEPrivate::ExeType::MZ && le16_to_cpu(mz.e_cblp) > 511) {
		off64_t file_size = file->size();
		if (file_size != -1) {
			off64_t image_size = le16_to_cpu(mz.e_cp)*512;
			if (mz.e_cblp != 0)
				image_size -= 512 - le16_to_cpu(mz.e_cblp);
			if (file_size < image_size) {
				fields->addField_string(C_("RomData", "Warning"),
					C_("EXE", "Program image truncated"), RomFields::STRF_WARNING);
				shownWarning = true;
			} else if (file_size > image_size) {
				fields->addField_string(C_("RomData", "Warning"),
					C_("EXE", "Extra data after end of file"), RomFields::STRF_WARNING);
				shownWarning = true;
			}
		}
	}

	// Min/Max allocated memory
	if (mz.e_maxalloc != 0) {
		fields->addField_string(C_("EXE", "Min. Memory"), formatFileSize(le16_to_cpu(mz.e_minalloc) * 16));
		fields->addField_string(C_("EXE", "Max. Memory"),
			mz.e_maxalloc == 0xFFFF ? C_("EXE", "All") : formatFileSize(le16_to_cpu(mz.e_maxalloc) * 16));
	} else {
		/* NOTE: A "high" load means the program it at the end of the allocated
		 * area, with extra pragraphs being between PSP and the program.
		 * Not to be confused with "LOADHIGH" which loads programs into UMB.
		 */
		fields->addField_string(C_("EXE", "Load Type"), C_("EXE", "High"));
	}

	// Initial CS:IP/SS:SP
	fields->addField_string(C_("EXE", "Initial CS:IP"),
		rp_sprintf("%04X:%04X", le16_to_cpu(mz.e_cs), le16_to_cpu(mz.e_ip)), RomFields::STRF_MONOSPACE);
	fields->addField_string(C_("EXE", "Initial SS:SP"),
		rp_sprintf("%04X:%04X", le16_to_cpu(mz.e_ss), le16_to_cpu(mz.e_sp)), RomFields::STRF_MONOSPACE);

	/* Linkers will happily put 0:0 in SS:SP if the stack is not defined.
	 * In this case, at least DOS 5 and later will do the following hacks:
	 * - If progsize < 64k-256, add 256 to it.
	 * - If allocation size < 64k, set SP to allocation size - 256 (size of PSP)
	 * The idea is that if a <64k program specifies 0:0 as the stack, it likely
	 * expects to own 0:FFFF, as that's where the first push will go. Now, the
	 * default maxalloc is FFFF, so unless you have <64k free memory, it'll
	 * work fine. This hack improves compatibility with such programs when
	 * you're low on memory.
	 * I think this warrants a warning.
	 */
	if (mz.e_ss == 0 && mz.e_sp == 0) {
		if (!shownWarning) {
			fields->addField_string(C_("RomData", "Warning"),
				C_("EXE", "No stack"), RomFields::STRF_WARNING);
		}
	}
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
	fields->setTabName(0, "LE");
	fields->setTabIndex(0);

	// CPU.
	const char *const cpu_title = C_("EXE", "CPU");
	const uint16_t cpu_type = le16_to_cpu(hdr.le.cpu_type);
	const char8_t *const s_cpu = EXEData::lookup_le_cpu(cpu_type);
	if (s_cpu) {
		fields->addField_string(cpu_title, s_cpu);
	} else {
		fields->addField_string(cpu_title,
			rp_sprintf(C_("RomData", "Unknown (0x%04X)"), cpu_type));
	}

	// Target OS.
	// NOTE: Same as NE.
	const char *const targetOS_title = C_("EXE", "Target OS");
	const uint16_t targOS = le16_to_cpu(hdr.le.targOS);
	if (targOS < ARRAY_SIZE(NE_TargetOSes)) {
		fields->addField_string(targetOS_title, NE_TargetOSes[targOS]);
	} else {
		fields->addField_string(targetOS_title,
			rp_sprintf(C_("RomData", "Unknown (0x%02X)"), targOS));
	}
}

/** EXE **/

/**
 * Read a DOS/Windows executable.
 *
 * A ROM file must be opened by the caller. The file handle
 * will be ref()'d and must be kept open in order to load
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
	d->mimeType = "application/x-ms-dos-executable";	// unofficial (TODO: More types?)
	d->fileType = FileType::Unknown;

	if (!d->file) {
		// Could not ref() the file handle.
		return;
	}

	// Read the DOS MZ header.
	d->file->rewind();
	size_t size = d->file->read(&d->mz, sizeof(d->mz));
	if (size != sizeof(d->mz)) {
		UNREF_AND_NULL_NOCHK(d->file);
		return;
	}

	// Check if this executable is supported.
	const DetectInfo info = {
		{0, sizeof(d->mz), reinterpret_cast<const uint8_t*>(&d->mz)},
		nullptr,	// ext (not needed for EXE)
		0		// szFile (not needed for EXE)
	};
	d->isValid = (isRomSupported_static(&info) >= 0);

	if (!d->isValid) {
		// Not an MZ executable.
		UNREF_AND_NULL_NOCHK(d->file);
		return;
	}

	// NOTE: isRomSupported_static() only determines if the
	// file has a DOS MZ executable stub. The actual executable
	// type is determined here.

	// Check for MS-DOS executables:
	// - Relocation table address less than 0x40
	// - Magic number is 'ZM' (Windows only accepts 'MZ')
	if (le16_to_cpu(d->mz.e_lfarlc) < 0x40 ||
	    d->mz.e_magic == cpu_to_be16('ZM'))
	{
		// MS-DOS executable.
		// NOTE: Some EFI executables have a 0 offset for the
		// relocation table. Check some other fields, and if
		// they're all zero, assume it's *not* MS-DOS.
		// NOTE 2: Byteswapping isn't needed for 0 checks.
		bool isDOS = true;
		if (d->mz.e_lfarlc == 0) {
			if (d->mz.e_cp == 0 &&
			    d->mz.e_cs == 0 && d->mz.e_ip == 0 &&
			    d->mz.e_ss == 0 && d->mz.e_sp == 0)
			{
				// Zero program size, CS:IP, and SS:SP.
				// This is *not* an MS-DOS executable.
				isDOS = false;
			}
		}

		if (isDOS) {
			d->exeType = EXEPrivate::ExeType::MZ;
			// TODO: Check for MS-DOS device drivers?
			d->fileType = FileType::Executable;
			return;
		}
	}

	// Load the secondary header. (NE/LE/LX/PE)
	// TODO: LE/LX.
	// NOTE: NE and PE secondary headers are both 64 bytes.
	uint32_t hdr_addr = le32_to_cpu(d->mz.e_lfanew);
	if (hdr_addr < sizeof(d->mz) || hdr_addr >= (d->file->size() - sizeof(d->hdr))) {
		// PE header address is out of range.
		d->exeType = EXEPrivate::ExeType::MZ;
		return;
	}

	size = d->file->seekAndRead(hdr_addr, &d->hdr, sizeof(d->hdr));
	if (size != sizeof(d->hdr)) {
		// Seek and/or read error.
		// TODO: Check the signature first instead of
		// depending on the full union being available?
		d->exeType = EXEPrivate::ExeType::Unknown;
		d->isValid = false;
		return;
	}

	// Check the signature.
	// NOTE: MSVC handles 'PE\0\0' as 0x00504500,
	// probably due to the embedded NULL bytes.
	if (d->hdr.pe.Signature == cpu_to_be32(0x50450000) /*'PE\0\0'*/) {
		// This is a PE executable.
		// Check if it's PE or PE32+.
		// (.NET is checked in loadFieldData().)
		switch (le16_to_cpu(d->hdr.pe.OptionalHeader.Magic)) {
			case IMAGE_NT_OPTIONAL_HDR32_MAGIC:
				d->exeType = EXEPrivate::ExeType::PE;
				d->pe_subsystem = le16_to_cpu(d->hdr.pe.OptionalHeader.opt32.Subsystem);
				break;
			case IMAGE_NT_OPTIONAL_HDR64_MAGIC:
				d->exeType = EXEPrivate::ExeType::PE32PLUS;
				d->pe_subsystem = le16_to_cpu(d->hdr.pe.OptionalHeader.opt64.Subsystem);
				break;
			default:
				// Unsupported PE executable.
				d->exeType = EXEPrivate::ExeType::Unknown;
				d->isValid = false;
				return;
		}

		// Check the file type.
		const uint16_t pe_flags = le16_to_cpu(d->hdr.pe.FileHeader.Characteristics);
		if (pe_flags & IMAGE_FILE_DLL) {
			// DLL file.
			d->fileType = FileType::DLL;
		} else {
			switch (d->pe_subsystem) {
				case IMAGE_SUBSYSTEM_NATIVE:
					// TODO: IMAGE_SUBSYSTEM_NATIVE may be either a
					// device driver or boot-time executable.
					// Need to check some other flag...
					d->fileType = FileType::Executable;
					break;
				case IMAGE_SUBSYSTEM_EFI_BOOT_SERVICE_DRIVER:
				case IMAGE_SUBSYSTEM_EFI_RUNTIME_DRIVER:
					d->fileType = FileType::DeviceDriver;
					break;
				case IMAGE_SUBSYSTEM_EFI_ROM:
					d->fileType = FileType::ROM_Image;
					break;
				default:
					d->fileType = FileType::Executable;
					break;
			}
		}
	} else if (d->hdr.ne.sig == cpu_to_be16('NE') /* 'NE' */) {
		// New Executable.
		d->exeType = EXEPrivate::ExeType::NE;

		// Check if this is a resource library.
		// (All segment size values are 0.)
		// NOTE: AutoDataSegIndex is 0 for .FON, but 1 for MORICONS.DLL.
		// FIXME: ULFONT.FON has non-zero values.

		// Check 0x10-0x1F for all 0s.
		const uint32_t *const res0chk = reinterpret_cast<const uint32_t*>(&d->hdr.ne.InitHeapSize);
		if (res0chk[0] == 0 && res0chk[1] == 0 &&
		    res0chk[2] == 0 && res0chk[3] == 0)
		{
			// This is a resource library.
			// May be a font (.FON) or an icon library (.ICL, moricons.dll).
			// TODO: Check the version resource if it's present?
			d->fileType = FileType::ResourceLibrary;
			return;
		}

		// TODO: Distinguish between DLL and driver?
		if (d->hdr.ne.ApplFlags & NE_DLL) {
			d->fileType = FileType::DLL;
		} else {
			d->fileType = FileType::Executable;
		}
	} else if (d->hdr.le.sig == cpu_to_be16('LE') ||
		   d->hdr.le.sig == cpu_to_be16('LX'))
	{
		// Linear Executable.
		if (d->hdr.le.sig == cpu_to_be16('LE')) {
			d->exeType = EXEPrivate::ExeType::LE;
		} else /*if (d->hdr.le.sig == cpu_to_be16('LX'))*/ {
			d->exeType = EXEPrivate::ExeType::LX;
		}

		// TODO: Check byteorder flags and adjust as necessary.
		if (d->hdr.le.targOS == cpu_to_le16(NE_OS_WIN386)) {
			// LE VxD
			d->fileType = FileType::DeviceDriver;
		} else if (d->hdr.le.module_type_flags & cpu_to_le32(LE_MODULE_IS_DLL)) {
			// LE DLL
			d->fileType = FileType::DLL;
		} else {
			// LE EXE
			d->fileType = FileType::Executable;
		}
	} else if (d->hdr.sig16 == cpu_to_be16('W3') /* 'W3' */) {
		// W3 executable. (Collection of LE executables.)
		// Only used by WIN386.EXE.
		// TODO: Check for W4. (Compressed version of W3 used by Win9x.)
		d->exeType = EXEPrivate::ExeType::W3;
		d->fileType = FileType::Executable;
	} else {
		// Unrecognized secondary header.
		d->exeType = EXEPrivate::ExeType::MZ;
		d->fileType = FileType::Executable;
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
		return static_cast<int>(EXEPrivate::ExeType::Unknown);
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
		return static_cast<int>(EXEPrivate::ExeType::MZ);
	}

	// Not supported.
	return static_cast<int>(EXEPrivate::ExeType::Unknown);
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

	// EXE has the same name worldwide, so we can
	// ignore the region selection.
	static_assert(SYSNAME_TYPE_MASK == 3,
		"EXE::systemName() array index optimization needs to be updated.");

	static const char *const sysNames_Windows[4] = {
		"Microsoft Windows", "Windows", "Windows", nullptr
	};

	// New Executable (and Linear Executable) operating systems.
	static const char *const sysNames_NE[6][4] = {
		// NE_OS_UNKNOWN
		// NOTE: Windows 1.0 executables have this value.
		{"Microsoft Windows", "Windows", "Windows", nullptr},
		// NE_OS_OS2
		{"IBM OS/2", "OS/2", "OS/2", nullptr},
		// NE_OS_WIN
		{"Microsoft Windows", "Windows", "Windows", nullptr},
		// NE_OS_DOS4
		{"European MS-DOS 4.x", "EuroDOS 4.x", "EuroDOS 4.x", nullptr},
		// NE_OS_WIN386 (TODO)
		{"Microsoft Windows", "Windows", "Windows", nullptr},
		// NE_OS_BOSS
		{"Borland Operating System Services", "BOSS", "BOSS", nullptr},
	};

	switch (d->exeType) {
		case EXEPrivate::ExeType::MZ: {
			// DOS executable.
			static const char *const sysNames_DOS[4] = {
				"Microsoft MS-DOS", "MS-DOS", "DOS", nullptr
			};
			return sysNames_DOS[type & SYSNAME_TYPE_MASK];
		}

		case EXEPrivate::ExeType::NE: {
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
					return C_("EXE", "Unknown NE");
				}
			}
			return sysNames_NE[d->hdr.ne.targOS][type & SYSNAME_TYPE_MASK];
		}

		case EXEPrivate::ExeType::LE:
		case EXEPrivate::ExeType::LX: {
			// Linear Executable.
			// TODO: Some DOS extenders have the target OS set to OS/2.
			// Check 'file' msdos magic.
			// TODO: Byteswapping.
			const uint16_t targOS = le16_to_cpu(d->hdr.le.targOS);
			if (targOS <= NE_OS_WIN386) {
				return sysNames_NE[targOS][type & SYSNAME_TYPE_MASK];
			}
			return C_("EXE", "Unknown LE/LX");
		}

		case EXEPrivate::ExeType::W3: {
			// W3 executable. (Collection of LE executables.)
			// Only used by WIN386.EXE.
			return sysNames_Windows[type & SYSNAME_TYPE_MASK];
		}

		case EXEPrivate::ExeType::PE:
		case EXEPrivate::ExeType::PE32PLUS: {
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
					// Check the CPU type.
					static const char *const sysNames_Xbox[3][4] = {
						{"Microsoft Xbox", "Xbox", "Xbox", nullptr},
						{"Microsoft Xbox 360", "Xbox 360", "X360", nullptr},
						{"Microsoft Xbox One", "Xbox One", "Xbone", nullptr},
					};
					switch (le16_to_cpu(d->hdr.pe.FileHeader.Machine)) {
						default:
						case IMAGE_FILE_MACHINE_I386:
							// TODO: Verify for original Xbox.
							return sysNames_Xbox[0][type & SYSNAME_TYPE_MASK];
						case IMAGE_FILE_MACHINE_POWERPCBE:
							return sysNames_Xbox[1][type & SYSNAME_TYPE_MASK];
						case IMAGE_FILE_MACHINE_AMD64:
							// TODO: Verify for Xbox One.
							return sysNames_Xbox[2][type & SYSNAME_TYPE_MASK];
					}
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
	return C_("EXE", "Unknown EXE");
}

/**
 * Load field data.
 * Called by RomData::fields() if the field data hasn't been loaded yet.
 * @return Number of fields read on success; negative POSIX error code on error.
 */
int EXE::loadFieldData(void)
{
	RP_D(EXE);
	if (!d->fields->empty()) {
		// Field data *has* been loaded...
		return 0;
	} else if (!d->file || !d->file->isOpen()) {
		// File isn't open.
		return -EBADF;
	} else if (!d->isValid || (int)d->exeType < 0) {
		// Unknown ROM image type.
		return -EIO;
	}

	// Maximum number of fields:
	// - MZ: 7
	// - NE: 9
	// - PE: 8
	//   - PE Version: +6
	//   - PE Manifest: +12
	d->fields->reserve(27);

	// Executable type.
	// NOTE: Not translatable.
	static const char *const exeTypes[(int)EXEPrivate::ExeType::Max] = {
		"MS-DOS Executable",		// ExeType::MZ
		"16-bit New Executable",	// ExeType::NE
		"Mixed-Mode Linear Executable",	// ExeType::LE
		"Windows/386 Kernel",		// ExeType::W3
		"32-bit Linear Executable",	// ExeType::LX
		"32-bit Portable Executable",	// ExeType::PE
		"64-bit Portable Executable",	// ExeType::PE32PLUS
	};
	const char *const type_title = C_("EXE", "Type");
	if (d->exeType >= EXEPrivate::ExeType::MZ && d->exeType < EXEPrivate::ExeType::Max) {
		d->fields->addField_string(type_title, exeTypes[(int)d->exeType]);
	} else {
		d->fields->addField_string(type_title, C_("EXE", "Unknown"));
	}

	switch (d->exeType) {
		case EXEPrivate::ExeType::MZ:
			d->addFields_MZ();
			break;

		case EXEPrivate::ExeType::NE:
			d->addFields_NE();
			break;

		case EXEPrivate::ExeType::LE:
		case EXEPrivate::ExeType::LX:
			d->addFields_LE();
			break;

		case EXEPrivate::ExeType::PE:
		case EXEPrivate::ExeType::PE32PLUS:
			d->addFields_PE();
			break;

		default:
			// TODO: Other executable types.
			break;
	}

	// Add MZ tab for non-MZ executables
	if (d->exeType != EXEPrivate::ExeType::MZ) {
		// NOTE: This doesn't actually create a separate tab for non-implemented types.
		d->fields->addTab("MZ");
		d->addFields_MZ();
	}

	// Finished reading the field data.
	return static_cast<int>(d->fields->count());
}

/**
 * Does this ROM image have "dangerous" permissions?
 *
 * @return True if the ROM image has "dangerous" permissions; false if not.
 */
bool EXE::hasDangerousPermissions(void) const
{
#ifdef ENABLE_XML
	RP_D(const EXE);
	// PE executables only.
	if (d->exeType != EXEPrivate::ExeType::PE &&
	    d->exeType != EXEPrivate::ExeType::PE32PLUS)
	{
		// Not a PE executable.
		return false;
	}

	// Check the Windows manifest for requestedExecutionLevel == requireAdministrator.
	return d->doesExeRequireAdministrator();
#else /* !ENABLE_XML */
	// Nothing to check here, since TinyXML2 is disabled...
	return false;
#endif /* ENABLE_XML */
}

/**
 * Check for "viewed" achievements.
 *
 * @return Number of achievements unlocked.
 */
int EXE::checkViewedAchievements(void) const
{
	RP_D(const EXE);
	if (!d->isValid) {
		// EXE is not valid.
		return 0;
	}

	// Checking for PE and PE32+ only, and only for
	// Windows GUI and console programs.
	switch (d->exeType) {
		case EXEPrivate::ExeType::PE:
			// Check for .NET.
			if (d->hdr.pe.OptionalHeader.opt32.DataDirectory[IMAGE_DATA_DIRECTORY_CLR_HEADER].Size != 0) {
				// It's .NET. Ignore this.
				return 0;
			}
			break;
		case EXEPrivate::ExeType::PE32PLUS:
			if (d->hdr.pe.OptionalHeader.opt64.DataDirectory[IMAGE_DATA_DIRECTORY_CLR_HEADER].Size != 0) {
				// It's .NET. Ignore this.
				return 0;
			}
			break;
		default:
			return 0;
	}

	switch (d->pe_subsystem) {
		case IMAGE_SUBSYSTEM_WINDOWS_GUI:
		case IMAGE_SUBSYSTEM_WINDOWS_CUI:
			break;
		default:
			return 0;
	}

	// Machine type should NOT be x86, amd64, CIL (.NET),
	// or big-endian PPC (Xbox 360).
	switch (le16_to_cpu(d->hdr.pe.FileHeader.Machine)) {
		case IMAGE_FILE_MACHINE_I386:
		case IMAGE_FILE_MACHINE_AMD64:
		case IMAGE_FILE_MACHINE_CEE:
		case IMAGE_FILE_MACHINE_POWERPCBE:
			return 0;
		default:
			break;
	}

	// Achievement unlocked!
	Achievements *const pAch = Achievements::instance();
	pAch->unlock(Achievements::ID::ViewedNonX86PE);
	return 1;
}

}
