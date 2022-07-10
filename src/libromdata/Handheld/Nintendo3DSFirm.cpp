/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * Nintendo3DSFirm.hpp: Nintendo 3DS firmware reader.                      *
 *                                                                         *
 * Copyright (c) 2016-2022 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "Nintendo3DSFirm.hpp"
#include "n3ds_firm_structs.h"
#include "data/Nintendo3DSFirmData.hpp"

// librpbase, librpfile
using namespace LibRpBase;
using LibRpFile::IRpFile;

// for memmem() if it's not available in <string.h>
#include "librpbase/TextFuncs_libc.h"

// C++ STL classes.
using std::string;
using std::unique_ptr;
using std::vector;

// zlib for crc32()
#include <zlib.h>
#ifdef _MSC_VER
// MSVC: Exception handling for /DELAYLOAD.
#  include "libwin32common/DelayLoadHelper.h"
#endif /* _MSC_VER */

namespace LibRomData {

#ifdef _MSC_VER
// DelayLoad test implementation.
DELAYLOAD_TEST_FUNCTION_IMPL0(get_crc_table);
#endif /* _MSC_VER */

class Nintendo3DSFirmPrivate final : public RomDataPrivate
{
	public:
		Nintendo3DSFirmPrivate(Nintendo3DSFirm *q, IRpFile *file);

	private:
		typedef RomDataPrivate super;
		RP_DISABLE_COPY(Nintendo3DSFirmPrivate)

	public:
		/** RomDataInfo **/
		static const char8_t *const exts[];
		static const char *const mimeTypes[];
		static const RomDataInfo romDataInfo;

