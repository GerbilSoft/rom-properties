/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * GameCubeSave.hpp: Nintendo GameCube save file reader.                   *
 *                                                                         *
 * Copyright (c) 2016-2022 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "GameCubeSave.hpp"
#include "data/NintendoPublishers.hpp"
#include "gcn_card.h"

// librpbase, librpfile, librptexture
#include "librpbase/SystemRegion.hpp"
#include "librptexture/decoder/ImageDecoder_GCN.hpp"
using namespace LibRpBase;
using LibRpFile::IRpFile;
using namespace LibRpTexture;

// C++ STL classes
#include <string>
#include <vector>
using std::u8string;
using std::vector;

namespace LibRomData {

class GameCubeSavePrivate final : public RomDataPrivate
{
	public:
		GameCubeSavePrivate(GameCubeSave *q, IRpFile *file);
		virtual ~GameCubeSavePrivate();

	private:
		typedef RomDataPrivate super;
		RP_DISABLE_COPY(GameCubeSavePrivate)

	public:
		/** RomDataInfo **/
		static const char8_t *const exts[];
		static const char *const mimeTypes[];
		static const RomDataInfo romDataInfo;

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
		enum class SaveType {
			Unknown	= -1,

			GCI	= 0,	// USB Memory Adapter
			GCS	= 1,	// GameShark
			SAV	= 2,	// MaxDrive

