/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * Dreamcast.hpp: Sega Dreamcast disc image reader.                        *
 *                                                                         *
 * Copyright (c) 2016-2022 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "Dreamcast.hpp"
#include "data/SegaPublishers.hpp"

#include "dc_structs.h"
#include "cdrom_structs.h"

// librpbase, librpfile
using namespace LibRpBase;
using namespace LibRpFile;

// librptexture
#include "librptexture/fileformat/SegaPVR.hpp"
using LibRpTexture::rp_image;
using LibRpTexture::SegaPVR;

// DiscReader
#include "disc/Cdrom2352Reader.hpp"
#include "disc/IsoPartition.hpp"
#include "disc/GdiReader.hpp"

// Other RomData subclasses
#include "Other/ISO.hpp"

// C++ STL classes
using std::string;
using std::u8string;
using std::vector;

namespace LibRomData {

class DreamcastPrivate final : public RomDataPrivate
{
	public:
		DreamcastPrivate(Dreamcast *q, IRpFile *file);
		virtual ~DreamcastPrivate();

	private:
		typedef RomDataPrivate super;
		RP_DISABLE_COPY(DreamcastPrivate)

	public:
		/** RomDataInfo **/
		static const char8_t *const exts[];
		static const char *const mimeTypes[];
		static const RomDataInfo romDataInfo;

	public:
		enum class DiscType {
			Unknown	= -1,

			Iso2048	= 0,	// ISO-9660, 2048-byte sectors.
			Iso2352	= 1,	// ISO-9660, 2352-byte sectors.
			GDI	= 2,	// GD-ROM cuesheet

			Max
		};
		DiscType discType;

		// Disc reader.
		union {
			IDiscReader *discReader;
			GdiReader *gdiReader;
		};
		IsoPartition *isoPartition;

		// Disc header.
		DC_IP0000_BIN_t discHeader;

		// 0GDTEX.PVR image.
		SegaPVR *pvrData;	// SegaPVR object

		// Track 03 start address.
		// ISO-9660 directories use physical offsets,
		// not offsets relative to the start of the track.
		// NOTE: Not used for GDI.
		int iso_start_offset;

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
		const rp_image *load0GDTEX(void);

