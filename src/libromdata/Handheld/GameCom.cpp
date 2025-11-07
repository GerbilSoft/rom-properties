/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * GameCom.hpp: Tiger game.com ROM reader.                                 *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "GameCom.hpp"
#include "gcom_structs.h"

// Other rom-properties libraries
using namespace LibRpBase;
using namespace LibRpFile;
using namespace LibRpText;
using namespace LibRpTexture;

// C++ STL classes
using std::array;
using std::string;
using std::unique_ptr;
using std::vector;

namespace LibRomData {

class GameComPrivate final : public RomDataPrivate
{
public:
	explicit GameComPrivate(const IRpFilePtr &file);

private:
	typedef RomDataPrivate super;
	RP_DISABLE_COPY(GameComPrivate)

public:
	/** RomDataInfo **/
	static const array<const char*, 2+1> exts;
	static const array<const char*, 1+1> mimeTypes;
	static const RomDataInfo romDataInfo;

public:
	// ROM header
	Gcom_RomHeader romHeader;

	// Icon
	rp_image_ptr img_icon;

	/**
	 * Load the icon.
	 * @return Icon, or nullptr on error.
	 */
	rp_image_const_ptr loadIcon(void);

protected:
	// Palette
	// NOTE: Index 0 is white; index 3 is black.
	// TODO: Use colors closer to the original screen?
	static const array<uint32_t, 4> gcom_palette;

	/**
	 * Decompress game.com RLE data.
	 *
	 * NOTE: The ROM header does *not* indicate how much data is
	 * available, so we'll keep going until we're out of either
	 * output or input buffer.
	 *
	 * @param pOut		[out] Output buffer.
	 * @param out_len	[in] Output buffer length.
	 * @param pIn		[in] Input buffer.
	 * @param in_len	[in] Input buffer length.
	 * @return Decoded data length on success; negative POSIX error code on error.
	 */
	static ssize_t rle_decompress(uint8_t *pOut, size_t out_len, const uint8_t *pIn, size_t in_len);

