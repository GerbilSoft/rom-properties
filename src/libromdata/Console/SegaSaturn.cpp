/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * SegaSaturn.hpp: Sega Saturn disc image reader.                          *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "SegaSaturn.hpp"
#include "data/SegaPublishers.hpp"
#include "saturn_structs.h"
#include "cdrom_structs.h"

// Other rom-properties libraries
using namespace LibRpBase;
using namespace LibRpFile;
using namespace LibRpText;

// CD-ROM reader
#include "disc/Cdrom2352Reader.hpp"

// Other RomData subclasses
#include "Media/ISO.hpp"

// C++ STL classes
using std::array;
using std::string;
using std::vector;

namespace LibRomData {

class SegaSaturnPrivate final : public RomDataPrivate
{
public:
	explicit SegaSaturnPrivate(const IRpFilePtr &file);

private:
	typedef RomDataPrivate super;
	RP_DISABLE_COPY(SegaSaturnPrivate)

public:
	/** RomDataInfo **/
	static const array<const char*, 2+1> exts;
	static const array<const char*, 1+1> mimeTypes;
	static const RomDataInfo romDataInfo;

public:
	// Region code bitfield names
	static const array<const char*, 4> region_code_bitfield_names;

public:
	/** RomFields **/

	// Peripherals (RFT_BITFIELD) [bit values]
	enum Saturn_Peripherals_Bitfield : unsigned int {
		SATURN_IOBF_CONTROL_PAD		= (1U <<  0),	// Standard control pad
		SATURN_IOBF_ANALOG_CONTROLLER	= (1U <<  1),	// Analog controller
		SATURN_IOBF_MOUSE		= (1U <<  2),	// Mouse
		SATURN_IOBF_KEYBOARD		= (1U <<  3),	// Keyboard
		SATURN_IOBF_STEERING		= (1U <<  4),	// Steering controller
		SATURN_IOBF_MULTITAP		= (1U <<  5),	// Multi-Tap
		SATURN_IOBF_LIGHT_GUN		= (1U <<  6),	// Light Gun
		SATURN_IOBF_RAM_CARTRIDGE	= (1U <<  7),	// RAM Cartridge
		SATURN_IOBF_3D_CONTROLLER	= (1U <<  8),	// 3D Controller
		SATURN_IOBF_LINK_CABLE		= (1U <<  9),	// Link Cable
		SATURN_IOBF_NETLINK		= (1U << 10),	// NetLink
		SATURN_IOBF_PACHINKO		= (1U << 11),	// Pachinko Controller
		SATURN_IOBF_FDD			= (1U << 12),	// Floppy Disk Drive
		SATURN_IOBF_ROM_CARTRIDGE	= (1U << 13),	// ROM Cartridge
		SATURN_IOBF_MPEG_CARD		= (1U << 14),	// MPEG Card
	};

	// Peripherals (RFT_BITFIELD) [bit numbers]
	enum Saturn_Peripherals_Bits : uint8_t {
		SATURN_IOBIT_CONTROL_PAD	=  0U,	// Standard control pad
		SATURN_IOBIT_ANALOG_CONTROLLER	=  1U,	// Analog controller
		SATURN_IOBIT_MOUSE		=  2U,	// Mouse
		SATURN_IOBIT_KEYBOARD		=  3U,	// Keyboard
		SATURN_IOBIT_STEERING		=  4U,	// Steering controller
		SATURN_IOBIT_MULTITAP		=  5U,	// Multi-Tap
		SATURN_IOBIT_LIGHT_GUN		=  6U,	// Light Gun
		SATURN_IOBIT_RAM_CARTRIDGE	=  7U,	// RAM Cartridge
		SATURN_IOBIT_3D_CONTROLLER	=  8U,	// 3D Controller
		SATURN_IOBIT_LINK_CABLE		=  9U,	// Link Cable
		SATURN_IOBIT_NETLINK		= 10U,	// NetLink
		SATURN_IOBIT_PACHINKO		= 11U,	// Pachinko Controller
		SATURN_IOBIT_FDD		= 12U,	// Floppy Disk Drive
		SATURN_IOBIT_ROM_CARTRIDGE	= 13U,	// ROM Cartridge
		SATURN_IOBIT_MPEG_CARD		= 14U,	// MPEG Card
	};

