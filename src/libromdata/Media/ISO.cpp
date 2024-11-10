/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * ISO.cpp: ISO-9660 disc image parser.                                    *
 *                                                                         *
 * Copyright (c) 2019-2024 by David Korth.                                 *
 * Copyright (c) 2020 by Egor.                                             *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "ISO.hpp"

#include "../cdrom_structs.h"
#include "../iso_structs.h"
#include "hsfs_structs.h"

// Other rom-properties libraries
#include "librpbase/Achievements.hpp"
#include "libi18n/i18n.h"
using namespace LibRpBase;
using namespace LibRpFile;
using namespace LibRpText;

// C++ STL classes
using std::array;
using std::string;
using std::vector;

namespace LibRomData {

class ISOPrivate final : public RomDataPrivate
{
public:
	explicit ISOPrivate(const IRpFilePtr &file);

private:
	typedef RomDataPrivate super;
	RP_DISABLE_COPY(ISOPrivate)

public:
	/** RomDataInfo **/
	static const char *const exts[];
	static const char *const mimeTypes[];
	static const RomDataInfo romDataInfo;

public:
	// Disc type
	enum class DiscType {
		Unknown = -1,

		ISO9660 = 0,
		HighSierra = 1,
		CDi = 2,

		Max
	};
	DiscType discType;

	// Primary volume descriptor
	union {
		ISO_Primary_Volume_Descriptor iso;	// ISO-9660
		HSFS_Primary_Volume_Descriptor hsfs;	// High Sierra
		uint8_t data[ISO_SECTOR_SIZE_MODE1_COOKED];
	} pvd;

	// Sector size
	// Usually 2048 or 2352. (2448 if subchannels are present)
	unsigned int sector_size;

	// Sector offset
	// Usually 0 (for 2048) or 16 (for 2352 or 2448).
	unsigned int sector_offset;

	// UDF version
	// TODO: Descriptors?
	const char *s_udf_version;

public:
	// El Torito boot catalog LBA. (present if non-zero)
	uint32_t boot_catalog_LBA;

	// TODO: Print more comprehensive boot information?
	// For now, just listing boot image types. (x86, EFI)
	enum BootPlatform {
		BOOT_PLATFORM_x86	= (1U << 0),
		BOOT_PLATFORM_EFI	= (1U << 1),
	};
	uint32_t boot_platforms;

public:
	/**
	 * Check additional volume descirptors.
	 */
	void checkVolumeDescriptors(void);

	/**
	 * Read the El Torito boot catalog.
	 * @param lba Boot catalog LBA
	 */
	void readBootCatalog(uint32_t lba);

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
	 * Add metadata properties common to HSFS and ISO-9660 (except timestamps)
	 * @param metaData RomMetaData object.
	 * @param pvd PVD
	 */
	template<typename T>
	static void addPVDCommon_metaData(RomMetaData *metaData, const T *pvd);

	/**
	 * Add timestamp metadata properties from PVD
	 * @param metaData RomMetaData object.
	 * @param pvd PVD
	 */
	template<typename T>
	static void addPVDTimestamps_metaData(RomMetaData *metaData, const T *pvd);

	/**
	 * Check the PVD and determine its type.
	 * @return DiscType value. (DiscType::Unknown if not valid)
	 */
	inline DiscType checkPVD(void) const
	{
		return static_cast<DiscType>(ISO::checkPVD(pvd.data));
	}

	/**
	 * Get the host-endian version of an LSB/MSB 16-bit value.
	 * @param lm16 LSB/MSB 16-bit value.
	 * @return Host-endian value.
	 */
	inline uint16_t host16(const uint16_lsb_msb_t &lm16) const
	{
		return (likely(discType != DiscType::CDi) ? lm16.he : be16_to_cpu(lm16.be));
	}

