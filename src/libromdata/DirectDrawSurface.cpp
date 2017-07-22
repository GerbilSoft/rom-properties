/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * DirectDrawSurface.hpp: DirectDraw Surface image reader.                 *
 *                                                                         *
 * Copyright (c) 2017 by David Korth.                                      *
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
 * You should have received a copy of the GNU General Public License along *
 * with this program; if not, write to the Free Software Foundation, Inc., *
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.           *
 ***************************************************************************/

#include "config.librpbase.h"

#include "DirectDrawSurface.hpp"
#include "librpbase/RomData_p.hpp"

#include "dds_structs.h"

// librpbase
#include "librpbase/common.h"
#include "librpbase/byteswap.h"
#include "librpbase/TextFuncs.hpp"
#include "librpbase/file/IRpFile.hpp"
#include "librpbase/img/rp_image.hpp"
#include "librpbase/img/ImageDecoder.hpp"
using namespace LibRpBase;

// C includes. (C++ namespace)
#include <cassert>

// C++ includes.
#include <algorithm>
#include <memory>
#include <vector>
using std::unique_ptr;
using std::vector;

#ifdef _MSC_VER
#include <intrin.h>
#endif

namespace LibRomData {

class DirectDrawSurfacePrivate : public RomDataPrivate
{
	public:
		DirectDrawSurfacePrivate(DirectDrawSurface *q, IRpFile *file);
		~DirectDrawSurfacePrivate();

	private:
		typedef RomDataPrivate super;
		RP_DISABLE_COPY(DirectDrawSurfacePrivate)

	public:
		// DDS header.
		DDS_HEADER ddsHeader;

		// Decoded image.
		rp_image *img;