	/**
	 * Load an RLE-compressed icon.
	 * This is called from loadIcon().
	 * @return Icon, or nullptr on error.
	 */
	rp_image_const_ptr loadIconRLE(void);
};

ROMDATA_IMPL(GameCom)

/** GameComPrivate **/

/* RomDataInfo */
const array<const char*, 2+1> GameComPrivate::exts = {{
	".bin",	// Most common (only one supported by the official emulator)
	".tgc",	// Less common

	nullptr
}};
const array<const char*, 1+1> GameComPrivate::mimeTypes = {{
	// Unofficial MIME types.
	// TODO: Get these upstreamed on FreeDesktop.org.
	"application/x-game-com-rom",

	nullptr
}};
const RomDataInfo GameComPrivate::romDataInfo = {
	"GameCom", exts.data(), mimeTypes.data()
};

// Palette
// NOTE: Index 0 is white; index 3 is black.
// TODO: Use colors closer to the original screen?
const array<uint32_t, 4> GameComPrivate::gcom_palette = {{
	0xFFFFFFFF,
	0xFFC0C0C0,
	0xFF808080,
	0xFF000000,
}};

GameComPrivate::GameComPrivate(const IRpFilePtr &file)
	: super(file, &romDataInfo)
{
	// Clear the ROM header struct.
	memset(&romHeader, 0, sizeof(romHeader));
}

/**
 * Load the icon.
 * @return Icon, or nullptr on error.
 */
rp_image_const_ptr GameComPrivate::loadIcon(void)
{
	if (img_icon) {
		// Icon has already been loaded.
		return img_icon;
	} else if (!this->isValid || !this->file) {
		// Can't load the icon.
		return nullptr;
	} else if (!(romHeader.flags & GCOM_FLAG_HAS_ICON)) {
		// ROM doesn't have an icon.
		return nullptr;
	}

	if (romHeader.flags & GCOM_FLAG_ICON_RLE) {
		// Icon is RLE-compressed.
		return loadIconRLE();
	}

	// Icon is 64x64.
	// Consequently, the X and Y coordinates must each be <= 192.
	if (romHeader.icon.x > (GCOM_ICON_BANK_W - GCOM_ICON_W) ||
	    romHeader.icon.y > (GCOM_ICON_BANK_H - GCOM_ICON_H))
	{
		// Icon is out of range.
		return nullptr;
	}

	const off64_t fileSize = this->file->size();
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
		const unsigned int lz = (1U << uilog2(static_cast<unsigned int>(fileSize)));
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
	rp_image_ptr tmp_icon = std::make_shared<rp_image>(GCOM_ICON_W, GCOM_ICON_H, rp_image::Format::CI8);

	uint32_t *const palette = tmp_icon->palette();
	assert(palette != nullptr);
	assert(tmp_icon->palette_len() >= 4);
	if (!palette || tmp_icon->palette_len() < 4) {
		return nullptr;
	}
	memcpy(palette, gcom_palette.data(), sizeof(gcom_palette));

	// Decode the 2bpp icon data into 8bpp.
	// NOTE: Each bank is 256px wide, so we'll end up
	// reading 256x64.
	unique_ptr<uint8_t[]> icon_data(new uint8_t[icon_data_len]);
	size_t size = file->seekAndRead(icon_file_offset, icon_data.get(), icon_data_len);
	if (size != icon_data_len) {
		// Short read.
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
		pDestBase = static_cast<uint8_t*>(tmp_icon->bits());
		dest_stride = tmp_icon->stride();
	}

	// Convert 2bpp to 8bpp.
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
		pDestBase = static_cast<uint8_t*>(tmp_icon->bits());
		dest_stride = tmp_icon->stride();
		if (dest_stride == GCOM_ICON_W) {
			// Stride matches. Copy everything all at once.
			memcpy(pDestBase, pSrc, GCOM_ICON_W*GCOM_ICON_H);
		} else {
			// Stride doesn't match. Copy line-by-line.
			for (int y = GCOM_ICON_H; y > 0; y--) {
				memcpy(pDestBase, pSrc, GCOM_ICON_W);
				pSrc += GCOM_ICON_W;
				pDestBase += dest_stride;
			}
		}
	}

	// Set the sBIT metadata.
	// TODO: Use grayscale instead of RGB.
	static const rp_image::sBIT_t sBIT = {2,2,2,0,0};
	tmp_icon->set_sBIT(sBIT);

	// Save and return the icon.
	this->img_icon = tmp_icon;
	return tmp_icon;
}

/**
 * Decompress game.com RLE data.
 *
 * NOTE: The ROM header does *not* indicate how much data is
 * available, so we'll keep going until we're out of either
 * output or input buffer.
 *
 * @param pOut		[out] Output buffer.
 * @param out_len	[in] Output buffer length.
 * @param pIn		[in] Input buffer.
 * @param in_len	[in] Input buffer length.
 * @return Decoded data length on success; negative POSIX error code on error.
 */
