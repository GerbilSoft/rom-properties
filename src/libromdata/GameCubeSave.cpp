/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * GameCubeSave.hpp: Nintendo GameCube save file reader.                   *
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

#include "GameCubeSave.hpp"
#include "RomData_p.hpp"

#include "data/NintendoPublishers.hpp"
#include "gcn_card.h"

#include "common.h"
#include "byteswap.h"
#include "TextFuncs.hpp"
#include "file/IRpFile.hpp"

#include "img/rp_image.hpp"
#include "img/ImageDecoder.hpp"
#include "img/IconAnimData.hpp"

// C includes. (C++ namespace)
#include <cassert>
#include <cctype>
#include <cstdio>
#include <cstring>

// C++ includes.
#include <algorithm>
#include <string>
#include <vector>
using std::string;
using std::vector;

namespace LibRomData {

class GameCubeSavePrivate : public RomDataPrivate
{
	public:
		GameCubeSavePrivate(GameCubeSave *q, IRpFile *file);
		virtual ~GameCubeSavePrivate();

	private:
		typedef RomDataPrivate super;
		RP_DISABLE_COPY(GameCubeSavePrivate)

	public:
		// Internal images.
		rp_image *img_banner;

		// Animated icon data.
		IconAnimData *iconAnimData;

	public:
		// RomFields data.

		// Directory entry from the GCI header.
		card_direntry direntry;

		// Save file type.
		enum SaveType {
			SAVE_TYPE_UNKNOWN = -1,	// Unknown save type.

			SAVE_TYPE_GCI = 0,	// USB Memory Adapter
			SAVE_TYPE_GCS = 1,	// GameShark
			SAVE_TYPE_SAV = 2,	// MaxDrive
		};
		int saveType;

		/**
		 * Byteswap a card_direntry struct.
		 * @param direntry card_direntry struct.
		 * @param saveType Apply quirks for a specific save type.
		 */
		static void byteswap_direntry(card_direntry *direntry, SaveType saveType);

		// Data offset. This is the actual starting address
		// of the game data, past the file-specific headers
		// and the CARD directory entry.
		int dataOffset;

		/**
		 * Is the specified buffer a valid CARD directory entry?
		 * @param buffer CARD directory entry. (Must be 64 bytes.)
		 * @param data_size Data area size. (no headers)
		 * @param saveType Apply quirks for a specific save type.
		 * @return True if this appears to be a valid CARD directory entry; false if not.
		 */
		static bool isCardDirEntry(const uint8_t *buffer, uint32_t data_size, SaveType saveType);

		/**
		 * Load the save file's icons.
		 *
		 * This will load all of the animated icon frames,
		 * though only the first frame will be returned.
		 *
		 * @return Icon, or nullptr on error.
		 */
		const rp_image *loadIcon(void);

