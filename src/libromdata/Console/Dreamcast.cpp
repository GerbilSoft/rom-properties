/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * Dreamcast.hpp: Sega Dreamcast disc image reader.                        *
 *                                                                         *
 * Copyright (c) 2016-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "Dreamcast.hpp"
#include "data/SegaPublishers.hpp"

#include "dc_structs.h"
#include "cdrom_structs.h"

// Other rom-properties libraries
#include "librptexture/fileformat/SegaPVR.hpp"
using namespace LibRpBase;
using namespace LibRpFile;
using namespace LibRpText;
using namespace LibRpTexture;

// DiscReader
#include "disc/Cdrom2352Reader.hpp"
#include "disc/IsoPartition.hpp"
#include "disc/GdiReader.hpp"
#include "disc/CdiReader.hpp"

// Other RomData subclasses
#include "Media/ISO.hpp"

// C++ STL classes
using std::array;
using std::shared_ptr;
using std::string;
using std::vector;

namespace LibRomData {

class DreamcastPrivate final : public RomDataPrivate
{
public:
	explicit DreamcastPrivate(const IRpFilePtr &file);

private:
	typedef RomDataPrivate super;
	RP_DISABLE_COPY(DreamcastPrivate)

public:
	/** RomDataInfo **/
	static const char *const exts[];
	static const char *const mimeTypes[];
	static const RomDataInfo romDataInfo;

public:
	enum class DiscType {
		Unknown	= -1,

		Iso2048	= 0,	// ISO-9660, 2048-byte sectors.
		Iso2352	= 1,	// ISO-9660, 2352-byte sectors.
		GDI	= 2,	// GD-ROM cuesheet
		CDI	= 3,	// DiscJuggler image

		Max
	};
	DiscType discType;

	// Track 03 start address.
	// ISO-9660 directories use physical offsets,
	// not offsets relative to the start of the track.
	// NOTE: Not used for GDI.
	int iso_start_offset;

	// Disc reader
	// NOTE: May be GdiReader. (TODO: std::variant<> so we don't need dynamic_cast<>?)
	IDiscReaderPtr discReader;

	// ISO-9660 data track (GD data, not CD data)
	IsoPartitionPtr isoPartition;

	// Disc header
	DC_IP0000_BIN_t discHeader;

	// 0GDTEX.PVR image
	shared_ptr<SegaPVR> pvrData;	// SegaPVR object

	/**
	 * Calculate the Product CRC16.
	 * @param ip0000_bin IP0000.bin struct.
	 * @return Product CRC16.
	 */
	static uint16_t calcProductCRC16(const DC_IP0000_BIN_t *ip0000_bin);

	/**
	 * Load 0GDTEX.PVR.
	 * @return 0GDTEX.PVR as rp_image, or nullptr on error.
	 */
	rp_image_const_ptr load0GDTEX(void);

	/**
	 * Get the disc publisher.
	 * @return Disc publisher.
	 */
	string getPublisher(void) const;

