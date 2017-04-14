/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * NintendoDS.hpp: Nintendo DS(i) ROM reader.                              *
 *                                                                         *
 * Copyright (c) 2016-2017 by David Korth.                                 *
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
 * You should have received a copy of the GNU General Public License along *
 * with this program; if not, write to the Free Software Foundation, Inc., *
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.           *
 ***************************************************************************/

#include "config.libromdata.h"

#include "NintendoDS.hpp"
#include "RomData_p.hpp"

#include "data/NintendoPublishers.hpp"
#include "nds_structs.h"
#include "SystemRegion.hpp"

#include "common.h"
#include "byteswap.h"
#include "TextFuncs.hpp"
#include "file/IRpFile.hpp"

#include "img/rp_image.hpp"
#include "img/ImageDecoder.hpp"
#include "img/IconAnimData.hpp"

// C includes. (C++ namespace)
#include <cassert>
#include <cstring>
#include <cstddef>

// C++ includes.
#include <algorithm>
#include <vector>
using std::vector;

namespace LibRomData {

class NintendoDSPrivate : public RomDataPrivate
{
	public:
		NintendoDSPrivate(NintendoDS *q, IRpFile *file, bool cia);
		virtual ~NintendoDSPrivate();

	private:
		typedef RomDataPrivate super;
		RP_DISABLE_COPY(NintendoDSPrivate)

	public:
		// Animated icon data.
		// This class owns all of the icons in here, so we
		// must delete all of them.
		IconAnimData *iconAnimData;

		// Pointer to the first frame in iconAnimData.
		// Used when showing a static icon.
		const rp_image *icon_first_frame;

	public:
		/** RomFields **/

		// Hardware type. (RFT_BITFIELD)
		enum NDS_HWType {
			DS_HW_DS	= (1 << 0),
			DS_HW_DSi	= (1 << 1),
		};

		// DS region. (RFT_BITFIELD)
		enum NDS_Region {
			NDS_REGION_FREE		= (1 << 0),
			NDS_REGION_SKOREA	= (1 << 1),
			NDS_REGION_CHINA	= (1 << 2),
		};

	public:
		// ROM type.
		enum RomType {
			ROM_UNKNOWN	= -1,	// Unknown ROM type.
			ROM_NDS		= 0,	// Nintendo DS ROM.
			ROM_NDS_SLOT2	= 1,	// Nintendo DS ROM. (Slot-2)
			ROM_DSi_ENH	= 2,	// Nintendo DSi-enhanced ROM.
			ROM_DSi_ONLY	= 3,	// Nintendo DSi-only ROM.
		};

		// ROM type.
		int romType;

		// ROM header.
		// NOTE: Must be byteswapped on access.
		NDS_RomHeader romHeader;

		// Icon/title data from the ROM header.
		// NOTE: Must be byteswapped on access.
		NDS_IconTitleData nds_icon_title;
		bool nds_icon_title_loaded;

		// If true, this is an SRL in a 3DS CIA.
		// Some fields shouldn't be displayed.
		bool cia;

		/**
		 * Load the icon/title data.
		 * @return 0 on success; negative POSIX error code on error.
		 */
		int loadIconTitleData(void);

		/**
		 * Load the ROM image's icon.
		 * @return Icon, or nullptr on error.
		 */
		const rp_image *loadIcon(void);

		/**
		 * Get the title index.
		 * The title that most closely matches the
		 * host system language will be selected.
		 * @return Title index, or -1 on error.
		 */
		int getTitleIndex(void) const;

		/**
		 * Check the NDS Secure Area type.
		 * @return Secure area type.
		 */
		const rp_char *checkNDSSecureArea(void);