		/**
		 * Load the save file's banner.
		 * @return Banner, or nullptr on error.
		 */
		const rp_image *loadBanner(void);
};

/** GameCubeSavePrivate **/

GameCubeSavePrivate::GameCubeSavePrivate(GameCubeSave *q, IRpFile *file)
	: super(q, file)
	, img_banner(nullptr)
	, iconAnimData(nullptr)
	, saveType(SAVE_TYPE_UNKNOWN)
	, dataOffset(-1)
{
	// Clear the directory entry.
	memset(&direntry, 0, sizeof(direntry));
}

GameCubeSavePrivate::~GameCubeSavePrivate()
{
	delete img_banner;
	if (iconAnimData) {
		for (int i = iconAnimData->count-1; i >= 0; i--) {
			delete iconAnimData->frames[i];
		}
		delete iconAnimData;
	}
}

/**
 * Byteswap a card_direntry struct.
 * @param direntry card_direntry struct.
 * @param saveType Apply quirks for a specific save type.
 */
void GameCubeSavePrivate::byteswap_direntry(card_direntry *direntry, SaveType saveType)
{
	if (saveType == SAVE_TYPE_SAV) {
		// Swap 16-bit values at 0x2E through 0x40.
		// Also 0x06 (pad_00 / bannerfmt).
		// Reference: https://github.com/dolphin-emu/dolphin/blob/master/Source/Core/Core/HW/GCMemcard.cpp
		uint16_t *u16ptr = reinterpret_cast<uint16_t*>(direntry);
		u16ptr[0x06>>1] = __swab16(u16ptr[0x06>>1]);
		for (int i = (0x2C>>1); i < (0x40>>1); i++) {
			u16ptr[i] = __swab16(u16ptr[i]);
		}
	}

	// FIXME: Dolphin says the GCS length field might not be accurate.

	// 16-bit fields.
	direntry->iconfmt	= be16_to_cpu(direntry->iconfmt);
	direntry->iconspeed	= be16_to_cpu(direntry->iconspeed);
	direntry->block		= be16_to_cpu(direntry->block);
	direntry->length	= be16_to_cpu(direntry->length);
	direntry->pad_01	= be16_to_cpu(direntry->pad_01);

	// 32-bit fields.
	direntry->lastmodified	= be32_to_cpu(direntry->lastmodified);
	direntry->iconaddr	= be32_to_cpu(direntry->iconaddr);
	direntry->commentaddr	= be32_to_cpu(direntry->commentaddr);
}

/**
 * Is the specified buffer a valid CARD directory entry?
 * @param buffer CARD directory entry. (Must be 64 bytes.)
 * @param data_size Data area size. (no headers)
 * @param saveType Apply quirks for a specific save type.
 * @return True if this appears to be a valid CARD directory entry; false if not.
 */
bool GameCubeSavePrivate::isCardDirEntry(const uint8_t *buffer, uint32_t data_size, SaveType saveType)
{
	// MaxDrive SAV files use 16-bit byteswapping for non-text fields.
	// This means PDP-endian for 32-bit fields!
	const card_direntry *const direntry = reinterpret_cast<const card_direntry*>(buffer);

	// Game ID should be alphanumeric.
	// TODO: NDDEMO has a NULL in the game ID, but I don't think
	// it has save files.
	for (int i = 6-1; i >= 0; i--) {
		if (!isalnum(direntry->id6[i])) {
			// Non-alphanumeric character.
			return false;
		}
	}

	// Padding should be 0xFF.
	if (saveType == SAVE_TYPE_SAV) {
		// MaxDrive SAV. pad_00 and bannerfmt are swappde.
		if (direntry->bannerfmt != 0xFF) {
			// Incorrect padding.
			return false;
		}
	} else {
		// Other formats.
		if (direntry->pad_00 != 0xFF) {
			// Incorrect padding.
			return false;
		}
	}

	if (be16_to_cpu(direntry->pad_01) != 0xFFFF) {
		// Incorrect padding.
		return false;
	}

	// Verify the block count.
	// NOTE: GCS block count is not always correct.
	// Dolphin says that the actual block size is
	// stored in the GSV file. If a GCS file is added
	// without using the GameSaves software, this
	// field will always be 1.
	unsigned int length;
	switch (saveType) {
		case SAVE_TYPE_GCS:
			// Just check for >= 1.
			length = be16_to_cpu(direntry->length);
			if (length == 0) {
				// Incorrect block count.
				return false;
			}
			break;

		case SAVE_TYPE_SAV:
			// SAV: Field is little-endian
			length = le16_to_cpu(direntry->length);
			if (length * 8192 != data_size) {
				// Incorrect block count.
				return false;
			}
			break;

		case SAVE_TYPE_GCI:
		default:
			// GCI: Field is big-endian.
			length = be16_to_cpu(direntry->length);
			if (length * 8192 != data_size) {
				// Incorrect block count.
				return false;
			}
			break;
	}

	// Comment and icon addresses should both be less than the file size,
	// minus 64 bytes for the GCI header.
#define PDP_SWAP(dest, src) \
	do { \
		union { uint16_t w[2]; uint32_t d; } tmp; \
		tmp.d = be32_to_cpu(src); \
		tmp.w[0] = __swab16(tmp.w[0]); \
		tmp.w[1] = __swab16(tmp.w[1]); \
		(dest) = tmp.d; \
	} while (0)
	uint32_t iconaddr, commentaddr;
	if (saveType != SAVE_TYPE_SAV) {
		iconaddr = be32_to_cpu(direntry->iconaddr);
		commentaddr = be32_to_cpu(direntry->commentaddr);
	} else {
		PDP_SWAP(iconaddr, direntry->iconaddr);
		PDP_SWAP(commentaddr, direntry->commentaddr);
	}
	if (iconaddr >= data_size || commentaddr >= data_size) {
		// Comment and/or icon are out of bounds.
		return false;
	}

	// This appears to be a valid CARD directory entry.
	return true;
}

/**
 * Load the save file's icons.
 *
 * This will load all of the animated icon frames,
 * though only the first frame will be returned.
 *
 * @return Icon, or nullptr on error.
 */
const rp_image *GameCubeSavePrivate::loadIcon(void)
{
	if (iconAnimData) {
		// Icon has already been loaded.
		return iconAnimData->frames[0];
	} else if (!this->file || !this->isValid) {
		// Can't load the icon.
		return nullptr;
	}

	// Calculate the icon start address.
	// The icon is located directly after the banner.
	uint32_t iconaddr = direntry.iconaddr;
	switch (direntry.bannerfmt & CARD_BANNER_MASK) {
		case CARD_BANNER_CI:
			// CI8 banner.
			iconaddr += (CARD_BANNER_W * CARD_BANNER_H * 1);
			iconaddr += (256 * 2);	// RGB5A3 palette
			break;
		case CARD_BANNER_RGB:
			// RGB5A3 banner.
			iconaddr += (CARD_BANNER_W * CARD_BANNER_H * 2);
			break;
		default:
			// No banner.
			break;
	}

	// Calculate the icon sizes.
	unsigned int iconsizetotal = 0;
	bool is_CI8_shared = false;
	uint16_t iconfmt = direntry.iconfmt;
	uint16_t iconspeed = direntry.iconspeed;
	for (int i = 0; i < CARD_MAXICONS; i++, iconfmt >>= 2, iconspeed >>= 2) {
		if ((iconspeed & CARD_SPEED_MASK) == CARD_SPEED_END) {
			// End of the icons.
			break;
		}

		switch (iconfmt & CARD_ICON_MASK) {
			case CARD_ICON_RGB:
				// RGB5A3
				iconsizetotal += (CARD_ICON_W * CARD_ICON_H * 2);
				break;

			case CARD_ICON_CI_UNIQUE:
				// CI8 with a unique palette.
				// Palette is located immediately after the icon.
				iconsizetotal += (CARD_ICON_W * CARD_ICON_H * 1) + (256*2);
				break;

			case CARD_ICON_CI_SHARED:
				// CI8 with a shared palette.
				// Palette is located after *all* of the icons.
				iconsizetotal += (CARD_ICON_W * CARD_ICON_H * 1);
				is_CI8_shared = true;
				break;

			default:
				// No icon.
				break;
		}
	}

	if (is_CI8_shared) {
		// CARD_ICON_CI_SHARED has a palette stored
		// after all of the icons.
		iconsizetotal += (256*2);
	}

	// Load the icon data.
	// TODO: Only read the first frame unless specifically requested?
	uint8_t *icondata = (uint8_t*)malloc(iconsizetotal);
	this->file->seek(dataOffset + iconaddr);
	size_t size = this->file->read(icondata, iconsizetotal);
	if (size != iconsizetotal) {
		// Error reading the icon data.
		free(icondata);
		return nullptr;
	}

	const uint16_t *pal_CI8_shared = nullptr;
	if (is_CI8_shared) {
		// Shared CI8 palette is at the end of the data.
		pal_CI8_shared = reinterpret_cast<const uint16_t*>(
			&icondata[iconsizetotal - (256*2)]);
	}

	this->iconAnimData = new IconAnimData();
	iconAnimData->count = 0;

	unsigned int iconaddr_cur = 0;
	iconfmt = direntry.iconfmt;
	iconspeed = direntry.iconspeed;
	for (int i = 0; i < CARD_MAXICONS; i++, iconfmt >>= 2, iconspeed >>= 2) {
		const int delay = (iconspeed & CARD_SPEED_MASK);
		if (delay == CARD_SPEED_END) {
			// End of the icons.
			break;
		}

		// Icon delay.
		// Using 125ms for the fastest speed.
		// TODO: Verify this?
		iconAnimData->delays[i].numer = (uint16_t)delay;
		iconAnimData->delays[i].denom = 8;
		iconAnimData->delays[i].ms = delay * 125;

		switch (iconfmt & CARD_ICON_MASK) {
			case CARD_ICON_RGB: {
				// RGB5A3
				const unsigned int iconsize = CARD_ICON_W * CARD_ICON_H * 2;
				iconAnimData->frames[i] = ImageDecoder::fromGcnRGB5A3(
					CARD_ICON_W, CARD_ICON_H,
					reinterpret_cast<const uint16_t*>(&icondata[iconaddr_cur]),
					iconsize);
				iconaddr_cur += iconsize;
				break;
			}

			case CARD_ICON_CI_UNIQUE: {
				// CI8 with a unique palette.
				// Palette is located immediately after the icon.
				const unsigned int iconsize = CARD_ICON_W * CARD_ICON_H * 1;
				iconAnimData->frames[i] = ImageDecoder::fromGcnCI8(
					CARD_ICON_W, CARD_ICON_H,
					&icondata[iconaddr_cur], iconsize,
					reinterpret_cast<const uint16_t*>(&icondata[iconaddr_cur+iconsize]), 256*2);
				iconaddr_cur += iconsize + (256*2);
				break;
			}

			case CARD_ICON_CI_SHARED: {
				const unsigned int iconsize = CARD_ICON_W * CARD_ICON_H * 1;
				iconAnimData->frames[i] = ImageDecoder::fromGcnCI8(
					CARD_ICON_W, CARD_ICON_H,
					&icondata[iconaddr_cur], iconsize,
					pal_CI8_shared, 256*2);
				iconaddr_cur += iconsize;
				break;
			}

			default:
				// No icon.
				// Add a nullptr as a placeholder.
				iconAnimData->frames[i] = 0;
				break;
		}

		iconAnimData->count++;
	}

	// Free the icon data.
	free(icondata);

	// NOTE: We're not deleting iconAnimData even if we only have
	// a single icon because iconAnimData() will call loadIcon()
	// if iconAnimData is nullptr.

	// Set up the icon animation sequence.
	int idx = 0;
	for (int i = 0; i < iconAnimData->count; i++, idx++) {
		iconAnimData->seq_index[idx] = i;
	}
	if (direntry.bannerfmt & CARD_ANIM_MASK) {
		// "Bounce" the icon.
		for (int i = iconAnimData->count-2; i > 0; i--, idx++) {
			iconAnimData->seq_index[idx] = i;
			iconAnimData->delays[idx] = iconAnimData->delays[i];
		}
	}
	iconAnimData->seq_count = idx;

	// Return the first frame.
	return iconAnimData->frames[0];
}

/**
 * Load the save file's banner.
 * @return Banner, or nullptr on error.
 */
const rp_image *GameCubeSavePrivate::loadBanner(void)
{
	if (img_banner) {
		// Banner is already loaded.
		return img_banner;
	} else if (!this->file || !this->isValid) {
		// Can't load the banner.
		return nullptr;
	}

	// Banner is located at direntry.iconaddr.
	// Determine the banner format and size.
	uint32_t bannersize;
	switch (direntry.bannerfmt & CARD_BANNER_MASK) {
		case CARD_BANNER_CI:
			// CI8 banner.
			bannersize = (CARD_BANNER_W * CARD_BANNER_H * 1);
			break;
		case CARD_BANNER_RGB:
			bannersize = (CARD_BANNER_W * CARD_BANNER_H * 2);
			break;
		default:
			// No banner.
			return nullptr;
	}

	// Read the banner data.
	static const int MAX_BANNER_SIZE = (CARD_BANNER_W * CARD_BANNER_H * 2);
	uint8_t bannerbuf[MAX_BANNER_SIZE];
	this->file->seek(this->dataOffset + direntry.iconaddr);
	size_t size = this->file->read(bannerbuf, bannersize);
	if (size != bannersize) {
		// Error reading the banner data.
		return nullptr;
	}

	if ((direntry.bannerfmt & CARD_BANNER_MASK) == CARD_BANNER_RGB) {
		// Convert the banner from GCN RGB5A3 format to ARGB32.
		img_banner = ImageDecoder::fromGcnRGB5A3(CARD_BANNER_W, CARD_BANNER_H,
			reinterpret_cast<const uint16_t*>(bannerbuf), bannersize);
	} else {
		// Read the palette data.
		uint16_t palbuf[256];
		this->file->seek(this->dataOffset + direntry.iconaddr + bannersize);
		size = this->file->read(palbuf, sizeof(palbuf));
		if (size != sizeof(palbuf)) {
			// Error reading the palette data.
			return nullptr;
		}

		// Convert the banner from GCN CI8 format to CI8.
		img_banner = ImageDecoder::fromGcnCI8(CARD_BANNER_W, CARD_BANNER_H,
			bannerbuf, bannersize, palbuf, sizeof(palbuf));
	}

	return img_banner;
}

/** GameCubeSave **/

/**
 * Read a Nintendo GameCube save file.
 *
 * A save file must be opened by the caller. The file handle
 * will be dup()'d and must be kept open in order to load
 * data from the disc image.
 *
 * To close the file, either delete this object or call close().
 *
 * NOTE: Check isValid() to determine if this is a valid ROM.
 *
 * @param file Open disc image.
 */
GameCubeSave::GameCubeSave(IRpFile *file)
	: super(new GameCubeSavePrivate(this, file))
{
	// This class handles save files.
	RP_D(GameCubeSave);
	d->fileType = FTYPE_SAVE_FILE;

	if (!d->file) {
		// Could not dup() the file handle.
		return;
	}

	// Read the save file header.
	uint8_t header[1024];
	d->file->rewind();
	size_t size = d->file->read(&header, sizeof(header));
	if (size != sizeof(header))
		return;

	// Check if this disc image is supported.
	DetectInfo info;
	info.header.addr = 0;
	info.header.size = sizeof(header);
	info.header.pData = header;
	info.ext = nullptr;	// Not needed for GCN save files.
	info.szFile = d->file->size();
	d->saveType = isRomSupported_static(&info);

	// Save the directory entry for later.
	uint32_t gciOffset;	// offset to GCI header
	switch (d->saveType) {
		case GameCubeSavePrivate::SAVE_TYPE_GCI:
			gciOffset = 0;
			break;
		case GameCubeSavePrivate::SAVE_TYPE_GCS:
			gciOffset = 0x110;
			break;
		case GameCubeSavePrivate::SAVE_TYPE_SAV:
			gciOffset = 0x80;
			break;
		default:
			// Unknown save type.
			d->saveType = GameCubeSavePrivate::SAVE_TYPE_UNKNOWN;
			return;
	}

	d->isValid = true;

	// Save the directory entry for later.
	memcpy(&d->direntry, &header[gciOffset], sizeof(d->direntry));
	d->byteswap_direntry(&d->direntry, (GameCubeSavePrivate::SaveType)d->saveType);
	// Data area offset.
	d->dataOffset = gciOffset + 0x40;
}

/** ROM detection functions. **/

/**
 * Is a ROM image supported by this class?
 * @param info DetectInfo containing ROM detection information.
 * @return Class-specific system ID (>= 0) if supported; -1 if not.
 */
int GameCubeSave::isRomSupported_static(const DetectInfo *info)
{
	assert(info != nullptr);
	assert(info->header.pData != nullptr);
	assert(info->header.addr == 0);
	if (!info || !info->header.pData ||
	    info->header.addr != 0 ||
	    info->header.size < 1024)
	{
		// Either no detection information was specified,
		// or the header is too small.
		return -1;
	}

	if (info->szFile > ((8192*2043) + 0x110)) {
		// File is larger than 2043 blocks, plus the size
		// of the largest header supported.
		// This isn't possible on an actual memory card.
		return -1;
	}

	// Check for GCS. (GameShark)
	static const uint8_t gcs_magic[] = {'G','C','S','A','V','E'};
	if (!memcmp(info->header.pData, gcs_magic, sizeof(gcs_magic))) {
		// Is the size correct?
		// GCS files are a multiple of 8 KB, plus 336 bytes:
		// - 272 bytes: GCS-specific header.
		// - 64 bytes: CARD directory entry.
		// TODO: GCS has a user-specified description field and other stuff.
		const uint32_t data_size = (uint32_t)(info->szFile - 336);
		if (data_size % 8192 == 0) {
			// Check the CARD directory entry.
			if (GameCubeSavePrivate::isCardDirEntry(
			    &info->header.pData[0x110], data_size, GameCubeSavePrivate::SAVE_TYPE_GCS))
			{
				// This is a GCS file.
				return GameCubeSavePrivate::SAVE_TYPE_GCS;
			}
		}
	}

	// Check for SAV. (MaxDrive)
	static const uint8_t sav_magic[] = {'D','A','T','E','L','G','C','_','S','A','V','E',0,0,0,0};
	if (!memcmp(info->header.pData, sav_magic, sizeof(sav_magic))) {
		// Is the size correct?
		// SAVE files are a multiple of 8 KB, plus 192 bytes:
		// - 128 bytes: SAV-specific header.
		// - 64 bytes: CARD directory entry.
		// TODO: SAV has a copy of the description, plus other fields?
		const uint32_t data_size = (uint32_t)(info->szFile - 192);
		if (data_size % 8192 == 0) {
			// Check the CARD directory entry.
			if (GameCubeSavePrivate::isCardDirEntry(
			    &info->header.pData[0x80], data_size, GameCubeSavePrivate::SAVE_TYPE_SAV))
			{
				// This is a GCS file.
				return GameCubeSavePrivate::SAVE_TYPE_SAV;
			}
		}
	}

	// Check for GCI.
	// GCI files are a multiple of 8 KB, plus 64 bytes:
	// - 64 bytes: CARD directory entry.
	const uint32_t data_size = (uint32_t)(info->szFile - 64);
	if (data_size % 8192 == 0) {
		// Check the CARD directory entry.
		if (GameCubeSavePrivate::isCardDirEntry(
		    info->header.pData, data_size, GameCubeSavePrivate::SAVE_TYPE_GCI))
		{
			// This is a GCI file.
			return GameCubeSavePrivate::SAVE_TYPE_GCI;
		}
	}

	// Not supported.
	return -1;
}

/**
 * Is a ROM image supported by this object?
 * @param info DetectInfo containing ROM detection information.
 * @return Class-specific system ID (>= 0) if supported; -1 if not.
 */
int GameCubeSave::isRomSupported(const DetectInfo *info) const
{
	return isRomSupported_static(info);
}

/**
 * Get the name of the system the loaded ROM is designed for.
 * @param type System name type. (See the SystemName enum.)
 * @return System name, or nullptr if type is invalid.
 */
const rp_char *GameCubeSave::systemName(uint32_t type) const
{
	RP_D(const GameCubeSave);
	if (!d->isValid || !isSystemNameTypeValid(type))
		return nullptr;

	// Bits 0-1: Type. (short, long, abbreviation)
	static const rp_char *const sysNames[4] = {
		// FIXME: "NGC" in Japan?
		_RP("Nintendo GameCube"), _RP("GameCube"), _RP("GCN"), nullptr
	};

	return sysNames[type & SYSNAME_TYPE_MASK];
}

/**
 * Get a list of all supported file extensions.
 * This is to be used for file type registration;
 * subclasses don't explicitly check the extension.
 *
 * NOTE: The extensions do not include the leading dot,
 * e.g. "bin" instead of ".bin".
 *
 * NOTE 2: The array and the strings in the array should
 * *not* be freed by the caller.
 *
 * @return NULL-terminated array of all supported file extensions, or nullptr on error.
 */
const rp_char *const *GameCubeSave::supportedFileExtensions_static(void)
{
	static const rp_char *const exts[] = {
		_RP(".gci"),	// USB Memory Adapter
		_RP(".gcs"),	// GameShark
		_RP(".sav"),	// MaxDrive (TODO: Too generic?)

		nullptr
	};
	return exts;
}

/**
 * Get a list of all supported file extensions.
 * This is to be used for file type registration;
 * subclasses don't explicitly check the extension.
 *
 * NOTE: The extensions do not include the leading dot,
 * e.g. "bin" instead of ".bin".
 *
 * NOTE 2: The strings in the std::vector should *not*
 * be freed by the caller.
 *
 * @return List of all supported file extensions.
 */
const rp_char *const *GameCubeSave::supportedFileExtensions(void) const
{
	return supportedFileExtensions_static();
}

/**
 * Get a bitfield of image types this class can retrieve.
 * @return Bitfield of supported image types. (ImageTypesBF)
 */
uint32_t GameCubeSave::supportedImageTypes_static(void)
{
	return IMGBF_INT_ICON | IMGBF_INT_BANNER;
}

/**
 * Get a bitfield of image types this class can retrieve.
 * @return Bitfield of supported image types. (ImageTypesBF)
 */
uint32_t GameCubeSave::supportedImageTypes(void) const
{
	return supportedImageTypes_static();
}

/**
 * Get a list of all available image sizes for the specified image type.
 *
 * The first item in the returned vector is the "default" size.
 * If the width/height is 0, then an image exists, but the size is unknown.
 *
 * @param imageType Image type.
 * @return Vector of available image sizes, or empty vector if no images are available.
 */
std::vector<RomData::ImageSizeDef> GameCubeSave::supportedImageSizes_static(ImageType imageType)
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
		case IMG_INT_BANNER: {
			static const ImageSizeDef sz_INT_BANNER[] = {
				{nullptr, 96, 32, 0},
			};
			return vector<ImageSizeDef>(sz_INT_BANNER,
				sz_INT_BANNER + ARRAY_SIZE(sz_INT_BANNER));
		}
		default:
			break;
	}

	// Unsupported image type.
	return std::vector<ImageSizeDef>();
}

