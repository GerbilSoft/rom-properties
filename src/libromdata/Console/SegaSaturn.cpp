/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * SegaSaturn.hpp: Sega Saturn disc image reader.                          *
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

#include "SegaSaturn.hpp"
#include "librpbase/RomData_p.hpp"

#include "data/SegaPublishers.hpp"
#include "saturn_structs.h"
#include "cdrom_structs.h"

// librpbase
#include "librpbase/common.h"
#include "librpbase/byteswap.h"
#include "librpbase/TextFuncs.hpp"
#include "librpbase/file/IRpFile.hpp"
#include "libi18n/i18n.h"
using namespace LibRpBase;

// CD-ROM reader.
#include "disc/Cdrom2352Reader.hpp"

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

ROMDATA_IMPL(SegaSaturn)

class SegaSaturnPrivate : public RomDataPrivate
{
	public:
		SegaSaturnPrivate(SegaSaturn *q, IRpFile *file);

	private:
		typedef RomDataPrivate super;
		RP_DISABLE_COPY(SegaSaturnPrivate)

	public:
		/** RomFields **/

		// Peripherals. (RFT_BITFIELD)
		enum Saturn_Peripherals_Bitfield {
			SATURN_IOBF_CONTROL_PAD		= (1 << 0),	// Standard control pad
			SATURN_IOBF_ANALOG_CONTROLLER	= (1 << 1),	// Analog controller
			SATURN_IOBF_MOUSE		= (1 << 2),	// Mouse
			SATURN_IOBF_KEYBOARD		= (1 << 3),	// Keyboard
			SATURN_IOBF_STEERING		= (1 << 4),	// Steering controller
			SATURN_IOBF_MULTITAP		= (1 << 5),	// Multi-Tap
			SATURN_IOBF_LIGHT_GUN		= (1 << 6),	// Light Gun
			SATURN_IOBF_RAM_CARTRIDGE	= (1 << 7),	// RAM Cartridge
			SATURN_IOBF_3D_CONTROLLER	= (1 << 8),	// 3D Controller
			SATURN_IOBF_LINK_CABLE		= (1 << 9),	// Link Cable
			SATURN_IOBF_NETLINK		= (1 << 10),	// NetLink
			SATURN_IOBF_PACHINKO		= (1 << 11),	// Pachinko Controller
			SATURN_IOBF_FDD			= (1 << 12),	// Floppy Disk Drive
			SATURN_IOBF_ROM_CARTRIDGE	= (1 << 13),	// ROM Cartridge
			SATURN_IOBF_MPEG_CARD		= (1 << 14),	// MPEG Card
		};

		// Region code.
		enum SaturnRegion {
			SATURN_REGION_JAPAN	= (1 << 0),
			SATURN_REGION_TAIWAN	= (1 << 1),
			SATURN_REGION_USA	= (1 << 2),
			SATURN_REGION_EUROPE	= (1 << 3),
		};

		/** Internal ROM data. **/

		/**
		 * Parse the peripherals field.
		 * @param peripherals Peripherals field.
		 * @param size Size of peripherals.
		 * @return peripherals bitfield.
		 */
		static uint32_t parsePeripherals(const char *peripherals, int size);

		/**
		 * Parse the region codes field from a Sega Saturn disc header.
		 * @param region_codes Region codes field. (area_symbols)
		 * @param size Size of region_codes.
		 * @return SaturnRegion bitfield.
		 */
		static unsigned int parseRegionCodes(const char *region_codes, int size);

	public:
		enum DiscType {
			DISC_UNKNOWN		= -1,	// Unknown ROM type.
			DISC_ISO_2048		= 0,	// ISO-9660, 2048-byte sectors.
			DISC_ISO_2352		= 1,	// ISO-9660, 2352-byte sectors.
		};

		// Disc type.
		int discType;

		// Disc header.
		Saturn_IP0000_BIN_t discHeader;

		// Region code. (Saturn_Region)
		unsigned int saturn_region;
};

/** SegaSaturnPrivate **/

SegaSaturnPrivate::SegaSaturnPrivate(SegaSaturn *q, IRpFile *file)
	: super(q, file)
	, discType(DISC_UNKNOWN)
	, saturn_region(0)
{
	// Clear the disc header struct.
	memset(&discHeader, 0, sizeof(discHeader));
}

/**
 * Parse the peripherals field.
 * @param peripherals Peripherals field.
 * @param size Size of peripherals.
 * @return peripherals bitfield.
 */
