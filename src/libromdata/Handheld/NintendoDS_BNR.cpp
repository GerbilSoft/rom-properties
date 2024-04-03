/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * NintendoDS_BNR.cpp: Nintendo DS icon/title data reader.                 *
 * Handles BNR files and icon/title sections.                              *
 *                                                                         *
 * Copyright (c) 2016-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"

#include "NintendoDS_BNR.hpp"
#include "nds_structs.h"
#include "data/NintendoLanguage.hpp"

// Other rom-properties libraries
#include "librptexture/decoder/ImageDecoder_NDS.hpp"
#include "librpbase/SystemRegion.hpp"
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
#define NintendoDS_BNRPrivate NintendoDS_BNR_Private

class NintendoDS_BNR_Private final : public RomDataPrivate
{
public:
	NintendoDS_BNR_Private(const IRpFilePtr &file);

private:
	typedef RomDataPrivate super;
	RP_DISABLE_COPY(NintendoDS_BNR_Private)

public:
	/** RomDataInfo **/
	static const char *const exts[];
	static const char *const mimeTypes[];
	static const RomDataInfo romDataInfo;

public:
	// Icon/title data from the ROM header.
	// NOTE: *NOT* byteswapped!
	NDS_IconTitleData nds_icon_title;

	// Animated icon data
	// This class owns all of the icons in here, so we
	// must delete all of them.
	LibRpBase::IconAnimDataPtr iconAnimData;

	// Pointer to the first frame in iconAnimData.
	// Used when showing a static icon.
	rp_image_const_ptr icon_first_frame;

	/**
	 * Load the ROM image's icon.
	 * @return Icon, or nullptr on error.
	 */
	rp_image_const_ptr loadIcon(void);

	/**
	* Get the maximum supported language for an icon/title version.
	* @param version Icon/title version.
	* @return Maximum supported language.
	*/
	static constexpr NDS_Language_ID getMaxSupportedLanguage(uint16_t version)
	{
		if (version >= NDS_ICON_VERSION_HANS_KO) {
			return NDS_LANG_KOREAN;
		} else if (version >= NDS_ICON_VERSION_HANS) {
			return NDS_LANG_CHINESE_SIMP;
		} else {
			return NDS_LANG_SPANISH;
		}
	}

	/**
	 * Get the language ID to use for the title fields.
	 * @return NDS language ID.
	 */
	NDS_Language_ID getLanguageID(void) const;

	/**
	 * Get the default language code for the multi-string fields.
	 * @return Language code, e.g. 'en' or 'es'.
	 */
	uint32_t getDefaultLC(void) const;

	/**
	 * Calculate the CRC16 of a block of data.
	 * @param buf Buffer
	 * @param size Size of buffer
	 * @return CRC16
	 */
	uint16_t crc16(const uint8_t *buf, size_t size);
};

ROMDATA_IMPL(NintendoDS_BNR)
ROMDATA_IMPL_IMG_TYPES(NintendoDS_BNR)
ROMDATA_IMPL_IMG_SIZES(NintendoDS_BNR)

/** NintendoDS_BNR_Private **/

/* RomDataInfo */
// NOTE: Using the same image settings as Nintendo3DS.
const char *const NintendoDS_BNR_Private::exts[] = {
	".bnr",		// Banner file

	nullptr
};
const char *const NintendoDS_BNR_Private::mimeTypes[] = {
	// Unofficial MIME types.
	// TODO: Get these upstreamed on FreeDesktop.org.
	"application/x-nintendo-ds-bnr",

	nullptr
};
const RomDataInfo NintendoDS_BNR_Private::romDataInfo = {
	"Nintendo3DS", exts, mimeTypes
};

NintendoDS_BNR_Private::NintendoDS_BNR_Private(const IRpFilePtr &file)
	: super(file, &romDataInfo)
{
	// Clear the icon/title data struct.
	memset(&nds_icon_title, 0, sizeof(nds_icon_title));
}

/**
 * Load the ROM image's icon.
 * @return Icon, or nullptr on error.
 */
