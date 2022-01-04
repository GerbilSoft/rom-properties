/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * CBMCart.cpp: Commodore ROM cartridge reader.                            *
 *                                                                         *
 * Copyright (c) 2016-2021 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "CBMCart.hpp"
#include "cbm_cart_structs.h"

// librpbase, librpfile
using namespace LibRpBase;
using LibRpFile::IRpFile;

// C++ STL classes.
using std::string;
using std::vector;

namespace LibRomData {

class CBMCartPrivate final : public RomDataPrivate
{
	public:
		CBMCartPrivate(CBMCart *q, IRpFile *file);

	private:
		typedef RomDataPrivate super;
		RP_DISABLE_COPY(CBMCartPrivate)

	public:
		/** RomDataInfo **/
		static const char *const exts[];
		static const char *const mimeTypes[];
		static const RomDataInfo romDataInfo;

	public:
		// C64 cartridge types
		// TODO: Move to a data file?
		static const char *const crt_types_c64[];

		// VIC-20 cartridge types
		// TODO: Move to a data file?
		static const char *const crt_types_vic20[];

	public:
		enum class RomType {
			Unknown	= -1,

			C64	= 0,
			C128	= 1,
			CBM2	= 2,
			VIC20	= 3,
			Plus4	= 4,

			Max
		};
		RomType romType;

