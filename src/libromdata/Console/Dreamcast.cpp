/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * Dreamcast.hpp: Sega Dreamcast disc image reader.                        *
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

#include "Dreamcast.hpp"
#include "librpbase/RomData_p.hpp"

#include "data/SegaPublishers.hpp"
#include "dc_structs.h"
#include "cdrom_structs.h"

// librpbase
#include "librpbase/common.h"
#include "librpbase/byteswap.h"
#include "librpbase/TextFuncs.hpp"
#include "librpbase/file/IRpFile.hpp"
#include "librpbase/file/FileSystem.hpp"
#include "librpbase/img/rp_image.hpp"
#include "libi18n/i18n.h"
using namespace LibRpBase;

// DiscReader
#include "librpbase/disc/DiscReader.hpp"
#include "disc/Cdrom2352Reader.hpp"
#include "disc/IsoPartition.hpp"
#include "disc/GdiReader.hpp"

// SegaPVR decoder.
#include "Texture/SegaPVR.hpp"

// C includes. (C++ namespace)
#include <cassert>
#include <cctype>
#include <cstring>

// C++ includes.
#include <memory>
#include <string>
#include <vector>
using std::string;
using std::unique_ptr;
using std::vector;

namespace LibRomData {

ROMDATA_IMPL(Dreamcast)
ROMDATA_IMPL_IMG_TYPES(Dreamcast)

class DreamcastPrivate : public RomDataPrivate
{
	public:
		DreamcastPrivate(Dreamcast *q, IRpFile *file);
		virtual ~DreamcastPrivate();

	private:
		typedef RomDataPrivate super;
		RP_DISABLE_COPY(DreamcastPrivate)

	public:
		enum DiscType {
			DISC_UNKNOWN		= -1,	// Unknown ROM type.
			DISC_ISO_2048		= 0,	// ISO-9660, 2048-byte sectors.
			DISC_ISO_2352		= 1,	// ISO-9660, 2352-byte sectors.
			DISC_GDI		= 2,	// GD-ROM cuesheet
		};

		// Disc type and reader.
		int discType;
		union {
			IDiscReader *discReader;
			GdiReader *gdiReader;
		};
		IsoPartition *isoPartition;

		// Disc header.
		DC_IP0000_BIN_t discHeader;

		// Track 03 start address.
		// ISO-9660 directories use physical offsets,
		// not offsets relative to the start of the track.
		// NOTE: Not used for GDI.
		int iso_start_offset;

		// 0GDTEX.PVR image.
		IRpFile *pvrFile;	// uses discReader
		SegaPVR *pvrData;	// SegaPVR object.

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
};

/** DreamcastPrivate **/

DreamcastPrivate::DreamcastPrivate(Dreamcast *q, IRpFile *file)
	: super(q, file)
	, discType(DISC_UNKNOWN)
	, discReader(nullptr)
	, isoPartition(nullptr)
	, iso_start_offset(-1)
	, pvrFile(nullptr)
	, pvrData(nullptr)
{
	// Clear the disc header struct.
	memset(&discHeader, 0, sizeof(discHeader));
}

DreamcastPrivate::~DreamcastPrivate()
{
	if (pvrData) {
		pvrData->unref();
	}
	delete pvrFile;
	delete discReader;
	delete isoPartition;
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
		return pvrData->image(RomData::IMG_INT_IMAGE);
	} else if (!this->file || !this->discReader) {
		// Can't load the image.
		return nullptr;
	}

