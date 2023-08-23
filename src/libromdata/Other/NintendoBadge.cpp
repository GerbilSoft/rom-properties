/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * NintendoBadge.hpp: Nintendo Badge Arcade image reader.                  *
 *                                                                         *
 * Copyright (c) 2017-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "NintendoBadge.hpp"
#include "badge_structs.h"
#include "Handheld/n3ds_structs.h"
#include "data/Nintendo3DSSysTitles.hpp"
#include "data/NintendoLanguage.hpp"

// Other rom-properties libraries
#include "librptexture/decoder/ImageDecoder_N3DS.hpp"
using namespace LibRpBase;
using namespace LibRpFile;
using namespace LibRpText;
using namespace LibRpTexture;

// C++ STL classes
using std::array;
using std::string;
using std::vector;

namespace LibRomData {

class NintendoBadgePrivate final : public RomDataPrivate
{
	public:
		NintendoBadgePrivate(const IRpFilePtr &file);
		~NintendoBadgePrivate() final = default;

	private:
		typedef RomDataPrivate super;
		RP_DISABLE_COPY(NintendoBadgePrivate)

	public:
		/** RomDataInfo **/
		static const char *const exts[];
		static const char *const mimeTypes[];
		static const RomDataInfo romDataInfo;

	public:
		enum class BadgeType {
			Unknown = -1,

			PRBS	= 0,	// PRBS (individual badge)
			CABS	= 1,	// CABS (set badge)

			Max
		};
		BadgeType badgeType;

		// PRBS badge index
		enum class BadgeIndex_PRBS {
			Small		= 0,	// 32x32
			Large		= 1,	// 64x64
			MegaSmall	= 2,	// Mega Badge: 32x32 tiles
			MegaLarge	= 3,	// Mega Badge: 64x64 tiles

			Max
		};

		// Is this a mega badge? (>1x1)
		bool megaBadge;

	public:
		// Badge header
		// Byteswapped to host-endian on load, except `magic` and `title_id`.
		union {
			Badge_PRBS_Header prbs;
			Badge_CABS_Header cabs;
		} badgeHeader;

		// Decoded images
		array<rp_image_ptr, static_cast<unsigned int>(BadgeIndex_PRBS::Max)> img_badges;

		/**
		 * Load the badge image.
		 * @param idx Image index.
		 * @return Image, or nullptr on error.
		 */
		rp_image_const_ptr loadImage(int idx);

		/**
		 * Get the language ID to use for the title fields.
		 * @return N3DS language ID.
		 */
		N3DS_Language_ID getLanguageID(void) const;