		// ROM header.
		CBM_CRTHeader romHeader;
};

ROMDATA_IMPL(CBMCart)

/** CBMCartPrivate **/

/* RomDataInfo */
const char *const CBMCartPrivate::exts[] = {
	".crt",

	nullptr
};
const char *const CBMCartPrivate::mimeTypes[] = {
	// Unofficial MIME types.
	// TODO: Get these upstreamed on FreeDesktop.org.
	"application/x-c64-cartridge",
	"application/x-c128-cartridge",
	"application/x-cbm2-cartridge",
	"application/x-vic20-cartridge",
	"application/x-plus4-cartridge",

	nullptr
};
const RomDataInfo CBMCartPrivate::romDataInfo = {
	"CBMCart", exts, mimeTypes
};

/// Cartridge types are synchronized with VICE 3.6.

// C64 cartridge types
// TODO: Move to a data file?
const char *const CBMCartPrivate::crt_types_c64[] = {
	// 0
	"generic cartridge", "Action Replay", "KCS Power Cartridge",
	"Final Cartridge III", "Simons' BASIC", "Ocean type 1",
	"Expert Cartridge", "Fun Play, Power Play", "Super Games",
	"Atomic Power",

	// 10
	"Epyx Fastload", "Westermann Learning", "Rex Utility",
	"Final Cartridge I", "Magic Formel", "C64 Game System, System 3",
	"Warp Speed", "Dinamic", "Zaxxon / Super Zaxxon (Sega)",
	"Magic Desk, Domark, HES Australia",

	// 20
	"Super Snapshot V5", "Comal-80", "Structured BASIC",
	"Ross", "Dela EP64", "Dela EP7x8", "Dela EP256",
	"Rex EP256", "Mikro Assembler", "Final Cartridge Plus",

	// 30
	"Action Replay 4", "Stardos", "EasyFlash", "EasyFlash Xbank",
	"Capture", "Action Replay 3", "Retro Replay",
	"MMC64", "MMC Replay", "IDE64",

	// 40
	"Super Snapshot V4", "IEEE-488", "Game Killer", "Prophet64",
	"EXOS", "Freeze Frame", "Freeze Machine", "Snapshot64",
	"Super Explode V5.0" "Magic Voice",

	// 50
	"Action Replay 2", "MACH 5", "Diashow-Maker", "Pagefox",
	"Kingsoft", "Silverrock 128K Cartridge", "Formel 64",
	"RGCD", "RR-Net MK3", "EasyCalc",

	// 60
	"GMod2", "MAX Basic", "GMod3", "ZIPP-CODE 48",
	"Blackbox V8", "Blackbox V3", "Blackbox V4",
	"REX RAM-Floppy", "BIS-Plus", "SD-BOX",

	// 70
	"MultiMAX", "Blackbox V9", "Lt. Kernal Host Adaptor",
	"RAMLink", "H.E.R.O.", "IEEE Flash! 64",
	"Turtle Graphics II", "Freeze Frame MK2",
};

// VIC-20 cartridge types
// TODO: Move to a data file?
const char *const CBMCartPrivate::crt_types_vic20[] = {
	"generic cartridge",
	"Mega-Cart",
	"Behr Bonz",
	"Vic Flash Plugin",
	"UltiMem",
	"Final Expansion",
};

CBMCartPrivate::CBMCartPrivate(CBMCart *q, IRpFile *file)
	: super(q, file, &romDataInfo)
	, romType(RomType::Unknown)
{
	// Clear the ROM header struct.
	memset(&romHeader, 0, sizeof(romHeader));
}

/** CBMCart **/

/**
 * Read a Commodore ROM cartridge image.
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
CBMCart::CBMCart(IRpFile *file)
	: super(new CBMCartPrivate(this, file))
{
	RP_D(CBMCart);

	if (!d->file) {
		// Could not ref() the file handle.
		return;
	}

	// Read the ROM header.
	d->file->rewind();
	size_t size = d->file->read(&d->romHeader, sizeof(d->romHeader));
	if (size != sizeof(d->romHeader)) {
		// Seek and/or read error.
		UNREF_AND_NULL_NOCHK(d->file);
		return;
	}

	// Check if this ROM image is supported.
	DetectInfo info;
	info.header.addr = 0;
	info.header.size = sizeof(d->romHeader);
	info.header.pData = reinterpret_cast<const uint8_t*>(&d->romHeader);
	info.ext = nullptr;	// Not needed for CBMCart.
	info.szFile = 0;	// Not needed for CBMCart.
	d->romType = static_cast<CBMCartPrivate::RomType>(isRomSupported_static(&info));
	d->isValid = (d->romType > CBMCartPrivate::RomType::Unknown);

	if (!d->isValid) {
		d->file->unref();
		d->file = nullptr;
	}
}

/**
 * Is a ROM image supported by this class?
 * @param info DetectInfo containing ROM detection information.
 * @return Class-specific system ID (>= 0) if supported; -1 if not.
 */
int CBMCart::isRomSupported_static(const DetectInfo *info)
{
	assert(info != nullptr);
	assert(info->header.pData != nullptr);
	if (!info || !info->header.pData ||
	    info->header.addr != 0 ||
	    info->header.size < sizeof(CBM_CRTHeader))
	{
		// Either no detection information was specified,
		// or the header is too small.
		return static_cast<int>(CBMCartPrivate::RomType::Unknown);
	}

	const CBM_CRTHeader *const romHeader =
		reinterpret_cast<const CBM_CRTHeader*>(info->header.pData);

	// Check the magic string.
	CBMCartPrivate::RomType romType = CBMCartPrivate::RomType::Unknown;
	if (!memcmp(romHeader->magic, CBM_C64_CRT_MAGIC, sizeof(romHeader->magic))) {
		romType = CBMCartPrivate::RomType::C64;
	} else if (!memcmp(romHeader->magic, CBM_C128_CRT_MAGIC, sizeof(romHeader->magic))) {
		romType = CBMCartPrivate::RomType::C128;
	} else if (!memcmp(romHeader->magic, CBM_CBM2_CRT_MAGIC, sizeof(romHeader->magic))) {
		romType = CBMCartPrivate::RomType::CBM2;
	} else if (!memcmp(romHeader->magic, CBM_VIC20_CRT_MAGIC, sizeof(romHeader->magic))) {
		romType = CBMCartPrivate::RomType::VIC20;
	} else if (!memcmp(romHeader->magic, CBM_PLUS4_CRT_MAGIC, sizeof(romHeader->magic))) {
		romType = CBMCartPrivate::RomType::Plus4;
	} else {
		// Not supported.
		return static_cast<int>(CBMCartPrivate::RomType::Unknown);
	}

	// CRT version number.
	const uint16_t version = be16_to_cpu(romHeader->version);

	/// Verify that certain features are not present in older versions.

	// Subtype requires v1.1.
	if (romHeader->subtype != 0) {
		if (version < 0x0101) {
			return static_cast<int>(CBMCartPrivate::RomType::Unknown);
		}
	}

	// Systems other than C64 require v2.0.
	if (romType > CBMCartPrivate::RomType::C64) {
		if (version < 0x0200) {
			return static_cast<int>(CBMCartPrivate::RomType::Unknown);
		}
	}

	// We're done here.
	return static_cast<int>(romType);
}

/**
 * Get the name of the system the loaded ROM is designed for.
 * @param type System name type. (See the SystemName enum.)
 * @return System name, or nullptr if type is invalid.
 */
const char *CBMCart::systemName(unsigned int type) const
{
	RP_D(const CBMCart);
	if (!d->isValid || !isSystemNameTypeValid(type))
		return nullptr;

	static_assert(SYSNAME_TYPE_MASK == 3,
		"CBMCart::systemName() array index optimization needs to be updated.");

	static const char *const sysNames[][4] = {
		{"Commodore 64", "C64", "C64", nullptr},
		{"Commodore 128", "C128", "C128", nullptr},
		{"Commodore CBM-II", "CBM-II", "CBM-II", nullptr},
		{"Commodore VIC-20", "VIC-20", "VIC-20", nullptr},
		{"Commodore Plus/4", "Plus/4", "Plus/4", nullptr},
	};
	static_assert(ARRAY_SIZE(sysNames) == static_cast<int>(CBMCartPrivate::RomType::Max),
		"CBMCart: sysNames[] is missing entries!");

	int i = static_cast<int>(d->romType);
	if (i < 0 || i >= ARRAY_SIZE_I(sysNames)) {
		// Invalid system ID. Default to C64.
		i = 0;
	}
	return sysNames[i][type & SYSNAME_TYPE_MASK];
}

/**
 * Load field data.
 * Called by RomData::fields() if the field data hasn't been loaded yet.
 * @return Number of fields read on success; negative POSIX error code on error.
 */
int CBMCart::loadFieldData(void)
{
	RP_D(CBMCart);
	if (!d->fields->empty()) {
		// Field data *has* been loaded...
		return 0;
	} else if (!d->file) {
		// File isn't open.
		return -EBADF;
	} else if (!d->isValid) {
		// Unknown ROM image type.
		return -EIO;
	}

	const CBM_CRTHeader *const romHeader = &d->romHeader;
	d->fields->reserve(1);	// Maximum of 1 field.

	// Title
	if (romHeader->title[0] != '\0') {
		d->fields->addField_string(C_("RomData", "Title"),
			cp1252_to_utf8(romHeader->title, sizeof(romHeader->title)),
			RomFields::STRF_TRIM_END);
	}

	// Cartridge type
	const uint64_t type = be16_to_cpu(romHeader->type);
	bool b_noType = false;
	const char *s_type = nullptr;
	switch (d->romType) {
		case CBMCartPrivate::RomType::C64:
			switch (type) {
				case 0: {
					// Generic cartridge.
					// Identify the type based on the EXROM/GAME lines.
					const uint8_t id = (romHeader->c64_game  != 0) |
					                  ((romHeader->c64_exrom != 0) << 1);
					static const char *const crt_types_c64_generic[] = {
						"16 KB game", "8 KB game",
						"UltiMax mode", "RAM/disabled,"
					};
					s_type = crt_types_c64_generic[id & 3];
					break;
				}
				case 36:
					s_type = (romHeader->subtype == 1 ? "Nordic Replay" : "Retro Replay");
					break;
				case 57:
					s_type = (romHeader->subtype == 1 ? "Hucky" : "RGCD");
					break;
				default:
					if (type < ARRAY_SIZE(CBMCartPrivate::crt_types_c64)) {
						s_type = CBMCartPrivate::crt_types_c64[type];
					}
					break;
			}
			break;

		case CBMCartPrivate::RomType::C128:
			switch (type) {
				case 0:
					s_type = "generic cartridge";
					break;
				case 1:
					switch (romHeader->subtype) {
						default:
						case 0:
							s_type = "Warpspeed128";
							break;
						case 1:
							s_type = "Warpspeed128, REU support";
							break;
						case 2:
							s_type = "Warpspeed128, REU support, with I/O and ROM banking";
							break;
					}
					break;
				default:
					break;
			}
			break;

		case CBMCartPrivate::RomType::VIC20:
			if (type < ARRAY_SIZE(CBMCartPrivate::crt_types_vic20)) {
				s_type = CBMCartPrivate::crt_types_vic20[type];
			}
			break;

		default:
			// Type is not supported for this platform.
			b_noType = true;
			break;
	}

	if (!b_noType) {
		const char *const s_type_title = C_("RomData", "Type");
		if (s_type != nullptr) {
			d->fields->addField_string(s_type_title, s_type);
		} else {
			d->fields->addField_string(s_type_title,
				rp_sprintf(C_("RomData", "Unknown (%u)"), type));
		}
	}

	// Finished reading the field data.
	return static_cast<int>(d->fields->count());
}

/**
 * Load metadata properties.
 * Called by RomData::metaData() if the field data hasn't been loaded yet.
 * @return Number of metadata properties read on success; negative POSIX error code on error.
 */
int CBMCart::loadMetaData(void)
{
	RP_D(CBMCart);
	if (d->metaData != nullptr) {
		// Metadata *has* been loaded...
		return 0;
	} else if (!d->file) {
		// File isn't open.
		return -EBADF;
	} else if (!d->isValid) {
		// Unknown ROM image type.
		return -EIO;
	}

	// Create the metadata object.
	d->metaData = new RomMetaData();
	d->metaData->reserve(1);	// Maximum of 1 metadata property.

	const CBM_CRTHeader *const romHeader = &d->romHeader;

	// Title
	if (romHeader->title[0] != '\0') {
		d->metaData->addMetaData_string(Property::Title,
			cp1252_to_utf8(romHeader->title, sizeof(romHeader->title)),
			RomMetaData::STRF_TRIM_END);
	}

	// Finished reading the metadata.
	return (d->metaData ? static_cast<int>(d->metaData->count()) : -ENOENT);
}

}
