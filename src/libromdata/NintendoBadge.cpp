/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * NintendoBadge.hpp: Nintendo Badge Arcade image reader.                  *
 *                                                                         *
 * Copyright (c) 2017 by David Korth.                                      *
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

#include "NintendoBadge.hpp"
#include "librpbase/RomData_p.hpp"

#include "badge_structs.h"

// librpbase
#include "librpbase/common.h"
#include "librpbase/byteswap.h"
#include "librpbase/TextFuncs.hpp"
#include "librpbase/file/IRpFile.hpp"
#include "librpbase/img/rp_image.hpp"
#include "librpbase/img/ImageDecoder.hpp"
using namespace LibRpBase;

// C includes. (C++ namespace)
#include <cassert>

// C++ includes.
#include <algorithm>
#include <memory>
#include <vector>
using std::unique_ptr;
using std::vector;

#ifdef _MSC_VER
#include <intrin.h>
#endif

namespace LibRomData {

class NintendoBadgePrivate : public RomDataPrivate
{
	public:
		NintendoBadgePrivate(NintendoBadge *q, IRpFile *file);
		~NintendoBadgePrivate();

	private:
		typedef RomDataPrivate super;
		RP_DISABLE_COPY(NintendoBadgePrivate)

	public:
		enum BadgeType {
			BADGE_TYPE_UNKNOWN	= -1,	// Unknown badge type.
			BADGE_TYPE_PRBS		= 0,	// PRBS (individual badge)
			BADGE_TYPE_CABS		= 1,	// CABS (set badge)

			BADGE_TYPE_MAX
		};

		// Badge type.
		int badgeType;
		// Is this mega badge? (>1x1)
		bool megaBadge;

		// Badge header.
		// TODO: CABS header.
		union {
			Badge_PRBS_Header prbs;
		} badgeHeader;

		// Decoded image.
		rp_image *img;