		/**
		 * Convert a Nintendo DS(i) region value to a GameTDB region code.
		 * @param ndsRegion Nintendo DS region.
		 * @param dsiRegion Nintendo DSi region.
		 * @param idRegion Game ID region.
		 *
		 * NOTE: Mulitple GameTDB region codes may be returned, including:
		 * - User-specified fallback region. [TODO]
		 * - General fallback region.
		 *
		 * @return GameTDB region code(s), or empty vector if the region value is invalid.
		 */
		static vector<const char*> ndsRegionToGameTDB(
			uint8_t ndsRegion, uint32_t dsiRegion, char idRegion);
};

/** NintendoDSPrivate **/

NintendoDSPrivate::NintendoDSPrivate(NintendoDS *q, IRpFile *file, bool cia)
	: super(q, file)
	, iconAnimData(nullptr)
	, icon_first_frame(nullptr)
	, romType(ROM_UNKNOWN)
	, nds_icon_title_loaded(false)
	, cia(cia)
{
	// Clear the various structs.
	memset(&romHeader, 0, sizeof(romHeader));
	memset(&nds_icon_title, 0, sizeof(nds_icon_title));
}

NintendoDSPrivate::~NintendoDSPrivate()
{
	if (iconAnimData) {
		for (int i = iconAnimData->count-1; i >= 0; i--) {
			delete iconAnimData->frames[i];
		}
		delete iconAnimData;
	}
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

	// Read the icon/title data.
	this->file->seek(icon_offset);
	size_t size = this->file->read(&nds_icon_title, sizeof(nds_icon_title));

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
		case NDS_ICON_VERSION_ZH:
			req_size = NDS_ICON_SIZE_ZH;
			break;
		case NDS_ICON_VERSION_ZH_KO:
			req_size = NDS_ICON_SIZE_ZH_KO;
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
	if ( le16_to_cpu(nds_icon_title.version) < NDS_ICON_VERSION_DSi ||
	    (le16_to_cpu(nds_icon_title.dsi_icon_seq[0]) & 0xFF) == 0)
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

		// Which bitmaps are used?
		std::array<bool, IconAnimData::MAX_FRAMES> bmp_used;
		bmp_used.fill(false);

		// Parse the icon sequence.
		int seq_idx;
		for (seq_idx = 0; seq_idx < ARRAY_SIZE(nds_icon_title.dsi_icon_seq); seq_idx++) {
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
			uint8_t bmp_pal_idx = ((seq >> 8) & 0x3F);
			bmp_used[bmp_pal_idx] = true;
			iconAnimData->seq_index[seq_idx] = bmp_pal_idx;
			iconAnimData->delays[seq_idx].numer = (uint16_t)delay;
			iconAnimData->delays[seq_idx].denom = 60;
			iconAnimData->delays[seq_idx].ms = delay * 1000 / 60;
		}
		iconAnimData->seq_count = seq_idx;

		// Convert the required bitmaps.
		for (int i = 0; i < (int)bmp_used.size(); i++) {
			if (bmp_used[i]) {
				iconAnimData->count = i + 1;

				const uint8_t bmp = (i & 7);
				const uint8_t pal = (i >> 3) & 7;
				iconAnimData->frames[i] = ImageDecoder::fromNDS_CI4(32, 32,
					nds_icon_title.dsi_icon_data[bmp],
					sizeof(nds_icon_title.dsi_icon_data[bmp]),
					nds_icon_title.dsi_icon_pal[pal],
					sizeof(nds_icon_title.dsi_icon_pal[pal]));
			}
		}
	}

	// NOTE: We're not deleting iconAnimData even if we only have
	// a single icon because iconAnimData() will call loadIcon()
	// if iconAnimData is nullptr.

	// Return a pointer to the first frame.
	icon_first_frame = iconAnimData->frames[iconAnimData->seq_index[0]];
	return icon_first_frame;
}

/**
 * Get the title index.
 * The title that most closely matches the
 * host system language will be selected.
 * @return Title index, or -1 on error.
 */
