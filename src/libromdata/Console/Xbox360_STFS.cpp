/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * Xbox360_STFS.cpp: Microsoft Xbox 360 package reader.                    *
 *                                                                         *
 * Copyright (c) 2016-2020 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "Xbox360_STFS.hpp"
#include "xbox360_stfs_structs.h"
#include "data/Xbox360_STFS_ContentType.hpp"

// for language codes
#include "xbox360_xdbf_structs.h"
#include "data/XboxLanguage.hpp"

// librpbase, librptexture
#include "librpbase/file/RpMemFile.hpp"
#include "librpbase/img/RpPng.hpp"
using namespace LibRpBase;
using namespace LibRpTexture;

// C++ STL classes.
using std::string;
using std::vector;


namespace LibRomData {

ROMDATA_IMPL(Xbox360_STFS)
ROMDATA_IMPL_IMG_TYPES(Xbox360_STFS)

// Workaround for RP_D() expecting the no-underscore naming convention.
#define Xbox360_STFSPrivate Xbox360_STFS_Private

class Xbox360_STFS_Private : public RomDataPrivate
{
	public:
		Xbox360_STFS_Private(Xbox360_STFS *q, IRpFile *file);
		~Xbox360_STFS_Private();

	private:
		typedef RomDataPrivate super;
		RP_DISABLE_COPY(Xbox360_STFS_Private)

	public:
		// STFS type.
		enum StfsType {
			STFS_TYPE_UNKNOWN = -1,	// Unknown STFS type.

			STFS_TYPE_CON	= 0,	// Console-signed
			STFS_TYPE_PIRS	= 1,	// MS-signed for non-Xbox Live
			STFS_TYPE_LIVE	= 2,	// MS-signed for Xbox Live

			STFS_TYPE_MAX
		};
		int stfsType;

		// Icon.
		// NOTE: Currently using Title Thumbnail.
		// Should we make regular Thumbnail available too?
		rp_image *img_icon;

		/**
		 * Load the icon.
		 * @return Icon, or nullptr on error.
		 */
		const rp_image *loadIcon(void);

		/**
		 * Get the default language code for the multi-string fields.
		 * @return Language code, e.g. 'en' or 'es'.
		 */
		inline uint32_t getDefaultLC(void) const;

	public:
		// STFS headers.
		// NOTE: These are **NOT** byteswapped!
		STFS_Package_Header stfsHeader;

		// Load-on-demand headers.
		STFS_Package_Metadata stfsMetadata;
		STFS_Package_Thumbnails stfsThumbnails;

		enum StfsPresent_e {
			STFS_PRESENT_HEADER	= (1U << 0),
			STFS_PRESENT_METADATA	= (1U << 1),
			STFS_PRESENT_THUMBNAILS	= (1U << 2),
		};
		uint32_t headers_loaded;	// StfsPresent_e

