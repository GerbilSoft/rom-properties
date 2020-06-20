/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * ISO.cpp: ISO-9660 disc image parser.                                    *
 *                                                                         *
 * Copyright (c) 2019-2020 by David Korth.                                 *
 * Copyright (c) 2020 by Egor.                                             *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "ISO.hpp"
#include "../iso_structs.h"
#include "hsfs_structs.h"

// librpbase, librpfile, librpcpu
#include "common.h"
#include "librpbase/TextFuncs.hpp"
#include "libi18n/i18n.h"
#include "librpcpu/byteswap.h"
using namespace LibRpBase;
using LibRpFile::IRpFile;

// C includes. (C++ namespace)
#include <cassert>
#include <cerrno>
#include <cstring>
#include <ctime>

// C++ includes.
#include <string>
#include <vector>
using std::string;
using std::vector;

namespace LibRomData {

ROMDATA_IMPL(ISO)

class ISOPrivate : public LibRpBase::RomDataPrivate
{
	public:
		ISOPrivate(ISO *q, LibRpFile::IRpFile *file);

	private:
		typedef RomDataPrivate super;
		RP_DISABLE_COPY(ISOPrivate)

	public:
		// ISO primary volume descriptor.
		ISO_Primary_Volume_Descriptor pvd;

		// HSFS primary volume descriptor.
		HSFS_Primary_Volume_Descriptor *const hspvd;

		// Whether this is ISO 9660 or High Sierra. Set by checkPVD.
		bool high_sierra;

		// Sector size.
		// Usually 2048 or 2352.
		unsigned int sector_size;

		// Sector offset.
		// Usually 0 (for 2048) or 16 (for 2352).
		unsigned int sector_offset;

		// UDF version.
		// TODO: Descriptors?
		const char *s_udf_version;

	public:
		/**
		 * Check additional volume descirptors.
		 */
		void checkVolumeDescriptors(void);

		/**
		 * Convert an ISO PVD timestamp to UNIX time.
		 * @param pvd_time PVD timestamp
		 * @return UNIX time, or -1 if invalid or not set.
		 */
		static inline time_t pvd_time_to_unix_time(const ISO_PVD_DateTime_t *pvd_time)
		{
			// Wrapper for RomData::pvd_time_to_unix_time(),
			// which doesn't take an ISO_PVD_DateTime_t struct.
			return RomDataPrivate::pvd_time_to_unix_time(pvd_time->full, pvd_time->tz_offset);
		}

		/**
		 * Convert an HSFS PVD timestamp to UNIX time.
		 * @param pvd_time PVD timestamp
		 * @return UNIX time, or -1 if invalid or not set.
		 */
		static inline time_t pvd_time_to_unix_time(const HSFS_PVD_DateTime_t *pvd_time)
		{
			// Wrapper for RomData::pvd_time_to_unix_time(),
			// which doesn't take an HSFS_PVD_DateTime_t struct.
			return RomDataPrivate::pvd_time_to_unix_time(pvd_time->full, 0);
		}

		/**
		 * Add fields common to HSFS and ISO-9660 (except timestamps)
		 * @param pvd PVD
		 */
		template<typename T>
		void addPVDCommon(const T *pvd);

		/**
		 * Add timestamp fields from PVD
		 * @param pvd PVD
		 */
		template<typename T>
		void addPVDTimestamps(const T *pvd);