int NintendoDSPrivate::getTitleIndex(void) const
{
	if (!nds_icon_title_loaded) {
		// Attempt to load the icon/title data.
		if (const_cast<NintendoDSPrivate*>(this)->loadIconTitleData() != 0) {
			// Error loading the icon/title data.
			return -1;
		}

		// Make sure it was actually loaded.
		if (!nds_icon_title_loaded) {
			// Icon/title data was not loaded.
			return -1;
		}
	}

	// Version number check is required for ZH and KO.
	const uint16_t version = le16_to_cpu(nds_icon_title.version);

	int lang = -1;
	switch (SystemRegion::getLanguageCode()) {
		case 'en':
		default:
			lang = NDS_LANG_ENGLISH;
			break;
		case 'ja':
			lang = NDS_LANG_JAPANESE;
			break;
		case 'fr':
			lang = NDS_LANG_FRENCH;
			break;
		case 'de':
			lang = NDS_LANG_GERMAN;
			break;
		case 'it':
			lang = NDS_LANG_ITALIAN;
			break;
		case 'es':
			lang = NDS_LANG_SPANISH;
			break;
		case 'zh':
			if (version >= NDS_ICON_VERSION_ZH) {
				// NOTE: No distinction between
				// Simplified and Traditional Chinese...
				lang = NDS_LANG_CHINESE;
			} else {
				// No Chinese title here.
				lang = NDS_LANG_ENGLISH;
			}
			break;
		case 'ko':
			if (version >= NDS_ICON_VERSION_ZH_KO) {
				lang = NDS_LANG_KOREAN;
			} else {
				// No Korean title here.
				lang = NDS_LANG_ENGLISH;
			}
			break;
	}

	// Check that the field is valid.
	if (nds_icon_title.title[lang][0] == 0) {
		// Not valid. Check English.
		if (nds_icon_title.title[NDS_LANG_ENGLISH][0] != 0) {
			// English is valid.
			lang = NDS_LANG_ENGLISH;
		} else {
			// Not valid. Check Japanese.
			if (nds_icon_title.title[NDS_LANG_JAPANESE][0] != 0) {
				// Japanese is valid.
				lang = NDS_LANG_JAPANESE;
			} else {
				// Not valid...
				// TODO: Check other languages?
				lang = -1;
			}
		}
	}

	return lang;
}

/**
 * Check the NDS Secure Area type.
 * @return Secure area type, or nullptr if unknown.
 */
const rp_char *NintendoDSPrivate::checkNDSSecureArea(void)
{
	// Read the start of the Secure Area.
	uint32_t secure_area[2];
	int ret = file->seek(0x4000);
	if (ret != 0) {
		// Seek error.
		return nullptr;
	}
	size_t size = file->read(secure_area, sizeof(secure_area));
	if (size != sizeof(secure_area)) {
		// Read error.
		return nullptr;
	}

	// Reference: https://github.com/devkitPro/ndstool/blob/master/source/header.cpp#L39

	// Byteswap the Secure Area start.
#if SYS_BYTEORDER == SYS_BIG_ENDIAN
	secure_area[0] = le32_to_cpu(secure_area[0]);
	secure_area[1] = le32_to_cpu(secure_area[1]);
#endif

	const rp_char *secType = nullptr;
	bool needs_encryption = false;
	if (le32_to_cpu(romHeader.arm9.rom_offset) < 0x4000) {
		// ARM9 secure area is not present.
		// This is only valid for homebrew.
		secType = _RP("Homebrew");
	} else if (secure_area[0] == 0x00000000 && secure_area[1] == 0x00000000) {
		// Secure area is empty. (DS Download Play)
		secType = _RP("Multiboot");
	} else if (secure_area[0] == 0xE7FFDEFF && secure_area[1] == 0xE7FFDEFF) {
		// Secure area is decrypted.
		// Probably dumped using wooddumper or Decrypt9WIP.
		secType = _RP("Decrypted");
		needs_encryption = true;	// CRC requires encryption.
	} else {
		// Make sure 0x1000-0x3FFF is blank.
		// NOTE: ndstool checks 0x0200-0x0FFF, but this may
		// contain extra data for DSi-enhanced ROMs, or even
		// for regular DS games released after the DSi.
		uint32_t blank_area[0x3000/4];
		ret = file->seek(0x1000);
		if (ret != 0) {
			// Seek error.
			return nullptr;
		}
		size = file->read(blank_area, sizeof(blank_area));
		if (size != sizeof(blank_area)) {
			// Read error.
			return nullptr;
		}

		for (int i = ARRAY_SIZE(blank_area)-1; i >= 0; i--) {
			if (blank_area[i] != 0) {
				// Not zero. This area is not accessible
				// on the NDS, so it might be an original
				// mask ROM dump. Either that, or a Wii U
				// Virtual Console dump.
				secType = _RP("Mask ROM");
				break;
			}
		}
		if (!secType) {
			// Encrypted ROM dump.
			secType = _RP("Encrypted");
		}
	}

	// TODO: Verify the CRC?
	// For decrypted ROMs, this requires re-encrypting the secure area.
	return secType;
}

/**
 * Convert a Nintendo DS(i) region value to a GameTDB region code.
 * @param ndsRegion Nintendo DS region.
 * @param dsiRegion Nintendo DSi region.
 * @param idRegion Game ID region.
 *
 * NOTE: Mulitple GameTDB region codes may be returned, including:
 * - User-specified fallback region. [TODO]
 * - General fallback region.
 *
 * @return GameTDB region code(s), or empty vector if the region value is invalid.
 */
