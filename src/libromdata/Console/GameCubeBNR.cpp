/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * GameCubeBNR.cpp: Nintendo GameCube banner reader.                       *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "GameCubeBNR.hpp"
#include "data/NintendoLanguage.hpp"

#include "gcn_banner.h"
#include "gcn_structs.h"

// Other rom-properties libraries
#include "librptexture/decoder/ImageDecoder_GCN.hpp"
using namespace LibRpBase;
using namespace LibRpFile;
using namespace LibRpText;
using namespace LibRpTexture;

// C++ STL classes
using std::shared_ptr;
using std::string;
using std::vector;

// Uninitialized vector class.
// Reference: http://andreoffringa.org/?q=uvector
#include "uvector.h"

namespace LibRomData {

class GameCubeBNRPrivate final : public RomDataPrivate
{
	public:
		GameCubeBNRPrivate(const IRpFilePtr &file, uint32_t gcnRegion = ~0U);
		~GameCubeBNRPrivate() final = default;

	private:
		typedef RomDataPrivate super;
		RP_DISABLE_COPY(GameCubeBNRPrivate)

	public:
		/** RomDataInfo **/
		static const char *const exts[];
		static const char *const mimeTypes[];
		static const RomDataInfo romDataInfo;

	public:
		// Banner type.
		enum class BannerType {
			Unknown	= -1,

			BNR1	= 0,	// BNR1 (US/JP)
			BNR2	= 1,	// BNR2 (EU)

			Max
		};
		BannerType bannerType;

		// GameCube region for BNR1 encoding
		uint32_t gcnRegion;

		// Internal images
		shared_ptr<rp_image> img_banner;

		// Banner comments
		// - If BNR1: 1 item.
		// - If BNR2: 6 items.
		ao::uvector<gcn_banner_comment_t> comments;

	public:
		/**
		 * Load the banner.
		 * @return Banner, or nullptr on error.
		 */
		shared_ptr<const rp_image> loadBanner(void);

		/**
		 * Should the string be handled as Shift-JIS?
		 * @param hasCopyrightSymbol True if the first character is '\xA9'.
		 * @return True if it should be handled as Shift-JIS; false if not.
		 */
		bool shouldHandleStringAsShiftJIS(bool hasCopyrightSymbol) const;

		/**
		 * Get the game name string for the specified comment.
		 * The character set is converted before returning the string.
		 *
		 * @param comment gcn_banner_comment_t*
		 * @return Game name string (UTF-8), or empty string on error.
		 */
		string getGameNameString(const gcn_banner_comment_t *comment) const;

		/**
		 * Get the company string for the specified comment.
		 * The character set is converted before returning the string.
		 *
		 * @param comment gcn_banner_comment_t*
		 * @return Company string (UTF-8), or empty string on error.
		 */
		string getCompanyString(const gcn_banner_comment_t *comment) const;

		/**
		 * Get the game description string for the specified comment.
		 * The character set is converted before returning the string.
		 *
		 * @param comment gcn_banner_comment_t*
		 * @return Game description string (UTF-8), or empty string on error.
		 */
		string getGameDescriptionString(const gcn_banner_comment_t *comment) const;

		/**
		 * Get a game information string for the specified comment.
		 * The string is automatically converted to UTF-8.
		 *
		 * This is used for addField_gameInfo().
		 *
		 * @param comment gcn_banner_comment_t*
		 * @return Game information string, or empty string on error.
		 */
		string getGameInfoString(const gcn_banner_comment_t *comment) const;
};

ROMDATA_IMPL(GameCubeBNR)
ROMDATA_IMPL_IMG(GameCubeBNR)

/** GameCubeBNRPrivate **/

/* RomDataInfo */
// NOTE: This will be handled using the same
// settings as GameCube.
const char *const GameCubeBNRPrivate::exts[] = {
	".bnr",

	nullptr
};
const char *const GameCubeBNRPrivate::mimeTypes[] = {
	// Unofficial MIME types.
	// TODO: Get these upstreamed on FreeDesktop.org.
	"application/x-gamecube-bnr",	// .bnr

	nullptr
};
const RomDataInfo GameCubeBNRPrivate::romDataInfo = {
	"GameCube", exts, mimeTypes
};

GameCubeBNRPrivate::GameCubeBNRPrivate(const IRpFilePtr &file, uint32_t gcnRegion)
	: super(file, &romDataInfo)
	, bannerType(BannerType::Unknown)
	, gcnRegion(gcnRegion)
	, img_banner(nullptr)
{ }

/**
 * Load the banner image.
 * @return Banner, or nullptr on error.
 */
shared_ptr<const rp_image> GameCubeBNRPrivate::loadBanner(void)
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
	img_banner = ImageDecoder::fromGcn16(
		ImageDecoder::PixelFormat::RGB5A3,
		GCN_BANNER_IMAGE_W, GCN_BANNER_IMAGE_H,
		bannerbuf.get(), GCN_BANNER_IMAGE_SIZE);
	return img_banner;
}

