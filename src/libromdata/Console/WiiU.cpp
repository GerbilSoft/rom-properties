/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * WiiU.cpp: Nintendo Wii U disc image reader.                             *
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

#include "librpbase/config.librpbase.h"

#include "WiiU.hpp"
#include "librpbase/RomData_p.hpp"

#include "data/NintendoPublishers.hpp"
#include "wiiu_structs.h"
#include "gcn_structs.h"
#include "data/WiiUData.hpp"

// librpbase
#include "librpbase/common.h"
#include "librpbase/byteswap.h"
#include "librpbase/TextFuncs.hpp"
#include "librpbase/file/IRpFile.hpp"
#include "librpbase/img/rp_image.hpp"
#include "libi18n/i18n.h"
using namespace LibRpBase;

// C includes. (C++ namespace)
#include <cassert>
#include <cctype>
#include <cerrno>
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

ROMDATA_IMPL(WiiU)
ROMDATA_IMPL_IMG(WiiU)

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
	d->className = "WiiU";
	d->fileType = FTYPE_DISC_IMAGE;

	if (!d->file) {
		// Could not dup() the file handle.
		return;
	}

	// Read the disc header.
	// NOTE: Using sizeof(GCN_DiscHeader) so we can verify that
	// GCN/Wii magic numbers are not present.
	static_assert(sizeof(GCN_DiscHeader) >= sizeof(WiiU_DiscHeader),
		"GCN_DiscHeader is smaller than WiiU_DiscHeader.");
	uint8_t header[sizeof(GCN_DiscHeader)];
	d->file->rewind();
	size_t size = d->file->read(header, sizeof(header));
	if (size != sizeof(header))
		return;

	// Check if this disc image is supported.
	DetectInfo info;
	info.header.addr = 0;
	info.header.size = sizeof(header);
	info.header.pData = header;
	info.ext = nullptr;	// Not needed for Wii U.
	info.szFile = d->file->size();
	if (isRomSupported_static(&info) < 0) {
		// Disc image is invalid.
		return;
	}

	// Verify the secondary magic number at 0x10000.
	uint32_t disc_magic;
	size = d->file->seekAndRead(0x10000, &disc_magic, sizeof(disc_magic));
	if (size != sizeof(disc_magic)) {
		// Seek and/or read error.
		return;
	}

	if (disc_magic == cpu_to_be32(WIIU_SECONDARY_MAGIC)) {
		// Secondary magic matches.
		d->isValid = true;

		// Save the disc header.
		memcpy(&d->discHeader, header, sizeof(d->discHeader));
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
	    info->header.size < sizeof(GCN_DiscHeader) ||
	    info->szFile < 0x20000)
	{
		// Either no detection information was specified,
		// or the header is too small.
		// szFile: Partition table is at 0x18000, so we
		// need to have at least 0x20000.
		return -1;
	}

	// Game ID must start with "WUP-".
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

	// Check for GCN/Wii magic numbers.
	const GCN_DiscHeader *gcn_header = reinterpret_cast<const GCN_DiscHeader*>(info->header.pData);
	if (be32_to_cpu(gcn_header->magic_wii) == WII_MAGIC ||
	    be32_to_cpu(gcn_header->magic_gcn) == GCN_MAGIC)
	{
		// GameCube and/or Wii magic is present.
		// This is not a Wii U disc image.
		return -1;
	}

	// Disc header is valid.
	return 0;
}

/**
 * Get the name of the system the loaded ROM is designed for.
 * @param type System name type. (See the SystemName enum.)
 * @return System name, or nullptr if type is invalid.
 */
