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
#include "NintendoPublishers.hpp"
#include "nds_structs.h"

#include "common.h"
#include "byteswap.h"
#include "TextFuncs.hpp"
#include "file/IRpFile.hpp"
#include "img/rp_image.hpp"
#include "img/ImageDecoder.hpp"

// C includes. (C++ namespace)
#include <cassert>
#include <cstring>
#include <cstddef>

// C++ includes.
#include <vector>
using std::vector;

namespace LibRomData {

class NintendoDSPrivate
{
	public:
		explicit NintendoDSPrivate(NintendoDS *q);
		~NintendoDSPrivate();

	private:
		NintendoDSPrivate(const NintendoDSPrivate &other);
		NintendoDSPrivate &operator=(const NintendoDSPrivate &other);
	private:
		NintendoDS *const q;

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
		enum DSi_Region {
			DSi_REGION_JAPAN	= (1 << 0),
			DSi_REGION_USA		= (1 << 1),
			DSi_REGION_EUROPE	= (1 << 2),
			DSi_REGION_AUSTRALIA	= (1 << 3),
			DSi_REGION_CHINA	= (1 << 4),
			DSi_REGION_SKOREA	= (1 << 5),
		};
		static const rp_char *const dsi_region_bitfield_names[];
		static const RomFields::BitfieldDesc dsi_region_bitfield;

		// ROM fields.
		static const struct RomFields::Desc nds_fields[];

	public:
		// ROM header.
		NDS_RomHeader romHeader;

		// Animated icon data.
		// NOTE: The first frame is owned by the RomData superclass.
		RomData::IconAnimData *iconAnimData;

		/**
		 * Load the ROM image's icon.
		 * @return Icon, or nullptr on error.
		 */
		rp_image *loadIcon(void);
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
	// TODO: Banner?
	{_RP("Title"), RomFields::RFT_STRING, {nullptr}},
	{_RP("Game ID"), RomFields::RFT_STRING, {nullptr}},
	{_RP("Publisher"), RomFields::RFT_STRING, {nullptr}},
	{_RP("Revision"), RomFields::RFT_STRING, {nullptr}},
	{_RP("Hardware"), RomFields::RFT_BITFIELD, {&nds_hw_bitfield}},
	{_RP("DS Region"), RomFields::RFT_BITFIELD, {&nds_region_bitfield}},
	{_RP("DSi Region"), RomFields::RFT_BITFIELD, {&dsi_region_bitfield}},