	/**
	 * Parse the disc number portion of the device information field.
	 * @param disc_num	[out] Disc number.
	 * @param disc_total	[out] Total number of discs.
	 */
	void parseDiscNumber(uint8_t &disc_num, uint8_t &disc_total) const;
};

ROMDATA_IMPL(Dreamcast)
ROMDATA_IMPL_IMG_TYPES(Dreamcast)

/** DreamcastPrivate **/

/* RomDataInfo */
const char *const DreamcastPrivate::exts[] = {
	".iso",	// ISO-9660 (2048-byte)
	".bin",	// Raw (2352-byte)
	".gdi",	// GD-ROM cuesheet
	".cdi",	// DiscJuggler

	// TODO: Add these formats?
	//".nrg",	// Nero

	nullptr
};
const char *const DreamcastPrivate::mimeTypes[] = {
	// Unofficial MIME types.
	"application/x-dreamcast-iso-image",
	"application/x-dc-rom",
	"application/x-cdi",

	// Unofficial MIME types from FreeDesktop.org.
	// TODO: Get the above types upstreamed and get rid of this.
	"application/x-dreamcast-rom",
	"application/x-gd-rom-cue",
	"application/x-discjuggler-cd-image",

	nullptr
};
const RomDataInfo DreamcastPrivate::romDataInfo = {
	"Dreamcast", exts, mimeTypes
};

DreamcastPrivate::DreamcastPrivate(const IRpFilePtr &file)
	: super(file, &romDataInfo)
	, discType(DiscType::Unknown)
	, iso_start_offset(-1)
{
	// Clear the disc header struct.
	memset(&discHeader, 0, sizeof(discHeader));
}

/**
 * Calculate the Product CRC16.
 * @param ip0000_bin IP0000.bin struct.
 * @return Product CRC16.
 */
uint16_t DreamcastPrivate::calcProductCRC16(const DC_IP0000_BIN_t *ip0000_bin)
{
	unsigned int n = 0xFFFF;

	// CRC16 is for product number and version,
	// so we'll start at product number.
	const uint8_t *p = reinterpret_cast<const uint8_t*>(&ip0000_bin->product_number);
 
	for (unsigned int i = 16; i > 0; i--, p++) {
		n ^= (*p << 8);
		for (unsigned int c = 8; c > 0; c--) {
			if (n & 0x8000) {
				n = (n << 1) ^ 4129;
			} else {
				n = (n << 1);
			}
		}
	}

	return (n & 0xFFFF);
}

/**
 * Load 0GDTEX.PVR.
 * @return 0GDTEX.PVR as rp_image, or nullptr on error.
 */
rp_image_const_ptr DreamcastPrivate::load0GDTEX(void)
{
	if (pvrData) {
		// Image has already been loaded.
		return pvrData->image();
	} else if (!this->file || (!this->discReader)) {
		// Can't load the image.
		return {};
	}

	// Create the ISO-9660 file system reader if it isn't already opened.
	if (!isoPartition) {
		switch (discType) {
			case DiscType::GDI:
			case DiscType::CDI: {
				// Open track 3 as ISO-9660.
				MultiTrackSparseDiscReader *const mtsDiscReader = dynamic_cast<MultiTrackSparseDiscReader*>(discReader.get());
				assert(mtsDiscReader != nullptr);
				if (!mtsDiscReader) {
					return {};
				}

				isoPartition = mtsDiscReader->openIsoPartition(3);
				if (!isoPartition) {
					// NOTE: Some CDIs only have two tracks.
					// Try reading track 2 instead.
					isoPartition = mtsDiscReader->openIsoPartition(2);
				}
				break;
			}

			default:
				// Standalone track.
				// Using the ISO start offset calculated earlier.
				isoPartition = std::make_shared<IsoPartition>(discReader, 0, iso_start_offset);
				break;
		}

		if (!isoPartition->isOpen()) {
			// Unable to open the ISO-9660 partition.
			isoPartition.reset();
			return {};
		}
	}

	// Find "0GDTEX.PVR".
	const IRpFilePtr pvrFile_tmp = isoPartition->open("/0GDTEX.PVR");
	if (!pvrFile_tmp) {
		// Error opening "0GDTEX.PVR".
		return {};
	}

	// Sanity check: PVR shouldn't be larger than 4 MB.
	if (pvrFile_tmp->size() > 4*1024*1024) {
		// PVR is too big.
		return {};
	}

	// Create the SegaPVR object.
	shared_ptr<SegaPVR> pvrData_tmp = std::make_shared<SegaPVR>(pvrFile_tmp);
	if (!pvrData_tmp->isValid()) {
		// PVR is invalid.
		return {};
	}

	// PVR is valid. Save it.
	this->pvrData = std::move(pvrData_tmp);
	return pvrData->image();
}

/**
 * Get the disc publisher.
 * @return Disc publisher.
 */
string DreamcastPrivate::getPublisher(void) const
{
	const char *publisher = nullptr;
	if (!memcmp(discHeader.publisher, DC_IP0000_BIN_MAKER_ID, sizeof(discHeader.publisher))) {
		// First-party Sega title.
		publisher = "Sega";
	} else if (!memcmp(discHeader.publisher, "SEGA LC-T-", 10)) {
		// This may be a third-party T-code.
		char *endptr;
		const unsigned int t_code = static_cast<unsigned int>(
			strtoul(&discHeader.publisher[10], &endptr, 10));
		if (t_code != 0 &&
		    endptr > &discHeader.publisher[10] &&
		    endptr <= &discHeader.publisher[15] &&
		    *endptr == ' ')
		{
			// Valid T-code. Look up the publisher.
			publisher = SegaPublishers::lookup(t_code);
		}
	}

	if (publisher) {
		// Found the publisher.
		return publisher;
	}

	// Unknown publisher.
	// List the field as-is.
	string s_ret = latin1_to_utf8(discHeader.publisher, sizeof(discHeader.publisher));
	trimEnd(s_ret);
	return s_ret;
}

/**
 * Parse the disc number portion of the device information field.
 * @param disc_num	[out] Disc number.
 * @param disc_total	[out] Total number of discs.
 */
void DreamcastPrivate::parseDiscNumber(uint8_t &disc_num, uint8_t &disc_total) const
{
	disc_num = 0;
	disc_total = 0;

	if (!memcmp(&discHeader.device_info[4], " GD-ROM", 7) &&
	    discHeader.device_info[12] == '/')
	{
		// "GD-ROM" is present.
		if (ISDIGIT(discHeader.device_info[11]) &&
		    ISDIGIT(discHeader.device_info[13]))
		{
			// Disc digits are present.
			disc_num = discHeader.device_info[11] & 0x0F;
			disc_total = discHeader.device_info[13] & 0x0F;
		}
	}
}

/** Dreamcast **/

/**
 * Read a Sega Dreamcast disc image.
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
Dreamcast::Dreamcast(const IRpFilePtr &file)
	: super(new DreamcastPrivate(file))
{
	// This class handles disc images.
	RP_D(Dreamcast);
	d->fileType = FileType::DiscImage;

	if (!d->file) {
		// Could not ref() the file handle.
		return;
	}

	// Read the disc header.
	// NOTE: Reading 2352 bytes due to CD-ROM sector formats.
	// NOTE 2: May be smaller if this is a cuesheet.
	CDROM_2352_Sector_t sector;
	d->file->rewind();
	size_t size = d->file->read(&sector, sizeof(sector));
	if (size == 0 || size > sizeof(sector)) {
		d->file.reset();
		return;
	}

	// Check if this disc image is supported.
	const char *const filename = file->filename();
	const DetectInfo info = {
		{0, static_cast<unsigned int>(size), reinterpret_cast<const uint8_t*>(&sector)},
		FileSystem::file_ext(filename),	// ext
		0		// szFile (not needed for Dreamcast)
	};
	d->discType = static_cast<DreamcastPrivate::DiscType>(isRomSupported_static(&info));

	if ((int)d->discType < 0) {
		d->file.reset();
		return;
	}

	switch (d->discType) {
		case DreamcastPrivate::DiscType::Iso2048:
			// 2048-byte sectors
			// TODO: Determine session start address.
			d->mimeType = "application/x-dreamcast-rom";	// unofficial
			memcpy(&d->discHeader, &sector, sizeof(d->discHeader));
			d->iso_start_offset = -1;
			d->discReader = std::make_shared<DiscReader>(d->file);
			if (d->file->size() <= 64*1024) {
				// 64 KB is way too small for a Dreamcast disc image.
				// We'll assume this is IP.bin.
				d->fileType = FileType::BootSector;
			}
			break;

		case DreamcastPrivate::DiscType::Iso2352: {
			// 2352-byte sectors
			d->mimeType = "application/x-dreamcast-rom";	// unofficial
			const uint8_t *const data = cdromSectorDataPtr(&sector);
			memcpy(&d->discHeader, data, sizeof(d->discHeader));
			d->discReader = std::make_shared<Cdrom2352Reader>(d->file);
			d->iso_start_offset = static_cast<int>(cdrom_msf_to_lba(&sector.msf));
			break;
		}

		case DreamcastPrivate::DiscType::GDI:
		case DreamcastPrivate::DiscType::CDI: {
			// GD-ROM cuesheet or DiscJuggler image
			// GDI does't use iso_start_offset.
			// CDI manages its own iso_start_offset.
			const char *mimeType;
			if (d->discType == DreamcastPrivate::DiscType::GDI) {
				d->discReader = std::make_shared<CdiReader>(d->file);
				mimeType = "application/x-gd-rom-cue";
			} else /*if (d->discType == DreamcastPrivate::DiscType::CDI)*/ {
				d->discReader = std::make_shared<CdiReader>(d->file);
				mimeType = "application/x-discjuggler-cd-image";
			}

