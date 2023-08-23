/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * Nintendo3DS_SMDH.hpp: Nintendo 3DS SMDH reader.                         *
 * Handles SMDH files and SMDH sections.                                   *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"

#include "Nintendo3DS_SMDH.hpp"
#include "n3ds_structs.h"
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

// Workaround for RP_D() expecting the no-underscore naming convention.
#define Nintendo3DS_SMDHPrivate Nintendo3DS_SMDH_Private

class Nintendo3DS_SMDH_Private final : public RomDataPrivate
{
	public:
		Nintendo3DS_SMDH_Private(const IRpFilePtr &file);
		~Nintendo3DS_SMDH_Private() final = default;

	private:
		typedef RomDataPrivate super;
		RP_DISABLE_COPY(Nintendo3DS_SMDH_Private)

	public:
		/** RomDataInfo **/
		static const char *const exts[];
		static const char *const mimeTypes[];
		static const RomDataInfo romDataInfo;

	public:
		// Internal images.
		// 0 == 24x24; 1 == 48x48
		array<rp_image_ptr, 2> img_icon;

	public:
		// SMDH headers.
		// NOTE: *NOT* byteswapped!
		struct {
			N3DS_SMDH_Header_t header;
			N3DS_SMDH_Icon_t icon;
		} smdh;

		/**
		 * Load the ROM image's icon.
		 * @param idx Image index. (0 == 24x24; 1 == 48x48)
		 * @return Icon, or nullptr on error.
		 */
		rp_image_const_ptr loadIcon(int idx = 1);

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

ROMDATA_IMPL(Nintendo3DS_SMDH)
ROMDATA_IMPL_IMG_TYPES(Nintendo3DS_SMDH)
ROMDATA_IMPL_IMG_SIZES(Nintendo3DS_SMDH)

/** Nintendo3DS_SMDH_Private **/

/* RomDataInfo */
// NOTE: Using the same image settings as Nintendo3DS.
const char *const Nintendo3DS_SMDH_Private::exts[] = {
	".smdh",	// SMDH (icon) file.

	nullptr
};
const char *const Nintendo3DS_SMDH_Private::mimeTypes[] = {
	// Unofficial MIME types.
	// TODO: Get these upstreamed on FreeDesktop.org.
	"application/x-nintendo-3ds-smdh",

	// Unofficial MIME types from Citra.
	"application/x-ctr-smdh",

	nullptr
};
const RomDataInfo Nintendo3DS_SMDH_Private::romDataInfo = {
	"Nintendo3DS", exts, mimeTypes
};

Nintendo3DS_SMDH_Private::Nintendo3DS_SMDH_Private(const IRpFilePtr &file)
	: super(file, &romDataInfo)
{
	// Clear the SMDH headers.
	memset(&smdh, 0, sizeof(smdh));
}

/**
 * Load the ROM image's icon.
 * @param idx Image index. (0 == 24x24; 1 == 48x48)
 * @return Icon, or nullptr on error.
 */
rp_image_const_ptr Nintendo3DS_SMDH_Private::loadIcon(int idx)
{
	assert(idx == 0 || idx == 1);
	if (idx != 0 && idx != 1) {
		// Invalid icon index.
		return nullptr;
	}

	if (img_icon[idx]) {
		// Icon has already been loaded.
		return img_icon[idx];
	} else if (!file || !isValid) {
		// Can't load the icon.
		return nullptr;
	}

	// Make sure the SMDH section is loaded.
	if (smdh.header.magic != cpu_to_be32(N3DS_SMDH_HEADER_MAGIC)) {
		// Not loaded. Cannot load an icon.
		return nullptr;
	}

	// Convert the icon to rp_image.
	// NOTE: Assuming RGB565 format.
	// 3dbrew.org says it could be any of various formats,
	// but only RGB565 has been used so far.
	// Reference: https://www.3dbrew.org/wiki/SMDH#Icon_graphics
	switch (idx) {
		case 0:
			// Small icon. (24x24)
			// NOTE: Some older homebrew, including RxTools,
			// has a broken 24x24 icon.
			img_icon[0] = ImageDecoder::fromN3DSTiledRGB565(
				N3DS_SMDH_ICON_SMALL_W, N3DS_SMDH_ICON_SMALL_H,
				smdh.icon.small, sizeof(smdh.icon.small));
			break;
		case 1:
			// Large icon. (48x48)
			img_icon[1] = ImageDecoder::fromN3DSTiledRGB565(
				N3DS_SMDH_ICON_LARGE_W, N3DS_SMDH_ICON_LARGE_H,
				smdh.icon.large, sizeof(smdh.icon.large));
			break;
		default:
			// Invalid icon index.
			assert(!"Invalid 3DS icon index.");
			return nullptr;
	}

	return img_icon[idx];
}

/**
 * Get the language ID to use for the title fields.
 * @return N3DS language ID.
 */
N3DS_Language_ID Nintendo3DS_SMDH_Private::getLanguageID(void) const
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

