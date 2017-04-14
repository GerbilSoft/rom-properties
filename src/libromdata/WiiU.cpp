/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * WiiU.cpp: Nintendo Wii U disc image reader.                             *
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

#include "WiiU.hpp"
#include "RomData_p.hpp"

#include "wiiu_structs.h"
#include "data/WiiUData.hpp"

#include "common.h"
#include "byteswap.h"
#include "TextFuncs.hpp"
#include "file/IRpFile.hpp"
#include "img/rp_image.hpp"

// C includes. (C++ namespace)
#include <cassert>
#include <cctype>
#include <cstdio>
#include <cstring>

// C++ includes.
#include <memory>
#include <string>
#include <vector>
using std::string;
using std::unique_ptr;
using std::vector;

namespace LibRomData {

class WiiUPrivate : public RomDataPrivate
{
	public:
		WiiUPrivate(WiiU *q, IRpFile *file);

	private:
		typedef RomDataPrivate super;
		RP_DISABLE_COPY(WiiUPrivate)

	public:
		// Disc header.
		WiiU_DiscHeader discHeader;
};

/** WiiUPrivate **/

WiiUPrivate::WiiUPrivate(WiiU *q, IRpFile *file)
	: super(q, file)
{
	// Clear the discHeader struct.
	memset(&discHeader, 0, sizeof(discHeader));
}

/** WiiU **/

/**
 * Read a Nintendo Wii U disc image.
 *
 * A disc image must be opened by the caller. The file handle
 * will be dup()'d and must be kept open in order to load
 * data from the disc image.
 *
 * To close the file, either delete this object or call close().
 *
 * NOTE: Check isValid() to determine if this is a valid ROM.
 *
 * @param file Open disc image.
 */
WiiU::WiiU(IRpFile *file)
	: super(new WiiUPrivate(this, file))
{
	// This class handles disc images.
	RP_D(WiiU);
	d->fileType = FTYPE_DISC_IMAGE;

	if (!d->file) {
		// Could not dup() the file handle.
		return;
	}

	// Read the disc header.
	d->file->rewind();
	size_t size = d->file->read(&d->discHeader, sizeof(d->discHeader));
	if (size != sizeof(d->discHeader))
		return;

	// Check if this disc image is supported.
	DetectInfo info;
	info.header.addr = 0;
	info.header.size = sizeof(d->discHeader);
	info.header.pData = reinterpret_cast<const uint8_t*>(&d->discHeader);
	info.ext = nullptr;	// Not needed for Wii U.
	info.szFile = d->file->size();
	bool isValid = (isRomSupported_static(&info) >= 0);
	if (!isValid)
		return;

	// Verify the secondary magic number at 0x10000.
	static const uint8_t wiiu_magic[4] = {0xCC, 0x54, 0x9E, 0xB9};
	int ret = d->file->seek(0x10000);
	if (ret != 0) {
		// Seek error.
		return;
	}

	uint8_t disc_magic[4];
	size = d->file->read(disc_magic, sizeof(disc_magic));
	if (size != sizeof(disc_magic)) {
		// Read error.
		return;
	}

	if (!memcmp(disc_magic, wiiu_magic, sizeof(wiiu_magic))) {
		// Secondary magic matches.
		d->isValid = true;
	}
}

/** ROM detection functions. **/

/**
 * Is a ROM image supported by this class?
 * @param info DetectInfo containing ROM detection information.
 * @return Class-specific system ID (>= 0) if supported; -1 if not.
 */
int WiiU::isRomSupported_static(const DetectInfo *info)
{
	assert(info != nullptr);
	assert(info->header.pData != nullptr);
	assert(info->header.addr == 0);
	if (!info || !info->header.pData ||
	    info->header.addr != 0 ||
	    info->header.size < sizeof(WiiU_DiscHeader) ||
	    info->szFile < 0x20000)
	{
		// Either no detection information was specified,
		// or the header is too small.
		// szFile: Partition table is at 0x18000, so we
		// need to have at least 0x20000.
		return -1;
	}

	// Game ID must start with "WUP-".
	// TODO: Make sure GCN/Wii magic numbers aren't present.
	// NOTE: There's also a secondary magic number at 0x10000,
	// but we can't check it here.
	const WiiU_DiscHeader *wiiu_header = reinterpret_cast<const WiiU_DiscHeader*>(info->header.pData);
	if (memcmp(wiiu_header->id, "WUP-", 4) != 0) {
		// Not Wii U.
		return -1;
	}

	// Check hyphens.
	// TODO: Verify version numbers and region code.
	if (wiiu_header->hyphen1 != '-' ||
	    wiiu_header->hyphen2 != '-' ||
	    wiiu_header->hyphen3 != '-' ||
	    wiiu_header->hyphen4 != '-' ||
	    wiiu_header->hyphen5 != '-')
	{
		// Missing hyphen.
		return -1;
	}

	// Disc header is valid.
	return 0;
}

/**
 * Is a ROM image supported by this object?
 * @param info DetectInfo containing ROM detection information.
 * @return Class-specific system ID (>= 0) if supported; -1 if not.
 */
int WiiU::isRomSupported(const DetectInfo *info) const
{
	return isRomSupported_static(info);
}

/**
 * Get the name of the system the loaded ROM is designed for.
 * @param type System name type. (See the SystemName enum.)
 * @return System name, or nullptr if type is invalid.
 */
const rp_char *WiiU::systemName(uint32_t type) const
{
	RP_D(const WiiU);
	if (!d->isValid || !isSystemNameTypeValid(type))
		return nullptr;

	// Bits 0-1: Type. (short, long, abbreviation)
	static const rp_char *const sysNames[4] = {
		_RP("Nintendo Wii U"), _RP("Wii U"), _RP("Wii U"), nullptr
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
const rp_char *const *WiiU::supportedFileExtensions_static(void)
{
	static const rp_char *const exts[] = {
		_RP(".wud"),

		// NOTE: May cause conflicts on Windows
		// if fallback handling isn't working.
		_RP(".iso"),

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
 * NOTE 2: The array and the strings in the array should
 * *not* be freed by the caller.
 *
 * @return NULL-terminated array of all supported file extensions, or nullptr on error.
 */
const rp_char *const *WiiU::supportedFileExtensions(void) const
{
	return supportedFileExtensions_static();
}

/**
 * Get a bitfield of image types this class can retrieve.
 * @return Bitfield of supported image types. (ImageTypesBF)
 */
uint32_t WiiU::supportedImageTypes_static(void)
{
#ifdef HAVE_JPEG
	return IMGBF_EXT_MEDIA | IMGBF_EXT_COVER | IMGBF_EXT_COVER_3D;
#else /* !HAVE_JPEG */
	return IMGBF_EXT_MEDIA | IMGBF_EXT_COVER_3D;
#endif /* HAVE_JPEG */
}

/**
 * Get a bitfield of image types this class can retrieve.
 * @return Bitfield of supported image types. (ImageTypesBF)
 */
uint32_t WiiU::supportedImageTypes(void) const
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
std::vector<RomData::ImageSizeDef> WiiU::supportedImageSizes_static(ImageType imageType)
{
	assert(imageType >= IMG_INT_MIN && imageType <= IMG_EXT_MAX);
	if (imageType < IMG_INT_MIN || imageType > IMG_EXT_MAX) {
		// ImageType is out of range.
		return std::vector<ImageSizeDef>();
	}

	switch (imageType) {
		case IMG_EXT_MEDIA: {
			static const ImageSizeDef sz_EXT_MEDIA[] = {
				{nullptr, 160, 160, 0},
				{"M", 500, 500, 1},
			};
			return vector<ImageSizeDef>(sz_EXT_MEDIA,
				sz_EXT_MEDIA + ARRAY_SIZE(sz_EXT_MEDIA));
		}
		case IMG_EXT_COVER: {
			static const ImageSizeDef sz_EXT_COVER[] = {
				{nullptr, 160, 224, 0},
				{"M", 350, 500, 1},
				{"HQ", 768, 1080, 2},
			};
			return vector<ImageSizeDef>(sz_EXT_COVER,
				sz_EXT_COVER + ARRAY_SIZE(sz_EXT_COVER));
		}
		case IMG_EXT_COVER_3D: {
			static const ImageSizeDef sz_EXT_COVER_3D[] = {
				{nullptr, 176, 248, 0},
			};
			return vector<ImageSizeDef>(sz_EXT_COVER_3D,
				sz_EXT_COVER_3D + ARRAY_SIZE(sz_EXT_COVER_3D));
		}
		case IMG_EXT_COVER_FULL: {
			static const ImageSizeDef sz_EXT_COVER[] = {
				{nullptr, 340, 224, 0},
				{"M", 752, 500, 1},
				{"HQ", 1632, 1080, 2},
			};
			return vector<ImageSizeDef>(sz_EXT_COVER,
				sz_EXT_COVER + ARRAY_SIZE(sz_EXT_COVER));
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
std::vector<RomData::ImageSizeDef> WiiU::supportedImageSizes(ImageType imageType) const
{
	return supportedImageSizes_static(imageType);
}

/**
 * Load field data.
 * Called by RomData::fields() if the field data hasn't been loaded yet.
 * @return Number of fields read on success; negative POSIX error code on error.
 */
int WiiU::loadFieldData(void)
{
	RP_D(WiiU);
	if (d->fields->isDataLoaded()) {
		// Field data *has* been loaded...
		return 0;
	} else if (!d->file || !d->file->isOpen()) {
		// File isn't open.
		return -EBADF;
	} else if (!d->isValid) {
		// Disc image isn't valid.
		return -EIO;
	}

	// Disc header is read in the constructor.
	d->fields->reserve(4);	// Maximum of 4 fields.

	// Temporary buffer for snprintf().
	char buf[64];
	int len;

	// Game ID.
	d->fields->addField_string(_RP("Game ID"),
		latin1_to_rp_string(d->discHeader.id, sizeof(d->discHeader.id)));

	// Game version.
	// TODO: Validate the version characters.
	len = snprintf(buf, sizeof(buf), "%.2s", d->discHeader.version);
	if (len > (int)sizeof(buf))
		len = sizeof(buf);
	d->fields->addField_string(_RP("Version"),
		len > 0 ? latin1_to_rp_string(buf, len) : _RP("Unknown"));

	// OS version.
	// TODO: Validate the version characters.
	len = snprintf(buf, sizeof(buf), "%c.%c.%c",
		d->discHeader.os_version[0],
		d->discHeader.os_version[1],
		d->discHeader.os_version[2]);
	if (len > (int)sizeof(buf))
		len = sizeof(buf);
	d->fields->addField_string(_RP("OS Version"),
		len > 0 ? latin1_to_rp_string(buf, len) : _RP("Unknown"));

	// Region.
	// TODO: Compare against list of regions and show the fancy name.
	d->fields->addField_string(_RP("Region"),
		latin1_to_rp_string(d->discHeader.region, sizeof(d->discHeader.region)));

	// Finished reading the field data.
	return (int)d->fields->count();
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
int WiiU::extURLs(ImageType imageType, vector<ExtURL> *pExtURLs, int size) const
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

	RP_D(const WiiU);
	if (!d->file || !d->file->isOpen()) {
		// File isn't open.
		return -EBADF;
	} else if (!d->isValid) {
		// Disc image isn't valid.
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
		case IMG_EXT_MEDIA:
			imageTypeName_base = "disc";
			ext = ".png";
			break;
#ifdef HAVE_JPEG
		case IMG_EXT_COVER:
			imageTypeName_base = "cover";
			ext = ".jpg";
			break;
#endif /* HAVE_JPEG */
		case IMG_EXT_COVER_3D:
			imageTypeName_base = "cover3D";
			ext = ".png";
			break;
#ifdef HAVE_JPEG
		case IMG_EXT_COVER_FULL:
			imageTypeName_base = "coverfull";
			ext = ".jpg";
			break;
#endif /* HAVE_JPEG */
		default:
			// Unsupported image type.
			return -ENOENT;
	}

	// Look up the publisher ID.
	uint32_t publisher_id = WiiUData::lookup_disc_publisher(d->discHeader.id4);
	if (publisher_id == 0 || (publisher_id & 0xFFFF0000) != 0x30300000) {
		// Either the publisher ID is unknown, or it's a
		// 4-character ID, which isn't supported by
		// GameTDB at the moment.
		return -ENOENT;
	}

	// Current image type.
	char imageTypeName[16];
	snprintf(imageTypeName, sizeof(imageTypeName), "%s%s",
		 imageTypeName_base, (sizeDef->name ? sizeDef->name : ""));

	// Determine the GameTDB region code(s).
	// TODO: Wii U version. (Figure out the region code field...)
	//vector<const char*> tdb_regions = d->gcnRegionToGameTDB(d->gcnRegion, d->discHeader.id4[3]);
	vector<const char*> tdb_regions;
	tdb_regions.push_back("US");

	// Game ID.
	// Replace any non-printable characters with underscores.
	// (GameCube NDDEMO has ID6 "00\0E01".)
	char id6[7];
	for (int i = 0; i < 4; i++) {
		id6[i] = (isprint(d->discHeader.id4[i])
			? d->discHeader.id4[i]
			: '_');
	}

	// Publisher ID.
	id6[4] = (publisher_id >> 8) & 0xFF;
	id6[5] = publisher_id & 0xFF;
	id6[6] = 0;

	// ExtURLs.
	// TODO: If multiple image sizes are added, add the
	// "default" size to the end of ExtURLs in case the
	// user has high-resolution downloads disabled.
	pExtURLs->reserve(4);

	// Get the URLs.
	for (auto iter = tdb_regions.cbegin(); iter != tdb_regions.cend(); ++iter) {
		int idx = (int)pExtURLs->size();
		pExtURLs->resize(idx+1);
		auto &extURL = pExtURLs->at(idx);

		extURL.url = d->getURL_GameTDB("wiiu", imageTypeName, *iter, id6, ext);
		extURL.cache_key = d->getCacheKey_GameTDB("wiiu", imageTypeName, *iter, id6, ext);
		extURL.width = sizeDef->width;
		extURL.height = sizeDef->height;
		extURL.high_res = false;	// Only one size is available.
	}

	// All URLs added.
	return 0;
}

}