		/**
		 * Load the image.
		 * @return Image, or nullptr on error.
		 */
		const rp_image *loadImage(void);
};

/** DirectDrawSurfacePrivate **/

DirectDrawSurfacePrivate::DirectDrawSurfacePrivate(DirectDrawSurface *q, IRpFile *file)
	: super(q, file)
	, img(nullptr)
{
	// Clear the DDS header structs.
	memset(&ddsHeader, 0, sizeof(ddsHeader));
}

DirectDrawSurfacePrivate::~DirectDrawSurfacePrivate()
{
	delete img;
}

/**
 * Load the image.
 * @return Image, or nullptr on error.
 */
const rp_image *DirectDrawSurfacePrivate::loadImage(void)
{
	if (img) {
		// Image has already been loaded.
		return img;
	} else if (!this->file || !this->isValid) {
		// Can't load the image.
		return nullptr;
	}

	if (file->size() > 128*1024*1024) {
		// Sanity check: DDS files shouldn't be more than 128 MB.
		return nullptr;
	}
	const uint32_t file_sz = (uint32_t)file->size();

	// Seek to the start of the image data.
	static const unsigned int img_data_start = sizeof(DDS_HEADER) + 4;
	int ret = file->seek(img_data_start);
	if (ret != 0) {
		// Seek error.
		return nullptr;
	}

	// TODO: Support more than just DXT1 and DXT5.

	// NOTE: Mipmaps are stored *after* the main image.
	// Hence, no mipmap processing is necessary.
	rp_image *ret_img = nullptr;
	const DDS_PIXELFORMAT &ddspf = ddsHeader.ddspf;
	if (ddspf.dwFlags & DDPF_FOURCC) {
		// Compressed RGB data.

#ifdef ENABLE_S3TC
		// NOTE: dwPitchOrLinearSize is not necessarily correct.
		// Calculate the expected size.
		uint32_t expected_size;
		switch (ddspf.dwFourCC) {
			case DDPF_FOURCC_DXT1:
				// 16 pixels compressed into 64 bits. (4bpp)
				expected_size = (ddsHeader.dwWidth * ddsHeader.dwHeight) / 2;
				break;

			case DDPF_FOURCC_DXT2:
			case DDPF_FOURCC_DXT3:
			case DDPF_FOURCC_DXT4:
			case DDPF_FOURCC_DXT5:
				// 16 pixels compressed into 128 bits. (8bpp)
				expected_size = ddsHeader.dwWidth * ddsHeader.dwHeight;
				break;

			default:
				// Not supported.
				return nullptr;
		}

		if (expected_size >= file_sz + img_data_start) {
			// File is too small.
			return nullptr;
		}

		unique_ptr<uint8_t[]> buf(new uint8_t[expected_size]);
		size_t size = file->read(buf.get(), expected_size);
		if (size != expected_size) {
			// Read error.
			return nullptr;
		}

		switch (ddspf.dwFourCC) {
			case DDPF_FOURCC_DXT1:
				ret_img = ImageDecoder::fromDXT1(
					ddsHeader.dwWidth, ddsHeader.dwHeight,
					buf.get(), expected_size);
				break;

			case DDPF_FOURCC_DXT2:
				ret_img = ImageDecoder::fromDXT2(
					ddsHeader.dwWidth, ddsHeader.dwHeight,
					buf.get(), expected_size);
				break;

			case DDPF_FOURCC_DXT3:
				ret_img = ImageDecoder::fromDXT3(
					ddsHeader.dwWidth, ddsHeader.dwHeight,
					buf.get(), expected_size);
				break;

			case DDPF_FOURCC_DXT4:
				ret_img = ImageDecoder::fromDXT4(
					ddsHeader.dwWidth, ddsHeader.dwHeight,
					buf.get(), expected_size);
				break;

			case DDPF_FOURCC_DXT5:
				ret_img = ImageDecoder::fromDXT5(
					ddsHeader.dwWidth, ddsHeader.dwHeight,
					buf.get(), expected_size);
				break;

			default:
				// Not supported.
				return nullptr;
		}
#else /* !ENABLE_S3TC */
		// S3TC is disabled in this build.
		break;
#endif
	}

	return ret_img;
}

/** DirectDrawSurface **/

/**
 * Read a DirectDraw Surface image file.
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
DirectDrawSurface::DirectDrawSurface(IRpFile *file)
	: super(new DirectDrawSurfacePrivate(this, file))
{
	// This class handles texture files.
	RP_D(DirectDrawSurface);
	d->className = "DirectDrawSurface";
	d->fileType = FTYPE_TEXTURE_FILE;

	if (!d->file) {
		// Could not dup() the file handle.
		return;
	}

	// Read the DDS magic number and header.
	uint8_t header[sizeof(DDS_HEADER)+4];
	d->file->rewind();
	size_t size = d->file->read(header, sizeof(header));
	if (size != sizeof(header))
		return;

	// Check if this DDS texture is supported.
	DetectInfo info;
	info.header.addr = 0;
	info.header.size = sizeof(header);
	info.header.pData = header;
	info.ext = nullptr;	// Not needed for DDS.
	info.szFile = file->size();
	d->isValid = (isRomSupported_static(&info) >= 0);

	if (d->isValid) {
		// Save the DDS header.
		memcpy(&d->ddsHeader, &header[4], sizeof(d->ddsHeader));

#if SYS_BYTEORDER == SYS_BIG_ENDIAN
		// Byteswap the DDS header.
		d->ddsHeader.dwSize		= le32_to_cpu(d->ddsHeader.dwSize);
		d->ddsHeader.dwFlags		= le32_to_cpu(d->ddsHeader.dwFlags);
		d->ddsHeader.dwHeight		= le32_to_cpu(d->ddsHeader.dwHeight);
		d->ddsHeader.dwWidth		= le32_to_cpu(d->ddsHeader.dwWidth);
		d->ddsHeader.dwPitchOrLinearSize	= le32_to_cpu(d->ddsHeader.dwPitchOrLinearSize);
		d->ddsHeader.dwDepth		= le32_to_cpu(d->ddsHeader.dwDepth);
		d->ddsHeader.dwMipMapCount	= le32_to_cpu(d->ddsHeader.dwMipMapCount);
		d->ddsHeader.dwCaps	= le32_to_cpu(d->ddsHeader.dwCaps);
		d->ddsHeader.dwCaps2	= le32_to_cpu(d->ddsHeader.dwCaps2);
		d->ddsHeader.dwCaps3	= le32_to_cpu(d->ddsHeader.dwCaps3);
		d->ddsHeader.dwCaps4	= le32_to_cpu(d->ddsHeader.dwCaps4);

		// Byteswap the DDS pixel format.
		DDS_PIXELFORMAT &ddspf = d->ddsHeader.ddspf;
		ddspf.dwSize		= le32_to_cpu(ddspf.dwSize);
		ddspf.dwFlags		= le32_to_cpu(ddspf.dwFlags);
		ddspf.dwFourCC		= le32_to_cpu(ddspf.dwFourCC);
		ddspf.dwRGBBitCount	= le32_to_cpu(ddspf.dwRGBBitCount);
		ddspf.dwRBitMask	= le32_to_cpu(ddspf.dwRBitMask);
		ddspf.dwGBitMask	= le32_to_cpu(ddspf.dwGBitMask);
		ddspf.dwBBitMask	= le32_to_cpu(ddspf.dwBBitMask);
		ddspf.dwABitMask	= le32_to_cpu(ddspf.dwABitMask);
#endif /* SYS_BYTEORDER == SYS_BIG_ENDIAN */
	}
}

