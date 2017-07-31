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
		// Is this multi-badge? (>1x1)
		bool multiBadge;

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
	, multiBadge(false)
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
	static const unsigned int badge64_sz = BADGE_SIZE_LARGE_W*BADGE_SIZE_LARGE_H*2;
	static const unsigned int badge32_sz = BADGE_SIZE_SMALL_W*BADGE_SIZE_SMALL_H*2;

	// Starting address depends on multi-badge status.
	unsigned int start_addr = (multiBadge ? 0x4300 : 0x1100);
	unsigned int badge_sz, badge_dims;
	if (idx == 1) {
		badge_sz = badge64_sz;
		badge_dims = BADGE_SIZE_LARGE_W;
	} else {
		badge_sz = badge32_sz;
		badge_dims = BADGE_SIZE_SMALL_W;
		start_addr += badge64_sz + 0x800;	// FIXME: What's the 0x800?
	}

	// TODO: Multiple internal image sizes.
	// For now, 64x64 only.

	// TODO: Multi-badge.
	unique_ptr<uint16_t[]> badgeData(new uint16_t[badge_sz/2]);
	size_t size = file->seekAndRead(start_addr, badgeData.get(), badge_sz);
	if (size != badge_sz) {
		// Seek and/or read error.
		return nullptr;
	}

	// Convert to rp_image.
	img = ImageDecoder::fromN3DSTiledRGB565(
		badge_dims, badge_dims,
		badgeData.get(), badge_sz);
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
	d->multiBadge = false;	// check later

	if (!d->isValid)
		return;

	// Check for multi-badge.
	switch (d->badgeType) {
		case NintendoBadgePrivate::BADGE_TYPE_PRBS:
			if (d->badgeHeader.prbs.width > 1 ||
			    d->badgeHeader.prbs.height > 1)
			{
				// This is multi-badge.
				d->multiBadge = true;
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

	// TODO: Multi-badge sizes.
	static const ImageSizeDef sz_INT_IMAGE[] = {
		{nullptr, 32, 32, 0},
		{nullptr, 64, 64, 1},
	};
	return vector<ImageSizeDef>(sz_INT_IMAGE,
		sz_INT_IMAGE + ARRAY_SIZE(sz_INT_IMAGE));
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
		case NintendoBadgePrivate::BADGE_TYPE_PRBS: {
			const Badge_PRBS_Header *const prbs = &d->badgeHeader.prbs;
			if (prbs->width > 1 && prbs->width > 1) {
				badgeType = _RP("Individual Badge");
			} else {
				badgeType = _RP("Mega Badge");
			}
			break;
		}

		case NintendoBadgePrivate::BADGE_TYPE_CABS: {
			const Badge_PRBS_Header *const prbs = &d->badgeHeader.prbs;
			if (prbs->width > 1 && prbs->width > 1) {
				badgeType = _RP("Badge Set");
			} else {
				badgeType = _RP("Mega Badge Set");
			}
			break;
		}

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

	// Multi-badge size.
	if (prbs->width > 1 || prbs->height > 1) {
		d->fields->addField_string(_RP("Multi-Badge Size"),
			rp_sprintf("%ux%u", prbs->width, prbs->height));
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
