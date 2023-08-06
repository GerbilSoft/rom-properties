/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * DMG.hpp: Game Boy (DMG/CGB/SGB) ROM reader.                             *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * Copyright (c) 2016-2018 by Egor.                                        *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "DMG.hpp"
#include "data/NintendoPublishers.hpp"
#include "data/DMGSpecialCases.hpp"
#include "dmg_structs.h"

// Other rom-properties libraries
#include "librpbase/config/Config.hpp"
using namespace LibRpBase;
using namespace LibRpText;
using namespace LibRpFile;

// For sections delegated to other RomData subclasses.
#include "Audio/GBS.hpp"
#include "Audio/gbs_structs.h"

// C++ STL classes.
using std::string;
using std::vector;

namespace LibRomData {

class DMGPrivate final : public RomDataPrivate
{
	public:
		DMGPrivate(IRpFile *file);

	private:
		typedef RomDataPrivate super;
		RP_DISABLE_COPY(DMGPrivate)

	public:
		/** RomDataInfo **/
		static const char *const exts[];
		static const char *const mimeTypes[];
		static const RomDataInfo romDataInfo;

	public:
		/** RomFields **/

		// System. (RFT_BITFIELD)
		enum DMG_System {
			DMG_SYSTEM_DMG		= (1U << 0),
			DMG_SYSTEM_SGB		= (1U << 1),
			DMG_SYSTEM_CGB		= (1U << 2),
		};

		// Cartridge hardware features. (RFT_BITFIELD)
		enum DMG_Feature {
			DMG_FEATURE_RAM		= (1U << 0),
			DMG_FEATURE_BATTERY	= (1U << 1),
			DMG_FEATURE_TIMER	= (1U << 2),
			DMG_FEATURE_RUMBLE	= (1U << 3),
			DMG_FEATURE_TILT	= (1U << 4),
		};

		/** Internal ROM data. **/

		// Cartridge hardware.
		enum class DMG_Hardware : uint8_t {
			Unknown = 0,

			ROM,
			MBC1,
			MBC2,
			MBC3,
			MBC4,
			MBC5,
			MBC6,
			MBC7,
			MMM01,
			HUC1,
			HUC3,
			TAMA5,
			Camera,

			Max
		};
		static const char *const dmg_hardware_names[];

		struct dmg_cart_type {
			DMG_Hardware hardware;
			uint8_t features;	// DMG_Feature
		};

	private:
		// Sparse array setup:
		// - "start" starts at 0x00.
		// - "end" ends at 0xFF.
		static const dmg_cart_type dmg_cart_types_start[];
		static const dmg_cart_type dmg_cart_types_end[];

	public:
		/**
		 * Get the system ID for the current ROM image.
		 * @return System ID. (DMG_System bitfield)
		 */
		uint32_t systemID(void) const;

		/**
		 * Get a dmg_cart_type struct describing a cartridge type byte.
		 * @param type Cartridge type byte.
		 * @return dmg_cart_type struct.
		 */
		static inline const dmg_cart_type& CartType(uint8_t type);

		/**
		 * Convert the ROM size value to an actual size.
		 * @param type ROM size value.
		 * @return ROM size, in kilobytes. (-1 on error)
		 */
		static inline int RomSize(uint8_t type);

	public:
		/**
		 * DMG RAM size array.
		 */
		static const uint8_t dmg_ram_size[];

		/**
		 * Nintendo's logo which is checked by bootrom.
		 * (Top half only.)
		 * 
		 * NOTE: CGB bootrom only checks the top half of the logo.
		 * (see 0x00D1 of CGB IPL)
		 */
		static const uint8_t dmg_nintendo[0x18];

	public:
		enum class RomType {
			Unknown	= -1,

			DMG	= 0,	// Game Boy
			CGB	= 1,	// Game Boy Color

			Max
		};
		RomType romType;

	public:
		// ROM header.
		DMG_RomHeader romHeader;

		// GBX footer.
		GBX_Footer gbxFooter;

		/**
		 * Get the title and game ID.
		 *
		 * NOTE: These have to be handled at the same time because
		 * later games take bytes away from the title field to use
		 * for the CGB flag and the game ID.
		 *
		 * @param s_title	[out] Title.
		 * @param s_gameID	[out] Game ID, or empty string if not available.
		 */
		void getTitleAndGameID(string &s_title, string &s_gameID) const;