	// Region code
	enum SaturnRegion {
		SATURN_REGION_JAPAN	= (1U << 0),
		SATURN_REGION_TAIWAN	= (1U << 1),
		SATURN_REGION_USA	= (1U << 2),
		SATURN_REGION_EUROPE	= (1U << 3),
	};

	/** Internal ROM data **/

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
	enum class DiscType {
		Unknown	= -1,

		Iso2048	= 0,	// ISO-9660, 2048-byte sectors.
		Iso2352	= 1,	// ISO-9660, 2352-byte sectors.

		Max
	};
	DiscType discType;

	// Disc header
	Saturn_IP0000_BIN_t discHeader;

	// Region code (Saturn_Region)
	unsigned int saturn_region;

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

/** SegaSaturnPrivate **/

ROMDATA_IMPL(SegaSaturn)

/* RomDataInfo */
const array<const char*, 2+1> SegaSaturnPrivate::exts = {{
	".iso",	// ISO-9660 (2048-byte)
	".bin",	// Raw (2352-byte)

	// TODO: Add these formats?
	//".cdi",	// DiscJuggler
	//".nrg",	// Nero

	nullptr
}};
const array<const char*, 1+1> SegaSaturnPrivate::mimeTypes = {{
	// Unofficial MIME types from FreeDesktop.org.
	"application/x-saturn-rom",

	nullptr
}};
const RomDataInfo SegaSaturnPrivate::romDataInfo = {
	"SegaSaturn", exts.data(), mimeTypes.data()
};

// Region code bitfield names
const array<const char*, 4> SegaSaturnPrivate::region_code_bitfield_names = {{
	NOP_C_("Region", "Japan"),
	NOP_C_("Region", "Taiwan"),
	NOP_C_("Region", "USA"),
	NOP_C_("Region", "Europe"),
}};

SegaSaturnPrivate::SegaSaturnPrivate(const IRpFilePtr &file)
	: super(file, &romDataInfo)
	, discType(DiscType::Unknown)
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

	// TODO: Sort by character and use bsearch()?
	#define SATURN_IO_SUPPORT_ENTRY(entry) {SATURN_IO_##entry, SATURN_IOBIT_##entry}
	struct saturn_io_tbl_t {
		char io_chr;	// Character in the Peripherals field
		uint8_t io_bit;	// Bit number in the returned bitfield
	};
	static const array<saturn_io_tbl_t, 17> saturn_io_tbl = {{
		{' ', 0},	// quick exit for empty entries
		SATURN_IO_SUPPORT_ENTRY(CONTROL_PAD),
		SATURN_IO_SUPPORT_ENTRY(ANALOG_CONTROLLER),
		SATURN_IO_SUPPORT_ENTRY(MOUSE),
		SATURN_IO_SUPPORT_ENTRY(KEYBOARD),
		SATURN_IO_SUPPORT_ENTRY(STEERING),
		SATURN_IO_SUPPORT_ENTRY(MULTITAP),
		SATURN_IO_SUPPORT_ENTRY(LIGHT_GUN),
		SATURN_IO_SUPPORT_ENTRY(RAM_CARTRIDGE),
		SATURN_IO_SUPPORT_ENTRY(3D_CONTROLLER),

		// TODO: Are these actually the same thing?
		{SATURN_IO_LINK_CABLE_JPN, SATURN_IOBIT_LINK_CABLE},
		{SATURN_IO_LINK_CABLE_USA, SATURN_IOBIT_LINK_CABLE},

		SATURN_IO_SUPPORT_ENTRY(NETLINK),
		SATURN_IO_SUPPORT_ENTRY(PACHINKO),
		SATURN_IO_SUPPORT_ENTRY(FDD),
		SATURN_IO_SUPPORT_ENTRY(ROM_CARTRIDGE),
		SATURN_IO_SUPPORT_ENTRY(MPEG_CARD),
	}};

