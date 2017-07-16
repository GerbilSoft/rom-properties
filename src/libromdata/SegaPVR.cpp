/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * SegaPVR.cpp: Sega PVR image reader.                                     *
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

#include "SegaPVR.hpp"
#include "librpbase/RomData_p.hpp"

#include "pvr_structs.h"

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

class SegaPVRPrivate : public RomDataPrivate
{
	public:
		SegaPVRPrivate(SegaPVR *q, IRpFile *file);
		~SegaPVRPrivate();

	private:
		typedef RomDataPrivate super;
		RP_DISABLE_COPY(SegaPVRPrivate)

	public:
		enum PVRType {
			PVR_TYPE_UNKNOWN	= -1,	// Unknown image type.
			PVR_TYPE_PVR		= 0,	// Dreamcast PVR
			PVR_TYPE_GVR		= 1,	// GameCube GVR
			PVR_TYPE_PVRX		= 2,	// Xbox PVRX

			PVR_TYPE_MAX
		};

		// PVR type.
		int pvrType;

		// PVR header.
		PVR_Header pvrHeader;

#if SYS_BYTEORDER == SYS_LIL_ENDIAN
		static inline void byteswap_pvr(PVR_Header *pvr) {
			RP_UNUSED(pvr);
		}
		static inline void byteswap_gvr(PVR_Header *gvr);
#else /* SYS_BYTEORDER == SYS_BIG_ENDIAN */
		static inline void byteswap_pvr(PVR_Header *pvr);
		static inline void byteswap_gvr(PVR_Header *gvr) {
			RP_UNUSED(pvr);
		}
#endif

		// Global Index.
		bool hasGbix;
		uint32_t gbix;

		// Decoded image.
		rp_image *img;

		/**
		 * Unsigned integer log2(n).
		 * @param n Value
		 * @return uilog2(n)
		 */
		static inline unsigned int uilog2(unsigned int n);

		/**
		 * Load the PVR image.
		 * @return Image, or nullptr on error.
		 */
		const rp_image *loadPvrImage(void);

		/**
		 * Load the GVR image.
		 * @return Image, or nullptr on error.
		 */
		const rp_image *loadGvrImage(void);
};

/** SegaPVRPrivate **/

SegaPVRPrivate::SegaPVRPrivate(SegaPVR *q, IRpFile *file)
	: super(q, file)
	, pvrType(PVR_TYPE_UNKNOWN)
	, hasGbix(false)
	, gbix(0)
	, img(nullptr)
{
	// Clear the PVR header structs.
	memset(&pvrHeader, 0, sizeof(pvrHeader));
}

SegaPVRPrivate::~SegaPVRPrivate()
{
	delete img;
}

#if SYS_BYTEORDER == SYS_BIG_ENDIAN
/**
 * Byteswap a PVR/PVRX header to host-endian.
 * NOTE: Only call this ONCE on a given PVR header!
 * @param pvr PVR header.
 */
inline void SegaPVRPrivate::byteswap_pvr(PVR_Header *pvr)
{
	pvr->length = le32_to_cpu(pvr->length);
	pvr->width = le16_to_cpu(pvr->width);
	pvr->height = le16_to_cpu(pvr->height);
}
#else /* SYS_BYTEORDER == SYS_LIL_ENDIAN */
/**
 * Byteswap a GVR header to host-endian.
 * NOTE: Only call this ONCE on a given GVR header!
 * @param gvr GVR header.
 */
inline void SegaPVRPrivate::byteswap_gvr(PVR_Header *gvr)
{
	gvr->length = be32_to_cpu(gvr->length);
	gvr->width = be16_to_cpu(gvr->width);
	gvr->height = be16_to_cpu(gvr->height);
}
#endif

/**
 * Unsigned integer log2(n).
 * @param n Value
 * @return uilog2(n)
 */