uint32_t SegaSaturnPrivate::parsePeripherals(const char *peripherals, int size)
{
	assert(peripherals != nullptr);
	assert(size > 0);
	if (!peripherals || size <= 0)
		return 0;

	uint32_t ret = 0;
	for (int i = size-1; i >= 0; i--) {
		switch (peripherals[i]) {
			case SATURN_IO_CONTROL_PAD:
				ret |= SATURN_IOBF_CONTROL_PAD;
				break;
			case SATURN_IO_ANALOG_CONTROLLER:
				ret |= SATURN_IOBF_ANALOG_CONTROLLER;
				break;
			case SATURN_IO_MOUSE:
				ret |= SATURN_IOBF_MOUSE;
				break;
			case SATURN_IO_KEYBOARD:
				ret |= SATURN_IOBF_KEYBOARD;
				break;
			case SATURN_IO_STEERING:
				ret |= SATURN_IOBF_STEERING;
				break;
			case SATURN_IO_MULTITAP:
				ret |= SATURN_IOBF_MULTITAP;
				break;
			case SATURN_IO_LIGHT_GUN:
				ret |= SATURN_IOBF_LIGHT_GUN;
				break;
			case SATURN_IO_RAM_CARTRIDGE:
				ret |= SATURN_IOBF_RAM_CARTRIDGE;
				break;
			case SATURN_IO_3D_CONTROLLER:
				ret |= SATURN_IOBF_3D_CONTROLLER;
				break;
			case SATURN_IO_LINK_CABLE_JPN:
			case SATURN_IO_LINK_CABLE_USA:
				// TODO: Are these actually the same thing?
				ret |= SATURN_IOBF_LINK_CABLE;
				break;
			case SATURN_IO_NETLINK:
				ret |= SATURN_IOBF_NETLINK;
				break;
			case SATURN_IO_PACHINKO:
				ret |= SATURN_IOBF_PACHINKO;
				break;
			case SATURN_IO_FDD:
				ret |= SATURN_IOBF_FDD;
				break;
			case SATURN_IO_ROM_CARTRIDGE:
				ret |= SATURN_IOBF_ROM_CARTRIDGE;
				break;
			case SATURN_IO_MPEG_CARD:
				ret |= SATURN_IOBF_MPEG_CARD;
				break;
			default:
				break;
		}
	}

	return ret;
}

/**
 * Parse the region codes field from a Sega Saturn disc header.
 * @param region_codes Region codes field.
 * @param size Size of region_codes.
 * @return SaturnRegion bitfield.
 */
unsigned int SegaSaturnPrivate::parseRegionCodes(const char *region_codes, int size)
{
	assert(region_codes != nullptr);
	assert(size > 0);
	if (!region_codes || size <= 0)
		return 0;

	unsigned int ret = 0;
	for (int i = 0; i < size; i++) {
		if (region_codes[i] == 0 || isspace(region_codes[i]))
			break;
		switch (region_codes[i]) {
			case 'J':
				ret |= SATURN_REGION_JAPAN;
				break;
			case 'T':
				ret |= SATURN_REGION_TAIWAN;
				break;
			case 'U':
				ret |= SATURN_REGION_USA;
				break;
			case 'E':
				ret |= SATURN_REGION_EUROPE;
				break;
			default:
				break;
		}
	}

	return ret;
}

/** SegaSaturn **/

/**
 * Read a Sega Saturn disc image.
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
SegaSaturn::SegaSaturn(IRpFile *file)
	: super(new SegaSaturnPrivate(this, file))
{
	// This class handles disc images.
	RP_D(SegaSaturn);
	d->className = "SegaSaturn";
	d->fileType = FTYPE_DISC_IMAGE;

	if (!d->file) {
		// Could not dup() the file handle.
		return;
	}

	// Read the disc header.
	// NOTE: Reading 2352 bytes due to CD-ROM sector formats.
	CDROM_2352_Sector_t sector;
	d->file->rewind();
	size_t size = d->file->read(&sector, sizeof(sector));
	if (size != sizeof(sector))
		return;

	// Check if this ROM image is supported.
	DetectInfo info;
	info.header.addr = 0;
	info.header.size = sizeof(sector);
	info.header.pData = reinterpret_cast<const uint8_t*>(&sector);
	info.ext = nullptr;	// Not needed for SegaSaturn.
	info.szFile = 0;	// Not needed for SegaSaturn.
	d->discType = isRomSupported_static(&info);

	if (d->discType < 0)
		return;

	switch (d->discType) {
		case SegaSaturnPrivate::DISC_ISO_2048:
			// 2048-byte sectors.
			// TODO: Determine session start address.
			memcpy(&d->discHeader, &sector, sizeof(d->discHeader));
			break;
		case SegaSaturnPrivate::DISC_ISO_2352:
			// 2352-byte sectors.
			// FIXME: Assuming Mode 1.
			memcpy(&d->discHeader, &sector.m1.data, sizeof(d->discHeader));
			break;
		default:
			// Unsupported.
			return;
	}
	d->isValid = true;

	// Parse the Saturn region code.
	d->saturn_region = d->parseRegionCodes(
		d->discHeader.area_symbols, sizeof(d->discHeader.area_symbols));
}

/**
 * Is a ROM image supported by this class?
 * @param info DetectInfo containing ROM detection information.
 * @return Class-specific system ID (>= 0) if supported; -1 if not.
 */