		/**
		 * Get the default language code for the multi-string fields.
		 * @return Language code, e.g. 'en' or 'es'.
		 */
		inline uint32_t getDefaultLC(void) const;
};

ROMDATA_IMPL(NintendoBadge)
ROMDATA_IMPL_IMG_TYPES(NintendoBadge)

/** NintendoBadgePrivate **/

/* RomDataInfo */
const char *const NintendoBadgePrivate::exts[] = {
	// NOTE: These extensions may cause conflicts on
	// Windows if fallback handling isn't working.

	".prb",	// PRBS file
	".cab",	// CABS file (NOTE: Conflicts with Microsoft CAB) [TODO: Unregister?]

	nullptr
};
const char *const NintendoBadgePrivate::mimeTypes[] = {
	// NOTE: Ordering matches BadgeType.

	// Unofficial MIME types.
	// TODO: Get these upstreamed on FreeDesktop.org.
	"application/x-nintendo-badge",
	"application/x-nintendo-badge-set",

	nullptr
};
const RomDataInfo NintendoBadgePrivate::romDataInfo = {
	"NintendoBadge", exts, mimeTypes
};

NintendoBadgePrivate::NintendoBadgePrivate(const IRpFilePtr &file)
	: super(file, &romDataInfo)
	, badgeType(BadgeType::Unknown)
	, megaBadge(false)
{
	static_assert(static_cast<unsigned int>(BadgeIndex_PRBS::Max) == 4,
		"BadgeIndex_PRBS::Max != 4");
	static_assert(static_cast<unsigned int>(BadgeIndex_PRBS::Max) == ARRAY_SIZE(img_badges),
		"BadgeIndex_PRBS::Max != ARRAY_SIZE(img)");

	// Clear the header structs.
	memset(&badgeHeader, 0, sizeof(badgeHeader));

	// Clear the decoded images.
	img_badges.fill(nullptr);
}

/**
 * Load the badge image.
 * @param idx Image index.
 * @return Image, or nullptr on error.
 */
rp_image_const_ptr NintendoBadgePrivate::loadImage(int idx)
{
	assert(idx >= 0 || idx < (int)img_badges.size());
	if (idx < 0 || idx >= (int)img_badges.size()) {
		// Invalid image index.
		return nullptr;
	}

	if (img_badges[idx]) {
		// Image has already been loaded.
		return img_badges[idx];
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
		case BadgeType::PRBS:
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
			if ((idx & 1) == (int)BadgeIndex_PRBS::Small) {
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

		case BadgeType::CABS:
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
	const size_t badge_sz = badge_rgb_sz + badge_a4_sz;
	auto badgeData = aligned_uptr<uint8_t>(16, badge_sz);

	rp_image_ptr img;
	if (!doMegaBadge) {
		// Single badge.
		size_t size = file->seekAndRead(start_addr, badgeData.get(), badge_sz);
		if (size != badge_sz) {
			// Seek and/or read error.
			return nullptr;
		}

		// Convert to rp_image.
		if (badge_a4_sz > 0) {
			img = ImageDecoder::fromN3DSTiledRGB565_A4(
				badge_dims, badge_dims,
				reinterpret_cast<const uint16_t*>(badgeData.get()), badge_rgb_sz,
				badgeData.get() + badge_rgb_sz, badge_a4_sz);
		} else {
			img = ImageDecoder::fromN3DSTiledRGB565(
				badge_dims, badge_dims,
				reinterpret_cast<const uint16_t*>(badgeData.get()), badge_rgb_sz);
		}

		if (badgeType == BadgeType::CABS) {
			// Need to crop the 64x64 image to 48x48.
			const rp_image_ptr img48 = img->resized(48, 48);
			if (img48) {
				img = img48;
			}
		}
	} else {
		// Mega badge. Need to convert each 64x64 badge
		// and concatenate them manually.

		// Mega badge dimensions.
		const unsigned int mb_width     = badgeHeader.prbs.mb_width;
		const unsigned int mb_height    = badgeHeader.prbs.mb_height;
		const unsigned int mb_row_bytes = badge_dims * sizeof(uint32_t);

		// Badges are stored vertically, then horizontally.
		img = std::make_shared<rp_image>(badge_dims * mb_width, badge_dims * mb_height, rp_image::Format::ARGB32);
		for (unsigned int y = 0; y < mb_height; y++) {
			const unsigned int my = y*badge_dims;
			for (unsigned int x = 0; x < mb_width; x++, start_addr += (0x2800+0xA00)) {
				size_t size = file->seekAndRead(start_addr, badgeData.get(), badge_sz);
				if (size != badge_sz) {
					// Seek and/or read error.
					return nullptr;
				}

				const rp_image_ptr mb_img = ImageDecoder::fromN3DSTiledRGB565_A4(
					badge_dims, badge_dims,
					reinterpret_cast<const uint16_t*>(badgeData.get()), badge_rgb_sz,
					badgeData.get() + badge_rgb_sz, badge_a4_sz);

				// Copy the image into place.
				// TODO: Pointer arithmetic instead of rp_image::scanLine().
				const unsigned int mx = x*badge_dims;
				for (int py = badge_dims-1; py >= 0; py--) {
					const uint32_t *src = static_cast<const uint32_t*>(mb_img->scanLine(py));
					uint32_t *dest = static_cast<uint32_t*>(img->scanLine(py+my)) + mx;
					memcpy(dest, src, mb_row_bytes);
				}
			}
		}
	}

	// Set the sBIT metadata.
	static const rp_image::sBIT_t sBIT = {5,6,5,0,4};
	img->set_sBIT(&sBIT);

	img_badges[idx] = img;
	return img;
}

/**
 * Get the language ID to use for the title fields.
 * @return N3DS language ID.
 */
N3DS_Language_ID NintendoBadgePrivate::getLanguageID(void) const
{
	// Get the system language.
	// TODO: Verify against the game's region code?
	N3DS_Language_ID langID = static_cast<N3DS_Language_ID>(NintendoLanguage::getN3DSLanguage());
	assert(langID >= 0);
	assert(langID < N3DS_LANG_MAX);
	if (langID < 0 || langID >= N3DS_LANG_MAX) {
		// This is bad...
		// Default to English.
		langID = N3DS_LANG_ENGLISH;
	}

	if (this->badgeType == BadgeType::PRBS) {
		// Check the header fields to determine if the language string is valid.
		const Badge_PRBS_Header *const prbs = &badgeHeader.prbs;

		if (prbs->name[langID][0] == cpu_to_le16('\0')) {
			// Not valid. Check English.
			if (prbs->name[N3DS_LANG_ENGLISH][0] != cpu_to_le16('\0')) {
				// English is valid.
				langID = N3DS_LANG_ENGLISH;
			} else {
				// Not valid. Check Japanese.
				if (prbs->name[N3DS_LANG_JAPANESE][0] != cpu_to_le16('\0')) {
					// Japanese is valid.
					langID = N3DS_LANG_JAPANESE;
				} else {
					// Not valid...
					// Default to English anyway.
					langID = N3DS_LANG_ENGLISH;
				}
			}
		}
	}

	return langID;
}

/**
 * Get the default language code for the multi-string fields.
 * @return Language code, e.g. 'en' or 'es'.
 */
inline uint32_t NintendoBadgePrivate::getDefaultLC(void) const
{
	// Get the system language.
	// TODO: Verify against the game's region code?
	const N3DS_Language_ID langID = getLanguageID();
	uint32_t lc = NintendoLanguage::getNDSLanguageCode(langID, N3DS_LANG_MAX-1);
	if (lc == 0) {
		// Invalid language code...
		// Default to English.
		lc = 'en';
	}
	return lc;
}

/** NintendoBadge **/

/**
 * Read a Nintendo Badge image file.
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
NintendoBadge::NintendoBadge(const IRpFilePtr &file)
	: super(new NintendoBadgePrivate(file))
{
	// This class handles texture files.
	RP_D(NintendoBadge);
	d->fileType = FileType::TextureFile;

	if (!d->file) {
		// Could not ref() the file handle.
		return;
	}

	// Read the badge header.
	// NOTE: Reading the full size, which should be valid
	// for both PRBS and CABS.
	d->file->rewind();
	size_t size = d->file->read(&d->badgeHeader, sizeof(d->badgeHeader));
	if (size != sizeof(d->badgeHeader)) {
		d->file.reset();
		return;
	}

	// Check if this badge is supported.
	const DetectInfo info = {
		{0, sizeof(d->badgeHeader), reinterpret_cast<const uint8_t*>(&d->badgeHeader)},
		nullptr,	// ext (not needed for NintendoBadge)
		0		// szFile (not needed for NintendoBadge)
	};
	d->badgeType = static_cast<NintendoBadgePrivate::BadgeType>(isRomSupported_static(&info));
	d->isValid = ((int)d->badgeType >= 0);
	d->megaBadge = false;	// check later

	if (!d->isValid) {
		d->file.reset();
		return;
	}

	// Check for mega badge.
	if (d->badgeType == NintendoBadgePrivate::BadgeType::PRBS) {
#if SYS_BYTEORDER == SYS_BIG_ENDIAN
		d->badgeHeader.prbs.badge_id	= le32_to_cpu(d->badgeHeader.prbs.badge_id);
		d->badgeHeader.prbs.mb_width	= le32_to_cpu(d->badgeHeader.prbs.mb_width);
		d->badgeHeader.prbs.mb_height	= le32_to_cpu(d->badgeHeader.prbs.mb_height);
#endif /* SYS_BYTEORDER == SYS_BIG_ENDIAN */
		d->megaBadge = (d->badgeHeader.prbs.mb_width > 1 ||
				d->badgeHeader.prbs.mb_height > 1);
	} else {
#if SYS_BYTEORDER == SYS_BIG_ENDIAN
		d->badgeHeader.cabs.set_id	= le32_to_cpu(d->badgeHeader.cabs.set_id);
#endif /* SYS_BYTEORDER == SYS_BIG_ENDIAN */
		// CABS is a set icon, so no mega badge here.
		d->megaBadge = false;
	}

	// Set the MIME type.
	if ((int)d->badgeType < ARRAY_SIZE_I(d->mimeTypes)-1) {
		d->mimeType = d->mimeTypes[(int)d->badgeType];
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
		return static_cast<int>(NintendoBadgePrivate::BadgeType::Unknown);
	}

	// Check for PRBS.
	NintendoBadgePrivate::BadgeType badgeType;
	const uint32_t magic = *((const uint32_t*)info->header.pData);
	if (magic == cpu_to_be32(BADGE_PRBS_MAGIC)) {
		// PRBS header is present.
		// TODO: Other checks?
		badgeType = NintendoBadgePrivate::BadgeType::PRBS;
	} else if (magic == cpu_to_be32(BADGE_CABS_MAGIC)) {
		// CABS header is present.
		// TODO: Other checks?
		badgeType = NintendoBadgePrivate::BadgeType::CABS;
	} else {
		// Not supported.
		badgeType = NintendoBadgePrivate::BadgeType::Unknown;
	}

	return static_cast<int>(badgeType);
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

	// Nintendo Badge Arcade has the same name worldwide, so we can
	// ignore the region selection.
	static_assert(SYSNAME_TYPE_MASK == 3,
		"NintendoBadge::systemName() array index optimization needs to be updated.");

	static const char *const sysNames[4] = {
		"Nintendo Badge Arcade", "Badge Arcade", "Badge", nullptr,
	};

	return sysNames[type & SYSNAME_TYPE_MASK];
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
	ASSERT_supportedImageSizes(imageType);

	RP_D(const NintendoBadge);
	if (!d->isValid || (imageType != IMG_INT_ICON && imageType != IMG_INT_IMAGE)) {
		return {};
	}

	switch (d->badgeType) {
		case NintendoBadgePrivate::BadgeType::PRBS: {
			// Badges have 32x32 and 64x64 variants.
			// Mega Badges have multiples of those, but they also
			// have 32x32 and 64x64 previews.
			if (imageType == IMG_INT_ICON || !d->megaBadge) {
				// Not a mega badge.
				static const ImageSizeDef imgsz[] = {
					{nullptr, BADGE_SIZE_SMALL_W, BADGE_SIZE_SMALL_H, (int)NintendoBadgePrivate::BadgeIndex_PRBS::Small},
					{nullptr, BADGE_SIZE_LARGE_W, BADGE_SIZE_LARGE_H, (int)NintendoBadgePrivate::BadgeIndex_PRBS::Large},
				};
				return {imgsz, imgsz + ARRAY_SIZE(imgsz)};
			}

			// Mega Badge.
			// List both the preview and full size images.
			const unsigned int mb_width = d->badgeHeader.prbs.mb_width;
			const unsigned int mb_height = d->badgeHeader.prbs.mb_height;

			const ImageSizeDef imgsz[] = {
				{nullptr, BADGE_SIZE_SMALL_W, BADGE_SIZE_SMALL_H, (int)NintendoBadgePrivate::BadgeIndex_PRBS::Small},
				{nullptr, BADGE_SIZE_LARGE_W, BADGE_SIZE_LARGE_H, (int)NintendoBadgePrivate::BadgeIndex_PRBS::Large},
				{nullptr, (uint16_t)(BADGE_SIZE_SMALL_W*mb_width), (uint16_t)(BADGE_SIZE_SMALL_H*mb_height), (int)NintendoBadgePrivate::BadgeIndex_PRBS::MegaSmall},
				{nullptr, (uint16_t)(BADGE_SIZE_LARGE_W*mb_width), (uint16_t)(BADGE_SIZE_LARGE_H*mb_height), (int)NintendoBadgePrivate::BadgeIndex_PRBS::MegaLarge},
			};
			return {imgsz, imgsz + ARRAY_SIZE(imgsz)};
		}

		case NintendoBadgePrivate::BadgeType::CABS: {
			// Badge set icons are always 48x48.
			static const ImageSizeDef sz_CABS[] = {
				{nullptr, 48, 48, 0},
			};
			return {sz_CABS, sz_CABS + ARRAY_SIZE(sz_CABS)};
		}

		default:
			break;
	}

	// Should not get here...
	assert(!"Unknown badge type. (Should not get here!)");
	return {};
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
	ASSERT_imgpf(imageType);

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
	if (!d->fields.empty()) {
		// Field data *has* been loaded...
		return 0;
	} else if (!d->file) {
		// File isn't open.
		return -EBADF;
	} else if (!d->isValid || (int)d->badgeType < 0) {
		// Unknown badge type.
		return -EIO;
	}

	// Maximum of 7 fields.
	d->fields.reserve(7);

	// Get the system language.
	// TODO: Verify against the region code somehow?
	int lang = NintendoLanguage::getN3DSLanguage();

	// Type.
	const char *const s_type_title = C_("NintendoBadge", "Type");
	const char *const s_name_title = C_("NintendoBadge", "Name");
	const char *const s_set_name_title = C_("NintendoBadge", "Set Name");
	switch (d->badgeType) {
		case NintendoBadgePrivate::BadgeType::PRBS: {
			d->fields.addField_string(s_type_title,
				d->megaBadge ? C_("NintendoBadge|Type", "Mega Badge")
				             : C_("NintendoBadge|Type", "Individual Badge"));

			// PRBS-specific fields.
			const Badge_PRBS_Header *const prbs = &d->badgeHeader.prbs;

			// Name: Check if English is valid.
			// If it is, we'll de-duplicate fields.
			const bool dedupe_titles = (prbs->name[N3DS_LANG_ENGLISH][0] != cpu_to_le16('\0'));

			// Name field.
			// NOTE: There are 16 entries for names, but only 12 Nintendo 3DS langauges...
			RomFields::StringMultiMap_t *const pMap_name = new RomFields::StringMultiMap_t();
			for (int langID = 0; langID < N3DS_LANG_MAX; langID++) {
				// Check for empty strings first.
				if (prbs->name[langID][0] == 0) {
					// Strings are empty.
					continue;
				}

				if (dedupe_titles && langID != N3DS_LANG_ENGLISH) {
					// Check if the name matches English.
					// NOTE: Not converting to host-endian first, since
					// u16_strncmp() checks for equality and for 0.
					if (!u16_strncmp(prbs->name[langID], prbs->name[N3DS_LANG_ENGLISH],
					                 ARRAY_SIZE(prbs->name[N3DS_LANG_ENGLISH])))
					{
						// Name matches English.
						continue;
					}
				}

				const uint32_t lc = NintendoLanguage::getNDSLanguageCode(langID, N3DS_LANG_MAX-1);
				assert(lc != 0);
				if (lc == 0)
					continue;

				if (prbs->name[langID][0] != cpu_to_le16('\0')) {
					pMap_name->emplace(lc, utf16le_to_utf8(
						prbs->name[langID], ARRAY_SIZE_I(prbs->name[langID])));
				}
			}

			if (!pMap_name->empty()) {
				const uint32_t def_lc = d->getDefaultLC();
				d->fields.addField_string_multi(s_name_title, pMap_name, def_lc);
			} else {
				delete pMap_name;
				d->fields.addField_string(s_name_title, C_("RomData", "Unknown"));
			}

			// Badge ID.
			d->fields.addField_string_numeric(C_("NintendoBadge", "Badge ID"),
				le32_to_cpu(prbs->badge_id));

			// Badge filename.
			d->fields.addField_string(C_("NintendoBadge", "Filename"),
				latin1_to_utf8(prbs->filename, sizeof(prbs->filename)));

			// Set name.
			d->fields.addField_string(s_set_name_title,
				latin1_to_utf8(prbs->setname, sizeof(prbs->setname)));

			// Mega badge size.
			if (d->megaBadge) {
				d->fields.addField_dimensions(C_("NintendoBadge", "Mega Badge Size"),
					prbs->mb_width, prbs->mb_height);
			}

			// Title ID.
			const char *const launch_title_id_title = C_("NintendoBadge", "Launch Title ID");
			if (prbs->title_id.id == cpu_to_le64(0xFFFFFFFFFFFFFFFFULL)) {
				// No title ID.
				d->fields.addField_string(launch_title_id_title,
					C_("NintendoBadge", "None"));
			} else {
				// Title ID is present.
				d->fields.addField_string(launch_title_id_title,
					rp_sprintf("%08X-%08X",
						le32_to_cpu(prbs->title_id.hi),
						le32_to_cpu(prbs->title_id.lo)));

				// Check if this is a known system title.
				const char *region = nullptr;
				const char *const title = Nintendo3DSSysTitles::lookup_sys_title(
					le32_to_cpu(prbs->title_id.hi),
					le32_to_cpu(prbs->title_id.lo), &region);
				if (title) {
					// Add optional fields.
					string str;
					const bool isN3DS = !!(le32_to_cpu(prbs->title_id.lo) & 0x20000000);
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
					d->fields.addField_string(C_("NintendoBadge", "Launch Title Name"), str);
				}
			}
			break;
		}

		case NintendoBadgePrivate::BadgeType::CABS: {
			d->fields.addField_string(s_type_title, C_("NintendoBadge", "Badge Set"));

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
			if (lang >= 0 && lang < ARRAY_SIZE_I(cabs->name)) {
				d->fields.addField_string(s_name_title,
					utf16le_to_utf8(cabs->name[lang], sizeof(cabs->name[lang])));
			}

			// Badge ID.
			d->fields.addField_string_numeric(C_("NintendoBadge", "Set ID"), cabs->set_id);

			// Set name.
			d->fields.addField_string(s_set_name_title,
				latin1_to_utf8(cabs->setname, sizeof(cabs->setname)));
			break;
		}

		default:
			// Unknown.
			assert(!"Unknown badge type. (Should not get here!)");
			d->fields.addField_string(s_type_title, C_("RomData", "Unknown"));
			break;
	}

	// Finished reading the field data.
	return static_cast<int>(d->fields.count());
}

/**
 * Load an internal image.
 * Called by RomData::image().
 * @param imageType	[in] Image type to load.
 * @param pImage	[out] Reference to rp_image_const_ptr to store the image in.
 * @return 0 on success; negative POSIX error code on error.
 */
int NintendoBadge::loadInternalImage(ImageType imageType, rp_image_const_ptr &pImage)
{
	ASSERT_loadInternalImage(imageType, pImage);

	RP_D(NintendoBadge);
	if (!d->file) {
		// File isn't open.
		pImage.reset();
		return -EBADF;
	} else if (!d->isValid) {
		// Badge isn't valid.
		pImage.reset();
		return -EIO;
	}

	// Check the image type.
	NintendoBadgePrivate::BadgeIndex_PRBS idx;
	switch (imageType) {
		case IMG_INT_ICON:
			// CABS: Use index 0. (only one available)
			// PRBS: Use index 1. (no mega badges)
			// - TODO: Select 64x64 or 32x32 depending on requested size.
			idx = (d->badgeType == NintendoBadgePrivate::BadgeType::PRBS
				? NintendoBadgePrivate::BadgeIndex_PRBS::Large
				: NintendoBadgePrivate::BadgeIndex_PRBS::Small);
			break;

		case IMG_INT_IMAGE:
			// CABS: Use index 0.
			// PRBS: Use index 1 (64x64), unless it's a Mega Badge,
			// in which case we're using index 3.
			// - TODO: Select mega large or small depending on requested size.
			switch (d->badgeType) {
				case NintendoBadgePrivate::BadgeType::PRBS:
					idx = (d->megaBadge
						? NintendoBadgePrivate::BadgeIndex_PRBS::MegaLarge
						: NintendoBadgePrivate::BadgeIndex_PRBS::Large);
					break;
				case NintendoBadgePrivate::BadgeType::CABS:
					idx = NintendoBadgePrivate::BadgeIndex_PRBS::Small;
					break;
				default:
					// Badge isn't valid.
					pImage.reset();
					return -EIO;
			}
			break;

		default:
			// Unsupported image type.
			pImage.reset();
			return -ENOENT;
	}

	// Load the image.
	pImage = d->loadImage((int)idx);
	return ((bool)pImage ? 0 : -EIO);
}

}
