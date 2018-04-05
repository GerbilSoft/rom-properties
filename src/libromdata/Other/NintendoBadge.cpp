/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * NintendoBadge.hpp: Nintendo Badge Arcade image reader.                  *
 *                                                                         *
 * Copyright (c) 2017-2018 by David Korth.                                 *
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

#include "NintendoBadge.hpp"
#include "librpbase/RomData_p.hpp"

#include "badge_structs.h"
#include "Handheld/n3ds_structs.h"
#include "data/Nintendo3DSSysTitles.hpp"
#include "data/NintendoLanguage.hpp"

// librpbase
#include "librpbase/common.h"
#include "librpbase/byteswap.h"
#include "librpbase/aligned_malloc.h"
#include "librpbase/TextFuncs.hpp"
#include "librpbase/file/IRpFile.hpp"

#include "librpbase/img/rp_image.hpp"
#include "librpbase/img/ImageDecoder.hpp"

#include "libi18n/i18n.h"
using namespace LibRpBase;

// C includes. (C++ namespace)
#include <cassert>
#include <cstring>

// C++ includes.
#include <string>
#include <vector>
using std::string;
using std::vector;

namespace LibRomData {

ROMDATA_IMPL(NintendoBadge)
ROMDATA_IMPL_IMG_TYPES(NintendoBadge)

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

		enum BadgeIndex_PRBS {
			PRBS_SMALL	= 0,	// 32x32
			PRBS_LARGE	= 1,	// 64x64
			PRBS_MEGA_SMALL	= 2,	// Mega Badge: 32x32 tiles
			PRBS_MEGA_LARGE	= 3,	// Mega Badge: 64x64 tiles

			PRBS_MAX
		};

		// Badge type.
		int badgeType;
		// Is this mega badge? (>1x1)
		bool megaBadge;

		// Badge header.
		union {
			Badge_PRBS_Header prbs;
			Badge_CABS_Header cabs;
		} badgeHeader;

		// Decoded images.
		rp_image *img[PRBS_MAX];

		/**
		 * Load the badge image.
		 * @param idx Image index.
		 * @return Image, or nullptr on error.
		 */
		const rp_image *loadImage(int idx);
};

/** NintendoBadgePrivate **/

NintendoBadgePrivate::NintendoBadgePrivate(NintendoBadge *q, IRpFile *file)
	: super(q, file)
	, badgeType(BADGE_TYPE_UNKNOWN)
	, megaBadge(false)
{
	// Clear the header structs.
	memset(&badgeHeader, 0, sizeof(badgeHeader));

	// Clear the decoded images.
	memset(img, 0, sizeof(img));
}

NintendoBadgePrivate::~NintendoBadgePrivate()
{
	static_assert(PRBS_MAX == 4, "PRBS_MAX != 4");
	static_assert(PRBS_MAX == ARRAY_SIZE(img), "PRBS_MAX != ARRAY_SIZE(img)");
	for (int i = ARRAY_SIZE(img)-1; i >= 0; i--) {
		delete img[i];
	}
}

/**
 * Load the badge image.
 * @param idx Image index.
 * @return Image, or nullptr on error.
 */
