/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * GameCubeBNR.cpp: Nintendo GameCube banner reader.                       *
 *                                                                         *
 * Copyright (c) 2016-2020 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "GameCubeBNR.hpp"
#include "data/NintendoLanguage.hpp"

#include "gcn_banner.h"
#include "gcn_structs.h"

// librpbase, librptexture
using namespace LibRpBase;
using namespace LibRpTexture;

// C++ STL classes.
using std::string;
using std::vector;

// Uninitialized vector class.
// Reference: http://andreoffringa.org/?q=uvector
#include "uvector.h"

namespace LibRomData {

ROMDATA_IMPL(GameCubeBNR)
ROMDATA_IMPL_IMG(GameCubeBNR)

class GameCubeBNRPrivate : public RomDataPrivate
{
	public:
		GameCubeBNRPrivate(GameCubeBNR *q, IRpFile *file);
		virtual ~GameCubeBNRPrivate();

	private:
		typedef RomDataPrivate super;
		RP_DISABLE_COPY(GameCubeBNRPrivate)

	public:
		// Banner type.
		enum RomType {
			BANNER_UNKNOWN	= -1,	// Unknown banner type.
			BANNER_BNR1	= 0,	// BNR1 (US/JP)
			BANNER_BNR2	= 1,	// BNR2 (EU)
		};

		// Banner type.
		int bannerType;

		// Internal images.
		rp_image *img_banner;

		// Banner comments.
		// - If BNR1: 1 item.
		// - If BNR2: 6 items.
		ao::uvector<gcn_banner_comment_t> comments;

	public:
		/**
		 * Load the banner.
		 * @return Banner, or nullptr on error.
		 */
		const rp_image *loadBanner(void);