ssize_t GameComPrivate::rle_decompress(uint8_t *pOut, size_t out_len, const uint8_t *pIn, size_t in_len)
{
	assert(pOut != nullptr);
	assert(out_len > 0);
	assert(pIn != nullptr);
	assert(in_len > 0);
	if (!pOut || out_len == 0 || !pIn || in_len == 0) {
		return -EINVAL;
	}

	uint8_t const *pOut_orig = pOut;
	uint8_t const *pOut_end = pOut + out_len;
	const uint8_t *const pIn_end = pIn + in_len;

	while (pIn < pIn_end && pOut < pOut_end) {
		unsigned int count = 0;

		if (pIn[0] == 0xC0) {
			// 16-bit RLE:
			// - pIn[2],pIn[1]: 16-bit count (LE)
			// - pIn[3]: Data byte
			if (pIn + 3 >= pIn_end) {
				// Input buffer overflow!
				return -EIO;
			}
			count = (pIn[2] << 8) | pIn[1];
			if (pOut + count > pOut_end) {
				// Output buffer overflow!
				return -EIO;
			}
			memset(pOut, pIn[3], count);
			pIn += 4;
		} else if (pIn[0] > 0xC0) {
			// 6-bit RLE:
			// - pIn[0]: 6-bit count (low 6 bits)
			// - pIn[1]: Data byte
			if (pIn + 1 >= pIn_end) {
				// Input buffer overflow!
				return -EIO;
			}
			count = pIn[0] & 0x3F;
			if (pOut + count > pOut_end) {
				// Output buffer overflow!
				return -EIO;
			}
			memset(pOut, pIn[1], count);
			pIn += 2;
		} else {
			// Regular data byte.
			*pOut++ = *pIn++;
		}

		// Output buffer: Add bytes for 6-bit and 16-bit RLE.
		pOut += count;
	}

	return (pOut - pOut_orig);
}

/**
 * Load an RLE-compressed icon.
 * This is called from loadIcon().
 * @return Icon, or nullptr on error.
 */
rp_image_const_ptr GameComPrivate::loadIconRLE(void)
{
	// Checks were already done in loadIcon().
	assert(romHeader.flags & GCOM_FLAG_ICON_RLE);

	const off64_t fileSize = this->file->size();
	uint8_t bank_number = romHeader.icon.bank;
	unsigned int bank_offset = bank_number * GCOM_ICON_BANK_SIZE_RLE;
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
		const unsigned int lz = (1U << uilog2(static_cast<unsigned int>(fileSize)));
		bank_number &= ((lz / GCOM_ICON_BANK_SIZE_RLE) - 1);
		bank_offset = (bank_number * GCOM_ICON_BANK_SIZE_RLE) - bank_adj;
	}

	// Icon is stored at bank_offset + ((x << 8) | y).
	// Up to 4 bytes per 4 pixels for the most extreme RLE test case.
	// (effectively 8bpp, though usually much less)
	static constexpr size_t icon_rle_data_max_len = GCOM_ICON_W * GCOM_ICON_H;
	unique_ptr<uint8_t[]> icon_rle_data(new uint8_t[icon_rle_data_max_len]);
	unsigned int icon_file_offset = bank_offset + ((romHeader.icon.x << 8) | (romHeader.icon.y));
	if (icon_file_offset >= GCOM_ICON_RLE_BANK_LOAD_OFFSET) {
		// Not sure why this is needed...
		icon_file_offset -= GCOM_ICON_RLE_BANK_LOAD_OFFSET;
	}
	size_t size = file->seekAndRead(icon_file_offset, icon_rle_data.get(), icon_rle_data_max_len);
	if (size == 0) {
		// No data...
		return nullptr;
	} else if (size < icon_rle_data_max_len) {
		// Zero out the remaining area.
		memset(&icon_rle_data[size], 0, icon_rle_data_max_len - size);
	}

	// Decompress the RLE data. (2bpp)
	static constexpr size_t icon_data_len = (GCOM_ICON_W * GCOM_ICON_H) / 4;
	unique_ptr<uint8_t[]> icon_data(new uint8_t[icon_data_len]);
	ssize_t ssize = rle_decompress(icon_data.get(), icon_data_len, icon_rle_data.get(), icon_rle_data_max_len);
	if (ssize != icon_data_len) {
		// Decompression failed.
		return nullptr;
	}

	// Create the icon.
	// TODO: Split into an ImageDecoder function?
	rp_image_ptr tmp_icon = std::make_shared<rp_image>(GCOM_ICON_W, GCOM_ICON_H, rp_image::Format::CI8);

	uint32_t *const palette = tmp_icon->palette();
	assert(palette != nullptr);
	assert(tmp_icon->palette_len() >= 4);
	if (!palette || tmp_icon->palette_len() < 4) {
		return nullptr;
	}
	memcpy(palette, gcom_palette.data(), sizeof(gcom_palette));

	// NOTE: The image is vertically mirrored and rotated 270 degrees.
	// Because of this, we can't use scanline pointer adjustment for
	// the destination image. Each pixel address will be calculated
	// manually.

	// NOTE 2: Due to RLE compression, the icon is *always* aligned
	// on a byte boundary in the decompressed data, so we won't need
	// to do manual realignment.
	uint8_t *const pDestBase = static_cast<uint8_t*>(tmp_icon->bits());
	const int dest_stride = tmp_icon->stride();

	// Convert 2bpp to 8bpp.
	const uint8_t *pSrc = icon_data.get();
	for (unsigned int x = 0; x < GCOM_ICON_W; x++) {
		uint8_t *pDest = pDestBase + x;
		for (unsigned int y = 0; y < GCOM_ICON_H; y += 4, pSrc++) {
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
	}

	// Set the sBIT metadata.
	// TODO: Use grayscale instead of RGB.
	static const rp_image::sBIT_t sBIT = {2,2,2,0,0};
	tmp_icon->set_sBIT(sBIT);

	// Save and return the icon.
	this->img_icon = tmp_icon;
	return tmp_icon;
}