rp_image_const_ptr NintendoDS_BNR_Private::loadIcon(void)
{
	if (icon_first_frame) {
		// Icon has already been loaded.
		return icon_first_frame;
	} else if (!this->isValid || !this->file) {
		// Can't load the icon.
		return {};
	}

	// Load the icon data.
	// TODO: Only read the first frame unless specifically requested?
	this->iconAnimData = std::make_shared<IconAnimData>();
	iconAnimData->count = 0;

	// Check if a DSi animated icon is present.
	// TODO: Some configuration option to return the standard
	// NDS icon for the standard icon instead of the first frame
	// of the animated DSi icon? (Except for DSiWare...)
	if (le16_to_cpu(nds_icon_title.version) < NDS_ICON_VERSION_DSi ||
	    (nds_icon_title.dsi_icon_seq[0] & cpu_to_le16(0xFF)) == 0)
	{
		// Either this isn't a DSi icon/title struct (pre-v0103),
		// or the animated icon sequence is invalid.

		// Convert the NDS icon to rp_image.
		iconAnimData->frames[0] = ImageDecoder::fromNDS_CI4(32, 32,
			nds_icon_title.icon_data, sizeof(nds_icon_title.icon_data),
			nds_icon_title.icon_pal,  sizeof(nds_icon_title.icon_pal));
		iconAnimData->count = 1;
	} else {
		// Animated icon is present.

		// Maximum number of combinations based on bitmap index,
		// palette index, and flip bits is 256. We don't want to
		// reserve 256 images, so we'll use a map to determine
		// which combinations go to which bitmap.

		// dsi_icon_seq is limited to 64, so there's still a maximum
		// of 64 possible bitmaps.

		// Index: High byte of token.
		// Value: Bitmap index. (0xFF for unused)
		array<uint8_t, 256> arr_bmpUsed;
		arr_bmpUsed.fill(0xFF);

		// Parse the icon sequence.
		uint8_t bmp_idx = 0;
		int seq_idx;
		for (seq_idx = 0; seq_idx < ARRAY_SIZE_I(nds_icon_title.dsi_icon_seq); seq_idx++) {
			const uint16_t seq = le16_to_cpu(nds_icon_title.dsi_icon_seq[seq_idx]);
			const int delay = (seq & 0xFF);
			if (delay == 0) {
				// End of sequence.
				break;
			}

			// Token format: (bits)
			// - 15:    V flip (1=yes, 0=no) [TODO]
			// - 14:    H flip (1=yes, 0=no) [TODO]
			// - 13-11: Palette index.
			// - 10-8:  Bitmap index.
			// - 7-0:   Frame duration. (units of 60 Hz)

			// NOTE: IconAnimData doesn't support arbitrary combinations
			// of palette and bitmap. As a workaround, we'll make each
			// combination a unique bitmap, which means we have a maximum
			// of 64 bitmaps.
			const uint8_t high_token = (seq >> 8);
			if (arr_bmpUsed[high_token] == 0xFF) {
				// Not used yet. Create the bitmap.
				const uint8_t bmp = (high_token & 7);
				const uint8_t pal = (high_token >> 3) & 7;
				rp_image_ptr img = ImageDecoder::fromNDS_CI4(32, 32,
					nds_icon_title.dsi_icon_data[bmp],
					sizeof(nds_icon_title.dsi_icon_data[bmp]),
					nds_icon_title.dsi_icon_pal[pal],
					sizeof(nds_icon_title.dsi_icon_pal[pal]));
				if (high_token & (3U << 6)) {
					// At least one flip bit is set.
					rp_image::FlipOp flipOp = rp_image::FLIP_NONE;
					if (high_token & (1U << 6)) {
						// H-flip
						flipOp = rp_image::FLIP_H;
					}
					if (high_token & (1U << 7)) {
						// V-flip
						flipOp = static_cast<rp_image::FlipOp>(flipOp | rp_image::FLIP_V);
					}
					const rp_image_ptr flipimg = img->flip(flipOp);
					if (flipimg && flipimg->isValid()) {
						img = flipimg;
					}
				}
				iconAnimData->frames[bmp_idx] = img;
				arr_bmpUsed[high_token] = bmp_idx;
				bmp_idx++;
			}
			iconAnimData->seq_index[seq_idx] = arr_bmpUsed[high_token];
			iconAnimData->delays[seq_idx].numer = static_cast<uint16_t>(delay);
			iconAnimData->delays[seq_idx].denom = 60;
			iconAnimData->delays[seq_idx].ms = delay * 1000 / 60;
		}
		iconAnimData->count = bmp_idx;
		iconAnimData->seq_count = seq_idx;
	}

	// NOTE: We're not deleting iconAnimData even if we only have
	// a single icon because iconAnimData() will call loadIcon()
	// if iconAnimData is nullptr.

	// Return a pointer to the first frame.
	icon_first_frame = iconAnimData->frames[iconAnimData->seq_index[0]];
	return icon_first_frame;
}

