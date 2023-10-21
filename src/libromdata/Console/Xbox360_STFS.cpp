/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * Xbox360_STFS.cpp: Microsoft Xbox 360 package reader.                    *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"

#include "Xbox360_STFS.hpp"
#include "Xbox360_XEX.hpp"
#include "xbox360_stfs_structs.h"
#include "data/Xbox360_STFS_ContentType.hpp"

// for language codes
#include "xbox360_xdbf_structs.h"
#include "data/XboxLanguage.hpp"

// Other rom-properties libraries
#include "librpbase/img/RpPng.hpp"
#include "librpfile/MemFile.hpp"
using namespace LibRpBase;
using namespace LibRpFile;
using namespace LibRpText;
using namespace LibRpTexture;

// C++ STL classes
using std::shared_ptr;
using std::string;
using std::vector;

namespace LibRomData {

// Workaround for RP_D() expecting the no-underscore naming convention.
#define Xbox360_STFSPrivate Xbox360_STFS_Private

class Xbox360_STFS_Private final : public RomDataPrivate
{
	public:
		Xbox360_STFS_Private(const IRpFilePtr &file);
		~Xbox360_STFS_Private() final;

	private:
		typedef RomDataPrivate super;
		RP_DISABLE_COPY(Xbox360_STFS_Private)

	public:
		/** RomDataInfo **/
		static const char *const exts[];
		static const char *const mimeTypes[];
		static const RomDataInfo romDataInfo;

	public:
		// STFS type
		enum class StfsType {
			Unknown	= -1,

			CON	= 0,	// Console-signed
			PIRS	= 1,	// MS-signed for non-Xbox Live
			LIVE	= 2,	// MS-signed for Xbox Live

			Max
		};
		StfsType stfsType;

		// Icon
		// NOTE: Currently using Title Thumbnail.
		// Should we make regular Thumbnail available too?
		rp_image_ptr img_icon;

		/**
		 * Load the icon.
		 * @return Icon, or nullptr on error.
		 */
		rp_image_const_ptr loadIcon(void);

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

	public:
		// XEX executable.
		Xbox360_XEX *xex;

		// File table.
		rp::uvector<STFS_DirEntry_t> fileTable;

		/**
		 * Convert a block number to an offset.
		 * @param blockNumber Block number.
		 * @return Offset, or -1 on error.
		 */
		inline int32_t blockNumberToOffset(int blockNumber)
		{
			// Reference: https://github.com/Free60Project/wiki/blob/master/STFS.md
			int ret;
			if (blockNumber > 0xFFFFFF) {
				ret = -1;
			} else {
				ret = (((be32_to_cpu(stfsMetadata.header_size) + 0xFFF) & 0xF000) +
				        (blockNumber * STFS_BLOCK_SIZE));
			}
			return ret;
		}

		/**
		 * Convert a data block number to a physical block number.
		 * Data block numbers don't include hash blocks.
		 * @param dataBlockNumber Data block number.
		 * @return physBlockNumber Physical block number.
		 */
		int32_t dataBlockNumberToPhys(int dataBlockNumber);

		/**
		 * Load the file table.
		 * @return 0 on success; negative POSIX error code on error.
		 */
		int loadFileTable(void);