		/**
		 * Get the publisher.
		 * @return Publisher, or "Unknown (xxx)" if unknown.
		 */
		string getPublisher(void) const;
};

ROMDATA_IMPL(DMG)

/** DMGPrivate **/

/* RomDataInfo */
const char *const DMGPrivate::exts[] = {
	".gb",  ".sgb", ".sgb2",
	".gbc", ".cgb",

	// ROMs with GBX footer.
	".gbx",

	nullptr
};
const char *const DMGPrivate::mimeTypes[] = {
	// Unofficial MIME types from FreeDesktop.org.
	"application/x-gameboy-rom",
	"application/x-gameboy-color-rom",

	nullptr
};
const RomDataInfo DMGPrivate::romDataInfo = {
	"DMG", exts, mimeTypes
};

/** Internal ROM data. **/

// Cartrige hardware.
const char *const DMGPrivate::dmg_hardware_names[] = {
	"Unknown",
	"ROM",
	"MBC1",
	"MBC2",
	"MBC3",
	"MBC4",
	"MBC5",
	"MBC6",
	"MBC7",
	"MMM01",
	"HuC1",
	"HuC3",
	"TAMA5",
	"POCKET CAMERA", // ???
};

const DMGPrivate::dmg_cart_type DMGPrivate::dmg_cart_types_start[] = {
	{DMG_Hardware::ROM,	0},
	{DMG_Hardware::MBC1,	0},
	{DMG_Hardware::MBC1,	DMG_FEATURE_RAM},
	{DMG_Hardware::MBC1,	DMG_FEATURE_RAM|DMG_FEATURE_BATTERY},
	{DMG_Hardware::Unknown,	0},
	{DMG_Hardware::MBC2,	0},
	{DMG_Hardware::MBC2,	DMG_FEATURE_BATTERY},
	{DMG_Hardware::Unknown,	0},
	{DMG_Hardware::ROM,	DMG_FEATURE_RAM},
	{DMG_Hardware::ROM,	DMG_FEATURE_RAM|DMG_FEATURE_BATTERY},
	{DMG_Hardware::Unknown,	0},
	{DMG_Hardware::MMM01,	0},
	{DMG_Hardware::MMM01,	DMG_FEATURE_RAM},
	{DMG_Hardware::MMM01,	DMG_FEATURE_RAM|DMG_FEATURE_BATTERY},
	{DMG_Hardware::Unknown,	0},
	{DMG_Hardware::MBC3,	DMG_FEATURE_TIMER|DMG_FEATURE_BATTERY},
	{DMG_Hardware::MBC3,	DMG_FEATURE_TIMER|DMG_FEATURE_RAM|DMG_FEATURE_BATTERY},
	{DMG_Hardware::MBC3,	0},
	{DMG_Hardware::MBC3,	DMG_FEATURE_RAM},
	{DMG_Hardware::MBC3,	DMG_FEATURE_RAM|DMG_FEATURE_BATTERY},
	{DMG_Hardware::Unknown,	0},
	{DMG_Hardware::MBC4,	0},
	{DMG_Hardware::MBC4,	DMG_FEATURE_RAM},
	{DMG_Hardware::MBC4,	DMG_FEATURE_RAM|DMG_FEATURE_BATTERY},
	{DMG_Hardware::Unknown,	0},
	{DMG_Hardware::MBC5,	0},
	{DMG_Hardware::MBC5,	DMG_FEATURE_RAM},
	{DMG_Hardware::MBC5,	DMG_FEATURE_RAM|DMG_FEATURE_BATTERY},
	{DMG_Hardware::MBC5,	DMG_FEATURE_RUMBLE},
	{DMG_Hardware::MBC5,	DMG_FEATURE_RUMBLE|DMG_FEATURE_RAM},
	{DMG_Hardware::MBC5,	DMG_FEATURE_RUMBLE|DMG_FEATURE_RAM|DMG_FEATURE_BATTERY},
	{DMG_Hardware::Unknown,	0},
	{DMG_Hardware::MBC6,	0},
	{DMG_Hardware::Unknown,	0},
	{DMG_Hardware::MBC7,	DMG_FEATURE_TILT|DMG_FEATURE_RAM|DMG_FEATURE_BATTERY},
};

const DMGPrivate::dmg_cart_type DMGPrivate::dmg_cart_types_end[] = {
	{DMG_Hardware::Camera, 0},
	{DMG_Hardware::TAMA5, 0},
	{DMG_Hardware::HUC3, 0},
	{DMG_Hardware::HUC1, DMG_FEATURE_RAM|DMG_FEATURE_BATTERY},
};

DMGPrivate::DMGPrivate(IRpFile *file)
	: super(file, &romDataInfo)
	, romType(RomType::Unknown)
{
	// Clear the various structs.
	memset(&romHeader, 0, sizeof(romHeader));
	memset(&gbxFooter, 0, sizeof(gbxFooter));
}

/**
 * Get the system ID for the current ROM image.
 * @return System ID. (DMG_System bitfield)
 */
uint32_t DMGPrivate::systemID(void) const
{
	uint32_t ret = 0;

	if (romHeader.cgbflag & 0x80) {
		// Game supports CGB.
		ret = DMG_SYSTEM_CGB;
		if (!(romHeader.cgbflag & 0x40)) {
			// Not CGB exclusive.
			ret |= DMG_SYSTEM_DMG;
		}
	} else {
		// Game does not support CGB.
		ret |= DMG_SYSTEM_DMG;
	}

	if (romHeader.old_publisher_code == 0x33 && romHeader.sgbflag == 0x03) {
		// Game supports SGB.
		ret |= DMG_SYSTEM_SGB;
	}

	assert(ret != 0);
	return ret;
}

/**
 * Get a dmg_cart_type struct describing a cartridge type byte.
 * @param type Cartridge type byte.
 * @return dmg_cart_type struct.
 */
inline const DMGPrivate::dmg_cart_type& DMGPrivate::CartType(uint8_t type)
{
	static const dmg_cart_type unk = {DMG_Hardware::Unknown, 0};
	if (type < ARRAY_SIZE(dmg_cart_types_start)) {
		return dmg_cart_types_start[type];
	}
	const unsigned end_offset = 0x100u-ARRAY_SIZE(dmg_cart_types_end);
	if (type>=end_offset) {
		return dmg_cart_types_end[type-end_offset];
	}
	return unk;
}

/**
 * Convert the ROM size value to an actual size.
 * @param type ROM size value.
 * @return ROM size, in kilobytes. (-1 on error)
 */
inline int DMGPrivate::RomSize(uint8_t type)
{
	static const int rom_size[] = {32, 64, 128, 256, 512, 1024, 2048, 4096};
	static const int rom_size_52[] = {1152, 1280, 1536};
	if (type < ARRAY_SIZE(rom_size)) {
		return rom_size[type];
	} else if (type >= 0x52 && type < 0x52+ARRAY_SIZE(rom_size_52)) {
		return rom_size_52[type-0x52];
	}
	return -1;
}

/**
 * DMG RAM size array.
 */
const uint8_t DMGPrivate::dmg_ram_size[] = {
	0, 2, 8, 32, 128, 64
};

/**
 * Nintendo's logo which is checked by bootrom.
 * (Top half only.)
 * 
 * NOTE: CGB bootrom only checks the top half of the logo.
 * (see 0x00D1 of CGB IPL)
 */
const uint8_t DMGPrivate::dmg_nintendo[0x18] = {
	0xCE, 0xED, 0x66, 0x66, 0xCC, 0x0D, 0x00, 0x0B,
	0x03, 0x73, 0x00, 0x83, 0x00, 0x0C, 0x00, 0x0D,
	0x00, 0x08, 0x11, 0x1F, 0x88, 0x89, 0x00, 0x0E
};

/**
 * Get the title and game ID.
 *
 * NOTE: These have to be handled at the same time because
 * later games take bytes away from the title field to use
 * for the CGB flag and the game ID.
 *
 * @param s_title	[out] Title.
 * @param s_gameID	[out] Game ID, or empty string if not available.
 */
void DMGPrivate::getTitleAndGameID(string &s_title, string &s_gameID) const
{
	/**
	 * NOTE: there are two approaches for doing this, when the 15 bytes are all used
	 * 1) prioritize id
	 * 2) prioritize title
	 * Both of those have counter examples:
	 * If you do the first, you will get "SUPER MARIO" and "LAND" on super mario land rom
	 * With the second one, you will get "MARIO DELUXAHYJ" and Unknown on super mario deluxe rom
	 *
	 * Current method is the first one.
	 *
	 * Encoding: The German version of "Wheel of Fortune" is called
	 * "Glücksrad", and the title has 0x9A, which is cp437.
	 * TODO: GBC titles might use cp1252 instead.
	 */
	bool isGameID = false;
	if (romHeader.cgbflag < 0x80) {
		// Assuming 16-character title for non-CGB.
		// Game ID is not present.
		s_title = cpN_to_utf8(437, romHeader.title16, sizeof(romHeader.title16));
		s_gameID.clear();
		goto trimTitle;
	}

	// Check if the CGB flag is set.
	if ((romHeader.cgbflag & 0x3F) == 0) {
		// CGB flag is set.
		// Check if a Game ID is present.
		isGameID = true;
		// First character must be: [ABHKV] [TODO: Verify HW byte?]
		// Region byte must be: [ADEFGHIJKPSXY]
		switch (romHeader.title15[11]) {
			case 'A':
			case 'B':
			case 'H':	// IR sensor
			case 'K':	// tilt sensor
			case 'V':	// rumble
				switch (romHeader.title15[14]) {
					case 'A': case 'B':	// B == Brazil
					case 'D': case 'E':
					case 'F': case 'G':
					case 'H': case 'I':
					case 'J': case 'K':
					case 'P': case 'S':
					case 'U': case 'X':
					case 'Y': case 'Z':
						// Region byte is valid.
						break;

					default:
						// Region byte is invalid.
						// This is not a Game ID.
						isGameID = false;
						break;
				}
				break;

			default:
				// First character is invalid.
				// This is not a Game ID.
				isGameID = false;
				break;
		}

		if (isGameID) {
			// Second and third bytes must be uppercase and/or numeric.
			if ((!ISUPPER(romHeader.title15[12]) && !ISDIGIT(romHeader.title15[12])) ||
			    (!ISUPPER(romHeader.title15[13]) && !ISDIGIT(romHeader.title15[13])))
			{
				// This is not a Game ID.
				isGameID = false;
			}
		}
	}

	if (isGameID) {
		// Game ID is present.
		s_title = cpN_to_utf8(437, romHeader.title11, sizeof(romHeader.title11));

		// Append the publisher code to make an ID6.
		s_gameID.clear();
		s_gameID.resize(6);
		s_gameID[0] = romHeader.id4[0];
		s_gameID[1] = romHeader.id4[1];
		s_gameID[2] = romHeader.id4[2];
		s_gameID[3] = romHeader.id4[3];
		if (romHeader.old_publisher_code == 0x33) {
			// New publisher code.
			s_gameID[4] = romHeader.new_publisher_code[0];
			s_gameID[5] = romHeader.new_publisher_code[1];
		} else {
			// Old publisher code.
			// NOTE: This probably won't ever happen,
			// since Game ID was added *after* CGB.
			static const char hex_lookup[16] = {
				'0','1','2','3','4','5','6','7',
				'8','9','A','B','C','D','E','F'
			};
			s_gameID[4] = hex_lookup[romHeader.old_publisher_code >> 4];
			s_gameID[5] = hex_lookup[romHeader.old_publisher_code & 0x0F];
		}
	} else {
		// Game ID is not present.
		s_title = cpN_to_utf8(437, romHeader.title15, sizeof(romHeader.title15));
		s_gameID.clear();
	}

trimTitle:
	// Trim trailing spaces if the title doesn't start with a space.
	// TODO: Trim leading spaces?
	if (!s_title.empty() && s_title[0] != ' ') {
		do {
			const size_t size = s_title.size();
			if (s_title[size-1] == ' ') {
				// Found a space. Remove it.
				s_title.resize(size-1);
			} else {
				// Not a space. We're done here.
				break;
			}
		} while (!s_title.empty());
	}
}

/**
 * Get the publisher.
 * @return Publisher, or "Unknown (xxx)" if unknown.
 */
string DMGPrivate::getPublisher(void) const
{
	const char* publisher;
	string s_publisher;
	if (romHeader.old_publisher_code == 0x33) {
		// New publisher code.
		publisher = NintendoPublishers::lookup(romHeader.new_publisher_code);
		if (publisher) {
			s_publisher = publisher;
		} else {
			if (ISALNUM(romHeader.new_publisher_code[0]) &&
			    ISALNUM(romHeader.new_publisher_code[1]))
			{
				s_publisher = rp_sprintf(C_("RomData", "Unknown (%.2s)"),
					romHeader.new_publisher_code);
			} else {
				s_publisher = rp_sprintf(C_("RomData", "Unknown (%02X %02X)"),
					static_cast<uint8_t>(romHeader.new_publisher_code[0]),
					static_cast<uint8_t>(romHeader.new_publisher_code[1]));
			}
		}
	} else {
		// Old publisher code.
		publisher = NintendoPublishers::lookup_old(romHeader.old_publisher_code);
		if (publisher) {
			s_publisher = publisher;
		} else {
			s_publisher = rp_sprintf(C_("RomData", "Unknown (%02X)"),
				romHeader.old_publisher_code);
		}
	}

	return s_publisher;
}

/** DMG **/

/**
 * Read a Game Boy ROM.
 *
 * A ROM file must be opened by the caller. The file handle
 * will be ref()'d and must be kept open in order to load
 * data from the ROM.
 *
 * To close the file, either delete this object or call close().
 *
 * NOTE: Check isValid() to determine if this is a valid ROM.
 *
 * @param file Open ROM file.
 */
DMG::DMG(IRpFile *file)
	: super(new DMGPrivate(file))
{
	RP_D(DMG);
	if (!d->file) {
		// Could not ref() the file handle.
		return;
	}

	// Seek to the beginning of the header.
	d->file->rewind();

	// Read the ROM header.
	// We're reading extra data in case the ROM has an extra
	// 512-byte copier header present.
	union {
		uint8_t   u8[ 0x300 + sizeof(d->romHeader)];
		uint32_t u32[(0x300 + sizeof(d->romHeader))/4];
	} header;
	size_t size = d->file->read(header.u8, sizeof(header.u8));
	if (size < (0x100 + sizeof(d->romHeader))) {
		UNREF_AND_NULL_NOCHK(d->file);
		return;
	}
	if (size < sizeof(header.u8)) {
		memset(&header.u8[size], 0, sizeof(header)-size);
	}

	// Check if this ROM is supported.
	const DetectInfo info = {
		{0, static_cast<unsigned int>(size), header.u8},
		nullptr,	// ext (not needed for DMG)
		0		// szFile (not needed for DMG)
	};
	d->romType = static_cast<DMGPrivate::RomType>(isRomSupported_static(&info));

	d->isValid = ((int)d->romType >= 0);
	if (d->isValid) {
		// Save the header for later.
		// TODO: Save the RST table?

		// Check the first DWORD of the Nintendo logo
		// to determine if a copier header is present.
		if (likely(header.u32[0x104/4] == cpu_to_be32(0xCEED6666))) {
			memcpy(&d->romHeader, &header.u8[0x100], sizeof(d->romHeader));
		} else {
			memcpy(&d->romHeader, &header.u8[0x300], sizeof(d->romHeader));
		}
	} else {
		UNREF_AND_NULL_NOCHK(d->file);
		return;
	}

	// Attempt to read the GBX footer.
	const off64_t addr = file->size() - sizeof(GBX_Footer);
	if (addr >= (off64_t)sizeof(GBX_Footer)) {
		size = file->seekAndRead(addr, &d->gbxFooter, sizeof(d->gbxFooter));
		if (size != sizeof(d->gbxFooter)) {
			// Unable to read the footer.
			// Zero out the magic number just in case.
			d->gbxFooter.magic = 0;
		}
	}

	// Set the MIME type. (unofficial)
	d->mimeType = (d->romType == DMGPrivate::RomType::CGB)
			? "application/x-gameboy-color-rom"
			: "application/x-gameboy-rom";
}

/** ROM detection functions. **/

/**
 * Is a ROM image supported by this class?
 * @param info DetectInfo containing ROM detection information.
 * @return Class-specific system ID (>= 0) if supported; -1 if not.
 */
int DMG::isRomSupported_static(const DetectInfo *info)
{
	assert(info != nullptr);
	assert(info->header.pData != nullptr);
	assert(info->header.addr == 0);
	if (!info || !info->header.pData ||
	    info->header.addr != 0 ||
	    info->header.size < (0x100 + sizeof(DMG_RomHeader)))
	{
		// Either no detection information was specified,
		// or the header is too small.
		return static_cast<int>(DMGPrivate::RomType::Unknown);
	}

	// Check for the ROM header at 0x100. (standard location)
	const DMG_RomHeader *romHeader =
		reinterpret_cast<const DMG_RomHeader*>(&info->header.pData[0x100]);
	if (!memcmp(romHeader->nintendo, DMGPrivate::dmg_nintendo,
	     sizeof(DMGPrivate::dmg_nintendo)))
	{
		// Found at the standard location.
		DMGPrivate::RomType romType;
		if (romHeader->cgbflag & 0x80) {
			romType = DMGPrivate::RomType::CGB; // CGB supported
		} else {
			romType = DMGPrivate::RomType::DMG;
		}
		return (int)romType;
	}

	// Not found at the standard location.

	// A few DMG ROMs have a 512-byte copier header, similar to
	// Super Magic Drive (MD) and Super Wild Card (SNES).
	// The header is sometimes missing the identification bytes
	// (0xAA, 0xBB), so we'll just check for zero bytes.
	if (info->header.size >= 0x300 + sizeof(DMG_RomHeader)) {
		const uint32_t *const pData32 =
			reinterpret_cast<const uint32_t*>(info->header.pData);
		if (pData32[0x10/4] == 0 && pData32[0x14/4] == 0 &&
		    pData32[0x18/4] == 0 && pData32[0x1C/4] == 0)
		{
			// Check the headered location.
			romHeader = reinterpret_cast<const DMG_RomHeader*>(&info->header.pData[0x300]);
			if (!memcmp(romHeader->nintendo, DMGPrivate::dmg_nintendo,
			     sizeof(DMGPrivate::dmg_nintendo)))
			{
				// Found at the headered location.
				DMGPrivate::RomType romType;
				if (romHeader->cgbflag & 0x80) {
					romType = DMGPrivate::RomType::CGB; // CGB supported
				} else {
					romType = DMGPrivate::RomType::DMG;
				}
				return (int)romType;
			}
		}
	}

	// Not supported.
	return (int)DMGPrivate::RomType::Unknown;
}

/**
 * Get the name of the system the loaded ROM is designed for.
 * @param type System name type. (See the SystemName enum.)
 * @return System name, or nullptr if type is invalid.
 */
const char *DMG::systemName(unsigned int type) const
{
	RP_D(const DMG);
	if (!d->isValid || !isSystemNameTypeValid(type))
		return nullptr;

	// GB/GBC have the same names worldwide, so we can
	// ignore the region selection.
	// TODO: Abbreviation might be different... (Japan uses DMG/CGB?)
	static_assert(SYSNAME_TYPE_MASK == 3,
		"DMG::systemName() array index optimization needs to be updated.");
	static_assert((int)DMGPrivate::RomType::Max == 2,
		"DMG::systemName() array index optimization needs to be updated.");

	// Bits 0-1: Type. (long, short, abbreviation)
	// Bit 2: Game Boy Color. (DMG-specific)
	static const char *const sysNames[2][4] = {
		{"Nintendo Game Boy", "Game Boy", "GB", nullptr},
		{"Nintendo Game Boy Color", "Game Boy Color", "GBC", nullptr}
	};

	// NOTE: This might return an incorrect system name if
	// d->romType is ROM_TYPE_UNKNOWN.
	return sysNames[(int)d->romType & 1][type & SYSNAME_TYPE_MASK];
}

/**
 * Get a bitfield of image types this class can retrieve.
 * @return Bitfield of supported image types. (ImageTypesBF)
 */
uint32_t DMG::supportedImageTypes_static(void)
{
	return IMGBF_EXT_TITLE_SCREEN;
}

/**
 * Get a bitfield of image types this object can retrieve.
 * @return Bitfield of supported image types. (ImageTypesBF)
 */
uint32_t DMG::supportedImageTypes(void) const
{
	return supportedImageTypes_static();
}

/**
 * Get a list of all available image sizes for the specified image type.
 * @param imageType Image type.
 * @return Vector of available image sizes, or empty vector if no images are available.
 */
vector<RomData::ImageSizeDef> DMG::supportedImageSizes(ImageType imageType) const
{
	ASSERT_supportedImageSizes(imageType);

	switch (imageType) {
		case IMG_EXT_TITLE_SCREEN: {
			static const ImageSizeDef sz_EXT_TITLE_SCREEN_DMG[] = {
				{nullptr, 160, 144, 0},
			};
			static const ImageSizeDef sz_EXT_TITLE_SCREEN_SGB[] = {
				{nullptr, 256, 224, 0},
			};

			// If this game supports SGB but not CGB, we'll have an SGB border.
			RP_D(const DMG);
			const uint32_t dmg_system = d->systemID();
			if (dmg_system & DMGPrivate::DMG_SYSTEM_SGB) {
				if (!(dmg_system & DMGPrivate::DMG_SYSTEM_CGB)) {
					// SGB but not CGB.
					return vector<ImageSizeDef>(sz_EXT_TITLE_SCREEN_SGB,
						sz_EXT_TITLE_SCREEN_SGB + 1);
				}
			}

			// Not SGB, or has CGB.
			return vector<ImageSizeDef>(sz_EXT_TITLE_SCREEN_DMG,
				sz_EXT_TITLE_SCREEN_DMG + 1);
		}
		default:
			break;
	}

	// Unsupported image type.
	return vector<ImageSizeDef>();
}

/**
 * Get image processing flags.
 *
 * These specify post-processing operations for images,
 * e.g. applying transparency masks.
 *
 * @param imageType Image type.
 * @return Bitfield of ImageProcessingBF operations to perform.
 */
uint32_t DMG::imgpf(ImageType imageType) const
{
	ASSERT_imgpf(imageType);

	uint32_t ret = 0;
	switch (imageType) {
		case IMG_EXT_TITLE_SCREEN:
			// Use nearest-neighbor scaling when resizing.
			ret = IMGPF_RESCALE_NEAREST;
			break;

		default:
			// GameTDB's Nintendo DS cover scans have alpha transparency.
			// Hence, no image processing is required.
			break;
	}
	return ret;
}

/**
 * Load field data.
 * Called by RomData::fields() if the field data hasn't been loaded yet.
 * @return Number of fields read on success; negative POSIX error code on error.
 */
int DMG::loadFieldData(void)
{
	RP_D(DMG);
	if (!d->fields.empty()) {
		// Field data *has* been loaded...
		return 0;
	} else if (!d->file || !d->file->isOpen()) {
		// File isn't open.
		return -EBADF;
	} else if (!d->isValid || (int)d->romType < 0) {
		// Unknown ROM image type.
		return -EIO;
	}

	// DMG ROM header, excluding the RST table.
	const DMG_RomHeader *const romHeader = &d->romHeader;

	// DMG ROM header:
	// - 12 regular fields.
	// - 5 fields for the GBX footer.
	d->fields.reserve(12+5);

	// Reserve at least 3 tabs:
	// DMG, GBX, GBS
	d->fields.reserveTabs(3);

	// Title and game ID
	// NOTE: These have to be handled at the same time because
	// later games take bytes away from the title field to use
	// for the CGB flag and the game ID.
	string s_title, s_gameID;
	d->getTitleAndGameID(s_title, s_gameID);
	d->fields.addField_string(C_("RomData", "Title"), s_title);
	d->fields.addField_string(C_("RomData", "Game ID"),
		!s_gameID.empty() ? s_gameID.c_str() : C_("RomData", "Unknown"));

	// System
	const uint32_t dmg_system = d->systemID();
	static const char *const system_bitfield_names[] = {
		"DMG", "SGB", "CGB"
	};
	vector<string> *const v_system_bitfield_names = RomFields::strArrayToVector(
		system_bitfield_names, ARRAY_SIZE(system_bitfield_names));
	d->fields.addField_bitfield(C_("DMG", "System"),
		v_system_bitfield_names, 0, dmg_system);

	// Set the tab name based on the system.
	if (dmg_system & DMGPrivate::DMG_SYSTEM_CGB) {
		d->fields.setTabName(0, "CGB");
	} else if (dmg_system & DMGPrivate::DMG_SYSTEM_SGB) {
		d->fields.setTabName(0, "SGB");
	} else {
		d->fields.setTabName(0, "DMG");
	}

	// Entry Point
	const char *const entry_point_title = C_("RomData", "Entry Point");
	if ((romHeader->entry[0] == 0x00 ||	// NOP
	     romHeader->entry[0] == 0xF3 ||	// DI
	     romHeader->entry[0] == 0x7F ||	// LD A,A
	     romHeader->entry[0] == 0x3F) &&	// CCF
	     romHeader->entry[1] == 0xC3)	// JP nnnn
	{
		// NOP; JP nnnn
		// This is the "standard" way of doing the entry point.
		// NOTE: Some titles use a different opcode instead of NOP.
		const uint16_t entry_address = (romHeader->entry[2] | (romHeader->entry[3] << 8));
		d->fields.addField_string_numeric(entry_point_title,
			entry_address, RomFields::Base::Hex, 4, RomFields::STRF_MONOSPACE);
	} else if (romHeader->entry[0] == 0xC3) {
		// JP nnnn without a NOP.
		const uint16_t entry_address = (romHeader->entry[1] | (romHeader->entry[2] << 8));
		d->fields.addField_string_numeric(entry_point_title,
			entry_address, RomFields::Base::Hex, 4, RomFields::STRF_MONOSPACE);
	} else if (romHeader->entry[0] == 0x18) {
		// JR nnnn
		// Found in many homebrew ROMs.
		const int8_t disp = static_cast<int8_t>(romHeader->entry[1]);
		// Current PC: 0x100
		// Add displacement, plus 2.
		const uint16_t entry_address = 0x100 + disp + 2;
		d->fields.addField_string_numeric(entry_point_title,
			entry_address, RomFields::Base::Hex, 4, RomFields::STRF_MONOSPACE);
	} else {
		d->fields.addField_string_hexdump(entry_point_title,
			romHeader->entry, 4, RomFields::STRF_MONOSPACE);
	}

	// Publisher
	d->fields.addField_string(C_("RomData", "Publisher"), d->getPublisher());

	// Hardware
	d->fields.addField_string(C_("DMG", "Hardware"),
		DMGPrivate::dmg_hardware_names[(int)DMGPrivate::CartType(romHeader->cart_type).hardware]);

	// Features
	static const char *const feature_bitfield_names[] = {
		NOP_C_("DMG|Features", "RAM"),
		NOP_C_("DMG|Features", "Battery"),
		NOP_C_("DMG|Features", "Timer"),
		NOP_C_("DMG|Features", "Rumble"),
		NOP_C_("DMG|Features", "Tilt Sensor"),
	};
	vector<string> *const v_feature_bitfield_names = RomFields::strArrayToVector_i18n(
		"DMG|Features", feature_bitfield_names, ARRAY_SIZE(feature_bitfield_names));
	d->fields.addField_bitfield(C_("DMG", "Features"),
		v_feature_bitfield_names, 3, DMGPrivate::CartType(romHeader->cart_type).features);

	// ROM Size
	const char *const rom_size_title = C_("DMG", "ROM Size");
	const int rom_size = DMGPrivate::RomSize(romHeader->rom_size);
	if (rom_size < 0) {
		d->fields.addField_string(rom_size_title, C_("DMG", "Unknown"));
	} else {
		if (rom_size > 32) {
			const int banks = rom_size / 16;
			d->fields.addField_string(rom_size_title,
				rp_sprintf_p(NC_("DMG", "%1$u KiB (%2$u bank)", "%1$u KiB (%2$u banks)", banks),
					static_cast<unsigned int>(rom_size),
					static_cast<unsigned int>(banks)));
		} else {
			d->fields.addField_string(rom_size_title,
				rp_sprintf(C_("DMG", "%u KiB"), static_cast<unsigned int>(rom_size)));
		}
	}

	// RAM Size
	const char *const ram_size_title = C_("DMG", "RAM Size");
	if (romHeader->ram_size >= ARRAY_SIZE(DMGPrivate::dmg_ram_size)) {
		d->fields.addField_string(ram_size_title, C_("RomData", "Unknown"));
	} else {
		const uint8_t ram_size = DMGPrivate::dmg_ram_size[romHeader->ram_size];
		if (ram_size == 0 &&
		    DMGPrivate::CartType(romHeader->cart_type).hardware == DMGPrivate::DMG_Hardware::MBC2)
		{
			d->fields.addField_string(ram_size_title,
				// tr: MBC2 internal memory - Not really RAM, but whatever.
				C_("DMG", "512 x 4 bits"));
		} else if(ram_size == 0) {
			d->fields.addField_string(ram_size_title, C_("DMG", "No RAM"));
		} else {
			if (ram_size > 8) {
				const int banks = ram_size / 8;
				d->fields.addField_string(ram_size_title,
					rp_sprintf_p(NC_("DMG", "%1$u KiB (%2$u bank)", "%1$u KiB (%2$u banks)", banks),
						static_cast<unsigned int>(ram_size),
						static_cast<unsigned int>(banks)));
			} else {
				d->fields.addField_string(ram_size_title,
					rp_sprintf(C_("DMG", "%u KiB"), static_cast<unsigned int>(ram_size)));
			}
		}
	}

	// Region Code
	const char *const region_code_title = C_("RomData", "Region Code");
	switch (romHeader->region) {
		case 0:
			d->fields.addField_string(region_code_title,
				C_("Region|DMG", "Japanese"));
			break;
		case 1:
			d->fields.addField_string(region_code_title,
				C_("Region|DMG", "Non-Japanese"));
			break;
		default:
			// Invalid value.
			d->fields.addField_string(region_code_title,
				rp_sprintf(C_("DMG", "0x%02X (INVALID)"), romHeader->region));
			break;
	}

	// Revision
	d->fields.addField_string_numeric(C_("RomData", "Revision"),
		romHeader->version, RomFields::Base::Dec, 2);

	// Header checksum.
	// This is a checksum of ROM addresses 0x134-0x14D.
	// Note that romHeader is a copy of the ROM header
	// starting at 0x100, so the values are offset accordingly.
	uint8_t checksum = 0xE7; // -0x19
	const uint8_t *romHeader8 = reinterpret_cast<const uint8_t*>(romHeader);
	for (unsigned int i = 0x0034; i < 0x004D; i++){
		checksum -= romHeader8[i];
	}

	const char *const checksum_title = C_("RomData", "Checksum");
	if (checksum - romHeader->header_checksum != 0) {
		d->fields.addField_string(checksum_title,
			rp_sprintf_p(C_("DMG", "0x%1$02X (INVALID; should be 0x%2$02X)"),
				romHeader->header_checksum, checksum));
	} else {
		d->fields.addField_string(checksum_title,
			rp_sprintf(C_("DMG", "0x%02X (valid)"), checksum));
	}

	/** GBX footer. **/
	const GBX_Footer *const gbxFooter = &d->gbxFooter;
	if (gbxFooter->magic == cpu_to_be32(GBX_MAGIC)) {
		// GBX footer is present.
		d->fields.addTab("GBX");

		// GBX version.
		// TODO: Do things based on the version number?
		d->fields.addField_string(C_("DMG", "GBX Version"),
			rp_sprintf_p("%1$u.%2$u",
				be32_to_cpu(gbxFooter->version.major),
				be32_to_cpu(gbxFooter->version.minor)));

		// Mapper.
		struct gbx_mapper_tbl_t {
			GBX_Mapper_e mapper_id;	// Host-endian
			const char *desc;
		};

		// TODO: Localization?
		static const gbx_mapper_tbl_t gbx_mapper_tbl[] = {
			// Nintendo
			{GBX_MAPPER_ROM_ONLY,		"ROM only"},
			{GBX_MAPPER_MBC1,		"Nintendo MBC1"},
			{GBX_MAPPER_MBC2,		"Nintendo MBC2"},
			{GBX_MAPPER_MBC3,		"Nintendo MBC3"},
			{GBX_MAPPER_MBC5,		"Nintendo MBC5"},
			{GBX_MAPPER_MBC7,		"Nintendo MBC7 (tilt sensor)"},
			{GBX_MAPPER_MBC1_MULTICART,	"Nintendo MBC1 multicart"},
			{GBX_MAPPER_MMM01,		"Nintendo/Mani MMM01"},
			{GBX_MAPPER_POCKET_CAMERA,	"Nintendo Game Boy Camera"},

			// Licensed third-party
			{GBX_MAPPER_HuC1,		"Hudson HuC1"},
			{GBX_MAPPER_HuC3,		"Hudson HuC3"},
			{GBX_MAPPER_TAMA5,		"Bandai TAMA5"},

			// Unlicensed
			{GBX_MAPPER_BBD,		"BBD"},
			{GBX_MAPPER_HITEK,		"Hitek"},
			{GBX_MAPPER_SINTAX,		"Sintax"},
			{GBX_MAPPER_NT_OLDER_TYPE_1,	"NT older type 1"},
			{GBX_MAPPER_NT_OLDER_TYPE_2,	"NT older type 2"},
			{GBX_MAPPER_NT_NEWER,		"NT newer"},
			{GBX_MAPPER_LI_CHENG,		"Li Cheng"},
			{GBX_MAPPER_LAST_BIBLE,		"\"Last Bible\" multicart"},
			{GBX_MAPPER_LIEBAO,		"Liebao Technology"},
		};
		static const gbx_mapper_tbl_t *const p_gbx_mapper_tbl_end = &gbx_mapper_tbl[ARRAY_SIZE(gbx_mapper_tbl)];

		const char *s_mapper = nullptr;
		const uint32_t lkup = be32_to_cpu(gbxFooter->mapper_id);
		auto iter = std::find_if(gbx_mapper_tbl, p_gbx_mapper_tbl_end,
			[lkup](const gbx_mapper_tbl_t &p) noexcept -> bool {
				return (p.mapper_id == lkup);
			});
		if (iter != p_gbx_mapper_tbl_end) {
			// Found the mapper.
			s_mapper = iter->desc;
		}

		if (s_mapper) {
			d->fields.addField_string(C_("DMG", "Mapper"), s_mapper);
		} else {
			// If the mapper ID is all printable characters, print the mapper as text.
			// Otherwise, print a hexdump.
			if (ISPRINT(gbxFooter->mapper[0]) &&
			    ISPRINT(gbxFooter->mapper[1]) &&
			    ISPRINT(gbxFooter->mapper[2]) &&
			    ISPRINT(gbxFooter->mapper[3]))
			{
				// All printable.
				d->fields.addField_string(C_("DMG", "Mapper"),
					latin1_to_utf8(gbxFooter->mapper, sizeof(gbxFooter->mapper)),
					RomFields::STRF_MONOSPACE);
			} else {
				// Not printable. Print a hexdump.
				d->fields.addField_string_hexdump(C_("DMG", "Mapper"),
					reinterpret_cast<const uint8_t*>(&gbxFooter->mapper[0]),
					sizeof(gbxFooter->mapper),
					RomFields::STRF_MONOSPACE);
			}
		}

		// Features.
		// NOTE: Same strings as the regular DMG header,
		// but the bitfield ordering is different.
		// NOTE: GBX spec says 00 = not present, 01 = present.
		// Assuming any non-zero value is present.
		uint32_t gbx_features = 0;
		if (gbxFooter->battery_flag) {
			gbx_features |= (1U << 0);
		}
		if (gbxFooter->rumble_flag) {
			gbx_features |= (1U << 1);
		}
		if (gbxFooter->timer_flag) {
			gbx_features |= (1U << 2);
		}

		static const char *const gbx_feature_bitfield_names[] = {
			NOP_C_("DMG|Features", "Battery"),
			NOP_C_("DMG|Features", "Rumble"),
			NOP_C_("DMG|Features", "Timer"),
		};
		vector<string> *const v_gbx_feature_bitfield_names = RomFields::strArrayToVector_i18n(
			"DMG|Features", gbx_feature_bitfield_names, ARRAY_SIZE(gbx_feature_bitfield_names));
		d->fields.addField_bitfield(C_("DMG", "Features"),
			v_gbx_feature_bitfield_names, 0, gbx_features);

		// ROM size, in bytes. (formatted as KiB)
		d->fields.addField_string(C_("DMG", "ROM Size"),
			formatFileSizeKiB(be32_to_cpu(gbxFooter->rom_size)));

		// RAM size, in bytes.
		// TODO: Use formatFileSize() instead?
		d->fields.addField_string(C_("DMG", "RAM Size"),
			formatFileSizeKiB(be32_to_cpu(gbxFooter->ram_size)));
	}

	/** GBS **/
	// Check for GBS.
	// NOTE: Loaded on demand, since GBS isn't used for metadata at the moment.
	// TODO: Maybe it should be?
	uint8_t gbs_jmp[3];
	size_t size = d->file->seekAndRead(0, gbs_jmp, sizeof(gbs_jmp));
	if (size == sizeof(gbs_jmp) && gbs_jmp[0] == 0xC3) {
		// Read the jump address.
		// GBS header is at the jump address minus sizeof(GBS_Header).
		uint16_t jp_addr = (gbs_jmp[2] << 8) | gbs_jmp[1];
		if (jp_addr >= sizeof(GBS_Header)) {
			jp_addr -= sizeof(GBS_Header);
			// Read the GBS magic number.
			uint32_t gbs_magic;
			size = d->file->seekAndRead(jp_addr, &gbs_magic, sizeof(gbs_magic));
			if (size == sizeof(gbs_magic) && gbs_magic == cpu_to_be32(GBS_MAGIC)) {
				// Found the GBS magic number.
				// Open the GBS.
				const off64_t fileSize = d->file->size();
				SubFile *const gbsFile = new SubFile(d->file, jp_addr, fileSize - jp_addr);
				if (gbsFile->isOpen()) {
					// Open the GBS.
					GBS *const gbs = new GBS(gbsFile);
					if (gbs->isOpen()) {
						// Add the fields.
						const RomFields *const gbsFields = gbs->fields();
						assert(gbsFields != nullptr);
						assert(!gbsFields->empty());
						if (gbsFields && !gbsFields->empty()) {
							d->fields.addFields_romFields(gbsFields,
								RomFields::TabOffset_AddTabs);
						}
					}
					gbs->unref();
				}
				gbsFile->unref();
			}
		}
	}

	// Finished reading the field data.
	return static_cast<int>(d->fields.count());
}

/**
 * Load metadata properties.
 * Called by RomData::metaData() if the field data hasn't been loaded yet.
 * @return Number of metadata properties read on success; negative POSIX error code on error.
 */
int DMG::loadMetaData(void)
{
	RP_D(DMG);
	if (d->metaData != nullptr) {
		// Metadata *has* been loaded...
		return 0;
	} else if (!d->file) {
		// File isn't open.
		return -EBADF;
	} else if (!d->isValid || (int)d->romType < 0) {
		// Unknown ROM image type.
		return -EIO;
	}

	// Create the metadata object.
	d->metaData = new RomMetaData();
	d->metaData->reserve(2);	// Maximum of 2 metadata properties.

	// DMG ROM header
	//const DMG_RomHeader *const romHeader = &d->romHeader;

	// Title
	// NOTE: We don't actually need the game ID right now,
	// but the function retrieves both at the same time.
	// TODO: Remove STRF_TRIM_END, since we're doing that ourselves?
	string s_title, s_gameID;
	d->getTitleAndGameID(s_title, s_gameID);
	if (!s_title.empty()) {
		d->metaData->addMetaData_string(Property::Title,
			s_title, RomMetaData::STRF_TRIM_END);
	}

	// Publisher
	d->metaData->addMetaData_string(Property::Publisher, d->getPublisher());

	// Finished reading the metadata.
	return static_cast<int>(d->metaData->count());
}

/**
 * Get a list of URLs for an external image type.
 *
 * A thumbnail size may be requested from the shell.
 * If the subclass supports multiple sizes, it should
 * try to get the size that most closely matches the
 * requested size.
 *
 * @param imageType	[in]     Image type.
 * @param pExtURLs	[out]    Output vector.
 * @param size		[in,opt] Requested image size. This may be a requested
 *                               thumbnail size in pixels, or an ImageSizeType
 *                               enum value.
 * @return 0 on success; negative POSIX error code on error.
 */
int DMG::extURLs(ImageType imageType, vector<ExtURL> *pExtURLs, int size) const
{
	ASSERT_extURLs(imageType, pExtURLs);
	pExtURLs->clear();

	RP_D(const DMG);
	if (!d->isValid || (int)d->romType < 0) {
		// ROM image isn't valid.
		return -EIO;
	}

	// Check if we have a valid game ID or ROM title.
	string s_title, s_gameID;
	d->getTitleAndGameID(s_title, s_gameID);
	if (s_gameID.empty() && s_title.empty()) {
		// Empty IDs. Can't check this ROM image.
		return -ENOENT;
	}

	if (!s_gameID.empty()) {
		assert(s_gameID.size() >= 4);
		if (s_gameID.size() < 4) {
			// Invalid game ID.
			return -EIO;
		}
	}

	// DMG ROM header, excluding the RST table.
	const DMG_RomHeader *const romHeader = &d->romHeader;
	const uint32_t dmg_system = d->systemID();

	// We'll need to append global checksums in cases where
	// we don't have a game ID and the title collides with
	// different ROM images.
	bool append_cksum = false;

	// Check the title screen mode variant to use.
	static const char ts_subdirs
		[Config::DMG_TitleScreen_Mode::DMG_TS_MAX]
		[Config::DMG_TitleScreen_Mode::DMG_TS_MAX][8] =
	{
		// Rows: ROM type (DMG, SGB, CGB)
		// Columns: User selection (DMG, SGB, CGB)

		// DMG (NOTE: DMG-SGB is not a thing; use SGB instead)
		{"DMG", "DMG", "DMG-CGB"},

		// SGB
		{"SGB-DMG", "SGB", "SGB-CGB"},

		// CGB
		{"CGB-DMG", "CGB-SGB", "CGB"},
	};

	string img_subdir;
	const Config *const config = Config::instance();
	Config::DMG_TitleScreen_Mode cfg_rom;
	if (dmg_system & DMGPrivate::DMG_SYSTEM_CGB) {
		cfg_rom = Config::DMG_TitleScreen_Mode::DMG_TS_CGB;
	} else if (dmg_system & DMGPrivate::DMG_SYSTEM_SGB) {
		cfg_rom = Config::DMG_TitleScreen_Mode::DMG_TS_SGB;
	} else {
		cfg_rom = Config::DMG_TitleScreen_Mode::DMG_TS_DMG;
	}

	Config::DMG_TitleScreen_Mode cfg_ts = config->dmgTitleScreenMode(cfg_rom);
	assert(cfg_ts >= Config::DMG_TitleScreen_Mode::DMG_TS_DMG);
	assert(cfg_ts <  Config::DMG_TitleScreen_Mode::DMG_TS_MAX);
	if (cfg_ts <  Config::DMG_TitleScreen_Mode::DMG_TS_DMG ||
	    cfg_ts >= Config::DMG_TitleScreen_Mode::DMG_TS_MAX)
	{
		// Out of range. Use the default.
		cfg_ts = cfg_rom;
	}

	// Special case: If CGB-SGB, make sure the ROM also supports SGB.
	// Some CGB ROMs do have SGB borders, but since the header doesn't
	// unlock SGB mode, it doesn't show up on hardware. It *does* show
	// up on mGBA, though...
	if (cfg_ts == Config::DMG_TitleScreen_Mode::DMG_TS_SGB &&
	    !(dmg_system & DMGPrivate::DMG_SYSTEM_SGB))
	{
		cfg_ts = Config::DMG_TitleScreen_Mode::DMG_TS_DMG;
	}

	// Get the image subdirectory from the table.
	img_subdir = ts_subdirs[cfg_rom][cfg_ts];

	// Subdirectory:
	// - CGB/x/:   CGB game. (x == region byte, or NoID if no Game ID)
	// - CGB-DMG/: CGB game, taken in DMG mode.
	// - CGB-SGB/: CGB+SGB game, taken in SGB mode. (with border)
	// - SGB/:     SGB-enhanced game, taken in SGB mode. (with border)
	// - SGB-DMG/: SGB-enhanced game, taken in DMG mode.
	// - SGB-CGB/: SGB-enhanced game, taken in DMG mode, with CGB colorizations.
	// - DMG/:     DMG-only game.
	// - DMG-CGB/: DMG-only game, with CGB colorizations.
	string img_filename;	// Filename.

	if (s_gameID.empty()) {
		// No game ID.
		if (dmg_system & DMGPrivate::DMG_SYSTEM_CGB) {
			// CGB has subdirectories for the region byte.
			// This game doesn't have a game ID, so use "NoID".
			img_subdir += "/NoID";
		}

		// Image filename is based on the title, plus publisher
		// code, plus "-J" if region is set to Japanese (0).

		// Manually filter out characters that are rejected by CacheKeys.
		img_filename.reserve(s_title.size() + 8);
		for (const char p : s_title) {
			switch (p) {
				case '*':
				case '/':
				case ':':
				case '<':
				case '>':
				case '?':
				case '\\':
					img_filename += '_';
					break;
				default:
					img_filename += p;
					break;
			}
		}

		char pbcode[3];
		if (!DMGSpecialCases::get_publisher_code(pbcode, romHeader)) {
			img_filename += '-';
			img_filename += pbcode;
		}
		if (romHeader->region == 0) {
			img_filename += "-J";
		}

		if (DMGSpecialCases::is_rpdb_checksum_needed_TitleBased(romHeader)) {
			// Special case: Append the ROM checksum.
			append_cksum = true;
		}
	} else {
		// Game ID is present. Subdirectory is based on the region byte.
		img_subdir += '/';
		img_subdir += s_gameID[3];

		// Image filename is the Game ID.
		img_filename = s_gameID;

		if (DMGSpecialCases::is_rpdb_checksum_needed_ID6(s_gameID.c_str())) {
			// Special case: Append the ROM checksum.
			append_cksum = true;
		}
	}

	if (append_cksum) {
		// Append the ROM checksum.
		// NOTE: pandocs says "high byte first", but the actual ROMs
		// seem to use little-endian.
		char cksum[16];
		snprintf(cksum, sizeof(cksum), "-%04X", le16_to_cpu(romHeader->rom_checksum));
		img_filename += cksum;
	}

	// Check for invalid characters and replace them with '_'.
	for (char &c : img_filename) {
		switch (c) {
			case '/':
			case '*':
			case '?':
			case ':':
				c = '_';
			default:
				break;
		}
	}

	RP_UNUSED(size);
	vector<ImageSizeDef> sizeDefs = supportedImageSizes(imageType);
	assert(sizeDefs.size() == 1);
	if (sizeDefs.empty()) {
		// No image sizes.
		return -ENOENT;
	}

	// NOTE: RPDB's title screen database only has one size.
	// There's no need to check image sizes, but we need to
	// get the image size for the extURLs struct.

	// Determine the image type name.
	const char *imageTypeName;
	const char *ext;
	switch (imageType) {
		case IMG_EXT_TITLE_SCREEN:
			imageTypeName = "title";
			ext = ".png";
			break;
		default:
			// Unsupported image type.
			return -ENOENT;
	}

	// Add the URLs.
	pExtURLs->resize(1);
	auto extURL_iter = pExtURLs->begin();
	extURL_iter->url = d->getURL_RPDB("gb", imageTypeName, img_subdir.c_str(), img_filename.c_str(), ext);
	extURL_iter->cache_key = d->getCacheKey_RPDB("gb", imageTypeName, img_subdir.c_str(), img_filename.c_str(), ext);
	extURL_iter->width = sizeDefs[0].width;
	extURL_iter->height = sizeDefs[0].height;
	extURL_iter->high_res = (sizeDefs[0].index >= 2);

	// All URLs added.
	return 0;
}

}