/**
 * Is a ROM image supported by this class?
 * @param info DetectInfo containing ROM detection information.
 * @return Class-specific system ID (>= 0) if supported; -1 if not.
 */
int DirectDrawSurface::isRomSupported_static(const DetectInfo *info)
{
	assert(info != nullptr);
	assert(info->header.pData != nullptr);
	assert(info->header.addr == 0);
	if (!info || !info->header.pData ||
	    info->header.addr != 0 ||
	    info->header.size < sizeof(DDS_HEADER)+4)
	{
		// Either no detection information was specified,
		// or the header is too small.
		return -1;
	}

	// Verify the DDS magic.
	// TODO: Other checks?
	if (!memcmp(info->header.pData, DDS_MAGIC, 4)) {
		// DDS magic is present.
		// Check the structure sizes.
		const DDS_HEADER *const ddsHeader = reinterpret_cast<const DDS_HEADER*>(&info->header.pData[4]);
		if (le32_to_cpu(ddsHeader->dwSize) == sizeof(*ddsHeader) &&
		    le32_to_cpu(ddsHeader->ddspf.dwSize) == sizeof(ddsHeader->ddspf))
		{
			// Structure sizes are correct.
			return 0;
		}
	}

	// Not supported.
	return -1;
}

/**
 * Is a ROM image supported by this object?
 * @param info DetectInfo containing ROM detection information.
 * @return Class-specific system ID (>= 0) if supported; -1 if not.
 */
int DirectDrawSurface::isRomSupported(const DetectInfo *info) const
{
	return isRomSupported_static(info);
}

/**
 * Get the name of the system the loaded ROM is designed for.
 * @param type System name type. (See the SystemName enum.)
 * @return System name, or nullptr if type is invalid.
 */