		/**
		 * Open the default executable.
		 * @return Default executable on success; nullptr on error.
		 */
		Xbox360_XEX *openDefaultXex(void);
};

ROMDATA_IMPL(Xbox360_STFS)
ROMDATA_IMPL_IMG_TYPES(Xbox360_STFS)

/** Xbox360_STFS_Private **/

/* RomDataInfo */
const char *const Xbox360_STFS_Private::exts[] = {
	//".stfs",	// FIXME: Not actually used...
	".fxs",		// Fallout
	".exs",		// Skyrim

	nullptr
};
const char *const Xbox360_STFS_Private::mimeTypes[] = {
	// Unofficial MIME types.
	// TODO: Get these upstreamed on FreeDesktop.org.
	"application/x-xbox360-stfs",

	nullptr
};
const RomDataInfo Xbox360_STFS_Private::romDataInfo = {
	"Xbox360_STFS", exts, mimeTypes
};

Xbox360_STFS_Private::Xbox360_STFS_Private(const IRpFilePtr &file)
	: super(file, &romDataInfo)
	, stfsType(StfsType::Unknown)
	, headers_loaded(0)
	, xex(nullptr)
{
	// Clear the headers.
	memset(&stfsHeader, 0, sizeof(stfsHeader));
	memset(&stfsMetadata, 0, sizeof(stfsMetadata));
	memset(&stfsThumbnails, 0, sizeof(stfsThumbnails));
}

Xbox360_STFS_Private::~Xbox360_STFS_Private()
{
	delete xex;
}

/**
 * Load the icon.
 * @return Icon, or nullptr on error.
 */
rp_image_const_ptr Xbox360_STFS_Private::loadIcon(void)
{
	if (img_icon) {
		// Icon has already been loaded.
		return img_icon;
	} else if (!this->isValid || (int)this->stfsType < 0) {
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

	// Create a MemFile and decode the image.
	// TODO: For rpcli, shortcut to extract the PNG directly.
	shared_ptr<MemFile> f_mem = std::make_shared<MemFile>(pIconData, iconSize);
	rp_image_ptr img = RpPng::load(f_mem);

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

		f_mem = std::make_shared<MemFile>(pIconData, iconSize);
		img = RpPng::load(f_mem);
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
	} else if (!this->isValid || (int)this->stfsType < 0) {
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

/**
 * Convert a data block number to a physical block number.
 * Data block numbers don't include hash blocks.
 * @param dataBlockNumber Data block number.
 * @return physBlockNumber Physical block number.
 */
int32_t Xbox360_STFS_Private::dataBlockNumberToPhys(int dataBlockNumber)
{
	// Reference: https://github.com/Free60Project/wiki/blob/master/STFS.md
	int blockShift;
	if (((be32_to_cpu(stfsMetadata.header_size) + 0xFFF) & 0xF000) == 0xB000) {
		blockShift = 1;
	} else {
		if ((stfsMetadata.stfs_desc.block_separation & 1) == 1) {
			blockShift = 0;
		} else {
			blockShift = 1;
		}
	}

	int32_t base = ((dataBlockNumber + 0xAA) / 0xAA);
	if (stfsType == StfsType::CON) {
		base <<= blockShift;
	}

	int32_t ret = (base + dataBlockNumber);
	if (dataBlockNumber > 0xAA) {
		base = ((dataBlockNumber + 0x70E4) / 0x70E4);
		if (stfsType == StfsType::CON) {
			base <<= blockShift;
		}
		ret += base;

		if (dataBlockNumber > 0x70E4) {
			base = ((dataBlockNumber + 0x4AF768) / 0x4AF768);
			// FIXME: Originally compared magic to blockShift,
			// which doesn't make sense...
			if (stfsType == StfsType::CON) {
				base <<= blockShift;
			}
			ret += base;
		}
	}

	return ret;
}

/**
 * Load the file table.
 * @return 0 on success; negative POSIX error code on error.
 */
int Xbox360_STFS_Private::loadFileTable(void)
{
	if (!fileTable.empty()) {
		// File table is already loaded.
		return 0;
	}

	if (!this->file) {
		// File isn't open.
		return -EBADF;
	} else if (!this->isValid || (int)this->stfsType < 0) {
		// STFS file isn't valid.
		return -EIO;
	}

	// Make sure the STFS metadata is loaded.
	int ret = loadHeader(Xbox360_STFS_Private::STFS_PRESENT_METADATA);
	if (ret != 0) {
		// Not loaded and unable to load.
		return ret;
	}

	// TODO: Verify that this is STFS and not SVOD.
	// NOTE: These values are signed. Make sure they're not negative.
	const int16_t blockCount = be16_to_cpu(stfsMetadata.stfs_desc.file_table_block_count);
	if (blockCount < 0 || stfsMetadata.stfs_desc.file_table_block_number[0] >= 0x80) {
		// Negative values.
		return -EIO;
	}
	const int32_t blockNumber =
		(stfsMetadata.stfs_desc.file_table_block_number[0] << 16) |
		(stfsMetadata.stfs_desc.file_table_block_number[1] <<  8) |
		 stfsMetadata.stfs_desc.file_table_block_number[2];
	const int32_t offset = blockNumberToOffset(dataBlockNumberToPhys(blockNumber));
	if (offset < 0) {
		// Invalid block number.
		return -EIO;
	}

	// Load the file table.
	size_t fileTableSize = ((uint32_t)blockCount * STFS_BLOCK_SIZE);
	assert(fileTableSize % sizeof(STFS_DirEntry_t) == 0);
	if (fileTableSize % sizeof(STFS_DirEntry_t) != 0) {
		fileTableSize += sizeof(STFS_DirEntry_t);
	}
	fileTable.resize(fileTableSize / sizeof(STFS_DirEntry_t));
	size_t size = file->seekAndRead(offset, fileTable.data(), fileTableSize);
	if (size != fileTableSize) {
		// Seek and/or read error.
		fileTable.clear();
		return -EIO;
	}

	// Find the end of the file table.
	for (size_t i = 0; i < fileTable.size(); i++) {
		if (fileTable[i].filename[0] == '\0') {
			// NULL filename. End of table.
			fileTable.resize(i);
			break;
		}
	}

	return (!fileTable.empty() ? 0 : -ENOENT);
}

/**
 * Open the default executable.
 * @return Default executable on success; nullptr on error.
 */
Xbox360_XEX *Xbox360_STFS_Private::openDefaultXex(void)
{
	if (this->xex) {
		return this->xex;
	}

	// Make sure the file table is loaded.
	int ret = loadFileTable();
	if (ret != 0) {
		// Unable to load the file table.
		return nullptr;
	}

	// Find default.xex or default.xexp and load it.
	// TODO: Handle subdirectories?
	const STFS_DirEntry_t *dirEntry = nullptr;
	for (const STFS_DirEntry_t &p : fileTable) {
		// Make sure this isn't a subdirectory.
		if (p.flags_len & 0x80) {
			// It's a subdirectory.
			continue;
		}

		// "default.xex" is 11 characters.
		// "default.xexp" is 12 characters.
		switch (p.flags_len & 0x3F) {
			case 11:
				// Check for default.xex.
				if (!strncasecmp("default.xex", p.filename, 12)) {
					// Found default.xex.
					dirEntry = &p;
					break;
				}
				break;

			case 12:
				// Check for default.xexp.
				if (!strncasecmp("default.xexp", p.filename, 13)) {
					// Found default.xexp.
					dirEntry = &p;
					break;
				}
				break;

			default:
				break;
		}
	}

	if (!dirEntry) {
		// Directory entry not found.
		return nullptr;
	}

	// Offset and filesize.
	// NOTE: Block number is **little-endian** here.
	const int32_t blockNumber =
		(dirEntry->block_number[2] << 16) |
		(dirEntry->block_number[1] <<  8) |
		 dirEntry->block_number[0];
	const int32_t offset = blockNumberToOffset(dataBlockNumberToPhys(blockNumber));
	const uint32_t filesize = be32_to_cpu(dirEntry->filesize);

	// Load default.xexp.
	// FIXME: Maybe add a reader class to handle the hashes,
	// though we only need the XEX header right now.
	shared_ptr<SubFile> xexFile_tmp = std::make_shared<SubFile>(this->file, offset, filesize);
	if (xexFile_tmp->isOpen()) {
		Xbox360_XEX *const xex_tmp = new Xbox360_XEX(xexFile_tmp);
		if (xex_tmp->isOpen()) {
			this->xex = xex_tmp;
		} else {
			delete xex_tmp;
		}
	}

	return this->xex;
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
Xbox360_STFS::Xbox360_STFS(const IRpFilePtr &file)
	: super(new Xbox360_STFS_Private(file))
{
	// This class handles application packages.
	// TODO: Change to Save File if the content is a save file.
	RP_D(Xbox360_STFS);
	d->mimeType = "application/x-xbox360-stfs";	// unofficial, not on fd.o
	d->fileType = FileType::ApplicationPackage;

	if (!d->file) {
		// Could not ref() the file handle.
		return;
	}

	// Read the STFS header.
	d->file->rewind();
	size_t size = d->file->read(&d->stfsHeader, sizeof(d->stfsHeader));
	if (size != sizeof(d->stfsHeader)) {
		// Read error.
		d->file.reset();
		return;
	}

	// Check if this file is supported.
	const DetectInfo info = {
		{0, sizeof(d->stfsHeader), reinterpret_cast<const uint8_t*>(&d->stfsHeader)},
		nullptr,	// ext (not needed for Xbox360_STFS)
		0		// szFile (not needed for Xbox360_STFS)
	};
	d->stfsType = static_cast<Xbox360_STFS_Private::StfsType>(isRomSupported_static(&info));
	d->isValid = ((int)d->stfsType >= 0);

	if (!d->isValid) {
		d->file.reset();
	}

	// Package metadata and thumbnails are loaded on demand.
}

/**
 * Close the opened file.
 */
void Xbox360_STFS::close(void)
{
	RP_D(Xbox360_STFS);

	// NOTE: Don't delete these. They have rp_image objects
	// that may be used by the UI later.
	if (d->xex) {
		d->xex->close();
	}

	// Call the superclass function.
	super::close();
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
		return static_cast<int>(Xbox360_STFS_Private::StfsType::Unknown);
	}

	Xbox360_STFS_Private::StfsType stfsType = Xbox360_STFS_Private::StfsType::Unknown;

	// Check for STFS.
	const STFS_Package_Header *const stfsHeader =
		reinterpret_cast<const STFS_Package_Header*>(info->header.pData);
	if (stfsHeader->magic == cpu_to_be32(STFS_MAGIC_CON)) {
		// We have a console-signed STFS package.
		stfsType = Xbox360_STFS_Private::StfsType::CON;
	} else if (stfsHeader->magic == cpu_to_be32(STFS_MAGIC_PIRS)) {
		// We have an MS-signed STFS package. (non-Xbox Live)
		stfsType = Xbox360_STFS_Private::StfsType::PIRS;
	} else if (stfsHeader->magic == cpu_to_be32(STFS_MAGIC_LIVE)) {
		// We have an MS-signed STFS package. (Xbox Live)
		stfsType = Xbox360_STFS_Private::StfsType::LIVE;
	}

	if (stfsType == Xbox360_STFS_Private::StfsType::Unknown) {
		// Not supported.
		return static_cast<int>(stfsType);
	}

	// Check certain fields to prevent conflicts with the
	// Nintendo DS ROM image "Live On Card Live-R DS".
	switch (stfsType) {
		default:
			assert(!"Invalid STFS type...");
			stfsType = Xbox360_STFS_Private::StfsType::Unknown;
			break;

		case Xbox360_STFS_Private::StfsType::CON:
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
					stfsType = Xbox360_STFS_Private::StfsType::Unknown;
					break;
			}

			// Datestamp field format: "MM-DD-YY" (assuming 20xx for year)
			// On Nintendo DS, this field is the ARM9 ROM offset and entry address.
			// TODO: Check the numeric values. Only checking dashes for now.
			if (stfsHeader->console.datestamp[2] != '-' ||
			    stfsHeader->console.datestamp[5] != '-')
			{
				// Not dashes. This isn't an Xbox 360 package.
				stfsType = Xbox360_STFS_Private::StfsType::Unknown;
			}
			break;

		case Xbox360_STFS_Private::StfsType::PIRS:
		case Xbox360_STFS_Private::StfsType::LIVE: {
			// MS-signed package.
			// Make sure the padding is empty.
			// This area overlaps the Nintendo DS logo section,
			// which is *never* empty.
			// TODO: Vectorize this?
			const uint8_t *const pPadding_end = &stfsHeader->ms.padding[sizeof(stfsHeader->ms.padding)];
			for (const uint8_t *pPadding = stfsHeader->ms.padding; pPadding < pPadding_end; pPadding++) {
				if (*pPadding != 0) {
					// Not empty.
					// This is not padding.
					stfsType = Xbox360_STFS_Private::StfsType::Unknown;
					break;
				}
			}
			break;
		}
	}

	return static_cast<int>(stfsType);
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
		return {};
	}

	// TODO: Actually check the title thumbnail.
	// Assuming 64x64 for now.
	return {{nullptr, 64, 64, 0}};
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
		return {};
	}