	// Create the ISO-9660 file system reader if it isn't already opened.
	// TODO: Support multi-track images.
	if (!isoPartition) {
		if (discType == DISC_GDI) {
			// Open track 3 as ISO-9660.
			isoPartition = gdiReader->openIsoPartition(3);
		} else {
			// Standalone track.
			// Using the ISO start offset calculated earlier.
			isoPartition = new IsoPartition(discReader, 0, iso_start_offset);
		}
		if (!isoPartition->isOpen()) {
			// Unable to open the ISO-9660 partition.
			delete isoPartition;
			isoPartition = nullptr;
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
		delete pvrFile_tmp;
		return nullptr;
	}

	// Create the SegaPVR object.
	SegaPVR *const pvrData_tmp = new SegaPVR(pvrFile_tmp);
	if (pvrData_tmp->isValid()) {
		// PVR is valid. Save it.
		this->pvrFile = pvrFile_tmp;
		this->pvrData = pvrData_tmp;
		return pvrData->image(RomData::IMG_INT_IMAGE);
	}

	// PVR is invalid.
	pvrData_tmp->unref();
	delete pvrFile_tmp;
	return nullptr;
}

/** Dreamcast **/

/**
 * Read a Sega Dreamcast disc image.
 *
 * A ROM image must be opened by the caller. The file handle
 * will be dup()'d and must be kept open in order to load
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
	d->className = "Dreamcast";
	d->fileType = FTYPE_DISC_IMAGE;

	if (!d->file) {
		// Could not dup() the file handle.
		return;
	}

	// Read the disc header.
	// NOTE: Reading 2352 bytes due to CD-ROM sector formats.
	// NOTE 2: May be smaller if this is a cuesheet.
	CDROM_2352_Sector_t sector;
	d->file->rewind();
	size_t size = d->file->read(&sector, sizeof(sector));
	if (size == 0 || size > sizeof(sector))
		return;

	// Check if this disc image is supported.
	DetectInfo info;
	info.header.addr = 0;
	info.header.size = (unsigned int)size;
	info.header.pData = reinterpret_cast<const uint8_t*>(&sector);
	const string filename = file->filename();
	info.ext = FileSystem::file_ext(filename);
	info.szFile = 0;	// Not needed for Dreamcast.
	d->discType = isRomSupported_static(&info);

	if (d->discType < 0)
		return;

	switch (d->discType) {
		case DreamcastPrivate::DISC_ISO_2048:
			// 2048-byte sectors.
			// TODO: Determine session start address.
			memcpy(&d->discHeader, &sector, sizeof(d->discHeader));
			d->iso_start_offset = -1;
			d->discReader = new DiscReader(d->file);
			break;

		case DreamcastPrivate::DISC_ISO_2352:
			// 2352-byte sectors.
			// FIXME: Assuming Mode 1.
			memcpy(&d->discHeader, &sector.m1.data, sizeof(d->discHeader));
			d->discReader = new Cdrom2352Reader(d->file);
			d->iso_start_offset = (int)cdrom_msf_to_lba(&sector.msf);
			break;

		case DreamcastPrivate::DISC_GDI: {
			// GD-ROM cuesheet.
			// iso_start_offset isn't used for GDI.
			d->gdiReader = new GdiReader(d->file);
			// Read the actual track 3 disc header.
			const int lba_track03 = d->gdiReader->startingLBA(3);
			if (lba_track03 < 0) {
				// Error getting the track 03 LBA.
				return;
			}
			// TODO: Don't hard-code 2048?
			d->gdiReader->seekAndRead(lba_track03*2048, &d->discHeader, sizeof(d->discHeader));
			break;
		}

		default:
			// Unsupported.
			return;
	}

	d->isValid = true;
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
		return -1;
	}

	if (info->ext && info->ext[0] != 0) {
		// Check for ".gdi".
		if (!strcasecmp(info->ext, ".gdi")) {
			// This is a GD-ROM cuesheet.
			// Check the first line.
			if (GdiReader::isDiscSupported_static(info->header.pData, info->header.size) >= 0) {
				// This is a supported GD-ROM cuesheet.
				return DreamcastPrivate::DISC_GDI;
			}
		}
	}

	// For files that aren't cuesheets, check for a minimum file size.
	if (info->header.size < sizeof(CDROM_2352_Sector_t)) {
		// Header is too small.
		return -1;
	}

	// Check for Dreamcast HW and Maker ID.

	// 0x0000: 2048-byte sectors.
	const DC_IP0000_BIN_t *ip0000_bin = reinterpret_cast<const DC_IP0000_BIN_t*>(info->header.pData);
	if (!memcmp(ip0000_bin->hw_id, DC_IP0000_BIN_HW_ID, sizeof(ip0000_bin->hw_id)) &&
	    !memcmp(ip0000_bin->maker_id, DC_IP0000_BIN_MAKER_ID, sizeof(ip0000_bin->maker_id)))
	{
		// Found HW and Maker IDs at 0x0000.
		// This is a 2048-byte sector image.
		return DreamcastPrivate::DISC_ISO_2048;
	}

	// 0x0010: 2352-byte sectors;
	ip0000_bin = reinterpret_cast<const DC_IP0000_BIN_t*>(&info->header.pData[0x10]);
	if (!memcmp(ip0000_bin->hw_id, DC_IP0000_BIN_HW_ID, sizeof(ip0000_bin->hw_id)) &&
	    !memcmp(ip0000_bin->maker_id, DC_IP0000_BIN_MAKER_ID, sizeof(ip0000_bin->maker_id)))
	{
		// Found HW and Maker IDs at 0x0010.
		// Verify the sync bytes.
		if (Cdrom2352Reader::isDiscSupported_static(info->header.pData, info->header.size) >= 0) {
			// Found CD-ROM sync bytes.
			// This is a 2352-byte sector image.
			return DreamcastPrivate::DISC_ISO_2352;
		}
	}

	// TODO: Check for other formats, including CDI and NRG?

	// Not supported.
	return -1;
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

	static const char *const sysNames[4] = {
		"Sega Dreamcast", "Dreamcast", "DC", nullptr
	};

	return sysNames[type & SYSNAME_TYPE_MASK];
}

/**
 * Get a list of all supported file extensions.
 * This is to be used for file type registration;
 * subclasses don't explicitly check the extension.
 *
 * NOTE: The extensions include the leading dot,
 * e.g. ".bin" instead of "bin".
 *
 * NOTE 2: The array and the strings in the array should
 * *not* be freed by the caller.
 *
 * @return NULL-terminated array of all supported file extensions, or nullptr on error.
 */
const char *const *Dreamcast::supportedFileExtensions_static(void)
{
	static const char *const exts[] = {
		".iso",	// ISO-9660 (2048-byte)
		".bin",	// Raw (2352-byte)
		".gdi",	// GD-ROM cuesheet

		// TODO: Add these formats?
		//".cdi",	// DiscJuggler
		//".nrg",	// Nero

		nullptr
	};
	return exts;
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
	// TODO: Forward to pvrData.
	assert(imageType >= IMG_INT_MIN && imageType <= IMG_EXT_MAX);
	if (imageType < IMG_INT_MIN || imageType > IMG_EXT_MAX) {
		// ImageType is out of range.
		return vector<ImageSizeDef>();
	}

	RP_D(Dreamcast);
	if (!d->isValid || imageType != IMG_INT_MEDIA) {
		return vector<ImageSizeDef>();
	}

	// TODO: Return the image's size.
	// For now, just return a generic image.
	const ImageSizeDef imgsz[] = {{nullptr, 0, 0, 0}};
	return vector<ImageSizeDef>(imgsz, imgsz + 1);
}

/**
 * Load field data.
 * Called by RomData::fields() if the field data hasn't been loaded yet.
 * @return Number of fields read on success; negative POSIX error code on error.
 */
int Dreamcast::loadFieldData(void)
{
	RP_D(Dreamcast);
	if (d->fields->isDataLoaded()) {
		// Field data *has* been loaded...
		return 0;
	} else if (!d->file) {
		// File isn't open.
		return -EBADF;
	} else if (!d->isValid || d->discType < 0) {
		// Unknown ROM image type.
		return -EIO;
	}

	// Dreamcast disc header.
	const DC_IP0000_BIN_t *const discHeader = &d->discHeader;
	d->fields->reserve(12);	// Maximum of 12 fields.

	// Title. (TODO: Encoding?)
	d->fields->addField_string(C_("Dreamcast", "Title"),
		latin1_to_utf8(discHeader->title, sizeof(discHeader->title)),
		RomFields::STRF_TRIM_END);

	// Publisher.
	const char *publisher = nullptr;
	if (!memcmp(discHeader->publisher, DC_IP0000_BIN_MAKER_ID, sizeof(discHeader->publisher))) {
		// First-party Sega title.
		publisher = "Sega";
	} else if (!memcmp(discHeader->publisher, "SEGA LC-T-", 10)) {
		// This may be a third-party T-code.
		char *endptr;
		unsigned int t_code = (unsigned int)strtoul(&discHeader->publisher[10], &endptr, 10);
		if (endptr > &discHeader->publisher[10] &&
		    endptr <= &discHeader->publisher[15] &&
		    *endptr == ' ')
		{
			// Valid T-code. Look up the publisher.
			publisher = SegaPublishers::lookup(t_code);
		}
	}

	if (publisher) {
		d->fields->addField_string(C_("Dreamcast", "Publisher"), publisher);
	} else {
		// Unknown publisher.
		// List the field as-is.
		d->fields->addField_string(C_("Dreamcast", "Publisher"),
			latin1_to_utf8(discHeader->publisher, sizeof(discHeader->publisher)),
			RomFields::STRF_TRIM_END);
	}

	// TODO: Latin-1, cp1252, or Shift-JIS?

	// Product number.
	d->fields->addField_string(C_("Dreamcast", "Product #"),
		latin1_to_utf8(discHeader->product_number, sizeof(discHeader->product_number)),
		RomFields::STRF_TRIM_END);

	// Product version.
	d->fields->addField_string(C_("Dreamcast", "Version"),
		latin1_to_utf8(discHeader->product_version, sizeof(discHeader->product_version)),
		RomFields::STRF_TRIM_END);

	// Release date.
	time_t release_date = d->ascii_yyyymmdd_to_unix_time(discHeader->release_date);
	d->fields->addField_dateTime(C_("Dreamcast", "Release Date"), release_date,
		RomFields::RFT_DATETIME_HAS_DATE |
		RomFields::RFT_DATETIME_IS_UTC  // Date only.
	);

	// Disc number.
	uint8_t disc_num = 0;
	uint8_t disc_total = 0;
	if (!memcmp(&discHeader->device_info[4], " GD-ROM", 7) &&
	    discHeader->device_info[12] == '/')
	{
		// "GD-ROM" is present.
		if (isdigit(discHeader->device_info[11]) &&
		    isdigit(discHeader->device_info[13]))
		{
			// Disc digits are present.
			disc_num = discHeader->device_info[11] & 0x0F;
			disc_total = discHeader->device_info[13] & 0x0F;
		}
	}

	if (disc_num != 0) {
		d->fields->addField_string(C_("Dreamcast", "Disc #"),
			// tr: Disc X of Y (for multi-disc games)
			rp_sprintf_p(C_("Dreamcast|Disc", "%1$u of %2$u"),
				disc_num, disc_total));
	} else {
		d->fields->addField_string(C_("Dreamcast", "Disc #"),
			C_("Dreamcast", "Unknown"));
	}

	// Region code.
	// Note that for Dreamcast, each character is assigned to
	// a specific position, so European games will be "  E",
	// not "E  ".
	unsigned int region_code = 0;
	region_code  = (discHeader->area_symbols[0] == 'J');
	region_code |= (discHeader->area_symbols[1] == 'U') << 1;
	region_code |= (discHeader->area_symbols[2] == 'E') << 2;

	static const char *const region_code_bitfield_names[] = {
		NOP_C_("Region", "Japan"),
		NOP_C_("Region", "USA"),
		NOP_C_("Region", "Europe"),
	};
	vector<string> *v_region_code_bitfield_names = RomFields::strArrayToVector_i18n(
		"Region", region_code_bitfield_names, ARRAY_SIZE(region_code_bitfield_names));
	d->fields->addField_bitfield(C_("Dreamcast", "Region Code"),
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
		if (isxdigit(*p)) {
			crc16_expected <<= 4;
			if (isdigit(*p)) {
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
			d->fields->addField_string(C_("Dreamcast", "Checksum"),
				rp_sprintf(C_("Dreamcast", "0x%04X (valid)"), crc16_expected));
		} else {
			// CRC16 is incorrect.
			d->fields->addField_string(C_("Dreamcast", "Checksum"),
				rp_sprintf_p(C_("Dreamcast", "0x%1$04X (INVALID; should be 0x%2$04X)"),
					crc16_expected, crc16_actual));
		}
	} else {
		// CRC16 in header is invalid.
		d->fields->addField_string(C_("Dreamcast", "Checksum"),
			rp_sprintf_p(C_("Dreamcast", "0x%1$04X (HEADER is INVALID: %2$.4s)"),
				crc16_expected, discHeader->device_info));
	}
#endif

	/** Peripeherals. **/

	// Peripherals are stored as an ASCII hex bitfield.
	char *endptr;
	unsigned int peripherals = (unsigned int)strtoul(discHeader->peripherals, &endptr, 16);
	if (endptr > discHeader->peripherals &&
	    endptr <= &discHeader->peripherals[7])
	{
		// Peripherals decoded.
		// OS support.
		static const char *const os_bitfield_names[] = {
			NOP_C_("Dreamcast|OSSupport", "Windows CE"),
			nullptr, nullptr, nullptr,
			NOP_C_("Dreamcast|OSSupport", "VGA Box"),
		};
		vector<string> *v_os_bitfield_names = RomFields::strArrayToVector_i18n(
			"Dreamcast|OSSupport", os_bitfield_names, ARRAY_SIZE(os_bitfield_names));
		d->fields->addField_bitfield(C_("Dreamcast", "OS Support"),
			v_os_bitfield_names, 0, peripherals);

		// Supported expansion units.
		static const char *const expansion_bitfield_names[] = {
			NOP_C_("Dreamcast|Expansion", "Other"),
			NOP_C_("Dreamcast|Expansion", "Jump Pack"),
			NOP_C_("Dreamcast|Expansion", "Microphone"),
			NOP_C_("Dreamcast|Expansion", "VMU"),
		};
		vector<string> *v_expansion_bitfield_names = RomFields::strArrayToVector_i18n(
			"Dreamcast|Expansion", expansion_bitfield_names, ARRAY_SIZE(expansion_bitfield_names));
		d->fields->addField_bitfield(C_("Dreamcast", "Expansion Units"),
			v_expansion_bitfield_names, 0, peripherals >> 8);

		// Required controller features.
		static const char *const req_controller_bitfield_names[] = {
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
		vector<string> *v_req_controller_bitfield_names = RomFields::strArrayToVector_i18n(
			"Dreamcast|ReqCtrl", req_controller_bitfield_names, ARRAY_SIZE(req_controller_bitfield_names));
		d->fields->addField_bitfield(C_("Dreamcast", "Req. Controller"),
			v_req_controller_bitfield_names, 3, peripherals >> 12);

		// Optional controller features.
		static const char *const opt_controller_bitfield_names[] = {
			NOP_C_("Dreamcast|OptCtrl", "Light Gun"),
			NOP_C_("Dreamcast|OptCtrl", "Keyboard"),
			NOP_C_("Dreamcast|OptCtrl", "Mouse"),
		};
		vector<string> *v_opt_controller_bitfield_names = RomFields::strArrayToVector_i18n(
			"Dreamcast|OptCtrl", opt_controller_bitfield_names, ARRAY_SIZE(opt_controller_bitfield_names));
		d->fields->addField_bitfield(C_("Dreamcast", "Opt. Controller"),
			v_opt_controller_bitfield_names, 0, peripherals >> 25);
	}

	// Finished reading the field data.
	return (int)d->fields->count();
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
	assert(imageType >= IMG_INT_MIN && imageType <= IMG_INT_MAX);
	assert(pImage != nullptr);
	if (!pImage) {
		// Invalid parameters.
		return -EINVAL;
	} else if (imageType < IMG_INT_MIN || imageType > IMG_INT_MAX) {
		// ImageType is out of range.
		*pImage = nullptr;
		return -ERANGE;
	}

	RP_D(Dreamcast);
	if (imageType != IMG_INT_MEDIA) {
		// Only IMG_INT_MEDIA is supported by Dreamcast.
		*pImage = nullptr;
		return -ENOENT;
	} else if (!d->file) {
		// File isn't open.
		*pImage = nullptr;
		return -EBADF;
	} else if (!d->isValid || d->discType < 0) {
		// PVR image isn't valid.
		*pImage = nullptr;
		return -EIO;
	}

	// Load the image.
	*pImage = d->load0GDTEX();
	return (*pImage != nullptr ? 0 : -EIO);
}

}