		/**
		 * Get a game information string for the specified comment.
		 *
		 * This is used for addField_gameInfo().
		 *
		 * @param comment gcn_banner_comment_t*
		 * @param gcnRegion GameCube region for BNR1 encoding.
		 * @return Game information string, or empty string on error.
		 */
		static string getGameInfoString(const gcn_banner_comment_t *comment, uint32_t gcnRegion);
};

/** GameCubeBNRPrivate **/

GameCubeBNRPrivate::GameCubeBNRPrivate(GameCubeBNR *q, IRpFile *file)
	: super(q, file)
	, bannerType(BANNER_UNKNOWN)
	, img_banner(nullptr)
{ }

GameCubeBNRPrivate::~GameCubeBNRPrivate()
{
	delete img_banner;
}

/**
 * Load the banner image.
 * @return Banner, or nullptr on error.
 */
const rp_image *GameCubeBNRPrivate::loadBanner(void)
{
	if (img_banner) {
		// Banner is already loaded.
		return img_banner;
	} else if (!this->file || !this->isValid) {
		// Can't load the banner.
		return nullptr;
	}

	// Banner is located at 0x0020.
	auto bannerbuf = aligned_uptr<uint16_t>(16, GCN_BANNER_IMAGE_SIZE/2);
	size_t size = file->seekAndRead(offsetof(gcn_banner_bnr1_t, banner), bannerbuf.get(), GCN_BANNER_IMAGE_SIZE);
	if (size != GCN_BANNER_IMAGE_SIZE) {
		// Seek and/or read error.
		return nullptr;
	}

	// Convert the banner from GCN RGB5A3 format to ARGB32.
	img_banner = ImageDecoder::fromGcn16(ImageDecoder::PXF_RGB5A3,
		GCN_BANNER_IMAGE_W, GCN_BANNER_IMAGE_H,
		bannerbuf.get(), GCN_BANNER_IMAGE_SIZE);
	return img_banner;
}

/**
 * Get a game information string for the specified comment.
 *
 * This is used for addField_gameInfo().
 *
 * @param comment gcn_banner_comment_t*
 * @param gcnRegion GameCube region for BNR1 encoding.
 * @return Game information string, or empty string on error.
 */
string GameCubeBNRPrivate::getGameInfoString(const gcn_banner_comment_t *comment, uint32_t gcnRegion)
{
	// Game info string.
	string s_gameInfo;
	s_gameInfo.reserve(sizeof(gcn_banner_comment_t) + 8);

	// Game name.
	if (comment->gamename_full[0] != '\0') {
		size_t field_len = strnlen(comment->gamename_full, sizeof(comment->gamename_full));
		s_gameInfo.append(comment->gamename_full, field_len);
		s_gameInfo += '\n';
	} else if (comment->gamename[0] != '\0') {
		size_t field_len = strnlen(comment->gamename, sizeof(comment->gamename));
		s_gameInfo.append(comment->gamename, field_len);
		s_gameInfo += '\n';
	}

	// Company.
	// NOTE: This usually has an extra newline at the end,
	// which causes it to show an extra line between the
	// company name and the game description.
	if (comment->company_full[0] != '\0') {
		size_t field_len = strnlen(comment->company_full, sizeof(comment->company_full));
		s_gameInfo.append(comment->company_full, field_len);
		s_gameInfo += '\n';
	} else if (comment->company[0] != '\0') {
		size_t field_len = strnlen(comment->company, sizeof(comment->company));
		s_gameInfo.append(comment->company, field_len);
		s_gameInfo += '\n';
	}

	// Game description.
	if (comment->gamedesc[0] != '\0') {
		// Add a second newline if necessary.
		if (!s_gameInfo.empty()) {
			s_gameInfo += '\n';
		}

		size_t field_len = strnlen(comment->gamedesc, sizeof(comment->gamedesc));
		s_gameInfo.append(comment->gamedesc, field_len);
	}

	// Remove trailing newlines.
	// TODO: Optimize this by using a `for` loop and counter. (maybe ptr)
	while (!s_gameInfo.empty() && s_gameInfo[s_gameInfo.size()-1] == '\n') {
		s_gameInfo.resize(s_gameInfo.size()-1);
	}

	if (!s_gameInfo.empty()) {
		// Convert from cp1252 or Shift-JIS.
		switch (gcnRegion) {
			case GCN_REGION_USA:
			case GCN_REGION_EUR:
			case GCN_REGION_ALL:	// TODO: Assume JP?
			default:
				// USA/PAL uses cp1252.
				s_gameInfo = cp1252_to_utf8(s_gameInfo);
				break;

			case GCN_REGION_JPN:
			case GCN_REGION_KOR:
			case GCN_REGION_CHN:
			case GCN_REGION_TWN:
				// Japan uses Shift-JIS.
				s_gameInfo = cp1252_sjis_to_utf8(s_gameInfo);
				break;
		}
	}

	return s_gameInfo;
}

/** GameCubeBNR **/

/**
 * Read a Nintendo GameCube banner file.
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
GameCubeBNR::GameCubeBNR(IRpFile *file)
	: super(new GameCubeBNRPrivate(this, file))
{
	// This class handles save files.
	// NOTE: This will be handled using the same
	// settings as GameCube.
	RP_D(GameCubeBNR);
	d->className = "GameCube";
	d->mimeType = "application/x-gamecube-bnr";	// unofficial, not on fd.o
	d->fileType = FTYPE_BANNER_FILE;

	if (!d->file) {
		// Could not ref() the file handle.
		return;
	}

	// Read the magic number.
	uint32_t bnr_magic;
	d->file->rewind();
	size_t size = d->file->read(&bnr_magic, sizeof(bnr_magic));
	if (size != sizeof(bnr_magic)) {
		d->file->unref();
		d->file = nullptr;
		return;
	}

	// Check if this file is supported.
	DetectInfo info;
	info.header.addr = 0;
	info.header.size = sizeof(bnr_magic);
	info.header.pData = reinterpret_cast<const uint8_t*>(&bnr_magic);
	info.ext = nullptr;	// Not needed for GameCube banner files.
	info.szFile = d->file->size();
	d->bannerType = isRomSupported_static(&info);
	d->isValid = (d->bannerType >= 0);

	if (!d->isValid) {
		d->file->unref();
		d->file = nullptr;
		return;
	}

	// Read the banner comments.
	unsigned int num;
	switch (d->bannerType) {
		default:
			// Unknown banner type.
			num = 0;
			break;
		case GameCubeBNRPrivate::BANNER_BNR1:
			// US/JP: One comment.
			num = 1;
			break;
		case GameCubeBNRPrivate::BANNER_BNR2:
			// PAL: Six comments.
			num = 6;
			break;
	}

	if (num > 0) {
		// Read the comments.
		d->comments.resize(num);
		const size_t expSize = sizeof(gcn_banner_comment_t) * num;
		size = file->seekAndRead(offsetof(gcn_banner_bnr1_t, comment), d->comments.data(), expSize);
		if (size != expSize) {
			// Seek and/or read error.
			d->comments.clear();
		}
	}
}

/** ROM detection functions. **/

/**
 * Is a ROM image supported by this class?
 * @param info DetectInfo containing ROM detection information.
 * @return Class-specific system ID (>= 0) if supported; -1 if not.
 */
int GameCubeBNR::isRomSupported_static(const DetectInfo *info)
{
	assert(info != nullptr);
	assert(info->header.pData != nullptr);
	assert(info->header.addr == 0);
	if (!info || !info->header.pData ||
	    info->header.addr != 0 ||
	    info->header.size < sizeof(uint32_t))
	{
		// Either no detection information was specified,
		// or the header is too small.
		return -1;
	}

	const uint32_t bnr_magic = be32_to_cpu(
		*(reinterpret_cast<const uint32_t*>(info->header.pData)));

	switch (bnr_magic) {
		case GCN_BANNER_MAGIC_BNR1:
			if (info->szFile >= (off64_t)sizeof(gcn_banner_bnr1_t)) {
				// This is BNR1.
				return GameCubeBNRPrivate::BANNER_BNR1;
			}
			break;
		case GCN_BANNER_MAGIC_BNR2:
			if (info->szFile >= (off64_t)sizeof(gcn_banner_bnr2_t)) {
				// This is BNR2.
				return GameCubeBNRPrivate::BANNER_BNR2;
			}
			// TODO: If size is >= BNR1 but not BNR2, handle as BNR1?
			break;
		default:
			break;
	}

	// Not suported.
	return -1;
}

/**
 * Get the name of the system the loaded ROM is designed for.
 * @param type System name type. (See the SystemName enum.)
 * @return System name, or nullptr if type is invalid.
 */
const char *GameCubeBNR::systemName(unsigned int type) const
{
	RP_D(const GameCubeBNR);
	if (!d->isValid || !isSystemNameTypeValid(type))
		return nullptr;

	// GameCube has the same name worldwide, so we can
	// ignore the region selection.
	static_assert(SYSNAME_TYPE_MASK == 3,
		"GameCubeBNR::systemName() array index optimization needs to be updated.");

	// Bits 0-1: Type. (long, short, abbreviation)
	static const char *const sysNames[4] = {
		// FIXME: "NGC" in Japan?
		"Nintendo GameCube", "GameCube", "GCN", nullptr,
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
const char *const *GameCubeBNR::supportedFileExtensions_static(void)
{
	// Banner is usually "opening.bnr" in the disc's root directory.
	static const char *const exts[] = {
		".bnr",

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
const char *const *GameCubeBNR::supportedMimeTypes_static(void)
{
	static const char *const mimeTypes[] = {
		// Unofficial MIME types.
		// TODO: Get these upstreamed on FreeDesktop.org.
		"application/x-gamecube-bnr",	// .bnr

		nullptr
	};
	return mimeTypes;
}

/**
 * Get a bitfield of image types this class can retrieve.
 * @return Bitfield of supported image types. (ImageTypesBF)
 */
uint32_t GameCubeBNR::supportedImageTypes_static(void)
{
	return IMGBF_INT_BANNER;
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
vector<RomData::ImageSizeDef> GameCubeBNR::supportedImageSizes_static(ImageType imageType)
{
	ASSERT_supportedImageSizes(imageType);

	if (imageType != IMG_INT_BANNER) {
		// Only banners are supported.
		return vector<ImageSizeDef>();
	}

	static const ImageSizeDef sz_INT_BANNER[] = {
		{nullptr, GCN_BANNER_IMAGE_W, GCN_BANNER_IMAGE_H, 0},
	};
	return vector<ImageSizeDef>(sz_INT_BANNER,
		sz_INT_BANNER + ARRAY_SIZE(sz_INT_BANNER));
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
uint32_t GameCubeBNR::imgpf(ImageType imageType) const
{
	ASSERT_imgpf(imageType);

	if (imageType == IMG_INT_BANNER) {
		// Use nearest-neighbor scaling.
		return IMGPF_RESCALE_NEAREST;
	}

	// Nothing else is supported.
	return 0;
}

/**
 * Load field data.
 * Called by RomData::fields() if the field data hasn't been loaded yet.
 * @return Number of fields read on success; negative POSIX error code on error.
 */
int GameCubeBNR::loadFieldData(void)
{
	RP_D(GameCubeBNR);
	if (!d->fields->empty()) {
		// Field data *has* been loaded...
		return 0;
	} else if (!d->file || !d->file->isOpen()) {
		// File isn't open.
		return -EBADF;
	} else if (!d->isValid || d->bannerType < 0) {
		// Unknown banner file type.
		return -EIO;
	}

	if (d->comments.empty()) {
		// Banner comment data wasn't loaded...
		return static_cast<int>(d->fields->count());
	}
	d->fields->reserve(3);	// Maximum of 3 fields.

	// TODO: Show both full and normal?
	// Currently showing full if it's there; otherwise, normal.
	const char *const s_game_name_title = C_("GameCubeBNR", "Game Name");
	const char *const s_company_title = C_("GameCubeBNR", "Company");
	const char *const s_description_title = C_("GameCubeBNR", "Description");

	if (d->bannerType == GameCubeBNRPrivate::BANNER_BNR1) {
		// BNR1: Assuming Shift-JIS with cp1252 fallback.
		// The language is either English or Japanese, so we're
		// using RFT_STRING here.

		// TODO: Improve Shift-JIS detection to eliminate the
		// false positive with Metroid Prime. (GM8E01)

		// Only one banner comment.
		const gcn_banner_comment_t &comment = d->comments.at(0);

		// Game name.
		if (comment.gamename_full[0] != '\0') {
			d->fields->addField_string(s_game_name_title,
				cp1252_sjis_to_utf8(comment.gamename_full, sizeof(comment.gamename_full)));
		} else if (comment.gamename[0] != '\0') {
			d->fields->addField_string(s_game_name_title,
				cp1252_sjis_to_utf8(comment.gamename, sizeof(comment.gamename)));
		}

		// Company.
		if (comment.company_full[0] != '\0') {
			d->fields->addField_string(s_company_title,
				cp1252_sjis_to_utf8(comment.company_full, sizeof(comment.company_full)));
		} else if (comment.company[0] != '\0') {
			d->fields->addField_string(s_company_title,
				cp1252_sjis_to_utf8(comment.company, sizeof(comment.company)));
		}

		// Game description.
		if (comment.gamedesc[0] != '\0') {
			d->fields->addField_string(s_description_title,
				cp1252_sjis_to_utf8(comment.gamedesc, sizeof(comment.gamedesc)));
		}
	} else {
		// BNR2: Assuming cp1252.
		// Multiple languages may be present, so we're using
		// RFT_STRING_MULTI here.

		// Check if English is valid.
		// If it is, we'll de-duplicate fields.
		const gcn_banner_comment_t &comment_en = d->comments.at(GCN_PAL_LANG_ENGLISH);
		bool dedupe_titles = (comment_en.gamename_full[0] != '\0') ||
		                     (comment_en.gamename[0] != '\0');

		// Fields.
		RomFields::StringMultiMap_t *const pMap_gamename = new RomFields::StringMultiMap_t();
		RomFields::StringMultiMap_t *const pMap_company = new RomFields::StringMultiMap_t();
		RomFields::StringMultiMap_t *const pMap_gamedesc = new RomFields::StringMultiMap_t();
		for (int langID = 0; langID < GCN_PAL_LANG_MAX; langID++) {
			const gcn_banner_comment_t &comment = d->comments.at(langID);

			// Check for empty strings first.
			if (comment.gamename_full[0] == '\0' &&
			    comment.gamename[0] == '\0' &&
			    comment.company_full[0] == '\0' &&
			    comment.company[0] == '\0' &&
			    comment.gamedesc[0] == '\0')
			{
				// Strings are empty.
				continue;
			}

			if (dedupe_titles && langID != GCN_PAL_LANG_ENGLISH) {
				// Check if the comments match English.
				if (!strncmp(comment.gamename_full, comment_en.gamename_full,
				             ARRAY_SIZE(comment_en.gamename_full)) &&
				    !strncmp(comment.gamename, comment_en.gamename,
				             ARRAY_SIZE(comment_en.gamename)) &&
				    !strncmp(comment.company_full, comment_en.company_full,
				             ARRAY_SIZE(comment_en.company_full)) &&
				    !strncmp(comment.company, comment_en.company,
				             ARRAY_SIZE(comment_en.company)) &&
				    !strncmp(comment.gamedesc, comment_en.gamedesc,
				             ARRAY_SIZE(comment_en.gamedesc)))
				{
					// All fields match English.
					continue;
				}
			}

			const uint32_t lc = NintendoLanguage::getGcnPalLanguageCode(langID);
			assert(lc != 0);
			if (lc == 0)
				continue;

			// Game name.
			if (comment.gamename_full[0] != '\0') {
				pMap_gamename->insert(std::make_pair(lc,
					cp1252_to_utf8(comment.gamename_full,
					ARRAY_SIZE(comment.gamename_full))));
			} else if (comment.gamename[0] != '\0') {
				pMap_gamename->insert(std::make_pair(lc,
					cp1252_to_utf8(comment.gamename,
					ARRAY_SIZE(comment.gamename))));
			}

			// Company.
			if (comment.company_full[0] != '\0') {
				pMap_company->insert(std::make_pair(lc,
					cp1252_to_utf8(comment.company_full,
					ARRAY_SIZE(comment.company_full))));
			} else if (comment.company[0] != '\0') {
				pMap_company->insert(std::make_pair(lc,
					cp1252_to_utf8(comment.company,
					ARRAY_SIZE(comment.company))));
			}

			// Game description.
			if (comment.gamedesc[0] != '\0') {
				pMap_gamedesc->insert(std::make_pair(lc,
					cp1252_to_utf8(comment.gamedesc,
					ARRAY_SIZE(comment.gamedesc))));
			}
		}

		const uint32_t def_lc = NintendoLanguage::getGcnPalLanguageCode(
			NintendoLanguage::getGcnPalLanguage());
		if (!pMap_gamename->empty()) {
			d->fields->addField_string_multi(s_game_name_title, pMap_gamename, def_lc);
		} else {
			delete pMap_gamename;
		}
		if (!pMap_company->empty()) {
			d->fields->addField_string_multi(s_company_title, pMap_company, def_lc);
		} else {
			delete pMap_company;
		}
		if (!pMap_gamedesc->empty()) {
			d->fields->addField_string_multi(s_description_title, pMap_gamedesc, def_lc);
		} else {
			delete pMap_gamedesc;
		}
	}

	// Finished reading the field data.
	return static_cast<int>(d->fields->count());
}

/**
 * Load metadata properties.
 * Called by RomData::metaData() if the field data hasn't been loaded yet.
 * @return Number of metadata properties read on success; negative POSIX error code on error.
 */
int GameCubeBNR::loadMetaData(void)
{
	RP_D(GameCubeBNR);
	if (d->metaData != nullptr) {
		// Metadata *has* been loaded...
		return 0;
	} else if (!d->file) {
		// File isn't open.
		return -EBADF;
	} else if (!d->isValid || d->bannerType < 0) {
		// Unknown banner file type.
		return -EIO;
	}

	assert(!d->comments.empty());
	if (d->comments.empty()) {
		// No comments...
		return 0;
	}

	// Create the metadata object.
	d->metaData = new RomMetaData();
	d->metaData->reserve(3);	// Maximum of 3 metadata properties.

	// TODO: Show both full and normal?
	// Currently showing full if it's there; otherwise, normal.
	// FIXME: Prince of Persia: The Sands of Time has a full game name in
	// company_full[], and an empty gamename_full[].

	if (d->bannerType == GameCubeBNRPrivate::BANNER_BNR1) {
		// BNR1: Assuming Shift-JIS with cp1252 fallback.
		// TODO: Improve Shift-JIS detection to eliminate the
		// false positive with Metroid Prime. (GM8E01)
		const gcn_banner_comment_t &comment = d->comments.at(0);

		// Game name.
		if (comment.gamename_full[0] != '\0') {
			d->metaData->addMetaData_string(Property::Title,
				cp1252_sjis_to_utf8(comment.gamename_full, sizeof(comment.gamename_full)));
		} else if (comment.gamename[0] != '\0') {
			d->metaData->addMetaData_string(Property::Title,
				cp1252_sjis_to_utf8(comment.gamename, sizeof(comment.gamename)));
		}

		// Company.
		if (comment.company_full[0] != '\0') {
			d->metaData->addMetaData_string(Property::Publisher,
				cp1252_sjis_to_utf8(comment.company_full, sizeof(comment.company_full)));
		} else if (comment.company[0] != '\0') {
			d->metaData->addMetaData_string(Property::Publisher,
				cp1252_sjis_to_utf8(comment.company, sizeof(comment.company)));
		}

		// Game description.
		if (comment.gamedesc[0] != '\0') {
			// TODO: Property::Comment is assumed to be user-added
			// on KDE Dolphin 18.08.1. Needs a description property.
			// Also needs verification on Windows.
			d->metaData->addMetaData_string(Property::Subject,
				cp1252_sjis_to_utf8(comment.gamedesc, sizeof(comment.gamedesc)));
		}
	} else {
		// BNR2: Assuming cp1252.
		int idx = NintendoLanguage::getGcnPalLanguage();
		assert(idx >= 0);
		assert(idx < (int)d->comments.size());
		if (idx < 0 || idx >= (int)d->comments.size()) {
			// Out of range. Default to English.
			idx = GCN_PAL_LANG_ENGLISH;
		}

		// If all of the language-specific fields are empty,
		// revert to English.
		if (idx != GCN_PAL_LANG_ENGLISH) {
			const gcn_banner_comment_t &comment = d->comments.at(idx);
			if (comment.gamename[0] == 0 &&
			    comment.company[0] == 0 &&
			    comment.gamename_full[0] == 0 &&
			    comment.company_full[0] == 0 &&
			    comment.gamedesc[0] == 0)
			{
				// Revert to English.
				idx = GCN_PAL_LANG_ENGLISH;
			}
		}

		const gcn_banner_comment_t &comment = d->comments.at(idx);

		// Game name.
		if (comment.gamename_full[0] != '\0') {
			d->metaData->addMetaData_string(Property::Title,
				cp1252_to_utf8(comment.gamename_full, sizeof(comment.gamename_full)));
		} else if (comment.gamename[0] != '\0') {
			d->metaData->addMetaData_string(Property::Title,
				cp1252_to_utf8(comment.gamename, sizeof(comment.gamename)));
		}

		// Company.
		if (comment.company_full[0] != '\0') {
			d->metaData->addMetaData_string(Property::Publisher,
				cp1252_to_utf8(comment.company_full, sizeof(comment.company_full)));
		} else if (comment.company[0] != '\0') {
			d->metaData->addMetaData_string(Property::Publisher,
				cp1252_to_utf8(comment.company, sizeof(comment.company)));
		}

		// Game description.
		if (comment.gamedesc[0] != '\0') {
			// TODO: Property::Comment is assumed to be user-added
			// on KDE Dolphin 18.08.1. Needs a description property.
			// Also needs verification on Windows.
			d->metaData->addMetaData_string(Property::Subject,
				cp1252_to_utf8(comment.gamedesc, sizeof(comment.gamedesc)));
		}
	}

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
int GameCubeBNR::loadInternalImage(ImageType imageType, const rp_image **pImage)
{
	ASSERT_loadInternalImage(imageType, pImage);
	RP_D(GameCubeBNR);
	ROMDATA_loadInternalImage_single(
		IMG_INT_BANNER,	// ourImageType
		d->file,	// file
		d->isValid,	// isValid
		d->bannerType,	// romType
		d->img_banner,	// imgCache
		d->loadBanner);	// func
}

/** GameCubeBNR accessors. **/

/**
 * Add a field for the GameCube banner.
 *
 * This adds an RFT_STRING field for BNR1, and
 * RFT_STRING_MULTI for BNR2.
 *
 * @param fields RomFields*
 * @param gcnRegion GameCube region for BNR1 encoding.
 * @return 0 on success; negative POSIX error code on error.
 */
int GameCubeBNR::addField_gameInfo(LibRpBase::RomFields *fields, uint32_t gcnRegion) const
{
	RP_D(const GameCubeBNR);
	assert(!d->comments.empty());
	if (d->comments.empty()) {
		// No comments available...
		return -ENOENT;
	}

	// Fields are not necessarily null-terminated.
	// NOTE: We're converting from cp1252 or Shift-JIS
	// *after* concatenating all the strings, which is
	// why we're using strnlen() here.

	// NOTE: Using GameCube for the translation context,
	// since this function is used by GameCube, not GameCubeBNR.
	const char *const game_info_title = C_("GameCube", "Game Info");

	if (d->bannerType == GameCubeBNRPrivate::BANNER_BNR1) {
		// BNR1: Assuming Shift-JIS with cp1252 fallback.
		// The language is either English or Japanese, so we're
		// using RFT_STRING here.

		// TODO: Improve Shift-JIS detection to eliminate the
		// false positive with Metroid Prime. (GM8E01)

		// Only one banner comment.
		const gcn_banner_comment_t *const comment = &d->comments[0];

		// Get the game info string.
		string s_gameInfo = d->getGameInfoString(comment, gcnRegion);

		// Add the field.
		fields->addField_string(game_info_title, s_gameInfo);
	} else {
		// BNR2: Assuming cp1252.
		// Multiple languages may be present, so we're using
		// RFT_STRING_MULTI here.

		// Check if English is valid.
		// If it is, we'll de-duplicate fields.
		const gcn_banner_comment_t &comment_en = d->comments[GCN_PAL_LANG_ENGLISH];
		bool dedupe_titles = (comment_en.gamename_full[0] != '\0') ||
		                     (comment_en.gamename[0] != '\0');

		// Fields.
		RomFields::StringMultiMap_t *const pMap_gameinfo = new RomFields::StringMultiMap_t();
		for (int langID = 0; langID < GCN_PAL_LANG_MAX; langID++) {
			const gcn_banner_comment_t &comment = d->comments[langID];
			if (dedupe_titles && langID != GCN_PAL_LANG_ENGLISH) {
				// Check if the comments match English.
				if (!strncmp(comment.gamename_full, comment_en.gamename_full,
				             ARRAY_SIZE(comment_en.gamename_full)) &&
				    !strncmp(comment.gamename, comment_en.gamename,
				             ARRAY_SIZE(comment_en.gamename)) &&
				    !strncmp(comment.company_full, comment_en.company_full,
				             ARRAY_SIZE(comment_en.company_full)) &&
				    !strncmp(comment.company, comment_en.company,
				             ARRAY_SIZE(comment_en.company)) &&
				    !strncmp(comment.gamedesc, comment_en.gamedesc,
				             ARRAY_SIZE(comment_en.gamedesc)))
				{
					// All fields match English.
					continue;
				}
			}

			const uint32_t lc = NintendoLanguage::getGcnPalLanguageCode(langID);
			assert(lc != 0);
			if (lc == 0)
				continue;

			// Get the game info string.
			// TODO: Always use GCN_REGION_EUR here instead of gcnRegion?
			string s_gameInfo = d->getGameInfoString(&d->comments[langID], gcnRegion);
			pMap_gameinfo->insert(std::make_pair(lc, std::move(s_gameInfo)));
		}

		// Add the field.
		const uint32_t def_lc = NintendoLanguage::getGcnPalLanguageCode(
			NintendoLanguage::getGcnPalLanguage());
		fields->addField_string_multi(game_info_title, pMap_gameinfo, def_lc);
	}

	// Game information field added successfully.
	return 0;
}

}