inline unsigned int SegaPVRPrivate::uilog2(unsigned int n)
{
#if defined(__GNUC__)
	return (n == 0 ? 0 : __builtin_clz(n));
#elif defined(_MSC_VER)
	if (n == 0)
		return 0;
	unsigned long index;
	unsigned char x = _BitScanReverse(&index, n);
	return (x ? index : 0);
#else
	unsigned int ret = 0;
	while (n >>= 1)
		ret++;
	return ret;
#endif
}

/**
 * Load the PVR image.
 * @return Image, or nullptr on error.
 */
const rp_image *SegaPVRPrivate::loadPvrImage(void)
{
	if (img) {
		// Image has already been loaded.
		return img;
	} else if (!this->file || this->pvrType != PVR_TYPE_PVR) {
		// Can't load the image.
		return nullptr;
	}

	if (this->file->size() > 128*1024*1024) {
		// Sanity check: PVR files shouldn't be more than 128 MB.
		return nullptr;
	}
	const uint32_t file_sz = (uint32_t)this->file->size();

	uint32_t start = (hasGbix ? 32 : 16);
	uint32_t expect_size = 0;

	switch (pvrHeader.pvr.img_data_type) {
		case PVR_IMG_SQUARE_TWIDDLED_MIPMAP:
		case PVR_IMG_SQUARE_TWIDDLED_MIPMAP_ALT: {
			// Similar to PVR_IMG_SQUARE_TWIDDLED, but mipmaps are present
			// before the main image data.
			// Reference: https://github.com/nickworonekin/puyotools/blob/ccab8e7f788435d1db1fa417b80b96ed29f02b79/Libraries/VrSharp/PvrTexture/PvrTexture.cs#L216
			static const unsigned int bytespp = 2;	// bytes per pixel
			unsigned int mipmapOffset;
			if (pvrHeader.pvr.img_data_type == PVR_IMG_SQUARE_TWIDDLED_MIPMAP) {
				// A 1x1 mipmap takes up as much space as a 2x1 mipmap.
				mipmapOffset = 1*bytespp;
			} else {
				// A 1x1 mipmap takes up as much space as a 2x2 mipmap.
				mipmapOffset = 3*bytespp;
			}

			// Get the log2 of the texture width.
			// FIXME: Make sure it's a power of two.
			assert(pvrHeader.width > 0);
			assert(pvrHeader.width == pvrHeader.height);
			if (pvrHeader.width == 0 || pvrHeader.width != pvrHeader.height)
				return nullptr;

			unsigned int len = uilog2(pvrHeader.width);
			for (unsigned int size = 1; len > 0; len--, size <<= 1) {
				mipmapOffset += std::max(size * size * bytespp, 1U);
			}

			// Skip the mipmaps.
			start += mipmapOffset;
		}

			// fall-through
		case PVR_IMG_SQUARE_TWIDDLED:
		case PVR_IMG_RECTANGLE:
			switch (pvrHeader.pvr.px_format) {
				case PVR_PX_ARGB1555:
				case PVR_PX_RGB565:
				case PVR_PX_ARGB4444:
					expect_size = ((pvrHeader.width * pvrHeader.height) * 2);
					break;

				default:
					// TODO
					return nullptr;
			}
			break;

		default:
			// TODO: Other formats.
			return nullptr;
	}

	if ((expect_size + start) > file_sz) {
		// File is too small.
		return nullptr;
	}

	int ret = file->seek(start);
	if (ret != 0) {
		// Seek error.
		return nullptr;
	}
	unique_ptr<uint8_t> buf(new uint8_t[expect_size]);
	size_t size = file->read(buf.get(), expect_size);
	if (size != expect_size) {
		// Read error.
		return nullptr;
	}

	rp_image *ret_img = nullptr;
	switch (pvrHeader.pvr.img_data_type) {
		case PVR_IMG_SQUARE_TWIDDLED:
		case PVR_IMG_SQUARE_TWIDDLED_MIPMAP:
		case PVR_IMG_SQUARE_TWIDDLED_MIPMAP_ALT:
			switch (pvrHeader.pvr.px_format) {
				case PVR_PX_ARGB1555:
					ret_img = ImageDecoder::fromDreamcastSquareTwiddled16<ImageDecoder::PXF_ARGB1555>(
						pvrHeader.width, pvrHeader.height,
						reinterpret_cast<uint16_t*>(buf.get()), expect_size);
					break;

				case PVR_PX_RGB565:
					ret_img = ImageDecoder::fromDreamcastSquareTwiddled16<ImageDecoder::PXF_RGB565>(
						pvrHeader.width, pvrHeader.height,
						reinterpret_cast<uint16_t*>(buf.get()), expect_size);
					break;

				case PVR_PX_ARGB4444:
					ret_img = ImageDecoder::fromDreamcastSquareTwiddled16<ImageDecoder::PXF_ARGB4444>(
						pvrHeader.width, pvrHeader.height,
						reinterpret_cast<uint16_t*>(buf.get()), expect_size);
					break;

				default:
					// TODO
					return nullptr;
			}
			break;

		case PVR_IMG_RECTANGLE:
			switch (pvrHeader.pvr.px_format) {
				case PVR_PX_ARGB1555:
					ret_img = ImageDecoder::fromLinear16<ImageDecoder::PXF_ARGB1555>(
						pvrHeader.width, pvrHeader.height,
						reinterpret_cast<uint16_t*>(buf.get()), expect_size);
					break;

				case PVR_PX_RGB565:
					ret_img = ImageDecoder::fromLinear16<ImageDecoder::PXF_RGB565>(
						pvrHeader.width, pvrHeader.height,
						reinterpret_cast<uint16_t*>(buf.get()), expect_size);
					break;

				case PVR_PX_ARGB4444:
					ret_img = ImageDecoder::fromLinear16<ImageDecoder::PXF_ARGB4444>(
						pvrHeader.width, pvrHeader.height,
						reinterpret_cast<uint16_t*>(buf.get()), expect_size);
					break;

				default:
					// TODO
					return nullptr;
			}
			break;

		default:
			// TODO: Other formats.
			return nullptr;
	}

	return ret_img;
}