	uint32_t ret = 0;
	for (int i = size-1; i >= 0; i--) {
		const char io_chr = peripherals[i];
		auto iter = std::find_if(saturn_io_tbl.cbegin(), saturn_io_tbl.cend(),
			[io_chr](const saturn_io_tbl_t &p) noexcept -> bool {
				return (p.io_chr == io_chr);
			});
		if (iter != saturn_io_tbl.cend()) {
			ret |= (1U << iter->io_bit);
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

	// Compatible area symbol reference:
	// https://segaretro.org/ROM_header#Compatible_area_symbol
	unsigned int ret = 0;
	for (int i = 0; i < size; i++) {
		if (region_codes[i] == 0 || ISSPACE(region_codes[i]))
			break;
		switch (region_codes[i]) {
			case 'J':
				ret |= SATURN_REGION_JAPAN;
				break;
			case 'T':
			case 'K':
				ret |= SATURN_REGION_TAIWAN;
				break;
			case 'U':
			case 'B':
				ret |= SATURN_REGION_USA;
				break;
			case 'E':
			case 'A':
			case 'L':
				ret |= SATURN_REGION_EUROPE;
				break;
			default:
				break;
		}
	}

	return ret;
}

/**
 * Get the disc publisher.
 * @return Disc publisher.
 */
string SegaSaturnPrivate::getPublisher(void) const
{
	const char *publisher = nullptr;
	if (!memcmp(discHeader.maker_id, SATURN_IP0000_BIN_MAKER_ID, sizeof(discHeader.maker_id))) {
		// First-party Sega title.
		publisher = "Sega";
	} else if (!memcmp(discHeader.maker_id, "SEGA TP T-", 10)) {
		// This may be a third-party T-code.
		char *endptr;
		const unsigned int t_code = static_cast<unsigned int>(
			strtoul(&discHeader.maker_id[10], &endptr, 10));
		if (t_code != 0 &&
		    endptr > &discHeader.maker_id[10] &&
		    endptr <= &discHeader.maker_id[15] &&
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
	string s_ret = latin1_to_utf8(discHeader.maker_id, sizeof(discHeader.maker_id));
	trimEnd(s_ret);
	return s_ret;
}

/**
 * Parse the disc number portion of the device information field.
 * @param disc_num	[out] Disc number.
 * @param disc_total	[out] Total number of discs.
 */
void SegaSaturnPrivate::parseDiscNumber(uint8_t &disc_num, uint8_t &disc_total) const
{
	disc_num = 0;
	disc_total = 0;

	if (!memcmp(&discHeader.device_info[0], "CD-", 3) &&
	             discHeader.device_info[4] == '/')
	{
		// "CD-ROM" is present.
		if (isdigit_ascii(discHeader.device_info[3]) &&
		    isdigit_ascii(discHeader.device_info[5]))
		{
			// Disc digits are present.
			disc_num = discHeader.device_info[3] & 0x0F;
			disc_total = discHeader.device_info[5] & 0x0F;
		}
	}
}

/** SegaSaturn **/

/**
 * Read a Sega Saturn disc image.
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
SegaSaturn::SegaSaturn(const IRpFilePtr &file)
	: super(new SegaSaturnPrivate(file))
{
	// This class handles disc images.
	RP_D(SegaSaturn);
	d->mimeType = "application/x-saturn-rom";	// unofficial
	d->fileType = FileType::DiscImage;

	if (!d->file) {
		// Could not ref() the file handle.
		return;
	}

	// Read the disc header.
	// NOTE: Reading 2352 bytes due to CD-ROM sector formats.
	CDROM_2352_Sector_t sector;
	d->file->rewind();
	size_t size = d->file->read(&sector, sizeof(sector));
	if (size != sizeof(sector)) {
		d->file.reset();
		return;
	}

	// Check if this ROM image is supported.
	const DetectInfo info = {
		{0, sizeof(sector), reinterpret_cast<const uint8_t*>(&sector)},
		nullptr,	// ext (not needed for SegaSaturn)
		0		// szFile (not needed for SegaSaturn)
	};
	d->discType = static_cast<SegaSaturnPrivate::DiscType>(isRomSupported_static(&info));

	switch (d->discType) {
		case SegaSaturnPrivate::DiscType::Iso2048:
			// 2048-byte sectors.
			// TODO: Determine session start address.
			memcpy(&d->discHeader, &sector, sizeof(d->discHeader));
			if (d->file->size() <= 64*1024) {
				// 64 KB is way too small for a Dreamcast disc image.
				// We'll assume this is IP.bin.
				d->fileType = FileType::BootSector;
			}
			break;
		case SegaSaturnPrivate::DiscType::Iso2352:
			// 2352-byte sectors.
			// Assuming Mode 1. (TODO: Check for Mode 2.)
			memcpy(&d->discHeader, &sector.m1.data, sizeof(d->discHeader));
			break;
		default:
			// Unsupported.
			d->file.reset();
			return;
	}
	d->isValid = true;

	// Parse the Saturn region code.
	d->saturn_region = d->parseRegionCodes(
		d->discHeader.area_symbols, sizeof(d->discHeader.area_symbols));

	// Is PAL? (TODO: Multi-region?)
	d->isPAL = (d->saturn_region == SegaSaturnPrivate::SATURN_REGION_EUROPE);
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
		return static_cast<int>(SegaSaturnPrivate::DiscType::Unknown);
	}

	// Check for Sega Saturn HW and Maker ID.

	// 0x0000: 2048-byte sectors.
	const Saturn_IP0000_BIN_t *ip0000_bin = reinterpret_cast<const Saturn_IP0000_BIN_t*>(info->header.pData);
	if (!memcmp(ip0000_bin->hw_id, SATURN_IP0000_BIN_HW_ID, sizeof(ip0000_bin->hw_id))) {
		// Found HW ID at 0x0000.
		// This is a 2048-byte sector image.
		return static_cast<int>(SegaSaturnPrivate::DiscType::Iso2048);
	}

	// 0x0010: 2352-byte sectors;
	ip0000_bin = reinterpret_cast<const Saturn_IP0000_BIN_t*>(&info->header.pData[0x10]);
	if (!memcmp(ip0000_bin->hw_id, SATURN_IP0000_BIN_HW_ID, sizeof(ip0000_bin->hw_id))) {
		// Found HW ID at 0x0010.
		// Verify the sync bytes.
		if (Cdrom2352Reader::isDiscSupported_static(info->header.pData, info->header.size) >= 0) {
			// Found CD-ROM sync bytes.
			// This is a 2352-byte sector image.
			return static_cast<int>(SegaSaturnPrivate::DiscType::Iso2352);
		}
	}

	// TODO: Check for other formats, including CDI and NRG?

	// Not supported.
	return static_cast<int>(SegaSaturnPrivate::DiscType::Unknown);
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

	static const array<const char*, 4> sysNames = {{
		"Sega Saturn", "Saturn", "Sat", nullptr
	}};

	return sysNames[type & SYSNAME_TYPE_MASK];
}

/**
 * Load field data.
 * Called by RomData::fields() if the field data hasn't been loaded yet.
 * @return Number of fields read on success; negative POSIX error code on error.
 */
int SegaSaturn::loadFieldData(void)
{
	RP_D(SegaSaturn);
	if (!d->fields.empty()) {
		// Field data *has* been loaded...
		return 0;
	} else if (!d->file) {
		// File isn't open.
		return -EBADF;
	} else if (!d->isValid || static_cast<int>(d->discType) < 0) {
		// Unknown ROM image type.
		return -EIO;
	}

	// Sega Saturn disc header
	const Saturn_IP0000_BIN_t *const discHeader = &d->discHeader;
	d->fields.reserve(8);	// Maximum of 8 fields.
	d->fields.setTabName(0, C_("SegaSaturn", "Saturn"));

	// Title. (TODO: Encoding?)
	d->fields.addField_string(C_("RomData", "Title"),
		latin1_to_utf8(discHeader->title, sizeof(discHeader->title)),
		RomFields::STRF_TRIM_END);

	// Publisher
	d->fields.addField_string(C_("RomData", "Publisher"), d->getPublisher());

	// TODO: Latin-1, cp1252, or Shift-JIS?

	// Product number
	d->fields.addField_string(C_("SegaSaturn", "Product #"),
		latin1_to_utf8(discHeader->product_number, sizeof(discHeader->product_number)),
		RomFields::STRF_TRIM_END);

	// Product version
	d->fields.addField_string(C_("RomData", "Version"),
		latin1_to_utf8(discHeader->product_version, sizeof(discHeader->product_version)),
		RomFields::STRF_TRIM_END);

	// Release date
	const time_t release_date = d->ascii_yyyymmdd_to_unix_time(discHeader->release_date);
	d->fields.addField_dateTime(C_("RomData", "Release Date"), release_date,
		RomFields::RFT_DATETIME_HAS_DATE |
		RomFields::RFT_DATETIME_IS_UTC  // Date only.
	);

	// Region code
	// Sega Saturn uses position-independent region code flags.
	// This is similar to older Mega Drive games, but different compared
	// to Dreamcast. The region code is parsed in the constructor, since
	// it might be used for branding purposes later.
	vector<string> *const v_region_code_bitfield_names = RomFields::strArrayToVector_i18n(
		"Region", d->region_code_bitfield_names);
	d->fields.addField_bitfield(C_("RomData", "Region Code"),
		v_region_code_bitfield_names, 0, d->saturn_region);

	// Disc number
	uint8_t disc_num, disc_total;
	d->parseDiscNumber(disc_num, disc_total);
	if (disc_num != 0 && disc_total > 1) {
		const char *const disc_number_title = C_("RomData", "Disc #");
		d->fields.addField_string(disc_number_title,
			// tr: Disc X of Y (for multi-disc games)
			fmt::format(FRUN(C_("RomData|Disc", "{0:d} of {1:d}")),
				disc_num, disc_total));
	}

	// Peripherals
	static const array<const char*, 15> peripherals_bitfield_names = {{
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
	}};
	vector<string> *const v_peripherals_bitfield_names = RomFields::strArrayToVector_i18n(
		"SegaSaturn|Peripherals", peripherals_bitfield_names);
	// Parse peripherals.
	const uint32_t peripherals = d->parsePeripherals(discHeader->peripherals, sizeof(discHeader->peripherals));
	d->fields.addField_bitfield(C_("SegaSaturn", "Peripherals"),
		v_peripherals_bitfield_names, 3, peripherals);

	// Try to open the ISO-9660 object.
	// NOTE: Only done here because the ISO-9660 fields
	// are used for field info only.
	ISO isoData(d->file);
	if (isoData.isOpen()) {
		// Add the fields.
		const RomFields *const isoFields = isoData.fields();
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
int SegaSaturn::loadMetaData(void)
{
	RP_D(SegaSaturn);
	if (!d->metaData.empty()) {
		// Metadata *has* been loaded...
		return 0;
	} else if (!d->file) {
		// File isn't open.
		return -EBADF;
	} else if (!d->isValid || static_cast<int>(d->discType) < 0) {
		// Unknown disc image type.
		return -EIO;
	}

	// Sega Saturn disc header
	const Saturn_IP0000_BIN_t *const discHeader = &d->discHeader;
	d->metaData.reserve(6);	// Maximum of 6 metadata properties.

	// Title (TODO: Encoding?)
	d->metaData.addMetaData_string(Property::Title,
		latin1_to_utf8(discHeader->title, sizeof(discHeader->title)),
		RomMetaData::STRF_TRIM_END);

	// Publisher
	d->metaData.addMetaData_string(Property::Publisher, d->getPublisher());

	// Release date
	d->metaData.addMetaData_timestamp(Property::CreationDate,
		d->ascii_yyyymmdd_to_unix_time(discHeader->release_date));

	// Disc number (multiple disc sets only)
	uint8_t disc_num, disc_total;
	d->parseDiscNumber(disc_num, disc_total);
	if (disc_num != 0 && disc_total > 1) {
		d->metaData.addMetaData_integer(Property::DiscNumber, disc_num);
	}

	/** Custom properties! **/

	// Product number (as Game ID)
	d->metaData.addMetaData_string(Property::GameID,
		latin1_to_utf8(discHeader->product_number, sizeof(discHeader->product_number)),
		RomFields::STRF_TRIM_END);

	// Region code
	// NOTE: Handling Japan and Taiwan as *separate* regions.
	// For multi-region titles, region will be formatted as: "JTUE"
	const char *i18n_region = nullptr;
	for (size_t i = 0; i < d->region_code_bitfield_names.size(); i++) {
		if (d->saturn_region == (1U << i)) {
			i18n_region = d->region_code_bitfield_names[i];
			break;
		}
	}

	if (i18n_region) {
		d->metaData.addMetaData_string(Property::RegionCode,
			pgettext_expr("Region", i18n_region));
	} else {
		// Multi-region
		static const char all_display_regions[] = "JTUE";
		string s_region_code;
		s_region_code.resize(sizeof(all_display_regions)-1, '-');
		for (unsigned int i = 0; i < d->region_code_bitfield_names.size(); i++) {
			if (d->saturn_region & (1U << i)) {
				s_region_code[i] = all_display_regions[i];
			}
		}
		d->metaData.addMetaData_string(Property::RegionCode, s_region_code);
	}

	// Finished reading the metadata.
	return static_cast<int>(d->metaData.count());
}

} // namespace LibRomData