const rp_char *DirectDrawSurface::systemName(uint32_t type) const
{
	RP_D(const DirectDrawSurface);
	if (!d->isValid || !isSystemNameTypeValid(type))
		return nullptr;

	// Bits 0-1: Type. (short, long, abbreviation)
	static const rp_char *const sysNames[4] = {
		_RP("DirectDraw Surface"), _RP("DirectDraw Surface"), _RP("DDS"), nullptr
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
const rp_char *const *DirectDrawSurface::supportedFileExtensions_static(void)
{
	static const rp_char *const exts[] = {
		_RP(".dds"),	// DirectDraw Surface

		nullptr
	};
	return exts;
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
const rp_char *const *DirectDrawSurface::supportedFileExtensions(void) const
{
	return supportedFileExtensions_static();
}

/**
 * Get a bitfield of image types this class can retrieve.
 * @return Bitfield of supported image types. (ImageTypesBF)
 */
uint32_t DirectDrawSurface::supportedImageTypes_static(void)
{
	return IMGBF_INT_IMAGE;
}

/**
 * Get a bitfield of image types this class can retrieve.
 * @return Bitfield of supported image types. (ImageTypesBF)
 */
uint32_t DirectDrawSurface::supportedImageTypes(void) const
{
	return supportedImageTypes_static();
}

/**
 * Get a list of all available image sizes for the specified image type.
 * @param imageType Image type.
 * @return Vector of available image sizes, or empty vector if no images are available.
 */
vector<RomData::ImageSizeDef> DirectDrawSurface::supportedImageSizes(ImageType imageType) const
{
	assert(imageType >= IMG_INT_MIN && imageType <= IMG_EXT_MAX);
	if (imageType < IMG_INT_MIN || imageType > IMG_EXT_MAX) {
		// ImageType is out of range.
		return vector<ImageSizeDef>();
	}

	RP_D(DirectDrawSurface);
	if (!d->isValid || imageType != IMG_INT_IMAGE) {
		return vector<ImageSizeDef>();
	}

	// Return the image's size.
	const ImageSizeDef imgsz[] = {{nullptr, (uint16_t)d->ddsHeader.dwWidth, (uint16_t)d->ddsHeader.dwHeight, 0}};
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
uint32_t DirectDrawSurface::imgpf(ImageType imageType) const
{
	assert(imageType >= IMG_INT_MIN && imageType <= IMG_EXT_MAX);
	if (imageType < IMG_INT_MIN || imageType > IMG_EXT_MAX) {
		// ImageType is out of range.
		return 0;
	}

	RP_D(DirectDrawSurface);
	if (imageType != IMG_INT_IMAGE) {
		// Only IMG_INT_IMAGE is supported by DDS.
		return 0;
	}

	// If both dimensions of the texture are 64 or less,
	// specify nearest-neighbor scaling.
	uint32_t ret = 0;
	if (d->ddsHeader.dwWidth <= 64 && d->ddsHeader.dwHeight <= 64) {
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
int DirectDrawSurface::loadFieldData(void)
{
	RP_D(DirectDrawSurface);
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

	// TODO: Flags, capabilities.

	// DDS header.
	const DDS_HEADER *const ddsHeader = &d->ddsHeader;
	d->fields->reserve(3);	// Maximum of 3 fields.

	// Texture size.
	if (ddsHeader->dwFlags & DDSD_DEPTH) {
		d->fields->addField_string(_RP("Texture Size"),
			rp_sprintf("%ux%ux%u", ddsHeader->dwWidth, ddsHeader->dwHeight, ddsHeader->dwDepth));
	} else {
		d->fields->addField_string(_RP("Texture Size"),
			rp_sprintf("%ux%u", ddsHeader->dwWidth, ddsHeader->dwHeight));
	}

	// Pitch (uncompressed)
	// Linear size (compressed)
	const rp_char *pitch_name;
	pitch_name = (ddsHeader->dwFlags & DDSD_LINEARSIZE) ? _RP("Linear Size") : _RP("Pitch");
	d->fields->addField_string_numeric(pitch_name,
		ddsHeader->dwPitchOrLinearSize, RomFields::FB_DEC, 0);

	// Pixel format.
	const DDS_PIXELFORMAT &ddspf = ddsHeader->ddspf;
	if (ddspf.dwFlags & DDPF_FOURCC) {
		// Compressed RGB data.
		d->fields->addField_string(_RP("Pixel Format"),
			rp_sprintf("%c%c%c%c",
				 ddspf.dwFourCC        & 0xFF,
				(ddspf.dwFourCC >>  8) & 0xFF,
				(ddspf.dwFourCC >> 16) & 0xFF,
				(ddspf.dwFourCC >> 24) & 0xFF));
	} else if (ddspf.dwFlags & DDPF_RGB) {
		// Uncompressed RGB data.
		// Check the masks to determine the actual type.
		const rp_char *pxfmt = nullptr;
		if (ddspf.dwFlags & DDPF_ALPHAPIXELS) {
			// Texture has an alpha channel.
			struct RGBA_Format_Table_t {
				unsigned int bits;
				uint32_t Rmask;
				uint32_t Gmask;
				uint32_t Bmask;
				uint32_t Amask;
				const rp_char *desc;
			};
			static const RGBA_Format_Table_t fmt_tbl[] = {
				{32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000, _RP("ARGB8888")},
				{32, 0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000, _RP("ABGR8888")},
				{32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0x000000FF, _RP("RGBA8888")},
				{32, 0x000000FF, 0x0000FF00, 0x00FF0000, 0x000000FF, _RP("BGRA8888")},
				{16, 0x7C00, 0x03E0, 0x001F, 0x8000, _RP("ARGB1555")},
				{16, 0x001F, 0x03E0, 0x007C, 0x8000, _RP("ABGR1555")},
				{16, 0xF800, 0x07C0, 0x003E, 0x0001, _RP("RGBA1555")},
				{16, 0x003E, 0x03E0, 0x00F8, 0x0001, _RP("BGRA1555")},
			};

			for (unsigned int i = 0; i < ARRAY_SIZE(fmt_tbl); i++) {
				const RGBA_Format_Table_t *const fmt = &fmt_tbl[i];
				if (ddspf.dwRGBBitCount == fmt->bits &&
				    ddspf.dwRBitMask == fmt->Rmask &&
				    ddspf.dwGBitMask == fmt->Gmask &&
				    ddspf.dwBBitMask == fmt->Bmask &&
				    ddspf.dwABitMask == fmt->Amask)
				{
					pxfmt = fmt->desc;
					break;
				}
			}
		} else {
			// Texture does not have an alpha channel.
			struct RGB_Format_Table_t {
				unsigned int bits;
				uint32_t Rmask;
				uint32_t Gmask;
				uint32_t Bmask;
				const rp_char *desc;
			};
			static const RGB_Format_Table_t fmt_tbl[] = {
				{32, 0x00FF0000, 0x0000FF00, 0x000000FF, _RP("XRGB8888")},
				{32, 0x000000FF, 0x0000FF00, 0x00FF0000, _RP("XBGR8888")},
				{32, 0x00FF0000, 0x0000FF00, 0x000000FF, _RP("RGBX8888")},
				{32, 0x000000FF, 0x0000FF00, 0x00FF0000, _RP("BGRX8888")},
				{24, 0x00FF0000, 0x0000FF00, 0x000000FF, _RP("RGB888")},
				{24, 0x000000FF, 0x0000FF00, 0x00FF0000, _RP("BGR888")},
				{16, 0xF800, 0x07E0, 0x001F, _RP("RGB565")},
				{16, 0x001F, 0x07E0, 0xF800, _RP("BGR565")},
				{15, 0x7C00, 0x03E0, 0x001F, _RP("RGB555")},
				{15, 0x001F, 0x03E0, 0x7C00, _RP("BGR555")},
			};

			for (unsigned int i = 0; i < ARRAY_SIZE(fmt_tbl); i++) {
				const RGB_Format_Table_t *const fmt = &fmt_tbl[i];
				if (ddspf.dwRGBBitCount == fmt->bits &&
				    ddspf.dwRBitMask == fmt->Rmask &&
				    ddspf.dwGBitMask == fmt->Gmask &&
				    ddspf.dwBBitMask == fmt->Bmask)
				{
					pxfmt = fmt->desc;
					break;
				}
			}
		}
		d->fields->addField_string(_RP("Pixel Format"),
			(pxfmt ? pxfmt : _RP("RGB")));
	} else if (ddspf.dwFlags & DDPF_ALPHA) {
		// Alpha channel.
		d->fields->addField_string(_RP("Pixel Format"),
			rp_sprintf("Alpha (%u-bit)", ddspf.dwRGBBitCount));
	} else if (ddspf.dwFlags & DDPF_YUV) {
		// YUV. (TODO: Determine the format.)
		d->fields->addField_string(_RP("Pixel Format"),
			rp_sprintf("YUV (%u-bit)", ddspf.dwRGBBitCount));
	} else if (ddspf.dwFlags & DDPF_LUMINANCE) {
		// Luminance.
		const char *pxfmt = (ddspf.dwFlags & DDPF_ALPHAPIXELS
			? "Luminance + Alpha" : "Luminance");
		d->fields->addField_string(_RP("Pixel Format"),
			rp_sprintf("%s (%u-bit)", pxfmt, ddspf.dwRGBBitCount));
	} else {
		// Unknown pixel format.
		d->fields->addField_string(_RP("Pixel Format"), _RP("Unknown"));
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
int DirectDrawSurface::loadInternalImage(ImageType imageType, const rp_image **pImage)
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

	RP_D(DirectDrawSurface);
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