		/**
		 * Load the badge image.
		 * @param idx Image index. (0 == 32x32; 1 == 64x64)
		 * @return Image, or nullptr on error.
		 */
		const rp_image *loadImage(int idx = 1);
};

/** NintendoBadgePrivate **/

NintendoBadgePrivate::NintendoBadgePrivate(NintendoBadge *q, IRpFile *file)
	: super(q, file)
	, badgeType(BADGE_TYPE_UNKNOWN)
	, megaBadge(false)
	, img(nullptr)
{
	// Clear the PVR header structs.
	memset(&badgeHeader, 0, sizeof(badgeHeader));
}

NintendoBadgePrivate::~NintendoBadgePrivate()
{
	delete img;
}

/**
 * Load the badge image.
 * @param idx Image index. (0 == 32x32; 1 == 64x64)
 * @return Image, or nullptr on error.
 */
const rp_image *NintendoBadgePrivate::loadImage(int idx)
{
	assert(idx == 0 || idx == 1);
	if (idx != 0 && idx != 1) {
		// Invalid image index.
		return nullptr;
	}

	if (img) {
		// Image has already been loaded.
		return img;
	} else if (!this->file) {
		// Can't load the image.
		return nullptr;
	}

	if (badgeType != BADGE_TYPE_PRBS) {
		// TODO: Support CABS.
		return nullptr;
	}

	// Badge sizes.
	// Badge data is RGB565 + A4.
	static const unsigned int badge64_rgb_sz = BADGE_SIZE_LARGE_W*BADGE_SIZE_LARGE_H*2;
	static const unsigned int badge64_a4_sz  = BADGE_SIZE_LARGE_W*BADGE_SIZE_LARGE_H/2;
	static const unsigned int badge32_rgb_sz = BADGE_SIZE_SMALL_W*BADGE_SIZE_SMALL_H*2;
	static const unsigned int badge32_a4_sz  = BADGE_SIZE_SMALL_W*BADGE_SIZE_SMALL_H*2;

	// Starting address depends on mega badge status.
	unsigned int start_addr = (megaBadge ? 0x4300 : 0x1100);
	unsigned int badge_rgb_sz, badge_a4_sz, badge_dims;
	if (idx == 1) {
		// 32x32 badges. (0xA00+0x200)
		badge_rgb_sz = badge64_rgb_sz;
		badge_a4_sz = badge64_a4_sz;
		badge_dims = BADGE_SIZE_LARGE_W;
	} else {
		// 64x64 badges. (0x2000+0x800)
		badge_rgb_sz = badge32_rgb_sz;
		badge_a4_sz = badge32_a4_sz;
		badge_dims = BADGE_SIZE_SMALL_W;
		start_addr += badge64_rgb_sz + badge64_a4_sz;
	}
	const unsigned int badge_sz = badge_rgb_sz + badge_a4_sz;

	// TODO: Multiple internal image sizes.
	// For now, 64x64 only.

	unique_ptr<uint8_t[]> badgeData(new uint8_t[badge_sz]);

	if (!megaBadge) {
		// Single badge.
		size_t size = file->seekAndRead(start_addr, badgeData.get(), badge_sz);
		if (size != badge_sz) {
			// Seek and/or read error.
			return nullptr;
		}

		// Convert to rp_image.
		img = ImageDecoder::fromN3DSTiledRGB565_A4(
			badge_dims, badge_dims,
			reinterpret_cast<const uint16_t*>(badgeData.get()), badge_rgb_sz,
			&badgeData.get()[badge_rgb_sz], badge_a4_sz);
	} else {
		// Mega badge. Need to convert each 64x64 badge
		// and concatenate them manually.

		// Mega badge dimensions.
		const unsigned int mb_width  = badgeHeader.prbs.mb_width;
		const unsigned int mb_height = badgeHeader.prbs.mb_height;
		const unsigned int mb_pitch  = badge_dims * sizeof(uint32_t);

		// Badges are stored vertically, then horizontally.
		img = new rp_image(badge_dims * mb_width, badge_dims * mb_height, rp_image::FORMAT_ARGB32);
		for (unsigned int x = 0; x < mb_width; x++) {
			const unsigned int mx = x*badge_dims;
			for (unsigned int y = 0; y < mb_height; y++, start_addr += (0x2800+0xA00)) {
				size_t size = file->seekAndRead(start_addr, badgeData.get(), badge_sz);
				if (size != badge_sz) {
					// Seek and/or read error.
					delete img;
					return nullptr;
				}

				rp_image *mb_img = ImageDecoder::fromN3DSTiledRGB565_A4(
					badge_dims, badge_dims,
					reinterpret_cast<const uint16_t*>(badgeData.get()), badge_rgb_sz,
					&badgeData.get()[badge_rgb_sz], badge_a4_sz);

				// Copy the image into place.
				const unsigned int my = y*badge_dims;
				for (int py = badge_dims-1; py >= 0; py--) {
					const uint32_t *src = reinterpret_cast<const uint32_t*>(mb_img->scanLine(py));
					uint32_t *dest = reinterpret_cast<uint32_t*>(img->scanLine(py+my)) + mx;
					memcpy(dest, src, mb_pitch);
				}

				delete mb_img;
			}
		}
	}

	return img;
}

/** NintendoBadge **/

/**
 * Read a Nintendo Badge image file.
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
NintendoBadge::NintendoBadge(IRpFile *file)
	: super(new NintendoBadgePrivate(this, file))
{
	// This class handles texture files.
	RP_D(NintendoBadge);
	d->className = "NintendoBadge";
	d->fileType = FTYPE_TEXTURE_FILE;

	if (!d->file) {
		// Could not dup() the file handle.
		return;
	}

	// Read the badge header.
	// TDOO: Different sizes?
	d->file->rewind();
	size_t size = d->file->read(&d->badgeHeader.prbs, sizeof(d->badgeHeader.prbs));
	if (size != sizeof(d->badgeHeader.prbs))
		return;

	// Check if this PVR image is supported.
	DetectInfo info;
	info.header.addr = 0;
	info.header.size = sizeof(d->badgeHeader.prbs);
	info.header.pData = reinterpret_cast<const uint8_t*>(&d->badgeHeader.prbs);
	info.ext = nullptr;	// Not needed for badges.
	info.szFile = 0;	// Not needed for badges.
	d->badgeType = isRomSupported_static(&info);
	d->isValid = (d->badgeType >= 0);
	d->megaBadge = false;	// check later

	if (!d->isValid)
		return;

	// Check for mega badge.
	switch (d->badgeType) {
		case NintendoBadgePrivate::BADGE_TYPE_PRBS:
			if (d->badgeHeader.prbs.mb_width > 1 ||
			    d->badgeHeader.prbs.mb_height > 1)
			{
				// This is mega badge.
				d->megaBadge = true;
			}
			break;
		default:
			break;
	}
}

/**
 * Is a ROM image supported by this class?
 * @param info DetectInfo containing ROM detection information.
 * @return Class-specific system ID (>= 0) if supported; -1 if not.
 */
int NintendoBadge::isRomSupported_static(const DetectInfo *info)
{
	assert(info != nullptr);
	assert(info->header.pData != nullptr);
	assert(info->header.addr == 0);
	if (!info || !info->header.pData ||
	    info->header.addr != 0 ||
	    info->header.size < sizeof(Badge_PRBS_Header))
	{
		// Either no detection information was specified,
		// or the header is too small.
		return -1;
	}

	// Check for PRBS.
	// TODO: Also CABS.
	if (!memcmp(info->header.pData, BADGE_PRBS_MAGIC, 4)) {
		// PRBS header is present.
		// TODO: Other checks?
		return NintendoBadgePrivate::BADGE_TYPE_PRBS;
	}

	// Not supported.
	return -1;
}

/**
 * Is a ROM image supported by this object?
 * @param info DetectInfo containing ROM detection information.
 * @return Class-specific system ID (>= 0) if supported; -1 if not.
 */
int NintendoBadge::isRomSupported(const DetectInfo *info) const
{
	return isRomSupported_static(info);
}

/**
 * Get the name of the system the loaded ROM is designed for.
 * @param type System name type. (See the SystemName enum.)
 * @return System name, or nullptr if type is invalid.
 */
const rp_char *NintendoBadge::systemName(unsigned int type) const
{
	RP_D(const NintendoBadge);
	if (!d->isValid || !isSystemNameTypeValid(type))
		return nullptr;

	// PVR has the same name worldwide, so we can
	// ignore the region selection.
	static_assert(SYSNAME_TYPE_MASK == 3,
		"NintendoBadge::systemName() array index optimization needs to be updated.");

	static const rp_char *const sysNames[4] = {
		_RP("Nintendo Badge Arcade"), _RP("Badge Arcade"), _RP("Badge"), nullptr,
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
const rp_char *const *NintendoBadge::supportedFileExtensions_static(void)
{
	static const rp_char *const exts[] = {
		_RP(".prb"),	// PRBS file
		_RP(".cab"),	// CABS file (NOTE: Conflicts with Microsoft CAB)

		nullptr
	};
	return exts;
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
const rp_char *const *NintendoBadge::supportedFileExtensions(void) const
{
	return supportedFileExtensions_static();
}

/**
 * Get a bitfield of image types this class can retrieve.
 * @return Bitfield of supported image types. (ImageTypesBF)
 */
uint32_t NintendoBadge::supportedImageTypes_static(void)
{
	return IMGBF_INT_IMAGE;
}

/**
 * Get a bitfield of image types this class can retrieve.
 * @return Bitfield of supported image types. (ImageTypesBF)
 */
uint32_t NintendoBadge::supportedImageTypes(void) const
{
	return supportedImageTypes_static();
}

/**
 * Get a list of all available image sizes for the specified image type.
 * @param imageType Image type.
 * @return Vector of available image sizes, or empty vector if no images are available.
 */
vector<RomData::ImageSizeDef> NintendoBadge::supportedImageSizes(ImageType imageType) const
{
	assert(imageType >= IMG_INT_MIN && imageType <= IMG_EXT_MAX);
	if (imageType < IMG_INT_MIN || imageType > IMG_EXT_MAX) {
		// ImageType is out of range.
		return vector<ImageSizeDef>();
	}

	RP_D(NintendoBadge);
	if (!d->isValid || imageType != IMG_INT_IMAGE) {
		return vector<ImageSizeDef>();
	}

	// Multiply the standard size by the mega badge sizes.
	unsigned int mb_width, mb_height;
	switch (d->badgeType) {
		case NintendoBadgePrivate::BADGE_TYPE_PRBS:
			mb_width = d->badgeHeader.prbs.mb_width;
			mb_height = d->badgeHeader.prbs.mb_height;
			break;
		default:
			mb_width = 1;
			mb_height = 1;
			break;
	}

	const ImageSizeDef imgsz[] = {
		{nullptr, (uint16_t)(BADGE_SIZE_SMALL_W*mb_width), (uint16_t)(BADGE_SIZE_SMALL_H*mb_height), 0},
		{nullptr, (uint16_t)(BADGE_SIZE_LARGE_W*mb_width), (uint16_t)(BADGE_SIZE_LARGE_H*mb_height), 1},
  	};
	return vector<ImageSizeDef>(imgsz, imgsz + 1);
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
uint32_t NintendoBadge::imgpf(ImageType imageType) const
{
	assert(imageType >= IMG_INT_MIN && imageType <= IMG_EXT_MAX);
	if (imageType < IMG_INT_MIN || imageType > IMG_EXT_MAX) {
		// ImageType is out of range.
		return 0;
	}

	if (imageType != IMG_INT_IMAGE) {
		// Only IMG_INT_IMAGE is supported by PVR.
		return 0;
	}

	// Badges are 32x32 and 64x64.
	// Always use nearest-neighbor scaling.
	return IMGPF_RESCALE_NEAREST;
}

/**
 * Load field data.
 * Called by RomData::fields() if the field data hasn't been loaded yet.
 * @return Number of fields read on success; negative POSIX error code on error.
 */
int NintendoBadge::loadFieldData(void)
{
	RP_D(NintendoBadge);
	if (d->fields->isDataLoaded()) {
		// Field data *has* been loaded...
		return 0;
	} else if (!d->file) {
		// File isn't open.
		return -EBADF;
	} else if (!d->isValid || d->badgeType < 0) {
		// Unknown badge type.
		return -EIO;
	}

	// Maximum of 6 fields.
	d->fields->reserve(6);

	// Type.
	const rp_char *badgeType;
	switch (d->badgeType) {
		case NintendoBadgePrivate::BADGE_TYPE_PRBS:
			badgeType = (d->megaBadge ? _RP("Mega Badge") : _RP("Individual Badge"));
			break;

		case NintendoBadgePrivate::BADGE_TYPE_CABS:
			badgeType = (d->megaBadge ? _RP("Mega Badge Set") : _RP("Badge Set"));
			break;

		default:
			// Unknown.
			assert(!"Unknown badge type. (Should not get here!)");
			badgeType = _RP("Unknown");
			break;
	}

	d->fields->addField_string(_RP("Type"), badgeType);

	// TODO: Add CABS support.
	if (d->badgeType != NintendoBadgePrivate::BADGE_TYPE_PRBS) {
		// Finished reading the field data.
		return (int)d->fields->count();
	}

	// Badge header.
	const Badge_PRBS_Header *const prbs = &d->badgeHeader.prbs;

	// Name.
	// TODO: Multi-language support?
	d->fields->addField_string(_RP("Name"),
		utf16_to_rp_string(prbs->name[0], sizeof(prbs->name[0])));

	// Badge ID.
	d->fields->addField_string_numeric(_RP("Badge ID"), le32_to_cpu(prbs->badge_id));

	// Badge filename.
	d->fields->addField_string(_RP("Filename"),
		latin1_to_rp_string(prbs->filename, sizeof(prbs->filename)));

	// Set name.
	d->fields->addField_string(_RP("Set Name"),
		latin1_to_rp_string(prbs->setname, sizeof(prbs->setname)));

	// Mega badge size.
	if (d->megaBadge) {
		d->fields->addField_string(_RP("Mega Badge Size"),
			rp_sprintf("%ux%u", prbs->mb_width, prbs->mb_height));
	}

	// Title ID.
	if (prbs->title_id.id == cpu_to_le64(0xFFFFFFFFFFFFFFFFULL)) {
		// No title ID.
		d->fields->addField_string(_RP("Launch Title ID"), _RP("None"));
	} else {
		// Title ID is present.
		d->fields->addField_string(_RP("Launch Title ID"),
			rp_sprintf("%08X-%08X",
				le32_to_cpu(prbs->title_id.hi),
				le32_to_cpu(prbs->title_id.lo)));
	}

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
int NintendoBadge::loadInternalImage(ImageType imageType, const rp_image **pImage)
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

	RP_D(NintendoBadge);
	if (imageType != IMG_INT_IMAGE) {
		// Only IMG_INT_IMAGE is supported by PVR.
		*pImage = nullptr;
		return -ENOENT;
	} else if (!d->file) {
		// File isn't open.
		*pImage = nullptr;
		return -EBADF;
	} else if (!d->isValid || d->badgeType < 0) {
		// PVR image isn't valid.
		*pImage = nullptr;
		return -EIO;
	}

	// NOTE: Assuming image index 1. (64x64)
	const int idx = 1;

	// Load the image.
	*pImage = d->loadImage(idx);
	return (*pImage != nullptr ? 0 : -EIO);
}

}