			Max
		};
		SaveType saveType;

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

ROMDATA_IMPL(GameCubeSave)
ROMDATA_IMPL_IMG(GameCubeSave)

/** GameCubeSavePrivate **/

/* RomDataInfo */
const char8_t *const GameCubeSavePrivate::exts[] = {
	U8(".gci"),	// USB Memory Adapter
	U8(".gcs"),	// GameShark
	U8(".sav"),	// MaxDrive (TODO: Too generic?)

	nullptr
};
const char *const GameCubeSavePrivate::mimeTypes[] = {
	// Unofficial MIME types.
	// TODO: Get these upstreamed on FreeDesktop.org.
	"application/x-gamecube-save",

	nullptr
};
const RomDataInfo GameCubeSavePrivate::romDataInfo = {
	"GameCubeSave", exts, mimeTypes
};

GameCubeSavePrivate::GameCubeSavePrivate(GameCubeSave *q, IRpFile *file)
	: super(q, file, &romDataInfo)
	, img_banner(nullptr)
	, iconAnimData(nullptr)
	, saveType(SaveType::Unknown)
	, dataOffset(-1)
{
	// Clear the directory entry.
	memset(&direntry, 0, sizeof(direntry));
}

GameCubeSavePrivate::~GameCubeSavePrivate()
{
	UNREF(img_banner);
	UNREF(iconAnimData);
}

/**
 * Byteswap a card_direntry struct.
 * @param direntry card_direntry struct.
 * @param saveType Apply quirks for a specific save type.
 */
void GameCubeSavePrivate::byteswap_direntry(card_direntry *direntry, SaveType saveType)
{
	if (saveType == SaveType::SAV) {
		// Swap 16-bit values at 0x2E through 0x40.
		// Also 0x06 (pad_00 / bannerfmt).
		// Reference: https://github.com/dolphin-emu/dolphin/blob/master/Source/Core/Core/HW/GCMemcard.cpp
		uint16_t *const u16ptr = reinterpret_cast<uint16_t*>(direntry);
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
		if (!ISALNUM(direntry->id6[i])) {
			// Non-alphanumeric character.
			return false;
		}
	}

	// Padding should be 0xFF.
	if (saveType == SaveType::SAV) {
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

	if (direntry->pad_01 != cpu_to_be16(0xFFFF)) {
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
		case SaveType::GCS:
			// Just check for >= 1.
			length = be16_to_cpu(direntry->length);
			if (length == 0) {
				// Incorrect block count.
				return false;
			}
			break;

		case SaveType::SAV:
			// SAV: Field is little-endian
			length = le16_to_cpu(direntry->length);
			if (length * 8192 != data_size) {
				// Incorrect block count.
				return false;
			}
			break;

		case SaveType::GCI:
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
	if (saveType != SaveType::SAV) {
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
	for (unsigned int i = 0; i < CARD_MAXICONS; i++, iconfmt >>= 2, iconspeed >>= 2) {
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
	auto icondata = aligned_uptr<uint8_t>(16, iconsizetotal);
	size_t size = file->seekAndRead(dataOffset + iconaddr, icondata.get(), iconsizetotal);
	if (size != iconsizetotal) {
		// Seek and/or read error.
		return nullptr;
	}

	const uint16_t *pal_CI8_shared = nullptr;
	if (is_CI8_shared) {
		// Shared CI8 palette is at the end of the data.
		pal_CI8_shared = reinterpret_cast<const uint16_t*>(
			icondata.get() + (iconsizetotal - (256*2)));
	}

	this->iconAnimData = new IconAnimData();
	iconAnimData->count = 0;

	unsigned int iconaddr_cur = 0;
	iconfmt = direntry.iconfmt;
	iconspeed = direntry.iconspeed;
	for (int i = 0; i < CARD_MAXICONS; i++, iconfmt >>= 2, iconspeed >>= 2) {
		const unsigned int delay = (iconspeed & CARD_SPEED_MASK);
		if (delay == CARD_SPEED_END) {
			// End of the icons.
			break;
		}

		// Icon delay.
		// Using 125ms for the fastest speed.
		// TODO: Verify this?
		iconAnimData->delays[i].numer = static_cast<uint16_t>(delay);
		iconAnimData->delays[i].denom = 8;
		iconAnimData->delays[i].ms = delay * 125;

		switch (iconfmt & CARD_ICON_MASK) {
			case CARD_ICON_RGB: {
				// RGB5A3
				static const size_t iconsize = CARD_ICON_W * CARD_ICON_H * 2;
				iconAnimData->frames[i] = ImageDecoder::fromGcn16(
					ImageDecoder::PixelFormat::RGB5A3, CARD_ICON_W, CARD_ICON_H,
					reinterpret_cast<const uint16_t*>(icondata.get() + iconaddr_cur),
					iconsize);
				iconaddr_cur += iconsize;
				break;
			}

			case CARD_ICON_CI_UNIQUE: {
				// CI8 with a unique palette.
				// Palette is located immediately after the icon.
				static const size_t iconsize = CARD_ICON_W * CARD_ICON_H * 1;
				iconAnimData->frames[i] = ImageDecoder::fromGcnCI8(
					CARD_ICON_W, CARD_ICON_H,
					icondata.get() + iconaddr_cur, iconsize,
					reinterpret_cast<const uint16_t*>(icondata.get() + iconaddr_cur + iconsize), 256*2);
				iconaddr_cur += iconsize + (256*2);
				break;
			}

			case CARD_ICON_CI_SHARED: {
				static const size_t iconsize = CARD_ICON_W * CARD_ICON_H * 1;
				iconAnimData->frames[i] = ImageDecoder::fromGcnCI8(
					CARD_ICON_W, CARD_ICON_H,
					icondata.get() + iconaddr_cur, iconsize,
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

	// NOTE: We're not deleting iconAnimData even if we only have
	// a single icon because iconAnimData() will call loadIcon()
	// if iconAnimData is nullptr.

	// Set up the icon animation sequence.
	// FIXME: This isn't done correctly if blank frames are present
	// and the icon uses the "bounce" animation.
	// 'rpcli -a' fails as a result.
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
	size_t bannersize;
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
	size_t size = file->seekAndRead(dataOffset + direntry.iconaddr,
					bannerbuf, bannersize);
	if (size != bannersize) {
		// Seek and/or read error.
		return nullptr;
	}

	if ((direntry.bannerfmt & CARD_BANNER_MASK) == CARD_BANNER_RGB) {
		// Convert the banner from GCN RGB5A3 format to ARGB32.
		img_banner = ImageDecoder::fromGcn16(
			ImageDecoder::PixelFormat::RGB5A3, CARD_BANNER_W, CARD_BANNER_H,
			reinterpret_cast<const uint16_t*>(bannerbuf), bannersize);
	} else {
		// Read the palette data.
		uint16_t palbuf[256];
		size = file->seekAndRead(dataOffset + direntry.iconaddr + bannersize,
					 palbuf, sizeof(palbuf));
		if (size != sizeof(palbuf)) {
			// Seek and/or read error.
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
 * will be ref()'d and must be kept open in order to load
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
	d->mimeType = "application/x-gamecube-save";	// unofficial, not on fd.o
	d->fileType = FileType::SaveFile;

	if (!d->file) {
		// Could not ref() the file handle.
		return;
	}

	// Read the save file header.
	uint8_t header[1024];
	d->file->rewind();
	size_t size = d->file->read(&header, sizeof(header));
	if (size != sizeof(header)) {
		UNREF_AND_NULL_NOCHK(d->file);
		return;
	}

	// Check if this disc image is supported.
	const DetectInfo info = {
		{0, sizeof(header), header},
		nullptr,	// ext (not needed for GameCubeSave)
		d->file->size()	// szFile
	};
	d->saveType = static_cast<GameCubeSavePrivate::SaveType>(isRomSupported_static(&info));

	// Save the directory entry for later.
	uint32_t gciOffset;	// offset to GCI header
	switch (d->saveType) {
		case GameCubeSavePrivate::SaveType::GCI:
			gciOffset = 0;
			break;
		case GameCubeSavePrivate::SaveType::GCS:
			gciOffset = 0x110;
			break;
		case GameCubeSavePrivate::SaveType::SAV:
			gciOffset = 0x80;
			break;
		default:
			// Unknown save type.
			d->saveType = GameCubeSavePrivate::SaveType::Unknown;
			UNREF_AND_NULL_NOCHK(d->file);
			return;
	}

	d->isValid = true;

	// Save the directory entry for later.
	memcpy(&d->direntry, &header[gciOffset], sizeof(d->direntry));
	d->byteswap_direntry(&d->direntry, static_cast<GameCubeSavePrivate::SaveType>(d->saveType));
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
		return static_cast<int>(GameCubeSavePrivate::SaveType::Unknown);
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
		const uint32_t data_size = static_cast<uint32_t>(info->szFile - 336);
		if (data_size % 8192 == 0) {
			// Check the CARD directory entry.
			if (GameCubeSavePrivate::isCardDirEntry(
			    &info->header.pData[0x110], data_size, GameCubeSavePrivate::SaveType::GCS))
			{
				// This is a GCS file.
				return static_cast<int>(GameCubeSavePrivate::SaveType::GCS);
			}
		}
	}

	// Check for SAV. (MaxDrive)
	static const uint8_t sav_magic[] = "DATELGC_SAVE\x00\x00\x00\x00";
	if (!memcmp(info->header.pData, sav_magic, sizeof(sav_magic)-1)) {
		// Is the size correct?
		// SAVE files are a multiple of 8 KB, plus 192 bytes:
		// - 128 bytes: SAV-specific header.
		// - 64 bytes: CARD directory entry.
		// TODO: SAV has a copy of the description, plus other fields?
		const uint32_t data_size = static_cast<uint32_t>(info->szFile - 192);
		if (data_size % 8192 == 0) {
			// Check the CARD directory entry.
			if (GameCubeSavePrivate::isCardDirEntry(
			    &info->header.pData[0x80], data_size, GameCubeSavePrivate::SaveType::SAV))
			{
				// This is a GCS file.
				return static_cast<int>(GameCubeSavePrivate::SaveType::SAV);
			}
		}
	}

	// Check for GCI.
	// GCI files are a multiple of 8 KB, plus 64 bytes:
	// - 64 bytes: CARD directory entry.
	const uint32_t data_size = static_cast<uint32_t>(info->szFile - 64);
	if (data_size % 8192 == 0) {
		// Check the CARD directory entry.
		if (GameCubeSavePrivate::isCardDirEntry(
		    info->header.pData, data_size, GameCubeSavePrivate::SaveType::GCI))
		{
			// This is a GCI file.
			return static_cast<int>(GameCubeSavePrivate::SaveType::GCI);
		}
	}

	// Not supported.
	return static_cast<int>(GameCubeSavePrivate::SaveType::Unknown);
}

/**
 * Get the name of the system the loaded ROM is designed for.
 * @param type System name type. (See the SystemName enum.)
 * @return System name, or nullptr if type is invalid.
 */
const char *GameCubeSave::systemName(unsigned int type) const
{
	RP_D(const GameCubeSave);
	if (!d->isValid || !isSystemNameTypeValid(type))
		return nullptr;

	// GameCube has the same name worldwide, so we can
	// ignore the region selection.
	static_assert(SYSNAME_TYPE_MASK == 3,
		"GameCubeSave::systemName() array index optimization needs to be updated.");

	// Bits 0-1: Type. (long, short, abbreviation)
	static const char *const sysNames[4] = {
		"Nintendo GameCube", "GameCube", "GCN", nullptr
	};

	// Special check for GCN abbreviation in Japan.
	if ((type & SYSNAME_REGION_MASK) == SYSNAME_REGION_ROM_LOCAL) {
		// Localized system name.
		if ((type & SYSNAME_TYPE_MASK) == SYSNAME_TYPE_ABBREVIATION) {
			// GameCube abbreviation.
			// If this is Japan or South Korea, use "NGC".
			const uint32_t cc = SystemRegion::getCountryCode();
			if (cc == 'JP' || cc == 'KR') {
				return "NGC";
			}
		}
	}

	return sysNames[type & SYSNAME_TYPE_MASK];
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
 * Get a list of all available image sizes for the specified image type.
 *
 * The first item in the returned vector is the "default" size.
 * If the width/height is 0, then an image exists, but the size is unknown.
 *
 * @param imageType Image type.
 * @return Vector of available image sizes, or empty vector if no images are available.
 */
vector<RomData::ImageSizeDef> GameCubeSave::supportedImageSizes_static(ImageType imageType)
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
uint32_t GameCubeSave::imgpf(ImageType imageType) const
{
	ASSERT_imgpf(imageType);

	uint32_t ret = 0;
	switch (imageType) {
		case IMG_INT_ICON: {
			RP_D(const GameCubeSave);
			// Use nearest-neighbor scaling when resizing.
			// Also, need to check if this is an animated icon.
			const_cast<GameCubeSavePrivate*>(d)->loadIcon();
			if (d->iconAnimData && d->iconAnimData->count > 1) {
				// Animated icon.
				ret = IMGPF_RESCALE_NEAREST | IMGPF_ICON_ANIMATED;
			} else {
				// Not animated.
				ret = IMGPF_RESCALE_NEAREST;
			}
			break;
		}

		case IMG_INT_BANNER:
			// Use nearest-neighbor scaling.
			ret = IMGPF_RESCALE_NEAREST;
			break;

		default:
			break;
	}
	return ret;
}

/**
 * Load field data.
 * Called by RomData::fields() if the field data hasn't been loaded yet.
 * @return Number of fields read on success; negative POSIX error code on error.
 */
int GameCubeSave::loadFieldData(void)
{
	RP_D(GameCubeSave);
	if (!d->fields->empty()) {
		// Field data *has* been loaded...
		return 0;
	} else if (!d->file || !d->file->isOpen()) {
		// File isn't open.
		return -EBADF;
	} else if (!d->isValid || (int)d->saveType < 0 || d->dataOffset < 0) {
		// Unknown save file type.
		return -EIO;
	}

	// Save file header is read and byteswapped in the constructor.
	const card_direntry *const direntry = &d->direntry;
	d->fields->reserve(8);	// Maximum of 8 fields.

	// Game ID
	// Replace any non-printable characters with underscores.
	// (NDDEMO has ID6 "00\0E01".)
	char id6[7];
	for (int i = 0; i < 6; i++) {
		id6[i] = (ISPRINT(direntry->id6[i])
			? direntry->id6[i]
			: '_');
	}
	id6[6] = 0;
	d->fields->addField_string(C_("RomData", "Game ID"), latin1_to_utf8(id6, 6));

	// Publisher
	// TODO: Combine with GameCubePrivate::getPublisher().
	// FIXME: U8STRFIX
	const char8_t *const publisher_title = C_("RomData", "Publisher");
	const char8_t *publisher = NintendoPublishers::lookup(direntry->company);
	if (publisher) {
		d->fields->addField_string(publisher_title, publisher);
	} else {
		d->fields->addField_string(publisher_title, C_("RomData", "Unknown"));
	}

	// Filename
	// TODO: Remove trailing spaces.
	d->fields->addField_string(C_("GameCubeSave", "Filename"),
		cp1252_sjis_to_utf8(
			direntry->filename, sizeof(direntry->filename)));

	// Description
	union {
		struct {
			char desc[32];
			char file[32];
		};
		char full[64];
	} comment;
	size_t size = d->file->seekAndRead(d->dataOffset + direntry->commentaddr,
					   &comment, sizeof(comment));
	if (size == sizeof(comment)) {
		// Add the description.
		// NOTE: Some games have garbage after the first NULL byte
		// in the two description fields, which prevents the rest
		// of the field from being displayed.

		// Check for a NULL byte in the game description.
		size_t desc_len = sizeof(comment.desc);
		const char *null_pos = static_cast<const char*>(memchr(comment.desc, 0, desc_len));
		if (null_pos) {
			// Found a NULL byte.
			desc_len = null_pos - comment.desc;
		}
		u8string desc = cp1252_sjis_to_utf8(comment.desc, static_cast<int>(desc_len));
		desc += '\n';

		// Check for a NULL byte in the file description.
		desc_len = sizeof(comment.file);
		null_pos = static_cast<const char*>(memchr(comment.file, 0, desc_len));
		if (null_pos) {
			// Found a NULL byte.
			desc_len = null_pos - comment.file;
		}
		desc += cp1252_sjis_to_utf8(comment.file, static_cast<int>(desc_len));

		d->fields->addField_string(C_("GameCubeSave", "Description"), desc);
	}

	// Last Modified timestamp.
	d->fields->addField_dateTime(C_("GameCubeSave", "Last Modified"),
		static_cast<time_t>(direntry->lastmodified) + GC_UNIX_TIME_DIFF,
		RomFields::RFT_DATETIME_HAS_DATE |
		RomFields::RFT_DATETIME_HAS_TIME |
		RomFields::RFT_DATETIME_IS_UTC	// GameCube doesn't support timezones.
		);

	// File mode.
	char file_mode[5];
	file_mode[0] = ((direntry->permission & CARD_ATTRIB_GLOBAL) ? 'G' : '-');
	file_mode[1] = ((direntry->permission & CARD_ATTRIB_NOMOVE) ? 'M' : '-');
	file_mode[2] = ((direntry->permission & CARD_ATTRIB_NOCOPY) ? 'C' : '-');
	file_mode[3] = ((direntry->permission & CARD_ATTRIB_PUBLIC) ? 'P' : '-');
	file_mode[4] = 0;
	d->fields->addField_string(C_("GameCubeSave", "Mode"), file_mode, RomFields::STRF_MONOSPACE);

	// Copy count.
	d->fields->addField_string_numeric(C_("GameCubeSave", "Copy Count"), direntry->copytimes);
	// Blocks.
	d->fields->addField_string_numeric(C_("GameCubeSave", "Blocks"), direntry->length);

	// Finished reading the field data.
	return static_cast<int>(d->fields->count());
}

/**
 * Load metadata properties.
 * Called by RomData::metaData() if the field data hasn't been loaded yet.
 * @return Number of metadata properties read on success; negative POSIX error code on error.
 */
int GameCubeSave::loadMetaData(void)
{
	RP_D(GameCubeSave);
	if (d->metaData != nullptr) {
		// Metadata *has* been loaded...
		return 0;
	} else if (!d->file) {
		// File isn't open.
		return -EBADF;
	} else if (!d->isValid || (int)d->saveType < 0 || d->dataOffset < 0) {
		// Unknown disc image type.
		return -EIO;
	}

	// Save file header is read and byteswapped in the constructor.
	const card_direntry *const direntry = &d->direntry;

	// Create the metadata object.
	d->metaData = new RomMetaData();
	d->metaData->reserve(3);	// Maximum of 4 metadata properties.

	// Publisher
	// FIXME: U8STRFIX
	const char8_t *publisher = NintendoPublishers::lookup(direntry->company);
	if (publisher) {
		d->metaData->addMetaData_string(Property::Publisher, publisher);
	}

	// Description (using this as the Title)
	// TODO: Consolidate with loadFieldData()?
	char desc_buf[64];
	size_t size = d->file->seekAndRead(d->dataOffset + direntry->commentaddr,
					   desc_buf, sizeof(desc_buf));
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
			desc_len = static_cast<int>(null_pos - desc_buf);
		}
		u8string desc = cp1252_sjis_to_utf8(desc_buf, desc_len);
		desc += '\n';

		// Check for a NULL byte in the file description.
		null_pos = static_cast<const char*>(memchr(&desc_buf[32], 0, 32));
		if (null_pos) {
			// Found a NULL byte.
			desc_len = static_cast<int>(null_pos - desc_buf - 32);
		}
		desc += cp1252_sjis_to_utf8(&desc_buf[32], desc_len);

		d->metaData->addMetaData_string(Property::Title, desc);
	}

	// Last Modified timestamp
	// NOTE: Using "CreationDate".
	// TODO: Adjust for local timezone, since it's UTC.
	d->metaData->addMetaData_timestamp(Property::CreationDate,
		static_cast<time_t>(direntry->lastmodified) + GC_UNIX_TIME_DIFF);

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
int GameCubeSave::loadInternalImage(ImageType imageType, const rp_image **pImage)
{
	ASSERT_loadInternalImage(imageType, pImage);

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
 * The retrieved IconAnimData must be ref()'d by the caller if the
 * caller stores it instead of using it immediately.
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