/**
 * Load the GVR image.
 * @return Image, or nullptr on error.
 */
const rp_image *SegaPVRPrivate::loadGvrImage(void)
{
	if (img) {
		// Image has already been loaded.
		return img;
	} else if (!this->file || this->pvrType != PVR_TYPE_GVR) {
		// Can't load the image.
		return nullptr;
	}

	if (this->file->size() > 128*1024*1024) {
		// Sanity check: GVR files shouldn't be more than 128 MB.
		return nullptr;
	}
	const uint32_t file_sz = (uint32_t)this->file->size();

	const uint32_t start = (hasGbix ? 32 : 16);
	uint32_t expect_size = 0;

	switch (pvrHeader.gvr.img_data_type) {
		case GVR_IMG_I4:
		case GVR_IMG_DXT1:
			expect_size = ((pvrHeader.width * pvrHeader.height) / 2);
			break;
		case GVR_IMG_I8:
		case GVR_IMG_IA4:
			expect_size = (pvrHeader.width * pvrHeader.height);
			break;
		case GVR_IMG_IA8:
		case GVR_IMG_RGB565:
		case GVR_IMG_RGB5A3:
			expect_size = ((pvrHeader.width * pvrHeader.height) * 2);
			break;
		case GVR_IMG_ARGB8888:
			expect_size = ((pvrHeader.width * pvrHeader.height) * 4);
			break;

		default:
			// TODO: CI4, CI8
			return nullptr;
	}

	if ((expect_size + start) > file_sz) {
		// File is too small.
		return nullptr;
	}

	int ret = file->seek(start);
	if (ret != 0) {
		// Seek error.
		return nullptr;
	}
	unique_ptr<uint8_t> buf(new uint8_t[expect_size]);
	size_t size = file->read(buf.get(), expect_size);
	if (size != expect_size) {
		// Read error.
		return nullptr;
	}

	rp_image *ret_img = nullptr;
	switch (pvrHeader.gvr.img_data_type) {
		case GVR_IMG_RGB5A3:
			ret_img = ImageDecoder::fromGcnRGB5A3(
				pvrHeader.width, pvrHeader.height,
				reinterpret_cast<uint16_t*>(buf.get()), expect_size);
			break;

		case GVR_IMG_DXT1:
			// TODO: Determine if color 3 should be black or transparent.
			ret_img = ImageDecoder::fromDXT1_BE(
				pvrHeader.width, pvrHeader.height,
				buf.get(), expect_size);
			break;

		default:
			// TODO: Other types.
			return nullptr;
	}

	return ret_img;
}