	// Check the header fields to determine if the language string is valid.
	if (smdh.header.titles[langID].desc_short[0] == cpu_to_le16('\0')) {
		// Not valid. Check English.
		if (smdh.header.titles[N3DS_LANG_ENGLISH].desc_short[0] != cpu_to_le16('\0')) {
			// English is valid.
			langID = N3DS_LANG_ENGLISH;
		} else {
			// Not valid. Check Japanese.
			if (smdh.header.titles[N3DS_LANG_JAPANESE].desc_short[0] != cpu_to_le16('\0')) {
				// Japanese is valid.
				langID = N3DS_LANG_JAPANESE;
			} else {
				// Not valid...
				// Default to English anyway.
				langID = N3DS_LANG_ENGLISH;
			}
		}
	}

	return langID;
}

/**
 * Get the default language code for the multi-string fields.
 * @return Language code, e.g. 'en' or 'es'.
 */
inline uint32_t Nintendo3DS_SMDH_Private::getDefaultLC(void) const
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

/** Nintendo3DS_SMDH **/

/**
 * Read a Nintendo 3DS SMDH file and/or section.
 *
 * A ROM image must be opened by the caller. The file handle
 * will be ref()'d and must be kept open in order to load
 * data from the disc image.
 *
 * To close the file, either delete this object or call close().
 *
 * NOTE: Check isValid() to determine if this is a valid ROM.
 *
 * @param file Open SMDH file and/or section..
 */
Nintendo3DS_SMDH::Nintendo3DS_SMDH(const IRpFilePtr &file)
	: super(new Nintendo3DS_SMDH_Private(file))
{
	// This class handles SMDH files and/or sections only.
	// NOTE: Using the same image settings as Nintendo3DS.
	RP_D(Nintendo3DS_SMDH);
	d->mimeType = "application/x-nintendo-3ds-smdh";	// unofficial, not on fd.o
	d->fileType = FileType::IconFile;

	if (!d->file) {
		// Could not ref() the file handle.
		return;
	}

	// Read the SMDH section.
	d->file->rewind();
	size_t size = d->file->read(&d->smdh, sizeof(d->smdh));
	if (size != sizeof(d->smdh)) {
		d->smdh.header.magic = 0;
		d->file.reset();
		return;
	}

	// Check if this ROM image is supported.
	const DetectInfo info = {
		{0, sizeof(d->smdh), reinterpret_cast<const uint8_t*>(&d->smdh)},
		nullptr,	// ext (not needed for Nintendo3DS_SMDH)
		0		// szFile (not needed for Nintendo3DS_SMDH)
	};
	d->isValid = (isRomSupported_static(&info) >= 0);

	if (!d->isValid) {
		d->smdh.header.magic = 0;
		d->file.reset();
		return;
	}
}

/** ROM detection functions. **/

/**
 * Is a ROM image supported by this class?
 * @param info DetectInfo containing ROM detection information.
 * @return Class-specific system ID (>= 0) if supported; -1 if not.
 */
