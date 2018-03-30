/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * ValveVTF.cpp: Valve VTF image reader.                                   *
 *                                                                         *
 * Copyright (c) 2017-2018 by David Korth.                                 *
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

/**
 * References:
 * - https://developer.valvesoftware.com/wiki/Valve_Texture_Format
 */

#include "ValveVTF.hpp"
#include "librpbase/RomData_p.hpp"

#include "vtf_structs.h"

// librpbase
#include "librpbase/common.h"
#include "librpbase/byteswap.h"
#include "librpbase/aligned_malloc.h"
#include "librpbase/TextFuncs.hpp"
#include "librpbase/file/IRpFile.hpp"

#include "librpbase/img/rp_image.hpp"
#include "librpbase/img/ImageDecoder.hpp"

#include "libi18n/i18n.h"
using namespace LibRpBase;

// C includes. (C++ namespace)
#include <cassert>
#include <cerrno>
#include <cstring>

// C++ includes.
#include <memory>
#include <string>
#include <vector>
using std::string;
using std::unique_ptr;
using std::vector;

namespace LibRomData {

ROMDATA_IMPL(ValveVTF)
ROMDATA_IMPL_IMG_TYPES(ValveVTF)

class ValveVTFPrivate : public RomDataPrivate
{
	public:
		ValveVTFPrivate(ValveVTF *q, IRpFile *file);
		~ValveVTFPrivate();

	private:
		typedef RomDataPrivate super;
		RP_DISABLE_COPY(ValveVTFPrivate)

	public:
		// VTF header.
		VTFHEADER vtfHeader;

		// Texture data start address.
		unsigned int texDataStartAddr;

		// Decoded image.
		rp_image *img;

		/**
		 * Calculate an image size.
		 * @param format VTF image format.
		 * @param width Image width.
		 * @param height Image height.
		 * @return Image size, in bytes.
		 */
		static unsigned int calcImageSize(VTF_IMAGE_FORMAT format, unsigned int width, unsigned int height);

		/**
		 * Get the minimum block size for the specified format.
		 * @param format VTF image format.
		 * @return Minimum block size, or 0 if invalid.
		 */
		static unsigned int getMinBlockSize(VTF_IMAGE_FORMAT format);