		/**
		 * Ensure the specified header is loaded.
		 * @param header Header ID. (See StfsPresent_e.)
		 * @return 0 on success; negative POSIX error code on error.
		 */
		int loadHeader(unsigned int header);
};

/** Xbox360_STFS_Private **/

Xbox360_STFS_Private::Xbox360_STFS_Private(Xbox360_STFS *q, IRpFile *file)
	: super(q, file)
	, stfsType(STFS_TYPE_UNKNOWN)
	, img_icon(nullptr)
	, headers_loaded(0)
{
	// Clear the headers.
	memset(&stfsHeader, 0, sizeof(stfsHeader));
	memset(&stfsMetadata, 0, sizeof(stfsMetadata));
	memset(&stfsThumbnails, 0, sizeof(stfsThumbnails));
}

Xbox360_STFS_Private::~Xbox360_STFS_Private()
{
	delete img_icon;
}

/**
 * Load the icon.
 * @return Icon, or nullptr on error.
 */
const rp_image *Xbox360_STFS_Private::loadIcon(void)
{
	if (img_icon) {
		// Icon has already been loaded.
		return img_icon;
	} else if (!this->isValid || this->stfsType < 0) {
		// Can't load the icon.
		return nullptr;
	}

	// Make sure the STFS metadata and thumbnails are loaded.
	int ret = loadHeader(STFS_PRESENT_METADATA);
	if (ret != 0) {
		// Not loaded and unable to load.
		return nullptr;
	}
	ret = loadHeader(STFS_PRESENT_THUMBNAILS);
	if (ret != 0) {
		// Not loaded and unable to load.
		return nullptr;
	}

	// TODO: Option to select title or regular thumbnail.
	const uint32_t metadata_version = be32_to_cpu(stfsMetadata.metadata_version);
	const uint8_t *pIconData;
	size_t iconSize;

	// Try the title thumbnail image first.
	if (metadata_version < 2) {
		// version 0 or 1
		pIconData = stfsThumbnails.mdv0.title_thumbnail_image;
		iconSize = sizeof(stfsThumbnails.mdv0.title_thumbnail_image);
	} else {
		// version 2 or later
		pIconData = stfsThumbnails.mdv2.title_thumbnail_image;
		iconSize = sizeof(stfsThumbnails.mdv2.title_thumbnail_image);
	}

	// Create an RpMemFile and decode the image.
	// TODO: For rpcli, shortcut to extract the PNG directly.
	RpMemFile *f_mem = new RpMemFile(pIconData, iconSize);
	rp_image *img = RpPng::load(f_mem);
	f_mem->unref();

	if (!img) {
		// Unable to load the title thumbnail image.
		// Try the regular thumbnail image.
		if (metadata_version < 2) {
			// version 0 or 1
			pIconData = stfsThumbnails.mdv0.thumbnail_image;
			iconSize = sizeof(stfsThumbnails.mdv0.thumbnail_image);
		} else {
			// version 2 or later
			pIconData = stfsThumbnails.mdv2.thumbnail_image;
			iconSize = sizeof(stfsThumbnails.mdv2.thumbnail_image);
		}

		f_mem = new RpMemFile(pIconData, iconSize);
		img = RpPng::load(f_mem);
		f_mem->unref();
	}

	this->img_icon = img;
	return img;
}

/**
 * Get the default language code for the multi-string fields.
 * @return Language code, e.g. 'en' or 'es'.
 */
inline uint32_t Xbox360_STFS_Private::getDefaultLC(void) const
{
	// Get the system language.
	// TODO: Does STFS have a default language field?
	const XDBF_Language_e langID = static_cast<XDBF_Language_e>(XboxLanguage::getXbox360Language());
	assert(langID > XDBF_LANGUAGE_UNKNOWN);
	assert(langID < XDBF_LANGUAGE_MAX);
	if (langID <= XDBF_LANGUAGE_UNKNOWN || langID >= XDBF_LANGUAGE_MAX) {
		// Invalid language ID.
		// Default to English.
		return 'en';
	}

	uint32_t lc = XboxLanguage::getXbox360LanguageCode(langID);
	if (lc == 0) {
		// Invalid language code...
		// Default to English.
		lc = 'en';
	}
	return lc;
}

/**
 * Ensure the specified header is loaded.
 * @param header Header ID. (See StfsPresent_e.)
 * @return 0 on success; negative POSIX error code on error.
 */
int Xbox360_STFS_Private::loadHeader(unsigned int header)
{
	// TODO: Ensure that only a single bit is set.
	assert(header != 0);
	if (headers_loaded & header) {
		// Header is already loaded.
		return 0;
	}

	if (!this->file) {
		// File isn't open.
		return -EBADF;
	} else if (!this->isValid || this->stfsType < 0) {
		// STFS file isn't valid.
		return -EIO;
	}

	// STFS_PRESENT_HEADER must have been loaded in the constructor.
	assert(header != STFS_PRESENT_HEADER);

	size_t size = 0;
	size_t size_expected;
	switch (header) {
		case STFS_PRESENT_METADATA:
			size_expected = sizeof(stfsMetadata);
			size = file->seekAndRead(STFS_METADATA_ADDRESS, &stfsMetadata, sizeof(stfsMetadata));
			break;
		case STFS_PRESENT_THUMBNAILS:
			size_expected = sizeof(stfsThumbnails);
			size = file->seekAndRead(STFS_THUMBNAILS_ADDRESS, &stfsThumbnails, sizeof(stfsThumbnails));
			break;
		default:
			assert(!"Invalid header value.");
			return -EINVAL;
	}

	int ret = 0;
	if (size != size_expected) {
		// Read error.
		ret = -file->lastError();
		if (ret == 0) {
			ret = -EIO;
		}
	}

	return ret;
}

/** Xbox360_STFS **/

/**
 * Read an Xbox 360 STFS file.
 *
 * A ROM image must be opened by the caller. The file handle
 * will be ref()'d and must be kept open in order to load
 * data from the disc image.
 *
 * To close the file, either delete this object or call close().
 *
 * NOTE: Check isValid() to determine if this is a valid ROM.
 *
 * @param file Open STFS file.
 */
Xbox360_STFS::Xbox360_STFS(IRpFile *file)
	: super(new Xbox360_STFS_Private(this, file))
{
	// This class handles application packages.
	// TODO: Change to Save File if the content is a save file.
	RP_D(Xbox360_STFS);
	d->className = "Xbox360_STFS";
	d->mimeType = "application/x-xbox360-stfs";	// unofficial, not on fd.o
	d->fileType = FTYPE_APPLICATION_PACKAGE;

	if (!d->file) {
		// Could not ref() the file handle.
		return;
	}

	// Read the STFS header.
	d->file->rewind();
	size_t size = d->file->read(&d->stfsHeader, sizeof(d->stfsHeader));
	if (size != sizeof(d->stfsHeader)) {
		// Read error.
		d->file->unref();
		d->file = nullptr;
		return;
	}

	// Check if this file is supported.
	DetectInfo info;
	info.header.addr = 0;
	info.header.size = sizeof(d->stfsHeader);
	info.header.pData = reinterpret_cast<const uint8_t*>(&d->stfsHeader);
	info.ext = nullptr;	// Not needed for STFS.
	info.szFile = 0;	// Not needed for STFS.
	d->stfsType = isRomSupported_static(&info);
	d->isValid = (d->stfsType >= 0);

	if (!d->isValid) {
		d->file->unref();
		d->file = nullptr;
	}

	// Package metadata and thumbnails are loaded on demand.
}

/** ROM detection functions. **/

/**
 * Is a ROM image supported by this class?
 * @param info DetectInfo containing ROM detection information.
 * @return Class-specific system ID (>= 0) if supported; -1 if not.
 */
int Xbox360_STFS::isRomSupported_static(const DetectInfo *info)
{
	assert(info != nullptr);
	assert(info->header.pData != nullptr);
	assert(info->header.addr == 0);
	if (!info || !info->header.pData ||
	    info->header.addr != 0 ||
	    info->header.size < sizeof(STFS_Package_Header))
	{
		// Either no detection information was specified,
		// or the header is too small.
		return Xbox360_STFS_Private::STFS_TYPE_UNKNOWN;
	}

	Xbox360_STFS_Private::StfsType stfsType = Xbox360_STFS_Private::STFS_TYPE_UNKNOWN;

	// Check for STFS.
	const STFS_Package_Header *const stfsHeader =
		reinterpret_cast<const STFS_Package_Header*>(info->header.pData);
	if (stfsHeader->magic == cpu_to_be32(STFS_MAGIC_CON)) {
		// We have a console-signed STFS package.
		stfsType = Xbox360_STFS_Private::STFS_TYPE_CON;
	} else if (stfsHeader->magic == cpu_to_be32(STFS_MAGIC_PIRS)) {
		// We have an MS-signed STFS package. (non-Xbox Live)
		stfsType = Xbox360_STFS_Private::STFS_TYPE_PIRS;
	} else if (stfsHeader->magic == cpu_to_be32(STFS_MAGIC_LIVE)) {
		// We have an MS-signed STFS package. (Xbox Live)
		stfsType = Xbox360_STFS_Private::STFS_TYPE_LIVE;
	}

	if (stfsType == Xbox360_STFS_Private::STFS_TYPE_UNKNOWN) {
		// Not supported.
		return stfsType;
	}

	// Check certain fields to prevent conflicts with the
	// Nintendo DS ROM image "Live On Card Live-R DS".
	switch (stfsType) {
		default:
			assert(!"Invalid STFS type...");
			return Xbox360_STFS_Private::STFS_TYPE_UNKNOWN;

		case Xbox360_STFS_Private::STFS_TYPE_CON:
			// Console-signed.
			// Check a few things.

			// Console type: 1 == debug, 2 == retail
			// On Nintendo DS, this is the "autostart" flag. (0 == no, 2 == autostart)
			// This will very rarely conflict.
			switch (stfsHeader->console.console_type) {
				case 1:
				case 2:
					break;
				default:
					// Invalid value.
					return Xbox360_STFS_Private::STFS_TYPE_UNKNOWN;
			}

			// Datestamp field format: "MM-DD-YY" (assuming 20xx for year)
			// On Nintendo DS, this field is the ARM9 ROM offset and entry address.
			// TODO: Check the numeric values. Only checking dashes for now.
			if (stfsHeader->console.datestamp[2] != '-' ||
			    stfsHeader->console.datestamp[5] != '-')
			{
				// Not dashes. This isn't an Xbox 360 package.
				return Xbox360_STFS_Private::STFS_TYPE_UNKNOWN;
			}
			break;

		case Xbox360_STFS_Private::STFS_TYPE_PIRS:
		case Xbox360_STFS_Private::STFS_TYPE_LIVE: {
			// MS-signed package.
			// Make sure the padding is empty.
			// This area overlaps the Nintendo DS logo section,
			// which is *never* empty.
			// TODO: Vectorize this?
			const uint8_t *pPadding = stfsHeader->ms.padding;
			const uint8_t *const pPadding_end = pPadding + sizeof(stfsHeader->ms.padding);
			for (; pPadding < pPadding_end; pPadding++) {
				if (*pPadding != 0) {
					// Not empty.
					// This is not padding.
					return Xbox360_STFS_Private::STFS_TYPE_UNKNOWN;
				}
			}
			break;
		}
	}

	return stfsType;
}

/**
 * Get the name of the system the loaded ROM is designed for.
 * @param type System name type. (See the SystemName enum.)
 * @return System name, or nullptr if type is invalid.
 */
const char *Xbox360_STFS::systemName(unsigned int type) const
{
	RP_D(const Xbox360_STFS);
	if (!d->isValid || !isSystemNameTypeValid(type))
		return nullptr;

	// Xbox 360 has the same name worldwide, so we can
	// ignore the region selection.
	static_assert(SYSNAME_TYPE_MASK == 3,
		"Xbox360_STFS::systemName() array index optimization needs to be updated.");

	// Bits 0-1: Type. (long, short, abbreviation)
	// TODO: STFS-specific, or just use Xbox 360?
	static const char *const sysNames[4] = {
		"Microsoft Xbox 360", "Xbox 360", "X360", nullptr
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
const char *const *Xbox360_STFS::supportedFileExtensions_static(void)
{
	static const char *const exts[] = {
		//".stfs",	// FIXME: Not actually used...

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
const char *const *Xbox360_STFS::supportedMimeTypes_static(void)
{
	static const char *const mimeTypes[] = {
		// Unofficial MIME types.
		// TODO: Get these upstreamed on FreeDesktop.org.
		"application/x-xbox360-stfs",

		nullptr
	};
	return mimeTypes;
}

/**
 * Get a bitfield of image types this class can retrieve.
 * @return Bitfield of supported image types. (ImageTypesBF)
 */
uint32_t Xbox360_STFS::supportedImageTypes_static(void)
{
	return IMGBF_INT_ICON;
}

/**
 * Get a list of all available image sizes for the specified image type.
 * @param imageType Image type.
 * @return Vector of available image sizes, or empty vector if no images are available.
 */
vector<RomData::ImageSizeDef> Xbox360_STFS::supportedImageSizes(ImageType imageType) const
{
	ASSERT_supportedImageSizes(imageType);

	RP_D(const Xbox360_STFS);
	if (!d->isValid || imageType != IMG_INT_MEDIA) {
		// Only IMG_INT_MEDIA is supported.
		return vector<ImageSizeDef>();
	}

	// TODO: Actually check the title thumbnail.
	// Assuming 64x64 for now.
	static const ImageSizeDef sz_INT_ICON[] = {
		{nullptr, 64, 64, 0},
	};
	return vector<ImageSizeDef>(sz_INT_ICON,
		sz_INT_ICON + ARRAY_SIZE(sz_INT_ICON));
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
vector<RomData::ImageSizeDef> Xbox360_STFS::supportedImageSizes_static(ImageType imageType)
{
	ASSERT_supportedImageSizes(imageType);

	if (imageType != IMG_INT_ICON) {
		// Only icons are supported.
		return vector<ImageSizeDef>();
	}

	// NOTE: Assuming the title thumbnail is 64x64.
	static const ImageSizeDef sz_INT_ICON[] = {
		{nullptr, 64, 64, 0},
	};
	return vector<ImageSizeDef>(sz_INT_ICON,
		sz_INT_ICON + ARRAY_SIZE(sz_INT_ICON));
}

/**
 * Load field data.
 * Called by RomData::fields() if the field data hasn't been loaded yet.
 * @return Number of fields read on success; negative POSIX error code on error.
 */
int Xbox360_STFS::loadFieldData(void)
{
	RP_D(Xbox360_STFS);
	if (!d->fields->empty()) {
		// Field data *has* been loaded...
		return 0;
	} else if (!d->file || !d->file->isOpen()) {
		// File isn't open.
		return -EBADF;
	} else if (!d->isValid || d->stfsType < 0) {
		// STFS file isn't valid.
		return -EIO;
	}

	// Make sure the STFS metadata is loaded.
	int ret = d->loadHeader(Xbox360_STFS_Private::STFS_PRESENT_METADATA);
	if (ret != 0) {
		// Not loaded and unable to load.
		return ret;
	}

	// Parse the STFS file.
	// NOTE: The STFS headers are **NOT** byteswapped.
	const STFS_Package_Header *const stfsHeader = &d->stfsHeader;
	const STFS_Package_Metadata *const stfsMetadata = &d->stfsMetadata;

	// Maximum of 14 fields.
	// - 10: Normal
	// -  3: Console-specific
	d->fields->reserve(13);
	d->fields->setTabName(0, "STFS");

	// Title fields.
	// Includes display name and description.

	// Check if English is valid.
	// If it is, we'll de-duplicate the fields.
	bool dedupe_titles = (stfsMetadata->display_name[0][0] != 0);

	// NOTE: The main section in the metadata has 18 languages.
	// Metadata version 2 adds an additional 6 languages, but we only
	// have up to 12 languages defined...

	// RFT_STRING_MULTI maps.
	RomFields::StringMultiMap_t *const pMap_name = new RomFields::StringMultiMap_t();
	RomFields::StringMultiMap_t *const pMap_desc = new RomFields::StringMultiMap_t();
	static_assert(XDBF_LANGUAGE_MAX-1 <= 18, "Too many languages for metadata v0!");
	for (int langID = XDBF_LANGUAGE_ENGLISH; langID < XDBF_LANGUAGE_MAX; langID++) {
		const int langID_off = langID - XDBF_LANGUAGE_ENGLISH;

		// Check for empty strings first.
		if (stfsMetadata->display_name[langID_off][0] == 0 &&
		    stfsMetadata->display_description[langID_off][0] == 0)
		{
			// Strings are empty.
			continue;
		}

		if (dedupe_titles && langID != XDBF_LANGUAGE_ENGLISH) {
			// Check if the title matches English.
			// NOTE: Not converting to host-endian first, since
			// u16_strncmp() checks for equality and for 0.
			if (!u16_strncmp(stfsMetadata->display_name[langID_off],
			                 stfsMetadata->display_name[XDBF_LANGUAGE_ENGLISH],
					 ARRAY_SIZE(stfsMetadata->display_name[langID_off])) &&
			    !u16_strncmp(stfsMetadata->display_description[langID_off],
			                 stfsMetadata->display_description[XDBF_LANGUAGE_ENGLISH],
					 ARRAY_SIZE(stfsMetadata->display_description[langID_off])))
			{
				// Both fields match English.
				continue;
			}
		}

		const uint32_t lc = XboxLanguage::getXbox360LanguageCode(langID);
		assert(lc != 0);
		if (lc == 0)
			continue;

		// Display name
		if (stfsMetadata->display_name[langID_off][0] != 0) {
			pMap_name->insert(std::make_pair(lc,
				utf16be_to_utf8(stfsMetadata->display_name[langID_off],
					ARRAY_SIZE(stfsMetadata->display_name[langID_off]))));
		}

		// Description
		if (stfsMetadata->display_description[langID_off][0] != 0) {
			pMap_name->insert(std::make_pair(lc,
				utf16be_to_utf8(stfsMetadata->display_description[langID_off],
					ARRAY_SIZE(stfsMetadata->display_description[langID_off]))));
		}
	}

	const uint32_t def_lc = d->getDefaultLC();
	const char *const s_name_title = C_("RomData", "Name");
	if (!pMap_name->empty()) {
		d->fields->addField_string_multi(s_name_title, pMap_name, def_lc);
	} else {
		delete pMap_name;
		d->fields->addField_string(s_name_title, C_("RomData", "Unknown"));
	}
	if (!pMap_desc->empty()) {
		d->fields->addField_string_multi(C_("RomData", "Description"), pMap_desc, def_lc);
	} else {
		delete pMap_desc;
	}

	// Publisher
	if (stfsMetadata->publisher_name[0] != 0) {
		d->fields->addField_string(C_("RomData", "Publisher"),
			utf16be_to_utf8(stfsMetadata->publisher_name,
				ARRAY_SIZE(stfsMetadata->publisher_name)));
	}

	// Title
	if (stfsMetadata->title_name[0] != 0) {
		d->fields->addField_string(C_("RomData", "Title"),
			utf16be_to_utf8(stfsMetadata->title_name,
				ARRAY_SIZE(stfsMetadata->title_name)));
	}

	// File type
	// TODO: Show console-specific information for 'CON '.
	static const char *const file_type_tbl[] = {
		NOP_C_("Xbox360_STFS|FileType", "Console-Specific Package"),
		NOP_C_("Xbox360_STFS|FileType", "Non-Xbox Live Package"),
		NOP_C_("Xbox360_STFS|FileType", "Xbox Live Package"),
	};
	if (d->stfsType > Xbox360_STFS_Private::STFS_TYPE_UNKNOWN &&
	    d->stfsType < Xbox360_STFS_Private::STFS_TYPE_MAX)
	{
		d->fields->addField_string(C_("Xbox360_STFS", "Package Type"),
			dpgettext_expr(RP_I18N_DOMAIN, "Xbox360_STFS|FileType",
				file_type_tbl[d->stfsType]));
	} else {
		d->fields->addField_string(C_("Xbox360_STFS|RomData", "Type"),
			C_("RomData", "Unknown"));
	}

	// Content type
	const char *const s_content_type = Xbox360_STFS_ContentType::lookup(
		be32_to_cpu(stfsMetadata->content_type));
	if (s_content_type) {
		d->fields->addField_string(C_("Xbox360_STFS", "Content Type"), s_content_type);
	} else {
		d->fields->addField_string(C_("Xbox360_STFS", "Content Type"),
			rp_sprintf(C_("RomData", "Unknown (0x%08X)"),
				be32_to_cpu(stfsMetadata->content_type)));
	}

	// Media ID
	d->fields->addField_string(C_("Xbox360_STFS", "Media ID"),
		rp_sprintf("%08X", be32_to_cpu(stfsMetadata->media_id)),
		RomFields::STRF_MONOSPACE);

	// Title ID
	// FIXME: Verify behavior on big-endian.
	// TODO: Consolidate implementations into a shared function.
	string tid_str;
	char hexbuf[4];
	if (stfsMetadata->title_id.a >= 0x20) {
		tid_str += (char)stfsMetadata->title_id.a;
	} else {
		tid_str += "\\x";
		snprintf(hexbuf, sizeof(hexbuf), "%02X",
			(uint8_t)stfsMetadata->title_id.a);
		tid_str.append(hexbuf, 2);
	}
	if (stfsMetadata->title_id.b >= 0x20) {
		tid_str += (char)stfsMetadata->title_id.b;
	} else {
		tid_str += "\\x";
		snprintf(hexbuf, sizeof(hexbuf), "%02X",
			(uint8_t)stfsMetadata->title_id.b);
		tid_str.append(hexbuf, 2);
	}

	d->fields->addField_string(C_("Xbox360_XEX", "Title ID"),
		rp_sprintf_p(C_("Xbox360_XEX", "%1$08X (%2$s-%3$04u)"),
			be32_to_cpu(stfsMetadata->title_id.u32),
			tid_str.c_str(),
			be16_to_cpu(stfsMetadata->title_id.u16)),
		RomFields::STRF_MONOSPACE);

	// Version and base version
	// TODO: What indicates the update version?
	Xbox360_Version_t ver, basever;
	ver.u32 = be32_to_cpu(stfsMetadata->version.u32);
	basever.u32 = be32_to_cpu(stfsMetadata->base_version.u32);
	d->fields->addField_string(C_("Xbox360_XEX", "Version"),
		rp_sprintf("%u.%u.%u.%u",
			ver.major, ver.minor, ver.build, ver.qfe));
	d->fields->addField_string(C_("Xbox360_XEX", "Base Version"),
		rp_sprintf("%u.%u.%u.%u",
			basever.major, basever.minor, basever.build, basever.qfe));

	// Console-specific packages.
	if (stfsHeader->magic == cpu_to_be32(STFS_MAGIC_CON)) {
		// NOTE: addField_string_numeric() is limited to 32-bit.
		// Print the console ID as a hexdump instead.
		d->fields->addField_string_hexdump(C_("Xbox360_XEX", "Console ID"),
			stfsHeader->console.console_id,
			sizeof(stfsHeader->console.console_id),
			RomFields::STRF_MONOSPACE | RomFields::STRF_HEXDUMP_NO_SPACES);

		// Part number.
		// Not entirely sure what this is referring to...
		d->fields->addField_string(C_("Xbox360_XEX", "Part Number"),
			latin1_to_utf8(stfsHeader->console.part_number,
				sizeof(stfsHeader->console.part_number)));

		// Console type.
		const char *s_console_type;
		switch (stfsHeader->console.console_type) {
			case STFS_CONSOLE_TYPE_DEBUG:
				s_console_type = C_("Xbox360_XEX|ConsoleType", "Debug");
				break;
			case STFS_CONSOLE_TYPE_RETAIL:
				s_console_type = C_("Xbox360_XEX|ConsoleType", "Retail");
				break;
			default:
				s_console_type = nullptr;
				break;
		}
		const char *const s_console_type_title = C_("Xbox360_XEX", "Console Type");
		if (s_console_type) {
			d->fields->addField_string(s_console_type_title, s_console_type);
		} else {
			d->fields->addField_string(s_console_type_title,
				rp_sprintf(C_("RomData", "Unknown (%u)"), stfsHeader->console.console_type));
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
int Xbox360_STFS::loadMetaData(void)
{
	RP_D(Xbox360_STFS);
	if (d->metaData != nullptr) {
		// Metadata *has* been loaded...
		return 0;
	} else if (!d->file) {
		// File isn't open.
		return -EBADF;
	} else if (!d->isValid || d->stfsType < 0) {
		// STFS file isn't valid.
		return -EIO;
	}

	// Make sure the STFS metadata is loaded.
	int ret = d->loadHeader(Xbox360_STFS_Private::STFS_PRESENT_METADATA);
	if (ret != 0) {
		// Not loaded and unable to load.
		return ret;
	}

	// Create the metadata object.
	d->metaData = new RomMetaData();
	d->metaData->reserve(2);	// Maximum of 2 metadata properties.

	const STFS_Package_Metadata *const stfsMetadata = &d->stfsMetadata;

	// Display name and/or title
	// TODO: Which one to prefer?
	// TODO: Language ID?
	if (stfsMetadata->display_name[0][0] != 0) {
		d->metaData->addMetaData_string(Property::Title,
			utf16be_to_utf8(stfsMetadata->display_name[0],
				ARRAY_SIZE(stfsMetadata->display_name[0])));
	} else if (stfsMetadata->title_name[0] != 0) {
		d->metaData->addMetaData_string(Property::Title,
			utf16be_to_utf8(stfsMetadata->title_name,
				ARRAY_SIZE(stfsMetadata->title_name)));
	}		

	// Publisher
	if (stfsMetadata->publisher_name[0] != 0) {
		d->metaData->addMetaData_string(Property::Publisher,
			utf16be_to_utf8(stfsMetadata->publisher_name,
				ARRAY_SIZE(stfsMetadata->publisher_name)));
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
int Xbox360_STFS::loadInternalImage(ImageType imageType, const rp_image **pImage)
{
	ASSERT_loadInternalImage(imageType, pImage);
	RP_D(Xbox360_STFS);
	ROMDATA_loadInternalImage_single(
		IMG_INT_ICON,	// ourImageType
		d->file,	// file
		d->isValid,	// isValid
		d->stfsType,	// romType
		d->img_icon,	// imgCache
		d->loadIcon);	// func
}

}