			MultiTrackSparseDiscReader *const mtsDiscReader = static_cast<MultiTrackSparseDiscReader*>(d->discReader.get());
			// Read the actual track 3 disc header.
			int lba_track03 = mtsDiscReader->startingLBA(3);
			if (lba_track03 < 0) {
				// NOTE: Some CDIs only hvae two tracks.
				// Try reading track 2 instead.
				lba_track03 = mtsDiscReader->startingLBA(2);
				if (lba_track03 < 0) {
					// Error getting the track 03 LBA.
					d->discReader.reset();
					d->file.reset();
					return;
				}
			}

			// TODO: Don't hard-code 2048?
			mtsDiscReader->seekAndRead(lba_track03*2048, &d->discHeader, sizeof(d->discHeader));
			d->mimeType = mimeType;
			break;
		}

		default:
			// Unsupported.
			return;
	}

	d->isValid = true;
}

/**
 * Close the opened file.
 */
void Dreamcast::close(void)
{
	RP_D(Dreamcast);

	// Close any child RomData subclasses.
	d->pvrData.reset();
	d->isoPartition.reset();
	d->discReader.reset();

	// Call the superclass function.
	super::close();
}

/**
 * Is a ROM image supported by this class?
 * @param info DetectInfo containing ROM detection information.
 * @return Class-specific system ID (>= 0) if supported; -1 if not.
 */