/**
 * Get the language ID to use for the title fields.
 * @return NDS language ID.
 */
NDS_Language_ID NintendoDS_BNR_Private::getLanguageID(void) const
{
	// Version number check is required for ZH and KO.
	const uint16_t version = le16_to_cpu(nds_icon_title.version);
	NDS_Language_ID langID = (NDS_Language_ID)NintendoLanguage::getNDSLanguage(version);

	// Check that the field is valid.
	if (nds_icon_title.title[langID][0] == cpu_to_le16('\0')) {
		// Not valid. Check English.
		if (nds_icon_title.title[NDS_LANG_ENGLISH][0] != cpu_to_le16('\0')) {
			// English is valid.
			langID = NDS_LANG_ENGLISH;
		} else {
			// Not valid. Check Japanese.
			if (nds_icon_title.title[NDS_LANG_JAPANESE][0] != cpu_to_le16('\0')) {
				// Japanese is valid.
				langID = NDS_LANG_JAPANESE;
			} else {
				// Not valid...
				// Default to English anyway.
				langID = NDS_LANG_ENGLISH;
			}
		}
	}

	return langID;
}

/**
 * Get the default language code for the multi-string fields.
 * @return Language code, e.g. 'en' or 'es'.
 */
uint32_t NintendoDS_BNR_Private::getDefaultLC(void) const
{
	// Get the system language.
	// TODO: Verify against the game's region code?
	const NDS_Language_ID langID = getLanguageID();

	// Version number check is required for ZH and KO.
	const NDS_Language_ID maxID = getMaxSupportedLanguage(
		le16_to_cpu(nds_icon_title.version));
	uint32_t lc = NintendoLanguage::getNDSLanguageCode(langID, maxID);
	if (lc == 0) {
		// Invalid language code...
		// Default to English.
		lc = 'en';
	}
	return lc;
}

/**
 * Calculate the CRC16 of a block of data.
 * @param buf Buffer
 * @param size Size of buffer
 * @return CRC16
 */
uint16_t NintendoDS_BNR_Private::crc16(const uint8_t *buf, size_t size)
{
	// Reference: https://www.reddit.com/r/embedded/comments/1acoobg/crc16_again_with_a_little_gift_for_you_all/
	// NOTE: NDS CRC16 uses polynomial 0x8005.
	uint16_t crc = 0xFFFFU;

	while (size--) {
		/*uint8_t t = crc >> 8 ^ *buf++;
		uint8_t z = t;
		t ^= (t >> 1);
		t ^= (t >> 2);
		t ^= (t >> 4);
		t &= 1;
		t |= (z << 1);

		crc = (crc << 8) ^ ((uint16_t)(t << 15)) ^ ((uint16_t)(t << 1)) ^ ((uint16_t)t);*/
		uint32_t x = ((crc ^ *buf++) & 0xff) << 8;
		uint32_t y = x;

		x ^= x << 1;
		x ^= x << 2;
		x ^= x << 4;

		x  = (x & 0x8000) | (y >> 1);

		crc = (crc >> 8) ^ (x >> 15) ^ (x >> 1) ^ x;
	}

	return crc;
}

/** NintendoDS_BNR **/

/**
 * Read a Nintendo DS BNR file and/or icon/title section.
 *
 * A ROM image must be opened by the caller. The file handle
 * will be ref()'d and must be kept open in order to load
 * data from the disc image.
 *
 * To close the file, either delete this object or call close().
 *
 * NOTE: Check isValid() to determine if this is a valid ROM.
 *
 * @param file Open SMDH file and/or section.
 */
