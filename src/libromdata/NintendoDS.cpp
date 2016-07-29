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
#include "TextFuncs.hpp"
#include "byteswap.h"
#include "common.h"

// rp_image for internal icon.
#include "rp_image.hpp"

// C includes. (C++ namespace)
#include <cassert>
#include <cstring>

// C++ includes.
#include <vector>
using std::vector;

namespace LibRomData {

// Hardware bitfield.
static const rp_char *nds_hw_bitfield_names[] = {
	_RP("Nintendo DS"), _RP("Nintendo DSi")
};

enum NDS_HWType {
	DS_HW_DS	= (1 << 0),
	DS_HW_DSi	= (1 << 1),
};

static const RomFields::BitfieldDesc nds_hw_bitfield = {
	ARRAY_SIZE(nds_hw_bitfield_names), 2, nds_hw_bitfield_names
};

// DS region bitfield.
static const rp_char *nds_region_bitfield_names[] = {
	_RP("Region-Free"), _RP("South Korea"), _RP("China")
};

enum NDS_Region {
	NDS_REGION_FREE		= (1 << 0),
	NDS_REGION_SKOREA	= (1 << 1),
	NDS_REGION_CHINA	= (1 << 2),
};

static const RomFields::BitfieldDesc nds_region_bitfield = {
	ARRAY_SIZE(nds_region_bitfield_names), 3, nds_region_bitfield_names
};

// DSi region bitfield.
static const rp_char *dsi_region_bitfield_names[] = {
	_RP("Japan"), _RP("USA"), _RP("Europe"),
	_RP("Australia"), _RP("China"), _RP("South Korea")
};

enum DSi_Region {
	DSi_REGION_JAPAN	= (1 << 0),
	DSi_REGION_USA		= (1 << 1),
	DSi_REGION_EUROPE	= (1 << 2),
	DSi_REGION_AUSTRALIA	= (1 << 3),
	DSi_REGION_CHINA	= (1 << 4),
	DSi_REGION_SKOREA	= (1 << 5),
};

static const RomFields::BitfieldDesc dsi_region_bitfield = {
	ARRAY_SIZE(dsi_region_bitfield_names), 3, dsi_region_bitfield_names
};

// ROM fields.
// TODO: Private class?
static const struct RomFields::Desc nds_fields[] = {
	// TODO: Banner?
	{_RP("Title"), RomFields::RFT_STRING, nullptr},
	{_RP("Game ID"), RomFields::RFT_STRING, nullptr},
	{_RP("Publisher"), RomFields::RFT_STRING, nullptr},
	{_RP("Revision"), RomFields::RFT_STRING, nullptr},
	{_RP("Hardware"), RomFields::RFT_BITFIELD, &nds_hw_bitfield},
	{_RP("DS Region"), RomFields::RFT_BITFIELD, &nds_region_bitfield},
	{_RP("DSi Region"), RomFields::RFT_BITFIELD, &dsi_region_bitfield},

	// TODO: Icon, full game title.
};

/**
 * Nintendo DS ROM header.
 * This matches the ROM header format exactly.
 * Reference: http://problemkaputt.de/gbatek.htm#dscartridgeheader
 * 
 * All fields are little-endian.
 * NOTE: Strings are NOT null-terminated!
 */
#pragma pack(1)
struct PACKED DS_ROMHeader {
	char title[12];
	union {
		char id6[6];	// Game code. (ID6)
		struct {
			char id4[4];		// Game code. (ID4)
			char company[2];	// Company code.
		};
	};

	// 0x12
	uint8_t unitcode;	// 00h == NDS, 02h == NDS+DSi, 03h == DSi only
	uint8_t enc_seed_select;
	uint8_t device_capacity;
	uint8_t reserved1[7];
	uint8_t reserved2_dsi;
	uint8_t nds_region;	// 0x00 == normal, 0x80 == China, 0x40 == Korea
	uint8_t rom_version;
	uint8_t autostart;

	// 0x20
	struct {
		uint32_t rom_offset;
		uint32_t entry_address;
		uint32_t ram_address;
		uint32_t size;
	} arm9;
	struct {
		uint32_t rom_offset;
		uint32_t entry_address;
		uint32_t ram_address;
		uint32_t size;
	} arm7;