vector<const char*> NintendoDSPrivate::ndsRegionToGameTDB(
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
	vector<const char*> ret;

	int fallback_region = 0;
	switch (dsiRegion) {
		case DSi_REGION_JAPAN:
			ret.push_back("JA");
			return ret;
		case DSi_REGION_USA:
			ret.push_back("US");
			return ret;
		case DSi_REGION_EUROPE:
		case DSi_REGION_EUROPE | DSi_REGION_AUSTRALIA:
			// Process the game ID and use "EN" as a fallback.
			fallback_region = 1;
			break;
		case DSi_REGION_AUSTRALIA:
			// Process the game ID and use "AU","EN" as fallbacks.
			fallback_region = 2;
			break;
		case DSi_REGION_CHINA:
			ret.push_back("ZHCN");
			ret.push_back("JA");
			ret.push_back("EN");
			return ret;
		case DSi_REGION_SKOREA:
			ret.push_back("KO");
			ret.push_back("JA");
			ret.push_back("EN");
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
		ret.push_back("ZHCN");
		ret.push_back("JA");
		ret.push_back("EN");
		return ret;
	} else if (ndsRegion & NDS_REGION_SKOREA) {
		ret.push_back("KO");
		ret.push_back("JA");
		ret.push_back("EN");
		return ret;
	}

	// Check for region-specific game IDs.
	switch (idRegion) {
		case 'E':	// USA
			ret.push_back("US");
			break;
		case 'J':	// Japan
			ret.push_back("JA");
			break;
		case 'O':
			// TODO: US/EU.
			// Compare to host system region.
			// For now, assuming US.
			ret.push_back("US");
			break;
		case 'P':	// PAL
		case 'X':	// Multi-language release
		case 'Y':	// Multi-language release
		case 'L':	// Japanese import to PAL regions
		case 'M':	// Japanese import to PAL regions
		default:
			if (fallback_region == 0) {
				// Use the fallback region.
				fallback_region = 1;
			}
			break;

		// European regions.
		case 'D':	// Germany
			ret.push_back("DE");
			break;
		case 'F':	// France
			ret.push_back("FR");
			break;
		case 'H':	// Netherlands
			ret.push_back("NL");
			break;
		case 'I':	// Italy
			ret.push_back("NL");
			break;
		case 'R':	// Russia
			ret.push_back("RU");
			break;
		case 'S':	// Spain
			ret.push_back("ES");
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
			ret.push_back("EN");
			break;
		case 2:
			// Australia
			ret.push_back("AU");
			ret.push_back("EN");
			break;

		case 0:	// None
		default:
			break;
	}

	return ret;
}

/** NintendoDS **/

/**
 * Read a Nintendo DS ROM image.
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
NintendoDS::NintendoDS(IRpFile *file)
	: super(new NintendoDSPrivate(this, file, false))
{
	RP_D(NintendoDS);
	if (!d->file) {
		// Could not dup() the file handle.
		return;
	}

	init();
}

/**
 * Read a Nintendo DS ROM image.
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
 * @param cia If true, hide fields that aren't relevant to DSiWare in 3DS CIA packages.
 */
NintendoDS::NintendoDS(IRpFile *file, bool cia)
	: super(new NintendoDSPrivate(this, file, cia))
{
	RP_D(NintendoDS);
	if (!d->file) {
		// Could not dup() the file handle.
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
	if (size != sizeof(d->romHeader))
		return;

	// Check if this ROM image is supported.
	DetectInfo info;
	info.header.addr = 0;
	info.header.size = sizeof(d->romHeader);
	info.header.pData = reinterpret_cast<const uint8_t*>(&d->romHeader);
	info.ext = nullptr;	// Not needed for NDS.
	info.szFile = 0;	// Not needed for NDS.
	d->romType = isRomSupported_static(&info);
	d->isValid = (d->romType >= 0);
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
		return -1;
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
	    le16_to_cpu(romHeader->nintendo_logo_checksum) == 0xCF56) {
		// Nintendo logo is valid. (Slot-1)
		static const uint8_t nds_romType[] = {
			NintendoDSPrivate::ROM_NDS,		// 0x00 == Nintendo DS
			NintendoDSPrivate::ROM_NDS,		// 0x01 == invalid (assuming DS)
			NintendoDSPrivate::ROM_DSi_ENH,		// 0x02 == DSi-enhanced
			NintendoDSPrivate::ROM_DSi_ONLY,	// 0x03 == DSi-only
		};
		return nds_romType[romHeader->unitcode & 3];
	} else if (!memcmp(romHeader->nintendo_logo, nintendo_ds_logo_slot2, sizeof(nintendo_ds_logo_slot2)) &&
		   le16_to_cpu(romHeader->nintendo_logo_checksum) == 0x9E1A) {
		// Nintendo logo is valid. (Slot-2)
		// NOTE: Slot-2 is NDS only.
		return NintendoDSPrivate::ROM_NDS_SLOT2;
	}

	// Not supported.
	return -1;
}

