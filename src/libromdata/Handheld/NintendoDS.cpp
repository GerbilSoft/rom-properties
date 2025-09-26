/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * NintendoDS.hpp: Nintendo DS(i) ROM reader.                              *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "librpbase/config.librpbase.h"

#include "NintendoDS.hpp"
#include "NintendoDS_p.hpp"
#include "data/NintendoPublishers.hpp"
#include "data/NintendoLanguage.hpp"
#include "../Console/WiiCommon.hpp"

// Other rom-properties libraries
#include "librpbase/config/Config.hpp"
#include "librpbase/SystemRegion.hpp"
using namespace LibRpBase;
using namespace LibRpFile;
using namespace LibRpText;
using namespace LibRpTexture;

// C++ STL classes
using std::array;
using std::shared_ptr;
using std::string;
using std::vector;

namespace LibRomData {

ROMDATA_IMPL(NintendoDS)
ROMDATA_IMPL_IMG(NintendoDS)

/** NintendoDSPrivate **/

/* RomDataInfo */
// NOTE: Using the same image settings as Nintendo3DS.
const array<const char*, 4+1> NintendoDSPrivate::exts = {{
	".nds",	// Nintendo DS
	".dsi",	// Nintendo DSi (devkitARM r46)
	".ids",	// iQue DS (no-intro)
	".srl",	// Official SDK extension

	nullptr
}};
const array<const char*, 3+1> NintendoDSPrivate::mimeTypes = {{
	// Unofficial MIME types from FreeDesktop.org.
	"application/x-nintendo-ds-rom",

	// Vendor-specific type listed in Fedora's mime.types.
	"application/vnd.nintendo.nitro.rom",

	// Unofficial MIME types.
	// TODO: Get these upstreamed on FreeDesktop.org.
	"application/x-nintendo-dsi-rom",

	nullptr
}};
const RomDataInfo NintendoDSPrivate::romDataInfo = {
	"NintendoDS", exts.data(), mimeTypes.data()
};

NintendoDSPrivate::NintendoDSPrivate(const IRpFilePtr &file, bool cia)
	: super(file, &romDataInfo)
	, romType(RomType::Unknown)
	, romSize(0)
	, secData(0)
	, secArea(NDS_SECAREA_UNKNOWN)
	, cia(cia)
	, fieldIdx_secData(-1)
	, fieldIdx_secArea(-1)
{
	// Clear the various structs.
	memset(&romHeader, 0, sizeof(romHeader));
}

/**
 * Get the game ID, with unprintable characters replaced with '_'.
 * @return Game ID
 */
inline string NintendoDSPrivate::getGameID(void) const
{
	// Replace any non-printable characters with underscores.
	string id6;
	id6.resize(6, '_');
	for (size_t i = 0; i < 6; i++) {
		if (ISPRINT(romHeader.id6[i])) {
			id6[i] = romHeader.id6[i];
		}
	}
	return id6;
}

/**
 * Get the title ID. (DSi only)
 * @return Title ID, or empty string on error.
 */
std::string NintendoDSPrivate::dsi_getTitleID(void) const
{
	assert(romHeader.unitcode & 0x02);
	if (!(romHeader.unitcode & 0x02)) {
		return {};
	}

	return fmt::format(FSTR("{:0>8X}-{:0>8X}"),
		le32_to_cpu(romHeader.dsi.title_id.hi),
		le32_to_cpu(romHeader.dsi.title_id.lo));
}

/**
 * Load the icon/title data.
 * @return 0 on success; negative POSIX error code on error.
 */
int NintendoDSPrivate::loadIconTitleData(void)
{
	assert(this->file != nullptr);

	if ((bool)nds_icon_title) {
		// Icon/title data is already loaded.
		return 0;
	}

	// Get the address of the icon/title information.
	const uint32_t icon_offset = le32_to_cpu(romHeader.icon_offset);
	// Icon must be located after the "secure area".
	if (icon_offset <= 0x8000) {
		// No icon/title information.
		return -ENOENT;
	}

	// Create a DiscReader for the icon/title.
	IDiscReaderPtr discReader = std::make_shared<DiscReader>(this->file, icon_offset, sizeof(NDS_IconTitleData));
	if (!discReader->isOpen()) {
		// Failed to open the DiscReader.
		return -discReader->lastError();
	}
	// Read the icon/title data.
	NintendoDS_BNR *const bnrFile = new NintendoDS_BNR(discReader);
	if (!bnrFile->isValid()) {
		// Failed to open the NintendoDS_BNR.
		delete bnrFile;
		return -EIO;
	}

	// Save the banner file.
	this->nds_icon_title.reset(bnrFile);
	return 0;
}

/**
 * Convert a Nintendo DS(i) region value to a GameTDB language code.
 * @param ndsRegion Nintendo DS region.
 * @param dsiRegion Nintendo DSi region.
 * @param idRegion Game ID region.
 *
 * NOTE: Mulitple GameTDB language codes may be returned, including:
 * - User-specified fallback language code for PAL.
 * - General fallback language code.
 *
 * @return GameTDB language code(s), or empty vector if the region value is invalid.
 * NOTE: The language code may need to be converted to uppercase!
 */
vector<uint16_t> NintendoDSPrivate::ndsRegionToGameTDB(
	uint8_t ndsRegion, uint32_t dsiRegion, char idRegion)
{
	/**
	 * There are up to three region codes for Nintendo DS games:
	 * - Game ID
	 * - NDS region (China/Korea only)
	 * - DSi region (DSi-enhanced/exclusive games only)
	 *
	 * Nintendo DS does not have region lock outside of
	 * China. (The Korea value isn't actually used.)
	 *
	 * Nintendo DSi does have region lock, but only for
	 * DSi-enhanced/exclusive games.
	 *
	 * If a DSi-enhanced/exclusive game has a single region
	 * code value set, that region will be displayed.
	 *
	 * If a DS-only game has China or Korea set, that region
	 * will be displayed.
	 *
	 * The game ID will always be used as a fallback.
	 *
	 * Game ID reference:
	 * - https://github.com/dolphin-emu/dolphin/blob/4c9c4568460df91a38d40ac3071d7646230a8d0f/Source/Core/DiscIO/Enums.cpp
	 */
	vector<uint16_t> ret;
	ret.reserve(3);

	int fallback_region = 0;
	switch (dsiRegion) {
		case DSi_REGION_JAPAN:
			ret.push_back('JA');
			return ret;
		case DSi_REGION_USA:
			ret.push_back('US');
			return ret;
		case DSi_REGION_EUROPE:
		case DSi_REGION_EUROPE | DSi_REGION_AUSTRALIA:
			// Process the game ID and use 'EN' as a fallback.
			fallback_region = 1;
			break;
		case DSi_REGION_AUSTRALIA:
			// Process the game ID and use 'AU','EN' as fallbacks.
			fallback_region = 2;
			break;
		case DSi_REGION_CHINA:
			// NOTE: GameTDB only has 'ZH' for boxart, not 'ZHCN' or 'ZHTW'.
			ret.push_back('ZH');
			ret.push_back('JA');
			ret.push_back('EN');
			return ret;
		case DSi_REGION_SKOREA:
			ret.push_back('KO');
			ret.push_back('JA');
			ret.push_back('EN');
			return ret;
		case 0:
		default:
			// No DSi region, or unsupported DSi region.
			break;
	}

	// TODO: If multiple DSi region bits are set,
	// compare each to the host system region.

	// Check for China/Korea.
	if (ndsRegion & NDS_REGION_CHINA) {
		// NOTE: GameTDB only has 'ZH' for boxart, not 'ZHCN' or 'ZHTW'.
		ret.push_back('ZH');
		ret.push_back('JA');
		ret.push_back('EN');
		return ret;
	} else if (ndsRegion & NDS_REGION_SKOREA) {
		ret.push_back('KO');
		ret.push_back('JA');
		ret.push_back('EN');
		return ret;
	}

	// Check for region-specific game IDs.
	switch (idRegion) {
		case 'E':	// USA
			ret.push_back('US');
			break;
		case 'J':	// Japan
			ret.push_back('JA');
			break;
		case 'O':
			// TODO: US/EU.
			// Compare to host system region.
			// For now, assuming US.
			ret.push_back('US');
			break;
		case 'P':	// PAL
		case 'X':	// Multi-language release
		case 'Y':	// Multi-language release
		case 'L':	// Japanese import to PAL regions
		case 'M':	// Japanese import to PAL regions
		default: {
			// Generic PAL release.
			// Use the user-specified fallback.
			const Config *const config = Config::instance();
			const uint32_t lc = config->palLanguageForGameTDB();
			if (lc != 0 && lc < 65536) {
				ret.push_back(static_cast<uint16_t>(lc));
				// Don't add English again if that's what the
				// user-specified fallback language is.
				if (lc != 'en' && lc != 'EN') {
					fallback_region = 1;
				}
			} else {
				// Invalid. Use 'EN'.
				fallback_region = 1;
			}
			break;
		}

		// European regions.
		case 'D':	// Germany
			ret.push_back('DE');
			fallback_region = 1;
			break;
		case 'F':	// France
			ret.push_back('FR');
			fallback_region = 1;
			break;
		case 'H':	// Netherlands
			ret.push_back('NL');
			fallback_region = 1;
			break;
		case 'I':	// Italy
			ret.push_back('IT');
			fallback_region = 1;
			break;
		case 'R':	// Russia
			ret.push_back('RU');
			fallback_region = 1;
			break;
		case 'S':	// Spain
			ret.push_back('ES');
			fallback_region = 1;
			break;
		case 'U':	// Australia
			if (fallback_region == 0) {
				// Use the fallback region.
				fallback_region = 2;
			}
			break;
	}

	// Check for fallbacks.
	switch (fallback_region) {
		case 1:
			// Europe
			ret.push_back('EN');
			break;
		case 2:
			// Australia
			ret.push_back('AU');
			ret.push_back('EN');
			break;

		case 0:	// None
		default:
			break;
	}

	return ret;
}

/**
 * Get the DSi flags string vector.
 * @return DSi flags string vector.
 */
RomFields::ListData_t *NintendoDSPrivate::getDSiFlagsStringVector(void)
{
	static const array<const char*, 8> dsi_flags_bitfield_names = {{
		// tr: Uses the DSi-specific touchscreen protocol.
		NOP_C_("NintendoDS|DSi_Flags", "DSi Touchscreen"),
		// tr: Game requires agreeing to the Nintendo online services agreement.
		NOP_C_("NintendoDS|DSi_Flags", "Require EULA"),
		// tr: Custom icon is used from the save file.
		NOP_C_("NintendoDS|DSi_Flags", "Custom Icon"),
		// tr: Game supports Nintendo Wi-Fi Connection.
		NOP_C_("NintendoDS|DSi_Flags", "Nintendo WFC"),
		NOP_C_("NintendoDS|DSi_Flags", "DS Wireless"),
		NOP_C_("NintendoDS|DSi_Flags", "NDS Icon SHA-1"),
		NOP_C_("NintendoDS|DSi_Flags", "NDS Header RSA"),
		NOP_C_("NintendoDS|DSi_Flags", "Developer"),
	}};

	// Convert to RomFields::ListData_t for RFT_LISTDATA.
	auto *const vv_dsi_flags = new RomFields::ListData_t(dsi_flags_bitfield_names.size());
	for (int i = static_cast<int>(dsi_flags_bitfield_names.size()) - 1; i >= 0; i--) {
		auto &data_row = vv_dsi_flags->at(i);
		data_row.emplace_back(
			pgettext_expr("NintendoDS|DSi_Flags",
				dsi_flags_bitfield_names[i]));
	}

	return vv_dsi_flags;
}

/** NintendoDS **/

/**
 * Read a Nintendo DS ROM image.
 *
 * A ROM image must be opened by the caller. The file handle
 * will be ref()'d and must be kept open in order to load
 * data from the ROM image.
 *
 * To close the file, either delete this object or call close().
 *
 * NOTE: Check isValid() to determine if this is a valid ROM.
 *
 * @param file Open ROM image
 * @param cia If true, hide fields that aren't relevant to DSiWare in 3DS CIA packages.
 */
NintendoDS::NintendoDS(const IRpFilePtr &file, bool cia)
	: super(new NintendoDSPrivate(file, cia))
{
	RP_D(NintendoDS);
	if (!d->file) {
		// Could not ref() the file handle.
		return;
	}

	// Read the ROM header.
	d->file->rewind();
	size_t size = d->file->read(&d->romHeader, sizeof(d->romHeader));
	if (size != sizeof(d->romHeader)) {
		d->file.reset();
		return;
	}

	// Get the ROM size for later.
	d->romSize = d->file->size();

	// Check if this ROM image is supported.
	const DetectInfo info = {
		{0, sizeof(d->romHeader), reinterpret_cast<const uint8_t*>(&d->romHeader)},
		nullptr,	// ext (not needed for Nintendo3DS_SMDH)
		d->romSize	// szFile
	};
	d->romType = static_cast<NintendoDSPrivate::RomType>(isRomSupported_static(&info));
	d->isValid = (static_cast<int>(d->romType) >= 0);

	if (!d->isValid) {
		d->file.reset();
		return;
	}

	// Is PAL?
	d->isPAL = (d->romHeader.id4[3] == 'P');

	// Check the secure area status.
	d->secData = d->checkNDSSecurityData();
	d->secArea = d->checkNDSSecureArea();

	// Set the MIME type. (unofficial)
	d->mimeType = (d->romType == NintendoDSPrivate::RomType::DSi_Exclusive)
			? "application/x-nintendo-dsi-rom"	// (not on fd.o)
			: "application/x-nintendo-ds-rom";
}

/**
 * Is a ROM image supported by this class?
 * @param info DetectInfo containing ROM detection information.
 * @return Class-specific system ID (>= 0) if supported; -1 if not.
 */
int NintendoDS::isRomSupported_static(const DetectInfo *info)
{
	assert(info != nullptr);
	assert(info->header.pData != nullptr);
	assert(info->header.addr == 0);
	if (!info || !info->header.pData ||
	    info->header.addr != 0 ||
	    info->header.size < sizeof(NDS_RomHeader))
	{
		// Either no detection information was specified,
		// or the header is too small.
		return static_cast<int>(NintendoDSPrivate::RomType::Unknown);
	}

	// Check the first 16 bytes of the Nintendo logo.
	static constexpr array<uint8_t, 16> nintendo_gba_logo = {{
		0x24, 0xFF, 0xAE, 0x51, 0x69, 0x9A, 0xA2, 0x21,
		0x3D, 0x84, 0x82, 0x0A, 0x84, 0xE4, 0x09, 0xAD
	}};
	static constexpr array<uint8_t, 16> nintendo_ds_logo_slot2 = {{
		0xC8, 0x60, 0x4F, 0xE2, 0x01, 0x70, 0x8F, 0xE2,
		0x17, 0xFF, 0x2F, 0xE1, 0x12, 0x4F, 0x11, 0x48,
	}};

	const NDS_RomHeader *const romHeader =
		reinterpret_cast<const NDS_RomHeader*>(info->header.pData);
	if (!memcmp(romHeader->nintendo_logo, nintendo_gba_logo.data(), nintendo_gba_logo.size()) &&
	    romHeader->nintendo_logo_checksum == cpu_to_le16(0xCF56)) {
		// Nintendo logo is valid. (Slot-1)
		static constexpr array<int8_t, 4> nds_romType = {{
			static_cast<int8_t>(NintendoDSPrivate::RomType::NDS),		// 0x00 == Nintendo DS
			static_cast<int8_t>(NintendoDSPrivate::RomType::NDS),		// 0x01 == invalid (assuming DS)
			static_cast<int8_t>(NintendoDSPrivate::RomType::DSi_Enhanced),	// 0x02 == DSi-enhanced
			static_cast<int8_t>(NintendoDSPrivate::RomType::DSi_Exclusive),	// 0x03 == DSi-only
		}};
		return nds_romType[romHeader->unitcode & 3];
	} else if (!memcmp(romHeader->nintendo_logo, nintendo_ds_logo_slot2.data(), nintendo_ds_logo_slot2.size()) &&
		   romHeader->nintendo_logo_checksum == cpu_to_le16(0x9E1A)) {
		// Nintendo logo is valid. (Slot-2)
		// NOTE: Slot-2 is NDS only.
		return static_cast<int>(NintendoDSPrivate::RomType::NDS_Slot2);
	}

	// Not supported.
	return static_cast<int>(NintendoDSPrivate::RomType::Unknown);
}

/**
 * Get the name of the system the loaded ROM is designed for.
 * @param type System name type. (See the SystemName enum.)
 * @return System name, or nullptr if type is invalid.
 */
const char *NintendoDS::systemName(unsigned int type) const
{
	RP_D(const NintendoDS);
	if (!d->isValid || !isSystemNameTypeValid(type))
		return nullptr;

	// NDS/DSi are mostly the same worldwide, except for China.
	static_assert(SYSNAME_TYPE_MASK == 3,
		"NintendoDS::systemName() array index optimization needs to be updated.");
	static_assert(SYSNAME_REGION_MASK == (1U << 2),
		"NintendoDS::systemName() array index optimization needs to be updated.");

	// Bits 0-1: Type. (long, short, abbreviation)
	// Bit 2: 0 for NDS, 1 for DSi-exclusive.
	// Bit 3: 0 for worldwide, 1 for China. (iQue DS)
	static const array<const char*, 4*4> sysNames = {{
		// Nintendo (worldwide)
		"Nintendo DS", "Nintendo DS", "NDS", nullptr,
		"Nintendo DSi", "Nintendo DSi", "DSi", nullptr,

		// iQue (China)
		"iQue DS", "iQue DS", "NDS", nullptr,
		"iQue DSi", "iQue DSi", "DSi", nullptr
	}};

	// "iQue" is only used if the localized system name is requested
	// *and* the ROM's region code is China only.
	unsigned int idx = (type & SYSNAME_TYPE_MASK);
	if (d->romType == NintendoDSPrivate::RomType::DSi_Exclusive) {
		// DSi-exclusive game.
		idx |= (1U << 2);
		if ((type & SYSNAME_REGION_MASK) == SYSNAME_REGION_ROM_LOCAL) {
			if ((d->romHeader.dsi.region_code == cpu_to_le32(DSi_REGION_CHINA)) ||
			    (d->romHeader.nds_region & 0x80))
			{
				// iQue DSi.
				idx |= (1U << 3);
			}
		}
	} else {
		// NDS-only and/or DSi-enhanced game.
		if ((type & SYSNAME_REGION_MASK) == SYSNAME_REGION_ROM_LOCAL) {
			if (d->romHeader.nds_region & 0x80) {
				// iQue DS.
				idx |= (1U << 3);
			}
		}
	}

	return sysNames[idx];
}

/**
 * Get a bitfield of image types this class can retrieve.
 * @return Bitfield of supported image types. (ImageTypesBF)
 */
uint32_t NintendoDS::supportedImageTypes_static(void)
{
#ifdef HAVE_JPEG
	return IMGBF_INT_ICON | IMGBF_EXT_BOX |
	       IMGBF_EXT_COVER | IMGBF_EXT_COVER_FULL;
#else /* !HAVE_JPEG */
	return IMGBF_INT_ICON | IMGBF_EXT_BOX;
#endif /* HAVE_JPEG */
}

/**
 * Get a list of all available image sizes for the specified image type.
 * @param imageType Image type.
 * @return Vector of available image sizes, or empty vector if no images are available.
 */
vector<RomData::ImageSizeDef> NintendoDS::supportedImageSizes_static(ImageType imageType)
{
	ASSERT_supportedImageSizes(imageType);

	switch (imageType) {
		case IMG_INT_ICON:
			return {{nullptr, 32, 32, 0}};
#ifdef HAVE_JPEG
		case IMG_EXT_COVER:
			return {
				{nullptr, 160, 144, 0},
				//{"S", 128, 115, 1},	// DISABLED; not needed.
				{"M", 400, 352, 2},
				{"HQ", 768, 680, 3},
			};
		case IMG_EXT_COVER_FULL:
			return {
				{nullptr, 340, 144, 0},
				//{"S", 272, 115, 1},	// Not currently present on GameTDB.
				{"M", 856, 352, 2},
				{"HQ", 1616, 680, 3},
			};
#endif /* HAVE_JPEG */
		case IMG_EXT_BOX:
			return {{nullptr, 240, 216, 0}};
		default:
			break;
	}

	// Unsupported image type.
	return {};
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
uint32_t NintendoDS::imgpf(ImageType imageType) const
{
	ASSERT_imgpf(imageType);

	RP_D(const NintendoDS);
	uint32_t ret = 0;
	switch (imageType) {
		case IMG_INT_ICON:
			// Wrapper function around NintendoDS_BNR.
			if (!d->nds_icon_title) {
				const_cast<NintendoDSPrivate*>(d)->loadIconTitleData();
			}
			if (d->nds_icon_title) {
				ret = d->nds_icon_title->imgpf(imageType);
			}
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
int NintendoDS::loadFieldData(void)
{
	RP_D(NintendoDS);
	if (!d->fields.empty()) {
		// Field data *has* been loaded...
		return 0;
	} else if (!d->file) {
		// File isn't open.
		return -EBADF;
	} else if (!d->isValid || static_cast<int>(d->romType) < 0) {
		// ROM image isn't valid.
		return -EIO;
	}

#ifdef _WIN32
	// Windows: 6 visible rows per RFT_LISTDATA.
	static constexpr int rows_visible = 6;
#else /* !_WIN32 */
	// Linux: 4 visible rows per RFT_LISTDATA.
	static constexpr int rows_visible = 4;
#endif /* _WIN32 */

	// ROM header is read in the constructor.
	const NDS_RomHeader *const romHeader = &d->romHeader;
	if (d->isDSi()) {
		// DSi-enhanced or DSi-exclusive.
		d->fields.reserve(10+8);
	} else {
		// NDS only.
		d->fields.reserve(10);
	}

	// NDS common fields
	d->fields.setTabName(0, "NDS");

	// Type
	// TODO:
	// - Show PassMe fields?
	//   Reference: http://imrannazar.com/The-Smallest-NDS-File
	// - Show IR cart and/or other accessories? (NAND ROM, etc.)
	const char *nds_romType;
	const uint16_t dsi_filetype = le16_to_cpu(romHeader->dsi.title_id.catID);
	if (d->cia || ((romHeader->unitcode & NintendoDSPrivate::DS_HW_DSi) &&
		dsi_filetype != DSi_FTYPE_CARTRIDGE))
	{
		// DSiWare.
		// TODO: Verify games that are available as both
		// cartridge and DSiWare.
		if (dsi_filetype == DSi_FTYPE_DSiWARE) {
			nds_romType = "DSiWare";
		} else {
			nds_romType = "DSi System Software";
		}
	} else {
		// TODO: Identify NDS Download Play titles.
		switch (d->romType) {
			case NintendoDSPrivate::RomType::NDS_Slot2:
				nds_romType = "Slot-2 (PassMe)";
				break;
			default:
				nds_romType = "Slot-1";
				break;
		}
	}
	d->fields.addField_string(C_("RomData", "Type"), nds_romType);

	// Title
	d->fields.addField_string(C_("RomData", "Title"),
		latin1_to_utf8(romHeader->title, ARRAY_SIZE_I(romHeader->title)),
		RomFields::STRF_TRIM_END);

	if (!d->nds_icon_title) {
		const_cast<NintendoDSPrivate*>(d)->loadIconTitleData();
	}
	if (d->nds_icon_title) {
		// Full title
		const RomFields *const other = d->nds_icon_title->fields();
		assert(other != nullptr);
		if (other) {
			// TODO: Verify that this has Full Title?
			d->fields.addFields_romFields(other, 0);
		}
	}

	// Game ID
	d->fields.addField_string(C_("RomData", "Game ID"), d->getGameID());

	// Publisher
	const char *const publisher_title = C_("RomData", "Publisher");
	const char *const publisher = NintendoPublishers::lookup(romHeader->company);
	if (publisher) {
		d->fields.addField_string(publisher_title, publisher);
	} else {
		if (isalnum_ascii(romHeader->company[0]) && isalnum_ascii(romHeader->company[1])) {
			const array<char, 3> s_company = {{
				romHeader->company[0],
				romHeader->company[1],
				'\0'
			}};
			d->fields.addField_string(publisher_title,
				fmt::format(FRUN(C_("RomData", "Unknown ({:s})")), s_company.data()));
		} else {
			d->fields.addField_string(publisher_title,
				fmt::format(FRUN(C_("RomData", "Unknown ({:0>2X} {:0>2X})")),
					static_cast<unsigned int>(romHeader->company[0]),
					static_cast<unsigned int>(romHeader->company[1])));
		}
	}

	// ROM version
	d->fields.addField_string_numeric(C_("RomData", "Revision"),
		romHeader->rom_version, RomFields::Base::Dec, 2);

	// Is the security data present?
	static const array<const char*, 3> nds_security_data_names = {{
		NOP_C_("NintendoDS|SecurityData", "Blowfish Tables"),
		NOP_C_("NintendoDS|SecurityData", "Static Data"),
		NOP_C_("NintendoDS|SecurityData", "Random Data"),
	}};
	vector<string> *const v_nds_security_data_names = RomFields::strArrayToVector_i18n(
		"NintendoDS|SecurityData", nds_security_data_names);
	d->fields.addField_bitfield(C_("NintendoDS", "Security Data"),
		v_nds_security_data_names, 0, d->secData);
	d->fieldIdx_secData = static_cast<int>(d->fields.count()-1);

	// Secure Area
	// TODO: Verify the CRC.
	d->fields.addField_string(C_("NintendoDS", "Secure Area"),
		d->getNDSSecureAreaString());
	d->fieldIdx_secArea = static_cast<int>(d->fields.count()-1);

	// Hardware type
	// NOTE: DS_HW_DS is inverted bit0; DS_HW_DSi is normal bit1.
	uint32_t hw_type = (romHeader->unitcode & 3) ^ NintendoDSPrivate::DS_HW_DS;
	if (hw_type == 0) {
		// 0x01 is invalid. Assume DS.
		hw_type = NintendoDSPrivate::DS_HW_DS;
	}

	static const array<const char*, 2> hw_bitfield_names = {{
		"Nintendo DS", "Nintendo DSi"
	}};
	vector<string> *const v_hw_bitfield_names = RomFields::strArrayToVector(hw_bitfield_names);
	d->fields.addField_bitfield(C_("NintendoDS", "Hardware"),
		v_hw_bitfield_names, 0, hw_type);

	// NDS Region
	// Only used for region locking on Chinese iQue DS consoles.
	// Not displayed for DSiWare wrapped in 3DS CIA packages.
	uint32_t nds_region = 0;
	if (romHeader->nds_region & NDS_REGION_CHINA) {
		nds_region |= NintendoDSPrivate::NDS_REGION_CHINA;
	}
	if (romHeader->nds_region & NDS_REGION_SKOREA) {
		nds_region |= NintendoDSPrivate::NDS_REGION_SKOREA;
	}
	if (nds_region == 0) {
		// No known region flags.
		// Note that the Sonic Colors demo has 0x02 here.
		nds_region = NintendoDSPrivate::NDS_REGION_FREE;
	}

	static const array<const char*, 3> nds_region_bitfield_names = {{
		NOP_C_("Region", "Region-Free"),
		NOP_C_("Region", "South Korea"),
		NOP_C_("Region", "China"),
	}};
	vector<string> *const v_nds_region_bitfield_names = RomFields::strArrayToVector_i18n("Region", nds_region_bitfield_names);
	d->fields.addField_bitfield(C_("NintendoDS", "DS Region Code"),
		v_nds_region_bitfield_names, 0, nds_region);

	if (!(hw_type & NintendoDSPrivate::DS_HW_DSi)) {
		// Not a DSi-enhanced or DSi-exclusive ROM image.
		if (romHeader->dsi.flags != 0) {
			// DSi flags.
			// NOTE: These are present in NDS games released after the DSi,
			// even if the game isn't DSi-enhanced.
			d->fields.addTab("DSi");
			auto *const vv_dsi_flags = d->getDSiFlagsStringVector();
			RomFields::AFLD_PARAMS params(RomFields::RFT_LISTDATA_CHECKBOXES, 8);
			params.headers = nullptr;
			params.data.single = vv_dsi_flags;
			params.mxd.checkboxes = romHeader->dsi.flags;
			d->fields.addField_listData(C_("RomData", "Flags"), &params);
		}
		return static_cast<int>(d->fields.count());
	}

	/** DSi-specific fields. **/
	d->fields.addTab("DSi");

	// Title ID
	d->fields.addField_string(C_("Nintendo", "Title ID"),
		d->dsi_getTitleID(), RomFields::STRF_MONOSPACE);

	// DSi filetype
	struct dsi_filetype_tbl_t{
		uint8_t dsi_filetype;
		const char *s_dsi_filetype;
	};
	static const array<dsi_filetype_tbl_t, 6> dsi_filetype_tbl = {{
		// tr: DSi-enhanced or DSi-exclusive cartridge.
		{DSi_FTYPE_CARTRIDGE,		NOP_C_("NintendoDS|DSiFileType", "Cartridge")},
		// tr: DSiWare (download-only title)
		{DSi_FTYPE_DSiWARE,		NOP_C_("NintendoDS|DSiFileType", "DSiWare")},
		// tr: DSi_FTYPE_SYSTEM_FUN_TOOL
		{DSi_FTYPE_SYSTEM_FUN_TOOL,	NOP_C_("NintendoDS|DSiFileType", "System Fun Tool")},
		// tr: Data file, e.g. DS cartridge whitelist.
		{DSi_FTYPE_NONEXEC_DATA,	NOP_C_("NintendoDS|DSiFileType", "Non-Executable Data File")},
		// tr: DSi_FTYPE_SYSTEM_BASE_TOOL
		{DSi_FTYPE_SYSTEM_BASE_TOOL,	NOP_C_("NintendoDS|DSiFileType", "System Base Tool")},
		// tr: System Menu
		{DSi_FTYPE_SYSTEM_MENU,		NOP_C_("NintendoDS|DSiFileType", "System Menu")},
	}};

	const char *s_dsi_filetype = nullptr;
	auto iter = std::find_if(dsi_filetype_tbl.cbegin(), dsi_filetype_tbl.cend(),
		[dsi_filetype](const dsi_filetype_tbl_t &p) noexcept -> bool {
			return (p.dsi_filetype == dsi_filetype);
		});
	if (iter != dsi_filetype_tbl.cend()) {
		// Found a match.
		s_dsi_filetype = iter->s_dsi_filetype;
	}

	// TODO: Is the field name too long?
	const char *const dsi_rom_type_title = C_("NintendoDS", "DSi ROM Type");
	if (s_dsi_filetype) {
		d->fields.addField_string(dsi_rom_type_title,
			pgettext_expr("NintendoDS|DSiFileType", s_dsi_filetype));
	} else {
		// Invalid file type.
		d->fields.addField_string(dsi_rom_type_title,
			fmt::format(FRUN(C_("RomData", "Unknown (0x{:0>2X})")), dsi_filetype));
	}

	// Key index. Determined by title ID.
	const uint32_t tid_hi = le32_to_cpu(romHeader->dsi.title_id.hi);
	int key_idx;
	if (tid_hi & 0x00000010) {
		// System application.
		key_idx = 2;
	} else if (tid_hi & 0x00000001) {
		// Applet.
		key_idx = 1;
	} else {
		// Cartridge and/or DSiWare.
		key_idx = 3;
	}

	// TODO: Keyset is determined by the system.
	// There might be some indicator in the cartridge header...
	d->fields.addField_string_numeric(C_("Nintendo", "Key Index"), key_idx);

	const char *const region_code_name = (d->cia
			? C_("RomData", "Region Code")
			: C_("NintendoDS", "DSi Region Code"));

	// DSi Region
	// Maps directly to the header field.
	// NOTE: Excluding the 'T' region.
	vector<string> *const v_dsi_region_bitfield_names = RomFields::strArrayToVector_i18n(
		"Region", WiiCommon::dsi_3ds_wiiu_region_bitfield_names);
	d->fields.addField_bitfield(region_code_name,
		v_dsi_region_bitfield_names, 3, le32_to_cpu(romHeader->dsi.region_code));

	// Age rating(s)
	// Note that not all 16 fields are present on DSi,
	// though the fields do match exactly, so no
	// mapping is necessary.
	RomFields::age_ratings_t age_ratings;
	// Valid ratings: 0-1, 3-9
	// TODO: Not sure if Finland is valid for DSi.
	static constexpr uint16_t valid_ratings = 0x3FB;

	for (int i = static_cast<int>(age_ratings.size())-1; i >= 0; i--) {
		if (!(valid_ratings & (1U << i))) {
			// Rating is not applicable for NintendoDS.
			age_ratings[i] = 0;
			continue;
		}

		// DSi ratings field:
		// - 0x1F: Age rating.
		// - 0x40: Prohibited in area. (TODO: Verify)
		// - 0x80: Rating is valid if set.
		const uint8_t dsi_rating = romHeader->dsi.age_ratings[i];
		if (!(dsi_rating & 0x80)) {
			// Rating is unused.
			age_ratings[i] = 0;
			continue;
		}

		// Set active | age value.
		age_ratings[i] = RomFields::AGEBF_ACTIVE | (dsi_rating & 0x1F);

		// Is the game prohibited?
		if (dsi_rating & 0x40) {
			age_ratings[i] |= RomFields::AGEBF_PROHIBITED;
		}
	}
	d->fields.addField_ageRatings(C_("RomData", "Age Ratings"), age_ratings);

	// Permissions and flags
	d->fields.addTab("Permissions");

	// Permissions
	static const array<const char*, 17> dsi_permissions_bitfield_names = {{
		NOP_C_("NintendoDS|DSi_Permissions", "Common Key"),
		NOP_C_("NintendoDS|DSi_Permissions", "AES Slot B"),
		NOP_C_("NintendoDS|DSi_Permissions", "AES Slot C"),
		NOP_C_("NintendoDS|DSi_Permissions", "SD Card"),
		NOP_C_("NintendoDS|DSi_Permissions", "eMMC Access"),
		NOP_C_("NintendoDS|DSi_Permissions", "Game Card Power On"),
		NOP_C_("NintendoDS|DSi_Permissions", "Shared2 File"),
		NOP_C_("NintendoDS|DSi_Permissions", "Sign JPEG for Launcher"),
		NOP_C_("NintendoDS|DSi_Permissions", "Game Card NTR Mode"),
		NOP_C_("NintendoDS|DSi_Permissions", "SSL Client Cert"),
		NOP_C_("NintendoDS|DSi_Permissions", "Sign JPEG for User"),
		NOP_C_("NintendoDS|DSi_Permissions", "Photo Read Access"),
		NOP_C_("NintendoDS|DSi_Permissions", "Photo Write Access"),
		NOP_C_("NintendoDS|DSi_Permissions", "SD Card Read Access"),
		NOP_C_("NintendoDS|DSi_Permissions", "SD Card Write Access"),
		NOP_C_("NintendoDS|DSi_Permissions", "Game Card Save Read Access"),
		NOP_C_("NintendoDS|DSi_Permissions", "Game Card Save Write Access"),

		// FIXME: How to handle unused entries for RFT_LISTDATA?
		/*
		// Bits 17-30 are not used.
		nullptr, nullptr, nullptr,
		nullptr, nullptr, nullptr, nullptr,
		nullptr, nullptr, nullptr, nullptr,
		nullptr, nullptr, nullptr,

		NOP_C_("NintendoDS|DSi_Permissions", "Debug Key"),
		*/
	}};

	// Convert to RomFields::ListData_t for RFT_LISTDATA.
	auto *const vv_dsi_perm = new RomFields::ListData_t(dsi_permissions_bitfield_names.size());
	for (int i = static_cast<int>(dsi_permissions_bitfield_names.size()) - 1; i >= 0; i--) {
		auto &data_row = vv_dsi_perm->at(i);
		data_row.emplace_back(
			pgettext_expr("NintendoDS|DSi_Permissions",
				dsi_permissions_bitfield_names[i]));
	}

	RomFields::AFLD_PARAMS params(RomFields::RFT_LISTDATA_CHECKBOXES, rows_visible);
	params.headers = nullptr;
	params.data.single = vv_dsi_perm;
	params.mxd.checkboxes = le32_to_cpu(romHeader->dsi.access_control);
	d->fields.addField_listData(C_("NintendoDS", "Permissions"), &params);

	// DSi flags
	auto *const vv_dsi_flags = d->getDSiFlagsStringVector();
	params.headers = nullptr;
	params.data.single = vv_dsi_flags;
	params.mxd.checkboxes = romHeader->dsi.flags;
	d->fields.addField_listData(C_("RomData", "Flags"), &params);

	// Finished reading the field data.
	return static_cast<int>(d->fields.count());
}

/**
 * Load metadata properties.
 * Called by RomData::metaData() if the metadata hasn't been loaded yet.
 * @return Number of metadata properties read on success; negative POSIX error code on error.
 */
int NintendoDS::loadMetaData(void)
{
	RP_D(NintendoDS);
	if (!d->metaData.empty()) {
		// Metadata *has* been loaded...
		return 0;
	} else if (!d->file) {
		// File isn't open.
		return -EBADF;
	} else if (!d->isValid || static_cast<int>(d->romType) < 0) {
		// ROM image isn't valid.
		return -EIO;
	}

	// ROM header is read in the constructor.
	const NDS_RomHeader *const romHeader = &d->romHeader;
	d->metaData.reserve(3);	// Maximum of 3 metadata properties.

	// Title
	bool has_full_title = false;
	if (!d->nds_icon_title) {
		const_cast<NintendoDSPrivate*>(d)->loadIconTitleData();
	}
	if (d->nds_icon_title) {
		// Full title
		const RomMetaData *const other = d->nds_icon_title->metaData();
		assert(other != nullptr);
		if (other) {
			d->metaData.addMetaData_metaData(other);
			has_full_title = true;	// TODO: Verify?
		}
	}

	if (!has_full_title) {
		// Full title is not available.
		// Use the short title from the NDS header.
		d->metaData.addMetaData_string(Property::Title,
			latin1_to_utf8(romHeader->title, ARRAY_SIZE_I(romHeader->title)),
			RomMetaData::STRF_TRIM_END);
	}

	// Publisher
	// TODO: Use publisher from the full title?
	const char *const publisher = NintendoPublishers::lookup(romHeader->company);
	if (publisher) {
		d->metaData.addMetaData_string(Property::Publisher, publisher);
	} else {
		if (isalnum_ascii(romHeader->company[0]) && isalnum_ascii(romHeader->company[1])) {
			const array<char, 3> s_company = {{
				romHeader->company[0],
				romHeader->company[1],
				'\0'
			}};
			d->metaData.addMetaData_string(Property::Publisher,
				fmt::format(FRUN(C_("RomData", "Unknown ({:s})")), s_company.data()));
		} else {
			d->metaData.addMetaData_string(Property::Publisher,
				fmt::format(FRUN(C_("RomData", "Unknown ({:0>2X} {:0>2X})")),
					static_cast<unsigned int>(romHeader->company[0]),
					static_cast<unsigned int>(romHeader->company[1])));
		}
	}

	/** Custom properties! **/

	// Game ID
	const string s_gameID = d->getGameID();
	// NOTE: Only showing the game ID if the first four characters are printable.
	if (s_gameID.compare(0, 4, "____") != 0) {
		d->metaData.addMetaData_string(Property::GameID, s_gameID);
	}

	// Title ID (DSi only)
	if (d->isDSi()) {
		d->metaData.addMetaData_string(Property::TitleID, d->dsi_getTitleID());
	}

	// Region code
	// Uses the DSi region if present.
	// Otherwise, uses the NDS region.
	if (d->isDSi()) {
		// NOTE: No 'T' region for DSi.
		d->metaData.addMetaData_string(Property::RegionCode,
			WiiCommon::getRegionCodeForMetadataProperty(
				le32_to_cpu(romHeader->dsi.region_code), false));
	} else {
		// Check for NDS regions.
		const char *s_region_code;
		switch (romHeader->nds_region & 0xC0) {
			case NDS_REGION_CHINA:
				s_region_code = C_("Region", "China");
				break;
			case NDS_REGION_SKOREA:
				s_region_code = C_("Region", "South Korea");
				break;

			case NDS_REGION_CHINA | NDS_REGION_SKOREA:
				// China *and* South Korea? Not valid...
				s_region_code = nullptr;
				break;

			default:
				s_region_code = C_("Region", "Region-Free");
				break;
		}
		d->metaData.addMetaData_string(Property::RegionCode, s_region_code);
	}

	// Finished reading the metadata.
	return static_cast<int>(d->metaData.count());
}

/**
 * Load an internal image.
 * Called by RomData::image().
 * @param imageType	[in] Image type to load.
 * @param pImage	[out] Reference to rp_image_const_ptr to store the image in.
 * @return 0 on success; negative POSIX error code on error.
 */
int NintendoDS::loadInternalImage(ImageType imageType, rp_image_const_ptr &pImage)
{
	ASSERT_loadInternalImage(imageType, pImage);

	// Wrapper function around NintendoDS_BNR.
	RP_D(NintendoDS);
	if (!d->nds_icon_title) {
		if (d->loadIconTitleData() != 0) {
			// Error loading the icon/title data.
			pImage.reset();
			return -EIO;
		}
	}

	return d->nds_icon_title->loadInternalImage(imageType, pImage);
}

/**
 * Get the animated icon data.
 *
 * Check imgpf for IMGPF_ICON_ANIMATED first to see if this
 * object has an animated icon.
 *
 * @return Animated icon data, or nullptr if no animated icon is present.
 */
IconAnimDataConstPtr NintendoDS::iconAnimData(void) const
{
	// Wrapper function around NintendoDS_BNR.
	RP_D(const NintendoDS);
	if (!d->nds_icon_title) {
		if (const_cast<NintendoDSPrivate*>(d)->loadIconTitleData() != 0) {
			// Error loading the icon/title data.
			return {};
		}
	}

	return d->nds_icon_title->iconAnimData();
}

/**
 * Get a list of URLs for an external image type.
 *
 * A thumbnail size may be requested from the shell.
 * If the subclass supports multiple sizes, it should
 * try to get the size that most closely matches the
 * requested size.
 *
 * @param imageType	[in]     Image type
 * @param extURLs	[out]    Output vector
 * @param size		[in,opt] Requested image size. This may be a requested
 *                               thumbnail size in pixels, or an ImageSizeType
 *                               enum value.
 * @return 0 on success; negative POSIX error code on error.
 */
int NintendoDS::extURLs(ImageType imageType, vector<ExtURL> &extURLs, int size) const
{
	extURLs.clear();
	ASSERT_extURLs(imageType);

	// Check for DS ROMs that don't have boxart.
	RP_D(const NintendoDS);
	if (!d->isValid || static_cast<int>(d->romType) < 0) {
		// ROM image isn't valid.
		return -EIO;
	} else if (!memcmp(d->romHeader.id4, "NTRJ", 4) ||
	           !memcmp(d->romHeader.id4, "####", 4))
	{
		// This is either a prototype, a download demo, or homebrew.
		// No external images are available.
		return -ENOENT;
	}

	if (d->isDSi()) {
		// Check for DSi SRLs that aren't cartridge dumps.
		// TODO: Does GameTDB have DSiWare covers?
		const uint16_t dsi_filetype = le16_to_cpu(d->romHeader.dsi.title_id.catID);
		if (dsi_filetype != DSi_FTYPE_CARTRIDGE) {
			// Not a cartridge dump.
			// No external images are available.
			return -ENOENT;
		}
	}

	// Get the image sizes and sort them based on the
	// requested image size.
	vector<ImageSizeDef> sizeDefs = supportedImageSizes(imageType);
	if (sizeDefs.empty()) {
		// No image sizes.
		return -ENOENT;
	}

	// Select the best size.
	const ImageSizeDef *const sizeDef = d->selectBestSize(sizeDefs, size);
	if (!sizeDef) {
		// No size available...
		return -ENOENT;
	}

	// NOTE: Only downloading the first size as per the
	// sort order, since GameTDB basically guarantees that
	// all supported sizes for an image type are available.
	// TODO: Add cache keys for other sizes in case they're
	// downloaded and none of these are available?

	// Determine the image type name.
	const char *imageTypeName_base;
	const char *ext;
	switch (imageType) {
#ifdef HAVE_JPEG
		case IMG_EXT_COVER:
			imageTypeName_base = "cover";
			ext = ".jpg";
			break;
		case IMG_EXT_COVER_FULL:
			imageTypeName_base = "coverfull";
			ext = ".jpg";
			break;
#endif /* HAVE_JPEG */
		case IMG_EXT_BOX:
			imageTypeName_base = "box";
			ext = ".png";
			break;
		default:
			// Unsupported image type.
			return -ENOENT;
	}

	// ROM header is read in the constructor.
	const NDS_RomHeader *const romHeader = &d->romHeader;

	// Game ID. (GameTDB uses ID4 for Nintendo DS.)
	// The ID4 cannot have non-printable characters.
	char id4[5];
	for (int i = ARRAY_SIZE(romHeader->id4)-1; i >= 0; i--) {
		if (!ISPRINT(romHeader->id4[i])) {
			// Non-printable character found.
			return -ENOENT;
		}
		id4[i] = romHeader->id4[i];
	}
	// NULL-terminated ID4 is needed for the
	// GameTDB URL functions.
	id4[4] = 0;

	// Determine the GameTDB language code(s).
	const vector<uint16_t> tdb_lc = d->ndsRegionToGameTDB(
		romHeader->nds_region,
		(d->isDSi())
			? le32_to_cpu(romHeader->dsi.region_code)
			: 0, /* not a DSi-enhanced/exclusive ROM */
		romHeader->id4[3]);

	// If we're downloading a "high-resolution" image (M or higher),
	// also add the default image to ExtURLs in case the user has
	// high-resolution image downloads disabled.
	array<const ImageSizeDef*, 2> szdefs_dl;
	szdefs_dl[0] = sizeDef;
	unsigned int szdef_count;
	if (sizeDef->index >= 2) {
		// M or higher.
		szdefs_dl[1] = &sizeDefs[0];
		szdef_count = 2;
	} else {
		// Default or S.
		szdef_count = 1;
	}

	// Add the URLs.
	extURLs.resize(szdef_count * tdb_lc.size());
	auto extURL_iter = extURLs.begin();
	for (unsigned int i = 0; i < szdef_count; i++) {
		// Current image type
		const string imageTypeName = fmt::format(FSTR("{:s}{:s}"),
			imageTypeName_base, (szdefs_dl[i]->name ? szdefs_dl[i]->name : ""));

		// Add the images.
		for (const uint16_t lc : tdb_lc) {
			const string lc_str = SystemRegion::lcToStringUpper(lc);
			ExtURL &extURL = *extURL_iter;
			extURL.url = d->getURL_GameTDB("ds", imageTypeName.c_str(), lc_str.c_str(), id4, ext);
			extURL.cache_key = d->getCacheKey_GameTDB("ds", imageTypeName.c_str(), lc_str.c_str(), id4, ext);
			extURL.width = szdefs_dl[i]->width;
			extURL.height = szdefs_dl[i]->height;
			extURL.high_res = (szdefs_dl[i]->index >= 2);
			++extURL_iter;
		}
	}

	// All URLs added.
	return 0;
}

/**
 * Does this ROM image have "dangerous" permissions?
 *
 * @return True if the ROM image has "dangerous" permissions; false if not.
 */
bool NintendoDS::hasDangerousPermissions(void) const
{
	// Load permissions.
	// TODO: If this is DSiWare, check DSiWare permissions?
	RP_D(const NintendoDS);

	// If Game Card Power On is set, eMMC Access and SD Card must be off.
	// This combination is normally not found in licensed games,
	// and is only found in the system menu. Some homebrew titles
	// might have this set, though.
	const uint32_t dsi_access_control = le32_to_cpu(d->romHeader.dsi.access_control);
	if (dsi_access_control & DSi_ACCESS_GAME_CARD_POWER_ON) {
		// Game Card Power On is set.
		if (dsi_access_control & (DSi_ACCESS_SD_CARD | DSi_ACCESS_eMMC_ACCESS)) {
			// SD and/or eMMC is set.
			// This combination is not allowed by Nintendo, and
			// usually indicates some sort of homebrew.
			return true;
		}
	}

	// Not dangerous.
	return false;
}

} // namespace LibRomData