int Dreamcast::isRomSupported_static(const DetectInfo *info)
{
	assert(info != nullptr);
	assert(info->header.pData != nullptr);
	assert(info->header.addr == 0);
	if (!info || !info->header.pData ||
	    info->header.addr != 0 ||
	    info->header.size == 0)
	{
		// Either no detection information was specified,
		// or the header is too small.
		return static_cast<int>(DreamcastPrivate::DiscType::Unknown);
	}

	if (info->ext && info->ext[0] != 0) {
		if (!strcasecmp(info->ext, ".gdi")) {
			// This is a GD-ROM cuesheet.
			// Check the first line.
			if (GdiReader::isDiscSupported_static(info->header.pData, info->header.size) >= 0) {
				// This is a supported GD-ROM cuesheet.
				return static_cast<int>(DreamcastPrivate::DiscType::GDI);
			}
		} else if (!strcasecmp(info->ext, ".cdi")) {
			// This is a DiscJuggler disc image.
			return static_cast<int>(DreamcastPrivate::DiscType::CDI);
		}
	}

	// For files that aren't cuesheets, check for a minimum file size.
	if (info->header.size < sizeof(CDROM_2352_Sector_t)) {
		// Header is too small.
		return static_cast<int>(DreamcastPrivate::DiscType::Unknown);
	}

	// Check for Dreamcast HW and Maker ID.

	// Try 2048-byte sectors. (IP0000.bin located at 0x0000.)
	if (info->header.size >= 2048) {
		const DC_IP0000_BIN_t *ip0000_bin = reinterpret_cast<const DC_IP0000_BIN_t*>(info->header.pData);
		if (!memcmp(ip0000_bin->hw_id, DC_IP0000_BIN_HW_ID, sizeof(ip0000_bin->hw_id)) &&
		    !memcmp(ip0000_bin->maker_id, DC_IP0000_BIN_MAKER_ID, sizeof(ip0000_bin->maker_id)))
		{
			// Found HW and Maker IDs at 0x0000.
			// This is a 2048-byte sector image.
			return static_cast<int>(DreamcastPrivate::DiscType::Iso2048);
		}
	}

	// Try 2352-byte sectors.
	if (info->header.size >= 2352 &&
	    Cdrom2352Reader::isDiscSupported_static(info->header.pData, info->header.size) >= 0)
	{
		// Sync bytes are valid.
		const CDROM_2352_Sector_t *const sector =
			reinterpret_cast<const CDROM_2352_Sector_t*>(info->header.pData);

		// Get the user data area. (Offset depends on Mode 1 vs. Mode 2 XA.)
		const uint8_t *const data = cdromSectorDataPtr(sector);

		// Check IP0000.bin.
		const DC_IP0000_BIN_t *ip0000_bin = reinterpret_cast<const DC_IP0000_BIN_t*>(data);
		if (!memcmp(ip0000_bin->hw_id, DC_IP0000_BIN_HW_ID, sizeof(ip0000_bin->hw_id)) &&
		    !memcmp(ip0000_bin->maker_id, DC_IP0000_BIN_MAKER_ID, sizeof(ip0000_bin->maker_id)))
		{
			// Found HW and Maker IDs.
			// This is a 2352-byte sector image.
			return static_cast<int>(DreamcastPrivate::DiscType::Iso2352);
		}
	}

	// TODO: Check for other formats, including CDI and NRG?

	// Not supported.
	return static_cast<int>(DreamcastPrivate::DiscType::Unknown);
}

