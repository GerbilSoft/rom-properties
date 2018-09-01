/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * WiiWIBN.hpp: Nintendo Wii save file banner reader.                      *
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

#include "WiiWIBN.hpp"
#include "librpbase/RomData_p.hpp"

#include "data/NintendoPublishers.hpp"
#include "gcn_card.h"
#include "wii_banner.h"

// librpbase
#include "librpbase/common.h"
#include "librpbase/byteswap.h"
#include "librpbase/aligned_malloc.h"
#include "librpbase/TextFuncs.hpp"
#include "librpbase/file/IRpFile.hpp"

#include "librpbase/img/rp_image.hpp"
#include "librpbase/img/ImageDecoder.hpp"
#include "librpbase/img/IconAnimData.hpp"

#include "libi18n/i18n.h"
using namespace LibRpBase;

// C includes. (C++ namespace)
#include "librpbase/ctypex.h"
#include <cassert>
#include <cerrno>
#include <cstring>

// C++ includes.
#include <string>
#include <vector>
using std::string;
using std::vector;

namespace LibRomData {

ROMDATA_IMPL(WiiWIBN)
ROMDATA_IMPL_IMG(WiiWIBN)

class WiiWIBNPrivate : public RomDataPrivate
{
	public:
		WiiWIBNPrivate(WiiWIBN *q, IRpFile *file);
		virtual ~WiiWIBNPrivate();

	private:
		typedef RomDataPrivate super;
		RP_DISABLE_COPY(WiiWIBNPrivate)

	public:
		// Internal images.
		rp_image *img_banner;

		// Animated icon data.
		IconAnimData *iconAnimData;