	public:
		// Firmware header.
		// NOTE: Must be byteswapped on access.
		N3DS_FIRM_Header_t firmHeader;
};

ROMDATA_IMPL(Nintendo3DSFirm)

/** Nintendo3DSFirmPrivate **/

/* RomDataInfo */
const char8_t *const Nintendo3DSFirmPrivate::exts[] = {
	U8(".firm"),	// boot9strap
	U8(".bin"),	// older

	nullptr
};
const char *const Nintendo3DSFirmPrivate::mimeTypes[] = {
	// Unofficial MIME types.
	// TODO: Get these upstreamed on FreeDesktop.org.
	"application/x-nintendo-3ds-firm",

	nullptr
};
const RomDataInfo Nintendo3DSFirmPrivate::romDataInfo = {
	"Nintendo3DSFirm", exts, mimeTypes
};

Nintendo3DSFirmPrivate::Nintendo3DSFirmPrivate(Nintendo3DSFirm *q, IRpFile *file)
	: super(q, file, &romDataInfo)
{
	// Clear the various structs.
	memset(&firmHeader, 0, sizeof(firmHeader));
}

/** Nintendo3DSFirm **/

/**
 * Read a Nintendo 3DS firmware binary.
 *
 * A ROM image must be opened by the caller. The file handle
 * will be ref()'d and must be kept open in order to load
 * data from the ROM image.
 *
 * To close the file, either delete this object or call close().
 *
 * NOTE: Check isValid() to determine if this is a valid ROM.
 *
 * @param file Open ROM image.
 */
Nintendo3DSFirm::Nintendo3DSFirm(IRpFile *file)
	: super(new Nintendo3DSFirmPrivate(this, file))
{
	RP_D(Nintendo3DSFirm);
	d->mimeType = "application/x-nintendo-3ds-firm";	// unofficial, not on fd.o
	d->fileType = FileType::FirmwareBinary;

	if (!d->file) {
		// Could not ref() the file handle.
		return;
	}

	// Read the firmware header.
	d->file->rewind();
	size_t size = d->file->read(&d->firmHeader, sizeof(d->firmHeader));
	if (size != sizeof(d->firmHeader)) {
		UNREF_AND_NULL_NOCHK(d->file);
		return;
	}

	// Check if this firmware binary is supported.
	const DetectInfo info = {
		{0, sizeof(d->firmHeader), reinterpret_cast<const uint8_t*>(&d->firmHeader)},
		nullptr,	// ext (not needed for Nintendo3DSFirm)
		0		// szFile (not needed for Nintendo3DSFirm)
	};
	d->isValid = (isRomSupported_static(&info) >= 0);

	if (!d->isValid) {
		UNREF_AND_NULL_NOCHK(d->file);
	}
}

/**
 * Is a ROM image supported by this class?
 * @param info DetectInfo containing ROM detection information.
 * @return Class-specific system ID (>= 0) if supported; -1 if not.
 */
int Nintendo3DSFirm::isRomSupported_static(const DetectInfo *info)
{
	assert(info != nullptr);
	assert(info->header.pData != nullptr);
	assert(info->header.addr == 0);
	if (!info || !info->header.pData ||
	    info->header.addr != 0 ||
	    info->header.size < sizeof(N3DS_FIRM_Header_t))
	{
		// Either no detection information was specified,
		// or the header is too small.
		return -1;
	}

	// Check the FIRM magic.
	// TODO: Other checks?
	const N3DS_FIRM_Header_t *const firmHeader =
		reinterpret_cast<const N3DS_FIRM_Header_t*>(info->header.pData);
	if (firmHeader->magic == cpu_to_be32(N3DS_FIRM_MAGIC)) {
		// This is a FIRM binary.
		return 0;
	}

	// Not supported.
	return -1;
}

/**
 * Get the name of the system the loaded ROM is designed for.
 * @param type System name type. (See the SystemName enum.)
 * @return System name, or nullptr if type is invalid.
 */
const char *Nintendo3DSFirm::systemName(unsigned int type) const
{
	RP_D(const Nintendo3DSFirm);
	if (!d->isValid || !isSystemNameTypeValid(type))
		return nullptr;

	// Nintendo 3DS has the same name worldwide, so we can
	// ignore the region selection.
	static_assert(SYSNAME_TYPE_MASK == 3,
		"Nintendo3DSFirm::systemName() array index optimization needs to be updated.");

	// Bits 0-1: Type. (long, short, abbreviation)
	// TODO: *New* Nintendo 3DS for N3DS-exclusive titles; iQue for China.
	static const char *const sysNames[4] = {
		"Nintendo 3DS", "Nintendo 3DS", "3DS", nullptr
	};

	return sysNames[type & SYSNAME_TYPE_MASK];
}

/**
 * Load field data.
 * Called by RomData::fields() if the field data hasn't been loaded yet.
 * @return Number of fields read on success; negative POSIX error code on error.
 */
int Nintendo3DSFirm::loadFieldData(void)
{
	RP_D(Nintendo3DSFirm);
	if (!d->fields->empty()) {
		// Field data *has* been loaded...
		return 0;
	} else if (!d->file) {
		// File isn't open.
		return -EBADF;
	} else if (!d->isValid) {
		// Firmware binary isn't valid.
		return -EIO;
	}

	// Nintendo 3DS firmware binary header.
	const N3DS_FIRM_Header_t *const firmHeader = &d->firmHeader;
	d->fields->reserve(6);	// Maximum of 6 fields.

	// Read the firmware binary.
	unique_ptr<uint8_t[]> firmBuf;
	unsigned int szFile = 0;
	if (d->file->size() <= 4*1024*1024) {
		// Firmware binary is 4 MB or less.
		szFile = static_cast<unsigned int>(d->file->size());
		firmBuf.reset(new uint8_t[szFile]);
		d->file->rewind();
		size_t size = d->file->read(firmBuf.get(), szFile);
		if (size != szFile) {
			// Error reading the firmware binary.
			firmBuf.reset();
		}
	}

	// If both ARM11 and ARM9 entry points are non-zero,
	// check if this is an official 3DS firmware binary.
	const Nintendo3DSFirmData::FirmBin_t *firmBin = nullptr;
	const uint32_t arm11_entrypoint = le32_to_cpu(firmHeader->arm11_entrypoint);
	const uint32_t arm9_entrypoint = le32_to_cpu(firmHeader->arm9_entrypoint);
	const char8_t *firmBinDesc = nullptr;
	bool checkCustomFIRM = false;	// Check for a custom FIRM, e.g. Boot9Strap.
	bool checkARM9 = false;		// Check for ARM9 homebrew.
	if (arm11_entrypoint != 0 && arm9_entrypoint != 0) {
#if defined(_MSC_VER) && defined(ZLIB_IS_DLL)
		// Delay load verification.
		// TODO: Only if linked with /DELAYLOAD?
		bool has_zlib = true;
		if (DelayLoad_test_get_crc_table() != 0) {
			// Delay load failed.
			// Can't calculate the CRC32.
			has_zlib = false;
		}
#else /* !defined(_MSC_VER) || !defined(ZLIB_IS_DLL) */
		// zlib isn't in a DLL, but we need to ensure that the
		// CRC table is initialized anyway.
		static const bool has_zlib = true;
		get_crc_table();
#endif /* defined(_MSC_VER) && defined(ZLIB_IS_DLL) */

		// Calculate the CRC32 and look it up.
		if (has_zlib && firmBuf) {
			const uint32_t crc = crc32(0, firmBuf.get(), static_cast<unsigned int>(szFile));
			firmBin = Nintendo3DSFirmData::lookup_firmBin(crc);
			if (firmBin != nullptr) {
				// Official firmware binary.
				firmBinDesc = (firmBin->isNew3DS ? U8("New3DS FIRM") : U8("Old3DS FIRM"));
			} else {
				// Check for a custom FIRM.
				checkCustomFIRM = true;

				// NOTE: Luma3DS v9.1 has an ARM11 entry point set,
				// so we should check for ARM9 homebrew as well.
				checkARM9 = true;
			}
		}
	} else if (arm11_entrypoint == 0 && arm9_entrypoint != 0) {
		// ARM9 homebrew.
		firmBinDesc = C_("Nintendo3DSFirm", "ARM9 Homebrew");
		checkARM9 = true;
	} else if (arm11_entrypoint != 0 && arm9_entrypoint == 0) {
		// ARM11 homebrew. (Not a thing...)
		firmBinDesc = C_("Nintendo3DSFirm", "ARM11 Homebrew");
	}

	if (checkCustomFIRM) {
		// TODO: Split into a separate function?

		// Check for "B9S" at 0x3D.
		if (!memcmp(&firmHeader->reserved[0x2D], "B9S", 3)) {
			// This is Boot9Strap.
			firmBinDesc = U8("Boot9Strap");
		} else if (firmBuf) {
			// Check for sighax installer.
			// NOTE: String has a NULL terminator.
			static const char sighax_magic[] = "3DS BOOTHAX INS";
			if (!memcmp(&firmBuf[0x208], sighax_magic, sizeof(sighax_magic))) {
				// Found derrek's sighax installer.
				firmBinDesc = U8("sighax installer");
			}
		}
	}

	if (firmBin) {
		// Official firmware binary fields.
		d->fields->addField_string(C_("Nintendo3DSFirm", "Type"),
			(firmBinDesc ? firmBinDesc : C_("RomData", "Unknown")));

		// FIRM version.
		d->fields->addField_string(C_("Nintendo3DSFirm", "FIRM Version"),
			rp_sprintf("%u.%u.%u", firmBin->kernel.major,
				firmBin->kernel.minor, firmBin->kernel.revision));

		// System version.
		d->fields->addField_string(C_("Nintendo3DSFirm", "System Version"),
			rp_sprintf("%u.%u", firmBin->sys.major, firmBin->sys.minor));
	} else if (checkARM9) {
		// Check for ARM9 homebrew.

		// Version strings.
		static const struct {
			const char *title;	// Application title.
			const char *searchstr;	// Search string.
			unsigned int searchlen;	// Search string length, without the NULL terminator.
		} arm9VerStr_tbl[] = {
			{"Luma3DS",		"Luma3DS v", 9},
			{"GodMode9",		"GodMode9 Explorer v", 19},	// Older versions
			{"GodMode9",		"GodMode9 v", 10},		// Newer versions (v1.9.1; TODO check for first one?)
			{"Decrypt9WIP",		"Decrypt9WIP (", 13},
			{"Hourglass9",		"Hourglass9 v", 12},
			{"ntrboot_flasher",	"ntrboot_flasher: %s", 19},	// version info isn't hard-coded
			{"SafeB9SInstaller",	"SafeB9SInstaller v", 18},
			{"OpenFirmInstaller",	"OpenFirmInstaller v", 19},
			{"fastboot3DS",		"fastboot3DS v", 13},
		};

		const char *arm9VerStr_title = nullptr;
		string s_verstr;
		for (const auto &p : arm9VerStr_tbl) {
			const char *verstr = static_cast<const char*>(memmem(
				firmBuf.get(), szFile, p.searchstr, p.searchlen));
			if (!verstr)
				continue;

			arm9VerStr_title = p.title;

			// Version does NOT include the 'v' character.
			verstr += p.searchlen;
			const char *end = (const char*)firmBuf.get() + szFile;
			int count = 0;
			while (verstr < end && count < 32 && verstr[count] != 0 &&
			       !ISSPACE(verstr[count]) && verstr[count] != ')')
			{
				count++;
			}
			if (count > 0) {
				// Make sure this is labeled as ARM9 homebrew.
				firmBinDesc = C_("Nintendo3DSFirm", "ARM9 Homebrew");

				// NOTE: Most ARM9 homebrew uses UTF-8 strings.
				s_verstr.assign(verstr, count);
			}

			// We're done here.
			break;
		}

		// Sighax status.
		// TODO: If it's SPI, we need to decrypt the FIRM contents.
		// Reference: https://github.com/TuxSH/firmtool/blob/master/firmtool/__main__.py
		const uint32_t first4 = be32_to_cpu(*(reinterpret_cast<const uint32_t*>(firmHeader->signature)));
		static const struct {
			uint32_t first4;
			char8_t status[12];
		} sighaxStatus_tbl[] = {
			{0xB6724531,	U8("NAND retail")},		// SciresM
			{0x6EFF209C,	U8("NAND retail")},		// sighax.com
			{0x88697CDC,	U8("NAND devkit")},		// SciresM

			{0x6CF52F89,	U8("NCSD retail")},
			{0x53CB0E4E,	U8("NCSD devkit")},

			{0x37E96B10,	U8("SPI retail")},
			{0x18722BC7,	U8("SPI devkit")},
		};

		const char8_t *s_sighax_status = nullptr;
		for (const auto &p : sighaxStatus_tbl) {
			if (p.first4 == first4) {
				s_sighax_status = p.status;
				break;
			}
		}

		if (s_sighax_status) {
			// Sighaxed. Assume it's ARM9 homebrew.
			firmBinDesc = C_("Nintendo3DSFirm", "ARM9 Homebrew");
		} else {
			// Not sighaxed.
			s_sighax_status = C_("Nintendo3DSFirm", "Not sighaxed");
		}

		// Add the firmware type field.
		d->fields->addField_string(C_("Nintendo3DSFirm", "Type"),
			(firmBinDesc ? firmBinDesc : C_("RomData", "Unknown")));

		if (arm9VerStr_title) {
			d->fields->addField_string(C_("RomData", "Title"), arm9VerStr_title);
		}

		// If the version was found, add it.
		if (!s_verstr.empty()) {
			d->fields->addField_string(C_("RomData", "Version"), s_verstr);
		}

		d->fields->addField_string(C_("Nintendo3DSFirm", "Sighax Status"), s_sighax_status);
	} else {
		// Add the firmware type field.
		d->fields->addField_string(C_("Nintendo3DSFirm", "Type"),
			(firmBinDesc ? firmBinDesc : C_("RomData", "Unknown")));
	}

	// Entry Points
	if (arm11_entrypoint != 0) {
		d->fields->addField_string_numeric(C_("Nintendo3DSFirm", "ARM11 Entry Point"),
			arm11_entrypoint, RomFields::Base::Hex, 8, RomFields::STRF_MONOSPACE);
	}
	if (arm9_entrypoint != 0) {
		d->fields->addField_string_numeric(C_("Nintendo3DSFirm", "ARM9 Entry Point"),
			arm9_entrypoint, RomFields::Base::Hex, 8, RomFields::STRF_MONOSPACE);
	}

	// Finished reading the field data.
	return static_cast<int>(d->fields->count());
}

}