/**
 * Get the name of the system the loaded ROM is designed for.
 * @param type System name type. (See the SystemName enum.)
 * @return System name, or nullptr if type is invalid.
 */
const char *Dreamcast::systemName(unsigned int type) const
{
	RP_D(const Dreamcast);
	if (!d->isValid || !isSystemNameTypeValid(type))
		return nullptr;

	// Dreamcast has the same name worldwide, so we can
	// ignore the region selection.
	static_assert(SYSNAME_TYPE_MASK == 3,
		"Dreamcast::systemName() array index optimization needs to be updated.");

	// Bits 0-1: Type. (long, short, abbreviation)
	static const array<const char*, 4> sysNames = {{
		"Sega Dreamcast", "Dreamcast", "DC", nullptr
	}};

	return sysNames[type & SYSNAME_TYPE_MASK];
}

/**
 * Get a bitfield of image types this class can retrieve.
 * @return Bitfield of supported image types. (ImageTypesBF)
 */
uint32_t Dreamcast::supportedImageTypes_static(void)
{
	return IMGBF_INT_MEDIA;
}

/**
 * Get a list of all available image sizes for the specified image type.
 * @param imageType Image type.
 * @return Vector of available image sizes, or empty vector if no images are available.
 */
vector<RomData::ImageSizeDef> Dreamcast::supportedImageSizes(ImageType imageType) const
{
	ASSERT_supportedImageSizes(imageType);

	RP_D(const Dreamcast);
	if (!d->isValid || imageType != IMG_INT_MEDIA) {
		// Only IMG_INT_MEDIA is supported.
		return {};
	}

	// TODO: Actually check the PVR.
	// Assuming 256x256 for now.
	return {{nullptr, 256, 256, 0}};
}

/**
 * Get a list of all available image sizes for the specified image type.
 * @param imageType Image type.
 * @return Vector of available image sizes, or empty vector if no images are available.
 */
vector<RomData::ImageSizeDef> Dreamcast::supportedImageSizes_static(ImageType imageType)
{
	ASSERT_supportedImageSizes(imageType);

	if (imageType != IMG_INT_MEDIA) {
		// Only IMG_INT_MEDIA is supported.
		return {};
	}

	// NOTE: Assuming the PVR is 256x256.
	return {{nullptr, 256, 256, 0}};
}

/**
 * Load field data.
 * Called by RomData::fields() if the field data hasn't been loaded yet.
 * @return Number of fields read on success; negative POSIX error code on error.
 */