const rp_image *NintendoBadgePrivate::loadImage(int idx)
{
	assert(idx >= 0 || idx < ARRAY_SIZE(img));
	if (idx < 0 || idx >= ARRAY_SIZE(img)) {
		// Invalid image index.
		return nullptr;
	}

	if (img[idx]) {
		// Image has already been loaded.
		return img[idx];
	} else if (!this->file) {
		// Can't load the image.
		return nullptr;
	}

	// Badge sizes.
	// Badge data is RGB565+A4.
	// Badge set data is RGB565 only. (No alpha!)
	static const unsigned int badge64_rgb_sz = BADGE_SIZE_LARGE_W*BADGE_SIZE_LARGE_H*2;
	static const unsigned int badge64_a4_sz  = BADGE_SIZE_LARGE_W*BADGE_SIZE_LARGE_H/2;
	static const unsigned int badge32_rgb_sz = BADGE_SIZE_SMALL_W*BADGE_SIZE_SMALL_H*2;
	static const unsigned int badge32_a4_sz  = BADGE_SIZE_SMALL_W*BADGE_SIZE_SMALL_H/2;

	// Starting address and sizes depend on file type and mega badge status.
	unsigned int start_addr;
	unsigned int badge_rgb_sz, badge_a4_sz, badge_dims;
	bool doMegaBadge = false;
	switch (badgeType) {
		case BADGE_TYPE_PRBS:
			if (megaBadge) {
				// Sanity check: Maximum of 16x16 for mega badges.
				assert(badgeHeader.prbs.mb_width <= 16);
				assert(badgeHeader.prbs.mb_height <= 16);
				if (badgeHeader.prbs.mb_width > 16 ||
				    badgeHeader.prbs.mb_height > 16)
				{
					// Mega Badge is too mega for us.
					return nullptr;
				}
			}

			// The 64x64 badge is located before the 32x32 badge
			// in the file, but we have the smaller one listed first.
			if ((idx & 1) == PRBS_SMALL) {
				// 32x32 badge. (0xA00+0x200)
				badge_rgb_sz = badge32_rgb_sz;
				badge_a4_sz = badge32_a4_sz;
				badge_dims = BADGE_SIZE_SMALL_W;
				start_addr = badge64_rgb_sz + badge64_a4_sz;
			} else {
				// 64x64 badge. (0x2000+0x800)
				badge_rgb_sz = badge64_rgb_sz;
				badge_a4_sz = badge64_a4_sz;
				badge_dims = BADGE_SIZE_LARGE_W;
				start_addr = 0;
			}

			if (idx & 2) {
				// Mega Badge requested.
				assert(megaBadge);
				if (!megaBadge) {
					// Not a Mega Badge.
					return nullptr;
				}
				// Mega Badge tiles start at 0x4300.
				doMegaBadge = true;
				start_addr += 0x4300;
			} else {
				// Standard badge requested.
				// Starts at 0x1100.
				doMegaBadge = false;
				start_addr += 0x1100;
			}
			break;

		case BADGE_TYPE_CABS:
			// CABS is technically 64x64 (0x2000),
			// but it should be cropped to 48x48.
			// No alpha channel.
			assert(idx == 0);
			if (idx != 0) {
				// Invalid index.
				return nullptr;
			}
			start_addr = 0x2080;
			badge_rgb_sz = badge64_rgb_sz;
			badge_a4_sz = 0;
			badge_dims = BADGE_SIZE_LARGE_W;
			break;

		default:
			assert(!"Unknown badge type. (Should not get here!)");
			return nullptr;
	}

	// TODO: Multiple internal image sizes.
	// For now, 64x64 only.
	const unsigned int badge_sz = badge_rgb_sz + badge_a4_sz;
	uint8_t *const badgeData = static_cast<uint8_t*>(aligned_malloc(16, badge_sz));
	if (!badgeData) {
		// Memory allocation failure.
		return nullptr;
	}

	if (!doMegaBadge) {
		// Single badge.
		size_t size = file->seekAndRead(start_addr, badgeData, badge_sz);
		if (size != badge_sz) {
			// Seek and/or read error.
			return nullptr;
		}

		// Convert to rp_image.
		if (badge_a4_sz > 0) {
			img[idx] = ImageDecoder::fromN3DSTiledRGB565_A4(
				badge_dims, badge_dims,
				reinterpret_cast<const uint16_t*>(badgeData), badge_rgb_sz,
				&badgeData[badge_rgb_sz], badge_a4_sz);
		} else {
			img[idx] = ImageDecoder::fromN3DSTiledRGB565(
				badge_dims, badge_dims,
				reinterpret_cast<const uint16_t*>(badgeData), badge_rgb_sz);
		}

		if (badgeType == BADGE_TYPE_CABS) {
			// Need to crop the 64x64 image to 48x48.
			rp_image *img48 = img[idx]->resized(48, 48);
			delete img[idx];
			img[idx] = img48;
		}
	} else {
		// Mega badge. Need to convert each 64x64 badge
		// and concatenate them manually.

		// Mega badge dimensions.
		const unsigned int mb_width     = badgeHeader.prbs.mb_width;
		const unsigned int mb_height    = badgeHeader.prbs.mb_height;
		const unsigned int mb_row_bytes = badge_dims * sizeof(uint32_t);

		// Badges are stored vertically, then horizontally.
		img[idx] = new rp_image(badge_dims * mb_width, badge_dims * mb_height, rp_image::FORMAT_ARGB32);
		for (unsigned int y = 0; y < mb_height; y++) {
			const unsigned int my = y*badge_dims;
			for (unsigned int x = 0; x < mb_width; x++, start_addr += (0x2800+0xA00)) {
				size_t size = file->seekAndRead(start_addr, badgeData, badge_sz);
				if (size != badge_sz) {
					// Seek and/or read error.
					delete img[idx];
					img[idx] = nullptr;
					return nullptr;
				}

				rp_image *mb_img = ImageDecoder::fromN3DSTiledRGB565_A4(
					badge_dims, badge_dims,
					reinterpret_cast<const uint16_t*>(badgeData), badge_rgb_sz,
					&badgeData[badge_rgb_sz], badge_a4_sz);

				// Copy the image into place.
				const unsigned int mx = x*badge_dims;
				for (int py = badge_dims-1; py >= 0; py--) {
					const uint32_t *src = reinterpret_cast<const uint32_t*>(mb_img->scanLine(py));
					uint32_t *dest = reinterpret_cast<uint32_t*>(img[idx]->scanLine(py+my)) + mx;
					memcpy(dest, src, mb_row_bytes);
				}

				delete mb_img;
			}
		}

		// Set the sBIT metadata.
		static const rp_image::sBIT_t sBIT = {5,6,5,0,4};
		img[idx]->set_sBIT(&sBIT);
	}

	return img[idx];
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
	// NOTE: Reading the full size, which should be valid
	// for both PRBS and CABS.
	d->file->rewind();
	size_t size = d->file->read(&d->badgeHeader, sizeof(d->badgeHeader));
	if (size != sizeof(d->badgeHeader))
		return;

	// Check if this badge is supported.
	DetectInfo info;
	info.header.addr = 0;
	info.header.size = sizeof(d->badgeHeader);
	info.header.pData = reinterpret_cast<const uint8_t*>(&d->badgeHeader);
	info.ext = nullptr;	// Not needed for badges.
	info.szFile = 0;	// Not needed for badges.
	d->badgeType = isRomSupported_static(&info);
	d->isValid = (d->badgeType >= 0);
	d->megaBadge = false;	// check later

	if (!d->isValid)
		return;

	// Check for mega badge.
	if (d->badgeType == NintendoBadgePrivate::BADGE_TYPE_PRBS) {
		d->megaBadge = (d->badgeHeader.prbs.mb_width > 1 ||
				d->badgeHeader.prbs.mb_height > 1);
	} else {
		// CABS is a set icon, so no mega badge here.
		d->megaBadge = false;
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
	    info->header.size < std::max(sizeof(Badge_PRBS_Header), sizeof(Badge_CABS_Header)))
	{
		// Either no detection information was specified,
		// or the header is too small.
		return -1;
	}

	// Check for PRBS.
	if (!memcmp(info->header.pData, BADGE_PRBS_MAGIC, 4)) {
		// PRBS header is present.
		// TODO: Other checks?
		return NintendoBadgePrivate::BADGE_TYPE_PRBS;
	} else if (!memcmp(info->header.pData, BADGE_CABS_MAGIC, 4)) {
		// CABS header is present.
		// TODO: Other checks?
		return NintendoBadgePrivate::BADGE_TYPE_CABS;
	}

	// Not supported.
	return -1;
}

