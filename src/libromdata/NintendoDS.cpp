/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * NintendoDS.hpp: Nintendo DS(i) ROM reader.                              *
 *                                                                         *
 * Copyright (c) 2016 by David Korth.                                      *
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
		NintendoDSPrivate(NintendoDS *q, IRpFile *file);
		virtual ~NintendoDSPrivate();

	private:
		typedef RomDataPrivate super;
		NintendoDSPrivate(const NintendoDSPrivate &other);
		NintendoDSPrivate &operator=(const NintendoDSPrivate &other);

	public:
		/** RomFields **/

		// Hardware type. (RFT_BITFIELD)
		enum NDS_HWType {
			DS_HW_DS	= (1 << 0),
			DS_HW_DSi	= (1 << 1),
		};
		static const rp_char *nds_hw_bitfield_names[];
		static const RomFields::BitfieldDesc nds_hw_bitfield;

		// DS region. (RFT_BITFIELD)
		enum NDS_Region {
			NDS_REGION_FREE		= (1 << 0),
			NDS_REGION_SKOREA	= (1 << 1),
			NDS_REGION_CHINA	= (1 << 2),
		};
		static const rp_char *const nds_region_bitfield_names[];
		static const RomFields::BitfieldDesc nds_region_bitfield;

		// DSi region. (RFT_BITFIELD)
		static const rp_char *const dsi_region_bitfield_names[];
		static const RomFields::BitfieldDesc dsi_region_bitfield;

		// ROM fields.
		static const struct RomFields::Desc nds_fields[];

	public:
		// ROM header.
		// NOTE: Must be byteswapped on access.
		NDS_RomHeader romHeader;

		// Icon/title data from the ROM header.
		// NOTE: Must be byteswapped on access.
		NDS_IconTitleData nds_icon_title;
		bool nds_icon_title_loaded;

		// Animated icon data.
		// NOTE: Nintendo DSi icons can have custom sequences,
		// so the first frame isn't necessarily the first in
		// the sequence. Hence, we return a copy of the first
		// frame in the sequence for loadIcon().
		// This class owns all of the icons in here, so we
		// must delete all of them.
		IconAnimData *iconAnimData;

		// The rp_image* copy. (DO NOT DELETE THIS!)
		rp_image *icon_first_frame;

		/**
		 * Load the icon/title data.
		 * @return 0 on success; negative POSIX error code on error.
		 */
		int loadIconTitleData(void);

		/**
		 * Load the ROM image's icon.
		 * @return Icon, or nullptr on error.
		 */
		rp_image *loadIcon(void);

		/**
		 * Get the title index.
		 * The title that most closely matches the
		 * host system language will be selected.
		 * @return Title index, or -1 on error.
		 */
		int getTitleIndex(void) const;
};

/** NintendoDSPrivate **/

// Hardware bitfield.
const rp_char *NintendoDSPrivate::nds_hw_bitfield_names[] = {
	_RP("Nintendo DS"), _RP("Nintendo DSi")
};

const RomFields::BitfieldDesc NintendoDSPrivate::nds_hw_bitfield = {
	ARRAY_SIZE(nds_hw_bitfield_names), 2, nds_hw_bitfield_names
};

// DS region bitfield.
const rp_char *const NintendoDSPrivate::nds_region_bitfield_names[] = {
	_RP("Region-Free"), _RP("South Korea"), _RP("China")
};

const RomFields::BitfieldDesc NintendoDSPrivate::nds_region_bitfield = {
	ARRAY_SIZE(nds_region_bitfield_names), 3, nds_region_bitfield_names
};

// DSi region bitfield.
const rp_char *const NintendoDSPrivate::dsi_region_bitfield_names[] = {
	_RP("Japan"), _RP("USA"), _RP("Europe"),
	_RP("Australia"), _RP("China"), _RP("South Korea")
};

const RomFields::BitfieldDesc NintendoDSPrivate::dsi_region_bitfield = {
	ARRAY_SIZE(dsi_region_bitfield_names), 3, dsi_region_bitfield_names
};

// ROM fields.
const struct RomFields::Desc NintendoDSPrivate::nds_fields[] = {
	{_RP("Title"), RomFields::RFT_STRING, {nullptr}},
	{_RP("Full Title"), RomFields::RFT_STRING, {nullptr}},
	{_RP("Game ID"), RomFields::RFT_STRING, {nullptr}},
	{_RP("Publisher"), RomFields::RFT_STRING, {nullptr}},
	{_RP("Revision"), RomFields::RFT_STRING, {nullptr}},
	{_RP("Hardware"), RomFields::RFT_BITFIELD, {&nds_hw_bitfield}},
	{_RP("DS Region"), RomFields::RFT_BITFIELD, {&nds_region_bitfield}},
	{_RP("DSi Region"), RomFields::RFT_BITFIELD, {&dsi_region_bitfield}},
	// TODO: Is the field name too long?
	{_RP("DSi ROM Type"), RomFields::RFT_STRING, {nullptr}},
};