/**
 * Is a ROM image supported by this object?
 * @param info DetectInfo containing ROM detection information.
 * @return Class-specific system ID (>= 0) if supported; -1 if not.
 */
int NintendoDS::isRomSupported(const DetectInfo *info) const
{
	return isRomSupported_static(info);
}

/**
 * Get the name of the system the loaded ROM is designed for.
 * @param type System name type. (See the SystemName enum.)
 * @return System name, or nullptr if type is invalid.
 */
const rp_char *NintendoDS::systemName(uint32_t type) const
{
	RP_D(const NintendoDS);
	if (!d->isValid || !isSystemNameTypeValid(type))
		return nullptr;

	// NDS/DSi are mostly the same worldwide, except for China.
	static_assert(SYSNAME_TYPE_MASK == 3,
		"NintendoDS::systemName() array index optimization needs to be updated.");
	static_assert(SYSNAME_REGION_MASK == (1 << 2),
		"NintendoDS::systemName() array index optimization needs to be updated.");

	// Bits 0-1: Type. (short, long, abbreviation)
	// Bit 2: 0 for NDS, 1 for DSi-exclusive.
	// Bit 3: 0 for worldwide, 1 for China. (iQue DS)
	static const rp_char *const sysNames[16] = {
		// Nintendo (worldwide)
		_RP("Nintendo DS"), _RP("Nintendo DS"), _RP("NDS"), nullptr,
		_RP("Nintendo DSi"), _RP("Nintendo DSi"), _RP("DSi"), nullptr,

		// iQue (China)
		_RP("iQue DS"), _RP("iQue DS"), _RP("NDS"), nullptr,
		_RP("iQue DSi"), _RP("iQue DSi"), _RP("DSi"), nullptr
	};

	uint32_t idx = (type & SYSNAME_TYPE_MASK);
	if ((d->romHeader.unitcode & 0x03) == 0x03) {
		// DSi-exclusive game.
		idx |= (1 << 2);
		if ((type & SYSNAME_REGION_MASK) == SYSNAME_REGION_ROM_LOCAL) {
			if ((le32_to_cpu(d->romHeader.dsi.region_code) & DSi_REGION_CHINA) ||
			    (d->romHeader.nds_region & 0x80))
			{
				// iQue DSi.
				idx |= (1 << 3);
			}
		}
	} else {
		// NDS-only and/or DSi-enhanced game.
		if ((type & SYSNAME_REGION_MASK) == SYSNAME_REGION_ROM_LOCAL) {
			if (d->romHeader.nds_region & 0x80) {
				// iQue DS.
				idx |= (1 << 3);
			}
		}
	}

	return sysNames[idx];
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
const rp_char *const *NintendoDS::supportedFileExtensions_static(void)
{
	static const rp_char *const exts[] = {
		_RP(".nds"),	// Nintendo DS
		_RP(".dsi"),	// Nintendo DSi (devkitARM r46)
		_RP(".srl"),	// Official SDK extension

		nullptr
	};
	return exts;
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
const rp_char *const *NintendoDS::supportedFileExtensions(void) const
{
	return supportedFileExtensions_static();
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
 * Get a bitfield of image types this class can retrieve.
 * @return Bitfield of supported image types. (ImageTypesBF)
 */
uint32_t NintendoDS::supportedImageTypes(void) const
{
	return supportedImageTypes_static();
}

/**
 * Get a list of all available image sizes for the specified image type.
 * @param imageType Image type.
 * @return Vector of available image sizes, or empty vector if no images are available.
 */
std::vector<RomData::ImageSizeDef> NintendoDS::supportedImageSizes_static(ImageType imageType)
{
	assert(imageType >= IMG_INT_MIN && imageType <= IMG_EXT_MAX);
	if (imageType < IMG_INT_MIN || imageType > IMG_EXT_MAX) {
		// ImageType is out of range.
		return std::vector<ImageSizeDef>();
	}

	switch (imageType) {
		case IMG_INT_ICON: {
			static const ImageSizeDef sz_INT_ICON[] = {
				{nullptr, 32, 32, 0},
			};
			return vector<ImageSizeDef>(sz_INT_ICON,
				sz_INT_ICON + ARRAY_SIZE(sz_INT_ICON));
		}
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
	return std::vector<ImageSizeDef>();
}

/**
 * Get a list of all available image sizes for the specified image type.
 * @param imageType Image type.
 * @return Vector of available image sizes, or empty vector if no images are available.
 */
std::vector<RomData::ImageSizeDef> NintendoDS::supportedImageSizes(ImageType imageType) const
{
	return supportedImageSizes_static(imageType);
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
	assert(imageType >= IMG_INT_MIN && imageType <= IMG_EXT_MAX);
	if (imageType < IMG_INT_MIN || imageType > IMG_EXT_MAX) {
		// ImageType is out of range.
		return 0;
	}

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
	if (d->fields->isDataLoaded()) {
		// Field data *has* been loaded...
		return 0;
	} else if (!d->file) {
		// File isn't open.
		return -EBADF;
	} else if (!d->isValid || d->romType < 0) {
		// ROM image isn't valid.
		return -EIO;
	}

	// Nintendo DS ROM header.
	const NDS_RomHeader *const romHeader = &d->romHeader;
	d->fields->reserve(12);	// Maximum of 11 fields.

	// Temporary buffer for snprintf().
	char buf[32];
	int len;

	// Type.
	// TODO:
	// - Show PassMe fields?
	//   Reference: http://imrannazar.com/The-Smallest-NDS-File
	// - Show IR cart and/or other accessories? (NAND ROM, etc.)
	const rp_char *nds_romType;
	switch (d->romType) {
		case NintendoDSPrivate::ROM_NDS_SLOT2:
			nds_romType = _RP("Slot-2 (PassMe)");
			break;
		default:
			nds_romType = _RP("Slot-1");
			break;
	}
	d->fields->addField_string(_RP("Type"), nds_romType);

	// Game title.
	d->fields->addField_string(_RP("Title"),
		latin1_to_rp_string(romHeader->title, ARRAY_SIZE(romHeader->title)));

	// Full game title.
	// TODO: Where should this go?
	int lang = d->getTitleIndex();
	if (lang >= 0 && lang < ARRAY_SIZE(d->nds_icon_title.title)) {
		d->fields->addField_string(_RP("Full Title"),
			utf16le_to_rp_string(d->nds_icon_title.title[lang],
				ARRAY_SIZE(d->nds_icon_title.title[lang])));
	}

	// Game ID and publisher.
	d->fields->addField_string(_RP("Game ID"),
		latin1_to_rp_string(romHeader->id6, ARRAY_SIZE(romHeader->id6)));

	// Look up the publisher.
	const rp_char *publisher = NintendoPublishers::lookup(romHeader->company);
	d->fields->addField_string(_RP("Publisher"),
		publisher ? publisher : _RP("Unknown"));

	// ROM version.
	d->fields->addField_string_numeric(_RP("Revision"),
		romHeader->rom_version, RomFields::FB_DEC, 2);

	// Secure Area.
	// TODO: Verify the CRC.
	const rp_char *secure_area = d->checkNDSSecureArea();
	if (secure_area) {
		d->fields->addField_string(_RP("Secure Area"), secure_area);
	} else {
		d->fields->addField_string(_RP("Secure Area"), _RP("Unknown"));
	}

	// Hardware type.
	// NOTE: DS_HW_DS is inverted bit0; DS_HW_DSi is normal bit1.
	uint32_t hw_type = (romHeader->unitcode & NintendoDSPrivate::DS_HW_DS) ^ NintendoDSPrivate::DS_HW_DS;
	hw_type |= (romHeader->unitcode & NintendoDSPrivate::DS_HW_DSi);
	if (hw_type == 0) {
		// 0x01 is invalid. Assume DS.
		hw_type = NintendoDSPrivate::DS_HW_DS;
	}

	if (!d->cia) {
		static const rp_char *const hw_bitfield_names[] = {
			_RP("Nintendo DS"), _RP("Nintendo DSi")
		};
		vector<rp_string> *v_hw_bitfield_names = RomFields::strArrayToVector(
			hw_bitfield_names, ARRAY_SIZE(hw_bitfield_names));
		d->fields->addField_bitfield(_RP("Hardware"),
			v_hw_bitfield_names, 0, hw_type);

		// NDS Region.
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

		static const rp_char *const nds_region_bitfield_names[] = {
			_RP("Region-Free"), _RP("South Korea"), _RP("China")
		};
		vector<rp_string> *v_nds_region_bitfield_names = RomFields::strArrayToVector(
			nds_region_bitfield_names, ARRAY_SIZE(nds_region_bitfield_names));
		d->fields->addField_bitfield(_RP("DS Region"),
			v_nds_region_bitfield_names, 0, nds_region);
	}

	if (hw_type & NintendoDSPrivate::DS_HW_DSi) {
		// DSi-specific fields.
		const rp_char *const region_code_name = (d->cia
				? _RP("Region Code")
				: _RP("DSi Region"));

		// DSi Region.
		// Maps directly to the header field.
		static const rp_char *const dsi_region_bitfield_names[] = {
			_RP("Japan"), _RP("USA"), _RP("Europe"),
			_RP("Australia"), _RP("China"), _RP("South Korea")
		};
		vector<rp_string> *v_dsi_region_bitfield_names = RomFields::strArrayToVector(
			dsi_region_bitfield_names, ARRAY_SIZE(dsi_region_bitfield_names));
		d->fields->addField_bitfield(region_code_name,
			v_dsi_region_bitfield_names, 3, le32_to_cpu(romHeader->dsi.region_code));

		// Title ID.
		len = snprintf(buf, sizeof(buf), "%08X-%08X",
			le32_to_cpu(romHeader->dsi.title_id.hi),
			le32_to_cpu(romHeader->dsi.title_id.lo));
		if (len > (int)sizeof(buf))
			len = sizeof(buf);
		d->fields->addField_string(_RP("Title ID"),
			len > 0 ? latin1_to_rp_string(buf, len) : _RP("Unknown"));

		// DSi filetype.
		const rp_char *filetype = nullptr;
		switch (romHeader->dsi.filetype) {
			case DSi_FTYPE_CARTRIDGE:
				filetype = _RP("Cartridge");
				break;
			case DSi_FTYPE_DSiWARE:
				filetype = _RP("DSiWare");
				break;
			case DSi_FTYPE_SYSTEM_FUN_TOOL:
				filetype = _RP("System Fun Tool");
				break;
			case DSi_FTYPE_NONEXEC_DATA:
				filetype = _RP("Non-Executable Data File");
				break;
			case DSi_FTYPE_SYSTEM_BASE_TOOL:
				filetype = _RP("System Base Tool");
				break;
			case DSi_FTYPE_SYSTEM_MENU:
				filetype = _RP("System Menu");
				break;
			default:
				break;
		}

		// TODO: Is the field name too long?
		if (filetype) {
			d->fields->addField_string(_RP("DSi ROM Type"), filetype);
		} else {
			// Invalid file type.
			len = snprintf(buf, sizeof(buf), "Unknown (0x%02X)", romHeader->dsi.filetype);
			if (len > (int)sizeof(buf))
				len = sizeof(buf);
			d->fields->addField_string(_RP("DSi ROM Type"),
				len > 0 ? latin1_to_rp_string(buf, len) : _RP(""));
		}

		// Age rating(s).
		// Note that not all 16 fields are present on DSi,
		// though the fields do match exactly, so no
		// mapping is necessary.
		RomFields::age_ratings_t age_ratings;
		// Valid ratings: 0-1, 3-9
		// TODO: Not sure if Finland is valid for DSi.
		static const uint16_t valid_ratings = 0x3FB;

		for (int i = (int)age_ratings.size()-1; i >= 0; i--) {
			if (!(valid_ratings & (1 << i))) {
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
		d->fields->addField_ageRatings(_RP("Age Rating"), age_ratings);
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
int NintendoDS::loadInternalImage(ImageType imageType, const rp_image **pImage)
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

	RP_D(NintendoDS);
	if (imageType != IMG_INT_ICON) {
		// Only IMG_INT_ICON is supported by DS.
		*pImage = nullptr;
		return -ENOENT;
	} else if (d->icon_first_frame) {
		// Image has already been loaded.
		*pImage = d->icon_first_frame;
		return 0;
	} else if (!d->file) {
		// File isn't open.
		*pImage = nullptr;
		return -EBADF;
	} else if (!d->isValid || d->romType < 0) {
		// ROM image isn't valid.
		*pImage = nullptr;
		return -EIO;
	}

	// Load the icon.
	*pImage = d->loadIcon();
	return (*pImage != nullptr ? 0 : -EIO);
}

/**
 * Get the animated icon data.
 *
 * Check imgpf for IMGPF_ICON_ANIMATED first to see if this
 * object has an animated icon.
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
	assert(imageType >= IMG_EXT_MIN && imageType <= IMG_EXT_MAX);
	if (imageType < IMG_EXT_MIN || imageType > IMG_EXT_MAX) {
		// ImageType is out of range.
		return -ERANGE;
	}
	assert(pExtURLs != nullptr);
	if (!pExtURLs) {
		// No vector.
		return -EINVAL;
	}
	pExtURLs->clear();

	// Check for DS ROMs that don't have boxart.
	RP_D(const NintendoDS);
	if (!memcmp(d->romHeader.id4, "NTRJ", 4) ||
	    !memcmp(d->romHeader.id4, "####", 4))
	{
		// This is either a prototype, a download demo, or homebrew.
		// No external images are available.
		return -ENOENT;
	} else if ((d->romHeader.unitcode & NintendoDSPrivate::DS_HW_DSi) &&
		d->romHeader.dsi.filetype != DSi_FTYPE_CARTRIDGE)
	{
		// This is a DSi SRL that isn't a cartridge dump.
		// No external images are available.
		return -ENOENT;
	}

	if (!d->file || !d->file->isOpen()) {
		// File isn't open.
		return -EBADF;
	} else if (!d->isValid || d->romType < 0) {
		// ROM image isn't valid.
		return -EIO;
	}

	// Get the image sizes and sort them based on the
	// requested image size.
	vector<ImageSizeDef> sizeDefs = supportedImageSizes(imageType);
	if (sizeDefs.empty()) {
		// No image sizes.
		return -ENOENT;
	}

	// Select the best size.
	const ImageSizeDef *sizeDef = d->selectBestSize(sizeDefs, size);
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

	// Determine the GameTDB region code(s).
	vector<const char*> tdb_regions = d->ndsRegionToGameTDB(
		d->romHeader.nds_region,
		((d->romHeader.unitcode & NintendoDSPrivate::DS_HW_DSi)
			? le32_to_cpu(d->romHeader.dsi.region_code)
			: 0 /* not a DSi-enhanced/exclusive ROM */
			),
		d->romHeader.id4[3]);

	// Game ID. (GameTDB uses ID4 for Nintendo DS.)
	// The ID4 cannot have non-printable characters.
	char id4[5];
	for (int i = ARRAY_SIZE(d->romHeader.id4)-1; i >= 0; i--) {
		if (!isprint(d->romHeader.id4[i])) {
			// Non-printable character found.
			return -ENOENT;
		}
		id4[i] = d->romHeader.id4[i];
	}
	// NULL-terminated ID4 is needed for the
	// GameTDB URL functions.
	id4[4] = 0;

	// If we're downloading a "high-resolution" image (M or higher),
	// also add the default image to ExtURLs in case the user has
	// high-resolution image downloads disabled.
	const ImageSizeDef *szdefs_dl[3];
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
	pExtURLs->reserve(4*szdef_count);
	for (unsigned int i = 0; i < szdef_count; i++) {
		// Current image type.
		char imageTypeName[16];
		snprintf(imageTypeName, sizeof(imageTypeName), "%s%s",
			 imageTypeName_base, (szdefs_dl[i]->name ? szdefs_dl[i]->name : ""));

		// Add the images.
		for (auto iter = tdb_regions.cbegin(); iter != tdb_regions.cend(); ++iter) {
			int idx = (int)pExtURLs->size();
			pExtURLs->resize(idx+1);
			auto &extURL = pExtURLs->at(idx);

			extURL.url = d->getURL_GameTDB("ds", imageTypeName, *iter, id4, ext);
			extURL.cache_key = d->getCacheKey_GameTDB("ds", imageTypeName, *iter, id4, ext);
			extURL.width = szdefs_dl[i]->width;
			extURL.height = szdefs_dl[i]->height;
			extURL.high_res = (szdefs_dl[i]->index >= 2);
		}
	}

	// All URLs added.
	return 0;
}

}