	/**
	 * Get the host-endian version of an LSB/MSB 32-bit value.
	 * @param lm32 LSB/MSB 32-bit value.
	 * @return Host-endian value.
	 */
	inline uint32_t host32(const uint32_lsb_msb_t &lm32) const
	{
		return (likely(discType != DiscType::CDi) ? lm32.he : be16_to_cpu(lm32.be));
	}
};

ROMDATA_IMPL(ISO)

/** ISOPrivate **/

/* RomDataInfo */
const char *const ISOPrivate::exts[] = {
	".iso",		// ISO
	".iso9660",	// ISO (listed in shared-mime-info)
	".bin",		// BIN (2352-byte)
	".xiso",	// Xbox ISO image
	".img",		// CCD/IMG
	// TODO: More?
	// TODO: Is there a separate extension for High Sierra or CD-i?

	nullptr
};
const char *const ISOPrivate::mimeTypes[] = {
	// Vendor-specific MIME types from FreeDesktop.org.
	"application/vnd.efi.iso",

	// Unofficial MIME types from FreeDesktop.org.
	"application/x-cd-image",
	"application/x-iso9660-image",

	// TODO: BIN (2352)?
	// TODO: Is there a separate MIME for High Sierra or CD-i?
	nullptr
};
const RomDataInfo ISOPrivate::romDataInfo = {
	"ISO", exts, mimeTypes
};

ISOPrivate::ISOPrivate(const IRpFilePtr &file)
	: super(file, &romDataInfo)
	, discType(DiscType::Unknown)
	, sector_size(0)
	, sector_offset(0)
	, s_udf_version(nullptr)
	, boot_catalog_LBA(0)
	, boot_platforms(0)
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

	ISO_Volume_Descriptor vd;
	uint32_t boot_LBA = 0;
	bool foundTerminator = false;
	do {
		addr += sector_size;
		size_t size = file->seekAndRead(addr, &vd, sizeof(vd));
		if (size != sizeof(vd)) {
			// Seek and/or read error.
			break;
		}

		if (memcmp(vd.header.identifier, ISO_VD_MAGIC, sizeof(vd.header.identifier)) != 0) {
			// Incorrect identifier.
			break;
		}

		switch (vd.header.type) {
			case ISO_VDT_TERMINATOR:
				// Found the terminator.
				foundTerminator = true;
				break;
			case ISO_VDT_BOOT_RECORD:
				if (boot_LBA != 0)
					break;
				// Check if this is El Torito.
				if (!strcmp(vd.boot.sysID, ISO_EL_TORITO_BOOT_SYSTEM_ID)) {
					// This is El Torito.
					boot_LBA = le32_to_cpu(vd.boot.boot_catalog_addr);
				}
				break;
			default:
				break;
		}
	} while (!foundTerminator && addr < maxaddr);
	if (!foundTerminator) {
		// No terminator...
		return;
	}

	if (boot_LBA != 0) {
		// Read the boot catalog.
		readBootCatalog(boot_LBA);
	}

	// Check for a UDF extended descriptor section.
	addr += sector_size;
	size_t size = file->seekAndRead(addr, &vd, sizeof(vd));
	if (size != sizeof(vd)) {
		// Seek and/or read error.
		return;
	}
	if (memcmp(vd.header.identifier, UDF_VD_BEA01, sizeof(vd.header.identifier)) != 0) {
		// Not an extended descriptor section.
		return;
	}