NintendoDSPrivate::NintendoDSPrivate(NintendoDS *q, IRpFile *file)
	: super(q, file, nds_fields, ARRAY_SIZE(nds_fields))
	, nds_icon_title_loaded(false)
	, iconAnimData(nullptr)
	, icon_first_frame(nullptr)
{
	// Clear the various structs.
	memset(&romHeader, 0, sizeof(romHeader));
	memset(&nds_icon_title, 0, sizeof(nds_icon_title));
}

NintendoDSPrivate::~NintendoDSPrivate()
{
	if (iconAnimData) {
		// NOTE: Nintendo DSi icons can have custom sequences,
		// so the first frame isn't necessarily the first in
		// the sequence. Hence, we return a copy of the first
		// frame in the sequence for loadIcon().
		// This class owns all of the icons in here, so we
		// must delete all of them.
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
	switch (nds_icon_title.version) {
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
rp_image *NintendoDSPrivate::loadIcon(void)
{
	if (!this->file || !this->isValid) {
		// Can't load the icon.
		return nullptr;
	}

	if (iconAnimData) {
		// Icon has already been loaded.
		return icon_first_frame;
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
		bool bmp_used[IconAnimData::MAX_FRAMES];
		memset(bmp_used, 0, sizeof(bmp_used));

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
		for (int i = 0; i < IconAnimData::MAX_FRAMES; i++) {
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

	// Return a copy of first frame.
	// TODO: rp_image assignment operator and copy constructor.
	const rp_image *src_img = iconAnimData->frames[iconAnimData->seq_index[0]];
	if (src_img) {
		icon_first_frame = new rp_image(32, 32, rp_image::FORMAT_CI8);

		// Copy the image data.
		// TODO: Image stride?
		assert(icon_first_frame->data_len() == src_img->data_len());
		const size_t data_len = std::min(icon_first_frame->data_len(), src_img->data_len());
		memcpy(icon_first_frame->bits(), src_img->bits(), data_len);

		// Copy the palette.
		assert(icon_first_frame->palette_len() > 0);
		assert(src_img->palette_len() > 0);
		assert(icon_first_frame->palette_len() == src_img->palette_len());
		const int palette_len = std::min(icon_first_frame->palette_len(), src_img->palette_len());
		if (palette_len > 0) {
			memcpy(icon_first_frame->palette(), src_img->palette(), palette_len);
		}
	}

	return icon_first_frame;
}

/** NintendoDS **/

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
	const uint16_t version = nds_icon_title.version;

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
	: super(new NintendoDSPrivate(this, file))
{
	RP_D(NintendoDS);
	if (!d->file) {
		// Could not dup() the file handle.
		return;
	}

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
	d->isValid = (isRomSupported_static(&info) >= 0);
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

	// TODO: Detect DS vs. DSi and return the system ID.

	// Check the first 16 bytes of the Nintendo logo.
	static const uint8_t nintendo_gba_logo[16] = {
		0x24, 0xFF, 0xAE, 0x51, 0x69, 0x9A, 0xA2, 0x21,
		0x3D, 0x84, 0x82, 0x0A, 0x84, 0xE4, 0x09, 0xAD
	};

	const NDS_RomHeader *const romHeader =
		reinterpret_cast<const NDS_RomHeader*>(info->header.pData);
	if (!memcmp(romHeader->nintendo_logo, nintendo_gba_logo, sizeof(nintendo_gba_logo))) {
		// Nintendo logo is present at the correct location.
		// TODO: DS vs. DSi?
		return 0;
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
			if ((d->romHeader.dsi.region_code & DSi_REGION_CHINA) ||
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
 * NOTE 2: The strings in the std::vector should *not*
 * be freed by the caller.
 *
 * @return List of all supported file extensions.
 */
vector<const rp_char*> NintendoDS::supportedFileExtensions_static(void)
{
	static const rp_char *const exts[] = {
		_RP(".nds"),
		_RP(".dsi"),
	};
	return vector<const rp_char*>(exts, exts + ARRAY_SIZE(exts));
}

/**
 * Get a list of all supported file extensions.
 * This is to be used for file type registration;
 * subclasses don't explicitly check the extension.
 *
 * NOTE: The extensions include the leading dot,
 * e.g. ".bin" instead of "bin".
 *
 * NOTE 2: The strings in the std::vector should *not*
 * be freed by the caller.
 *
 * @return List of all supported file extensions.
 */
vector<const rp_char*> NintendoDS::supportedFileExtensions(void) const
{
	return supportedFileExtensions_static();
}

/**
 * Get a bitfield of image types this class can retrieve.
 * @return Bitfield of supported image types. (ImageTypesBF)
 */
uint32_t NintendoDS::supportedImageTypes_static(void)
{
	return IMGBF_INT_ICON;
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
	} else if (!d->isValid) {
		// ROM image isn't valid.
		return -EIO;
	}

	// Nintendo DS ROM header.
	const NDS_RomHeader *const romHeader = &d->romHeader;

	// Game title.
	d->fields->addData_string(latin1_to_rp_string(romHeader->title, sizeof(romHeader->title)));

	// Full game title.
	// TODO: Where should this go?
	int lang = d->getTitleIndex();
	if (lang >= 0 && lang < ARRAY_SIZE(d->nds_icon_title.title)) {
		d->fields->addData_string(
			utf16le_to_rp_string(d->nds_icon_title.title[lang], sizeof(d->nds_icon_title.title[lang])));
	} else {
		// Full game title is not available.
		d->fields->addData_invalid();
	}

	// Game ID and publisher.
	d->fields->addData_string(latin1_to_rp_string(romHeader->id6, sizeof(romHeader->id6)));

	// Look up the publisher.
	const rp_char *publisher = NintendoPublishers::lookup(romHeader->company);
	d->fields->addData_string(publisher ? publisher : _RP("Unknown"));

	// ROM version.
	d->fields->addData_string_numeric(romHeader->rom_version, RomFields::FB_DEC, 2);

	// Hardware type.
	// NOTE: DS_HW_DS is inverted bit0; DS_HW_DSi is normal bit1.
	uint32_t hw_type = (romHeader->unitcode & NintendoDSPrivate::DS_HW_DS) ^ NintendoDSPrivate::DS_HW_DS;
	hw_type |= (romHeader->unitcode & NintendoDSPrivate::DS_HW_DSi);
	if (hw_type == 0) {
		// 0x01 is invalid. Assume DS.
		hw_type = NintendoDSPrivate::DS_HW_DS;
	}
	d->fields->addData_bitfield(hw_type);

	// TODO: Combine DS Region and DSi Region into one bitfield?

	// DS Region.
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
	d->fields->addData_bitfield(nds_region);

	if (hw_type & NintendoDSPrivate::DS_HW_DSi) {
		// DSi-specific fields.

		// DSi Region.
		// Maps directly to the header field.
		d->fields->addData_bitfield(romHeader->dsi.region_code);

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

		if (filetype) {
			d->fields->addData_string(filetype);
		} else {
			// Invalid file type.
			char buf[24];
			int len = snprintf(buf, sizeof(buf), "Unknown (0x%02X)", romHeader->dsi.filetype);
			if (len > (int)sizeof(buf))
				len = sizeof(buf);
			d->fields->addData_string(len > 0 ? latin1_to_rp_string(buf, len) : _RP(""));
		}
	} else {
		// Hide the DSi-specific fields.
		d->fields->addData_invalid();
		d->fields->addData_invalid();
	}

	// Finished reading the field data.
	return (int)d->fields->count();
}

/**
 * Load an internal image.
 * Called by RomData::image() if the image data hasn't been loaded yet.
 * @param imageType Image type to load.
 * @return 0 on success; negative POSIX error code on error.
 */
int NintendoDS::loadInternalImage(ImageType imageType)
{
	assert(imageType >= IMG_INT_MIN && imageType <= IMG_INT_MAX);
	if (imageType < IMG_INT_MIN || imageType > IMG_INT_MAX) {
		// ImageType is out of range.
		return -ERANGE;
	}

	RP_D(NintendoDS);
	if (d->images[imageType]) {
		// Icon *has* been loaded...
		return 0;
	} else if (!d->file) {
		// File isn't open.
		return -EBADF;
	} else if (!d->isValid) {
		// ROM image isn't valid.
		return -EIO;
	}

	// Check for supported image types.
	if (imageType != IMG_INT_ICON) {
		// Only IMG_INT_ICON is supported by DS.
		return -ENOENT;
	}

	// Use nearest-neighbor scaling when resizing.
	d->imgpf[imageType] = IMGPF_RESCALE_NEAREST;
	d->images[imageType] = d->loadIcon();
	if (d->iconAnimData && d->iconAnimData->count > 1) {
		// Animated icon.
		d->imgpf[imageType] |= IMGPF_ICON_ANIMATED;
	}

	// TODO: -ENOENT if the file doesn't actually have an icon.
	return (d->images[imageType] != nullptr ? 0 : -EIO);
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

}