/**
 * Get the name of the system the loaded ROM is designed for.
 * @param type System name type. (See the SystemName enum.)
 * @return System name, or nullptr if type is invalid.
 */
const char *NintendoBadge::systemName(unsigned int type) const
{
	RP_D(const NintendoBadge);
	if (!d->isValid || !isSystemNameTypeValid(type))
		return nullptr;

	// PVR has the same name worldwide, so we can
	// ignore the region selection.
	static_assert(SYSNAME_TYPE_MASK == 3,
		"NintendoBadge::systemName() array index optimization needs to be updated.");

	static const char *const sysNames[4] = {
		"Nintendo Badge Arcade", "Badge Arcade", "Badge", nullptr,
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
const char *const *NintendoBadge::supportedFileExtensions_static(void)
{
	static const char *const exts[] = {
		".prb",	// PRBS file
		".cab",	// CABS file (NOTE: Conflicts with Microsoft CAB)

		nullptr
	};
	return exts;
}

/**
 * Get a bitfield of image types this class can retrieve.
 * @return Bitfield of supported image types. (ImageTypesBF)
 */
uint32_t NintendoBadge::supportedImageTypes_static(void)
{
	return IMGBF_INT_ICON | IMGBF_INT_IMAGE;
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
	if (!d->isValid || (imageType != IMG_INT_ICON && imageType != IMG_INT_IMAGE)) {
		return vector<ImageSizeDef>();
	}

	switch (d->badgeType) {
		case NintendoBadgePrivate::BADGE_TYPE_PRBS: {
			// Badges have 32x32 and 64x64 variants.
			// Mega Badges have multiples of those, but they also
			// have 32x32 and 64x64 previews.
			if (imageType == IMG_INT_ICON || !d->megaBadge) {
				// Not a mega badge.
				static const ImageSizeDef imgsz[] = {
					{nullptr, BADGE_SIZE_SMALL_W, BADGE_SIZE_SMALL_H, NintendoBadgePrivate::PRBS_SMALL},
					{nullptr, BADGE_SIZE_LARGE_W, BADGE_SIZE_LARGE_H, NintendoBadgePrivate::PRBS_LARGE},
				};
				return vector<ImageSizeDef>(imgsz,
					imgsz + ARRAY_SIZE(imgsz));
			}

			// Mega Badge.
			// List both the preview and full size images.
			const unsigned int mb_width = d->badgeHeader.prbs.mb_width;
			const unsigned int mb_height = d->badgeHeader.prbs.mb_height;

			const ImageSizeDef imgsz[] = {
				{nullptr, BADGE_SIZE_SMALL_W, BADGE_SIZE_SMALL_H, NintendoBadgePrivate::PRBS_SMALL},
				{nullptr, BADGE_SIZE_LARGE_W, BADGE_SIZE_LARGE_H, NintendoBadgePrivate::PRBS_LARGE},
				{nullptr, (uint16_t)(BADGE_SIZE_SMALL_W*mb_width), (uint16_t)(BADGE_SIZE_SMALL_H*mb_height), NintendoBadgePrivate::PRBS_MEGA_SMALL},
				{nullptr, (uint16_t)(BADGE_SIZE_LARGE_W*mb_width), (uint16_t)(BADGE_SIZE_LARGE_H*mb_height), NintendoBadgePrivate::PRBS_MEGA_LARGE},
			};
			return vector<ImageSizeDef>(imgsz,
				imgsz + ARRAY_SIZE(imgsz));
		}

		case NintendoBadgePrivate::BADGE_TYPE_CABS: {
			// Badge set icons are always 48x48.
			static const ImageSizeDef sz_CABS[] = {
				{nullptr, 48, 48, 0},
			};
			return vector<ImageSizeDef>(sz_CABS,
				sz_CABS + ARRAY_SIZE(sz_CABS));
		}

		default:
			break;
	}

	// Should not get here...
	assert(!"Unknown badge type. (Should not get here!)");
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
uint32_t NintendoBadge::imgpf(ImageType imageType) const
{
	assert(imageType >= IMG_INT_MIN && imageType <= IMG_EXT_MAX);
	if (imageType < IMG_INT_MIN || imageType > IMG_EXT_MAX) {
		// ImageType is out of range.
		return 0;
	}

	switch (imageType) {
		case IMG_INT_ICON:
		case IMG_INT_IMAGE:
			// Badges are 32x32 and 64x64.
			// Badge set icons are 48x48.
			// Always use nearest-neighbor scaling.
			// TODO: Not for Mega Badges?
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

	// Maximum of 7 fields.
	d->fields->reserve(7);

	// Get the system language.
	// TODO: Verify against the region code somehow?
	int lang = NintendoLanguage::getN3DSLanguage();

	// Type.
	switch (d->badgeType) {
		case NintendoBadgePrivate::BADGE_TYPE_PRBS: {
			d->fields->addField_string(C_("NintendoBadge", "Type"),
				d->megaBadge ? C_("NintendoBadge|Type", "Mega Badge")
				             : C_("NintendoBadge|Type", "Individual Badge"));

			// PRBS-specific fields.
			const Badge_PRBS_Header *const prbs = &d->badgeHeader.prbs;

			// Name.
			// Check that the field is valid.
			if (prbs->name[lang][0] == cpu_to_le16(0)) {
				// Not valid. Check English.
				if (prbs->name[N3DS_LANG_ENGLISH][0] != cpu_to_le16(0)) {
					// English is valid.
					lang = N3DS_LANG_ENGLISH;
				} else {
					// Not valid. Check Japanese.
					if (prbs->name[N3DS_LANG_JAPANESE][0] != cpu_to_le16(0)) {
						// Japanese is valid.
						lang = N3DS_LANG_JAPANESE;
					} else {
						// Not valid...
						// TODO: Check other languages?
						lang = -1;
					}
				}
			}
			// NOTE: There aer 16 name entries, but only 12 languages...
			if (lang >= 0 && lang < ARRAY_SIZE(prbs->name)) {
				d->fields->addField_string(C_("NintendoBadge", "Name"),
					utf16le_to_utf8(prbs->name[lang], sizeof(prbs->name[lang])));
			}

			// Badge ID.
			d->fields->addField_string_numeric(C_("NintendoBadge", "Badge ID"),
				le32_to_cpu(prbs->badge_id));

			// Badge filename.
			d->fields->addField_string(C_("NintendoBadge", "Filename"),
				latin1_to_utf8(prbs->filename, sizeof(prbs->filename)));

			// Set name.
			d->fields->addField_string(C_("NintendoBadge", "Set Name"),
				latin1_to_utf8(prbs->setname, sizeof(prbs->setname)));

			// Mega badge size.
			if (d->megaBadge) {
				d->fields->addField_string(C_("NintendoBadge", "Mega Badge Size"),
					rp_sprintf("%ux%u", prbs->mb_width, prbs->mb_height));
			}

			// Title ID.
			if (prbs->title_id.id == cpu_to_le64(0xFFFFFFFFFFFFFFFFULL)) {
				// No title ID.
				d->fields->addField_string(C_("NintendoBadge", "Launch Title ID"),
					C_("NintendoBadge", "None"));
			} else {
				// Title ID is present.
				d->fields->addField_string(C_("NintendoBadge", "Launch Title ID"),
					rp_sprintf("%08X-%08X",
						le32_to_cpu(prbs->title_id.hi),
						le32_to_cpu(prbs->title_id.lo)));

				// Check if this is a known system title.
				const char *region = nullptr;
				const char *title = Nintendo3DSSysTitles::lookup_sys_title(
					le32_to_cpu(prbs->title_id.hi),
					le32_to_cpu(prbs->title_id.lo), &region);
				if (title) {
					// Add optional fields.
					// TODO: Positional arguments.
					string str;
					bool isN3DS = !!(le32_to_cpu(prbs->title_id.lo) & 0x20000000);
					if (isN3DS) {
						if (region) {
							// tr: %1$s == Title name, %2$s == Region
							str = rp_sprintf_p(C_("NintendoBadge", "%1$s (New3DS) (%2$s)"), title, region);
						} else {
							// tr: Title name
							str = rp_sprintf(C_("NintendoBadge", "%s (New3DS)"), title);
						}
					} else {
						if (region) {
							// tr: %1$s == Title name, %2$s == Region
							str = rp_sprintf_p(C_("NintendoBadge", "%1$s (%2$s)"), title, region);
						} else {
							str = title;
						}
					}
					d->fields->addField_string(C_("NintendoBadge", "Launch Title Name"), str);
				}
			}
			break;
		}

		case NintendoBadgePrivate::BADGE_TYPE_CABS: {
			d->fields->addField_string(C_("NintendoBadge", "Type"), C_("NintendoBadge", "Badge Set"));

			// CABS-specific fields.
			const Badge_CABS_Header *const cabs = &d->badgeHeader.cabs;

			// Name.
			// Check that the field is valid.
			if (cabs->name[lang][0] == 0) {
				// Not valid. Check English.
				if (cabs->name[N3DS_LANG_ENGLISH][0] != 0) {
					// English is valid.
					lang = N3DS_LANG_ENGLISH;
				} else {
					// Not valid. Check Japanese.
					if (cabs->name[N3DS_LANG_JAPANESE][0] != 0) {
						// Japanese is valid.
						lang = N3DS_LANG_JAPANESE;
					} else {
						// Not valid...
						// TODO: Check other languages?
						lang = -1;
					}
				}
			}
			// NOTE: There aer 16 name entries, but only 12 languages...
			if (lang >= 0 && lang < ARRAY_SIZE(cabs->name)) {
				d->fields->addField_string(C_("NintendoBadge", "Name"),
					utf16le_to_utf8(cabs->name[lang], sizeof(cabs->name[lang])));
			}

			// Badge ID.
			d->fields->addField_string_numeric(C_("NintendoBadge", "Set ID"), le32_to_cpu(cabs->set_id));

			// Set name.
			d->fields->addField_string(C_("NintendoBadge", "Set Name"),
				latin1_to_utf8(cabs->setname, sizeof(cabs->setname)));
			break;
		}

		default:
			// Unknown.
			assert(!"Unknown badge type. (Should not get here!)");
			d->fields->addField_string(C_("NintendoBadge", "Type"), C_("NintendoBadge", "Unknown"));
			break;
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
	if (!d->file) {
		// File isn't open.
		*pImage = nullptr;
		return -EBADF;
	} else if (!d->isValid) {
		// Badge isn't valid.
		*pImage = nullptr;
		return -EIO;
	}

	// Check the image type.
	int idx;
	switch (imageType) {
		case IMG_INT_ICON:
			// CABS: Use index 0. (only one available)
			// PRBS: Use index 1. (no mega badges)
			// - TODO: Select 64x64 or 32x32 depending on requested size.
			idx = (d->badgeType == NintendoBadgePrivate::BADGE_TYPE_PRBS
				? NintendoBadgePrivate::PRBS_LARGE : 0);
			break;

		case IMG_INT_IMAGE:
			// CABS: Use index 0.
			// PRBS: Use index 1 (64x64), unless it's a Mega Badge,
			// in which case we're using index 3.
			// - TODO: Select mega large or small depending on requested size.
			switch (d->badgeType) {
				case NintendoBadgePrivate::BADGE_TYPE_PRBS:
					idx = (d->megaBadge ? NintendoBadgePrivate::PRBS_MEGA_LARGE : NintendoBadgePrivate::PRBS_LARGE);
					break;
				case NintendoBadgePrivate::BADGE_TYPE_CABS:
					idx = 0;
					break;
				default:
					// Badge isn't valid.
					return -EIO;
			}
			break;

		default:
			// Unsupported image type.
			return -ENOENT;
	}

	// Load the image.
	*pImage = d->loadImage(idx);
	return (*pImage != nullptr ? 0 : -EIO);
}

}