		/**
		 * Check PVD and determine whether it is High Sierra or not.
		 * @return whether the PVD is valid
		 */
		bool checkPVD(void);
};

/** ISOPrivate **/

ISOPrivate::ISOPrivate(ISO *q, IRpFile *file)
	: super(q, file)
	, hspvd(reinterpret_cast<HSFS_Primary_Volume_Descriptor*>(&pvd))
	, high_sierra(false)
	, sector_size(0)
	, sector_offset(0)
	, s_udf_version(nullptr)
{
	// Clear the disc header structs.
	memset(&pvd, 0, sizeof(pvd));
}

/**
 * Check additional volume descirptors.
 */
void ISOPrivate::checkVolumeDescriptors(void)
{
	// Check for additional descriptors.
	// First, we want to find the volume descriptor terminator.
	// TODO: Boot record?

	// Starting address.
	off64_t addr = (ISO_PVD_LBA * static_cast<off64_t>(sector_size)) + sector_offset;
	const off64_t maxaddr = 0x100 * static_cast<off64_t>(sector_size);

	ISO_Volume_Descriptor_Header deschdr;
	bool foundVDT = false;
	while (addr < maxaddr) {
		addr += sector_size;
		size_t size = file->seekAndRead(addr, &deschdr, sizeof(deschdr));
		if (size != sizeof(deschdr)) {
			// Seek and/or read error.
			break;
		}

		if (memcmp(deschdr.identifier, ISO_VD_MAGIC, sizeof(deschdr.identifier)) != 0) {
			// Incorrect identifier.
			break;
		}

		if (deschdr.type == ISO_VDT_TERMINATOR) {
			// Found the terminator.
			foundVDT = true;
			break;
		}
	}
	if (!foundVDT) {
		// No terminator...
		return;
	}

	// Check for a UDF extended descriptor section.
	addr += sector_size;
	size_t size = file->seekAndRead(addr, &deschdr, sizeof(deschdr));
	if (size != sizeof(deschdr)) {
		// Seek and/or read error.
		return;
	}
	if (memcmp(deschdr.identifier, UDF_VD_BEA01, sizeof(deschdr.identifier)) != 0) {
		// Not an extended descriptor section.
		return;
	}

	// Look for NSR02/NSR03.
	while (addr < maxaddr) {
		addr += sector_size;
		size_t size = file->seekAndRead(addr, &deschdr, sizeof(deschdr));
		if (size != sizeof(deschdr)) {
			// Seek and/or read error.
			break;
		}

		if (!memcmp(deschdr.identifier, "NSR0", 4)) {
			// Found an NSR descriptor.
			switch (deschdr.identifier[4]) {
				case '1':
					s_udf_version = "1.00";
					break;
				case '2':
					s_udf_version = "1.50";
					break;
				case '3':
					s_udf_version = "2.00";
					break;
				default:
					s_udf_version = nullptr;
			}
			break;
		}

		if (!memcmp(deschdr.identifier, UDF_VD_TEA01, sizeof(deschdr.identifier))) {
			// End of extended descriptor section.
			break;
		}
	}

	// Done reading UDF for now.
	// TODO: More descriptors?
}

/**
 * Add fields common to HSFS and ISO-9660 (except timestamps)
 * @param pvd PVD
 */
template<typename T>
void ISOPrivate::addPVDCommon(const T *pvd)
{
	// System ID
	fields->addField_string(C_("ISO", "System ID"),
		latin1_to_utf8(pvd->sysID, sizeof(pvd->sysID)),
		RomFields::STRF_TRIM_END);

	// Volume ID
	fields->addField_string(C_("ISO", "Volume ID"),
		latin1_to_utf8(pvd->volID, sizeof(pvd->volID)),
		RomFields::STRF_TRIM_END);

	// Size of volume
	fields->addField_string(C_("ISO", "Volume Size"),
		formatFileSize(
			static_cast<off64_t>(pvd->volume_space_size.he) *
			static_cast<off64_t>(pvd->logical_block_size.he)));

	// TODO: Show block size?

	// Disc number
	if (pvd->volume_seq_number.he != 0 && pvd->volume_set_size.he > 1) {
		const char *const disc_number_title = C_("RomData", "Disc #");
		fields->addField_string(disc_number_title,
			// tr: Disc X of Y (for multi-disc games)
			rp_sprintf_p(C_("RomData|Disc", "%1$u of %2$u"),
				pvd->volume_seq_number.he,
				pvd->volume_set_size.he));
	}

	// Volume set ID
	fields->addField_string(C_("ISO", "Volume Set"),
		latin1_to_utf8(pvd->volume_set_id, sizeof(pvd->volume_set_id)),
		RomFields::STRF_TRIM_END);

	// Publisher
	fields->addField_string(C_("ISO", "Publisher"),
		latin1_to_utf8(pvd->publisher, sizeof(pvd->publisher)),
		RomFields::STRF_TRIM_END);

	// Data Preparer
	fields->addField_string(C_("ISO", "Data Preparer"),
		latin1_to_utf8(pvd->data_preparer, sizeof(pvd->data_preparer)),
		RomFields::STRF_TRIM_END);

	// Application
	fields->addField_string(C_("ISO", "Application"),
		latin1_to_utf8(pvd->application, sizeof(pvd->application)),
		RomFields::STRF_TRIM_END);

	// Copyright file
	fields->addField_string(C_("ISO", "Copyright File"),
		latin1_to_utf8(pvd->copyright_file, sizeof(pvd->copyright_file)),
		RomFields::STRF_TRIM_END);

	// Abstract file
	fields->addField_string(C_("ISO", "Abstract File"),
		latin1_to_utf8(pvd->abstract_file, sizeof(pvd->abstract_file)),
		RomFields::STRF_TRIM_END);
}

/**
 * Add timestamp fields from PVD
 * @param pvd PVD
 */
template<typename T>
void ISOPrivate::addPVDTimestamps(const T *pvd)
{
	// TODO: Show the original timezone?
	// For now, converting to UTC and showing as local time.

	// Volume creation time
	fields->addField_dateTime(C_("ISO", "Creation Time"),
		pvd_time_to_unix_time(&pvd->btime),
		RomFields::RFT_DATETIME_HAS_DATE |
		RomFields::RFT_DATETIME_HAS_TIME);

	// Volume modification time
	fields->addField_dateTime(C_("ISO", "Modification Time"),
		pvd_time_to_unix_time(&pvd->mtime),
		RomFields::RFT_DATETIME_HAS_DATE |
		RomFields::RFT_DATETIME_HAS_TIME);

	// Volume expiration time
	fields->addField_dateTime(C_("ISO", "Expiration Time"),
		pvd_time_to_unix_time(&pvd->exptime),
		RomFields::RFT_DATETIME_HAS_DATE |
		RomFields::RFT_DATETIME_HAS_TIME);

	// Volume effective time
	fields->addField_dateTime(C_("ISO", "Effective Time"),
		pvd_time_to_unix_time(&pvd->efftime),
		RomFields::RFT_DATETIME_HAS_DATE |
		RomFields::RFT_DATETIME_HAS_TIME);
}

/**
 * Check PVD and determine whether it is High Sierra or not.
 * @return whether the PVD is valid
 */
bool ISOPrivate::checkPVD(void)
{
	if (pvd.header.type == ISO_VDT_PRIMARY &&
	    pvd.header.version == ISO_VD_VERSION &&
	    !memcmp(pvd.header.identifier, ISO_VD_MAGIC, sizeof(pvd.header.identifier)))
	{
		high_sierra = false;
		return true;
	} else if (hspvd->header.type == ISO_VDT_PRIMARY &&
	    hspvd->header.version == HSFS_VD_VERSION &&
	    !memcmp(hspvd->header.identifier, HSFS_VD_MAGIC, sizeof(hspvd->header.identifier)))
	{
		high_sierra = true;
		return true;
	} else {
		return false;
	}
}

/** ISO **/

/**
 * Read an ISO-9660 disc image.
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
ISO::ISO(IRpFile *file)
	: super(new ISOPrivate(this, file))
{
	// This class handles disc images.
	RP_D(ISO);
	d->className = "ISO";
	d->mimeType = "application/x-cd-image";	// unofficial [TODO: Others?]
	d->fileType = FTYPE_DISC_IMAGE;

	if (!d->file) {
		// Could not ref() the file handle.
		return;
	}

	// Read the PVD. (2048-byte sector address)
	size_t size = d->file->seekAndRead(ISO_PVD_ADDRESS_2048 + ISO_DATA_OFFSET_MODE1_COOKED,
		&d->pvd, sizeof(d->pvd));
	if (size != sizeof(d->pvd)) {
		// Seek and/or read error.
		d->file->unref();
		d->file = nullptr;
		return;
	}

	// Check if the PVD is valid.
	// NOTE: Not using isRomSupported_static(), since this function
	// only checks the file extension.
	if (d->checkPVD()) {
		// Found the PVD using 2048-byte sectors.
		d->sector_size = ISO_SECTOR_SIZE_MODE1_COOKED;
		d->sector_offset = ISO_DATA_OFFSET_MODE1_COOKED;
	} else {
		// Try again using 2352-byte sectors.
		size = d->file->seekAndRead(ISO_PVD_ADDRESS_2352 + ISO_DATA_OFFSET_MODE1_RAW,
			&d->pvd, sizeof(d->pvd));
		if (size != sizeof(d->pvd)) {
			// Seek and/or read error.
			d->file->unref();
			d->file = nullptr;
			return;
		}

		if (d->checkPVD()) {
			// Found the PVD using 2352-byte sectors.
			d->sector_size = ISO_SECTOR_SIZE_MODE1_RAW;
			d->sector_offset = ISO_DATA_OFFSET_MODE1_RAW;
		} else {
			// Not a PVD.
			d->file->unref();
			d->file = nullptr;
			return;
		}
	}

	// This is a valid PVD.
	d->isValid = true;

	// Check for additional volume descriptors.
	if (!d->high_sierra)
		d->checkVolumeDescriptors();
}

/** ROM detection functions. **/

/**
 * Is a ROM image supported by this class?
 * @param info DetectInfo containing ROM detection information.
 * @return Class-specific system ID (>= 0) if supported; -1 if not.
 */
int ISO::isRomSupported_static(const DetectInfo *info)
{
	// NOTE: Only checking for supported file extensions.
	assert(info->ext != nullptr);
	if (!info->ext) {
		// No file extension specified...
		return -1;
	}

	const char *const *exts = supportedFileExtensions_static();
	for (; *exts != nullptr; exts++) {
		if (!strcasecmp(info->ext, *exts)) {
			// Found a match.
			return 0;
		}
	}

	// No match.
	return -1;
}

/**
 * Get the name of the system the loaded ROM is designed for.
 * @param type System name type. (See the SystemName enum.)
 * @return System name, or nullptr if type is invalid.
 */
const char *ISO::systemName(unsigned int type) const
{
	RP_D(const ISO);
	if (!d->isValid || !isSystemNameTypeValid(type))
		return nullptr;

	// ISO-9660 has the same name worldwide, so we can
	// ignore the region selection.
	// TODO: Identify the OS, or list that in the fields instead?
	static_assert(SYSNAME_TYPE_MASK == 3,
		"ISO::systemName() array index optimization needs to be updated.");

	// TODO: UDF, HFS, others?
	static const char *const sysNames[8] = {
		"ISO-9660", "ISO", "ISO", nullptr,
		"High Sierra Format", "High Sierra", "HSF", nullptr,
	};

	return sysNames[(type & SYSNAME_TYPE_MASK) | (d->high_sierra ? 4 : 0)];
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
const char *const *ISO::supportedFileExtensions_static(void)
{
	static const char *const exts[] = {
		".iso",		// ISO
		".iso9660",	// ISO (listed in shared-mime-info)
		".bin",		// BIN (2352-byte)
		".xiso",	// Xbox ISO image
		// TODO: More?
		// TODO: Is there a separate extension for High Sierra?

		nullptr
	};
	return exts;
}

/**
 * Get a list of all supported MIME types.
 * This is to be used for metadata extractors that
 * must indicate which MIME types they support.
 *
 * NOTE: The array and the strings in the array should
 * *not* be freed by the caller.
 *
 * @return NULL-terminated array of all supported file extensions, or nullptr on error.
 */
const char *const *ISO::supportedMimeTypes_static(void)
{
	static const char *const mimeTypes[] = {
		// Unofficial MIME types from FreeDesktop.org.
		"application/x-cd-image",
		"application/x-iso9660-image",

		// TODO: BIN (2352)?
		// TODO: Is there a separate MIME for High Sierra?
		nullptr
	};
	return mimeTypes;
}

/**
 * Load field data.
 * Called by RomData::fields() if the field data hasn't been loaded yet.
 * @return Number of fields read on success; negative POSIX error code on error.
 */
int ISO::loadFieldData(void)
{
	RP_D(ISO);
	if (!d->fields->empty()) {
		// Field data *has* been loaded...
		return 0;
	} else if (!d->file || !d->file->isOpen()) {
		// File isn't open.
		return -EBADF;
	} else if (!d->isValid) {
		// Unsupported file.
		return -EIO;
	}

	// ISO-9660 Primary Volume Descriptor.
	// TODO: Other descriptors?
	const ISO_Primary_Volume_Descriptor *const pvd = &d->pvd;
	const HSFS_Primary_Volume_Descriptor *const hspvd = d->hspvd;
	d->fields->reserve(16);	// Maximum of 16 fields.

	// NOTE: All fields are space-padded. (0x20, ' ')
	// TODO: ascii_to_utf8()?

	// ISO-9660 PVD
	d->fields->setTabName(0, d->high_sierra ? C_("ISO", "High Sierra PVD")
	                                        : C_("ISO", "ISO-9660 PVD"));
	// Common fields between HSFS and ISO
	if (d->high_sierra)
		d->addPVDCommon(hspvd);
	else
		d->addPVDCommon(pvd);

	// Bibliographic file
	if (!d->high_sierra)
		d->fields->addField_string(C_("ISO", "Bibliographic File"),
			latin1_to_utf8(pvd->bibliographic_file, sizeof(pvd->bibliographic_file)),
			RomFields::STRF_TRIM_END);

	// Timestamps
	if (d->high_sierra)
		d->addPVDTimestamps(hspvd);
	else
		d->addPVDTimestamps(pvd);

	if (d->s_udf_version) {
		// UDF version.
		// TODO: Parse the UDF volume descriptors and
		// show a separate tab for UDF?
		d->fields->addField_string(C_("ISO", "UDF Version"),
			d->s_udf_version);
	}

	// Finished reading the field data.
	return static_cast<int>(d->fields->count());
}

}