/**
 * Should the string be handled as Shift-JIS?
 * @param hasCopyrightSymbol True if the first character is '\xA9'.
 * @return True if it should be handled as Shift-JIS; false if not.
 */
bool GameCubeBNRPrivate::shouldHandleStringAsShiftJIS(bool hasCopyrightSymbol) const
{
	if (bannerType == BannerType::BNR2) {
		// BNR2 is always cp1252.
		assert(this->gcnRegion == GCN_REGION_EUR || this->gcnRegion == ~0U);
		return false;
	}

	bool isShiftJIS;

	switch (this->gcnRegion) {
		case GCN_REGION_USA:
		case GCN_REGION_EUR:
			// USA/PAL uses cp1252.
			isShiftJIS = false;
			break;

		case GCN_REGION_JPN:
		case GCN_REGION_ALL:	// Special discs only!
		case GCN_REGION_KOR:
		case GCN_REGION_CHN:
		case GCN_REGION_TWN:
			// Japan uses Shift-JIS.
			// NOTE: Assuming JP encoding if no region is provided.
			isShiftJIS = true;
			break;

		default:
			// Use cp1252 if the first character is '\xA9' (©).
			// Otherwise, use Shift-JIS with cp1252 fallback.
			isShiftJIS = !hasCopyrightSymbol;
	}

	return isShiftJIS;
}

/**
 * Get the game name string for the specified comment.
 * The character set is converted before returning the string.
 *
 * @param comment gcn_banner_comment_t*
 * @return Game name string (UTF-8), or empty string on error.
 */
string GameCubeBNRPrivate::getGameNameString(const gcn_banner_comment_t *comment) const
{
	string s_ret;
	bool hasCopyrightSymbol = false;

	if (comment->gamename_full[0] != '\0') {
		const size_t field_len = strnlen(comment->gamename_full, sizeof(comment->gamename_full));
		s_ret.assign(comment->gamename_full, field_len);
		if ((uint8_t)comment->gamename_full[0] == 0xA9) {
			hasCopyrightSymbol = true;
		}
	} else if (comment->gamename[0] != '\0') {
		const size_t field_len = strnlen(comment->gamename, sizeof(comment->gamename));
		s_ret.assign(comment->gamename, field_len);
		if ((uint8_t)comment->gamename[0] == 0xA9) {
			hasCopyrightSymbol = true;
		}
	}

	return (shouldHandleStringAsShiftJIS(hasCopyrightSymbol))
		? cp1252_sjis_to_utf8(s_ret)
		: cp1252_to_utf8(s_ret);
}

/**
 * Get the company string for the specified comment.
 * The character set is converted before returning the string.
 *
 * @param comment gcn_banner_comment_t*
 * @return Company string (UTF-8), or empty string on error.
 */
string GameCubeBNRPrivate::getCompanyString(const gcn_banner_comment_t *comment) const
{
	string s_ret;
	bool hasCopyrightSymbol = false;

	if (comment->company_full[0] != '\0') {
		const size_t field_len = strnlen(comment->company_full, sizeof(comment->company_full));
		s_ret.assign(comment->company_full, field_len);
		if ((uint8_t)comment->company_full[0] == 0xA9) {
			hasCopyrightSymbol = true;
		}
	} else if (comment->company[0] != '\0') {
		const size_t field_len = strnlen(comment->company, sizeof(comment->company));
		s_ret.assign(comment->company, field_len);
		if ((uint8_t)comment->company[0] == 0xA9) {
			hasCopyrightSymbol = true;
		}
	}

	return (shouldHandleStringAsShiftJIS(hasCopyrightSymbol))
		? cp1252_sjis_to_utf8(s_ret)
		: cp1252_to_utf8(s_ret);
}

/**
 * Get the game description string for the specified comment.
 * The character set is converted before returning the string.
 *
 * @param comment gcn_banner_comment_t*
 * @return Game description string (UTF-8), or empty string on error.
 */