	// NOTE: Assuming the title thumbnail is 64x64.
	return {{nullptr, 64, 64, 0}};
}

/**
 * Load field data.
 * Called by RomData::fields() if the field data hasn't been loaded yet.
 * @return Number of fields read on success; negative POSIX error code on error.
 */
int Xbox360_STFS::loadFieldData(void)
{
	RP_D(Xbox360_STFS);
	if (!d->fields.empty()) {
		// Field data *has* been loaded...
		return 0;
	} else if (!d->file || !d->file->isOpen()) {
		// File isn't open.
		return -EBADF;
	} else if (!d->isValid || (int)d->stfsType < 0) {
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
	d->fields.reserve(13);
	d->fields.setTabName(0, "STFS");

	// Title fields.
	// Includes display name and description.

	// Check if English is valid.
	// If it is, we'll de-duplicate the fields.
	const bool dedupe_titles = (stfsMetadata->display_name[0][0] != 0);

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
			pMap_name->emplace(lc, utf16be_to_utf8(
				stfsMetadata->display_name[langID_off],
				ARRAY_SIZE(stfsMetadata->display_name[langID_off])));
		}

		// Description
		if (stfsMetadata->display_description[langID_off][0] != 0) {
			pMap_name->emplace(lc, utf16be_to_utf8(
				stfsMetadata->display_description[langID_off],
				ARRAY_SIZE(stfsMetadata->display_description[langID_off])));
		}
	}

	const uint32_t def_lc = d->getDefaultLC();
	const char *const s_name_title = C_("RomData", "Name");
	if (!pMap_name->empty()) {
		d->fields.addField_string_multi(s_name_title, pMap_name, def_lc);
	} else {
		delete pMap_name;
		d->fields.addField_string(s_name_title, C_("RomData", "Unknown"));
	}
	if (!pMap_desc->empty()) {
		d->fields.addField_string_multi(C_("RomData", "Description"), pMap_desc, def_lc);
	} else {
		delete pMap_desc;
	}

	// Publisher
	if (stfsMetadata->publisher_name[0] != 0) {
		d->fields.addField_string(C_("RomData", "Publisher"),
			utf16be_to_utf8(stfsMetadata->publisher_name,
				ARRAY_SIZE(stfsMetadata->publisher_name)));
	}

	// Title
	if (stfsMetadata->title_name[0] != 0) {
		d->fields.addField_string(C_("RomData", "Title"),
			utf16be_to_utf8(stfsMetadata->title_name,
				ARRAY_SIZE(stfsMetadata->title_name)));
	}

	// File type
	// TODO: Show console-specific information for 'CON '.
	static const std::array<const char*, (int)Xbox360_STFS_Private::StfsType::Max> file_type_tbl = {{
		NOP_C_("Xbox360_STFS|FileType", "Console-Specific Package"),
		NOP_C_("Xbox360_STFS|FileType", "Non-Xbox Live Package"),
		NOP_C_("Xbox360_STFS|FileType", "Xbox Live Package"),
	}};
	if (d->stfsType > Xbox360_STFS_Private::StfsType::Unknown &&
	    d->stfsType < Xbox360_STFS_Private::StfsType::Max)
	{
		d->fields.addField_string(C_("Xbox360_STFS", "Package Type"),
			dpgettext_expr(RP_I18N_DOMAIN, "Xbox360_STFS|FileType",
				file_type_tbl[(int)d->stfsType]));
	} else {
		d->fields.addField_string(C_("Xbox360_STFS|RomData", "Type"),
			C_("RomData", "Unknown"));
	}

	// Content type
	const char *const s_content_type = Xbox360_STFS_ContentType::lookup(
		be32_to_cpu(stfsMetadata->content_type));
	if (s_content_type) {
		d->fields.addField_string(C_("Xbox360_STFS", "Content Type"), s_content_type);
	} else {
		d->fields.addField_string(C_("Xbox360_STFS", "Content Type"),
			rp_sprintf(C_("RomData", "Unknown (0x%08X)"),
				be32_to_cpu(stfsMetadata->content_type)));
	}

	// Media ID
	d->fields.addField_string(C_("Xbox360_STFS", "Media ID"),
		rp_sprintf("%08X", be32_to_cpu(stfsMetadata->media_id)),
		RomFields::STRF_MONOSPACE);

	// Title ID
	// FIXME: Verify behavior on big-endian.
	// TODO: Consolidate implementations into a shared function.
	string tid_str;
	char hexbuf[4];
	if (ISUPPER(stfsMetadata->title_id.a)) {
		tid_str += (char)stfsMetadata->title_id.a;
	} else {
		tid_str += "\\x";
		snprintf(hexbuf, sizeof(hexbuf), "%02X",
			(uint8_t)stfsMetadata->title_id.a);
		tid_str.append(hexbuf, 2);
	}
	if (ISUPPER(stfsMetadata->title_id.b)) {
		tid_str += (char)stfsMetadata->title_id.b;
	} else {
		tid_str += "\\x";
		snprintf(hexbuf, sizeof(hexbuf), "%02X",
			(uint8_t)stfsMetadata->title_id.b);
		tid_str.append(hexbuf, 2);
	}

	d->fields.addField_string(C_("Xbox360_XEX", "Title ID"),
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
	d->fields.addField_string(C_("Xbox360_XEX", "Version"),
		rp_sprintf("%u.%u.%u.%u",
			static_cast<unsigned int>(ver.major),
			static_cast<unsigned int>(ver.minor),
			static_cast<unsigned int>(ver.build),
			static_cast<unsigned int>(ver.qfe)));
	d->fields.addField_string(C_("Xbox360_XEX", "Base Version"),
		rp_sprintf("%u.%u.%u.%u",
			static_cast<unsigned int>(basever.major),
			static_cast<unsigned int>(basever.minor),
			static_cast<unsigned int>(basever.build),
			static_cast<unsigned int>(basever.qfe)));

	// Console-specific packages.
	if (stfsHeader->magic == cpu_to_be32(STFS_MAGIC_CON)) {
		// NOTE: addField_string_numeric() is limited to 32-bit.
		// Print the console ID as a hexdump instead.
		d->fields.addField_string_hexdump(C_("Xbox360_XEX", "Console ID"),
			stfsHeader->console.console_id,
			sizeof(stfsHeader->console.console_id),
			RomFields::STRF_MONOSPACE | RomFields::STRF_HEXDUMP_NO_SPACES);

		// Part number.
		// Not entirely sure what this is referring to...
		d->fields.addField_string(C_("Xbox360_XEX", "Part Number"),
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
			d->fields.addField_string(s_console_type_title, s_console_type);
		} else {
			d->fields.addField_string(s_console_type_title,
				rp_sprintf(C_("RomData", "Unknown (%u)"), stfsHeader->console.console_type));
		}
	}

	// Attempt to open the default executable.
	Xbox360_XEX *const default_xex = d->openDefaultXex();
	if (default_xex) {
		// Add the fields.
		const RomFields *const xexFields = default_xex->fields();
		if (xexFields) {
			d->fields.addFields_romFields(xexFields, RomFields::TabOffset_AddTabs);
		}
	}

	// Finished reading the field data.
	return static_cast<int>(d->fields.count());
}

/**
 * Load metadata properties.
 * Called by RomData::metaData() if the metadata hasn't been loaded yet.
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
	} else if (!d->isValid || (int)d->stfsType < 0) {
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
 * @param pImage	[out] Reference to rp_image_const_ptr to store the image in.
 * @return 0 on success; negative POSIX error code on error.
 */
int Xbox360_STFS::loadInternalImage(ImageType imageType, rp_image_const_ptr &pImage)
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