	// 0x40
	uint32_t fnt_offset;	// File Name Table offset
	uint32_t fnt_size;	// File Name Table size
	uint32_t fat_offset;
	uint32_t fat_size;

	// 0x50
	uint32_t arm9_overlay_offset;
	uint32_t arm9_overlay_size;
	uint32_t arm7_overlay_offset;
	uint32_t arm7_overlay_size;

	// 0x60
	uint32_t cardControl13;	// Port 0x40001A4 setting for normal commands (usually 0x00586000)
	uint32_t cardControlBF;	// Port 0x40001A4 setting for KEY1 commands (usually 0x001808F8)

	// 0x68
        uint32_t icon_offset;
        uint16_t secure_area_checksum;		// CRC32 of 0x0020...0x7FFF
        uint16_t secure_area_delay;		// Delay, in 131 kHz units (0x051E=10ms, 0x0D7E=26ms)

        uint32_t arm9_auto_load_list_ram_address;
        uint32_t arm7_auto_load_list_ram_address;

        uint64_t secure_area_disable;

	// 0x80
        uint32_t total_used_rom_size;		// Excluding DSi area
        uint32_t rom_header_size;		// Usually 0x4000
        uint8_t reserved3[0x38];
        uint8_t nintendo_logo[0x9C];		// GBA-style Nintendo logo
        uint16_t nintendo_logo_checksum;	// CRC16 of nintendo_logo[] (always 0xCF56)
        uint16_t header_checksum;		// CRC16 of 0x0000...0x015D

        // 0x160
        struct {
                uint32_t rom_offset;
                uint32_t size;
                uint32_t ram_address;
        } debug;

	// 0x16C
        uint8_t reserved4[4];
        uint8_t reserved5[0x10];

	// 0x180 [DSi header]
	uint32_t dsi_region;	// DSi region flags.

	// TODO: More DSi header entries.
	// Reference: http://problemkaputt.de/gbatek.htm#dsicartridgeheader
	uint8_t reserved_more_dsi[3708];
};
#pragma pack()

/**
 * Nintendo DS icon and title struct.
 * Reference: http://problemkaputt.de/gbatek.htm#dscartridgeheader
 *
 * All fields are little-endian.
 */
#pragma pack(1)
struct PACKED DS_IconTitleData {
	uint16_t version;	// known values: 0x0001, 0x0002, 0x0003, 0x0103
	uint16_t crc16[4];	// CRC16s for the four known versions.
	uint8_t reserved1[0x16];

	uint8_t icon_data[0x200];	// Icon data. (32x32, 4x4 tiles, 4-bit color)
	uint16_t icon_pal[0x10];	// Icon palette. (16-bit color; color 0 is transparent)

	// Titles. (128 characters each; UTF-16)
	// Order: JP, EN, FR, DE, IT, ES, ZH (v0002), KR (v0003)
	char title[8][128];

	// Reserved space, possibly for other titles.
	char reserved2[0x800];