string GameCubeBNRPrivate::getGameDescriptionString(const gcn_banner_comment_t *comment) const
{
	string s_ret;
	bool hasCopyrightSymbol = false;

	if (comment->gamedesc[0] != '\0') {
		const size_t field_len = strnlen(comment->gamedesc, sizeof(comment->gamedesc));
		s_ret.assign(comment->gamedesc, field_len);
		if ((uint8_t)comment->gamedesc[0] == 0xA9) {
			hasCopyrightSymbol = true;
		}
	}

	return (shouldHandleStringAsShiftJIS(hasCopyrightSymbol))
		? cp1252_sjis_to_utf8(s_ret)
		: cp1252_to_utf8(s_ret);
}

/**
 * Get a game information string for the specified comment.
 * The string is automatically converted to UTF-8.
 *
 * This is used for addField_gameInfo().
 *
 * @param comment gcn_banner_comment_t*
 * @return Game information string, or empty string on error.
 */
string GameCubeBNRPrivate::getGameInfoString(const gcn_banner_comment_t *comment) const
{
	// Game info string.
	string s_gameInfo;
	s_gameInfo.reserve(sizeof(gcn_banner_comment_t) + 8);

	// Game name
	string s_tmp = getGameNameString(comment);
	if (!s_tmp.empty()) {
		s_gameInfo.append(s_tmp);
		s_gameInfo += '\n';
	}

	// Company
	// NOTE: This usually has an extra newline at the end,
	// which causes it to show an extra line between the
	// company name and the game description.
	s_tmp = getCompanyString(comment);
	if (!s_tmp.empty()) {
		s_gameInfo.append(s_tmp);
		s_gameInfo += '\n';
	}

	// Game description
	s_tmp = getGameDescriptionString(comment);
	if (!s_tmp.empty()) {
		// Add a second newline if necessary.
		if (!s_gameInfo.empty()) {
			s_gameInfo += '\n';
		}
		s_gameInfo.append(s_tmp);
	}

	// Remove trailing newlines.
	// TODO: Optimize this by using a `for` loop and counter. (maybe ptr)
	while (!s_gameInfo.empty() && s_gameInfo[s_gameInfo.size()-1] == '\n') {
		s_gameInfo.resize(s_gameInfo.size()-1);
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
 * @param file Open banner file
 */
GameCubeBNR::GameCubeBNR(const IRpFilePtr &file)
	: super(new GameCubeBNRPrivate(file))
{
	init();
}

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
 * @param file Open banner file
 */
GameCubeBNR::GameCubeBNR(const IRpFilePtr &file, uint32_t gcnRegion)
	: super(new GameCubeBNRPrivate(file, gcnRegion))
{
	init();
}

/**
 * Common initialization function for the constructors.
 */
void GameCubeBNR::init(void)
{
	// This class handles banner files.
	// NOTE: This will be handled using the same
	// settings as GameCube.
	RP_D(GameCubeBNR);
	d->mimeType = "application/x-gamecube-bnr";	// unofficial, not on fd.o
	d->fileType = FileType::BannerFile;

	if (!d->file) {
		// Could not ref() the file handle.
		return;
	}

	// Read the magic number.
	uint32_t bnr_magic;
	d->file->rewind();
	size_t size = d->file->read(&bnr_magic, sizeof(bnr_magic));
	if (size != sizeof(bnr_magic)) {
		d->file.reset();
		return;
	}

	// Check if this file is supported.
	const DetectInfo info = {
		{0, sizeof(bnr_magic), reinterpret_cast<const uint8_t*>(&bnr_magic)},
		nullptr,	// ext (not needed for GameCubeBNR)
		d->file->size()	// szFile
	};
	d->bannerType = static_cast<GameCubeBNRPrivate::BannerType>(isRomSupported_static(&info));
	d->isValid = ((int)d->bannerType >= 0);

	if (!d->isValid) {
		d->file.reset();
		return;
	}

	// Read the banner comments.
	unsigned int num;
	switch (d->bannerType) {
		default:
			// Unknown banner type.
			num = 0;
			break;
		case GameCubeBNRPrivate::BannerType::BNR1:
			// US/JP: One comment.
			num = 1;
			break;
		case GameCubeBNRPrivate::BannerType::BNR2:
			// PAL: Six comments.
			assert(d->gcnRegion == GCN_REGION_EUR || d->gcnRegion == ~0U);
			num = 6;
			break;
	}

	if (num > 0) {
		// Read the comments.
		d->comments.resize(num);
		const size_t expSize = sizeof(gcn_banner_comment_t) * num;
		size = d->file->seekAndRead(offsetof(gcn_banner_bnr1_t, comment), d->comments.data(), expSize);
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
		return static_cast<int>(GameCubeBNRPrivate::BannerType::Unknown);
	}

	const uint32_t bnr_magic = be32_to_cpu(
		*(reinterpret_cast<const uint32_t*>(info->header.pData)));

	GameCubeBNRPrivate::BannerType bannerType = GameCubeBNRPrivate::BannerType::Unknown;
	switch (bnr_magic) {
		case GCN_BANNER_MAGIC_BNR1:
			if (info->szFile >= (off64_t)sizeof(gcn_banner_bnr1_t)) {
				// This is BNR1.
				bannerType = GameCubeBNRPrivate::BannerType::BNR1;
			}
			break;
		case GCN_BANNER_MAGIC_BNR2:
			if (info->szFile >= (off64_t)sizeof(gcn_banner_bnr2_t)) {
				// This is BNR2.
				bannerType = GameCubeBNRPrivate::BannerType::BNR2;
			}
			// TODO: If size is >= BNR1 but not BNR2, handle as BNR1?
			break;
		default:
			break;
	}

	return static_cast<int>(bannerType);
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
	if (!d->fields.empty()) {
		// Field data *has* been loaded...
		return 0;
	} else if (!d->file || !d->file->isOpen()) {
		// File isn't open.
		return -EBADF;
	} else if (!d->isValid || (int)d->bannerType < 0) {
		// Unknown banner file type.
		return -EIO;
	}

	if (d->comments.empty()) {
		// Banner comment data wasn't loaded...
		return static_cast<int>(d->fields.count());
	}
	d->fields.reserve(3);	// Maximum of 3 fields.

	// TODO: Show both full and normal?
	// Currently showing full if it's there; otherwise, normal.
	const char *const s_game_name_title = C_("GameCubeBNR", "Game Name");
	const char *const s_company_title = C_("GameCubeBNR", "Company");
	const char *const s_description_title = C_("GameCubeBNR", "Description");

	if (d->bannerType == GameCubeBNRPrivate::BannerType::BNR1) {
		// BNR1: Assuming Shift-JIS with cp1252 fallback.
		// The language is either English or Japanese, so we're
		// using RFT_STRING here.

		// Only one banner comment.
		const gcn_banner_comment_t *const comment = &d->comments.at(0);

		// Game name
		string s_tmp = d->getGameNameString(comment);
		if (!s_tmp.empty()) {
			d->fields.addField_string(s_game_name_title, s_tmp);
		}

		// Company
		s_tmp = d->getCompanyString(comment);
		if (!s_tmp.empty()) {
			d->fields.addField_string(s_company_title, s_tmp);
		}

		// Game description
		s_tmp = d->getGameDescriptionString(comment);
		if (!s_tmp.empty()) {
			d->fields.addField_string(s_description_title, s_tmp);
		}
	} else {
		// BNR2: Assuming cp1252.
		// Multiple languages may be present, so we're using
		// RFT_STRING_MULTI here.

		// Check if English is valid.
		// If it is, we'll de-duplicate fields.
		const gcn_banner_comment_t &comment_en = d->comments.at(GCN_PAL_LANG_ENGLISH);
		const bool dedupe_titles = (comment_en.gamename_full[0] != '\0') ||
		                           (comment_en.gamename[0] != '\0');

		// Fields.
		RomFields::StringMultiMap_t *const pMap_gamename = new RomFields::StringMultiMap_t();
		RomFields::StringMultiMap_t *const pMap_company = new RomFields::StringMultiMap_t();
		RomFields::StringMultiMap_t *const pMap_gamedesc = new RomFields::StringMultiMap_t();
		for (int langID = 0; langID < GCN_PAL_LANG_MAX; langID++) {
			const gcn_banner_comment_t *const comment = &d->comments.at(langID);

			// Check for empty strings first.
			if (comment->gamename_full[0] == '\0' &&
			    comment->gamename[0] == '\0' &&
			    comment->company_full[0] == '\0' &&
			    comment->company[0] == '\0' &&
			    comment->gamedesc[0] == '\0')
			{
				// Strings are empty.
				continue;
			}

			if (dedupe_titles && langID != GCN_PAL_LANG_ENGLISH) {
				// Check if the comments match English.
				if (!strncmp(comment->gamename_full, comment_en.gamename_full,
				             ARRAY_SIZE(comment_en.gamename_full)) &&
				    !strncmp(comment->gamename, comment_en.gamename,
				             ARRAY_SIZE(comment_en.gamename)) &&
				    !strncmp(comment->company_full, comment_en.company_full,
				             ARRAY_SIZE(comment_en.company_full)) &&
				    !strncmp(comment->company, comment_en.company,
				             ARRAY_SIZE(comment_en.company)) &&
				    !strncmp(comment->gamedesc, comment_en.gamedesc,
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

			// Game name
			string s_tmp = d->getGameNameString(comment);
			if (!s_tmp.empty()) {
				pMap_gamename->emplace(lc, std::move(s_tmp));
			}

			// Company
			s_tmp = d->getCompanyString(comment);
			if (!s_tmp.empty()) {
				pMap_company->emplace(lc, std::move(s_tmp));
			}

			// Game description.
			s_tmp = d->getGameDescriptionString(comment);
			if (!s_tmp.empty()) {
				pMap_gamedesc->emplace(lc, std::move(s_tmp));
			}
		}

		const uint32_t def_lc = NintendoLanguage::getGcnPalLanguageCode(
			NintendoLanguage::getGcnPalLanguage());
		if (!pMap_gamename->empty()) {
			d->fields.addField_string_multi(s_game_name_title, pMap_gamename, def_lc);
		} else {
			delete pMap_gamename;
		}
		if (!pMap_company->empty()) {
			d->fields.addField_string_multi(s_company_title, pMap_company, def_lc);
		} else {
			delete pMap_company;
		}
		if (!pMap_gamedesc->empty()) {
			d->fields.addField_string_multi(s_description_title, pMap_gamedesc, def_lc);
		} else {
			delete pMap_gamedesc;
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
int GameCubeBNR::loadMetaData(void)
{
	RP_D(GameCubeBNR);
	if (d->metaData != nullptr) {
		// Metadata *has* been loaded...
		return 0;
	} else if (!d->file) {
		// File isn't open.
		return -EBADF;
	} else if (!d->isValid || (int)d->bannerType < 0) {
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

	const gcn_banner_comment_t *comment;
	if (d->bannerType == GameCubeBNRPrivate::BannerType::BNR1) {
		// BNR1: Assuming Shift-JIS with cp1252 fallback.
		comment = &d->comments.at(0);
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

		comment = &d->comments.at(idx);
	}

	// Game name
	d->metaData->addMetaData_string(Property::Title, d->getGameNameString(comment));

	// Company
	d->metaData->addMetaData_string(Property::Publisher, d->getCompanyString(comment));

	// Game description
	d->metaData->addMetaData_string(Property::Description, d->getGameDescriptionString(comment));

	// Finished reading the metadata.
	return static_cast<int>(d->metaData->count());
}

/**
 * Load an internal image.
 * Called by RomData::image().
 * @param imageType	[in] Image type to load.
 * @param pImage	[out] Reference to shared_ptr<const rp_image> to store the image in.
 * @return 0 on success; negative POSIX error code on error.
 */
int GameCubeBNR::loadInternalImage(ImageType imageType, shared_ptr<const rp_image> &pImage)
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
 * @return 0 on success; negative POSIX error code on error.
 */
int GameCubeBNR::addField_gameInfo(LibRpBase::RomFields *fields) const
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

	if (d->bannerType == GameCubeBNRPrivate::BannerType::BNR1) {
		// BNR1: Assuming Shift-JIS with cp1252 fallback.
		// The language is either English or Japanese, so we're
		// using RFT_STRING here.

		// Only one banner comment.
		const gcn_banner_comment_t *const comment = &d->comments[0];

		// Get the game info string.
		const string s_gameInfo = d->getGameInfoString(comment);

		// Add the field.
		fields->addField_string(game_info_title, s_gameInfo);
	} else {
		// BNR2: Assuming cp1252.
		// Multiple languages may be present, so we're using
		// RFT_STRING_MULTI here.

		// Check if English is valid.
		// If it is, we'll de-duplicate fields.
		const gcn_banner_comment_t &comment_en = d->comments[GCN_PAL_LANG_ENGLISH];
		const bool dedupe_titles = (comment_en.gamename_full[0] != '\0') ||
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
			pMap_gameinfo->emplace(lc, d->getGameInfoString(&d->comments[langID]));
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