/** SegaPVR **/

/**
 * Read a Sega PVR image file.
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
SegaPVR::SegaPVR(IRpFile *file)
	: super(new SegaPVRPrivate(this, file))
{
	// This class handles texture files.
	RP_D(SegaPVR);
	d->className = "SegaPVR";
	d->fileType = FTYPE_TEXTURE_FILE;

	if (!d->file) {
		// Could not dup() the file handle.
		return;
	}

	// Read the PVR header.
	uint8_t header[32];
	d->file->rewind();
	size_t size = d->file->read(header, sizeof(header));
	if (size != sizeof(header))
		return;

	// Check if this ROM image is supported.
	DetectInfo info;
	info.header.addr = 0;
	info.header.size = sizeof(header);
	info.header.pData = header;
	info.ext = nullptr;	// Not needed for PVR.
	info.szFile = file->size();
	d->pvrType = isRomSupported_static(&info);
	d->isValid = (d->pvrType >= 0);

	if (!d->isValid)
		return;

	// Check if we have a GBIX header.
	// (or GCIX for some Wii titles)
	if (!memcmp(header, "GBIX", 4) ||
	    !memcmp(header, "GCIX", 4))
	{
		// GBIX header.
		// TODO: Check the "offset" field?
		d->hasGbix = true;
		const PVR_GBIX_Header *const gbixHeader = reinterpret_cast<const PVR_GBIX_Header*>(header);
		if (d->pvrType == SegaPVRPrivate::PVR_TYPE_GVR) {
			// GameCube. GBIX is in big-endian.
			d->gbix = be32_to_cpu(gbixHeader->index);
		} else {
			// Dreamcast, Xbox, or other system.
			// GBIX is in little-endian.
			d->gbix = le32_to_cpu(gbixHeader->index);
		}

		// Copy the main header.
		memcpy(&d->pvrHeader, &header[16], sizeof(d->pvrHeader));
	}
	else
	{
		// No GBIX header. Copy the primary header.
		memcpy(&d->pvrHeader, header, sizeof(d->pvrHeader));
	}

	// Byteswap the fields if necessary.
	switch (d->pvrType) {
		case SegaPVRPrivate::PVR_TYPE_PVR:
		case SegaPVRPrivate::PVR_TYPE_PVRX:
			// Little-endian.
			d->byteswap_pvr(&d->pvrHeader);
			break;

		case SegaPVRPrivate::PVR_TYPE_GVR:
			// Big-endian.
			d->byteswap_gvr(&d->pvrHeader);
			break;

		default:
			// Should not get here...
			assert(!"Invalid PVR type.");
			d->pvrType = SegaPVRPrivate::PVR_TYPE_UNKNOWN;
			d->isValid = false;
			break;
	}
}

/**
 * Is a ROM image supported by this class?
 * @param info DetectInfo containing ROM detection information.
 * @return Class-specific system ID (>= 0) if supported; -1 if not.
 */