int Dreamcast::loadFieldData(void)
{
	RP_D(Dreamcast);
	if (!d->fields.empty()) {
		// Field data *has* been loaded...
		return 0;
	} else if (!d->file) {
		// File isn't open.
		return -EBADF;
	} else if (!d->isValid || (int)d->discType < 0) {
		// Unknown disc image type.
		return -EIO;
	}

	// Dreamcast disc header.
	const DC_IP0000_BIN_t *const discHeader = &d->discHeader;
	d->fields.reserve(12);	// Maximum of 12 fields.
	d->fields.setTabName(0, C_("Dreamcast", "Dreamcast"));

	// Title. (TODO: Encoding?)
	d->fields.addField_string(C_("RomData", "Title"),
		latin1_to_utf8(discHeader->title, sizeof(discHeader->title)),
		RomFields::STRF_TRIM_END);

	// Publisher.
	d->fields.addField_string(C_("RomData", "Publisher"), d->getPublisher());

	// TODO: Latin-1, cp1252, or Shift-JIS?

	// Product number.
	d->fields.addField_string(C_("Dreamcast", "Product #"),
		latin1_to_utf8(discHeader->product_number, sizeof(discHeader->product_number)),
		RomFields::STRF_TRIM_END);

	// Product version.
	d->fields.addField_string(C_("RomData", "Version"),
		latin1_to_utf8(discHeader->product_version, sizeof(discHeader->product_version)),
		RomFields::STRF_TRIM_END);

	// Release date.
	const time_t release_date = d->ascii_yyyymmdd_to_unix_time(discHeader->release_date);
	d->fields.addField_dateTime(C_("RomData", "Release Date"), release_date,
		RomFields::RFT_DATETIME_HAS_DATE |
		RomFields::RFT_DATETIME_IS_UTC  // Date only.
	);

	// Disc number.
	uint8_t disc_num, disc_total;
	d->parseDiscNumber(disc_num, disc_total);
	if (disc_num != 0 && disc_total > 1) {
		const char *const disc_number_title = C_("RomData", "Disc #");
		d->fields.addField_string(disc_number_title,
			// tr: Disc X of Y (for multi-disc games)
			rp_sprintf_p(C_("RomData|Disc", "%1$u of %2$u"),
				disc_num, disc_total));
	}

	// Region code.
	// Note that for Dreamcast, each character is assigned to
	// a specific position, so European games will be "  E",
	// not "E  ".
	unsigned int region_code = 0;
	region_code  = (discHeader->area_symbols[0] == 'J');
	region_code |= (discHeader->area_symbols[1] == 'U') << 1;
	region_code |= (discHeader->area_symbols[2] == 'E') << 2;

	static const array<const char*, 3> region_code_bitfield_names = {{
		NOP_C_("Region", "Japan"),
		NOP_C_("Region", "USA"),
		NOP_C_("Region", "Europe"),
	}};
	vector<string> *const v_region_code_bitfield_names = RomFields::strArrayToVector_i18n(
		"Region", region_code_bitfield_names);
	d->fields.addField_bitfield(C_("RomData", "Region Code"),
		v_region_code_bitfield_names, 0, region_code);

	// Boot filename.
	d->fields.addField_string(C_("Dreamcast", "Boot Filename"),
		latin1_to_utf8(discHeader->boot_filename, sizeof(discHeader->boot_filename)),
		RomFields::STRF_TRIM_END);

	// FIXME: The CRC algorithm isn't working right...
#if 0
	// Product CRC16.
	// TODO: Use strtoul().
	unsigned int crc16_expected = 0;
	const char *p = discHeader->device_info;
	for (unsigned int i = 4; i > 0; i--, p++) {
		if (ISXDIGIT(*p)) {
			crc16_expected <<= 4;
			if (ISDIGIT(*p)) {
				crc16_expected |= (*p & 0xF);
			} else {
				crc16_expected |= ((*p & 0xF) + 10);
			}
		} else {
			// Invalid digit.
			crc16_expected = ~0;
			break;
		}
	}

	uint16_t crc16_actual = d->calcProductCRC16(discHeader);
	if (crc16_expected < 0x10000) {
		if (crc16_expected == crc16_actual) {
			// CRC16 is correct.
			d->fields.addField_string(C_("RomData", "Checksum"),
				rp_sprintf(C_("Dreamcast", "0x%04X (valid)"), crc16_expected));
		} else {
			// CRC16 is incorrect.
			d->fields.addField_string(C_("RomData", "Checksum"),
				rp_sprintf_p(C_("Dreamcast", "0x%1$04X (INVALID; should be 0x%2$04X)"),
					crc16_expected, crc16_actual));
		}
	} else {
		// CRC16 in header is invalid.
		d->fields.addField_string(C_("RomData", "Checksum"),
			rp_sprintf_p(C_("Dreamcast", "0x%1$04X (HEADER is INVALID: %2$.4s)"),
				crc16_expected, discHeader->device_info));
	}
#endif

	/** Peripeherals. **/

	// Peripherals are stored as an ASCII hex bitfield.
	char *endptr;
	const unsigned int peripherals = static_cast<unsigned int>(strtoul(
		discHeader->peripherals, &endptr, 16));
	if (endptr > discHeader->peripherals &&
	    endptr <= &discHeader->peripherals[7])
	{
		// Peripherals decoded.
		// OS support.
		static const array<const char*, 5> os_bitfield_names = {{
			NOP_C_("Dreamcast|OSSupport", "Windows CE"),
			nullptr, nullptr, nullptr,
			NOP_C_("Dreamcast|OSSupport", "VGA Box"),
		}};
		vector<string> *const v_os_bitfield_names = RomFields::strArrayToVector_i18n(
			"Dreamcast|OSSupport", os_bitfield_names);
		d->fields.addField_bitfield(C_("Dreamcast", "OS Support"),
			v_os_bitfield_names, 0, peripherals);

		// Supported expansion units.
		static const array<const char*, 4> expansion_bitfield_names = {{
			NOP_C_("Dreamcast|Expansion", "Other"),
			NOP_C_("Dreamcast|Expansion", "Jump Pack"),
			NOP_C_("Dreamcast|Expansion", "Microphone"),
			// tr: "VMS" in Japan; "VMU" in USA; "VM" in Europe
			NOP_C_("Dreamcast|Expansion", "VMU"),
		}};
		vector<string> *const v_expansion_bitfield_names = RomFields::strArrayToVector_i18n(
			"Dreamcast|Expansion", expansion_bitfield_names);
		d->fields.addField_bitfield(C_("Dreamcast", "Expansion Units"),
			v_expansion_bitfield_names, 0, peripherals >> 8);

		// Required controller features.
		static const array<const char*, 13> req_controller_bitfield_names = {{
			NOP_C_("Dreamcast|ReqCtrl", "Start, A, B, D-Pad"),
			NOP_C_("Dreamcast|ReqCtrl", "C Button"),
			NOP_C_("Dreamcast|ReqCtrl", "D Button"),
			NOP_C_("Dreamcast|ReqCtrl", "X Button"),
			NOP_C_("Dreamcast|ReqCtrl", "Y Button"),
			NOP_C_("Dreamcast|ReqCtrl", "Z Button"),
			NOP_C_("Dreamcast|ReqCtrl", "Second D-Pad"),
			NOP_C_("Dreamcast|ReqCtrl", "Analog L Trigger"),
			NOP_C_("Dreamcast|ReqCtrl", "Analog R Trigger"),
			NOP_C_("Dreamcast|ReqCtrl", "Analog H1"),
			NOP_C_("Dreamcast|ReqCtrl", "Analog V1"),
			NOP_C_("Dreamcast|ReqCtrl", "Analog H2"),
			NOP_C_("Dreamcast|ReqCtrl", "Analog V2"),
		}};
		vector<string> *const v_req_controller_bitfield_names = RomFields::strArrayToVector_i18n(
			"Dreamcast|ReqCtrl", req_controller_bitfield_names);
		// tr: Required controller features.
		d->fields.addField_bitfield(C_("Dreamcast", "Req. Controller"),
			v_req_controller_bitfield_names, 3, peripherals >> 12);

		// Optional controller features.
		static const array<const char*, 3> opt_controller_bitfield_names = {{
			NOP_C_("Dreamcast|OptCtrl", "Light Gun"),
			NOP_C_("Dreamcast|OptCtrl", "Keyboard"),
			NOP_C_("Dreamcast|OptCtrl", "Mouse"),
		}};
		vector<string> *const v_opt_controller_bitfield_names = RomFields::strArrayToVector_i18n(
			"Dreamcast|OptCtrl", opt_controller_bitfield_names);
		// tr: Optional controller features.
		d->fields.addField_bitfield(C_("Dreamcast", "Opt. Controller"),
			v_opt_controller_bitfield_names, 0, peripherals >> 25);
	}

	// Try to open the ISO-9660 object.
	// NOTE: Only done here because the ISO-9660 fields
	// are used for field info only.
	ISOPtr isoData;
	switch (d->discType) {
		case DreamcastPrivate::DiscType::GDI:
		case DreamcastPrivate::DiscType::CDI: {
			// Open track 3 as ISO-9660.
			MultiTrackSparseDiscReader *const mtsDiscReader = dynamic_cast<MultiTrackSparseDiscReader*>(d->discReader.get());
			assert(mtsDiscReader != nullptr);
			if (mtsDiscReader) {
				isoData = mtsDiscReader->openIsoRomData(3);
				if (!isoData) {
					// NOTE: Some CDIs only have two tracks.
					// Try reading track 2 instead.
					isoData = mtsDiscReader->openIsoRomData(2);
				}
			}
			break;
		}

		default:
			// ISO object for ISO-9660 PVD
			isoData = std::make_shared<ISO>(d->discReader);
			break;
	}

	if (isoData && isoData->isOpen()) {
		// Add the fields.
		const RomFields *const isoFields = isoData->fields();
		assert(isoFields != nullptr);
		if (isoFields) {
			d->fields.addFields_romFields(isoFields,
				RomFields::TabOffset_AddTabs);
		}
	}

	// Finished reading the field data.
	return static_cast<int>(d->fields.count());
}