	// DSi animated icons (v0103h)
	// Icons use the same format as DS icons.
	uint8_t dsi_icon_data[8][0x200];	// Icon data. (Up to 8 frames)
	uint8_t dsi_icon_pal[8][0x10];		// Icon palette.
	uint16_t dsi_icon_seq[0x40];		// Icon animation sequence.
};
#pragma pack()

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
NintendoDS::NintendoDS(FILE *file)
	: RomData(file, nds_fields, ARRAY_SIZE(nds_fields))
{
	if (!m_file) {
		// Could not dup() the file handle.
		return;
	}

	// Seek to the beginning of the file.
	rewind(m_file);
	fflush(m_file);

	// Read the ROM header.
	static_assert(sizeof(DS_ROMHeader) == 4096, "DS_ROMHeader is not 4,096 bytes.");
	DS_ROMHeader header;
	size_t size = fread(&header, 1, sizeof(header), m_file);
	if (size != sizeof(header))
		return;

	// Check if this ROM image is supported.
	m_isValid = isRomSupported(reinterpret_cast<const uint8_t*>(&header), sizeof(header));
}

/**
 * Detect if a ROM image is supported by this class.
 * TODO: Actually detect the type; for now, just returns true if it's supported.
 * @param header Header data.
 * @param size Size of header.
 * @return 1 if the ROM image is supported; 0 if it isn't.
 */
int NintendoDS::isRomSupported(const uint8_t *header, size_t size)
{
	if (size >= sizeof(DS_ROMHeader)) {
		// Check the first 16 bytes of the Nintendo logo.
		static const uint8_t nintendo_gba_logo[16] = {
			0x24, 0xFF, 0xAE, 0x51, 0x69, 0x9A, 0xA2, 0x21,
			0x3D, 0x84, 0x82, 0x0A, 0x84, 0xE4, 0x09, 0xAD
		};

		const DS_ROMHeader *ds_header = reinterpret_cast<const DS_ROMHeader*>(header);
		if (!memcmp(ds_header->nintendo_logo, nintendo_gba_logo, sizeof(nintendo_gba_logo))) {
			// Nintendo logo is present at the correct location.
			// TODO: DS vs. DSi?
			return 1;
		}
	}

	// Not supported.
	return 0;
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
	vector<const rp_char*> ret;
	ret.reserve(1);
	ret.push_back(_RP(".nds"));
	return ret;
}

/**
 * Get a bitfield of image types this class can retrieve.
 * @return Bitfield of supported image types. (ImageTypesBF)
 */
uint32_t NintendoDS::supportedImageTypes(void) const
{
	return IMGBF_INT_ICON;
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
	}
	if (!m_file) {
		// File isn't open.
		return -EBADF;
	}

	// Seek to the beginning of the file.
	rewind(m_file);
	fflush(m_file);

	// Read the ROM header.
	DS_ROMHeader header;
	size_t size = fread(&header, 1, sizeof(header), m_file);
	if (size != sizeof(header)) {
		// File isn't big enough for a Nintendo DS header...
		return -EIO;
	}

	// Game title.
	// TODO: Is Shift-JIS actually permissible here?
	m_fields->addData_string(cp1252_sjis_to_rp_string(header.title, sizeof(header.title)));

	// Game ID and publisher.
	m_fields->addData_string(ascii_to_rp_string(header.id6, sizeof(header.id6)));

	// Look up the publisher.
	const rp_char *publisher = NintendoPublishers::lookup(header.company);
	m_fields->addData_string(publisher ? publisher : _RP("Unknown"));

	// ROM version.
	m_fields->addData_string_numeric(header.rom_version, RomFields::FB_DEC, 2);

	// Hardware type.
	// NOTE: DS_HW_DS is inverted bit0; DS_HW_DSi is normal bit1.
	uint32_t hw_type = (header.unitcode & DS_HW_DS) ^ DS_HW_DS;
	hw_type |= (header.unitcode & DS_HW_DSi);
	if (hw_type == 0) {
		// 0x01 is invalid. Assume DS.
		hw_type = DS_HW_DS;
	}
	m_fields->addData_bitfield(hw_type);

	// TODO: Combine DS Region and DSi Region into one bitfield?

	// DS Region.
	uint32_t nds_region;
	if (header.nds_region == 0) {
		// Region-free.
		nds_region = NDS_REGION_FREE;
	} else {
		nds_region = 0;
		if (header.nds_region & 0x80)
			nds_region |= NDS_REGION_CHINA;
		if (header.nds_region & 0x40)
			nds_region |= NDS_REGION_SKOREA;
	}
	m_fields->addData_bitfield(nds_region);

	// DSi Region.
	// Maps directly to the header field.
	if (hw_type & DS_HW_DSi) {
		m_fields->addData_bitfield(header.dsi_region);
	} else {
		// No DSi region.
		// TODO: addData_null()?
		m_fields->addData_string(nullptr);
	}

	// Finished reading the field data.
	return (int)m_fields->count();
}

/**
 * Convert an RGB555 pixel to ARGB32.
 * @param px16 RGB555 pixel.
 * @return ARGB32 pixel.
 */