NintendoDS_BNR::NintendoDS_BNR(const IRpFilePtr &file)
	: super(new NintendoDS_BNR_Private(file))
{
	// This class handles BNR files and/or icon/title sections only.
	// NOTE: Using the same image settings as Nintendo3DS.
	RP_D(NintendoDS_BNR);
	d->mimeType = "application/x-nintendo-ds-bnr";	// unofficial, not on fd.o
	d->fileType = FileType::IconFile;

	if (!d->file) {
		// Could not ref() the file handle.
		return;
	}

	// Read the icon/title data.
	d->file->rewind();
	size_t size = d->file->read(&d->nds_icon_title, sizeof(d->nds_icon_title));

	// Make sure we have the correct size based on the version.
	if (size < sizeof(d->nds_icon_title.version)) {
		// Couldn't even load the version number...
		d->file.reset();
		return;
	}

	unsigned int req_size;
	const uint16_t version = le16_to_cpu(d->nds_icon_title.version);
	switch (version) {
		default:
			// Invalid version number.
			assert(!"NDS icon/title version number is invalid.");
			d->file.reset();
			return;
		case NDS_ICON_VERSION_ORIGINAL:
			req_size = NDS_ICON_SIZE_ORIGINAL;
			break;
		case NDS_ICON_VERSION_HANS:
			req_size = NDS_ICON_SIZE_HANS;
			break;
		case NDS_ICON_VERSION_HANS_KO:
			req_size = NDS_ICON_SIZE_HANS_KO;
			break;
		case NDS_ICON_VERSION_DSi:
			req_size = NDS_ICON_SIZE_DSi;
			break;
	}

	if (size < req_size) {
		// Error reading the icon data.
		d->file.reset();
		return;
	}

	// Validate the "reserved" section. (Should be 0.)
	for (unsigned int i = 0; i < sizeof(d->nds_icon_title.reserved1); i++) {
		if (d->nds_icon_title.reserved1[i] != 0) {
			// Non-zero. This is an error.
			d->file.reset();
			return;
		}
	}

	// Validate all CRC16s.
	// NOTE: Unused CRC16s should be 0.
	// NOTE 2: Using cpu_to_le16() so we can memcmp() against nds_icon_title directly.
	uint16_t calc_crc16[4] = {0, 0, 0, 0};
	const uint8_t *const pData = reinterpret_cast<const uint8_t*>(&d->nds_icon_title);
	switch (version) {
		default:
			// Invalid version number.
			assert(!"NDS icon/title version number is invalid.");
			d->file.reset();
			return;

		case NDS_ICON_VERSION_DSi:
			// Verify CRC16 3 [0x1240 - 0x23BF]
			calc_crc16[3] = cpu_to_le16(d->crc16(&pData[0x1240], (0x23C0 - 0x1240)));
			// fall-through

		case NDS_ICON_VERSION_HANS_KO:
			// Verify CRC16 2 [0x0020 - 0x0A3F]
			calc_crc16[2] = cpu_to_le16(d->crc16(&pData[0x0020], (0x0A40 - 0x0020)));
			// fall-through

		case NDS_ICON_VERSION_HANS:
			// Verify CRC16 1 [0x0020 - 0x093F]
			calc_crc16[1] = cpu_to_le16(d->crc16(&pData[0x0020], (0x0940 - 0x0020)));
			// fall-through

		case NDS_ICON_VERSION_ORIGINAL:
			// Verify CRC16 0 [0x0020 - 0x083F]
			calc_crc16[0] = cpu_to_le16(d->crc16(&pData[0x0020], (0x0840 - 0x0020)));
			// fall-through
	}

	if (memcmp(calc_crc16, d->nds_icon_title.crc16, sizeof(calc_crc16)) != 0) {
		// CRC16s are incorrect.
		d->file.reset();
		return;
	}

	// TODO: Verify CRC16s?
	d->isValid = true;
}

/** ROM detection functions **/

/**
 * Is a ROM image supported by this class?
 * @param info DetectInfo containing ROM detection information.
 * @return Class-specific system ID (>= 0) if supported; -1 if not.
 */
int NintendoDS_BNR::isRomSupported_static(const DetectInfo *info)
{
	assert(info != nullptr);
	assert(info->header.pData != nullptr);
	assert(info->header.addr == 0);
	if (!info || !info->header.pData ||
	    info->header.addr != 0 ||
	    info->header.size < sizeof(NDS_IconTitleData::version))
	{
		// Either no detection information was specified,
		// or the header is too small.
		return -1;
	}

	// Use heuristics to determine if this is valid.
	// TODO: Verify CRC16s?
	const NDS_IconTitleData *const nds_icon_title =
		reinterpret_cast<const NDS_IconTitleData*>(info->header.pData);

	unsigned int req_size;
	switch (le16_to_cpu(nds_icon_title->version)) {
		case NDS_ICON_VERSION_ORIGINAL:
			req_size = NDS_ICON_SIZE_ORIGINAL;
			break;
		case NDS_ICON_VERSION_HANS:
			req_size = NDS_ICON_SIZE_HANS;
			break;
		case NDS_ICON_VERSION_HANS_KO:
			req_size = NDS_ICON_SIZE_HANS_KO;
			break;
		case NDS_ICON_VERSION_DSi:
			req_size = NDS_ICON_SIZE_DSi;
			break;
		default:
			// Invalid version number.
			return -1;
	}

	if (info->szFile < req_size) {
		// File is too small...
		return -1;
	}

	// This is probably a supported BNR file.
	return 0;
}