/**
 * Load metadata properties.
 * Called by RomData::metaData() if the metadata hasn't been loaded yet.
 * @return Number of metadata properties read on success; negative POSIX error code on error.
 */
int Dreamcast::loadMetaData(void)
{
	RP_D(Dreamcast);
	if (d->metaData != nullptr) {
		// Metadata *has* been loaded...
		return 0;
	} else if (!d->file) {
		// File isn't open.
		return -EBADF;
	} else if (!d->isValid || (int)d->discType < 0) {
		// Unknown disc image type.
		return -EIO;
	}

	// Create the metadata object.
	d->metaData = new RomMetaData();
	d->metaData->reserve(4);	// Maximum of 4 metadata properties.

	// Dreamcast disc header.
	const DC_IP0000_BIN_t *const discHeader = &d->discHeader;

	// Title. (TODO: Encoding?)
	d->metaData->addMetaData_string(Property::Title,
		latin1_to_utf8(discHeader->title, sizeof(discHeader->title)),
		RomMetaData::STRF_TRIM_END);

	// Publisher.
	d->metaData->addMetaData_string(Property::Publisher, d->getPublisher());

	// Release date.
	d->metaData->addMetaData_timestamp(Property::CreationDate,
		d->ascii_yyyymmdd_to_unix_time(discHeader->release_date));

	// Disc number. (multiple disc sets only)
	uint8_t disc_num, disc_total;
	d->parseDiscNumber(disc_num, disc_total);
	if (disc_num != 0 && disc_total > 1) {
		d->metaData->addMetaData_integer(Property::DiscNumber, disc_num);
	}

	// Finished reading the metadata.
	return static_cast<int>(d->metaData->count());
}

/**
 * Load an internal image.
 * Called by RomData::image().
 * @param imageType	[in] Image type to load.
 * @param pImage	[out] Reference to rp_image_const_ptr to store the image in.
 * @return 0 on success; negative POSIX error code on error.
 */
int Dreamcast::loadInternalImage(ImageType imageType, rp_image_const_ptr &pImage)
{
	ASSERT_loadInternalImage(imageType, pImage);
	RP_D(Dreamcast);
	ROMDATA_loadInternalImage_single(
		IMG_INT_MEDIA,	// ourImageType
		d->file,	// file
		d->isValid,	// isValid
		d->discType,	// romType
		nullptr,	// imgCache
		d->load0GDTEX);	// func
}

} // namespace LibRomData
