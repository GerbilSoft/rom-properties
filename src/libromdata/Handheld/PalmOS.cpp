/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * PalmOS.cpp: Palm OS application reader.                                 *
 *                                                                         *
 * Copyright (c) 2016-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

// References:
// - https://en.wikipedia.org/wiki/PRC_(Palm_OS)
// - https://web.mit.edu/pilot/pilot-docs/V1.0/cookbook.pdf
// - https://web.mit.edu/Tytso/www/pilot/prc-format.html
// - https://stuff.mit.edu/afs/sipb/user/yonah/docs/Palm%20OS%20Companion.pdf
// - https://stuff.mit.edu/afs/sipb/user/yonah/docs/Palm%20OS%20Reference.pdf
// - http://www.cs.trinity.edu/~jhowland/class.files.cs3194.html/palm-docs/Constructor%20for%20Palm%20OS.pdf

#include "stdafx.h"
#include "PalmOS.hpp"
#include "palmos_structs.h"

// Other rom-properties libraries
#include "librptexture/decoder/ImageDecoder_Linear.hpp"
using namespace LibRpBase;
using namespace LibRpFile;
using namespace LibRpText;
using namespace LibRpTexture;

// C++ STL classes
using std::string;
using std::unique_ptr;
using std::vector;

// Uninitialized vector class
#include "uvector.h"

namespace LibRomData {

class PalmOSPrivate final : public RomDataPrivate
{
public:
	PalmOSPrivate(const IRpFilePtr &file);
	~PalmOSPrivate() final = default;

private:
	typedef RomDataPrivate super;
	RP_DISABLE_COPY(PalmOSPrivate)

public:
	/** RomDataInfo **/
	static const char *const exts[];
	static const char *const mimeTypes[];
	static const RomDataInfo romDataInfo;

public:
	// Application header
	PalmOS_PRC_Header_t prcHeader;

	// Resource headers
	// NOTE: Kept in big-endian format.
	rp::uvector<PalmOS_PRC_ResHeader_t> resHeaders;

	// Icon
	rp_image_ptr img_icon;

	/**
	 * Find a resource header.
	 * @param type Resource type
	 * @param id ID, or 0 for "first available"
	 * @return Pointer to resource header, or nullptr if not found.
	 */
	const PalmOS_PRC_ResHeader_t *findResHeader(uint32_t type, uint16_t id = 0) const;