		/**
		 * Get the disc publisher.
		 * @return Disc publisher.
		 */
		u8string getPublisher(void) const;

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
const char8_t *const DreamcastPrivate::exts[] = {
	U8(".iso"),	// ISO-9660 (2048-byte)
	U8(".bin"),	// Raw (2352-byte)
	U8(".gdi"),	// GD-ROM cuesheet

	// TODO: Add these formats?
	//U8(".cdi"),	// DiscJuggler
	//U8(".nrg"),	// Nero

	nullptr
};
const char *const DreamcastPrivate::mimeTypes[] = {
	// Unofficial MIME types.
	"application/x-dreamcast-iso-image",
	"application/x-dc-rom",

	// Unofficial MIME types from FreeDesktop.org.
	// TODO: Get the above types upstreamed and get rid of this.
	"application/x-dreamcast-rom",
	"application/x-gd-rom-cue",

	nullptr
};
const RomDataInfo DreamcastPrivate::romDataInfo = {
	"Dreamcast", exts, mimeTypes
};

DreamcastPrivate::DreamcastPrivate(Dreamcast *q, IRpFile *file)
	: super(q, file, &romDataInfo)
	, discType(DiscType::Unknown)
	, discReader(nullptr)
	, isoPartition(nullptr)
	, pvrData(nullptr)
	, iso_start_offset(-1)
{
	// Clear the disc header struct.
	memset(&discHeader, 0, sizeof(discHeader));
}

DreamcastPrivate::~DreamcastPrivate()
{
	UNREF(pvrData);
	UNREF(isoPartition);
	UNREF(discReader);
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
const rp_image *DreamcastPrivate::load0GDTEX(void)
{
	if (pvrData) {
		// Image has already been loaded.
		return pvrData->image();
	} else if (!this->file || !this->discReader) {
		// Can't load the image.
		return nullptr;
	}

	// Create the ISO-9660 file system reader if it isn't already opened.
	if (!isoPartition) {
		if (discType == DiscType::GDI) {
			// Open track 3 as ISO-9660.
			isoPartition = gdiReader->openIsoPartition(3);
		} else {
			// Standalone track.
			// Using the ISO start offset calculated earlier.
			isoPartition = new IsoPartition(discReader, 0, iso_start_offset);
		}
		if (!isoPartition->isOpen()) {
			// Unable to open the ISO-9660 partition.
			UNREF_AND_NULL_NOCHK(isoPartition);
			return nullptr;
		}
	}

	// Find "0GDTEX.PVR".
	IRpFile *pvrFile_tmp = isoPartition->open("/0GDTEX.PVR");
	if (!pvrFile_tmp) {
		// Error opening "0GDTEX.PVR".
		return nullptr;
	}

	// Sanity check: PVR shouldn't be larger than 4 MB.
	if (pvrFile_tmp->size() > 4*1024*1024) {
		// PVR is too big.
		pvrFile_tmp->unref();
		return nullptr;
	}

	// Create the SegaPVR object.
	SegaPVR *const pvrData_tmp = new SegaPVR(pvrFile_tmp);
	pvrFile_tmp->unref();
	if (pvrData_tmp->isValid()) {
		// PVR is valid. Save it.
		this->pvrData = pvrData_tmp;
		return pvrData->image();
	}

	// PVR is invalid.
	pvrData_tmp->unref();
	return nullptr;
}

/**
 * Get the disc publisher.
 * @return Disc publisher.
 */
u8string DreamcastPrivate::getPublisher(void) const
{
	const char8_t *publisher = nullptr;
	if (!memcmp(discHeader.publisher, DC_IP0000_BIN_MAKER_ID, sizeof(discHeader.publisher))) {
		// First-party Sega title.
		publisher = U8("Sega");
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
		// FIXME: U8STRFIX
		return u8string((const char8_t*)publisher);
	}

	// Unknown publisher.
	// List the field as-is.
	u8string s_ret = latin1_to_utf8(discHeader.publisher, sizeof(discHeader.publisher));
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
Dreamcast::Dreamcast(IRpFile *file)
	: super(new DreamcastPrivate(this, file))
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
		UNREF_AND_NULL_NOCHK(d->file);
		return;
	}

	// Check if this disc image is supported.
	// FIXME: U8STRFIX
	const char8_t *const filename = file->filename();
	const DetectInfo info = {
		{0, static_cast<unsigned int>(size), reinterpret_cast<const uint8_t*>(&sector)},
		reinterpret_cast<const char*>(FileSystem::file_ext(filename)),	// ext
		0		// szFile (not needed for Dreamcast)
	};
	d->discType = static_cast<DreamcastPrivate::DiscType>(isRomSupported_static(&info));

	if ((int)d->discType < 0) {
		UNREF_AND_NULL_NOCHK(d->file);
		return;
	}

	switch (d->discType) {
		case DreamcastPrivate::DiscType::Iso2048:
			// 2048-byte sectors.
			// TODO: Determine session start address.
			d->mimeType = "application/x-dreamcast-rom";	// unofficial
			memcpy(&d->discHeader, &sector, sizeof(d->discHeader));
			d->iso_start_offset = -1;
			d->discReader = new DiscReader(d->file);
			if (d->file->size() <= 64*1024) {
				// 64 KB is way too small for a Dreamcast disc image.
				// We'll assume this is IP.bin.
				d->fileType = FileType::BootSector;
			}
			break;

		case DreamcastPrivate::DiscType::Iso2352: {
			// 2352-byte sectors.
			d->mimeType = "application/x-dreamcast-rom";	// unofficial
			const uint8_t *const data = cdromSectorDataPtr(&sector);
			memcpy(&d->discHeader, data, sizeof(d->discHeader));
			d->discReader = new Cdrom2352Reader(d->file);
			d->iso_start_offset = static_cast<int>(cdrom_msf_to_lba(&sector.msf));
			break;
		}

		case DreamcastPrivate::DiscType::GDI: {
			// GD-ROM cuesheet.
			// iso_start_offset isn't used for GDI.
			d->gdiReader = new GdiReader(d->file);
			// Read the actual track 3 disc header.
			const int lba_track03 = d->gdiReader->startingLBA(3);
			if (lba_track03 < 0) {
				// Error getting the track 03 LBA.
				UNREF_AND_NULL_NOCHK(d->gdiReader);
				UNREF_AND_NULL_NOCHK(d->file);
				return;
			}
			// TODO: Don't hard-code 2048?
			d->gdiReader->seekAndRead(lba_track03*2048, &d->discHeader, sizeof(d->discHeader));
			d->mimeType = "application/x-gd-rom-cue";	// unofficial
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
	UNREF_AND_NULL(d->pvrData);
	UNREF_AND_NULL(d->isoPartition);
	UNREF_AND_NULL(d->discReader);

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
		// Check for ".gdi".
		if (!strcasecmp(info->ext, ".gdi")) {
			// This is a GD-ROM cuesheet.
			// Check the first line.
			if (GdiReader::isDiscSupported_static(info->header.pData, info->header.size) >= 0) {
				// This is a supported GD-ROM cuesheet.
				return static_cast<int>(DreamcastPrivate::DiscType::GDI);
			}
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
	static const char *const sysNames[4] = {
		"Sega Dreamcast", "Dreamcast", "DC", nullptr
	};

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
		return vector<ImageSizeDef>();
	}

	// TODO: Actually check the PVR.
	// Assuming 256x256 for now.
	static const ImageSizeDef sz_INT_MEDIA[] = {
		{nullptr, 256, 256, 0}
	};
	return vector<ImageSizeDef>(sz_INT_MEDIA,
		sz_INT_MEDIA + ARRAY_SIZE(sz_INT_MEDIA));
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
		return vector<ImageSizeDef>();
	}

	// NOTE: Assuming the PVR is 256x256.
	static const ImageSizeDef sz_INT_MEDIA[] = {
		{nullptr, 256, 256, 0}
	};
	return vector<ImageSizeDef>(sz_INT_MEDIA,
		sz_INT_MEDIA + ARRAY_SIZE(sz_INT_MEDIA));
}

/**
 * Load field data.
 * Called by RomData::fields() if the field data hasn't been loaded yet.
 * @return Number of fields read on success; negative POSIX error code on error.
 */
int Dreamcast::loadFieldData(void)
{
	RP_D(Dreamcast);
	if (!d->fields->empty()) {
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
	d->fields->reserve(12);	// Maximum of 12 fields.
	d->fields->setTabName(0, C_("Dreamcast", "Dreamcast"));

	// Title. (TODO: Encoding?)
	d->fields->addField_string(C_("RomData", "Title"),
		latin1_to_utf8(discHeader->title, sizeof(discHeader->title)),
		RomFields::STRF_TRIM_END);

	// Publisher.
	d->fields->addField_string(C_("RomData", "Publisher"), d->getPublisher());

	// TODO: Latin-1, cp1252, or Shift-JIS?

	// Product number.
	d->fields->addField_string(C_("Dreamcast", "Product #"),
		latin1_to_utf8(discHeader->product_number, sizeof(discHeader->product_number)),
		RomFields::STRF_TRIM_END);

	// Product version.
	d->fields->addField_string(C_("RomData", "Version"),
		latin1_to_utf8(discHeader->product_version, sizeof(discHeader->product_version)),
		RomFields::STRF_TRIM_END);

	// Release date.
	time_t release_date = d->ascii_yyyymmdd_to_unix_time(discHeader->release_date);
	d->fields->addField_dateTime(C_("RomData", "Release Date"), release_date,
		RomFields::RFT_DATETIME_HAS_DATE |
		RomFields::RFT_DATETIME_IS_UTC  // Date only.
	);

	// Disc number.
	uint8_t disc_num, disc_total;
	d->parseDiscNumber(disc_num, disc_total);
	if (disc_num != 0 && disc_total > 1) {
		// FIXME: U8STRFIX - rp_sprintf_p()
		const char8_t *const disc_number_title = C_("RomData", "Disc #");
		d->fields->addField_string(disc_number_title,
			// tr: Disc X of Y (for multi-disc games)
			rp_sprintf_p(reinterpret_cast<const char*>(C_("RomData|Disc", "%1$u of %2$u")),
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

	static const char8_t *const region_code_bitfield_names[] = {
		NOP_C_("Region", "Japan"),
		NOP_C_("Region", "USA"),
		NOP_C_("Region", "Europe"),
	};
	vector<string> *const v_region_code_bitfield_names = RomFields::strArrayToVector_i18n(
		U8("Region"), region_code_bitfield_names, ARRAY_SIZE(region_code_bitfield_names));
	d->fields->addField_bitfield(C_("RomData", "Region Code"),
		v_region_code_bitfield_names, 0, region_code);

	// Boot filename.
	d->fields->addField_string(C_("Dreamcast", "Boot Filename"),
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
			d->fields->addField_string(C_("RomData", "Checksum"),
				rp_sprintf(C_("Dreamcast", "0x%04X (valid)"), crc16_expected));
		} else {
			// CRC16 is incorrect.
			d->fields->addField_string(C_("RomData", "Checksum"),
				rp_sprintf_p(C_("Dreamcast", "0x%1$04X (INVALID; should be 0x%2$04X)"),
					crc16_expected, crc16_actual));
		}
	} else {
		// CRC16 in header is invalid.
		d->fields->addField_string(C_("RomData", "Checksum"),
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
		static const char8_t *const os_bitfield_names[] = {
			NOP_C_("Dreamcast|OSSupport", "Windows CE"),
			nullptr, nullptr, nullptr,
			NOP_C_("Dreamcast|OSSupport", "VGA Box"),
		};
		vector<string> *const v_os_bitfield_names = RomFields::strArrayToVector_i18n(
			U8("Dreamcast|OSSupport"), os_bitfield_names, ARRAY_SIZE(os_bitfield_names));
		d->fields->addField_bitfield(C_("Dreamcast", "OS Support"),
			v_os_bitfield_names, 0, peripherals);

		// Supported expansion units.
		static const char8_t *const expansion_bitfield_names[] = {
			NOP_C_("Dreamcast|Expansion", "Other"),
			NOP_C_("Dreamcast|Expansion", "Jump Pack"),
			NOP_C_("Dreamcast|Expansion", "Microphone"),
			NOP_C_("Dreamcast|Expansion", "VMU"),
		};
		vector<string> *const v_expansion_bitfield_names = RomFields::strArrayToVector_i18n(
			U8("Dreamcast|Expansion"), expansion_bitfield_names, ARRAY_SIZE(expansion_bitfield_names));
		d->fields->addField_bitfield(C_("Dreamcast", "Expansion Units"),
			v_expansion_bitfield_names, 0, peripherals >> 8);

		// Required controller features.
		static const char8_t *const req_controller_bitfield_names[] = {
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
		};
		vector<string> *const v_req_controller_bitfield_names = RomFields::strArrayToVector_i18n(
			U8("Dreamcast|ReqCtrl"), req_controller_bitfield_names, ARRAY_SIZE(req_controller_bitfield_names));
		// tr: Required controller features.
		d->fields->addField_bitfield(C_("Dreamcast", "Req. Controller"),
			v_req_controller_bitfield_names, 3, peripherals >> 12);

		// Optional controller features.
		static const char8_t *const opt_controller_bitfield_names[] = {
			NOP_C_("Dreamcast|OptCtrl", "Light Gun"),
			NOP_C_("Dreamcast|OptCtrl", "Keyboard"),
			NOP_C_("Dreamcast|OptCtrl", "Mouse"),
		};
		vector<string> *const v_opt_controller_bitfield_names = RomFields::strArrayToVector_i18n(
			U8("Dreamcast|OptCtrl"), opt_controller_bitfield_names, ARRAY_SIZE(opt_controller_bitfield_names));
		// tr: Optional controller features.
		d->fields->addField_bitfield(C_("Dreamcast", "Opt. Controller"),
			v_opt_controller_bitfield_names, 0, peripherals >> 25);
	}

	// Try to open the ISO-9660 object.
	// NOTE: Only done here because the ISO-9660 fields
	// are used for field info only.
	// TODO: Get from GdiReader for GDI.
	if (d->discType == DreamcastPrivate::DiscType::GDI) {
		// Open track 3 as ISO-9660.
		ISO *const isoData = d->gdiReader->openIsoRomData(3);
		if (isoData) {
			if (isoData->isOpen()) {
				// Add the fields.
				const RomFields *const isoFields = isoData->fields();
				assert(isoFields != nullptr);
				if (isoFields) {
					d->fields->addFields_romFields(isoFields,
						RomFields::TabOffset_AddTabs);
				}
			}
			isoData->unref();
		}
	} else {
		// ISO object for ISO-9660 PVD
		PartitionFile *const isoFile = new PartitionFile(d->discReader, 0, d->discReader->size());
		if (isoFile->isOpen()) {
			ISO *const isoData = new ISO(isoFile);
			if (isoData->isOpen()) {
				// Add the fields.
				const RomFields *const isoFields = isoData->fields();
				assert(isoFields != nullptr);
				if (isoFields) {
					d->fields->addFields_romFields(isoFields,
						RomFields::TabOffset_AddTabs);
				}
			}
			isoData->unref();
		}
		isoFile->unref();
	}

	// Finished reading the field data.
	return static_cast<int>(d->fields->count());
}

/**
 * Load metadata properties.
 * Called by RomData::metaData() if the field data hasn't been loaded yet.
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
 * @param pImage	[out] Pointer to const rp_image* to store the image in.
 * @return 0 on success; negative POSIX error code on error.
 */
int Dreamcast::loadInternalImage(ImageType imageType, const rp_image **pImage)
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

}
