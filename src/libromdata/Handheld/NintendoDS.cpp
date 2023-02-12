/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * NintendoDS.hpp: Nintendo DS(i) ROM reader.                              *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "librpbase/config.librpbase.h"

#include "NintendoDS.hpp"
#include "NintendoDS_p.hpp"
#include "data/NintendoPublishers.hpp"
#include "data/NintendoLanguage.hpp"

// Other rom-properties libraries
#include "librpbase/config/Config.hpp"
#include "librpbase/SystemRegion.hpp"
#include "librptexture/decoder/ImageDecoder_NDS.hpp"
using namespace LibRpBase;
using namespace LibRpText;
using namespace LibRpTexture;
using LibRpFile::IRpFile;
using LibRpFile::RpFile;

// C++ STL classes.
using std::array;
using std::string;
using std::vector;

namespace LibRomData {

ROMDATA_IMPL(NintendoDS)
ROMDATA_IMPL_IMG(NintendoDS)

/** NintendoDSPrivate **/

/* RomDataInfo */
// NOTE: Using the same image settings as Nintendo3DS.
const char *const NintendoDSPrivate::exts[] = {
	".nds",	// Nintendo DS
	".dsi",	// Nintendo DSi (devkitARM r46)
	".ids",	// iQue DS (no-intro)
	".srl",	// Official SDK extension

	nullptr
};
const char *const NintendoDSPrivate::mimeTypes[] = {
	// Unofficial MIME types from FreeDesktop.org.
	"application/x-nintendo-ds-rom",

	// Vendor-specific type listed in Fedora's mime.types.
	"application/vnd.nintendo.nitro.rom",

	// Unofficial MIME types.
	// TODO: Get these upstreamed on FreeDesktop.org.
	"application/x-nintendo-dsi-rom",

	nullptr
};
const RomDataInfo NintendoDSPrivate::romDataInfo = {
	"NintendoDS", exts, mimeTypes
};

NintendoDSPrivate::NintendoDSPrivate(NintendoDS *q, IRpFile *file, bool cia)
	: super(q, file, &romDataInfo)
	, iconAnimData(nullptr)
	, icon_first_frame(nullptr)
	, romType(RomType::Unknown)
	, romSize(0)
	, secData(0)
	, secArea(NDS_SECAREA_UNKNOWN)
	, nds_icon_title_loaded(false)
	, cia(cia)
	, fieldIdx_secData(-1)
	, fieldIdx_secArea(-1)
{
	// Clear the various structs.
	memset(&romHeader, 0, sizeof(romHeader));
	memset(&nds_icon_title, 0, sizeof(nds_icon_title));
}

NintendoDSPrivate::~NintendoDSPrivate()
{
	UNREF(iconAnimData);
}

/**
 * Load the icon/title data.
 * @return 0 on success; negative POSIX error code on error.
 */
int NintendoDSPrivate::loadIconTitleData(void)
{
	assert(this->file != nullptr);

	if (nds_icon_title_loaded) {
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

	// Read the icon/title data.
	size_t size = this->file->seekAndRead(icon_offset, &nds_icon_title, sizeof(nds_icon_title));

	// Make sure we have the correct size based on the version.
	if (size < sizeof(nds_icon_title.version)) {
		// Couldn't even load the version number...
		return -EIO;
	}

	unsigned int req_size;
	switch (le16_to_cpu(nds_icon_title.version)) {
		case NDS_ICON_VERSION_ORIGINAL:
			req_size = NDS_ICON_SIZE_ORIGINAL;
			break;
		case NDS_ICON_VERSION_HANS:
			req_size = NDS_ICON_SIZE_HANS;
			break;
		case NDS_ICON_VERSION_HANS_KO:
			req_size = NDS_ICON_SIZE_HANS_KO;
			break;
		case NDS_ICON_VERSION_DSi:
			req_size = NDS_ICON_SIZE_DSi;
			break;
		default:
			// Invalid version number.
			return -EIO;
	}

	if (size < req_size) {
		// Error reading the icon data.
		return -EIO;
	}

	// Icon data loaded.
	nds_icon_title_loaded = true;
	return 0;
}

/**
 * Load the ROM image's icon.
 * @return Icon, or nullptr on error.
 */
const rp_image *NintendoDSPrivate::loadIcon(void)
{
	if (icon_first_frame) {
		// Icon has already been loaded.
		return icon_first_frame;
	} else if (!this->file || !this->isValid) {
		// Can't load the icon.
		return nullptr;
	}

	// Attempt to load the icon/title data.
	int ret = loadIconTitleData();
	if (ret != 0) {
		// Error loading the icon/title data.
		return nullptr;
	}

	// Load the icon data.
	// TODO: Only read the first frame unless specifically requested?
	this->iconAnimData = new IconAnimData();
	iconAnimData->count = 0;

	// Check if a DSi animated icon is present.
	// TODO: Some configuration option to return the standard
	// NDS icon for the standard icon instead of the first frame
	// of the animated DSi icon? (Except for DSiWare...)
	if (le16_to_cpu(nds_icon_title.version) < NDS_ICON_VERSION_DSi ||
	    (nds_icon_title.dsi_icon_seq[0] & cpu_to_le16(0xFF)) == 0)
	{
		// Either this isn't a DSi icon/title struct (pre-v0103),
		// or the animated icon sequence is invalid.

		// Convert the NDS icon to rp_image.
		iconAnimData->frames[0] = ImageDecoder::fromNDS_CI4(32, 32,
			nds_icon_title.icon_data, sizeof(nds_icon_title.icon_data),
			nds_icon_title.icon_pal,  sizeof(nds_icon_title.icon_pal));
		iconAnimData->count = 1;
	} else {
		// Animated icon is present.

		// Maximum number of combinations based on bitmap index,
		// palette index, and flip bits is 256. We don't want to
		// reserve 256 images, so we'll use a map to determine
		// which combinations go to which bitmap.

		// dsi_icon_seq is limited to 64, so there's still a maximum
		// of 64 possible bitmaps.

		// Index: High byte of token.
		// Value: Bitmap index. (0xFF for unused)
		array<uint8_t, 256> arr_bmpUsed;
		arr_bmpUsed.fill(0xFF);

		// Parse the icon sequence.
		uint8_t bmp_idx = 0;
		int seq_idx;
		for (seq_idx = 0; seq_idx < ARRAY_SIZE_I(nds_icon_title.dsi_icon_seq); seq_idx++) {
			const uint16_t seq = le16_to_cpu(nds_icon_title.dsi_icon_seq[seq_idx]);
			const int delay = (seq & 0xFF);
			if (delay == 0) {
				// End of sequence.
				break;
			}

			// Token format: (bits)
			// - 15:    V flip (1=yes, 0=no) [TODO]
			// - 14:    H flip (1=yes, 0=no) [TODO]
			// - 13-11: Palette index.
			// - 10-8:  Bitmap index.
			// - 7-0:   Frame duration. (units of 60 Hz)

			// NOTE: IconAnimData doesn't support arbitrary combinations
			// of palette and bitmap. As a workaround, we'll make each
			// combination a unique bitmap, which means we have a maximum
			// of 64 bitmaps.
			uint8_t high_token = (seq >> 8);
			if (arr_bmpUsed[high_token] == 0xFF) {
				// Not used yet. Create the bitmap.
				const uint8_t bmp = (high_token & 7);
				const uint8_t pal = (high_token >> 3) & 7;
				rp_image *img = ImageDecoder::fromNDS_CI4(32, 32,
					nds_icon_title.dsi_icon_data[bmp],
					sizeof(nds_icon_title.dsi_icon_data[bmp]),
					nds_icon_title.dsi_icon_pal[pal],
					sizeof(nds_icon_title.dsi_icon_pal[pal]));
				if (high_token & (3U << 6)) {
					// At least one flip bit is set.
					rp_image::FlipOp flipOp = rp_image::FLIP_NONE;
					if (high_token & (1U << 6)) {
						// H-flip
						flipOp = rp_image::FLIP_H;
					}
					if (high_token & (1U << 7)) {
						// V-flip
						flipOp = static_cast<rp_image::FlipOp>(flipOp | rp_image::FLIP_V);
					}
					rp_image *const flipimg = img->flip(flipOp);
					img->unref();
					img = flipimg;
				}
				iconAnimData->frames[bmp_idx] = img;
				arr_bmpUsed[high_token] = bmp_idx;
				bmp_idx++;
			}
			iconAnimData->seq_index[seq_idx] = arr_bmpUsed[high_token];
			iconAnimData->delays[seq_idx].numer = static_cast<uint16_t>(delay);
			iconAnimData->delays[seq_idx].denom = 60;
			iconAnimData->delays[seq_idx].ms = delay * 1000 / 60;
		}
		iconAnimData->count = bmp_idx;
		iconAnimData->seq_count = seq_idx;
	}

	// NOTE: We're not deleting iconAnimData even if we only have
	// a single icon because iconAnimData() will call loadIcon()
	// if iconAnimData is nullptr.

	// Return a pointer to the first frame.
	icon_first_frame = iconAnimData->frames[iconAnimData->seq_index[0]];
	return icon_first_frame;
}

/**
 * Get the maximum supported language for an icon/title version.
 * @param version Icon/title version.
 * @return Maximum supported language.
 */
NDS_Language_ID NintendoDSPrivate::getMaxSupportedLanguage(uint16_t version)
{
	NDS_Language_ID maxID;
	if (version >= NDS_ICON_VERSION_HANS_KO) {
		maxID = NDS_LANG_KOREAN;
	} else if (version >= NDS_ICON_VERSION_HANS) {
		maxID = NDS_LANG_CHINESE_SIMP;
	} else {
		maxID = NDS_LANG_SPANISH;
	}
	return maxID;
}

/**
 * Get the language ID to use for the title fields.
 * @return NDS language ID.
 */
NDS_Language_ID NintendoDSPrivate::getLanguageID(void) const
{
	if (!nds_icon_title_loaded) {
		// Attempt to load the icon/title data.
		if (const_cast<NintendoDSPrivate*>(this)->loadIconTitleData() != 0) {
			// Error loading the icon/title data.
			return (NDS_Language_ID)-1;
		}

		// Make sure it was actually loaded.
		if (!nds_icon_title_loaded) {
			// Icon/title data was not loaded.
			return (NDS_Language_ID)-1;
		}
	}

	// Version number check is required for ZH and KO.
	const uint16_t version = le16_to_cpu(nds_icon_title.version);
	NDS_Language_ID langID = (NDS_Language_ID)NintendoLanguage::getNDSLanguage(version);

	// Check that the field is valid.
	if (nds_icon_title.title[langID][0] == cpu_to_le16('\0')) {
		// Not valid. Check English.
		if (nds_icon_title.title[NDS_LANG_ENGLISH][0] != cpu_to_le16('\0')) {
			// English is valid.
			langID = NDS_LANG_ENGLISH;
		} else {
			// Not valid. Check Japanese.
			if (nds_icon_title.title[NDS_LANG_JAPANESE][0] != cpu_to_le16('\0')) {
				// Japanese is valid.
				langID = NDS_LANG_JAPANESE;
			} else {
				// Not valid...
				// Default to English anyway.
				langID = NDS_LANG_ENGLISH;
			}
		}
	}

	return langID;
}

/**
 * Get the default language code for the multi-string fields.
 * @return Language code, e.g. 'en' or 'es'.
 */
uint32_t NintendoDSPrivate::getDefaultLC(void) const
{
	// Get the system language.
	// TODO: Verify against the game's region code?
	NDS_Language_ID langID = getLanguageID();

	// Version number check is required for ZH and KO.
	const NDS_Language_ID maxID = getMaxSupportedLanguage(
		le16_to_cpu(nds_icon_title.version));
	uint32_t lc = NintendoLanguage::getNDSLanguageCode(langID, maxID);
	if (lc == 0) {
		// Invalid language code...
		// Default to English.
		lc = 'en';
	}
	return lc;
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
	static const char *const dsi_flags_bitfield_names[] = {
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
	};

	// Convert to RomFields::ListData_t for RFT_LISTDATA.
	auto vv_dsi_flags = new RomFields::ListData_t(ARRAY_SIZE(dsi_flags_bitfield_names));
	for (int i = ARRAY_SIZE(dsi_flags_bitfield_names)-1; i >= 0; i--) {
		auto &data_row = vv_dsi_flags->at(i);
		data_row.emplace_back(
			dpgettext_expr(RP_I18N_DOMAIN, "NintendoDS|DSi_Flags",
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
 * @param file Open ROM image.
 */
NintendoDS::NintendoDS(IRpFile *file)
	: super(new NintendoDSPrivate(this, file, false))
{
	RP_D(NintendoDS);
	if (!d->file) {
		// Could not ref() the file handle.
		return;
	}

	init();
}

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
 * @param file Open ROM image.
 * @param cia If true, hide fields that aren't relevant to DSiWare in 3DS CIA packages.
 */
NintendoDS::NintendoDS(IRpFile *file, bool cia)
	: super(new NintendoDSPrivate(this, file, cia))
{
	RP_D(NintendoDS);
	if (!d->file) {
		// Could not ref() the file handle.
		return;
	}

	init();
}

/**
 * Common initialization function for the constructors.
 */
void NintendoDS::init(void)
{
	RP_D(NintendoDS);

	// Read the ROM header.
	d->file->rewind();
	size_t size = d->file->read(&d->romHeader, sizeof(d->romHeader));
	if (size != sizeof(d->romHeader)) {
		UNREF_AND_NULL_NOCHK(d->file);
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
	d->isValid = ((int)d->romType >= 0);

	if (!d->isValid) {
		UNREF_AND_NULL_NOCHK(d->file);
		return;
	}

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
	static const uint8_t nintendo_gba_logo[16] = {
		0x24, 0xFF, 0xAE, 0x51, 0x69, 0x9A, 0xA2, 0x21,
		0x3D, 0x84, 0x82, 0x0A, 0x84, 0xE4, 0x09, 0xAD
	};
	static const uint8_t nintendo_ds_logo_slot2[16] = {
		0xC8, 0x60, 0x4F, 0xE2, 0x01, 0x70, 0x8F, 0xE2,
		0x17, 0xFF, 0x2F, 0xE1, 0x12, 0x4F, 0x11, 0x48,
	};

	const NDS_RomHeader *const romHeader =
		reinterpret_cast<const NDS_RomHeader*>(info->header.pData);
	if (!memcmp(romHeader->nintendo_logo, nintendo_gba_logo, sizeof(nintendo_gba_logo)) &&
	    romHeader->nintendo_logo_checksum == cpu_to_le16(0xCF56)) {
		// Nintendo logo is valid. (Slot-1)
		static const int8_t nds_romType[] = {
			(int8_t)NintendoDSPrivate::RomType::NDS,		// 0x00 == Nintendo DS
			(int8_t)NintendoDSPrivate::RomType::NDS,		// 0x01 == invalid (assuming DS)
			(int8_t)NintendoDSPrivate::RomType::DSi_Enhanced,	// 0x02 == DSi-enhanced
			(int8_t)NintendoDSPrivate::RomType::DSi_Exclusive,	// 0x03 == DSi-only
		};
		return nds_romType[romHeader->unitcode & 3];
	} else if (!memcmp(romHeader->nintendo_logo, nintendo_ds_logo_slot2, sizeof(nintendo_ds_logo_slot2)) &&
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
	static const char *const sysNames[16] = {
		// Nintendo (worldwide)
		"Nintendo DS", "Nintendo DS", "NDS", nullptr,
		"Nintendo DSi", "Nintendo DSi", "DSi", nullptr,

		// iQue (China)
		"iQue DS", "iQue DS", "NDS", nullptr,
		"iQue DSi", "iQue DSi", "DSi", nullptr
	};

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
		case IMG_INT_ICON: {
			static const ImageSizeDef sz_INT_ICON[] = {
				{nullptr, 32, 32, 0},
			};
			return vector<ImageSizeDef>(sz_INT_ICON,
				sz_INT_ICON + ARRAY_SIZE(sz_INT_ICON));
		}
#ifdef HAVE_JPEG
		case IMG_EXT_COVER: {
			static const ImageSizeDef sz_EXT_COVER[] = {
				{nullptr, 160, 144, 0},
				//{"S", 128, 115, 1},	// DISABLED; not needed.
				{"M", 400, 352, 2},
				{"HQ", 768, 680, 3},
			};
			return vector<ImageSizeDef>(sz_EXT_COVER,
				sz_EXT_COVER + ARRAY_SIZE(sz_EXT_COVER));
		}
		case IMG_EXT_COVER_FULL: {
			static const ImageSizeDef sz_EXT_COVER_FULL[] = {
				{nullptr, 340, 144, 0},
				//{"S", 272, 115, 1},	// Not currently present on GameTDB.
				{"M", 856, 352, 2},
				{"HQ", 1616, 680, 3},
			};
			return vector<ImageSizeDef>(sz_EXT_COVER_FULL,
				sz_EXT_COVER_FULL + ARRAY_SIZE(sz_EXT_COVER_FULL));
		}
#endif /* HAVE_JPEG */
		case IMG_EXT_BOX: {
			static const ImageSizeDef sz_EXT_BOX[] = {
				{nullptr, 240, 216, 0},
			};
			return vector<ImageSizeDef>(sz_EXT_BOX,
				sz_EXT_BOX + ARRAY_SIZE(sz_EXT_BOX));
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
uint32_t NintendoDS::imgpf(ImageType imageType) const
{
	ASSERT_imgpf(imageType);

	RP_D(const NintendoDS);
	uint32_t ret = 0;
	switch (imageType) {
		case IMG_INT_ICON:
			// Use nearest-neighbor scaling when resizing.
			// Also, need to check if this is an animated icon.
			const_cast<NintendoDSPrivate*>(d)->loadIcon();
			if (d->iconAnimData && d->iconAnimData->count > 1) {
				// Animated icon.
				ret = IMGPF_RESCALE_NEAREST | IMGPF_ICON_ANIMATED;
			} else {
				// Not animated.
				ret = IMGPF_RESCALE_NEAREST;
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
	if (!d->fields->empty()) {
		// Field data *has* been loaded...
		return 0;
	} else if (!d->file) {
		// File isn't open.
		return -EBADF;
	} else if (!d->isValid || (int)d->romType < 0) {
		// ROM image isn't valid.
		return -EIO;
	}

#ifdef _WIN32
	// Windows: 6 visible rows per RFT_LISTDATA.
	static const int rows_visible = 6;
#else
	// Linux: 4 visible rows per RFT_LISTDATA.
	static const int rows_visible = 4;
#endif

	// ROM header is read in the constructor.
	const NDS_RomHeader *const romHeader = &d->romHeader;
	const bool hasDSi = !!(romHeader->unitcode & NintendoDSPrivate::DS_HW_DSi);
	if (hasDSi) {
		// DSi-enhanced or DSi-exclusive.
		d->fields->reserve(10+7);
	} else {
		// NDS only.
		d->fields->reserve(10);
	}

	// NDS common fields
	d->fields->setTabName(0, "NDS");

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
	d->fields->addField_string(C_("NintendoDS", "Type"), nds_romType);

	// Title
	d->fields->addField_string(C_("RomData", "Title"),
		latin1_to_utf8(romHeader->title, ARRAY_SIZE_I(romHeader->title)),
		RomFields::STRF_TRIM_END);

	if (!d->nds_icon_title_loaded) {
		// Attempt to load the icon/title data.
		const_cast<NintendoDSPrivate*>(d)->loadIconTitleData();
	}
	if (d->nds_icon_title_loaded) {
		// Full title: Check if English is valid.
		// If it is, we'll de-duplicate fields.
		bool dedupe_titles = (d->nds_icon_title.title[NDS_LANG_ENGLISH][0] != cpu_to_le16(0));

		// Full title field.
		RomFields::StringMultiMap_t *const pMap_full_title = new RomFields::StringMultiMap_t();
		const NDS_Language_ID maxID = d->getMaxSupportedLanguage(
			le16_to_cpu(d->nds_icon_title.version));
		for (int langID = 0; langID <= maxID; langID++) {
			// Check for empty strings first.
			if (d->nds_icon_title.title[langID][0] == 0) {
				// Strings are empty.
				continue;
			}

			if (dedupe_titles && langID != NDS_LANG_ENGLISH) {
				// Check if the title matches English.
				// NOTE: Not converting to host-endian first, since
				// u16_strncmp() checks for equality and for 0.
				if (!u16_strncmp(d->nds_icon_title.title[langID],
						d->nds_icon_title.title[NDS_LANG_ENGLISH],
						ARRAY_SIZE(d->nds_icon_title.title[NDS_LANG_ENGLISH])))
				{
					// Full title field matches English.
					continue;
				}
			}

			const uint32_t lc = NintendoLanguage::getNDSLanguageCode(langID, maxID);
			assert(lc != 0);
			if (lc == 0)
				continue;

			if (d->nds_icon_title.title[langID][0] != cpu_to_le16('\0')) {
				pMap_full_title->emplace(lc, utf16le_to_utf8(
					d->nds_icon_title.title[langID],
					ARRAY_SIZE(d->nds_icon_title.title[langID])));
			}
		}

		if (!pMap_full_title->empty()) {
			const uint32_t def_lc = d->getDefaultLC();
			d->fields->addField_string_multi(C_("NintendoDS", "Full Title"), pMap_full_title, def_lc);
		} else {
			delete pMap_full_title;
		}
	}

	// Game ID
	d->fields->addField_string(C_("RomData", "Game ID"),
		latin1_to_utf8(romHeader->id6, ARRAY_SIZE_I(romHeader->id6)));

	// Publisher
	const char *const publisher_title = C_("RomData", "Publisher");
	const char *const publisher = NintendoPublishers::lookup(romHeader->company);
	if (publisher) {
		d->fields->addField_string(publisher_title, publisher);
	} else {
		if (ISALNUM(romHeader->company[0]) && ISALNUM(romHeader->company[1])) {
			d->fields->addField_string(publisher_title,
				rp_sprintf(C_("RomData", "Unknown (%.2s)"), romHeader->company));
		} else {
			d->fields->addField_string(publisher_title,
				rp_sprintf(C_("RomData", "Unknown (%02X %02X)"),
					static_cast<unsigned int>(romHeader->company[0]),
					static_cast<unsigned int>(romHeader->company[1])));
		}
	}

	// ROM version
	d->fields->addField_string_numeric(C_("RomData", "Revision"),
		romHeader->rom_version, RomFields::Base::Dec, 2);

	// Is the security data present?
	static const char *const nds_security_data_names[] = {
		NOP_C_("NintendoDS|SecurityData", "Blowfish Tables"),
		NOP_C_("NintendoDS|SecurityData", "Static Data"),
		NOP_C_("NintendoDS|SecurityData", "Random Data"),
	};
	vector<string> *const v_nds_security_data_names = RomFields::strArrayToVector_i18n(
		"NintendoDS|SecurityData", nds_security_data_names, ARRAY_SIZE(nds_security_data_names));
	d->fields->addField_bitfield(C_("NintendoDS", "Security Data"),
		v_nds_security_data_names, 0, d->secData);
	d->fieldIdx_secData = static_cast<int>(d->fields->count()-1);

	// Secure Area
	// TODO: Verify the CRC.
	d->fields->addField_string(C_("NintendoDS", "Secure Area"),
		d->getNDSSecureAreaString());
	d->fieldIdx_secArea = static_cast<int>(d->fields->count()-1);

	// Hardware type
	// NOTE: DS_HW_DS is inverted bit0; DS_HW_DSi is normal bit1.
	uint32_t hw_type = (romHeader->unitcode & 3) ^ NintendoDSPrivate::DS_HW_DS;
	if (hw_type == 0) {
		// 0x01 is invalid. Assume DS.
		hw_type = NintendoDSPrivate::DS_HW_DS;
	}

	static const char *const hw_bitfield_names[] = {
		"Nintendo DS", "Nintendo DSi"
	};
	vector<string> *const v_hw_bitfield_names = RomFields::strArrayToVector(
		hw_bitfield_names, ARRAY_SIZE(hw_bitfield_names));
	d->fields->addField_bitfield(C_("NintendoDS", "Hardware"),
		v_hw_bitfield_names, 0, hw_type);

	// NDS Region
	// Only used for region locking on Chinese iQue DS consoles.
	// Not displayed for DSiWare wrapped in 3DS CIA packages.
	uint32_t nds_region = 0;
	if (romHeader->nds_region & 0x80) {
		nds_region |= NintendoDSPrivate::NDS_REGION_CHINA;
	}
	if (romHeader->nds_region & 0x40) {
		nds_region |= NintendoDSPrivate::NDS_REGION_SKOREA;
	}
	if (nds_region == 0) {
		// No known region flags.
		// Note that the Sonic Colors demo has 0x02 here.
		nds_region = NintendoDSPrivate::NDS_REGION_FREE;
	}

	static const char *const nds_region_bitfield_names[] = {
		NOP_C_("Region", "Region-Free"),
		NOP_C_("Region", "South Korea"),
		NOP_C_("Region", "China"),
	};
	vector<string> *const v_nds_region_bitfield_names = RomFields::strArrayToVector_i18n(
		"Region", nds_region_bitfield_names, ARRAY_SIZE(nds_region_bitfield_names));
	d->fields->addField_bitfield(C_("NintendoDS", "DS Region Code"),
		v_nds_region_bitfield_names, 0, nds_region);

	if (!(hw_type & NintendoDSPrivate::DS_HW_DSi)) {
		// Not a DSi-enhanced or DSi-exclusive ROM image.
		if (romHeader->dsi.flags != 0) {
			// DSi flags.
			// NOTE: These are present in NDS games released after the DSi,
			// even if the game isn't DSi-enhanced.
			d->fields->addTab("DSi");
			auto vv_dsi_flags = d->getDSiFlagsStringVector();
			RomFields::AFLD_PARAMS params(RomFields::RFT_LISTDATA_CHECKBOXES, 8);
			params.headers = nullptr;
			params.data.single = vv_dsi_flags;
			params.mxd.checkboxes = romHeader->dsi.flags;
			d->fields->addField_listData(C_("NintendoDS", "Flags"), &params);
		}
		return static_cast<int>(d->fields->count());
	}

	/** DSi-specific fields. **/
	d->fields->addTab("DSi");

	// Title ID
	const uint32_t tid_hi = le32_to_cpu(romHeader->dsi.title_id.hi);
	d->fields->addField_string(C_("Nintendo", "Title ID"),
		rp_sprintf("%08X-%08X",
			tid_hi, le32_to_cpu(romHeader->dsi.title_id.lo)));

	// DSi filetype
	static const struct {
		uint8_t dsi_filetype;
		const char *s_dsi_filetype;
	} dsi_filetype_lkup_tbl[] = {
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
	};

	const char *s_dsi_filetype = nullptr;
	for (const auto &p : dsi_filetype_lkup_tbl) {
		if (p.dsi_filetype == dsi_filetype) {
			// Found a match.
			s_dsi_filetype = p.s_dsi_filetype;
			break;
		}
	}

	// TODO: Is the field name too long?
	const char *const dsi_rom_type_title = C_("NintendoDS", "DSi ROM Type");
	if (s_dsi_filetype) {
		d->fields->addField_string(dsi_rom_type_title,
			dpgettext_expr(RP_I18N_DOMAIN, "NintendoDS|DSiFileType", s_dsi_filetype));
	} else {
		// Invalid file type.
		d->fields->addField_string(dsi_rom_type_title,
			rp_sprintf(C_("RomData", "Unknown (0x%02X)"), dsi_filetype));
	}

	// Key index. Determined by title ID.
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
	d->fields->addField_string_numeric(C_("NintendoDS", "Key Index"), key_idx);

	const char *const region_code_name = (d->cia
			? C_("RomData", "Region Code")
			: C_("NintendoDS", "DSi Region Code"));

	// DSi Region
	// Maps directly to the header field.
	static const char *const dsi_region_bitfield_names[] = {
		NOP_C_("Region", "Japan"),
		NOP_C_("Region", "USA"),
		NOP_C_("Region", "Europe"),
		NOP_C_("Region", "Australia"),
		NOP_C_("Region", "China"),
		NOP_C_("Region", "South Korea"),
	};
	vector<string> *const v_dsi_region_bitfield_names = RomFields::strArrayToVector_i18n(
		"Region", dsi_region_bitfield_names, ARRAY_SIZE(dsi_region_bitfield_names));
	d->fields->addField_bitfield(region_code_name,
		v_dsi_region_bitfield_names, 3, le32_to_cpu(romHeader->dsi.region_code));

	// Age rating(s)
	// Note that not all 16 fields are present on DSi,
	// though the fields do match exactly, so no
	// mapping is necessary.
	RomFields::age_ratings_t age_ratings;
	// Valid ratings: 0-1, 3-9
	// TODO: Not sure if Finland is valid for DSi.
	static const uint16_t valid_ratings = 0x3FB;

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
	d->fields->addField_ageRatings(C_("RomData", "Age Ratings"), age_ratings);

	// Permissions and flags
	d->fields->addTab("Permissions");

	// Permissions
	static const char *const dsi_permissions_bitfield_names[] = {
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
	};

	// Convert to RomFields::ListData_t for RFT_LISTDATA.
	auto vv_dsi_perm = new RomFields::ListData_t(ARRAY_SIZE(dsi_permissions_bitfield_names));
	for (int i = ARRAY_SIZE(dsi_permissions_bitfield_names)-1; i >= 0; i--) {
		auto &data_row = vv_dsi_perm->at(i);
		data_row.emplace_back(
			dpgettext_expr(RP_I18N_DOMAIN, "NintendoDS|DSi_Permissions",
				dsi_permissions_bitfield_names[i]));
	}

	RomFields::AFLD_PARAMS params(RomFields::RFT_LISTDATA_CHECKBOXES, rows_visible);
	params.headers = nullptr;
	params.data.single = vv_dsi_perm;
	params.mxd.checkboxes = le32_to_cpu(romHeader->dsi.access_control);
	d->fields->addField_listData(C_("NintendoDS", "Permissions"), &params);

	// DSi flags
	auto vv_dsi_flags = d->getDSiFlagsStringVector();
	params.headers = nullptr;
	params.data.single = vv_dsi_flags;
	params.mxd.checkboxes = romHeader->dsi.flags;
	d->fields->addField_listData(C_("NintendoDS", "Flags"), &params);

	// Finished reading the field data.
	return static_cast<int>(d->fields->count());
}

/**
 * Load metadata properties.
 * Called by RomData::metaData() if the field data hasn't been loaded yet.
 * @return Number of metadata properties read on success; negative POSIX error code on error.
 */
int NintendoDS::loadMetaData(void)
{
	RP_D(NintendoDS);
	if (d->metaData != nullptr) {
		// Metadata *has* been loaded...
		return 0;
	} else if (!d->file) {
		// File isn't open.
		return -EBADF;
	} else if (!d->isValid || (int)d->romType < 0) {
		// ROM image isn't valid.
		return -EIO;
	}

	// Create the metadata object.
	d->metaData = new RomMetaData();
	d->metaData->reserve(2);	// Maximum of 2 metadata properties.

	// ROM header is read in the constructor.
	const NDS_RomHeader *const romHeader = &d->romHeader;

	// Title
	string s_title;
	if (!d->nds_icon_title_loaded) {
		// Attempt to load the icon/title data.
		const_cast<NintendoDSPrivate*>(d)->loadIconTitleData();
	}
	if (d->nds_icon_title_loaded) {
		// Full title.
		// TODO: Use the default LC if it's available.
		// For now, default to English.
		if (d->nds_icon_title.title[NDS_LANG_ENGLISH][0] != cpu_to_le16(0)) {
			s_title = utf16le_to_utf8(d->nds_icon_title.title[NDS_LANG_ENGLISH],
			                        ARRAY_SIZE(d->nds_icon_title.title[NDS_LANG_ENGLISH]));

			// Adjust the title based on the number of lines.
			size_t nl_1 = s_title.find('\n');
			if (nl_1 != string::npos) {
				// Found the first newline.
				size_t nl_2 = s_title.find('\n', nl_1+1);
				if (nl_2 != string::npos) {
					// Found the second newline.
					// Change the first to a space, and remove the third line.
					s_title[nl_1] = ' ';
					s_title.resize(nl_2);
				} else {
					// Only two lines.
					// Remove the second line.
					s_title.resize(nl_1);
				}
			}
		}
	}

	if (s_title.empty()) {
		// Full title is not available.
		// Use the short title from the NDS header.
		s_title = latin1_to_utf8(romHeader->title, ARRAY_SIZE_I(romHeader->title));
	}

	d->metaData->addMetaData_string(Property::Title, s_title, RomMetaData::STRF_TRIM_END);

	// Publisher
	// TODO: Use publisher from the full title?
	const char *const publisher = NintendoPublishers::lookup(romHeader->company);
	d->metaData->addMetaData_string(Property::Publisher,
		publisher ? publisher :
			rp_sprintf(C_("RomData", "Unknown (%.2s)"), romHeader->company));

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
int NintendoDS::loadInternalImage(ImageType imageType, const rp_image **pImage)
{
	ASSERT_loadInternalImage(imageType, pImage);
	RP_D(NintendoDS);
	ROMDATA_loadInternalImage_single(
		IMG_INT_ICON,		// ourImageType
		d->file,		// file
		d->isValid,		// isValid
		d->romType,		// romType
		d->icon_first_frame,	// imgCache
		d->loadIcon);		// func
}

/**
 * Get the animated icon data.
 *
 * Check imgpf for IMGPF_ICON_ANIMATED first to see if this
 * object has an animated icon.
 *
 * The retrieved IconAnimData must be ref()'d by the caller if the
 * caller stores it instead of using it immediately.
 *
 * @return Animated icon data, or nullptr if no animated icon is present.
 */
const IconAnimData *NintendoDS::iconAnimData(void) const
{
	RP_D(const NintendoDS);
	if (!d->iconAnimData) {
		// Load the icon.
		if (!const_cast<NintendoDSPrivate*>(d)->loadIcon()) {
			// Error loading the icon.
			return nullptr;
		}
		if (!d->iconAnimData) {
			// Still no icon...
			return nullptr;
		}
	}

	if (d->iconAnimData->count <= 1) {
		// Not an animated icon.
		return nullptr;
	}

	// Return the icon animation data.
	return d->iconAnimData;
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
int NintendoDS::extURLs(ImageType imageType, vector<ExtURL> *pExtURLs, int size) const
{
	ASSERT_extURLs(imageType, pExtURLs);
	pExtURLs->clear();

	// Check for DS ROMs that don't have boxart.
	RP_D(const NintendoDS);
	if (!d->isValid || (int)d->romType < 0) {
		// ROM image isn't valid.
		return -EIO;
	} else if (!memcmp(d->romHeader.id4, "NTRJ", 4) ||
	           !memcmp(d->romHeader.id4, "####", 4))
	{
		// This is either a prototype, a download demo, or homebrew.
		// No external images are available.
		return -ENOENT;
	}

	if (d->romHeader.unitcode & NintendoDSPrivate::DS_HW_DSi) {
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
	vector<uint16_t> tdb_lc = d->ndsRegionToGameTDB(
		romHeader->nds_region,
		(romHeader->unitcode & NintendoDSPrivate::DS_HW_DSi)
			? le32_to_cpu(romHeader->dsi.region_code)
			: 0, /* not a DSi-enhanced/exclusive ROM */
		romHeader->id4[3]);

	// If we're downloading a "high-resolution" image (M or higher),
	// also add the default image to ExtURLs in case the user has
	// high-resolution image downloads disabled.
	const ImageSizeDef *szdefs_dl[2];
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
	pExtURLs->resize(szdef_count * tdb_lc.size());
	auto extURL_iter = pExtURLs->begin();
	const auto tdb_lc_cend = tdb_lc.cend();
	for (unsigned int i = 0; i < szdef_count; i++) {
		// Current image type.
		char imageTypeName[16];
		snprintf(imageTypeName, sizeof(imageTypeName), "%s%s",
			 imageTypeName_base, (szdefs_dl[i]->name ? szdefs_dl[i]->name : ""));

		// Add the images.
		for (auto tdb_iter = tdb_lc.cbegin();
		     tdb_iter != tdb_lc_cend; ++tdb_iter, ++extURL_iter)
		{
			const string lc_str = SystemRegion::lcToStringUpper(*tdb_iter);
			extURL_iter->url = d->getURL_GameTDB("ds", imageTypeName, lc_str.c_str(), id4, ext);
			extURL_iter->cache_key = d->getCacheKey_GameTDB("ds", imageTypeName, lc_str.c_str(), id4, ext);
			extURL_iter->width = szdefs_dl[i]->width;
			extURL_iter->height = szdefs_dl[i]->height;
			extURL_iter->high_res = (szdefs_dl[i]->index >= 2);
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

}