int SegaSaturn::isRomSupported_static(const DetectInfo *info)
{
	assert(info != nullptr);
	assert(info->header.pData != nullptr);
	assert(info->header.addr == 0);
	if (!info || !info->header.pData ||
	    info->header.addr != 0 ||
	    info->header.size < sizeof(CDROM_2352_Sector_t))
	{
		// Either no detection information was specified,
		// or the header is too small.
		return -1;
	}

	// Check for Sega Saturn HW and Maker ID.

	// 0x0000: 2048-byte sectors.
	const Saturn_IP0000_BIN_t *ip0000_bin = reinterpret_cast<const Saturn_IP0000_BIN_t*>(info->header.pData);
	if (!memcmp(ip0000_bin->hw_id, SATURN_IP0000_BIN_HW_ID, sizeof(ip0000_bin->hw_id))) {
		// Found HW ID at 0x0000.
		// This is a 2048-byte sector image.
		return SegaSaturnPrivate::DISC_ISO_2048;
	}

	// 0x0010: 2352-byte sectors;
	ip0000_bin = reinterpret_cast<const Saturn_IP0000_BIN_t*>(&info->header.pData[0x10]);
	if (!memcmp(ip0000_bin->hw_id, SATURN_IP0000_BIN_HW_ID, sizeof(ip0000_bin->hw_id))) {
		// Found HW ID at 0x0010.
		// Verify the sync bytes.
		if (Cdrom2352Reader::isDiscSupported_static(info->header.pData, info->header.size) >= 0) {
			// Found CD-ROM sync bytes.
			// This is a 2352-byte sector image.
			return SegaSaturnPrivate::DISC_ISO_2352;
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
const char *SegaSaturn::systemName(unsigned int type) const
{
	RP_D(const SegaSaturn);
	if (!d->isValid || !isSystemNameTypeValid(type))
		return nullptr;

	// Sega Saturn has the same name worldwide, so we can
	// ignore the region selection.
	static_assert(SYSNAME_TYPE_MASK == 3,
		"SegaSaturn::systemName() array index optimization needs to be updated.");

	static const char *const sysNames[4] = {
		"Sega Saturn", "Saturn", "Sat", nullptr
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
const char *const *SegaSaturn::supportedFileExtensions_static(void)
{
	static const char *const exts[] = {
		".iso",	// ISO-9660 (2048-byte)
		".bin",	// Raw (2352-byte)

		// TODO: Add these formats?
		//".cdi",	// DiscJuggler
		//".nrg",	// Nero

		nullptr
	};
	return exts;
}

/**
 * Load field data.
 * Called by RomData::fields() if the field data hasn't been loaded yet.
 * @return Number of fields read on success; negative POSIX error code on error.
 */
int SegaSaturn::loadFieldData(void)
{
	RP_D(SegaSaturn);
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

	// Sega Saturn disc header.
	const Saturn_IP0000_BIN_t *const discHeader = &d->discHeader;
	d->fields->reserve(8);	// Maximum of 8 fields.

	// Title. (TODO: Encoding?)
	d->fields->addField_string(C_("SegaSaturn", "Title"),
		latin1_to_utf8(discHeader->title, sizeof(discHeader->title)),
		RomFields::STRF_TRIM_END);

	// Publisher.
	const char *publisher = nullptr;
	if (!memcmp(discHeader->maker_id, SATURN_IP0000_BIN_MAKER_ID, sizeof(discHeader->maker_id))) {
		// First-party Sega title.
		publisher = "Sega";
	} else if (!memcmp(discHeader->maker_id, "SEGA TP T-", 10)) {
		// This may be a third-party T-code.
		char *endptr;
		unsigned int t_code = (unsigned int)strtoul(&discHeader->maker_id[10], &endptr, 10);
		if (t_code != 0 &&
		    endptr > &discHeader->maker_id[10] &&
		    endptr <= &discHeader->maker_id[15])
		{
			// Valid T-code. Look up the publisher.
			publisher = SegaPublishers::lookup(t_code);
		}
	}

	if (publisher) {
		d->fields->addField_string(C_("SegaSaturn", "Publisher"), publisher);
	} else {
		// Unknown publisher.
		// List the field as-is.
		d->fields->addField_string(C_("SegaSaturn", "Publisher"),
			latin1_to_utf8(discHeader->maker_id, sizeof(discHeader->maker_id)),
			RomFields::STRF_TRIM_END);
	}

	// TODO: Latin-1, cp1252, or Shift-JIS?

	// Product number.
	d->fields->addField_string(C_("SegaSaturn", "Product #"),
		latin1_to_utf8(discHeader->product_number, sizeof(discHeader->product_number)),
		RomFields::STRF_TRIM_END);

	// Product version.
	d->fields->addField_string(C_("SegaSaturn", "Version"),
		latin1_to_utf8(discHeader->product_version, sizeof(discHeader->product_version)),
		RomFields::STRF_TRIM_END);

	// Release date.
	time_t release_date = d->ascii_yyyymmdd_to_unix_time(discHeader->release_date);
	d->fields->addField_dateTime(C_("SegaSaturn", "Release Date"), release_date,
		RomFields::RFT_DATETIME_HAS_DATE |
		RomFields::RFT_DATETIME_IS_UTC  // Date only.
	);

	// Region code.
	// Sega Saturn uses position-independent region code flags.
	// This is similar to older Mega Drive games, but different
	// compared to Dreamcast. The region code is parsed in the
	// constructor, since it might be used for branding purposes
	// later.
	static const char *const region_code_bitfield_names[] = {
		NOP_C_("Region", "Japan"),
		NOP_C_("Region", "Taiwan"),
		NOP_C_("Region", "USA"),
		NOP_C_("Region", "Europe"),
	};
	vector<string> *v_region_code_bitfield_names = RomFields::strArrayToVector_i18n(
		"Region", region_code_bitfield_names, ARRAY_SIZE(region_code_bitfield_names));
	d->fields->addField_bitfield(C_("SegaSaturn", "Region Code"),
		v_region_code_bitfield_names, 0, d->saturn_region);

	// Disc number.
	uint8_t disc_num = 0;
	uint8_t disc_total = 0;
	if (!memcmp(discHeader->device_info, "CD-", 3) &&
	    discHeader->device_info[4] == '/')
	{
		// "GD-ROM" is present.
		if (isdigit(discHeader->device_info[3]) &&
		    isdigit(discHeader->device_info[5]))
		{
			// Disc digits are present.
			disc_num = discHeader->device_info[3] & 0x0F;
			disc_total = discHeader->device_info[5] & 0x0F;
		}
	}

	if (disc_num != 0) {
		d->fields->addField_string(C_("SegaSaturn", "Disc #"),
			rp_sprintf_p(C_("SegaSaturn|Disc", "%1$u of %2$u"),
				disc_num, disc_total));
	} else {
		d->fields->addField_string(C_("SegaSaturn", "Disc #"),
			C_("SegaSaturn", "Unknown"));
	}

	// Peripherals.
	static const char *const peripherals_bitfield_names[] = {
		NOP_C_("SegaSaturn|Peripherals", "Control Pad"),
		NOP_C_("SegaSaturn|Peripherals", "Analog Controller"),
		NOP_C_("SegaSaturn|Peripherals", "Mouse"),
		NOP_C_("SegaSaturn|Peripherals", "Keyboard"),
		NOP_C_("SegaSaturn|Peripherals", "Steering Controller"),
		NOP_C_("SegaSaturn|Peripherals", "Multi-Tap"),
		NOP_C_("SegaSaturn|Peripherals", "Light Gun"),
		NOP_C_("SegaSaturn|Peripherals", "RAM Cartridge"),
		NOP_C_("SegaSaturn|Peripherals", "3D Controller"),
		NOP_C_("SegaSaturn|Peripherals", "Link Cable"),
		NOP_C_("SegaSaturn|Peripherals", "NetLink"),
		NOP_C_("SegaSaturn|Peripherals", "Pachinko"),
		NOP_C_("SegaSaturn|Peripherals", "Floppy Drive"),
		NOP_C_("SegaSaturn|Peripherals", "ROM Cartridge"),
		NOP_C_("SegaSaturn|Peripherals", "MPEG Card"),
	};
	vector<string> *v_peripherals_bitfield_names = RomFields::strArrayToVector_i18n(
		"SegaSaturn|Peripherals", peripherals_bitfield_names, ARRAY_SIZE(peripherals_bitfield_names));
	// Parse peripherals.
	uint32_t peripherals = d->parsePeripherals(discHeader->peripherals, sizeof(discHeader->peripherals));
	d->fields->addField_bitfield(C_("SegaSaturn", "Peripherals"),
		v_peripherals_bitfield_names, 3, peripherals);

	// Finished reading the field data.
	return (int)d->fields->count();
}

}