		/**
		 * Load the image.
		 * @return Image, or nullptr on error.
		 */
		const rp_image *loadImage(void);

#if SYS_BYTEORDER == SYS_BIG_ENDIAN
		/**
		 * Byteswap a float. (TODO: Move to byteswap.h?)
		 * @param f Float to byteswap.
		 * @return Byteswapped flaot.
		 */
		static inline float __swabf(float f)
		{
			union {
				uint32_t u32;
				float f;
			} u32_f;
			u32_f.f = f;
			u32_f.u32 = __swab32(u32_f.u32);
			return u32_f.f;
		}
#endif
};

/** ValveVTFPrivate **/

ValveVTFPrivate::ValveVTFPrivate(ValveVTF *q, IRpFile *file)
	: super(q, file)
	, texDataStartAddr(0)
	, img(nullptr)
{
	// Clear the VTF header struct.
	memset(&vtfHeader, 0, sizeof(vtfHeader));
}

ValveVTFPrivate::~ValveVTFPrivate()
{
	delete img;
}

/**
 * Calculate an image size.
 * @param format VTF image format.
 * @param width Image width.
 * @param height Image height.
 * @return Image size, in bytes.
 */
unsigned int ValveVTFPrivate::calcImageSize(VTF_IMAGE_FORMAT format, unsigned int width, unsigned int height)
{
	unsigned int expected_size = width * height;

	switch (format) {
		case VTF_IMAGE_FORMAT_RGBA8888:
		case VTF_IMAGE_FORMAT_ABGR8888:
		case VTF_IMAGE_FORMAT_ARGB8888:
		case VTF_IMAGE_FORMAT_BGRA8888:
		case VTF_IMAGE_FORMAT_BGRx8888:
		case VTF_IMAGE_FORMAT_UVWQ8888:
		case VTF_IMAGE_FORMAT_UVLX8888:
			// 32-bit color formats.
			expected_size *= 4;
			break;

		case VTF_IMAGE_FORMAT_RGB888:
		case VTF_IMAGE_FORMAT_BGR888:
		case VTF_IMAGE_FORMAT_RGB888_BLUESCREEN:
		case VTF_IMAGE_FORMAT_BGR888_BLUESCREEN:
			// 24-bit color formats.
			expected_size *= 3;
			break;

		case VTF_IMAGE_FORMAT_RGB565:
		case VTF_IMAGE_FORMAT_IA88:
		case VTF_IMAGE_FORMAT_BGR565:
		case VTF_IMAGE_FORMAT_BGRx5551:
		case VTF_IMAGE_FORMAT_BGRA4444:
		case VTF_IMAGE_FORMAT_BGRA5551:
		case VTF_IMAGE_FORMAT_UV88:
			// 16-bit color formats.
			expected_size *= 2;
			break;

		case VTF_IMAGE_FORMAT_I8:
		case VTF_IMAGE_FORMAT_P8:
		case VTF_IMAGE_FORMAT_A8:
			// 8-bit color formats.
			break;

		case VTF_IMAGE_FORMAT_DXT1:
		case VTF_IMAGE_FORMAT_DXT1_ONEBITALPHA:
			// 16 pixels compressed into 64 bits. (4bpp)
			expected_size /= 2;
			break;

		case VTF_IMAGE_FORMAT_DXT3:
		case VTF_IMAGE_FORMAT_DXT5:
			// 16 pixels compressed into 128 bits. (8bpp)
			break;

		case VTF_IMAGE_FORMAT_RGBA16161616F:
		case VTF_IMAGE_FORMAT_RGBA16161616:
			// 64-bit color formats.
			expected_size *= 8;
			break;

		default:
			// Not supported.
			expected_size = 0;
			break;
	}

	return expected_size;
}

/**
* Get the minimum block size for the specified format.
* @param format VTF image format.
* @return Minimum block size, or 0 if invalid.
*/
unsigned int ValveVTFPrivate::getMinBlockSize(VTF_IMAGE_FORMAT format)
{
	unsigned int minBlockSize;

	switch (format) {
		case VTF_IMAGE_FORMAT_RGBA8888:
		case VTF_IMAGE_FORMAT_ABGR8888:
		case VTF_IMAGE_FORMAT_ARGB8888:
		case VTF_IMAGE_FORMAT_BGRA8888:
		case VTF_IMAGE_FORMAT_BGRx8888:
		case VTF_IMAGE_FORMAT_UVWQ8888:
		case VTF_IMAGE_FORMAT_UVLX8888:
			// 32-bit color formats.
			minBlockSize = 4;
			break;

		case VTF_IMAGE_FORMAT_RGB888:
		case VTF_IMAGE_FORMAT_BGR888:
		case VTF_IMAGE_FORMAT_RGB888_BLUESCREEN:
		case VTF_IMAGE_FORMAT_BGR888_BLUESCREEN:
			// 24-bit color formats.
			minBlockSize = 3;
			break;

		case VTF_IMAGE_FORMAT_RGB565:
		case VTF_IMAGE_FORMAT_IA88:
		case VTF_IMAGE_FORMAT_BGR565:
		case VTF_IMAGE_FORMAT_BGRx5551:
		case VTF_IMAGE_FORMAT_BGRA4444:
		case VTF_IMAGE_FORMAT_BGRA5551:
		case VTF_IMAGE_FORMAT_UV88:
			// 16-bit color formats.
			minBlockSize = 2;
			break;

		case VTF_IMAGE_FORMAT_I8:
		case VTF_IMAGE_FORMAT_P8:
		case VTF_IMAGE_FORMAT_A8:
			// 8-bit color formats.
			minBlockSize = 1;
			break;

		case VTF_IMAGE_FORMAT_DXT1:
		case VTF_IMAGE_FORMAT_DXT1_ONEBITALPHA:
			// 16 pixels compressed into 64 bits. (4bpp)
			// Minimum block size is 8 bytes.
			minBlockSize = 8;
			break;

		case VTF_IMAGE_FORMAT_DXT3:
		case VTF_IMAGE_FORMAT_DXT5:
			// 16 pixels compressed into 128 bits. (8bpp)
			// Minimum block size is 16 bytes.
			minBlockSize = 16;
			break;

		case VTF_IMAGE_FORMAT_RGBA16161616F:
		case VTF_IMAGE_FORMAT_RGBA16161616:
			// 64-bit color formats.
			minBlockSize = 8;
			break;

		default:
			// Not supported.
			minBlockSize = 0;
			break;
	}

	return minBlockSize;
}

/**
 * Load the image.
 * @return Image, or nullptr on error.
 */
const rp_image *ValveVTFPrivate::loadImage(void)
{
	// TODO: Option to load the low-res image instead?

	if (img) {
		// Image has already been loaded.
		return img;
	} else if (!this->file || !this->isValid) {
		// Can't load the image.
		return nullptr;
	}

	// Sanity check: Maximum image dimensions of 32768x32768.
	// NOTE: `height == 0` is allowed here. (1D texture)
	assert(vtfHeader.width > 0);
	assert(vtfHeader.width <= 32768);
	assert(vtfHeader.height <= 32768);
	if (vtfHeader.width == 0 || vtfHeader.width > 32768 ||
	    vtfHeader.height > 32768)
	{
		// Invalid image dimensions.
		return nullptr;
	}

	if (file->size() > 128*1024*1024) {
		// Sanity check: VTF files shouldn't be more than 128 MB.
		return nullptr;
	}
	const uint32_t file_sz = (uint32_t)file->size();

	// Handle a 1D texture as a "width x 1" 2D texture.
	// NOTE: Handling a 3D texture as a single 2D texture.
	const int height = (vtfHeader.height > 0 ? vtfHeader.height : 1);

	// Calculate the expected size.
	unsigned int expected_size = calcImageSize((VTF_IMAGE_FORMAT)vtfHeader.highResImageFormat,
		vtfHeader.width, height);
	if (expected_size == 0) {
		// Invalid image size.
		return nullptr;
	}

	// TODO: Handle environment maps (6-faced cube map) and volumetric textures.

	// Adjust for the number of mipmaps.
	// NOTE: Dimensions must be powers of two.
	unsigned int texDataStartAddr_adj = texDataStartAddr;
	unsigned int mipmap_size = expected_size;
	const unsigned int minBlockSize = getMinBlockSize((VTF_IMAGE_FORMAT)vtfHeader.highResImageFormat);
	for (unsigned int mipmapLevel = vtfHeader.mipmapCount; mipmapLevel > 1; mipmapLevel--) {
		mipmap_size /= 4;
		if (mipmap_size >= minBlockSize) {
			texDataStartAddr_adj += mipmap_size;
		} else {
			// Mipmap is smaller than the minimum block size
			// for this format.
			texDataStartAddr_adj += minBlockSize;
		}
	}

	// Low-resolution image size.
	texDataStartAddr_adj += calcImageSize(
		(VTF_IMAGE_FORMAT)vtfHeader.lowResImageFormat,
		vtfHeader.lowResImageWidth,
		(vtfHeader.lowResImageHeight > 0 ? vtfHeader.lowResImageHeight : 1));

	// Verify file size.
	if (texDataStartAddr_adj + expected_size > file_sz) {
		// File is too small.
		return nullptr;
	}

	// Texture cannot start inside of the VTF header.
	assert(texDataStartAddr_adj >= sizeof(vtfHeader));
	if (texDataStartAddr_adj < sizeof(vtfHeader)) {
		// Invalid texture data start address.
		return nullptr;
	}

	// Seek to the start of the texture data.
	int ret = file->seek(texDataStartAddr_adj);
	if (ret != 0) {
		// Seek error.
		return nullptr;
	}

	// Read the texture data.
	// TODO: unique_ptr<> helper that uses aligned_malloc() and aligned_free()?
	uint8_t *const buf = static_cast<uint8_t*>(aligned_malloc(16, expected_size));
	if (!buf) {
		// Memory allocation failure.
		return nullptr;
	}
	size_t size = file->read(buf, expected_size);
	if (size != expected_size) {
		// Read error.
		aligned_free(buf);
		return nullptr;
	}

	// Decode the image.
	// NOTE: VTF channel ordering does NOT match ImageDecoder channel ordering.
	// (The channels appear to be backwards.)
	// TODO: Lookup table to convert to PXF constants?
	// TODO: Verify on big-endian?
	switch (vtfHeader.highResImageFormat) {
		/* 32-bit */
		case VTF_IMAGE_FORMAT_RGBA8888:
		case VTF_IMAGE_FORMAT_UVWQ8888:	// handling as RGBA8888
		case VTF_IMAGE_FORMAT_UVLX8888:	// handling as RGBA8888
			img = ImageDecoder::fromLinear32(ImageDecoder::PXF_ABGR8888,
				vtfHeader.width, height,
				reinterpret_cast<const uint32_t*>(buf), expected_size);
			break;
		case VTF_IMAGE_FORMAT_ABGR8888:
			img = ImageDecoder::fromLinear32(ImageDecoder::PXF_RGBA8888,
				vtfHeader.width, height,
				reinterpret_cast<const uint32_t*>(buf), expected_size);
			break;
		case VTF_IMAGE_FORMAT_ARGB8888:
			// This is stored as RAGB for some reason...
			// FIXME: May be a bug in VTFEdit. (Tested versions: 1.2.5, 1.3.3)
			img = ImageDecoder::fromLinear32(ImageDecoder::PXF_RABG8888,
				vtfHeader.width, height,
				reinterpret_cast<const uint32_t*>(buf), expected_size);
			break;
		case VTF_IMAGE_FORMAT_BGRA8888:
			img = ImageDecoder::fromLinear32(ImageDecoder::PXF_ARGB8888,
				vtfHeader.width, height,
				reinterpret_cast<const uint32_t*>(buf), expected_size);
			break;
		case VTF_IMAGE_FORMAT_BGRx8888:
			img = ImageDecoder::fromLinear32(ImageDecoder::PXF_xRGB8888,
				vtfHeader.width, height,
				reinterpret_cast<const uint32_t*>(buf), expected_size);
			break;

		/* 24-bit */
		case VTF_IMAGE_FORMAT_RGB888:
			img = ImageDecoder::fromLinear24(ImageDecoder::PXF_BGR888,
				vtfHeader.width, height,
				buf, expected_size);
			break;
		case VTF_IMAGE_FORMAT_BGR888:
			img = ImageDecoder::fromLinear24(ImageDecoder::PXF_RGB888,
				vtfHeader.width, height,
				buf, expected_size);
			break;
		case VTF_IMAGE_FORMAT_RGB888_BLUESCREEN:
			img = ImageDecoder::fromLinear24(ImageDecoder::PXF_BGR888,
				vtfHeader.width, height,
				buf, expected_size);
			img->apply_chroma_key(0xFF0000FF);
			break;
		case VTF_IMAGE_FORMAT_BGR888_BLUESCREEN:
			img = ImageDecoder::fromLinear24(ImageDecoder::PXF_RGB888,
				vtfHeader.width, height,
				buf, expected_size);
			img->apply_chroma_key(0xFF0000FF);
			break;

		/* 16-bit */
		case VTF_IMAGE_FORMAT_RGB565:
			img = ImageDecoder::fromLinear16(ImageDecoder::PXF_BGR565,
				vtfHeader.width, height,
				reinterpret_cast<const uint16_t*>(buf), expected_size);
			break;
		case VTF_IMAGE_FORMAT_BGR565:
			img = ImageDecoder::fromLinear16(ImageDecoder::PXF_RGB565,
				vtfHeader.width, height,
				reinterpret_cast<const uint16_t*>(buf), expected_size);
			break;
		case VTF_IMAGE_FORMAT_BGRx5551:
			img = ImageDecoder::fromLinear16(ImageDecoder::PXF_RGB555,
				vtfHeader.width, height,
				reinterpret_cast<const uint16_t*>(buf), expected_size);
			break;
		case VTF_IMAGE_FORMAT_BGRA4444:
			img = ImageDecoder::fromLinear16(ImageDecoder::PXF_ARGB4444,
				vtfHeader.width, height,
				reinterpret_cast<const uint16_t*>(buf), expected_size);
			break;
		case VTF_IMAGE_FORMAT_BGRA5551:
			img = ImageDecoder::fromLinear16(ImageDecoder::PXF_ARGB1555,
				vtfHeader.width, height,
				reinterpret_cast<const uint16_t*>(buf), expected_size);
			break;
		case VTF_IMAGE_FORMAT_IA88:
			// FIXME: I8 might have the alpha channel set to the I channel,
			// whereas L8 has A=1.0.
			// https://www.opengl.org/discussion_boards/showthread.php/151701-GL_LUMINANCE-vs-GL_INTENSITY
			// NOTE: Using A8L8 format, not IA8, which is GameCube-specific.
			// (Channels are backwards.)
			// TODO: Add ImageDecoder::fromLinear16() support for IA8 later.
			img = ImageDecoder::fromLinear16(ImageDecoder::PXF_A8L8,
				vtfHeader.width, height,
				reinterpret_cast<const uint16_t*>(buf), expected_size);
			break;
		case VTF_IMAGE_FORMAT_UV88:
			// We're handling this as a GR88 texture.
			img = ImageDecoder::fromLinear16(ImageDecoder::PXF_GR88,
				vtfHeader.width, height,
				reinterpret_cast<const uint16_t*>(buf), expected_size);
			break;

		/* 8-bit */
		case VTF_IMAGE_FORMAT_I8:
			// FIXME: I8 might have the alpha channel set to the I channel,
			// whereas L8 has A=1.0.
			// https://www.opengl.org/discussion_boards/showthread.php/151701-GL_LUMINANCE-vs-GL_INTENSITY
			img = ImageDecoder::fromLinear8(ImageDecoder::PXF_L8,
				vtfHeader.width, height,
				buf, expected_size);
			break;
		case VTF_IMAGE_FORMAT_A8:
			img = ImageDecoder::fromLinear8(ImageDecoder::PXF_A8,
				vtfHeader.width, height,
				buf, expected_size);
			break;

		/* Compressed */
		case VTF_IMAGE_FORMAT_DXT1:
			img = ImageDecoder::fromDXT1(
				vtfHeader.width, height,
				buf, expected_size);
			break;
		case VTF_IMAGE_FORMAT_DXT1_ONEBITALPHA:
			img = ImageDecoder::fromDXT1_A1(
				vtfHeader.width, height,
				buf, expected_size);
			break;
		case VTF_IMAGE_FORMAT_DXT3:
			img = ImageDecoder::fromDXT3(
				vtfHeader.width, height,
				buf, expected_size);
			break;
		case VTF_IMAGE_FORMAT_DXT5:
			img = ImageDecoder::fromDXT5(
				vtfHeader.width, height,
				buf, expected_size);
			break;

		case VTF_IMAGE_FORMAT_P8:
		case VTF_IMAGE_FORMAT_RGBA16161616F:
		case VTF_IMAGE_FORMAT_RGBA16161616:
		default:
			// Not supported.
			break;
	}

	aligned_free(buf);
	return img;
}

/** ValveVTF **/

/**
 * Read a Valve VTF image file.
 *
 * A ROM image must be opened by the caller. The file handle
 * will be dup()'d and must be kept open in order to load
 * data from the ROM image.
 *
 * To close the file, either delete this object or call close().
 *
 * NOTE: Check isValid() to determine if this is a valid ROM.
 *
 * @param file Open ROM image.
 */
ValveVTF::ValveVTF(IRpFile *file)
	: super(new ValveVTFPrivate(this, file))
{
	// This class handles texture files.
	RP_D(ValveVTF);
	d->className = "ValveVTF";
	d->fileType = FTYPE_TEXTURE_FILE;

	if (!d->file) {
		// Could not dup() the file handle.
		return;
	}

	// Read the VTF header.
	d->file->rewind();
	size_t size = d->file->read(&d->vtfHeader, sizeof(d->vtfHeader));
	if (size != sizeof(d->vtfHeader))
		return;

	// Check if this VTF texture is supported.
	DetectInfo info;
	info.header.addr = 0;
	info.header.size = (uint32_t)size;
	info.header.pData = reinterpret_cast<const uint8_t*>(&d->vtfHeader);
	info.ext = nullptr;	// Not needed for VTF.
	info.szFile = file->size();
	d->isValid = (isRomSupported_static(&info) >= 0);

	if (d->isValid) {
#if SYS_BYTEORDER == SYS_BIG_ENDIAN
		// Header is stored in little-endian, so it always
		// needs to be byteswapped on big-endian.
		d->vtfHeader.signature		= le32_to_cpu(d->vtfHeader.signature);
		d->vtfHeader.version[0]		= le32_to_cpu(d->vtfHeader.version[0]);
		d->vtfHeader.version[1]		= le32_to_cpu(d->vtfHeader.version[1]);
		d->vtfHeader.headerSize		= le32_to_cpu(d->vtfHeader.headerSize);
		d->vtfHeader.width		= le16_to_cpu(d->vtfHeader.width);
		d->vtfHeader.height		= le16_to_cpu(d->vtfHeader.height);
		d->vtfHeader.flags		= le32_to_cpu(d->vtfHeader.flags);
		d->vtfHeader.frames		= le16_to_cpu(d->vtfHeader.frames);
		d->vtfHeader.firstFrame		= le16_to_cpu(d->vtfHeader.firstFrame);
		d->vtfHeader.reflectivity[0]	= d->__swabf(d->vtfHeader.reflectivity[0]);
		d->vtfHeader.reflectivity[1]	= d->__swabf(d->vtfHeader.reflectivity[1]);
		d->vtfHeader.reflectivity[2]	= d->__swabf(d->vtfHeader.reflectivity[2]);
		d->vtfHeader.bumpmapScale	= d->__swabf(d->vtfHeader.bumpmapScale);
		d->vtfHeader.highResImageFormat	= le32_to_cpu(d->vtfHeader.highResImageFormat);
		d->vtfHeader.lowResImageFormat	= le32_to_cpu(d->vtfHeader.lowResImageFormat);
		d->vtfHeader.depth		= le16_to_cpu(d->vtfHeader.depth);
		d->vtfHeader.numResources	= le32_to_cpu(d->vtfHeader.numResources);
#endif

		// Texture data start address.
		// Note that this is the start of *all* texture data,
		// including the low-res texture and mipmaps.
		// TODO: Should always be 16-byte aligned?
		// TODO: Verify header size against sizeof(VTFHEADER).
		// Test VTFs are 7.2 with 80-byte headers; sizeof(VTFHEADER) is 72...
		d->texDataStartAddr = d->vtfHeader.headerSize;
	}
}

/**
 * Is a ROM image supported by this class?
 * @param info DetectInfo containing ROM detection information.
 * @return Class-specific system ID (>= 0) if supported; -1 if not.
 */
int ValveVTF::isRomSupported_static(const DetectInfo *info)
{
	assert(info != nullptr);
	assert(info->header.pData != nullptr);
	assert(info->header.addr == 0);
	if (!info || !info->header.pData ||
	    info->header.addr != 0 ||
	    info->header.size < sizeof(VTFHEADER))
	{
		// Either no detection information was specified,
		// or the header is too small.
		return -1;
	}

	// Verify the VTF signature.
	const VTFHEADER *const vtfHeader =
		reinterpret_cast<const VTFHEADER*>(info->header.pData);
	if (vtfHeader->signature == le32_to_cpu(VTF_SIGNATURE)) {
		// VTF signature is correct.
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
const char *ValveVTF::systemName(unsigned int type) const
{
	RP_D(const ValveVTF);
	if (!d->isValid || !isSystemNameTypeValid(type))
		return nullptr;

	// Bits 0-1: Type. (short, long, abbreviation)
	static const char *const sysNames[4] = {
		"Valve VTF Texture", "Valve VTF", "VTF", nullptr
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
const char *const *ValveVTF::supportedFileExtensions_static(void)
{
	static const char *const exts[] = {
		".vtf",
		//".vtx",	// TODO: Some files might use the ".vtx" extension.
		nullptr
	};
	return exts;
}

/**
 * Get a bitfield of image types this class can retrieve.
 * @return Bitfield of supported image types. (ImageTypesBF)
 */
uint32_t ValveVTF::supportedImageTypes_static(void)
{
	return IMGBF_INT_IMAGE;
}

/**
 * Get a list of all available image sizes for the specified image type.
 * @param imageType Image type.
 * @return Vector of available image sizes, or empty vector if no images are available.
 */
vector<RomData::ImageSizeDef> ValveVTF::supportedImageSizes(ImageType imageType) const
{
	assert(imageType >= IMG_INT_MIN && imageType <= IMG_EXT_MAX);
	if (imageType < IMG_INT_MIN || imageType > IMG_EXT_MAX) {
		// ImageType is out of range.
		return vector<ImageSizeDef>();
	}

	RP_D(ValveVTF);
	if (!d->isValid || imageType != IMG_INT_IMAGE) {
		return vector<ImageSizeDef>();
	}

	// Return the image's size.
	const ImageSizeDef imgsz[] = {{nullptr,
		d->vtfHeader.width,
		d->vtfHeader.height, 0}};
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
uint32_t ValveVTF::imgpf(ImageType imageType) const
{
	assert(imageType >= IMG_INT_MIN && imageType <= IMG_EXT_MAX);
	if (imageType < IMG_INT_MIN || imageType > IMG_EXT_MAX) {
		// ImageType is out of range.
		return 0;
	}

	RP_D(ValveVTF);
	if (imageType != IMG_INT_IMAGE) {
		// Only IMG_INT_IMAGE is supported by DDS.
		return 0;
	}

	// If both dimensions of the texture are 64 or less,
	// specify nearest-neighbor scaling.
	uint32_t ret = 0;
	if (d->vtfHeader.width <= 64 && d->vtfHeader.height <= 64) {
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
int ValveVTF::loadFieldData(void)
{
	// TODO: Move to RomFields?
#ifdef _WIN32
	// Windows: 6 visible rows per RFT_LISTDATA.
	static const int rows_visible = 6;
#else
	// Linux: 4 visible rows per RFT_LISTDATA.
	static const int rows_visible = 4;
#endif

	RP_D(ValveVTF);
	if (d->fields->isDataLoaded()) {
		// Field data *has* been loaded...
		return 0;
	} else if (!d->file) {
		// File isn't open.
		return -EBADF;
	} else if (!d->isValid) {
		// Unknown file type.
		return -EIO;
	}

	// VTF header.
	const VTFHEADER *const vtfHeader = &d->vtfHeader;
	d->fields->reserve(12);	// Maximum of 12 fields.

	// VTF version.
	d->fields->addField_string(C_("ValveVTF", "VTF Version"),
		rp_sprintf("%u.%u", vtfHeader->version[0], vtfHeader->version[1]));

	// Texture size.
	// 7.2+ supports 3D textures.
	if ((vtfHeader->version[0] > 7 || (vtfHeader->version[0] == 7 && vtfHeader->version[1] >= 2)) &&
	     vtfHeader->depth > 1)
	{
		d->fields->addField_string(C_("ValveVTF", "Texture Size"),
			rp_sprintf("%ux%ux%u", vtfHeader->width,
				vtfHeader->height,
				vtfHeader->depth));
	} else if (vtfHeader->height > 0) {
		// TODO: >0 or >1?
		d->fields->addField_string(C_("ValveVTF", "Texture Size"),
			rp_sprintf("%ux%u", vtfHeader->width,
				vtfHeader->height));
	} else {
		d->fields->addField_string_numeric(C_("ValveVTF", "Texture Size"), vtfHeader->width);
	}

	// Flags.
	// TODO: Show "deprecated" flags for older versions.
	static const char *const flags_names[] = {
		// 0x1-0x8
		NOP_C_("ValveVTF|Flags", "Point Sampling"),
		NOP_C_("ValveVTF|Flags", "Trilinear Sampling"),
		NOP_C_("ValveVTF|Flags", "Clamp S"),
		NOP_C_("ValveVTF|Flags", "Clamp T"),
		// 0x10-0x80
		NOP_C_("ValveVTF|Flags", "Anisotropic Sampling"),
		NOP_C_("ValveVTF|Flags", "Hint DXT5"),
		NOP_C_("ValveVTF|Flags", "PWL Corrected"),	// "No Compress" (deprecated)
		NOP_C_("ValveVTF|Flags", "Normal Map"),
		// 0x100-0x800
		NOP_C_("ValveVTF|Flags", "No Mipmaps"),
		NOP_C_("ValveVTF|Flags", "No Level of Detail"),
		NOP_C_("ValveVTF|Flags", "No Minimum Mipmap"),
		NOP_C_("ValveVTF|Flags", "Procedural"),
		// 0x1000-0x8000
		NOP_C_("ValveVTF|Flags", "1-bit Alpha"),
		NOP_C_("ValveVTF|Flags", "8-bit Alpha"),
		NOP_C_("ValveVTF|Flags", "Environment Map"),
		NOP_C_("ValveVTF|Flags", "Render Target"),
		// 0x10000-0x80000
		NOP_C_("ValveVTF|Flags", "Depth Render Target"),
		NOP_C_("ValveVTF|Flags", "No Debug Override"),
		NOP_C_("ValveVTF|Flags", "Single Copy"),
		NOP_C_("ValveVTF|Flags", "Pre SRGB"),		// "One Over Mipmap Level in Alpha" (deprecated)
		// 0x100000-0x800000
		NOP_C_("ValveVTF|Flags", "Premult Color by 1/mipmap"),
		NOP_C_("ValveVTF|Flags", "Normal to DuDv"),
		NOP_C_("ValveVTF|Flags", "Alpha Test Mipmap Gen"),
		NOP_C_("ValveVTF|Flags", "No depth Buffer"),
		// 0x1000000-0x8000000
		NOP_C_("ValveVTF|Flags", "Nice Filtered"),
		NOP_C_("ValveVTF|Flags", "Clamp U"),
		NOP_C_("ValveVTF|Flags", "Vertex Texture"),
		NOP_C_("ValveVTF|Flags", "SSBump"),
		// 0x10000000-0x20000000
		nullptr,
		NOP_C_("ValveVTF|Flags", "Border"),
	};

	// Convert to vector<vector<string> > for RFT_LISTDATA.
	auto vv_flags = new vector<vector<string> >();
	vv_flags->resize(ARRAY_SIZE(flags_names));
	for (int i = ARRAY_SIZE(flags_names)-1; i >= 0; i--) {
		auto &data_row = vv_flags->at(i);
		if (flags_names[i]) {
			data_row.push_back(dpgettext_expr(RP_I18N_DOMAIN, "ValveVTF|Flags", flags_names[i]));
		}
	}

	d->fields->addField_listData(C_("ValveVTF", "Flags"), nullptr, vv_flags,
		rows_visible, RomFields::RFT_LISTDATA_CHECKBOXES, vtfHeader->flags);

	// Number of frames.
	d->fields->addField_string_numeric(C_("ValveVTF", "# of Frames"), vtfHeader->frames);
	if (vtfHeader->frames > 1) {
		d->fields->addField_string_numeric(C_("ValveVTF", "First Frame"), vtfHeader->firstFrame);
	}

	// Reflectivity vector.
	d->fields->addField_string(C_("ValveVTF", "Reflectivity Vector"),
		rp_sprintf("(%0.1f, %0.1f, %0.1f)",
			vtfHeader->reflectivity[0],
			vtfHeader->reflectivity[1],
			vtfHeader->reflectivity[2]));

	// Bumpmap scale.
	d->fields->addField_string(C_("ValveVTF", "Bumpmap Scale"),
		rp_sprintf("%0.1f", vtfHeader->bumpmapScale));

	// High-resolution image format.
	static const char *const img_format_tbl[] = {
		"RGBA8888",
		"ABGR8888",
		"RGB888",
		"BGR888",
		"RGB565",
		"I8",
		"IA88",
		"P8",
		"A8",
		NOP_C_("ValveVTF|ImageFormat", "RGB888 (Bluescreen)"),
		NOP_C_("ValveVTF|ImageFormat", "BGR888 (Bluescreen)"),
		"ARGB8888",
		"BGRA8888",
		"DXT1",
		"DXT3",
		"DXT5",
		"BGRx8888",
		"BGR565",
		"BGRx5551",
		"BGRA4444",
		"DXT1_A1",
		"BGRA5551",
		"UV88",
		"UVWQ8888",
		"RGBA16161616F",
		"RGBA16161616",
		"UVLX8888",
	};
	static_assert(ARRAY_SIZE(img_format_tbl) == VTF_IMAGE_FORMAT_MAX, "Missing VTF image formats.");
	const char *img_format = nullptr;
	if (vtfHeader->highResImageFormat < ARRAY_SIZE(img_format_tbl)) {
		img_format = img_format_tbl[vtfHeader->highResImageFormat];
	} else if (vtfHeader->highResImageFormat == (unsigned int)-1) {
		img_format = NOP_C_("ValveVTF|ImageFormat", "None");
	}

	if (img_format) {
		d->fields->addField_string(C_("ValveVTF", "High-Res Image Format"),
			dpgettext_expr(RP_I18N_DOMAIN, "ValveVTF|ImageFormat", img_format));
	} else {
		d->fields->addField_string(C_("ValveVTF", "High-Res Image Format"),
			rp_sprintf(C_("ValveVTF", "Unknown (%d)"), vtfHeader->highResImageFormat));
	}

	// Mipmap count.
	d->fields->addField_string_numeric(C_("ValveVTF", "Mipmap Count"), vtfHeader->mipmapCount);

	// Low-resolution image format.
	img_format = nullptr;
	if (vtfHeader->lowResImageFormat < ARRAY_SIZE(img_format_tbl)) {
		img_format = img_format_tbl[vtfHeader->lowResImageFormat];
	} else if (vtfHeader->lowResImageFormat == (unsigned int)-1) {
		img_format = NOP_C_("ValveVTF|ImageFormat", "None");
	}

	if (img_format) {
		d->fields->addField_string(C_("ValveVTF", "Low-Res Image Format"),
			dpgettext_expr(RP_I18N_DOMAIN, "ValveVTF|ImageFormat", img_format));
		// Low-res image size.
		if (vtfHeader->lowResImageHeight > 0) {
			// TODO: >0 or >1?
			d->fields->addField_string(C_("ValveVTF", "Low-Res Size"),
				rp_sprintf("%ux%u", vtfHeader->lowResImageWidth,
					vtfHeader->lowResImageHeight));
		} else {
			d->fields->addField_string_numeric(C_("ValveVTF", "Low-Res Size"),
				vtfHeader->lowResImageWidth);
		}		
	} else {
		d->fields->addField_string(C_("ValveVTF", "Low-Res Image Format"),
			rp_sprintf(C_("ValveVTF", "Unknown (%d)"), vtfHeader->highResImageFormat));
	}

	if (vtfHeader->version[0] > 7 ||
	    (vtfHeader->version[0] == 7 && vtfHeader->version[1] >= 3))
	{
		// 7.3+: Resources.
		// TODO: Display the resources as RFT_LISTDATA?
		d->fields->addField_string_numeric(C_("ValveVTF", "# of Resources"),
			vtfHeader->numResources);
	}

	// Finished reading the field data.
	return (int)d->fields->count();
}

/**
 * Load an internal image.
 * Called by RomData::image().
 * @param imageType	[in] Image type to load.
 * @param pImage	[out] Pointer to const rp_image* to store the image in.
 * @return 0 on success; negative POSIX error code on error.
 */
int ValveVTF::loadInternalImage(ImageType imageType, const rp_image **pImage)
{
	assert(imageType >= IMG_INT_MIN && imageType <= IMG_INT_MAX);
	assert(pImage != nullptr);
	if (!pImage) {
		// Invalid parameters.
		return -EINVAL;
	} else if (imageType < IMG_INT_MIN || imageType > IMG_INT_MAX) {
		// ImageType is out of range.
		*pImage = nullptr;
		return -ERANGE;
	}

	RP_D(ValveVTF);
	if (imageType != IMG_INT_IMAGE) {
		// Only IMG_INT_IMAGE is supported by DDS.
		*pImage = nullptr;
		return -ENOENT;
	} else if (!d->file) {
		// File isn't open.
		*pImage = nullptr;
		return -EBADF;
	} else if (!d->isValid) {
		// DDS texture isn't valid.
		*pImage = nullptr;
		return -EIO;
	}

	// Load the image.
	*pImage = d->loadImage();
	return (*pImage != nullptr ? 0 : -EIO);
}

}