const char *WiiU::systemName(unsigned int type) const
{
	RP_D(const WiiU);
	if (!d->isValid || !isSystemNameTypeValid(type))
		return nullptr;

	// Bits 0-1: Type. (short, long, abbreviation)
	static const char *const sysNames[4] = {
		"Nintendo Wii U", "Wii U", "Wii U", nullptr
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
const char *const *WiiU::supportedFileExtensions_static(void)
{
	static const char *const exts[] = {
		".wud",

		// NOTE: May cause conflicts on Windows
		// if fallback handling isn't working.
		".iso",

		nullptr
	};
	return exts;
}

/**
 * Get a bitfield of image types this class can retrieve.
 * @return Bitfield of supported image types. (ImageTypesBF)
 */
uint32_t WiiU::supportedImageTypes_static(void)
{
#ifdef HAVE_JPEG
	return IMGBF_EXT_MEDIA |
	       IMGBF_EXT_COVER | IMGBF_EXT_COVER_3D |
	       IMGBF_EXT_COVER_FULL;
#else /* !HAVE_JPEG */
	return IMGBF_EXT_MEDIA | IMGBF_EXT_COVER_3D;
#endif /* HAVE_JPEG */
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
#ifdef HAVE_JPEG
		case IMG_EXT_COVER: {
			static const ImageSizeDef sz_EXT_COVER[] = {
				{nullptr, 160, 224, 0},
				{"M", 350, 500, 1},
				{"HQ", 768, 1080, 2},
			};
			return vector<ImageSizeDef>(sz_EXT_COVER,
				sz_EXT_COVER + ARRAY_SIZE(sz_EXT_COVER));
		}
#endif /* HAVE_JPEG */
		case IMG_EXT_COVER_3D: {
			static const ImageSizeDef sz_EXT_COVER_3D[] = {
				{nullptr, 176, 248, 0},
			};
			return vector<ImageSizeDef>(sz_EXT_COVER_3D,
				sz_EXT_COVER_3D + ARRAY_SIZE(sz_EXT_COVER_3D));
		}
#ifdef HAVE_JPEG
		case IMG_EXT_COVER_FULL: {
			static const ImageSizeDef sz_EXT_COVER[] = {
				{nullptr, 340, 224, 0},
				{"M", 752, 500, 1},
				{"HQ", 1632, 1080, 2},
			};
			return vector<ImageSizeDef>(sz_EXT_COVER,
				sz_EXT_COVER + ARRAY_SIZE(sz_EXT_COVER));
		}
#endif /* HAVE_JPEG */
		default:
			break;
	}

	// Unsupported image type.
	return std::vector<ImageSizeDef>();
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

	// Game ID.
	d->fields->addField_string(C_("WiiU", "Game ID"),
		latin1_to_utf8(d->discHeader.id, sizeof(d->discHeader.id)));

	// Publisher.
	// Look up the publisher ID.
	char publisher_code[5];
	const char *publisher = nullptr;
	string s_publisher;

	const uint32_t publisher_id = WiiUData::lookup_disc_publisher(d->discHeader.id4);
	publisher_code[0] = (publisher_id >> 24) & 0xFF;
	publisher_code[1] = (publisher_id >> 16) & 0xFF;
	publisher_code[2] = (publisher_id >>  8) & 0xFF;
	publisher_code[3] =  publisher_id & 0xFF;
	publisher_code[4] = 0;
	if (publisher_id != 0 && (publisher_id & 0xFFFF0000) == 0x30300000) {
		// Publisher ID is a valid two-character ID.
		publisher = NintendoPublishers::lookup(&publisher_code[2]);
	}
	if (publisher) {
		s_publisher = publisher;
	} else {
		if (isalnum(publisher_code[0]) && isalnum(publisher_code[1]) &&
		    isalnum(publisher_code[2]) && isalnum(publisher_code[3]))
		{
			s_publisher = rp_sprintf(C_("WiiU", "Unknown (%.4s)"), publisher_code);
		} else {
			s_publisher = rp_sprintf(C_("WiiU", "Unknown (%02X %02X %02X %02X)"),
				(uint8_t)publisher_code[0], (uint8_t)publisher_code[1],
				(uint8_t)publisher_code[2], (uint8_t)publisher_code[3]);
		}
	}
	d->fields->addField_string(C_("WiiU", "Publisher"), s_publisher);

	// Game version.
	// TODO: Validate the version characters.
	d->fields->addField_string(C_("WiiU", "Version"),
		latin1_to_utf8(d->discHeader.version, sizeof(d->discHeader.version)));

	// OS version.
	// TODO: Validate the version characters.
	d->fields->addField_string(C_("WiiU", "OS Version"),
		rp_sprintf("%c.%c.%c",
			d->discHeader.os_version[0],
			d->discHeader.os_version[1],
			d->discHeader.os_version[2]));

	// Region.
	// TODO: Compare against list of regions and show the fancy name.
	d->fields->addField_string(C_("WiiU", "Region"),
		latin1_to_utf8(d->discHeader.region, sizeof(d->discHeader.region)));

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

	// If we're downloading a "high-resolution" image (M or higher),
	// also add the default image to ExtURLs in case the user has
	// high-resolution image downloads disabled.
	const ImageSizeDef *szdefs_dl[2];
	szdefs_dl[0] = sizeDef;
	unsigned int szdef_count;
	if (sizeDef->index > 0) {
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

			extURL.url = d->getURL_GameTDB("wiiu", imageTypeName, *iter, id6, ext);
			extURL.cache_key = d->getCacheKey_GameTDB("wiiu", imageTypeName, *iter, id6, ext);
			extURL.width = szdefs_dl[i]->width;
			extURL.height = szdefs_dl[i]->height;
			extURL.high_res = (szdefs_dl[i]->index > 0);
		}
	}

	// All URLs added.
	return 0;
}

}
