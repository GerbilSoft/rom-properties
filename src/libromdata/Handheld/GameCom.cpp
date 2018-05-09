/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * GameCom.hpp: Tiger game.com ROM reader.                                 *
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

#include "GameCom.hpp"
#include "librpbase/RomData_p.hpp"
#include "gcom_structs.h"

// librpbase
#include "librpbase/common.h"
#include "librpbase/byteswap.h"
#include "librpbase/bitstuff.h"
#include "librpbase/TextFuncs.hpp"
#include "librpbase/file/IRpFile.hpp"

#include "librpbase/img/rp_image.hpp"

#include "libi18n/i18n.h"
using namespace LibRpBase;

// C includes. (C++ namespace)
#include <cassert>
#include <cerrno>
#include <cstring>

// C++ includes.
#include <memory>
#include <string>
#include <vector>
using std::string;
using std::unique_ptr;
using std::vector;

namespace LibRomData {

ROMDATA_IMPL(GameCom)
ROMDATA_IMPL_IMG(GameCom)

class GameComPrivate : public RomDataPrivate
{
	public:
		GameComPrivate(GameCom *q, IRpFile *file);
		virtual ~GameComPrivate();

	private:
		typedef RomDataPrivate super;
		RP_DISABLE_COPY(GameComPrivate)

	public:
		// ROM header.
		Gcom_RomHeader romHeader;

		// Icon.
		rp_image *icon;