static inline uint32_t RGB555_to_ARGB32(uint16_t px16)
{
	uint32_t px32;

	// BGR555: xBBBBBGG GGGRRRRR
	// ARGB32: AAAAAAAA RRRRRRRR GGGGGGGG BBBBBBBB
	px32 = ((((px16 << 19) & 0xF80000) | ((px16 << 17) & 0x070000))) |	// Red
	       ((((px16 <<  6) & 0x00F800) | ((px16 <<  1) & 0x000700))) |	// Green
	       ((((px16 >>  7) & 0x0000F8) | ((px16 >> 12) & 0x000007)));	// Blue

	// No alpha channel.
	px32 |= 0xFF000000U;
	return px32;
}

/**
 * Blit a tile to a linear image buffer.
 * @param pixel		[in] Pixel type.
 * @param tileW		[in] Tile width.
 * @param tileH		[in] Tile height.
 * @param imgBuf	[out] Linear image buffer.
 * @param pitch		[in] Pitch of image buffer, in pixels.
 * @param tileBuf	[in] Tile buffer.
 * @param tileX		[in] Horizontal tile number.
 * @param tileY		[in] Vertical tile number.
 */
template<typename pixel, int tileW, int tileH>
static inline void BlitTile(pixel *imgBuf, int pitch,
			    const pixel *tileBuf, int tileX, int tileY)
{
	// Go to the first pixel for this tile.
	imgBuf += ((tileY * tileH * pitch) + (tileX * tileW));

	for (int y = tileH; y != 0; y--) {
		memcpy(imgBuf, tileBuf, (tileW * sizeof(pixel)));
		imgBuf += pitch;
		tileBuf += tileW;
	}
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
	}
	if (!m_file) {
		// File isn't open.
		return -EBADF;
	}

	// Check for supported image types.
	if (imageType != IMG_INT_ICON) {
		// Only IMG_INT_ICON is supported by DS.
		return -ENOENT;
	}

	// Address of icon/title information is located at
	// 0x68 in the cartridge header.
	uint32_t icon_addr;
	fseek(m_file, 0x68, SEEK_SET);
	size_t size = fread(&icon_addr, 1, sizeof(icon_addr), m_file);
	if (size != sizeof(icon_addr))
		return -EIO;
	icon_addr = le32_to_cpu(icon_addr);

	// Read the icon data.
	// TODO: Also store titles?
	DS_IconTitleData ds_icon_title;
	fseek(m_file, icon_addr, SEEK_SET);
	size = fread(&ds_icon_title, 1, sizeof(ds_icon_title), m_file);
	if (size != sizeof(ds_icon_title))
		return -EIO;

	// Convert the icon from DS format to 256-color.
	rp_image *icon = new rp_image(32, 32, rp_image::FORMAT_CI8);

	// Convert the palette first.
	// TODO: Make sure there's at least 16 entries?
	uint32_t *palette = icon->palette();
	palette[0] = 0; // Color 0 is always transparent.
	icon->set_tr_idx(0);
	for (int i = 1; i < 16; i++) {
		// DS color format is BGR555.
		palette[i] = RGB555_to_ARGB32(le16_to_cpu(ds_icon_title.icon_pal[i]));
	}

	// 2 bytes = 4 pixels
	// Image is composed of 8x8px tiles.
	// 4 tiles wide, 4 tiles tall.
	uint8_t tileBuf[8*8];
	const uint8_t *src = ds_icon_title.icon_data;

	for (int y = 0; y < 4; y++) {
		for (int x = 0; x < 4; x++) {
			// Convert each tile to 8-bit color manually.
			for (int i = 0; i < 8*8; i += 2, src++) {
				tileBuf[i+0] = *src & 0x0F;
				tileBuf[i+1] = *src >> 4;
			}

			// Blit the tile to the main image buffer.
			BlitTile<uint8_t, 8, 8>((uint8_t*)icon->bits(), 32, tileBuf, x, y);
		}
	}

	// Finished decoding the icon.
	m_images[imageType] = icon;
	return 0;
}

}