/** GameCom **/

/**
 * Read a Tiger game.com ROM image.
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
GameCom::GameCom(const IRpFilePtr &file)
	: super(new GameComPrivate(file))
{
	RP_D(GameCom);
	d->mimeType = "application/x-game-com-rom";	// unofficial, not on fd.o

	if (!d->file) {
		// Could not ref() the file handle.
		return;
	}

	DetectInfo info = {
		{0, sizeof(d->romHeader), reinterpret_cast<const uint8_t*>(&d->romHeader)},
		nullptr,	// ext (not needed for GameCom)
		0		// szFile (not needed for GameCom)
	};

	// Read the ROM header at the standard address.
	size_t size = d->file->seekAndRead(GCOM_HEADER_ADDRESS, &d->romHeader, sizeof(d->romHeader));
	if (size == sizeof(d->romHeader)) {
		// Check if this ROM image is supported.
		info.header.addr = GCOM_HEADER_ADDRESS;
		d->isValid = (isRomSupported_static(&info) >= 0);
	}

	if (!d->isValid) {
		// Try again at the alternate address.
		size = d->file->seekAndRead(GCOM_HEADER_ADDRESS_ALT, &d->romHeader, sizeof(d->romHeader));
		if (size == sizeof(d->romHeader)) {
			// Check if this ROM image is supported.
			info.header.addr = GCOM_HEADER_ADDRESS_ALT;
			d->isValid = (isRomSupported_static(&info) >= 0);
		}
	}

	if (!d->isValid) {
		// Still not valid.
		d->file.reset();
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

	// game.com has the same name worldwide, so we can
	// ignore the region selection.
	static_assert(SYSNAME_TYPE_MASK == 3,
		"GameCom::systemName() array index optimization needs to be updated.");

	static const array<const char*, 4> sysNames = {{
		"Tiger game.com", "game.com", "game.com", nullptr
	}};

	return sysNames[type & SYSNAME_TYPE_MASK];
}

/**
 * Get a bitfield of image types this object can retrieve.
 * @return Bitfield of supported image types. (ImageTypesBF)
 */