		/**
		 * Load the icon.
		 * @return Icon, or nullptr on error.
		 */
		const rp_image *loadIcon(void);
};

/** GameComPrivate **/

GameComPrivate::GameComPrivate(GameCom *q, IRpFile *file)
	: super(q, file)
	, icon(nullptr)
{
	// Clear the ROM header struct.
	memset(&romHeader, 0, sizeof(romHeader));
}

GameComPrivate::~GameComPrivate()
{
	delete icon;
}

/**
 * Load the icon.
 * @return Icon, or nullptr on error.
 */
const rp_image *GameComPrivate::loadIcon(void)
{
	if (icon) {
		// Icon has already been loaded.
		return icon;
	} else if (!this->file || !this->isValid) {
		// Can't load the icon.
		return nullptr;
	}

	// Icon is 64x64.
	// Consequently, the X and Y coordinates must each be <= 192.
	if (romHeader.icon.x > (GCOM_ICON_BANK_W - GCOM_ICON_W) ||
	    romHeader.icon.y > (GCOM_ICON_BANK_H - GCOM_ICON_H))
	{
		// Icon is out of range.
		return nullptr;
	}

	const int64_t fileSize = this->file->size();
	uint8_t bank_number = romHeader.icon.bank;
	unsigned int bank_offset = bank_number * GCOM_ICON_BANK_SIZE;
	unsigned int bank_adj = 0;
	if (bank_offset > 256*1024) {
		// Check if the ROM is 0.8 MB or 1.8 MB.
		// These ROMs are incorrectly dumped, but we can usually
		// get the icon by subtracting 256 KB.
		if (fileSize == 768*1024 || fileSize == 1792*1024) {
			bank_adj = 256*1024;
			bank_offset -= bank_adj;
		}
	}

	// If the bank number is past the end of the ROM, it may be underdumped.
	if (bank_offset > fileSize) {
		// If the bank number is more than 2x the filesize,
		// and it's over the 1 MB mark, forget it.
		if (bank_offset > (fileSize * 2) && bank_offset > 1048576) {
			// Completely out of range.
			return nullptr;
		}
		// Get the lowest power of two size and mask the bank number.
		unsigned int lz = (1 << uilog2((unsigned int)fileSize));
		bank_number &= ((lz / GCOM_ICON_BANK_SIZE) - 1);
		bank_offset = (bank_number * GCOM_ICON_BANK_SIZE) - bank_adj;
	}

	// Make sure the icon address is valid.
	// NOTE: Last line doesn't have to be the full width.
	// NOTE: If (y % 4 != 0), we'll have to read at least one extra byte.
	unsigned int icon_data_len = ((GCOM_ICON_BANK_W * (GCOM_ICON_H - 1)) + GCOM_ICON_W) / 4;
	const unsigned int iconYalign = (romHeader.icon.y % 4);
	if (iconYalign != 0) {
		icon_data_len++;
	}
	unsigned int icon_file_offset = bank_offset;
	icon_file_offset += (romHeader.icon.y / 4);
	icon_file_offset += ((romHeader.icon.x * GCOM_ICON_BANK_W) / 4);
	if (icon_file_offset + icon_data_len > fileSize) {
		// Out of range.
		return nullptr;
	}

	// Create the icon.
	// TODO: Split into an ImageDecoder function?
	rp_image *icon = new rp_image(GCOM_ICON_W, GCOM_ICON_H, rp_image::FORMAT_CI8);

	// Set the palette.
	// NOTE: Index 0 is white; index 3 is black.
	// TODO: Use colors closer to the original screen?
	static const uint32_t gcom_palette[4] = {
		0xFFFFFFFF,
		0xFFC0C0C0,
		0xFF808080,
		0xFF000000,
	};

	uint32_t *const palette = icon->palette();
	assert(palette != nullptr);
	assert(icon->palette_len() >= 4);
	if (!palette || icon->palette_len() < 4) {
		delete icon;
		return nullptr;
	}
	memcpy(palette, gcom_palette, sizeof(gcom_palette));

	// Decode the 2bpp icon data into 8bpp.
	// NOTE: Each bank is 256px wide, so we'll end up
	// reading 256x64.
	unique_ptr<uint8_t[]> icon_data(new uint8_t[icon_data_len]);
	size_t size = file->seekAndRead(icon_file_offset, icon_data.get(), icon_data_len);
	if (size != icon_data_len) {
		// Short read.
		delete icon;
		return nullptr;
	}

	// NOTE: The image is vertically mirrored and rotated 270 degrees.
	// Because of this, we can't use scanline pointer adjustment for
	// the destination image. Each pixel address will be calculated
	// manually.
	uint8_t *pDestBase;
	int dest_stride;

	// NOTE 2: Icons might not be aligned on a byte boundary. Because of
	// this, we'll need to convert the icon using a temporary buffer,
	// then memcpy() it to the rp_image.
	unique_ptr<uint8_t[]> tmpbuf;
	if (iconYalign != 0) {
		// Y is not a multiple of 4.
		// Blit to the temporary buffer first.
		tmpbuf.reset(new uint8_t[GCOM_ICON_W * (GCOM_ICON_H + 4)]);
		pDestBase = tmpbuf.get();
		dest_stride = GCOM_ICON_W;
	} else {
		// Y is a multiple of 4.
		// Blit directly to rp_image.
		pDestBase = reinterpret_cast<uint8_t*>(icon->bits());
		dest_stride = icon->stride();
	}

	const uint8_t *pSrc = icon_data.get();
	const unsigned int Ytotal = GCOM_ICON_H + iconYalign;
	for (unsigned int x = 0; x < GCOM_ICON_W; x++) {
		uint8_t *pDest = pDestBase + x;
		for (unsigned int y = 0; y < Ytotal; y += 4, pSrc++) {
			uint8_t px2bpp = *pSrc;
			pDest[dest_stride*3] = px2bpp & 0x03;
			px2bpp >>= 2;
			pDest[dest_stride*2] = px2bpp & 0x03;
			px2bpp >>= 2;
			pDest[dest_stride*1] = px2bpp & 0x03;
			px2bpp >>= 2;
			pDest[0] = px2bpp;

			// Next group of pixels.
			pDest += (dest_stride*4);
		}

		// Next line.
		pSrc += ((GCOM_ICON_BANK_H - Ytotal) / 4);
	}

	// Copy the temporary buffer into the icon if necessary.
	if (iconYalign != 0) {
		pSrc = &tmpbuf[GCOM_ICON_W * iconYalign];
		pDestBase = reinterpret_cast<uint8_t*>(icon->bits());
		dest_stride = icon->stride();
		// TODO: Skip the loop if the strides match?
		for (int y = GCOM_ICON_H; y > 0; y--) {
			memcpy(pDestBase, pSrc, GCOM_ICON_W);
			pSrc += GCOM_ICON_W;
			pDestBase += dest_stride;
		}
	}

	// Save and return the icon.
	this->icon = icon;
	return icon;
}

/** GameCom **/

/**
 * Read a Tiger game.com ROM image.
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
GameCom::GameCom(IRpFile *file)
	: super(new GameComPrivate(this, file))
{
	RP_D(GameCom);
	d->className = "GameCom";

	if (!d->file) {
		// Could not dup() the file handle.
		return;
	}

	// Read the ROM header at the standard address.
	size_t size = d->file->seekAndRead(GCOM_HEADER_ADDRESS, &d->romHeader, sizeof(d->romHeader));
	if (size == sizeof(d->romHeader)) {
		// Check if this ROM image is supported.
		DetectInfo info;
		info.header.addr = GCOM_HEADER_ADDRESS;
		info.header.size = sizeof(d->romHeader);
		info.header.pData = reinterpret_cast<const uint8_t*>(&d->romHeader);
		info.ext = nullptr;	// Not needed for Gcom.
		info.szFile = 0;	// Not needed for Gcom.
		d->isValid = (isRomSupported_static(&info) >= 0);
	}

	if (!d->isValid) {
		// Try again at the alternate address.
		size_t size = d->file->seekAndRead(GCOM_HEADER_ADDRESS_ALT, &d->romHeader, sizeof(d->romHeader));
		if (size == sizeof(d->romHeader)) {
			// Check if this ROM image is supported.
			DetectInfo info;
			info.header.addr = GCOM_HEADER_ADDRESS_ALT;
			info.header.size = sizeof(d->romHeader);
			info.header.pData = reinterpret_cast<const uint8_t*>(&d->romHeader);
			info.ext = nullptr;	// Not needed for Gcom.
			info.szFile = 0;	// Not needed for Gcom.
			d->isValid = (isRomSupported_static(&info) >= 0);
		}
	}
}

/**
 * Is a ROM image supported by this class?
 * @param info DetectInfo containing ROM detection information.
 * @return Class-specific system ID (>= 0) if supported; -1 if not.
 */
int GameCom::isRomSupported_static(const DetectInfo *info)
{
	// NOTE: The official game.com emulator requires the header to be at 0x40000.
	// Some ROMs have the header at 0, though.
	assert(info != nullptr);
	assert(info->header.pData != nullptr);
	// TODO: Proper address handling to ensure that 0x40000 is within the buffer.
	// (See SNES for more information.)
	assert(info->header.addr == GCOM_HEADER_ADDRESS || info->header.addr == GCOM_HEADER_ADDRESS_ALT);
	if (!info || !info->header.pData ||
	    (info->header.addr != GCOM_HEADER_ADDRESS && info->header.addr != GCOM_HEADER_ADDRESS_ALT) ||
	    info->header.size < sizeof(Gcom_RomHeader))
	{
		// Either no detection information was specified,
		// or the header is too small.
		return -1;
	}

	// Check the system ID.
	const Gcom_RomHeader *const gcom_header =
		reinterpret_cast<const Gcom_RomHeader*>(info->header.pData);
	if (!memcmp(gcom_header->sys_id, GCOM_SYS_ID, sizeof(GCOM_SYS_ID)-1)) {
		// System ID is correct.
		return 0;
	}

	// Not supported.
	return -1;
}

/**
 * Get the name of the system the loaded ROM is designed for.
 * @param type System name type. (See the SystemName enum.)
 * @return System name, or nullptr if type is invalid.
 */
const char *GameCom::systemName(unsigned int type) const
{
	RP_D(const GameCom);
	if (!d->isValid || !isSystemNameTypeValid(type))
		return nullptr;

	// GBA has the same name worldwide, so we can
	// ignore the region selection.
	static_assert(SYSNAME_TYPE_MASK == 3,
		"GameCom::systemName() array index optimization needs to be updated.");

	static const char *const sysNames[4] = {
		"Tiger game.com",
		"game.com",
		"game.com",
		nullptr
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
const char *const *GameCom::supportedFileExtensions_static(void)
{
	static const char *const exts[] = {
		".bin",	// Most common (only one supported by the official emulator)
		".tgc",	// Less common

		nullptr
	};
	return exts;
}

/**
 * Get a bitfield of image types this class can retrieve.
 * @return Bitfield of supported image types. (ImageTypesBF)
 */
uint32_t GameCom::supportedImageTypes_static(void)
{
	return IMGBF_INT_ICON;
}

/**
 * Get a list of all available image sizes for the specified image type.
 * @param imageType Image type.
 * @return Vector of available image sizes, or empty vector if no images are available.
 */
std::vector<RomData::ImageSizeDef> GameCom::supportedImageSizes_static(ImageType imageType)
{
	assert(imageType >= IMG_INT_MIN && imageType <= IMG_EXT_MAX);
	if (imageType < IMG_INT_MIN || imageType > IMG_EXT_MAX) {
		// ImageType is out of range.
		return std::vector<ImageSizeDef>();
	}

	if (imageType != IMG_INT_ICON) {
		// Only icons are supported.
		return std::vector<ImageSizeDef>();
	}

	// game.com ROM images have 64x64 icons.
	static const ImageSizeDef sz_INT_ICON[] = {
		{nullptr, GCOM_ICON_W, GCOM_ICON_H, 0},
	};
	return vector<ImageSizeDef>(sz_INT_ICON,
		sz_INT_ICON + ARRAY_SIZE(sz_INT_ICON));
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
uint32_t GameCom::imgpf(ImageType imageType) const
{
	assert(imageType >= IMG_INT_MIN && imageType <= IMG_EXT_MAX);
	if (imageType < IMG_INT_MIN || imageType > IMG_EXT_MAX) {
		// ImageType is out of range.
		return 0;
	}

	switch (imageType) {
		case IMG_INT_ICON:
		case IMG_INT_BANNER:
			// Use nearest-neighbor scaling.
			return IMGPF_RESCALE_NEAREST;
		default:
			break;
	}
	return 0;
}

/**
 * Load field data.
 * Called by RomData::fields() if the field data hasn't been loaded yet.
 * @return Number of fields read on success; negative POSIX error code on error.
 */
int GameCom::loadFieldData(void)
{
	RP_D(GameCom);
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

	// TODO: Add more fields?

	// game.com ROM header.
	const Gcom_RomHeader *const romHeader = &d->romHeader;
	d->fields->reserve(3);	// Maximum of 3 fields.

	// Game title.
	// TODO: Trim spaces?
	d->fields->addField_string(C_("GameCom", "Title"),
		latin1_to_utf8(romHeader->title, sizeof(romHeader->title)));

	// Game ID.
	d->fields->addField_string_numeric(C_("GameCom", "Game ID"),
		le16_to_cpu(romHeader->game_id), RomFields::FB_HEX, 4);

	// Entry point..
	d->fields->addField_string_numeric(C_("GameCom", "Entry Point"),
		le16_to_cpu(romHeader->entry_point), RomFields::FB_HEX, 4);

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
int GameCom::loadInternalImage(ImageType imageType, const rp_image **pImage)
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

	RP_D(GameCom);
	if (imageType != IMG_INT_ICON) {
		// Only IMG_INT_ICON is supported by PS1.
		*pImage = nullptr;
		return -ENOENT;
	} else if (d->icon) {
		// Image has already been loaded.
		*pImage = d->icon;
		return 0;
	} else if (!d->file) {
		// File isn't open.
		*pImage = nullptr;
		return -EBADF;
	} else if (!d->isValid) {
		// Save file isn't valid.
		*pImage = nullptr;
		return -EIO;
	}

	// Load the icon.
	// TODO: -ENOENT if the file doesn't actually have an icon.
	*pImage = d->loadIcon();
	return (*pImage != nullptr ? 0 : -EIO);
}

}