	// TODO: Icon, full game title.
};

NintendoDSPrivate::NintendoDSPrivate(NintendoDS *q)
	: q(q)
	, iconAnimData(nullptr)
{ }

NintendoDSPrivate::~NintendoDSPrivate()
{
	if (iconAnimData) {
		// Delete all except the first animated icon frame.
		// (The first frame is owned by the RomData superclass.)
		for (int i = iconAnimData->count-1; i >= 1; i--) {
			delete iconAnimData->frames[i];
		}
		delete iconAnimData;
	}
}

/**
 * Load the ROM image's icon.
 * @return Icon, or nullptr on error.
 */
rp_image *NintendoDSPrivate::loadIcon(void)
{
	if (!q->m_file || !q->m_isValid) {
		// Can't load the icon.
		return nullptr;
	}

	if (iconAnimData) {
		// Icon has already been loaded.
		return const_cast<rp_image*>(iconAnimData->frames[0]);
	}

	// Get the address of the icon/title information.
	const uint32_t icon_offset = le32_to_cpu(romHeader.icon_offset);

	// Read the icon data.
	// TODO: Also store titles?
	static_assert(sizeof(NDS_IconTitleData) == NDS_IconTitleData_SIZE,
		      "NDS_IconTitleData is not 9,152 bytes.");
	NDS_IconTitleData nds_icon_title;
	q->m_file->seek(icon_offset);
	size_t size = q->m_file->read(&nds_icon_title, sizeof(nds_icon_title));
	// Make sure we have up to the icon plus two titles.
	if (size < (offsetof(NDS_IconTitleData, title) + 0x200)) {
		// Error reading the icon data.
		return nullptr;
	}

	// Check if a DSi animated icon is present.
	if (size < sizeof(NDS_IconTitleData) ||
	    le16_to_cpu(nds_icon_title.version) < NDS_ICON_VERSION_DSi ||
	    le16_to_cpu(nds_icon_title.dsi_icon_seq[0]) == 0)
	{
		// Either this isn't a DSi icon/title struct (pre-v0103),
		// or the animated icon sequence is invalid.

		// Convert the NDS icon to rp_image.
		return ImageDecoder::fromNDS_CI4(32, 32,
			nds_icon_title.icon_data, sizeof(nds_icon_title.icon_data),
			nds_icon_title.icon_pal,  sizeof(nds_icon_title.icon_pal));
	}

	// FIXME: DSi supports a lot of wacky stuff:
	// - Each sequence token includes Hflip/Vflip.
	// - Palettes and bitmaps can be mixed and matched.
	// For now, we'll assume one palette per bitmap.
	this->iconAnimData = new RomData::IconAnimData();
	iconAnimData->count = 0;

	// Which bitmaps are used?
	bool bmp_used[RomData::ICONANIMDATA_MAX_FRAMES];
	memset(bmp_used, 0, sizeof(bmp_used));

	// Parse the icon sequence.
	int seq_idx;
	for (seq_idx = 0; seq_idx < ARRAY_SIZE(nds_icon_title.dsi_icon_seq); seq_idx++) {
		const uint16_t seq = le16_to_cpu(nds_icon_title.dsi_icon_seq[seq_idx]);
		if (seq == 0) {
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
		iconAnimData->delays[seq_idx] = (seq & 0xFF) * 1000 / 60;
	}
	iconAnimData->seq_count = seq_idx;

	// Convert the required bitmaps.
	for (int i = 0; i < RomData::ICONANIMDATA_MAX_FRAMES; i++) {
		if (bmp_used[i]) {
			iconAnimData->count = i;

			const uint8_t bmp = (i & 7);
			const uint8_t pal = (i >> 3) & 7;
			iconAnimData->frames[i] = ImageDecoder::fromNDS_CI4(32, 32,
				nds_icon_title.dsi_icon_data[bmp],
				sizeof(nds_icon_title.dsi_icon_data[bmp]),
				nds_icon_title.dsi_icon_pal[pal],
				sizeof(nds_icon_title.dsi_icon_pal[pal]));
		}
	}

	// Return the first frame.
	return const_cast<rp_image*>(iconAnimData->frames[0]);
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
	: RomData(file, NintendoDSPrivate::nds_fields, ARRAY_SIZE(NintendoDSPrivate::nds_fields))
	, d(new NintendoDSPrivate(this))
{
	if (!m_file) {
		// Could not dup() the file handle.
		return;
	}

	// Read the ROM header.
	static_assert(sizeof(NDS_RomHeader) == NDS_RomHeader_SIZE,
		"NDS_RomHeader is not 4,096 bytes.");
	NDS_RomHeader header;
	m_file->rewind();
	size_t size = m_file->read(&header, sizeof(header));
	if (size != sizeof(header))
		return;

	// Check if this ROM image is supported.
	DetectInfo info;
	info.pHeader = reinterpret_cast<const uint8_t*>(&header);
	info.szHeader = sizeof(header);
	info.ext = nullptr;	// Not needed for NDS.
	info.szFile = 0;	// Not needed for NDS.
	m_isValid = (isRomSupported(&info) >= 0);

	if (m_isValid) {
		// Save the header for later.
		memcpy(&d->romHeader, &header, sizeof(d->romHeader));
	}
}

NintendoDS::~NintendoDS()
{
	delete d;
}

/**
 * Is a ROM image supported by this class?
 * @param info DetectInfo containing ROM detection information.
 * @return Class-specific system ID (>= 0) if supported; -1 if not.
 */
int NintendoDS::isRomSupported_static(const DetectInfo *info)
{
	if (!info || info->szHeader < sizeof(NDS_RomHeader)) {
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

	const NDS_RomHeader *romHeader =
		reinterpret_cast<const NDS_RomHeader*>(info->pHeader);
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
	if (!m_isValid || !isSystemNameTypeValid(type))
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
			if ((d->romHeader.dsi_region & NintendoDSPrivate::DSi_REGION_CHINA) ||
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
	vector<const rp_char*> ret;
	ret.reserve(1);
	ret.push_back(_RP(".nds"));
	return ret;
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
	if (m_fields->isDataLoaded()) {
		// Field data *has* been loaded...
		return 0;
	} else if (!m_file) {
		// File isn't open.
		return -EBADF;
	} else if (!m_isValid) {
		// ROM image isn't valid.
		return -EIO;
	}

	// Nintendo DS ROM header.
	const NDS_RomHeader *romHeader = &d->romHeader;

	// Game title.
	m_fields->addData_string(latin1_to_rp_string(romHeader->title, sizeof(romHeader->title)));

	// Game ID and publisher.
	m_fields->addData_string(latin1_to_rp_string(romHeader->id6, sizeof(romHeader->id6)));

	// Look up the publisher.
	const rp_char *publisher = NintendoPublishers::lookup(romHeader->company);
	m_fields->addData_string(publisher ? publisher : _RP("Unknown"));

	// ROM version.
	m_fields->addData_string_numeric(romHeader->rom_version, RomFields::FB_DEC, 2);

	// Hardware type.
	// NOTE: DS_HW_DS is inverted bit0; DS_HW_DSi is normal bit1.
	uint32_t hw_type = (romHeader->unitcode & NintendoDSPrivate::DS_HW_DS) ^ NintendoDSPrivate::DS_HW_DS;
	hw_type |= (romHeader->unitcode & NintendoDSPrivate::DS_HW_DSi);
	if (hw_type == 0) {
		// 0x01 is invalid. Assume DS.
		hw_type = NintendoDSPrivate::DS_HW_DS;
	}
	m_fields->addData_bitfield(hw_type);

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
	m_fields->addData_bitfield(nds_region);

	// DSi Region.
	// Maps directly to the header field.
	if (hw_type & NintendoDSPrivate::DS_HW_DSi) {
		m_fields->addData_bitfield(romHeader->dsi_region);
	} else {
		// No DSi region.
		m_fields->addData_invalid();
	}

	// Finished reading the field data.
	return (int)m_fields->count();
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
	if (m_images[imageType]) {
		// Icon *has* been loaded...
		return 0;
	} else if (!m_file) {
		// File isn't open.
		return -EBADF;
	} else if (!m_isValid) {
		// ROM image isn't valid.
		return -EIO;
	}

	// Check for supported image types.
	if (imageType != IMG_INT_ICON) {
		// Only IMG_INT_ICON is supported by DS.
		return -ENOENT;
	}

	// Use nearest-neighbor scaling when resizing.
	m_imgpf[imageType] = IMGPF_RESCALE_NEAREST;
	m_images[imageType] = d->loadIcon();
	if (d->iconAnimData && d->iconAnimData->count > 1) {
		// Animated icon.
		m_imgpf[imageType] |= IMGPF_ICON_ANIMATED;
	}

	// TODO: -ENOENT if the file doesn't actually have an icon.
	return (m_images[imageType] != nullptr ? 0 : -EIO);
}

/**
 * Get the animated icon data.
 *
 * Check imgpf for IMGPF_ICON_ANIMATED first to see if this
 * object has an animated icon.
 *
 * @return Animated icon data, or nullptr if no animated icon is present.
 */
const RomData::IconAnimData *NintendoDS::iconAnimData(void) const
{
	if (!d->iconAnimData) {
		// Load the icon.
		if (!d->loadIcon()) {
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