	/**
	 * Load the icon.
	 * @return Icon, or nullptr on error.
	 */
	rp_image_const_ptr loadIcon(void);
};

ROMDATA_IMPL(PalmOS)

/** PalmOSPrivate **/

/* RomDataInfo */
const char *const PalmOSPrivate::exts[] = {
	".prc",

	nullptr
};
const char *const PalmOSPrivate::mimeTypes[] = {
	// Vendor-specific MIME types from FreeDesktop.org.
	"application/vnd.palm",

	// Unofficial MIME types from FreeDesktop.org.
	// TODO: Get these upstreamed on FreeDesktop.org.
	"application/x-palm-database",
	"application/x-mobipocket-ebook",	// May show up on some systems, so reference it here.

	nullptr
};
const RomDataInfo PalmOSPrivate::romDataInfo = {
	"PalmOS", exts, mimeTypes
};

PalmOSPrivate::PalmOSPrivate(const IRpFilePtr &file)
	: super(file, &romDataInfo)
{
	// Clear the PRC header struct.
	memset(&prcHeader, 0, sizeof(prcHeader));
}

/**
 * Find a resource header.
 * @param type Resource type
 * @param id ID, or 0 for "first available"
 * @return Pointer to resource header, or nullptr if not found.
 */
const PalmOS_PRC_ResHeader_t *PalmOSPrivate::findResHeader(uint32_t type, uint16_t id) const
{
	if (resHeaders.empty()) {
		// No resource headers...
		return nullptr;
	}

#if SYS_BYTEORDER != SYS_BIG_ENDIAN
	// Convert type and ID to big-endian for faster parsing.
	type = cpu_to_be32(type);
	id = cpu_to_be16(id);
#endif /* SYS_BYTEORDER != SYS_BIG_ENDIAN */

	// Find the specified resource header.
	for (const auto &hdr : resHeaders) {
		if (hdr.type == type) {
			if (id == 0 || hdr.id == id) {
				// Found a matching resource.
				return &hdr;
			}
		}
	}

	// Resource not found.
	return nullptr;
}

/**
 * Load the icon.
 * @return Icon, or nullptr on error.
 */
rp_image_const_ptr PalmOSPrivate::loadIcon(void)
{
	if (img_icon) {
		// Icon has already been loaded.
		return img_icon;
	} else if (!this->isValid || !this->file) {
		// Can't load the icon.
		return {};
	}

	// Find the application icon resource.
	// - Type: 'tAIB'
	// - Large icon: 1000 (usually 22x22; may be up to 32x32)
	// - Small icon: 1001 (15x9)
	// TODO: Allow user selection. For now, large icon only.
	const PalmOS_PRC_ResHeader_t *const iconHdr = findResHeader(PalmOS_PRC_ResType_ApplicationIcon, 1000);
	if (!iconHdr) {
		// Not found...
		return {};
	}

	// Found the application icon.
	// Read the BitmapType struct.
	uint32_t addr = be32_to_cpu(iconHdr->addr);
	// TODO: Verify the address is after the resource header section.

	PalmOS_BitmapType_t bitmapType;
	size_t size = file->seekAndRead(addr, &bitmapType, sizeof(bitmapType));
	if (size != sizeof(bitmapType)) {
		// Failed to read the BitmapType struct.
		return {};
	}

	// Only 1-bpp icons are supported right now.
	// TODO: For v1-v3, check for additional bitmaps and pick the
	// bitmap with the highest color depth. (and/or largest size?)
	// TODO: Split into a general icon loading function?
	size_t bitmapType_size;
	switch (bitmapType.version) {
		default:
			// Not supported.
			return {};

		case 0:
			// v0
			bitmapType_size = PalmOS_BitmapType_v0_SIZE;
			break;

		case 1:
			// v1
			// NOTE: Only allowing 1-bpp images for now.
			if (bitmapType.pixelSize != 1) {
				return {};
			}
			bitmapType_size = PalmOS_BitmapType_v1_SIZE;
			break;

		case 2:
			// v2
			// NOTE: Only allowing 1-bpp images for now.
			if (bitmapType.pixelSize != 1) {
				return {};
			}
			bitmapType_size = PalmOS_BitmapType_v2_SIZE;
			break;

		case 3:
			// v2
			// NOTE: Only allowing 1-bpp images for now.
			if (bitmapType.pixelSize != 1) {
				return {};
			}
			bitmapType_size = PalmOS_BitmapType_v3_SIZE;
			break;
	}

	// Parse a 1-bpp icon.
	const int width = be16_to_cpu(bitmapType.width);
	const int height = be16_to_cpu(bitmapType.height);
	assert(width > 0);
	assert(width <= 256);
	assert(height > 0);
	assert(height <= 256);
	if (width <= 0 || width > 256 ||
	    height <= 0 || height > 256)
	{
		// Icon size is probably out of range.
		return {};
	}
	const unsigned int rowBytes = be16_to_cpu(bitmapType.rowBytes);
	const size_t icon_data_len = (size_t)rowBytes * (size_t)height;

	addr += bitmapType_size;
	unique_ptr<uint8_t[]> icon_data(new uint8_t[icon_data_len]);
	size = file->seekAndRead(addr, icon_data.get(), icon_data_len);
	if (size != icon_data_len) {
		// Seek and/or read error.
		return {};
	}

	img_icon = ImageDecoder::fromLinearMono(width, height, icon_data.get(), icon_data_len, static_cast<int>(rowBytes));
	return img_icon;
}

/** PalmOS **/

/**
 * Read a Palm OS application.
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
PalmOS::PalmOS(const IRpFilePtr &file)
	: super(new PalmOSPrivate(file))
{
	// This class handles applications. (we'll indicate "Executable")
	RP_D(PalmOS);
	d->mimeType = PalmOSPrivate::mimeTypes[0];
	d->fileType = FileType::Executable;

	if (!d->file) {
		// Could not ref() the file handle.
		return;
	}

	// Read the PRC header.
	d->file->rewind();
	size_t size = d->file->read(&d->prcHeader, sizeof(d->prcHeader));
	if (size != sizeof(d->prcHeader)) {
		d->file.reset();
		return;
	}

	// Check if this application is supported.
	const DetectInfo info = {
		{0, sizeof(d->prcHeader), reinterpret_cast<const uint8_t*>(&d->prcHeader)},
		nullptr,	// ext (not needed for PalmOS)
		0		// szFile (not needed for PalmOS)
	};
	d->isValid = (isRomSupported_static(&info) >= 0);

	if (!d->isValid) {
		d->file.reset();
		return;
	}

	// Load the resource headers.
	const unsigned int num_records = be16_to_cpu(d->prcHeader.num_records);
	const size_t res_size = num_records * sizeof(PalmOS_PRC_ResHeader_t);
	d->resHeaders.resize(num_records);
	size = d->file->seekAndRead(sizeof(d->prcHeader), d->resHeaders.data(), res_size);
	if (size != res_size) {
		// Seek and/or read error.
		d->resHeaders.clear();
		d->file.reset();
		d->isValid = false;
	}
}

/**
 * Is a ROM image supported by this class?
 * @param info DetectInfo containing ROM detection information.
 * @return Class-specific system ID (>= 0) if supported; -1 if not.
 */
int PalmOS::isRomSupported_static(const DetectInfo *info)
{
	assert(info != nullptr);
	assert(info->header.pData != nullptr);
	assert(info->header.addr == 0);
	if (!info || !info->header.pData ||
	    info->header.addr != 0 ||
	    info->header.size < sizeof(PalmOS_PRC_Header_t))
	{
		// Either no detection information was specified,
		// or the header is too small.
		return -1;
	}

	// TODO: Check for some more fields.
	// For now, only checking that the type field is "appl".
	const PalmOS_PRC_Header_t *const prcHeader =
		reinterpret_cast<const PalmOS_PRC_Header_t*>(info->header.pData);
	if (prcHeader->type == cpu_to_be32(PalmOS_PRC_FileType_Application)) {
		// It's an application.
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
const char *PalmOS::systemName(unsigned int type) const
{
	RP_D(const PalmOS);
	if (!d->isValid || !isSystemNameTypeValid(type))
		return nullptr;

	// game.com has the same name worldwide, so we can
	// ignore the region selection.
	static_assert(SYSNAME_TYPE_MASK == 3,
		"PalmOS::systemName() array index optimization needs to be updated.");

	static const char *const sysNames[4] = {
		"Palm OS",
		"Palm OS",
		"Palm",
		nullptr
	};

	return sysNames[type & SYSNAME_TYPE_MASK];
}

/**
 * Get a bitfield of image types this object can retrieve.
 * @return Bitfield of supported image types. (ImageTypesBF)
 */
uint32_t PalmOS::supportedImageTypes(void) const
{
	// TODO: Check for a valid "tAIB" resource first?
	return IMGBF_INT_ICON;
}

/**
 * Get a bitfield of image types this class can retrieve.
 * @return Bitfield of supported image types. (ImageTypesBF)
 */
uint32_t PalmOS::supportedImageTypes_static(void)
{
	return IMGBF_INT_ICON;
}

/**
 * Get a list of all available image sizes for the specified image type.
 * @param imageType Image type.
 * @return Vector of available image sizes, or empty vector if no images are available.
 */
vector<RomData::ImageSizeDef> PalmOS::supportedImageSizes(ImageType imageType) const
{
	ASSERT_supportedImageSizes(imageType);

	// TODO: Check for a valid "tAIB" resource first?
	// Also, what are the valid icon sizes?
	RP_D(const PalmOS);
	if (!d->isValid || imageType != IMG_INT_ICON) {
		// Only IMG_INT_ICON is supported,
		// and/or the ROM doesn't have an icon.
		return {};
	}

	return {{nullptr, 32, 32, 0}};
}

/**
 * Get a list of all available image sizes for the specified image type.
 * @param imageType Image type.
 * @return Vector of available image sizes, or empty vector if no images are available.
 */
vector<RomData::ImageSizeDef> PalmOS::supportedImageSizes_static(ImageType imageType)
{
	ASSERT_supportedImageSizes(imageType);

	if (imageType != IMG_INT_ICON) {
		// Only IMG_INT_ICON is supported.
		return {};
	}

	// TODO: What are the valid icon sizes?
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
uint32_t PalmOS::imgpf(ImageType imageType) const
{
	ASSERT_imgpf(imageType);

	switch (imageType) {
		case IMG_INT_ICON: {
			// TODO: Check for a valid "tAIB" resource first?
			// Use nearest-neighbor scaling.
			return IMGPF_RESCALE_NEAREST;
			break;
		}
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
int PalmOS::loadFieldData(void)
{
	RP_D(PalmOS);
	if (!d->fields.empty()) {
		// Field data *has* been loaded...
		return 0;
	} else if (!d->file) {
		// File isn't open.
		return -EBADF;
	} else if (!d->isValid) {
		// ROM image isn't valid.
		return -EIO;
	}

	// TODO: Add more fields?
	// TODO: Text encoding?
	const PalmOS_PRC_Header_t *const prcHeader = &d->prcHeader;
	d->fields.reserve(3);	// Maximum of 3 fields.

	// Internal name
	d->fields.addField_string(C_("PalmOS", "Internal Name"),
		latin1_to_utf8(prcHeader->name, sizeof(prcHeader->name)),
		RomFields::STRF_TRIM_END);

	// Creator ID
	// TODO: Filter out non-ASCII characters.
	if (prcHeader->creator_id != 0) {
		char s_creator_id[5];
		uint32_t creator_id = be32_to_cpu(prcHeader->creator_id);
		for (int i = 3; i >= 0; i--, creator_id >>= 8) {
			s_creator_id[i] = (creator_id & 0xFF);
		}
		s_creator_id[4] = '\0';
		d->fields.addField_string(C_("PalmOS", "Creator ID"), s_creator_id,
			RomFields::STRF_MONOSPACE);
	}

	// Finished reading the field data.
	return static_cast<int>(d->fields.count());
}

/**
 * Load metadata properties.
 * Called by RomData::metaData() if the metadata hasn't been loaded yet.
 * @return Number of metadata properties read on success; negative POSIX error code on error.
 */
int PalmOS::loadMetaData(void)
{
	RP_D(PalmOS);
	if (d->metaData != nullptr) {
		// Metadata *has* been loaded...
		return 0;
	} else if (!d->file) {
		// File isn't open.
		return -EBADF;
	} else if (!d->isValid) {
		// Unknown ROM image type.
		return -EIO;
	}

	// Create the metadata object.
	d->metaData = new RomMetaData();
	d->metaData->reserve(1);	// Maximum of 1 metadata property.

	// TODO: Text encoding?
	const PalmOS_PRC_Header_t *const prcHeader = &d->prcHeader;

	// Internal name
	// TODO: Use "tAIN" resource if available?
	d->metaData->addMetaData_string(Property::Title,
		latin1_to_utf8(prcHeader->name, sizeof(prcHeader->name)),
		RomMetaData::STRF_TRIM_END);

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
int PalmOS::loadInternalImage(ImageType imageType, rp_image_const_ptr &pImage)
{
	ASSERT_loadInternalImage(imageType, pImage);

	RP_D(PalmOS);
	if (imageType != IMG_INT_ICON) {
		pImage.reset();
		return -ENOENT;
	} else if (d->img_icon != nullptr) {
		pImage = d->img_icon;
		return 0;
	} else if (!d->file) {
		pImage.reset();
		return -EBADF;
	} else if (!d->isValid) {
		pImage.reset();
		return -EIO;
	}

	pImage = d->loadIcon();
	return ((bool)pImage ? 0 : -EIO);
}

}