int SegaPVR::isRomSupported_static(const DetectInfo *info)
{
	assert(info != nullptr);
	assert(info->header.pData != nullptr);
	assert(info->header.addr == 0);
	if (!info || !info->header.pData ||
	    info->header.addr != 0 ||
	    info->header.size < 32)
	{
		// Either no detection information was specified,
		// or the header is too small.
		return -1;
	}

	const PVR_Header *pvrHeader;

	// Check if we have a GBIX header.
	// (or GCIX for some Wii titles)
	if (!memcmp(info->header.pData, "GBIX", 4) ||
	    !memcmp(info->header.pData, "GCIX", 4))
	{
		// GBIX header is present.
		// TODO: Validate offset?
		pvrHeader = reinterpret_cast<const PVR_Header*>(&info->header.pData[16]);
	}
	else
	{
		// No GBIX header.
		pvrHeader = reinterpret_cast<const PVR_Header*>(info->header.pData);
	}

	// Check the PVR header magic.
	if (!memcmp(pvrHeader->magic, "PVRT", 4)) {
		// Sega Dreamcast PVR.
		return SegaPVRPrivate::PVR_TYPE_PVR;
	} else if (!memcmp(pvrHeader->magic, "GVRT", 4)) {
		// GameCube GVR.
		return SegaPVRPrivate::PVR_TYPE_GVR;
	} else if (!memcmp(pvrHeader->magic, "PVRX", 4)) {
		// Xbox PVRX.
		return SegaPVRPrivate::PVR_TYPE_PVRX;
	}

	// Not supported.
	return -1;
}

/**
 * Is a ROM image supported by this object?
 * @param info DetectInfo containing ROM detection information.
 * @return Class-specific system ID (>= 0) if supported; -1 if not.
 */
int SegaPVR::isRomSupported(const DetectInfo *info) const
{
	return isRomSupported_static(info);
}

/**
 * Get the name of the system the loaded ROM is designed for.
 * @param type System name type. (See the SystemName enum.)
 * @return System name, or nullptr if type is invalid.
 */