	// Look for NSR02/NSR03.
	while (addr < maxaddr) {
		addr += sector_size;
		size_t size = file->seekAndRead(addr, &vd, sizeof(vd));
		if (size != sizeof(vd)) {
			// Seek and/or read error.
			break;
		}

		if (!memcmp(vd.header.identifier, "NSR0", 4)) {
			// Found an NSR descriptor.
			switch (vd.header.identifier[4]) {
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

		if (!memcmp(vd.header.identifier, UDF_VD_TEA01, sizeof(vd.header.identifier))) {
			// End of extended descriptor section.
			break;
		}
	}

	// Done reading UDF for now.
	// TODO: More descriptors?
}

/**
 * Read the El Torito boot catalog.
 * @param lba Boot catalog LBA
 */
void ISOPrivate::readBootCatalog(uint32_t lba)
{
	assert(lba != 0);
	if (lba == 0)
		return;

	// Read the entire sector.
	uint8_t sector_buf[ISO_SECTOR_SIZE_MODE1_COOKED];
	const off64_t addr = (lba * static_cast<off64_t>(sector_size)) + sector_offset;
	size_t size = file->seekAndRead(addr, sector_buf, sizeof(sector_buf));
	if (size != sizeof(sector_buf)) {
		// Seek and/or read error.
		return;
	}

	// Parse the entries.
	const uint8_t *p = sector_buf;
	const uint8_t *const p_end = &sector_buf[sizeof(sector_buf)];
	bool isFirst = true, isFinal = false;
	do {
		const ISO_Boot_Section_Header_Entry *const header =
			reinterpret_cast<const ISO_Boot_Section_Header_Entry*>(p);
		if (isFirst) {
			// Header ID must be ISO_BOOT_SECTION_HEADER_ID_FIRST,
			// and the key bytes must be valid.
			if (header->header_id != ISO_BOOT_SECTION_HEADER_ID_FIRST ||
			    header->key_55 != 0x55 || header->key_AA != 0xAA)
			{
				// Invalid header ID and/or key bytes.
				return;
			}
		} else {
			if (header->header_id == ISO_BOOT_SECTION_HEADER_ID_FINAL) {
				// Final header.
				isFinal = true;
			} else if (header->header_id != ISO_BOOT_SECTION_HEADER_ID_NEXT) {
				// Invalid header ID.
				break;
			}
		}
		p += sizeof(*header);
		if (p >= p_end)
			break;

		// TODO: Validate checksum and key bytes?

		// Get header values, and handle first vs. next.
		unsigned int entries;
		if (isFirst) {
			entries = 1;
			isFirst = false;
		} else {
			entries = le16_to_cpu(header->entries);
		}

		// Section entries.
		for (unsigned int i = 0; i < entries && p < p_end; i++, p += sizeof(ISO_Boot_Section_Entry)) {
			const ISO_Boot_Section_Entry *const entry =
				reinterpret_cast<const ISO_Boot_Section_Entry*>(p);
			if (entry->boot_indicator != ISO_BOOT_INDICATOR_IS_BOOTABLE)
				continue;

			// This entry is bootable.
			// TODO: Save it? For now, merely setting bootable flags.
			// TODO: Do this for the header, not the entries?
			switch (header->platform_id) {
				default:
					break;
				case ISO_BOOT_PLATFORM_80x86:
					boot_platforms |= BOOT_PLATFORM_x86;
					break;
				case ISO_BOOT_PLATFORM_EFI:
					boot_platforms |= BOOT_PLATFORM_EFI;
					break;
			}
		}
	} while (!isFinal && p < p_end);

	// Finished reading the boot catalog.
	boot_catalog_LBA = lba;
}

/**
 * Add fields common to HSFS and ISO-9660 (except timestamps)
 * @param pvd PVD
 */
template<typename T>
void ISOPrivate::addPVDCommon(const T *pvd)
{
	// NOTE: CD-i discs only have the BE fields filled in.
	// If the host-endian value is zero, check the swap-endian version.

	// System ID
	fields.addField_string(C_("ISO", "System ID"),
		latin1_to_utf8(pvd->sysID, sizeof(pvd->sysID)),
		RomFields::STRF_TRIM_END);

	// Volume ID
	fields.addField_string(C_("ISO", "Volume ID"),
		latin1_to_utf8(pvd->volID, sizeof(pvd->volID)),
		RomFields::STRF_TRIM_END);

	// Size of volume
	fields.addField_string(C_("ISO", "Volume Size"),
		formatFileSize(
			static_cast<off64_t>(host32(pvd->volume_space_size)) *
			static_cast<off64_t>(host16(pvd->logical_block_size))));

	// TODO: Show block size?

	// Disc number
	const uint16_t volume_seq_number = host16(pvd->volume_seq_number);
	const uint16_t volume_set_size = host16(pvd->volume_set_size);
	if (volume_seq_number != 0 && volume_set_size > 1) {
		const char *const disc_number_title = C_("RomData", "Disc #");
		fields.addField_string(disc_number_title,
			// tr: Disc X of Y (for multi-disc games)
			rp_sprintf_p(C_("RomData|Disc", "%1$u of %2$u"),
				volume_seq_number, volume_set_size));
	}

	// Volume set ID
	fields.addField_string(C_("ISO", "Volume Set"),
		latin1_to_utf8(pvd->volume_set_id, sizeof(pvd->volume_set_id)),
		RomFields::STRF_TRIM_END);

	// Publisher
	fields.addField_string(C_("RomData", "Publisher"),
		latin1_to_utf8(pvd->publisher, sizeof(pvd->publisher)),
		RomFields::STRF_TRIM_END);

	// Data Preparer
	fields.addField_string(C_("ISO", "Data Preparer"),
		latin1_to_utf8(pvd->data_preparer, sizeof(pvd->data_preparer)),
		RomFields::STRF_TRIM_END);

	// Application
	fields.addField_string(C_("ISO", "Application"),
		latin1_to_utf8(pvd->application, sizeof(pvd->application)),
		RomFields::STRF_TRIM_END);

	// Copyright file
	fields.addField_string(C_("ISO", "Copyright File"),
		latin1_to_utf8(pvd->copyright_file, sizeof(pvd->copyright_file)),
		RomFields::STRF_TRIM_END);

	// Abstract file
	fields.addField_string(C_("ISO", "Abstract File"),
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
	fields.addField_dateTime(C_("ISO", "Creation Time"),
		pvd_time_to_unix_time(&pvd->btime),
		RomFields::RFT_DATETIME_HAS_DATE |
		RomFields::RFT_DATETIME_HAS_TIME);

	// Volume modification time
	fields.addField_dateTime(C_("ISO", "Modification Time"),
		pvd_time_to_unix_time(&pvd->mtime),
		RomFields::RFT_DATETIME_HAS_DATE |
		RomFields::RFT_DATETIME_HAS_TIME);

	// Volume expiration time
	fields.addField_dateTime(C_("ISO", "Expiration Time"),
		pvd_time_to_unix_time(&pvd->exptime),
		RomFields::RFT_DATETIME_HAS_DATE |
		RomFields::RFT_DATETIME_HAS_TIME);

	// Volume effective time
	fields.addField_dateTime(C_("ISO", "Effective Time"),
		pvd_time_to_unix_time(&pvd->efftime),
		RomFields::RFT_DATETIME_HAS_DATE |
		RomFields::RFT_DATETIME_HAS_TIME);
}

/**
 * Add fields common to HSFS and ISO-9660 (except timestamps)
 * @param metaData RomMetaData object.
 * @param pvd PVD
 */
template<typename T>
void ISOPrivate::addPVDCommon_metaData(RomMetaData *metaData, const T *pvd)
{
	// TODO: More properties?

	// Title
	metaData->addMetaData_string(Property::Title,
		latin1_to_utf8(pvd->volID, sizeof(pvd->volID)),
		RomMetaData::STRF_TRIM_END);

	// Publisher
	metaData->addMetaData_string(Property::Publisher,
		latin1_to_utf8(pvd->publisher, sizeof(pvd->publisher)),
		RomFields::STRF_TRIM_END);
}

/**
 * Add metadata properties common to HSFS and ISO-9660 (except timestamps)
 * @param metaData RomMetaData object.
 * @param pvd PVD
 */
template<typename T>
void ISOPrivate::addPVDTimestamps_metaData(RomMetaData *metaData, const T *pvd)
{
	// TODO: More properties?

	// Volume creation time
	metaData->addMetaData_timestamp(Property::CreationDate,
		pvd_time_to_unix_time(&pvd->btime));
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
ISO::ISO(const IRpFilePtr &file)
	: super(new ISOPrivate(file))
{
	// This class handles disc images.
	RP_D(ISO);
	d->mimeType = "application/x-cd-image";	// unofficial [TODO: Others?]
	d->fileType = FileType::DiscImage;

	if (!d->file) {
		// Could not ref() the file handle.
		return;
	}

	// Read the PVD. (2048-byte sector address)
	size_t size = d->file->seekAndRead(ISO_PVD_ADDRESS_2048 + ISO_DATA_OFFSET_MODE1_COOKED,
		&d->pvd, sizeof(d->pvd));
	if (size != sizeof(d->pvd)) {
		// Seek and/or read error.
		d->file.reset();
		return;
	}

	// Check if the PVD is valid.
	// NOTE: Not using isRomSupported_static(), since this function
	// only checks the file extension.
	d->discType = d->checkPVD();
	if (d->discType > ISOPrivate::DiscType::Unknown) {
		// Found the PVD using 2048-byte sectors.
		d->sector_size = ISO_SECTOR_SIZE_MODE1_COOKED;
		d->sector_offset = ISO_DATA_OFFSET_MODE1_COOKED;
	} else {
		// Try again using raw sectors: 2352, 2448
		static constexpr array<uint16_t, 2> sector_sizes = {{2352, 2448}};
		CDROM_2352_Sector_t sector;

		for (const unsigned int p : sector_sizes) {
			size_t size = d->file->seekAndRead(p * ISO_PVD_LBA, &sector, sizeof(sector));
			if (size != sizeof(sector)) {
				// Unable to read the PVD.
				d->file.reset();
				return;
			}

			const uint8_t *const pData = cdromSectorDataPtr(&sector);
			d->discType = static_cast<ISOPrivate::DiscType>(ISO::checkPVD(pData));
			if (d->discType > ISOPrivate::DiscType::Unknown) {
				// Found the correct sector size.
				memcpy(&d->pvd, pData, sizeof(d->pvd));
				d->sector_size = p;
				d->sector_offset = (sector.mode == 2 ? ISO_DATA_OFFSET_MODE2_XA : ISO_DATA_OFFSET_MODE1_RAW);
				break;
			}
		}

		if (d->sector_size == 0) {
			// Could not find a valid PVD.
			d->file.reset();
			return;
		}
	}

	// This is a valid PVD.
	d->isValid = true;

	// Check for additional volume descriptors.
	if (d->discType == ISOPrivate::DiscType::ISO9660) {
		d->checkVolumeDescriptors();
	}
}

/** ROM detection functions. **/

/**
 * Check for a valid PVD.
 * @param data Potential PVD. (Must be 2048 bytes)
 * @return DiscType if valid; -1 if not.
 */
int ISO::checkPVD(const uint8_t *data)
{
	// Check for an ISO-9660 PVD.
	const ISO_Primary_Volume_Descriptor *const pvd_iso =
		reinterpret_cast<const ISO_Primary_Volume_Descriptor*>(data);
	if (pvd_iso->header.type == ISO_VDT_PRIMARY && pvd_iso->header.version == ISO_VD_VERSION &&
	    !memcmp(pvd_iso->header.identifier, ISO_VD_MAGIC, sizeof(pvd_iso->header.identifier)))
	{
		// This is an ISO-9660 PVD.
		return static_cast<int>(ISOPrivate::DiscType::ISO9660);
	}

	// Check for a High Sierra PVD.
	const HSFS_Primary_Volume_Descriptor *const pvd_hsfs =
		reinterpret_cast<const HSFS_Primary_Volume_Descriptor*>(data);
	if (pvd_hsfs->header.type == ISO_VDT_PRIMARY && pvd_hsfs->header.version == HSFS_VD_VERSION &&
	    !memcmp(pvd_hsfs->header.identifier, HSFS_VD_MAGIC, sizeof(pvd_hsfs->header.identifier)))
	{
		// This is a High Sierra PVD.
		return static_cast<int>(ISOPrivate::DiscType::HighSierra);
	}

	// Check for a CD-i PVD.
	// NOTE: CD-i PVD uses the same format as ISO-9660.
	if (pvd_iso->header.type == ISO_VDT_PRIMARY && pvd_iso->header.version == CDi_VD_VERSION &&
	    !memcmp(pvd_iso->header.identifier, CDi_VD_MAGIC, sizeof(pvd_iso->header.identifier)))
	{
		// This is a CD-i PVD.
		return static_cast<int>(ISOPrivate::DiscType::CDi);
	}

	// Not supported.
	return static_cast<int>(ISOPrivate::DiscType::Unknown);
}

/**
 * Add metadata properties from an ISO-9660 PVD.
 * Convenience function for other classes.
 * @param metaData RomMetaData object.
 * @param pvd ISO-9660 PVD.
 */
void ISO::addMetaData_PVD(RomMetaData *metaData, const struct _ISO_Primary_Volume_Descriptor *pvd)
{
	ISOPrivate::addPVDCommon_metaData(metaData, pvd);
	ISOPrivate::addPVDTimestamps_metaData(metaData, pvd);
}

/**
 * Is a ROM image supported by this class?
 * @param info DetectInfo containing ROM detection information.
 * @return Class-specific system ID (>= 0) if supported; -1 if not.
 */
int ISO::isRomSupported_static(const DetectInfo *info)
{
	// NOTE: Only checking for supported file extensions.
	assert(info != nullptr);
	if (!info || !info->ext) {
		// No file extension specified...
		return -1;
	}

	for (const char *const *ext = ISOPrivate::exts;
	     *ext != nullptr; ext++)
	{
		if (!strcasecmp(info->ext, *ext)) {
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
	static const array<array<const char*, 4>, 3> sysNames = {{
		{{"ISO-9660", "ISO", "ISO", nullptr}},
		{{"High Sierra Format", "High Sierra", "HSF", nullptr}},
		{{"Compact Disc Interactive", "CD-i", "CD-i", nullptr}},
	}};

	unsigned int sysID = 0;
	if (static_cast<int>(d->discType) >= 0 && d->discType < ISOPrivate::DiscType::Max) {
		sysID = static_cast<unsigned int>(d->discType);
	}
	return sysNames[sysID][type & SYSNAME_TYPE_MASK];
}

/**
 * Load field data.
 * Called by RomData::fields() if the field data hasn't been loaded yet.
 * @return Number of fields read on success; negative POSIX error code on error.
 */
int ISO::loadFieldData(void)
{
	RP_D(ISO);
	if (!d->fields.empty()) {
		// Field data *has* been loaded...
		return 0;
	} else if (!d->file || !d->file->isOpen()) {
		// File isn't open.
		return -EBADF;
	} else if (!d->isValid) {
		// Unsupported file.
		return -EIO;
	}

	d->fields.reserve(18);	// Maximum of 18 fields.

	// NOTE: All fields are space-padded. (0x20, ' ')
	// TODO: ascii_to_utf8()?

	// Sector size
	d->fields.addField_string_numeric(C_("ISO", "Sector Size"), d->sector_size);

	switch (d->discType) {
		case ISOPrivate::DiscType::ISO9660:
			// ISO-9660
			d->fields.setTabName(0, C_("ISO", "ISO-9660 PVD"));

			// PVD common fields
			d->addPVDCommon(&d->pvd.iso);

			// Bibliographic file
			d->fields.addField_string(C_("ISO", "Bibliographic File"),
				latin1_to_utf8(d->pvd.iso.bibliographic_file, sizeof(d->pvd.iso.bibliographic_file)),
				RomFields::STRF_TRIM_END);

			// Timestamps
			d->addPVDTimestamps(&d->pvd.iso);

			// Is this disc bootable? (El Torito)
			if (d->boot_catalog_LBA != 0) {
				// TODO: More comprehensive boot catalog.
				// For now, only showing boot platforms, and
				// only if a boot catalog is present.
				static const array<const char*, 2> boot_platforms_names = {{
					"x86", "EFI"
				}};
				vector<string> *const v_boot_platforms_names = RomFields::strArrayToVector(boot_platforms_names);
				d->fields.addField_bitfield(C_("ISO", "Boot Platforms"),
					v_boot_platforms_names, 0, d->boot_platforms);

			}
			break;

		case ISOPrivate::DiscType::HighSierra:
			// High Sierra
			d->fields.setTabName(0, C_("ISO", "High Sierra PVD"));

			// PVD common fields
			d->addPVDCommon(&d->pvd.hsfs);

			// Timestamps
			d->addPVDTimestamps(&d->pvd.hsfs);
			break;

		case ISOPrivate::DiscType::CDi:
			// CD-i
			d->fields.setTabName(0, C_("ISO", "CD-i PVD"));

			// PVD common fields
			d->addPVDCommon(&d->pvd.iso);

			// Bibliographic file
			d->fields.addField_string(C_("ISO", "Bibliographic File"),
				latin1_to_utf8(d->pvd.iso.bibliographic_file, sizeof(d->pvd.iso.bibliographic_file)),
				RomFields::STRF_TRIM_END);

			// Timestamps
			d->addPVDTimestamps(&d->pvd.iso);
			break;

		default:
			// Should not get here...
			assert(!"Invalid ISO disc type.");
			d->fields.setTabName(0, "ISO");
			break;
	}

	if (d->s_udf_version) {
		// UDF version.
		// TODO: Parse the UDF volume descriptors and
		// show a separate tab for UDF?
		d->fields.addField_string(C_("ISO", "UDF Version"),
			d->s_udf_version);
	}

	// Finished reading the field data.
	return static_cast<int>(d->fields.count());
}

/**
 * Load metadata properties.
 * Called by RomData::metaData() if the metadata hasn't been loaded yet.
 * @return Number of metadata properties read on success; negative POSIX error code on error.
 */
int ISO::loadMetaData(void)
{
	RP_D(ISO);
	if (!d->metaData.empty()) {
		// Metadata *has* been loaded...
		return 0;
	} else if (!d->isValid || static_cast<int>(d->discType) < 0) {
		// Unknown disc image type.
		return -EIO;
	}

	d->metaData.reserve(3);	// Maximum of 3 metadata properties.

	switch (d->discType) {
		default:
		case ISOPrivate::DiscType::Unknown:
			assert(!"Unknown disc type.");
			break;

		case ISOPrivate::DiscType::ISO9660:
		case ISOPrivate::DiscType::CDi:
			d->addPVDCommon_metaData(&d->metaData, &d->pvd.iso);
			d->addPVDTimestamps_metaData(&d->metaData, &d->pvd.iso);
			break;

		case ISOPrivate::DiscType::HighSierra:
			d->addPVDCommon_metaData(&d->metaData, &d->pvd.hsfs);
			d->addPVDTimestamps_metaData(&d->metaData, &d->pvd.hsfs);
			break;
	}

	// Finished reading the metadata.
	return static_cast<int>(d->metaData.count());
}

/**
 * Check for "viewed" achievements.
 *
 * @return Number of achievements unlocked.
 */
int ISO::checkViewedAchievements(void) const
{
	RP_D(const ISO);
	if (!d->isValid) {
		// Disc image is not valid.
		return 0;
	}

	Achievements *const pAch = Achievements::instance();
	int ret = 0;

	// Check for a CD-i disc image.
	if (d->discType == ISOPrivate::DiscType::CDi) {
		pAch->unlock(Achievements::ID::ViewedCDiDiscImage);
		ret++;
	}

	return ret;
}

} // namespace LibRomData