uint32_t GameCom::supportedImageTypes(void) const
{
	RP_D(const GameCom);
	if (d->isValid && (d->romHeader.flags & GCOM_FLAG_HAS_ICON)) {
		return IMGBF_INT_ICON;
	}
	return 0;
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
vector<RomData::ImageSizeDef> GameCom::supportedImageSizes(ImageType imageType) const
{
	ASSERT_supportedImageSizes(imageType);

	RP_D(const GameCom);
	if (!d->isValid || imageType != IMG_INT_ICON ||
	    !(d->romHeader.flags & GCOM_FLAG_HAS_ICON))
	{
		// Only IMG_INT_ICON is supported,
		// and/or the ROM doesn't have an icon.
		return {};
	}

	return {{nullptr, GCOM_ICON_W, GCOM_ICON_H, 0}};
}

/**
 * Get a list of all available image sizes for the specified image type.
 * @param imageType Image type.
 * @return Vector of available image sizes, or empty vector if no images are available.
 */
vector<RomData::ImageSizeDef> GameCom::supportedImageSizes_static(ImageType imageType)
{
	ASSERT_supportedImageSizes(imageType);

	if (imageType != IMG_INT_ICON) {
		// Only IMG_INT_ICON is supported.
		return {};
	}

	return {{nullptr, GCOM_ICON_W, GCOM_ICON_H, 0}};
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
	ASSERT_imgpf(imageType);

	switch (imageType) {
		case IMG_INT_ICON: {
			RP_D(const GameCom);
			if (d->isValid && (d->romHeader.flags & GCOM_FLAG_HAS_ICON)) {
				// Use nearest-neighbor scaling.
				return IMGPF_RESCALE_NEAREST;
			}
			break;
		}
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
	if (!d->fields.empty()) {
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
	d->fields.reserve(3);	// Maximum of 3 fields.

	// Title
	d->fields.addField_string(C_("RomData", "Title"),
		latin1_to_utf8(romHeader->title, sizeof(romHeader->title)),
		RomFields::STRF_TRIM_END);

	// Game ID
	d->fields.addField_string_numeric(C_("RomData", "Game ID"),
		le16_to_cpu(romHeader->game_id),
		RomFields::Base::Hex, 4, RomFields::STRF_MONOSPACE);

	// Entry point
	d->fields.addField_string_numeric(C_("RomData", "Entry Point"),
		le16_to_cpu(romHeader->entry_point),
		RomFields::Base::Hex, 4, RomFields::STRF_MONOSPACE);

	// Finished reading the field data.
	return static_cast<int>(d->fields.count());
}

/**
 * Load metadata properties.
 * Called by RomData::metaData() if the metadata hasn't been loaded yet.
 * @return Number of metadata properties read on success; negative POSIX error code on error.
 */
int GameCom::loadMetaData(void)
{
	RP_D(GameCom);
	if (!d->metaData.empty()) {
		// Metadata *has* been loaded...
		return 0;
	} else if (!d->file) {
		// File isn't open.
		return -EBADF;
	} else if (!d->isValid) {
		// Unknown ROM image type.
		return -EIO;
	}

	// game.com ROM header
	const Gcom_RomHeader *const romHeader = &d->romHeader;
	d->metaData.reserve(2);	// Maximum of 2 metadata properties.

	// Game title
	d->metaData.addMetaData_string(Property::Title,
		latin1_to_utf8(romHeader->title, sizeof(romHeader->title)),
		RomMetaData::STRF_TRIM_END);

	/** Custom properties! **/

	// Game ID
	char buf[16];
	snprintf(buf, sizeof(buf), "0x%04X", le16_to_cpu(romHeader->game_id));
	d->metaData.addMetaData_string(Property::GameID, buf);

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
int GameCom::loadInternalImage(ImageType imageType, rp_image_const_ptr &pImage)
{
	ASSERT_loadInternalImage(imageType, pImage);

	RP_D(GameCom);
	if (imageType != IMG_INT_ICON) {
		pImage.reset();
		return -ENOENT;
	} else if (d->img_icon != nullptr) {
		pImage = d->img_icon;
		return 0;
	} else if (!(d->romHeader.flags & GCOM_FLAG_HAS_ICON)) {
		// ROM doesn't have an icon.
		pImage.reset();
		return -ENOENT;
	} else if (!d->file) {
		pImage.reset();
		return -EBADF;
	} else if (!d->isValid) {
		pImage.reset();
		return -EIO;
	}

	pImage = d->loadIcon();
	return (pImage) ? 0 : -EIO;
}

} // namespace LibRomData