	public:
		// File header.
		Wii_WIBN_Header_t wibnHeader;

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

/** WiiWIBNPrivate **/

WiiWIBNPrivate::WiiWIBNPrivate(WiiWIBN *q, IRpFile *file)
	: super(q, file)
	, img_banner(nullptr)
	, iconAnimData(nullptr)
{
	// Clear the WIBN header struct.
	memset(&wibnHeader, 0, sizeof(wibnHeader));
}

WiiWIBNPrivate::~WiiWIBNPrivate()
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
 * Load the save file's icons.
 *
 * This will load all of the animated icon frames,
 * though only the first frame will be returned.
 *
 * @return Icon, or nullptr on error.
 */
const rp_image *WiiWIBNPrivate::loadIcon(void)
{
	if (iconAnimData) {
		// Icon has already been loaded.
		return iconAnimData->frames[0];
	} else if (!this->file || !this->isValid) {
		// Can't load the icon.
		return nullptr;
	}

	// Icon starts after the header and banner.
	// Read up to 8 icons.
	static const unsigned int iconstartaddr = BANNER_WIBN_STRUCT_SIZE;
	static const unsigned int iconsizetotal = BANNER_WIBN_ICON_SIZE*CARD_MAXICONS;
	auto icondata = aligned_uptr<uint8_t>(16, iconsizetotal);
	size_t size = file->seekAndRead(iconstartaddr, icondata.get(), iconsizetotal);
	if (size < BANNER_WIBN_ICON_SIZE) {
		// Unable to read *any* icons.
		return nullptr;
	}

	// Number of icons read.
	const unsigned int icons_read = (unsigned int)(size / BANNER_WIBN_ICON_SIZE);

	this->iconAnimData = new IconAnimData();
	iconAnimData->count = 0;

	// Process the icons.
	// We'll process up to:
	// - Number of icons read.
	// - Until we hit CARD_SPEED_END.
	// NOTE: Files with static icons should have a non-zero speed
	// for the first frame, and 0 for all other frames.
	uint16_t iconspeed = be16_to_cpu(wibnHeader.iconspeed);
	unsigned int iconaddr_cur = 0;
	for (unsigned int i = 0; i < CARD_MAXICONS && i < icons_read; i++, iconspeed >>= 2) {
		const unsigned int delay = (iconspeed & CARD_SPEED_MASK);
		if (delay == CARD_SPEED_END) {
			// End of the icons.
			// NOTE: Ignore this for the first icon.
			if (i > 0)
				break;

			// First icon. Keep going.
			iconspeed = 0;
		}

		// Icon delay.
		// Using 125ms for the fastest speed.
		// TODO: Verify this?
		iconAnimData->delays[i].numer = static_cast<uint16_t>(delay);
		iconAnimData->delays[i].denom = 8;
		iconAnimData->delays[i].ms = delay * 125;

		// Wii save icons are always RGB5A3.
		iconAnimData->frames[i] = ImageDecoder::fromGcn16(ImageDecoder::PXF_RGB5A3,
			BANNER_WIBN_ICON_W, BANNER_WIBN_ICON_H,
			reinterpret_cast<const uint16_t*>(icondata.get() + iconaddr_cur),
			BANNER_WIBN_ICON_SIZE);
		iconaddr_cur += BANNER_WIBN_ICON_SIZE;

		// Next icon.
		iconAnimData->count++;
	}

	// NOTE: We're not deleting iconAnimData even if we only have
	// a single icon because iconAnimData() will call loadIcon()
	// if iconAnimData is nullptr.

	// Set up the icon animation sequence.
	int idx = 0;
	for (int i = 0; i < iconAnimData->count; i++, idx++) {
		iconAnimData->seq_index[idx] = i;
	}

	const uint32_t flags = be32_to_cpu(wibnHeader.flags);
	if (flags & WII_WIBN_FLAG_ICON_BOUNCE) {
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
const rp_image *WiiWIBNPrivate::loadBanner(void)
{
	if (img_banner) {
		// Banner is already loaded.
		return img_banner;
	} else if (!this->file || !this->isValid) {
		// Can't load the banner.
		return nullptr;
	}

	// Banner is located after the WIBN header,
	// and is always RGB5A3 format.
	uint8_t bannerbuf[BANNER_WIBN_IMAGE_SIZE];
	size_t size = file->seekAndRead(sizeof(wibnHeader), bannerbuf, sizeof(bannerbuf));
	if (size != sizeof(bannerbuf)) {
		// Seek and/or read error.
		return nullptr;
	}

	// Convert the banner from GCN RGB5A3 format to ARGB32.
	img_banner = ImageDecoder::fromGcn16(ImageDecoder::PXF_RGB5A3,
		BANNER_WIBN_IMAGE_W, BANNER_WIBN_IMAGE_H,
		reinterpret_cast<const uint16_t*>(bannerbuf), sizeof(bannerbuf));
	return img_banner;
}

/** WiiWIBN **/

/**
 * Read a Nintendo Wii save banner file.
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
WiiWIBN::WiiWIBN(IRpFile *file)
	: super(new WiiWIBNPrivate(this, file))
{
	// This class handles save files.
	// NOTE: This will be handled using the same
	// settings as WiiSave.
	RP_D(WiiWIBN);
	d->className = "WiiSave";
	d->fileType = FTYPE_BANNER_FILE;

	if (!d->file) {
		// Could not dup() the file handle.
		return;
	}

	// Read the save file header.
	d->file->rewind();
	size_t size = d->file->read(&d->wibnHeader, sizeof(d->wibnHeader));
	if (size != sizeof(d->wibnHeader)) {
		delete d->file;
		d->file = nullptr;
		return;
	}

	// Check if this file is supported.
	DetectInfo info;
	info.header.addr = 0;
	info.header.size = sizeof(d->wibnHeader);
	info.header.pData = reinterpret_cast<const uint8_t*>(&d->wibnHeader);
	info.ext = nullptr;	// Not needed for Wii save banner files.
	info.szFile = d->file->size();
	d->isValid = (isRomSupported_static(&info) >= 0);

	if (!d->isValid) {
		delete d->file;
		d->file = nullptr;
	}
}

/** ROM detection functions. **/

/**
 * Is a ROM image supported by this class?
 * @param info DetectInfo containing ROM detection information.
 * @return Class-specific system ID (>= 0) if supported; -1 if not.
 */
int WiiWIBN::isRomSupported_static(const DetectInfo *info)
{
	assert(info != nullptr);
	assert(info->header.pData != nullptr);
	assert(info->header.addr == 0);
	if (!info || !info->header.pData ||
	    info->header.addr != 0 ||
	    info->header.size < sizeof(Wii_WIBN_Header_t))
	{
		// Either no detection information was specified,
		// or the header is too small.
		return -1;
	}

	const Wii_WIBN_Header_t *const wibnHeader =
		reinterpret_cast<const Wii_WIBN_Header_t*>(info->header.pData);

	// Check the WIBN magic number.
	if (wibnHeader->magic == cpu_to_be32(WII_WIBN_MAGIC)) {
		// Found the WIBN magic number.
		return 0;
	}

	// Not suported.
	return -1;
}

/**
 * Get the name of the system the loaded ROM is designed for.
 * @param type System name type. (See the SystemName enum.)
 * @return System name, or nullptr if type is invalid.
 */
const char *WiiWIBN::systemName(unsigned int type) const
{
	RP_D(const WiiWIBN);
	if (!d->isValid || !isSystemNameTypeValid(type))
		return nullptr;

	// Bits 0-1: Type. (short, long, abbreviation)
	static const char *const sysNames[4] = {
		// NOTE: Same as Wii.
		"Nintendo Wii", "Wii", "Wii", nullptr
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
const char *const *WiiWIBN::supportedFileExtensions_static(void)
{
	// Save banner is usually "banner.bin" in the save directory.
	static const char *const exts[] = {
		".bin",
		".wibn",	// Custom

		nullptr
	};
	return exts;
}

/**
 * Get a list of all supported MIME types.
 * This is to be used for metadata extractors that
 * must indicate which MIME types they support.
 *
 * NOTE: The array and the strings in the array should
 * *not* be freed by the caller.
 *
 * @return NULL-terminated array of all supported file extensions, or nullptr on error.
 */
const char *const *WiiWIBN::supportedMimeTypes_static(void)
{
	static const char *const mimeTypes[] = {
		// Unofficial MIME types.
		// TODO: Get these upstreamed on FreeDesktop.org.
		"application/x-wii-wibn",	// .wibn

		nullptr
	};
	return mimeTypes;
}

/**
 * Get a bitfield of image types this class can retrieve.
 * @return Bitfield of supported image types. (ImageTypesBF)
 */
uint32_t WiiWIBN::supportedImageTypes_static(void)
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
std::vector<RomData::ImageSizeDef> WiiWIBN::supportedImageSizes_static(ImageType imageType)
{
	assert(imageType >= IMG_INT_MIN && imageType <= IMG_EXT_MAX);
	if (imageType < IMG_INT_MIN || imageType > IMG_EXT_MAX) {
		// ImageType is out of range.
		return std::vector<ImageSizeDef>();
	}

	switch (imageType) {
		case IMG_INT_ICON: {
			static const ImageSizeDef sz_INT_ICON[] = {
				{nullptr, BANNER_WIBN_ICON_W, BANNER_WIBN_ICON_H, 0},
			};
			return vector<ImageSizeDef>(sz_INT_ICON,
				sz_INT_ICON + ARRAY_SIZE(sz_INT_ICON));
		}
		case IMG_INT_BANNER: {
			static const ImageSizeDef sz_INT_BANNER[] = {
				{nullptr, BANNER_WIBN_IMAGE_W, BANNER_WIBN_IMAGE_H, 0},
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
 * Get image processing flags.
 *
 * These specify post-processing operations for images,
 * e.g. applying transparency masks.
 *
 * @param imageType Image type.
 * @return Bitfield of ImageProcessingBF operations to perform.
 */
uint32_t WiiWIBN::imgpf(ImageType imageType) const
{
	assert(imageType >= IMG_INT_MIN && imageType <= IMG_EXT_MAX);
	if (imageType < IMG_INT_MIN || imageType > IMG_EXT_MAX) {
		// ImageType is out of range.
		return 0;
	}

	uint32_t ret = 0;
	switch (imageType) {
		case IMG_INT_ICON: {
			RP_D(const WiiWIBN);
			// Use nearest-neighbor scaling when resizing.
			// Also, need to check if this is an animated icon.
			const_cast<WiiWIBNPrivate*>(d)->loadIcon();
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
			return IMGPF_RESCALE_NEAREST;

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
int WiiWIBN::loadFieldData(void)
{
	RP_D(WiiWIBN);
	if (!d->fields->empty()) {
		// Field data *has* been loaded...
		return 0;
	} else if (!d->file || !d->file->isOpen()) {
		// File isn't open.
		return -EBADF;
	} else if (!d->isValid) {
		// Unknown save banner file type.
		return -EIO;
	}

	// Wii WIBN header.
	const Wii_WIBN_Header_t *const wibnHeader = &d->wibnHeader;
	d->fields->reserve(2);	// Maximum of 2 fields.

	// TODO: Combine title and subtitle into one field?

	// Title.
	d->fields->addField_string(C_("WiiWIBN", "Title"),
		utf16be_to_utf8(wibnHeader->gameTitle, sizeof(wibnHeader->gameTitle)));

	// Subtitle.
	if (wibnHeader->gameSubTitle[0] != 0) {
		d->fields->addField_string(C_("WiiWIBN", "Subtitle"),
			utf16be_to_utf8(wibnHeader->gameSubTitle, sizeof(wibnHeader->gameSubTitle)));
	}

	// Finished reading the field data.
	return static_cast<int>(d->fields->count());
}

/**
 * Load an internal image.
 * Called by RomData::image().
 * @param imageType	[in] Image type to load.
 * @param pImage	[out] Pointer to const rp_image* to store the image in.
 * @return 0 on success; negative POSIX error code on error.
 */
int WiiWIBN::loadInternalImage(ImageType imageType, const rp_image **pImage)
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

	RP_D(WiiWIBN);
	switch (imageType) {
		case IMG_INT_ICON:
			if (d->iconAnimData) {
				// Return the first icon frame.
				// NOTE: Wii save icon animations are always
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
const IconAnimData *WiiWIBN::iconAnimData(void) const
{
	RP_D(const WiiWIBN);
	if (!d->iconAnimData) {
		// Load the icon.
		if (!const_cast<WiiWIBNPrivate*>(d)->loadIcon()) {
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
