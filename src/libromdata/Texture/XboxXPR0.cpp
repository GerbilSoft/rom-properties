/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * XboxXPR0.hpp: Microsoft Xbox XPR0 image reader.                         *
 *                                                                         *
 * Copyright (c) 2019 by David Korth.                                      *
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

#include "XboxXPR0.hpp"
#include "librpbase/RomData_p.hpp"

#include "xbox_xpr0_structs.h"

// librpbase
#include "librpbase/common.h"
#include "librpbase/byteswap.h"
#include "librpbase/bitstuff.h"
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
#include <vector>
using std::vector;

namespace LibRomData {

ROMDATA_IMPL(XboxXPR0)
ROMDATA_IMPL_IMG_TYPES(XboxXPR0)

class XboxXPR0Private : public RomDataPrivate
{
	public:
		XboxXPR0Private(XboxXPR0 *q, IRpFile *file);
		~XboxXPR0Private();

	private:
		typedef RomDataPrivate super;
		RP_DISABLE_COPY(XboxXPR0Private)

	public:
		// XPR0 header.
		Xbox_XPR0_Header xpr0Header;

		// Decoded image.
		rp_image *img;

		/**
		 * Load the XboxXPR0 image.
		 * @return Image, or nullptr on error.
		 */
		const rp_image *loadXboxXPR0Image(void);
};

/** XboxXPR0Private **/

XboxXPR0Private::XboxXPR0Private(XboxXPR0 *q, IRpFile *file)
	: super(q, file)
	, img(nullptr)
{
	// Clear the XboxXPR0 header struct.
	memset(&xpr0Header, 0, sizeof(xpr0Header));
}

XboxXPR0Private::~XboxXPR0Private()
{
	delete img;
}

/**
 * Load the XboxXPR0 image.
 * @return Image, or nullptr on error.
 */
const rp_image *XboxXPR0Private::loadXboxXPR0Image(void)
{
	if (img) {
		// Image has already been loaded.
		return img;
	} else if (!this->file) {
		// Can't load the image.
		return nullptr;
	}

	if (file->size() > 16*1024*1024) {
		// Sanity check: XPR0 files shouldn't be more than 16 MB.
		return nullptr;
	}

	// XPR0 textures are always square and encoded using DXT1.
	// TODO: Maybe other formats besides DXT1?

	// DXT1 is 8 bytes per 4x4 pixel block.
	// Divide the image area by 2.
	const uint32_t file_sz = static_cast<uint32_t>(file->size());
	const uint32_t data_offset = le32_to_cpu(xpr0Header.data_offset);

	// Determine the expected size based on the pixel format.
	const unsigned int area_shift = (xpr0Header.width_pow2 >> 4) +
					(xpr0Header.height_pow2 & 0x0F);
	uint32_t expected_size;
	switch (xpr0Header.pixel_format) {
		case XPR0_PIXEL_FORMAT_DXT1:
			// 8 bytes per 4x4 block
			expected_size = static_cast<uint32_t>(1U << (area_shift - 1));
			break;
		case XPR0_PIXEL_FORMAT_DXT2:
		case XPR0_PIXEL_FORMAT_DXT4:
			// 16 bytes per 4x4 block
			expected_size = static_cast<uint32_t>(1U << area_shift);
			break;
		default:
			// Unsupported...
			return nullptr;
	}

	if (expected_size > file_sz - data_offset) {
		// File is too small.
		return nullptr;
	}

	// Read the image data.
	auto buf = aligned_uptr<uint8_t>(16, expected_size);
	size_t size = file->seekAndRead(data_offset, buf.get(), expected_size);
	if (size != expected_size) {
		// Seek and/or read error.
		return nullptr;
	}

	const int width  = 1 << (xpr0Header.width_pow2 >> 4);
	const int height = 1 << (xpr0Header.height_pow2 & 0x0F);
	switch (xpr0Header.pixel_format) {
		case XPR0_PIXEL_FORMAT_DXT1:
			// NOTE: Assuming we have transparent pixels.
			img = ImageDecoder::fromDXT1_A1(width, height,
				buf.get(), expected_size);
			break;
		case XPR0_PIXEL_FORMAT_DXT2:
			img = ImageDecoder::fromDXT2(width, height,
				buf.get(), expected_size);
			break;
		case XPR0_PIXEL_FORMAT_DXT4:
			img = ImageDecoder::fromDXT4(width, height,
				buf.get(), expected_size);
			break;
		default:
			// Unsupported...
			return nullptr;
	}
	return img;
}

/** XboxXPR0 **/

/**
 * Read a Microsoft Xbox XPR0 image file.
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
XboxXPR0::XboxXPR0(IRpFile *file)
	: super(new XboxXPR0Private(this, file))
{
	// This class handles texture files.
	RP_D(XboxXPR0);
	d->className = "XboxXPR0";
	d->fileType = FTYPE_TEXTURE_FILE;

	if (!d->file) {
		// Could not ref() the file handle.
		return;
	}

	// Read the XPR0 header.
	d->file->rewind();
	size_t size = d->file->read(&d->xpr0Header, sizeof(d->xpr0Header));
	if (size != sizeof(d->xpr0Header)) {
		d->file->unref();
		d->file = nullptr;
		return;
	}

	// Check if this XPR0 image is supported.
	DetectInfo info;
	info.header.addr = 0;
	info.header.size = sizeof(d->xpr0Header);
	info.header.pData = reinterpret_cast<const uint8_t*>(&d->xpr0Header);
	info.ext = nullptr;	// Not needed for XPR0.
	info.szFile = file->size();
	d->isValid = (isRomSupported_static(&info) >= 0);

	if (!d->isValid) {
		d->file->unref();
		d->file = nullptr;
	}
}

/**
 * Is a ROM image supported by this class?
 * @param info DetectInfo containing ROM detection information.
 * @return Class-specific system ID (>= 0) if supported; -1 if not.
 */
int XboxXPR0::isRomSupported_static(const DetectInfo *info)
{
	assert(info != nullptr);
	assert(info->header.pData != nullptr);
	assert(info->header.addr == 0);
	if (!info || !info->header.pData ||
	    info->header.addr != 0 ||
	    info->header.size < sizeof(Xbox_XPR0_Header))
	{
		// Either no detection information was specified,
		// or the header is too small.
		return -1;
	}

	// Verify the XPR0 magic.
	const Xbox_XPR0_Header *const xpr0Header =
		reinterpret_cast<const Xbox_XPR0_Header*>(info->header.pData);
	if (xpr0Header->magic == cpu_to_be32(XBOX_XPR0_MAGIC)) {
		// Valid magic.
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
const char *XboxXPR0::systemName(unsigned int type) const
{
	RP_D(const XboxXPR0);
	if (!d->isValid || !isSystemNameTypeValid(type))
		return nullptr;

	// Microsoft Xbox has the same name worldwide, so we can
	// ignore the region selection.
	static_assert(SYSNAME_TYPE_MASK == 3,
		"XboxXPR0::systemName() array index optimization needs to be updated.");

	// Bits 0-1: Type. (long, short, abbreviation)
	static const char *const sysNames[4] = {
		"Microsoft Xbox", "Xbox", "Xbox", nullptr
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
const char *const *XboxXPR0::supportedFileExtensions_static(void)
{
	static const char *const exts[] = {
		".xbx",

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
const char *const *XboxXPR0::supportedMimeTypes_static(void)
{
	static const char *const mimeTypes[] = {
		// Unofficial MIME types.
		// TODO: Get these upstreamed on FreeDesktop.org.
		"image/x-xbox-xpr0",

		nullptr
	};
	return mimeTypes;
}

/**
 * Get a bitfield of image types this class can retrieve.
 * @return Bitfield of supported image types. (ImageTypesBF)
 */
uint32_t XboxXPR0::supportedImageTypes_static(void)
{
	return IMGBF_INT_IMAGE;
}

/**
 * Get a list of all available image sizes for the specified image type.
 * @param imageType Image type.
 * @return Vector of available image sizes, or empty vector if no images are available.
 */
vector<RomData::ImageSizeDef> XboxXPR0::supportedImageSizes(ImageType imageType) const
{
	ASSERT_supportedImageSizes(imageType);

	RP_D(const XboxXPR0);
	if (!d->isValid || imageType != IMG_INT_IMAGE) {
		return vector<ImageSizeDef>();
	}

	// Return the image's size.
	const ImageSizeDef imgsz[] = {{nullptr,
		static_cast<uint16_t>(1U << (d->xpr0Header.width_pow2 >> 4)),
		static_cast<uint16_t>(1U << (d->xpr0Header.height_pow2 & 0x0F)),
		0
	}};
	return vector<ImageSizeDef>(imgsz, imgsz + 1);
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
uint32_t XboxXPR0::imgpf(ImageType imageType) const
{
	ASSERT_imgpf(imageType);

	RP_D(const XboxXPR0);
	if (imageType != IMG_INT_IMAGE) {
		// Only IMG_INT_IMAGE is supported by PVR.
		return 0;
	}

	// If both dimensions of the texture are 64 or less,
	// specify nearest-neighbor scaling.
	uint32_t ret = 0;
	if ((d->xpr0Header.width_pow2 >> 4) <= 6 ||
	    (d->xpr0Header.height_pow2 & 0x0F) <= 6)
	{
		// 64x64 or smaller.
		ret = IMGPF_RESCALE_NEAREST;
	}
	return ret;
}

/**
 * Load field data.
 * Called by RomData::fields() if the field data hasn't been loaded yet.
 * @return Number of fields read on success; negative POSIX error code on error.
 */
int XboxXPR0::loadFieldData(void)
{
	RP_D(XboxXPR0);
	if (!d->fields->empty()) {
		// Field data *has* been loaded...
		return 0;
	} else if (!d->file) {
		// File isn't open.
		return -EBADF;
	} else if (!d->isValid) {
		// Unknown file type.
		return -EIO;
	}

	// XboxXPR0 header
	const Xbox_XPR0_Header *const xpr0Header = &d->xpr0Header;
	d->fields->reserve(2);	// Maximum of 2 fields.

	// Pixel format
	static const char *const pxfmt_tbl[] = {
		// 0x00
		nullptr, nullptr, "ARGB1555", nullptr,
		"ARGB4444", "RGB565", "ARGB8888", "xRGB8888",
		// 0x08
		nullptr, nullptr, nullptr, nullptr,
		"DXT1", nullptr, "DXT2", "DXT4",
		// 0x10
		"Linear ARGB1555", "Linear RGB565",
		"Linear ARGB8888", nullptr,
		nullptr, nullptr, nullptr, nullptr,
		// 0x18
		nullptr, nullptr, nullptr, nullptr,
		nullptr, "Linear ARGB4444", "Linear xRGB8888", nullptr,
	};
	if (xpr0Header->pixel_format < ARRAY_SIZE(pxfmt_tbl)) {
		d->fields->addField_string(C_("XboxXPR0", "Pixel Format"),
			pxfmt_tbl[xpr0Header->pixel_format]);
	} else {
		d->fields->addField_string(C_("XboxXPR0", "Pixel Format"),
			rp_sprintf(C_("RomData", "Unknown (0x%02X)"), xpr0Header->pixel_format));
	}

	// Texture size
	d->fields->addField_dimensions(C_("XboxXPR0", "Texture Size"),
		1 << (xpr0Header->width_pow2 >> 4),
		1 << (xpr0Header->height_pow2 & 0x0F));

	// TODO: More fields.

	// Finished reading the field data.
	return static_cast<int>(d->fields->count());
}

/**
 * Load metadata properties.
 * Called by RomData::metaData() if the field data hasn't been loaded yet.
 * @return Number of metadata properties read on success; negative POSIX error code on error.
 */
int XboxXPR0::loadMetaData(void)
{
	RP_D(XboxXPR0);
	if (d->metaData != nullptr) {
		// Metadata *has* been loaded...
		return 0;
	} else if (!d->file) {
		// File isn't open.
		return -EBADF;
	} else if (!d->isValid) {
		// Unknown file type.
		return -EIO;
	}

	// Create the metadata object.
	d->metaData = new RomMetaData();
	d->metaData->reserve(2);	// Maximum of 2 metadata properties.

	// XboxXPR0 header.
	const Xbox_XPR0_Header *const xpr0Header = &d->xpr0Header;

	// Dimensions.
	d->metaData->addMetaData_integer(Property::Width, 1 << (xpr0Header->width_pow2 >> 4));
	d->metaData->addMetaData_integer(Property::Height, 1 << (xpr0Header->height_pow2 & 0x0F));

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
int XboxXPR0::loadInternalImage(ImageType imageType, const rp_image **pImage)
{
	ASSERT_loadInternalImage(imageType, pImage);

	RP_D(XboxXPR0);
	if (imageType != IMG_INT_IMAGE) {
		// Only IMG_INT_IMAGE is supported by PVR.
		*pImage = nullptr;
		return -ENOENT;
	} else if (!d->file) {
		// File isn't open.
		*pImage = nullptr;
		return -EBADF;
	} else if (!d->isValid) {
		// Unknown file type.
		*pImage = nullptr;
		return -EIO;
	}

	// Load the image.
	*pImage = d->loadXboxXPR0Image();
	return (*pImage != nullptr ? 0 : -EIO);
}

}