int Nintendo3DS_SMDH::isRomSupported_static(const DetectInfo *info)
{
	assert(info != nullptr);
	assert(info->header.pData != nullptr);
	assert(info->header.addr == 0);
	if (!info || !info->header.pData ||
	    info->header.addr != 0 ||
	    info->header.size < 512)
	{
		// Either no detection information was specified,
		// or the header is too small.
		return -1;
	}

	// Check for SMDH.
	const N3DS_SMDH_Header_t *const smdhHeader =
		reinterpret_cast<const N3DS_SMDH_Header_t*>(info->header.pData);
	if (smdhHeader->magic == cpu_to_be32(N3DS_SMDH_HEADER_MAGIC)) {
		// We have an SMDH file.
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
const char *Nintendo3DS_SMDH::systemName(unsigned int type) const
{
	RP_D(const Nintendo3DS_SMDH);
	if (!d->isValid || !isSystemNameTypeValid(type))
		return nullptr;

	static_assert(SYSNAME_TYPE_MASK == 3,
		"Nintendo3DS_SMDH::systemName() array index optimization needs to be updated.");

	unsigned int idx = (type & SYSNAME_TYPE_MASK);

	// "iQue" is only used if the localized system name is requested
	// *and* the ROM's region code is China only.
	if ((type & SYSNAME_REGION_MASK) == SYSNAME_REGION_ROM_LOCAL) {
		// SMDH contains a region code bitfield.
		if (d->smdh.header.settings.region_code == cpu_to_le32(N3DS_REGION_CHINA)) {
			// Chinese exclusive.
			idx |= (1U << 2);
		}
	}

	// Bits 0-1: Type. (long, short, abbreviation)
	// Bit 2: iQue
	// TODO: Is it possible to identify "*New*" Nintendo 3DS" from just the SMDH?
	static const char *const sysNames[4*4] = {
		"Nintendo 3DS", "Nintendo 3DS", "3DS", nullptr,
		"iQue 3DS", "iQue 3DS", "3DS", nullptr,
	};

	return sysNames[idx];
}

/**
 * Get a bitfield of image types this class can retrieve.
 * @return Bitfield of supported image types. (ImageTypesBF)
 */
uint32_t Nintendo3DS_SMDH::supportedImageTypes_static(void)
{
	return IMGBF_INT_ICON;
}

/**
 * Get a list of all available image sizes for the specified image type.
 * @param imageType Image type.
 * @return Vector of available image sizes, or empty vector if no images are available.
 */
vector<RomData::ImageSizeDef> Nintendo3DS_SMDH::supportedImageSizes_static(ImageType imageType)
{
	ASSERT_supportedImageSizes(imageType);

	if (imageType != IMG_INT_ICON) {
		// Only icons are supported.
		return {};
	}

	return {
		{nullptr, 24, 24, 0},
		{nullptr, 48, 48, 1},
	};
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
uint32_t Nintendo3DS_SMDH::imgpf(ImageType imageType) const
{
	ASSERT_imgpf(imageType);

	uint32_t ret = 0;
	switch (imageType) {
		case IMG_INT_ICON:
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
int Nintendo3DS_SMDH::loadFieldData(void)
{
	RP_D(Nintendo3DS_SMDH);
	if (!d->fields.empty()) {
		// Field data *has* been loaded...
		return 0;
	} else if (!d->file || !d->file->isOpen()) {
		// File isn't open.
		return -EBADF;
	} else if (!d->isValid) {
		// SMDH file isn't valid.
		return -EIO;
	}

	// NOTE: Using "Nintendo3DS" as the localization context.

	// Parse the SMDH header.
	const N3DS_SMDH_Header_t *const smdhHeader = &d->smdh.header;
	if (smdhHeader->magic != cpu_to_be32(N3DS_SMDH_HEADER_MAGIC)) {
		// Invalid magic number.
		return 0;
	}

	// Maximum of 5 fields, plus 3 for iQue 3DS.
	const bool is_iQue = (smdhHeader->settings.region_code == cpu_to_le32(N3DS_REGION_CHINA));
	d->fields.reserve(is_iQue ? 8 : 5);
	d->fields.setTabName(0, "SMDH");

	// Title: Check if English is valid.
	// If it is, we'll de-duplicate fields.
	const bool dedupe_titles = (smdhHeader->titles[N3DS_LANG_ENGLISH].desc_short[0] != cpu_to_le16('\0'));

	// Title fields.
	RomFields::StringMultiMap_t *const pMap_desc_short = new RomFields::StringMultiMap_t();
	RomFields::StringMultiMap_t *const pMap_desc_long = new RomFields::StringMultiMap_t();
	RomFields::StringMultiMap_t *const pMap_publisher = new RomFields::StringMultiMap_t();
	for (int langID = 0; langID < N3DS_LANG_MAX; langID++) {
		// Check for empty strings first.
		if (smdhHeader->titles[langID].desc_short[0] == 0 &&
		    smdhHeader->titles[langID].desc_long[0] == 0 &&
		    smdhHeader->titles[langID].publisher[0] == 0)
		{
			// Strings are empty.
			continue;
		}

		if (dedupe_titles && langID != N3DS_LANG_ENGLISH) {
			// Check if the title matches English.
			// NOTE: Not converting to host-endian first, since
			// u16_strncmp() checks for equality and for 0.
			if (!u16_strncmp(smdhHeader->titles[langID].desc_short,
			                 smdhHeader->titles[N3DS_LANG_ENGLISH].desc_short,
					 ARRAY_SIZE(smdhHeader->titles[N3DS_LANG_ENGLISH].desc_short)) &&
			    !u16_strncmp(smdhHeader->titles[langID].desc_long,
			                 smdhHeader->titles[N3DS_LANG_ENGLISH].desc_long,
					 ARRAY_SIZE(smdhHeader->titles[N3DS_LANG_ENGLISH].desc_long)) &&
			    !u16_strncmp(smdhHeader->titles[langID].publisher,
			                 smdhHeader->titles[N3DS_LANG_ENGLISH].publisher,
					 ARRAY_SIZE(smdhHeader->titles[N3DS_LANG_ENGLISH].publisher)))
			{
				// All three title fields match English.
				continue;
			}
		}

		const uint32_t lc = NintendoLanguage::getNDSLanguageCode(langID, N3DS_LANG_MAX-1);
		assert(lc != 0);
		if (lc == 0)
			continue;

		if (smdhHeader->titles[langID].desc_short[0] != cpu_to_le16('\0')) {
			pMap_desc_short->emplace(lc, utf16le_to_utf8(
				smdhHeader->titles[langID].desc_short,
				ARRAY_SIZE(smdhHeader->titles[langID].desc_short)));
		}
		if (smdhHeader->titles[langID].desc_long[0] != cpu_to_le16('\0')) {
			pMap_desc_long->emplace(lc, utf16le_to_utf8(
				smdhHeader->titles[langID].desc_long,
				ARRAY_SIZE(smdhHeader->titles[langID].desc_long)));
		}
		if (smdhHeader->titles[langID].publisher[0] != cpu_to_le16('\0')) {
			pMap_publisher->emplace(lc, utf16le_to_utf8(
				smdhHeader->titles[langID].publisher,
				ARRAY_SIZE(smdhHeader->titles[langID].publisher)));
		}
	}

	const char *const s_title_title = C_("Nintendo3DS", "Title");
	const char *const s_full_title_title = C_("Nintendo3DS", "Full Title");
	const char *const s_publisher_title = C_("Nintendo3DS", "Publisher");
	const char *const s_unknown = C_("RomData", "Unknown");

	const uint32_t def_lc = d->getDefaultLC();
	if (!pMap_desc_short->empty()) {
		d->fields.addField_string_multi(s_title_title, pMap_desc_short, def_lc);
	} else {
		delete pMap_desc_short;
		d->fields.addField_string(s_title_title, s_unknown);
	}
	if (!pMap_desc_long->empty()) {
		d->fields.addField_string_multi(s_full_title_title, pMap_desc_long, def_lc);
	} else {
		delete pMap_desc_long;
		d->fields.addField_string(s_full_title_title, s_unknown);
	}
	if (!pMap_publisher->empty()) {
		d->fields.addField_string_multi(s_publisher_title, pMap_publisher, def_lc);
	} else {
		delete pMap_publisher;
		d->fields.addField_string(s_publisher_title, s_unknown);
	}

	// Region code.
	// Maps directly to the SMDH field.
	static const char *const n3ds_region_bitfield_names[] = {
		NOP_C_("Region", "Japan"),
		NOP_C_("Region", "USA"),
		NOP_C_("Region", "Europe"),
		NOP_C_("Region", "Australia"),
		NOP_C_("Region", "China"),
		NOP_C_("Region", "South Korea"),
		NOP_C_("Region", "Taiwan"),
	};
	vector<string> *const v_n3ds_region_bitfield_names = RomFields::strArrayToVector_i18n(
		"Region", n3ds_region_bitfield_names, ARRAY_SIZE(n3ds_region_bitfield_names));
	d->fields.addField_bitfield(C_("RomData", "Region Code"),
		v_n3ds_region_bitfield_names, 3, le32_to_cpu(smdhHeader->settings.region_code));

	// Age rating(s).
	// Note that not all 16 fields are present on 3DS,
	// though the fields do match exactly, so no
	// mapping is necessary.
	RomFields::age_ratings_t age_ratings;
	// Valid ratings: 0-1, 3-4, 6-10
	static const uint16_t valid_ratings = 0x7DB;

	for (int i = static_cast<int>(age_ratings.size())-1; i >= 0; i--) {
		if (!(valid_ratings & (1U << i))) {
			// Rating is not applicable for NintendoDS.
			age_ratings[i] = 0;
			continue;
		}

		// 3DS ratings field:
		// - 0x1F: Age rating.
		// - 0x20: No age restriction.
		// - 0x40: Rating pending.
		// - 0x80: Rating is valid if set.
		const uint8_t n3ds_rating = smdhHeader->settings.ratings[i];
		if (!(n3ds_rating & 0x80)) {
			// Rating is unused.
			age_ratings[i] = 0;
		} else if (n3ds_rating & 0x40) {
			// Rating pending.
			age_ratings[i] = RomFields::AGEBF_ACTIVE | RomFields::AGEBF_PENDING;
		} else if (n3ds_rating & 0x20) {
			// No age restriction.
			age_ratings[i] = RomFields::AGEBF_ACTIVE | RomFields::AGEBF_NO_RESTRICTION;
		} else {
			// Set active | age value.
			age_ratings[i] = RomFields::AGEBF_ACTIVE | (n3ds_rating & 0x1F);
		}
	}
	d->fields.addField_ageRatings(C_("RomData", "Age Ratings"), age_ratings);

	if (is_iQue) {
		// Check for iQue 3DS fields.
		// NOTE: Stored as ASCII, not UTF-16!
		const char *const ique_data =
			&reinterpret_cast<const char*>(smdhHeader->titles[N3DS_LANG_CHINESE_SIMP].desc_long)[218];
		if (ISDIGIT(ique_data[0])) {
			// Found it.
			// Each field is fixed-width.
			// Format:
			// - ISBN: 17 chars
			// - Contract Reg. No. [合同登记号]: 11 chars, followed by NULL
			// - Publishing Approval No.: 7 chars, formatted as: "新出审字 [2012]555号"
			// TODO: Figure out what "新出审字" means.

			// TODO: Use the fields directly instead of latin1_to_utf8()?

			// ISBN
			d->fields.addField_string(C_("RomData", "ISBN"),
				latin1_to_utf8(&ique_data[0], 17));

			// Contract Reg. No.
			d->fields.addField_string(C_("RomData", "Contract Reg. No."),
				latin1_to_utf8(&ique_data[17], 11));

			// Publishing Approval No.
			// Special formatting for this one.
			// NOTE: MSVC is known to mishandle UTF-8 on certain systems.
			// The UTF-8 text is: "新出审字 [%s]%s号"
			d->fields.addField_string(C_("RomData", "Publishing Approval No."),
				rp_sprintf("\xE6\x96\xB0\xE5\x87\xBA\xE5\xAE\xA1\xE5\xAD\x97 [%s]%s\xE5\x8F\xB7",
					latin1_to_utf8(&ique_data[17+11+1], 4).c_str(),
					latin1_to_utf8(&ique_data[17+11+1+4], 3).c_str()));
		}
	}

	// Finished reading the field data.
	return static_cast<int>(d->fields.count());
}

/**
 * Load metadata properties.
 * Called by RomData::metaData() if the field data hasn't been loaded yet.
 * @return Number of metadata properties read on success; negative POSIX error code on error.
 */
int Nintendo3DS_SMDH::loadMetaData(void)
{
	RP_D(Nintendo3DS_SMDH);
	if (d->metaData != nullptr) {
		// Metadata *has* been loaded...
		return 0;
	} else if (!d->file) {
		// File isn't open.
		return -EBADF;
	} else if (!d->isValid) {
		// SMDH file isn't valid.
		return -EIO;
	}

	// Parse the SMDH header.
	const N3DS_SMDH_Header_t *const smdhHeader = &d->smdh.header;
	if (smdhHeader->magic != cpu_to_be32(N3DS_SMDH_HEADER_MAGIC)) {
		// Invalid magic number.
		return 0;
	}

	// Create the metadata object.
	d->metaData = new RomMetaData();
	d->metaData->reserve(2);	// Maximum of 2 metadata properties.

	// Title.
	// NOTE: Preferring Full Title. If not found, using Title.
	const N3DS_Language_ID langID = d->getLanguageID();
	if (smdhHeader->titles[langID].desc_long[0] != '\0') {
		// Using the Full Title.
		d->metaData->addMetaData_string(Property::Title,
			utf16le_to_utf8(
				smdhHeader->titles[langID].desc_long,
				ARRAY_SIZE(smdhHeader->titles[langID].desc_long)));
	} else if (smdhHeader->titles[langID].desc_short[0] != '\0') {
		// Using the regular Title.
		d->metaData->addMetaData_string(Property::Title,
			utf16le_to_utf8(
				smdhHeader->titles[langID].desc_short,
				ARRAY_SIZE(smdhHeader->titles[langID].desc_short)));
	}

	// Publisher.
	if (smdhHeader->titles[langID].publisher[0] != '\0') {
		d->metaData->addMetaData_string(Property::Publisher,
			utf16le_to_utf8(
				smdhHeader->titles[langID].publisher,
				ARRAY_SIZE(smdhHeader->titles[langID].publisher)));
	}

	// Finished reading the metadata.
	return static_cast<int>(d->metaData->count());
}

/**
 * Load an internal image.
 * Called by RomData::image().
 * @param imageType	[in] Image type to load.
 * @param pImage	[out] Reference to rp_image_const_ptr to store the image in.
 * @return 0 on success; negative POSIX error code on error.
 */
int Nintendo3DS_SMDH::loadInternalImage(ImageType imageType, rp_image_const_ptr &pImage)
{
	ASSERT_loadInternalImage(imageType, pImage);

	// NOTE: Assuming icon index 1. (48x48)
	const int idx = 1;

	RP_D(Nintendo3DS_SMDH);
	if (imageType != IMG_INT_ICON) {
		// Only IMG_INT_ICON is supported by 3DS.
		pImage.reset();
		return -ENOENT;
	} else if (d->img_icon[idx]) {
		// Image has already been loaded.
		pImage = d->img_icon[idx];
		return 0;
	} else if (!d->file) {
		// File isn't open.
		pImage.reset();
		return -EBADF;
	} else if (!d->isValid) {
		// SMDH file isn't valid.
		pImage.reset();
		return -EIO;
	}

	// Load the icon.
	pImage = d->loadIcon(idx);
	return ((bool)pImage ? 0 : -EIO);
}

/** Special SMDH accessor functions. **/

/**
 * Get the SMDH region code.
 * @return SMDH region code, or 0 on error.
 */
uint32_t Nintendo3DS_SMDH::getRegionCode(void) const
{
	RP_D(const Nintendo3DS_SMDH);
	if (d->smdh.header.magic != cpu_to_be32(N3DS_SMDH_HEADER_MAGIC)) {
		// Invalid magic number.
		return 0;
	}
	return le32_to_cpu(d->smdh.header.settings.region_code);
}

}