/**
 * Get a list of all available image sizes for the specified image type.
 *
 * The first item in the returned vector is the "default" size.
 * If the width/height is 0, then an image exists, but the size is unknown.
 *
 * @param imageType Image type.
 * @return Vector of available image sizes, or empty vector if no images are available.
 */
std::vector<RomData::ImageSizeDef> GameCubeSave::supportedImageSizes(ImageType imageType) const
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
uint32_t GameCubeSave::imgpf(ImageType imageType) const
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
int GameCubeSave::loadFieldData(void)
{
	RP_D(GameCubeSave);
	if (d->fields->isDataLoaded()) {
		// Field data *has* been loaded...
		return 0;
	} else if (!d->file || !d->file->isOpen()) {
		// File isn't open.
		return -EBADF;
	} else if (!d->isValid || d->saveType < 0 || d->dataOffset < 0) {
		// Unknown save file type.
		return -EIO;
	}

	// Save file header is read and byteswapped in the constructor.
	d->fields->reserve(8);	// Maximum of 8 fields.

	// Game ID.
	// Replace any non-printable characters with underscores.
	// (NDDEMO has ID6 "00\0E01".)
	char id6[7];
	for (int i = 0; i < 6; i++) {
		id6[i] = (isprint(d->direntry.id6[i])
			? d->direntry.id6[i]
			: '_');
	}
	id6[6] = 0;
	d->fields->addField_string(_RP("Game ID"), latin1_to_rp_string(id6, 6));

	// Look up the publisher.
	const rp_char *publisher = NintendoPublishers::lookup(d->direntry.company);
	d->fields->addField_string(_RP("Publisher"),
		publisher ? publisher : _RP("Unknown"));

	// Filename.
	// TODO: Remove trailing nulls and/or spaces.
	// (Implicit length version of cp1252_sjis_to_rp_string()?)
	d->fields->addField_string(_RP("Filename"),
		cp1252_sjis_to_rp_string(
			d->direntry.filename, sizeof(d->direntry.filename)));

	// Description.
	char desc_buf[64];
	d->file->seek(d->dataOffset + d->direntry.commentaddr);
	size_t size = d->file->read(desc_buf, sizeof(desc_buf));
	if (size == sizeof(desc_buf)) {
		// Add the description.
		// NOTE: Some games have garbage after the first NULL byte
		// in the two description fields, which prevents the rest
		// of the field from being displayed.

		// Check for a NULL byte in the game description.
		int desc_len = 32;
		const char *null_pos = static_cast<const char*>(memchr(desc_buf, 0, 32));
		if (null_pos) {
			// Found a NULL byte.
			desc_len = (int)(null_pos - desc_buf);
		}
		rp_string desc = cp1252_sjis_to_rp_string(desc_buf, desc_len);
		desc += _RP_CHR('\n');

		// Check for a NULL byte in the file description.
		null_pos = static_cast<const char*>(memchr(&desc_buf[32], 0, 32));
		if (null_pos) {
			// Found a NULL byte.
			desc_len = (int)(null_pos - desc_buf - 32);
		}
		desc += cp1252_sjis_to_rp_string(&desc_buf[32], desc_len);

		d->fields->addField_string(_RP("Description"), desc);
	}

	// Last Modified timestamp.
	d->fields->addField_dateTime(_RP("Last Modified"),
		(int64_t)d->direntry.lastmodified + GC_UNIX_TIME_DIFF,
		RomFields::RFT_DATETIME_HAS_DATE |
		RomFields::RFT_DATETIME_HAS_TIME |
		RomFields::RFT_DATETIME_IS_UTC	// GameCube doesn't support timezones.
		);

	// File mode.
	rp_char file_mode[5];
	file_mode[0] = ((d->direntry.permission & CARD_ATTRIB_GLOBAL) ? _RP_CHR('G') : _RP_CHR('-'));
	file_mode[1] = ((d->direntry.permission & CARD_ATTRIB_NOMOVE) ? _RP_CHR('M') : _RP_CHR('-'));
	file_mode[2] = ((d->direntry.permission & CARD_ATTRIB_NOCOPY) ? _RP_CHR('C') : _RP_CHR('-'));
	file_mode[3] = ((d->direntry.permission & CARD_ATTRIB_PUBLIC) ? _RP_CHR('P') : _RP_CHR('-'));
	file_mode[4] = 0;
	d->fields->addField_string(_RP("Mode"), file_mode, RomFields::STRF_MONOSPACE);

	// Copy count.
	d->fields->addField_string_numeric(_RP("Copy Count"), d->direntry.copytimes);
	// Blocks.
	d->fields->addField_string_numeric(_RP("Blocks"), d->direntry.length);

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
int GameCubeSave::loadInternalImage(ImageType imageType, const rp_image **pImage)
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

	RP_D(GameCubeSave);
	switch (imageType) {
		case IMG_INT_ICON:
			if (d->iconAnimData) {
				// Return the first icon frame.
				// NOTE: GCN save icon animations are always
				// sequential, so we can use a shortcut here.
				*pImage = d->iconAnimData->frames[0];
				return 0;
			}
			break;
		case IMG_INT_BANNER:
			if (d->img_banner) {
				// Banner is loaded.
				*pImage = d->img_banner;
				return 0;
			}
			break;
		default:
			// Unsupported image type.
			*pImage = nullptr;
			return 0;
	}

	if (!d->file) {
		// File isn't open.
		return -EBADF;
	} else if (!d->isValid) {
		// Save file isn't valid.
		return -EIO;
	}

	// Load the image.
	switch (imageType) {
		case IMG_INT_ICON:
			*pImage = d->loadIcon();
			break;
		case IMG_INT_BANNER:
			*pImage = d->loadBanner();
			break;
		default:
			// Unsupported.
			return -ENOENT;
	}

	// TODO: -ENOENT if the file doesn't actually have an icon/banner.
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
const IconAnimData *GameCubeSave::iconAnimData(void) const
{
	RP_D(const GameCubeSave);
	if (!d->iconAnimData) {
		// Load the icon.
		if (!const_cast<GameCubeSavePrivate*>(d)->loadIcon()) {
			// Error loading the icon.
			return nullptr;
		}
		if (!d->iconAnimData) {
			// Still no icon...
			return nullptr;
		}
	}

	if (d->iconAnimData->count <= 1 ||
	    d->iconAnimData->seq_count <= 1)
	{
		// Not an animated icon.
		return nullptr;
	}

	// Return the icon animation data.
	return d->iconAnimData;
}

}