/**
 * Get the name of the system the loaded ROM is designed for.
 * @param type System name type. (See the SystemName enum.)
 * @return System name, or nullptr if type is invalid.
 */
const char *NintendoDS_BNR::systemName(unsigned int type) const
{
	RP_D(const NintendoDS_BNR);
	if (!d->isValid || !isSystemNameTypeValid(type))
		return nullptr;

	// NDS/DSi are mostly the same worldwide, except for China.
	// NOTE: We don't have region information here.
	// Assuming DSi if the version is >= NDS_ICON_VERSION_DSi.
	static_assert(SYSNAME_TYPE_MASK == 3,
		"NintendoDS::systemName() array index optimization needs to be updated.");

	// Bits 0-1: Type. (long, short, abbreviation)
	// Bit 2: 0 for NDS, 1 for DSi-exclusive.
	static const array<const char*, 8> sysNames = {{
		"Nintendo DS", "Nintendo DS", "NDS", nullptr,
		"Nintendo DSi", "Nintendo DSi", "DSi", nullptr,
	}};

	// "iQue" is only used if the localized system name is requested
	// *and* the ROM's region code is China only.
	unsigned int idx = (type & SYSNAME_TYPE_MASK);
	if (le16_to_cpu(d->nds_icon_title.version) >= NDS_ICON_VERSION_DSi) {
		// DSi-exclusive game.
		idx |= (1U << 2);
	}

	return sysNames[idx];
}

/**
 * Get a bitfield of image types this class can retrieve.
 * @return Bitfield of supported image types. (ImageTypesBF)
 */
uint32_t NintendoDS_BNR::supportedImageTypes_static(void)
{
	return IMGBF_INT_ICON;
}

/**
 * Get a list of all available image sizes for the specified image type.
 * @param imageType Image type.
 * @return Vector of available image sizes, or empty vector if no images are available.
 */
