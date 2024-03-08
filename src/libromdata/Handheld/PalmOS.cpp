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
// - https://www.cs.trinity.edu/~jhowland/class.files.cs3194.html/palm-docs/Constructor%20for%20Palm%20OS.pdf

#include "stdafx.h"
#include "PalmOS.hpp"
#include "palmos_structs.h"
#include "palmos_system_palette.h"

// Other rom-properties libraries
#include "librptexture/decoder/ImageDecoder_common.hpp"
#include "librptexture/decoder/ImageDecoder_Linear.hpp"
#include "librptexture/decoder/ImageDecoder_Linear_Gray.hpp"
using namespace LibRpBase;
using namespace LibRpFile;
using namespace LibRpText;
using namespace LibRpTexture;
using LibRpTexture::ImageDecoder::PixelFormat;

// C++ STL classes
using std::map;
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

	// TODO: Make this a general icon loading function?

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

	// Read all of the BitmapType struct headers.
	// - Key: Struct header address
	// - Value: PalmOS_BitmapType_t
	// TODO: Do we need to store all of them?
	map<uint32_t, PalmOS_BitmapType_t> bitmapTypeMap;

	while (addr != 0) {
		PalmOS_BitmapType_t bitmapType;
		size_t size = file->seekAndRead(addr, &bitmapType, sizeof(bitmapType));
		if (size != sizeof(bitmapType)) {
			// Failed to read the BitmapType struct.
			return {};
		}

		// Validate the bitmap version and get the next bitmap address.
		const uint32_t cur_addr = addr;
		switch (bitmapType.version) {
			default:
				// Not supported.
				assert(!"Unsupported BitmapType version.");
				return {};

			case 0:
				// v0: no chaining, so this is the last bitmap
				addr = 0;
				break;

			case 1:
				// v1: next bitmap has a relative offset in DWORDs
				if (bitmapType.pixelSize == 255) {
					// FIXME: This is the next bitmap after a v2 bitmap,
					// but there's 16 bytes of weird data between it?
					addr += 16;
					continue;
				}
				if (bitmapType.v1.nextDepthOffset != 0) {
					const unsigned int nextDepthOffset = be16_to_cpu(bitmapType.v1.nextDepthOffset);
					addr += nextDepthOffset * sizeof(uint32_t);
				} else {
					addr = 0;
				}
				break;

			case 2:
				// v2: next bitmap has a relative offset in DWORDs
				// FIXME: v2 sometimes has an extra +0x04 DWORDs offset to the next bitmap? (+0x10 bytes)
				if (bitmapType.v2.nextDepthOffset != 0) {
					const unsigned int nextDepthOffset = be16_to_cpu(bitmapType.v2.nextDepthOffset);// + 0x4;
					addr += nextDepthOffset * sizeof(uint32_t);
				} else {
					addr = 0;
				}
				break;

			case 3:
				// v3: next bitmap has a relative offset in bytes
				if (bitmapType.v3.nextBitmapOffset != 0) {
					addr += be32_to_cpu(bitmapType.v3.nextBitmapOffset);
				} else {
					addr = 0;
				}
				break;
		}

		// NOTE: Byteswapping bitmapType width/height fields here.
#if SYS_BYTEORDER != SYS_BIG_ENDIAN
		bitmapType.width = be16_to_cpu(bitmapType.width);
		bitmapType.height = be16_to_cpu(bitmapType.height);
#endif /* SYS_BYTEORDER != SYS_BIG_ENDIAN */

		// Sanity check: Icon must have valid dimensions.
		assert(bitmapType.width > 0);
		assert(bitmapType.height > 0);
		if (bitmapType.width > 0 && bitmapType.height >= 0) {
			bitmapTypeMap.emplace(cur_addr, bitmapType);
		}
	}

	if (bitmapTypeMap.empty()) {
		// No bitmaps...
		return {};
	}

	// Select the "best" bitmap.
	const PalmOS_BitmapType_t *selBitmapType = nullptr;
	const auto iter_end = bitmapTypeMap.cend();
	for (auto iter = bitmapTypeMap.cbegin(); iter != iter_end; ++iter) {
		if (!selBitmapType) {
			// No bitmap selected yet.
			// NOTE: Only allowing 1-8 bpp for now.
			if (iter->second.pixelSize <= 8) {
				addr = iter->first;
				selBitmapType = &(iter->second);
			}
			continue;
		}

		// Check if this bitmap is "better" than the currently selected one.
		const PalmOS_BitmapType_t *checkBitmapType = &(iter->second);

		// NOTE: Only allowing 1-8 bpp for now.
		if (checkBitmapType->pixelSize > 8)
			continue;

		// First check: Is it a newer version?
		if (checkBitmapType->version > selBitmapType->version) {
			addr = iter->first;
			selBitmapType = checkBitmapType;
			continue;
		}

		// Next check: Is the color depth higher? (bpp; pixelSize)
		if (checkBitmapType->pixelSize > selBitmapType->pixelSize) {
			addr = iter->first;
			selBitmapType = checkBitmapType;
			continue;
		}

		// TODO: v3: Does it have a higher pixel density?

		// Final check: Is it a bigger icon?
		// TODO: Check total area instead of width vs. height?
		if (checkBitmapType->width > selBitmapType->width ||
		    checkBitmapType->height > selBitmapType->height)
		{
			addr = iter->first;
			selBitmapType = checkBitmapType;
			continue;
		}
	}

	if (!selBitmapType) {
		// No bitmap was selected...
		return {};
	}

	static const uint8_t header_size_tbl[] = {
		PalmOS_BitmapType_v0_SIZE,
		PalmOS_BitmapType_v1_SIZE,
		PalmOS_BitmapType_v2_SIZE,
		PalmOS_BitmapType_v3_SIZE,
	};
	assert(selBitmapType->version < ARRAY_SIZE(header_size_tbl));
	if (selBitmapType->version >= ARRAY_SIZE(header_size_tbl)) {
		// Version is not supported...
		return {};
	}
	addr += header_size_tbl[selBitmapType->version];

	// Decode the icon.
	const int width = selBitmapType->width;
	const int height = selBitmapType->height;
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
	const unsigned int rowBytes = be16_to_cpu(selBitmapType->rowBytes);
	const size_t icon_data_len = (size_t)rowBytes * (size_t)height;

	unique_ptr<uint8_t[]> icon_data(new uint8_t[icon_data_len]);
	size_t size = file->seekAndRead(addr, icon_data.get(), icon_data_len);
	if (size != icon_data_len) {
		// Seek and/or read error.
		return {};
	}

	switch (selBitmapType->pixelSize) {
		default:
			assert(!"Pixel size is not supported yet!");
			break;

		case 0:	// NOTE: for v0 only
		case 1:
			// 1-bpp monochrome
			img_icon = ImageDecoder::fromLinearMono(width, height, icon_data.get(), icon_data_len, static_cast<int>(rowBytes));
			break;

		case 2:
			// 2-bpp grayscale
			// TODO: Use $00/$88/$CC/$FF palette instead of $00/$80/$C0/$FF?
			img_icon = ImageDecoder::fromLinearGray2bpp(width, height, icon_data.get(), icon_data_len, static_cast<int>(rowBytes));
			break;

		case 4: {
			// 4-bpp grayscale
			// NOTE: Using a function intended for 16-color images,
			// so we'll have to provide our own palette.
			uint32_t palette[16];
			uint32_t gray = 0xFFFFFFFFU;
			for (unsigned int i = 0; i < ARRAY_SIZE(palette); i++, gray -= 0x111111U) {
				palette[i] = gray;
			}

			img_icon = ImageDecoder::fromLinearCI4(PixelFormat::Host_ARGB32, true,
					width, height,
					icon_data.get(), icon_data_len,
					palette, sizeof(palette), rowBytes);
			if (img_icon) {
				// Set the sBIT metadata.
				// NOTE: Setting the grayscale value, though we're
				// not saving grayscale PNGs at the moment.
				static const rp_image::sBIT_t sBIT = {4,4,4,4,0};
				img_icon->set_sBIT(&sBIT);
			}
			break;
		}

		case 8: {
			// 8-bpp indexed (palette)
			// TODO: Handle various flags.
			// TODO: Use the default Palm OS color table. Using grayscale for now.
			const uint16_t flags = be16_to_cpu(selBitmapType->flags);
			if (flags & (PalmOS_BitmapType_Flags_hasColorTable |
			             /*PalmOS_BitmapType_Flags_hasTransparency |*/ // TODO: not supported yet, but skip it for now
			             PalmOS_BitmapType_Flags_directColor |
			             PalmOS_BitmapType_Flags_indirectColorTable))
			{
				// Flag not supported.
				break;
			}

			// Decompress certain types of images. (TODO: Separate function?)
			if (selBitmapType->version >= 2 && (flags & PalmOS_BitmapType_Flags_compressed)) {
				switch (selBitmapType->v2.compressionType) {
					default:
						// Not supported...
						return {};

					case PalmOS_BitmapType_CompressionType_None:
						// Not actually compressed...
						break;

					case PalmOS_BitmapType_CompressionType_ScanLine: {
						// Scanline compression
						const uint8_t *src = icon_data.get();

						// The image starts with the compressed data size.
						uint32_t compr_size;
						if (selBitmapType->version >= 3) {
							// v3: 32-bit size
							compr_size = (src[0] << 24) | (src[1] << 16) | (src[2] << 8) | src[3];
							src += sizeof(uint32_t);
						} else {
							// v2: 16-bit size
							compr_size = (src[1] << 8) | src[0];
							src += sizeof(uint16_t);
						}

						// TODO: Make sure we don't overrun the source buffer.
						unique_ptr<uint8_t[]> decomp_buf(new uint8_t[icon_data_len]);
						uint8_t *dest = decomp_buf.get();
						const uint8_t *lastrow = dest;
						for (int y = 0; y < height; y++) {
							for (unsigned int x = 0; x < rowBytes; x += 8) {
								// First byte is a diffmask indicating which bytes in
								// an 8-byte group are the same as the previous row.
								// NOTE: Assumed to be 0 for the first row.
								uint8_t diffmask = *src++;
								if (y == 0)
									diffmask = 0xFF;

								// Process 8 bytes.
								unsigned int bytecount = std::min(rowBytes - x, 8U);
								for (unsigned int b = 0; b < bytecount; b++, diffmask <<= 1) {
									uint8_t px;
									if (!(diffmask & (1U << 7))) {
										// Read a byte from the last row.
										px = lastrow[x + b];
									} else {
										// Read a byte from the source data.
										px = *src++;
									}
									*dest++ = px;
								}
							}

							if (y > 0)
								lastrow += rowBytes;
						}

						// Swap the data buffers.
						std::swap(icon_data, decomp_buf);
						break;
					}
				}
			}

			img_icon = ImageDecoder::fromLinearCI8(PixelFormat::Host_ARGB32,
					width, height,
					icon_data.get(), icon_data_len,
					palmos_system_palette, sizeof(palmos_system_palette), rowBytes);
			break;
		}
	}

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