const rp_char *SegaPVR::systemName(uint32_t type) const
{
	RP_D(const SegaPVR);
	if (!d->isValid || !isSystemNameTypeValid(type))
		return nullptr;

	// PVR has the same name worldwide, so we can
	// ignore the region selection.
	static_assert(SYSNAME_TYPE_MASK == 3,
		"SegaPVR::systemName() array index optimization needs to be updated.");

	static const rp_char *const sysNames[3][4] = {
		{_RP("Sega Dreamcast PVR"), _RP("Sega PVR"), _RP("PVR"), nullptr},
		{_RP("Sega GVR for GameCube"), _RP("Sega GVR"), _RP("GVR"), nullptr},
		{_RP("Sega PVRX for Xbox"), _RP("Sega PVRX"), _RP("PVRX"), nullptr},
	};

	if (d->pvrType < 0 || d->pvrType >= SegaPVRPrivate::PVR_TYPE_MAX)
		return nullptr;

	return sysNames[d->pvrType][type & SYSNAME_TYPE_MASK];
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
const rp_char *const *SegaPVR::supportedFileExtensions_static(void)
{
	static const rp_char *const exts[] = {
		_RP(".pvr"),	// Sega Dreamcast PVR
		_RP(".gvr"),	// GameCube GVR

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
const rp_char *const *SegaPVR::supportedFileExtensions(void) const
{
	return supportedFileExtensions_static();
}

/**
 * Get a bitfield of image types this class can retrieve.
 * @return Bitfield of supported image types. (ImageTypesBF)
 */
uint32_t SegaPVR::supportedImageTypes_static(void)
{
	return IMGBF_INT_IMAGE;
}

/**
 * Get a bitfield of image types this class can retrieve.
 * @return Bitfield of supported image types. (ImageTypesBF)
 */
uint32_t SegaPVR::supportedImageTypes(void) const
{
	return supportedImageTypes_static();
}

/**
 * Get a list of all available image sizes for the specified image type.
 * @param imageType Image type.
 * @return Vector of available image sizes, or empty vector if no images are available.
 */
vector<RomData::ImageSizeDef> SegaPVR::supportedImageSizes(ImageType imageType) const
{
	assert(imageType >= IMG_INT_MIN && imageType <= IMG_EXT_MAX);
	if (imageType < IMG_INT_MIN || imageType > IMG_EXT_MAX) {
		// ImageType is out of range.
		return vector<ImageSizeDef>();
	}

	RP_D(SegaPVR);
	if (!d->isValid || imageType != IMG_INT_IMAGE) {
		return vector<ImageSizeDef>();
	}

	// Return the image's size.
	const ImageSizeDef imgsz[] = {{nullptr, d->pvrHeader.width, d->pvrHeader.height, 0}};
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
uint32_t SegaPVR::imgpf(ImageType imageType) const
{
	assert(imageType >= IMG_INT_MIN && imageType <= IMG_EXT_MAX);
	if (imageType < IMG_INT_MIN || imageType > IMG_EXT_MAX) {
		// ImageType is out of range.
		return 0;
	}

	RP_D(SegaPVR);
	if (imageType != IMG_INT_IMAGE) {
		// Only IMG_INT_IMAGE is supported by PVR.
		return 0;
	}

	// If both dimensions of the texture are 64 or less,
	// specify nearest-neighbor scaling.
	uint32_t ret = 0;
	if (d->pvrHeader.width <= 64 && d->pvrHeader.width <= 64) {
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
int SegaPVR::loadFieldData(void)
{
	RP_D(SegaPVR);
	if (d->fields->isDataLoaded()) {
		// Field data *has* been loaded...
		return 0;
	} else if (!d->file) {
		// File isn't open.
		return -EBADF;
	} else if (!d->isValid || d->pvrType < 0) {
		// Unknown ROM image type.
		return -EIO;
	}

	// PVR header.
	const PVR_Header *const pvrHeader = &d->pvrHeader;
	d->fields->reserve(4);	// Maximum of 4 fields.

	// Texture size.
	d->fields->addField_string(_RP("Texture Size"),
		rp_sprintf("%ux%u", pvrHeader->width, pvrHeader->height));

	// Pixel format.
	static const rp_char *const pxfmt_tbl[SegaPVRPrivate::PVR_TYPE_MAX][8] = {
		// Sega Dreamcast PVR
		{_RP("ARGB1555"), _RP("RGB565"),
		 _RP("ARGB4444"), _RP("YUV422"),
		 _RP("BUMP"), _RP("4-bit per pixel"),
		 _RP("8-bit per pixel"), nullptr},

		// GameCube GVR
		{_RP("IA8"), _RP("RGB565"), _RP("RGB5A3"),
		 nullptr, nullptr, nullptr, nullptr, nullptr},

		// Xbox PVRX (TODO)
		{nullptr, nullptr, nullptr, nullptr,
		 nullptr, nullptr, nullptr, nullptr},
	};

	// Image data type.
	static const rp_char *const idt_tbl[SegaPVRPrivate::PVR_TYPE_MAX][0x13] = {
		// Sega Dreamcast PVR
		{
			nullptr,					// 0x00
			_RP("Square (Twiddled)"),			// 0x01
			_RP("Square (Twiddled, Mipmap)"),		// 0x02
			_RP("Vector Quantized"),			// 0x03
			_RP("Vector Quantized (Mipmap)"),		// 0x04
			_RP("8-bit Paletted (Twiddled)"),		// 0x05
			_RP("4-bit Paletted (Twiddled)"),		// 0x06
			_RP("8-bit (Twiddled)"),			// 0x07
			_RP("4-bit (Twiddled)"),			// 0x08
			_RP("Rectangle"),				// 0x09
			nullptr,					// 0x0A
			_RP("Rectangle (Stride)"),			// 0x0B
			nullptr,					// 0x0C
			_RP("Rectangle (Twiddled)"),			// 0x0D
			nullptr,					// 0x0E
			nullptr,					// 0x0F
			_RP("Small (Vector Quantized)"),		// 0x10
			_RP("Small (Vector Quantized, Mipmap)"),	// 0x11
			_RP("Square (Twiddled, Mipmap) (Alt)"),		// 0x12
		},

		// GameCube GVR
		{
			_RP("I4"),		// 0x00
			_RP("I8"),		// 0x01
			_RP("IA4"),		// 0x02
			_RP("IA8"),		// 0x03
			_RP("RGB565"),		// 0x04
			_RP("RGB5A3"),		// 0x05
			_RP("ARGB8888"),	// 0x06
			nullptr,		// 0x07
			_RP("CI4"),		// 0x08
			_RP("CI8"),		// 0x09
			nullptr, nullptr,	// 0x0A,0x0B
			nullptr, nullptr,	// 0x0C,0x0D
			_RP("DXT1"),		// 0x0E
			nullptr, nullptr,	// 0x0F,0x10
			nullptr, nullptr, 	// 0x11,0x12
		},

		// Xbox PVRX (TODO)
		{nullptr, nullptr, nullptr, nullptr,
		 nullptr, nullptr, nullptr, nullptr,
		 nullptr, nullptr, nullptr, nullptr,
		 nullptr, nullptr, nullptr, nullptr,
		 nullptr, nullptr, nullptr},
	};

	// GVR has these values located at a different offset.
	// TODO: Verify PVRX.
	uint8_t px_format, img_data_type;
	if (d->pvrType == SegaPVRPrivate::PVR_TYPE_GVR) {
		px_format = pvrHeader->gvr.px_format;
		img_data_type = pvrHeader->gvr.img_data_type;
	} else {
		px_format = pvrHeader->pvr.px_format;
		img_data_type = pvrHeader->pvr.img_data_type;
	}

	const rp_char *pxfmt = nullptr;
	const rp_char *idt = nullptr;
	if (d->pvrType >= 0 && d->pvrType < SegaPVRPrivate::PVR_TYPE_MAX) {
		if (px_format < 8) {
			pxfmt = pxfmt_tbl[d->pvrType][px_format];
		}
		if (img_data_type < 0x13) {
			idt = idt_tbl[d->pvrType][img_data_type];
		}
	}

	// NOTE: Pixel Format might only be valid for GVR if it's DXT1.
	const bool hasPxFmt = (d->pvrType != SegaPVRPrivate::PVR_TYPE_GVR ||
			       img_data_type == GVR_IMG_DXT1);
	if (hasPxFmt) {
		if (pxfmt) {
			d->fields->addField_string(_RP("Pixel Format"), pxfmt);
		} else {
			d->fields->addField_string(_RP("Pixel Format"),
				rp_sprintf("Unknown (0x%02X)", px_format));
		}
	}

	if (idt) {
		d->fields->addField_string(_RP("Image Data Type"), idt);
	} else {
		d->fields->addField_string(_RP("Image Data Type"),
			rp_sprintf("Unknown (0x%02X)", img_data_type));
	}

	// Global index (if present).
	if (d->hasGbix) {
		d->fields->addField_string_numeric(_RP("Global Index"),
			d->gbix, RomFields::FB_DEC, 0);
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
int SegaPVR::loadInternalImage(ImageType imageType, const rp_image **pImage)
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

	RP_D(SegaPVR);
	if (imageType != IMG_INT_IMAGE) {
		// Only IMG_INT_IMAGE is supported by PVR.
		*pImage = nullptr;
		return -ENOENT;
	} else if (!d->file) {
		// File isn't open.
		*pImage = nullptr;
		return -EBADF;
	} else if (!d->isValid || d->pvrType < 0) {
		// PVR image isn't valid.
		*pImage = nullptr;
		return -EIO;
	}

	// Load the image.
	switch (d->pvrType) {
		case SegaPVRPrivate::PVR_TYPE_PVR:
			*pImage = d->loadPvrImage();
			break;
		case SegaPVRPrivate::PVR_TYPE_GVR:
			*pImage = d->loadGvrImage();
			break;
		default:
			// Not supported yet.
			*pImage = nullptr;
			break;
	}
	return (*pImage != nullptr ? 0 : -EIO);
}

}