vector<RomData::ImageSizeDef> NintendoDS_BNR::supportedImageSizes_static(ImageType imageType)
{
	ASSERT_supportedImageSizes(imageType);

	if (imageType != IMG_INT_ICON) {
		// Only icons are supported.
		return {};
	}

	return {{nullptr, 32, 32, 0}};
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
uint32_t NintendoDS_BNR::imgpf(ImageType imageType) const
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
int NintendoDS_BNR::loadFieldData(void)
{
	RP_D(NintendoDS_BNR);
	if (!d->fields.empty()) {
		// Field data *has* been loaded...
		return 0;
	} else if (!d->file) {
		// No file.
		// A closed file is OK, since we already loaded the data.
		return -EBADF;
	} else if (!d->isValid) {
		// Banner file isn't valid.
		return -EIO;
	}

	// NOTE: Using "Nintendo3DS" as the localization context.

	// Parse the icon/title data.
	const NDS_IconTitleData *const nds_icon_title = &d->nds_icon_title;
	d->fields.reserve(1);	// Maximum of 1 field.

	// Full title: Check if English is valid.
	// If it is, we'll de-duplicate fields.
	const bool dedupe_titles = (nds_icon_title->title[NDS_LANG_ENGLISH][0] != cpu_to_le16(0));

	// Full title field.
	RomFields::StringMultiMap_t *const pMap_full_title = new RomFields::StringMultiMap_t();
	const NDS_Language_ID maxID = d->getMaxSupportedLanguage(
		le16_to_cpu(nds_icon_title->version));
	for (int langID = 0; langID <= maxID; langID++) {
		// Check for empty strings first.
		if (nds_icon_title->title[langID][0] == 0) {
			// Strings are empty.
			continue;
		}

		if (dedupe_titles && langID != NDS_LANG_ENGLISH) {
			// Check if the title matches English.
			// NOTE: Not converting to host-endian first, since
			// u16_strncmp() checks for equality and for 0.
			if (!u16_strncmp(nds_icon_title->title[langID],
					nds_icon_title->title[NDS_LANG_ENGLISH],
					ARRAY_SIZE(nds_icon_title->title[NDS_LANG_ENGLISH])))
			{
				// Full title field matches English.
				continue;
			}
		}

		const uint32_t lc = NintendoLanguage::getNDSLanguageCode(langID, maxID);
		assert(lc != 0);
		if (lc == 0)
			continue;

		if (nds_icon_title->title[langID][0] != cpu_to_le16('\0')) {
			pMap_full_title->emplace(lc, utf16le_to_utf8(
				nds_icon_title->title[langID],
				ARRAY_SIZE(nds_icon_title->title[langID])));
		}
	}

	if (!pMap_full_title->empty()) {
		const uint32_t def_lc = d->getDefaultLC();
		d->fields.addField_string_multi(C_("Nintendo", "Full Title"), pMap_full_title, def_lc);
	} else {
		delete pMap_full_title;
	}

	// Finished reading the field data.
	return static_cast<int>(d->fields.count());
}

/**
 * Load metadata properties.
 * Called by RomData::metaData() if the metadata hasn't been loaded yet.
 * @return Number of metadata properties read on success; negative POSIX error code on error.
 */
int NintendoDS_BNR::loadMetaData(void)
{
	RP_D(NintendoDS_BNR);
	if (d->metaData != nullptr) {
		// Metadata *has* been loaded...
		return 0;
	} else if (!d->file) {
		// No file.
		// A closed file is OK, since we already loaded the data.
		return -EBADF;
	} else if (!d->isValid) {
		// SMDH file isn't valid.
		return -EIO;
	}

	// Parse the icon/title data.
	const NDS_IconTitleData *const nds_icon_title = &d->nds_icon_title;

	// Create the metadata object.
	d->metaData = new RomMetaData();
	d->metaData->reserve(1);	// Maximum of 1 metadata property.

	// Full title
	// TODO: Use the default LC if it's available.
	// For now, default to English.
	if (nds_icon_title->title[NDS_LANG_ENGLISH][0] != cpu_to_le16(0)) {
		string s_title = utf16le_to_utf8(nds_icon_title->title[NDS_LANG_ENGLISH],
			ARRAY_SIZE(nds_icon_title->title[NDS_LANG_ENGLISH]));

		// Adjust the title based on the number of lines.
		const size_t nl_1 = s_title.find('\n');
		if (nl_1 != string::npos) {
			// Found the first newline.
			const size_t nl_2 = s_title.find('\n', nl_1+1);
			if (nl_2 != string::npos) {
				// Found the second newline.
				// Change the first to a space, and remove the third line.
				s_title[nl_1] = ' ';
				s_title.resize(nl_2);
			} else {
				// Only two lines.
				// Remove the second line.
				s_title.resize(nl_1);
			}
		}

		d->metaData->addMetaData_string(Property::Title, s_title);
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
int NintendoDS_BNR::loadInternalImage(ImageType imageType, rp_image_const_ptr &pImage)
{
	ASSERT_loadInternalImage(imageType, pImage);
	RP_D(NintendoDS_BNR);
	ROMDATA_loadInternalImage_single(
		IMG_INT_ICON,		// ourImageType
		d->file,		// file
		d->isValid,		// isValid
		0,			// romType (not used here)
		d->icon_first_frame,	// imgCache
		d->loadIcon);		// func
}

/**
 * Get the animated icon data.
 *
 * Check imgpf for IMGPF_ICON_ANIMATED first to see if this
 * object has an animated icon.
 *
 * @return Animated icon data, or nullptr if no animated icon is present.
 */
IconAnimDataConstPtr NintendoDS_BNR::iconAnimData(void) const
{
	RP_D(const NintendoDS_BNR);
	if (!d->iconAnimData) {
		// Load the icon.
		if (!const_cast<NintendoDS_BNR_Private*>(d)->loadIcon()) {
			// Error loading the icon.
			return nullptr;
		}
		if (!d->iconAnimData) {
			// Still no icon...
			return nullptr;
		}
	}

	if (d->iconAnimData->count <= 1) {
		// Not an animated icon.
		return nullptr;
	}

	// Return the icon animation data.
	return d->iconAnimData;
}

}
