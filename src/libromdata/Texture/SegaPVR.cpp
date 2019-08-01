/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * SegaPVR.cpp: Sega PVR image reader.                                     *
 *                                                                         *
 * Copyright (c) 2017-2019 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "SegaPVR.hpp"
#include "librpbase/RomData_p.hpp"

#include "pvr_structs.h"

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
#include <memory>
#include <vector>
using std::unique_ptr;
using std::vector;

namespace LibRomData {

ROMDATA_IMPL(SegaPVR)
ROMDATA_IMPL_IMG_TYPES(SegaPVR)

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
		// gbix_len is 0 if it's not present.
		// Otherwise, may be 16 (common) or 12 (uncommon).
		unsigned int gbix_len;
		uint32_t gbix;

		// Decoded image.
		rp_image *img;

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

		/**
		 * Is this a PlayStation SVR texture?
		 * @return True if format is PVR and pxfmt or idt is SVR; false if not.
		 */
		bool isSvrTexture(void) const;
};

/** SegaPVRPrivate **/

SegaPVRPrivate::SegaPVRPrivate(SegaPVR *q, IRpFile *file)
	: super(q, file)
	, pvrType(PVR_TYPE_UNKNOWN)
	, gbix_len(0)
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

	// Sanity check: Maximum image dimensions of 32768x32768.
	assert(pvrHeader.width > 0);
	assert(pvrHeader.width <= 32768);
	assert(pvrHeader.height > 0);
	assert(pvrHeader.height <= 32768);
	if (pvrHeader.width == 0 || pvrHeader.width > 32768 ||
	    pvrHeader.height == 0 || pvrHeader.height > 32768)
	{
		// Invalid image dimensions.
		return nullptr;
	}

	// Sanity check: PVR files shouldn't be more than 16 MB.
	if (file->size() > 16*1024*1024) {
		return nullptr;
	}
	const uint32_t file_sz = static_cast<uint32_t>(file->size());

	// TODO: Support YUV422, 4-bit, 8-bit, and BUMP formats.
	// Currently assuming all formats use 16bpp.

	const unsigned int pvrDataStart = gbix_len + sizeof(PVR_Header);
	uint32_t mipmap_size = 0;
	uint32_t expected_size = 0;

	// Do we need to skip mipmap data?
	switch (pvrHeader.pvr.img_data_type) {
		case PVR_IMG_SQUARE_TWIDDLED_MIPMAP:
		case PVR_IMG_SQUARE_TWIDDLED_MIPMAP_ALT:
		case PVR_IMG_VQ_MIPMAP:
		case PVR_IMG_SMALL_VQ_MIPMAP: {
			// Skip the mipmaps.
			// Reference: https://github.com/nickworonekin/puyotools/blob/ccab8e7f788435d1db1fa417b80b96ed29f02b79/Libraries/VrSharp/PvrTexture/PvrTexture.cs#L216
			// TODO: For square, determine bpp from pixel format.
			unsigned int bpp;	// bits per pixel
			switch (pvrHeader.pvr.img_data_type) {
				case PVR_IMG_SQUARE_TWIDDLED_MIPMAP:
					// A 1x1 mipmap takes up as much space as a 2x1 mipmap.
					bpp = 16;
					mipmap_size = (1*bpp)>>3;
					break;
				case PVR_IMG_SQUARE_TWIDDLED_MIPMAP_ALT:
					// A 1x1 mipmap takes up as much space as a 2x2 mipmap.
					bpp = 16;
					mipmap_size = (3*bpp)>>3;
					break;
				case PVR_IMG_VQ_MIPMAP:
				case PVR_IMG_SMALL_VQ_MIPMAP:
					// VQ mipmap is technically 2 bits per pixel.
					bpp = 2;
					break;
				default:
					return nullptr;
			}

			// Get the log2 of the texture width.
			// FIXME: Make sure it's a power of two.
			assert(pvrHeader.width == pvrHeader.height);
			if (pvrHeader.width != pvrHeader.height)
				return nullptr;

			unsigned int len = uilog2(pvrHeader.width);
			for (unsigned int size = 1; len > 0; len--, size <<= 1) {
				mipmap_size += std::max((size*size*bpp)>>3, 1U);
			}
			break;
		}

		default:
			// No mipmaps.
			break;
	}

	// Determine the image size.
	switch (pvrHeader.pvr.img_data_type) {
		case PVR_IMG_SQUARE_TWIDDLED_MIPMAP:
		case PVR_IMG_SQUARE_TWIDDLED_MIPMAP_ALT:
		case PVR_IMG_SQUARE_TWIDDLED:
		case PVR_IMG_RECTANGLE:
			switch (pvrHeader.pvr.px_format) {
				case PVR_PX_ARGB1555:
				case PVR_PX_RGB565:
				case PVR_PX_ARGB4444:
					expected_size = ((pvrHeader.width * pvrHeader.height) * 2);
					break;

				default:
					// TODO
					return nullptr;
			}
			break;

		case PVR_IMG_VQ:
			// VQ images have 1024 palette entries,
			// and the image data is 2bpp.
			expected_size = (1024*2) + ((pvrHeader.width * pvrHeader.height) / 4);
			break;

		case PVR_IMG_VQ_MIPMAP:
			// VQ images have 1024 palette entries,
			// and the image data is 2bpp.
			// Skip the palette, since that's handled later.
			mipmap_size += (1024*2);
			expected_size = (pvrHeader.width * pvrHeader.height) / 4;
			break;

		case PVR_IMG_SMALL_VQ: {
			// Small VQ images have up to 1024 palette entries based on width,
			// and the image data is 2bpp.
			const unsigned int pal_siz =
				ImageDecoder::calcDreamcastSmallVQPaletteEntries(pvrHeader.width) * 2;
			expected_size = pal_siz + ((pvrHeader.width * pvrHeader.height) / 4);
			break;
		}

		case PVR_IMG_SMALL_VQ_MIPMAP: {
			// Small VQ images have up to 1024 palette entries based on width,
			// and the image data is 2bpp.
			// Skip the palette, since that's handled later.
			const unsigned int pal_siz =
				ImageDecoder::calcDreamcastSmallVQPaletteEntries(pvrHeader.width) * 2;
			mipmap_size += pal_siz;
			expected_size = ((pvrHeader.width * pvrHeader.height) / 4);
			break;
		}

		default:
			// TODO: Other formats.
			return nullptr;
	}

	if ((pvrDataStart + mipmap_size + expected_size) > file_sz) {
		// File is too small.
		return nullptr;
	}

	int ret = file->seek(pvrDataStart + mipmap_size);
	if (ret != 0) {
		// Seek error.
		return nullptr;
	}

	// Read the texture data.
	auto buf = aligned_uptr<uint8_t>(16, expected_size);
	size_t size = file->read(buf.get(), expected_size);
	if (size != expected_size) {
		// Read error.
		return nullptr;
	}

	// Determine the pixel format.
	// TODO: Not for 4-bit or 8-bit?
	ImageDecoder::PixelFormat px_format;
	switch (pvrHeader.pvr.px_format) {
		case PVR_PX_ARGB1555:
			px_format = ImageDecoder::PXF_ARGB1555;
			break;
		case PVR_PX_RGB565:
			px_format = ImageDecoder::PXF_RGB565;
			break;
		case PVR_PX_ARGB4444:
			px_format = ImageDecoder::PXF_ARGB4444;
			break;
		default:
			// Unsupported pixel format.
			return nullptr;
	}

	switch (pvrHeader.pvr.img_data_type) {
		case PVR_IMG_SQUARE_TWIDDLED:
		case PVR_IMG_SQUARE_TWIDDLED_MIPMAP:
		case PVR_IMG_SQUARE_TWIDDLED_MIPMAP_ALT:
			img = ImageDecoder::fromDreamcastSquareTwiddled16(px_format,
				pvrHeader.width, pvrHeader.height,
				reinterpret_cast<uint16_t*>(buf.get()), expected_size);
			break;

		case PVR_IMG_RECTANGLE:
			img = ImageDecoder::fromLinear16(px_format,
				pvrHeader.width, pvrHeader.height,
				reinterpret_cast<uint16_t*>(buf.get()), expected_size);
			break;

		case PVR_IMG_VQ: {
			// VQ images have a 1024-entry palette.
			static const unsigned int pal_siz = 1024*2;
			const uint16_t *const pal_buf = reinterpret_cast<const uint16_t*>(buf.get());
			const uint8_t *const img_buf = buf.get() + pal_siz;
			const unsigned int img_siz = expected_size - pal_siz;

			img = ImageDecoder::fromDreamcastVQ16<false>(px_format,
				pvrHeader.width, pvrHeader.height,
				img_buf, img_siz, pal_buf, pal_siz);
			break;
		}

		case PVR_IMG_VQ_MIPMAP: {
			// VQ images have a 1024-entry palette.
			// This is stored before the mipmaps, so we need to read it manually.
			static const unsigned int pal_siz = 1024*2;
			ret = file->seek(pvrDataStart);
			if (ret != 0) {
				// Seek error.
				break;
			}
			unique_ptr<uint16_t[]> pal_buf(new uint16_t[pal_siz/2]);
			size = file->read(pal_buf.get(), pal_siz);
			if (size != pal_siz) {
				// Read error.
				break;
			}

			img = ImageDecoder::fromDreamcastVQ16<false>(px_format,
				pvrHeader.width, pvrHeader.height,
				buf.get(), expected_size, pal_buf.get(), pal_siz);
			break;
		}

		case PVR_IMG_SMALL_VQ: {
			// Small VQ images have up to 1024 palette entries based on width.
			const unsigned int pal_siz =
				ImageDecoder::calcDreamcastSmallVQPaletteEntries(pvrHeader.width) * 2;
			const uint16_t *const pal_buf = reinterpret_cast<const uint16_t*>(buf.get());
			const uint8_t *const img_buf = buf.get() + pal_siz;
			const unsigned int img_siz = expected_size - pal_siz;

			img = ImageDecoder::fromDreamcastVQ16<true>(px_format,
				pvrHeader.width, pvrHeader.height,
				img_buf, img_siz, pal_buf, pal_siz);
			break;
		}

		case PVR_IMG_SMALL_VQ_MIPMAP: {
			// Small VQ images have up to 1024 palette entries based on width.
			// This is stored before the mipmaps, so we need to read it manually.
			const unsigned int pal_siz =
				ImageDecoder::calcDreamcastSmallVQPaletteEntries(pvrHeader.width) * 2;
			ret = file->seek(pvrDataStart);
			if (ret != 0) {
				// Seek error.
				break;
			}
			unique_ptr<uint16_t[]> pal_buf(new uint16_t[pal_siz/2]);
			size = file->read(pal_buf.get(), pal_siz);
			if (size != pal_siz) {
				// Read error.
				break;
			}

			img = ImageDecoder::fromDreamcastVQ16<true>(px_format,
				pvrHeader.width, pvrHeader.height,
				buf.get(), expected_size, pal_buf.get(), pal_siz);
			break;
		}

		default:
			// TODO: Other formats.
			break;
	}

	return img;
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

	// Sanity check: Maximum image dimensions of 32768x32768.
	assert(pvrHeader.width > 0);
	assert(pvrHeader.width <= 32768);
	assert(pvrHeader.height > 0);
	assert(pvrHeader.height <= 32768);
	if (pvrHeader.width == 0 || pvrHeader.width > 32768 ||
	    pvrHeader.height == 0 || pvrHeader.height > 32768)
	{
		// Invalid image dimensions.
		return nullptr;
	}

	// Sanity check: GVR files shouldn't be more than 16 MB.
	if (file->size() > 16*1024*1024) {
		return nullptr;
	}
	const uint32_t file_sz = static_cast<uint32_t>(file->size());

	const unsigned int pvrDataStart = gbix_len + sizeof(PVR_Header);
	uint32_t expected_size = 0;

	switch (pvrHeader.gvr.img_data_type) {
		case GVR_IMG_I4:
			expected_size = ((pvrHeader.width * pvrHeader.height) / 2);
			break;
		case GVR_IMG_I8:
		case GVR_IMG_IA4:
			expected_size = (pvrHeader.width * pvrHeader.height);
			break;
		case GVR_IMG_IA8:
		case GVR_IMG_RGB565:
		case GVR_IMG_RGB5A3:
			expected_size = ((pvrHeader.width * pvrHeader.height) * 2);
			break;
		case GVR_IMG_ARGB8888:
			expected_size = ((pvrHeader.width * pvrHeader.height) * 4);
			break;
		case GVR_IMG_DXT1:
			expected_size = ((pvrHeader.width * pvrHeader.height) / 2);
			break;

		default:
			// TODO: CI4, CI8
			return nullptr;
	}

	if ((expected_size + pvrDataStart) > file_sz) {
		// File is too small.
		return nullptr;
	}

	int ret = file->seek(pvrDataStart);
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

	switch (pvrHeader.gvr.img_data_type) {
		case GVR_IMG_I8:
			// FIXME: Untested.
			img = ImageDecoder::fromGcnI8(
				pvrHeader.width, pvrHeader.height,
				buf, expected_size);
			break;

		case GVR_IMG_IA8:
			// FIXME: Untested.
			img = ImageDecoder::fromGcn16(ImageDecoder::PXF_IA8,
				pvrHeader.width, pvrHeader.height,
				reinterpret_cast<uint16_t*>(buf), expected_size);
			break;

		case GVR_IMG_RGB565:
			// FIXME: Untested.
			img = ImageDecoder::fromGcn16(ImageDecoder::PXF_RGB565,
				pvrHeader.width, pvrHeader.height,
				reinterpret_cast<uint16_t*>(buf), expected_size);
			break;

		case GVR_IMG_RGB5A3:
			img = ImageDecoder::fromGcn16(ImageDecoder::PXF_RGB5A3,
				pvrHeader.width, pvrHeader.height,
				reinterpret_cast<uint16_t*>(buf), expected_size);
			break;

		case GVR_IMG_DXT1:
			// TODO: Determine if color 3 should be black or transparent.
			img = ImageDecoder::fromDXT1_GCN(
				pvrHeader.width, pvrHeader.height,
				buf, expected_size);
			break;

		default:
			// TODO: Other types.
			break;
	}

	aligned_free(buf);
	return img;
}

/**
 * Is this a PlayStation SVR texture?
 * @return True if format is PVR and pxfmt or idt is SVR; false if not.
 */
bool SegaPVRPrivate::isSvrTexture(void) const
{
	if (pvrType != PVR_TYPE_PVR) {
		// Not a PVR format file.
		return false;
	}

	if ((pvrHeader.pvr.px_format >= SVR_PX_MIN &&
	     pvrHeader.pvr.px_format <= SVR_PX_MAX) ||
	    (pvrHeader.pvr.img_data_type >= SVR_IMG_MIN &&
	     pvrHeader.pvr.img_data_type <= SVR_IMG_MAX))
	{
		// Pixel format and/or image data type is SVR.
		return true;
	}

	// Not SVR.
	return false;
}

/** SegaPVR **/

/**
 * Read a Sega PVR image file.
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
SegaPVR::SegaPVR(IRpFile *file)
	: super(new SegaPVRPrivate(this, file))
{
	// This class handles texture files.
	RP_D(SegaPVR);
	d->className = "SegaPVR";
	d->fileType = FTYPE_TEXTURE_FILE;

	if (!d->file) {
		// Could not ref() the file handle.
		return;
	}

	// Read the PVR header.
	// Allow up to 32+128 bytes, since the GBIX header
	// might be larger than the normal 8 bytes.
	uint8_t header[32+128];
	d->file->rewind();
	size_t sz_header = d->file->read(header, sizeof(header));
	if (sz_header < 32) {
		d->file->unref();
		d->file = nullptr;
		return;
	}

	// Check if this PVR image is supported.
	DetectInfo info;
	info.header.addr = 0;
	info.header.size = static_cast<uint32_t>(sz_header);
	info.header.pData = header;
	info.ext = nullptr;	// Not needed for PVR.
	info.szFile = file->size();
	d->pvrType = isRomSupported_static(&info);
	d->isValid = (d->pvrType >= 0);

	if (!d->isValid) {
		d->file->unref();
		d->file = nullptr;
		return;
	}

	// Check if we have a GBIX header.
	// (or GCIX for some Wii titles)
	if (!memcmp(header, "GBIX", 4) ||
	    !memcmp(header, "GCIX", 4))
	{
		// GBIX header.
		const PVR_GBIX_Header *const gbixHeader =
			reinterpret_cast<const PVR_GBIX_Header*>(header);

		// GBIX length is *always* in little-endian.
		d->gbix_len = 8 + le32_to_cpu(gbixHeader->length);

		if (d->pvrType == SegaPVRPrivate::PVR_TYPE_GVR) {
			// GameCube. GBIX is in big-endian.
			d->gbix = be32_to_cpu(gbixHeader->index);
		} else {
			// Dreamcast, Xbox, or other system.
			// GBIX is in little-endian.
			d->gbix = le32_to_cpu(gbixHeader->index);
		}

		// Sanity check: gbix_len must be in the range [4,128].
		// NOTE: sz_header is always 32 or higher.
		assert(d->gbix_len >= 4);
		assert(d->gbix_len <= 128);
		if (d->gbix_len < 4 || d->gbix_len > 128 || (d->gbix_len > (sz_header-8))) {
			// Invalid GBIX header.
			d->file->unref();
			d->file = nullptr;
			d->pvrType = SegaPVRPrivate::PVR_TYPE_UNKNOWN;
			d->isValid = false;
			return;
		}

		// Copy the main header.
		memcpy(&d->pvrHeader, &header[d->gbix_len], sizeof(d->pvrHeader));
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
			d->file->unref();
			d->file = nullptr;
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
	    info->header.size < sizeof(PVR_Header))
	{
		// Either no detection information was specified,
		// or the header is too small.
		return -1;
	}

	const PVR_Header *pvrHeader;

	// Check if we have a GBIX header.
	// (or GCIX for some Wii titles)
	const PVR_GBIX_Header *const gbixHeader =
		reinterpret_cast<const PVR_GBIX_Header*>(info->header.pData);
	if (gbixHeader->magic == cpu_to_be32(PVR_MAGIC_GBIX) ||
	    gbixHeader->magic == cpu_to_be32(PVR_MAGIC_GCIX))
	{
		// GBIX header is present.
		// Length should be between 4 and 16.

		// Try little-endian.
		unsigned int gbix_len = le32_to_cpu(gbixHeader->length);
		if (gbix_len < 4 || gbix_len > 128) {
			// Try big-endian.
			gbix_len = be32_to_cpu(gbixHeader->length);
			if (gbix_len < 4 || gbix_len > 128) {
				// Invalid GBIX header.
				return -1;
			}
		}

		// Make sure the header size is correct.
		const unsigned int expected_size = 8 + gbix_len + sizeof(PVR_Header);
		if (info->header.size < expected_size) {
			// Header size is too small.
			return -1;
		}

		pvrHeader = reinterpret_cast<const PVR_Header*>(&info->header.pData[8+gbix_len]);
	} else {
		// No GBIX header.
		pvrHeader = reinterpret_cast<const PVR_Header*>(info->header.pData);
	}

	// Check the PVR header magic.
	if (pvrHeader->magic == cpu_to_be32(PVR_MAGIC_PVRT)) {
		// Sega Dreamcast PVR.
		return SegaPVRPrivate::PVR_TYPE_PVR;
	} else if (pvrHeader->magic == cpu_to_be32(PVR_MAGIC_GVRT)) {
		// GameCube GVR.
		return SegaPVRPrivate::PVR_TYPE_GVR;
	} else if (pvrHeader->magic == cpu_to_be32(PVR_MAGIC_PVRX)) {
		// Xbox PVRX.
		return SegaPVRPrivate::PVR_TYPE_PVRX;
	}

	// Not supported.
	return -1;
}

/**
 * Get the name of the system the loaded ROM is designed for.
 * @param type System name type. (See the SystemName enum.)
 * @return System name, or nullptr if type is invalid.
 */
const char *SegaPVR::systemName(unsigned int type) const
{
	RP_D(const SegaPVR);
	if (!d->isValid || !isSystemNameTypeValid(type))
		return nullptr;

	// PVR has the same name worldwide, so we can
	// ignore the region selection.
	static_assert(SYSNAME_TYPE_MASK == 3,
		"SegaPVR::systemName() array index optimization needs to be updated.");
	type &= SYSNAME_TYPE_MASK;

	if (d->isSvrTexture()) {
		// SVR texture. This is for PlayStation 2.
		static const char *const sysNames_SVR[4] = {
			"Sega SVR for PlayStation 2", "Sega SVR", "SVR", nullptr
		};
		return sysNames_SVR[type];
	}

	static const char *const sysNames[12] = {
		"Sega Dreamcast PVR", "Sega PVR", "PVR", nullptr,
		"Sega GVR for GameCube", "Sega GVR", "GVR", nullptr,
		"Sega PVRX for Xbox", "Sega PVRX", "PVRX", nullptr,
	};

	unsigned int idx = (d->pvrType << 2) | type;
	if (idx >= ARRAY_SIZE(sysNames)) {
		// Invalid index...
		idx &= SYSNAME_TYPE_MASK;
	}

	return sysNames[idx];
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
const char *const *SegaPVR::supportedFileExtensions_static(void)
{
	static const char *const exts[] = {
		".pvr",	// Sega Dreamcast PVR
		".gvr",	// GameCube GVR

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
const char *const *SegaPVR::supportedMimeTypes_static(void)
{
	static const char *const mimeTypes[] = {
		// Unofficial MIME types.
		// TODO: Get these upstreamed on FreeDesktop.org.
		"image/x-sega-pvr",
		"image/x-sega-gvr",
		"image/x-sega-pvrx",
		"image/x-sega-svr",

		nullptr
	};
	return mimeTypes;
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
 * Get a list of all available image sizes for the specified image type.
 * @param imageType Image type.
 * @return Vector of available image sizes, or empty vector if no images are available.
 */
vector<RomData::ImageSizeDef> SegaPVR::supportedImageSizes(ImageType imageType) const
{
	ASSERT_supportedImageSizes(imageType);

	RP_D(const SegaPVR);
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
	ASSERT_imgpf(imageType);

	RP_D(const SegaPVR);
	if (imageType != IMG_INT_IMAGE) {
		// Only IMG_INT_IMAGE is supported by PVR.
		return 0;
	}

	// If both dimensions of the texture are 64 or less,
	// specify nearest-neighbor scaling.
	uint32_t ret = 0;
	if (d->pvrHeader.width <= 64 && d->pvrHeader.height <= 64) {
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
	if (!d->fields->empty()) {
		// Field data *has* been loaded...
		return 0;
	} else if (!d->file) {
		// File isn't open.
		return -EBADF;
	} else if (!d->isValid || d->pvrType < 0) {
		// Unknown PVR image type.
		return -EIO;
	}

	// PVR header.
	const PVR_Header *const pvrHeader = &d->pvrHeader;
	d->fields->reserve(4);	// Maximum of 4 fields.

	// Texture size.
	d->fields->addField_dimensions(C_("SegaPVR", "Texture Size"),
		pvrHeader->width, pvrHeader->height);

	// Pixel format.
	static const char *const pxfmt_tbl_pvr[] = {
		// Sega Dreamcast (PVR)
		"ARGB1555", "RGB565",		// 0x00-0x01
		"ARGB4444", "YUV422",		// 0x02-0x03
		"BUMP", "4-bit per pixel",	// 0x04-0x05
		"8-bit per pixel", nullptr,	// 0x06-0x07

		// Sony PlayStation 2 (SVR)
		"RGB5A3", "ARGB8888",		// 0x08-0x09
	};
	static const char *const pxfmt_tbl_gvr[] = {
		// GameCube (GVR)
		"IA8", "RGB565", "RGB5A3",	// 0x00-0x02
	};
	static const char *const pxfmt_tbl_pvrx[] = {
		// Xbox (PVRX) (TODO)
		nullptr,
	};

	static const char *const *const pxfmt_tbl_ptrs[SegaPVRPrivate::PVR_TYPE_MAX] = {
		pxfmt_tbl_pvr,
		pxfmt_tbl_gvr,
		pxfmt_tbl_pvrx,
	};
	static const uint8_t pxfmt_tbl_sizes[SegaPVRPrivate::PVR_TYPE_MAX] = {
		static_cast<uint8_t>(ARRAY_SIZE(pxfmt_tbl_pvr)),
		static_cast<uint8_t>(ARRAY_SIZE(pxfmt_tbl_gvr)),
		static_cast<uint8_t>(ARRAY_SIZE(pxfmt_tbl_pvrx)),
	};

	// Image data type.
	static const char *const idt_tbl_pvr[] = {
		// Sega Dreamcast (PVR)
		nullptr,				// 0x00
		"Square (Twiddled)",			// 0x01
		"Square (Twiddled, Mipmap)",		// 0x02
		"Vector Quantized",			// 0x03
		"Vector Quantized (Mipmap)",		// 0x04
		"8-bit Paletted (Twiddled)",		// 0x05
		"4-bit Paletted (Twiddled)",		// 0x06
		"8-bit (Twiddled)",			// 0x07
		"4-bit (Twiddled)",			// 0x08
		"Rectangle",				// 0x09
		nullptr,				// 0x0A
		"Rectangle (Stride)",			// 0x0B
		nullptr,				// 0x0C
		"Rectangle (Twiddled)",			// 0x0D
		nullptr,				// 0x0E
		nullptr,				// 0x0F
		"Small VQ",				// 0x10
		"Small VQ (Mipmap)",			// 0x11
		"Square (Twiddled, Mipmap) (Alt)",	// 0x12
	};
	static const char *const idt_tbl_svr[] = {
		// Sony PlayStation 2 (SVR)
		// NOTE: First index represents format 0x60.
		"Rectangle",			// 0x60
		nullptr,			// 0x61
		"8-bit (external palette)",	// 0x62
		nullptr,			// 0x63
		"8-bit (external palette)",	// 0x64
		nullptr,			// 0x65
		"4-bit (RGB5A3), Rectangle",	// 0x66
		"4-bit (RGB5A3), Square",	// 0x67
		"4-bit (ARGB8), Rectangle",	// 0x68
		"4-bit (ARGB8), Square",	// 0x69
		"8-bit (RGB5A3), Rectangle",	// 0x6A
		"8-bit (RGB5A3), Square",	// 0x6B
		"8-bit (ARGB8), Rectangle",	// 0x6C
		"8-bit (ARGB8), Square",	// 0x6D
	};
	static const char *const idt_tbl_gvr[] = {
		// GameCube (GVR)
		"I4",			// 0x00
		"I8",			// 0x01
		"IA4",			// 0x02
		"IA8",			// 0x03
		"RGB565",		// 0x04
		"RGB5A3",		// 0x05
		"ARGB8888",		// 0x06
		nullptr,		// 0x07
		"CI4",			// 0x08
		"CI8",			// 0x09
		nullptr, nullptr,	// 0x0A,0x0B
		nullptr, nullptr,	// 0x0C,0x0D
		"DXT1",			// 0x0E
	};
	static const char *const idt_tbl_pvrx[] = {
		// Xbox (PVRX) (TODO)
		nullptr
	};

	static const char *const *const idt_tbl_ptrs[SegaPVRPrivate::PVR_TYPE_MAX] = {
		idt_tbl_pvr,
		idt_tbl_gvr,
		idt_tbl_pvrx,
	};
	static const uint8_t idt_tbl_sizes[SegaPVRPrivate::PVR_TYPE_MAX] = {
		static_cast<uint8_t>(ARRAY_SIZE(idt_tbl_pvr)),
		static_cast<uint8_t>(ARRAY_SIZE(idt_tbl_gvr)),
		static_cast<uint8_t>(ARRAY_SIZE(idt_tbl_pvrx)),
	};

	// GVR has these values located at a different offset.
	// TODO: Verify PVRX.
	uint8_t px_format, img_data_type;
	switch (d->pvrType) {
		default:
		case SegaPVRPrivate::PVR_TYPE_PVR:
		case SegaPVRPrivate::PVR_TYPE_PVRX:	// TODO
			px_format = pvrHeader->pvr.px_format;
			img_data_type = pvrHeader->pvr.img_data_type;
			break;
		case SegaPVRPrivate::PVR_TYPE_GVR:
			px_format = pvrHeader->gvr.px_format;
			img_data_type = pvrHeader->gvr.img_data_type;
			break;
	}

	const char *pxfmt = nullptr;
	const char *idt = nullptr;
	if (d->pvrType >= 0 && d->pvrType < SegaPVRPrivate::PVR_TYPE_MAX) {
		const char *const *const p_pxfmt_tbl = pxfmt_tbl_ptrs[d->pvrType];
		const uint8_t pxfmt_tbl_sz = pxfmt_tbl_sizes[d->pvrType];
		const char *const *const p_idt_tbl = idt_tbl_ptrs[d->pvrType];
		const uint8_t idt_tbl_sz = idt_tbl_sizes[d->pvrType];

		if (px_format < pxfmt_tbl_sz) {
			pxfmt = p_pxfmt_tbl[px_format];
		}

		if (d->pvrType == SegaPVRPrivate::PVR_TYPE_PVR &&
		    img_data_type >= SVR_IMG_MIN && img_data_type <= SVR_IMG_MAX)
		{
			// SVR image data type.
			idt = idt_tbl_svr[img_data_type - SVR_IMG_MIN];
		} else {
			// Other image data type.
			if (img_data_type < idt_tbl_sz) {
				idt = p_idt_tbl[img_data_type];
			}
		}
	}

	// NOTE: Pixel Format is not valid for GVR.
	const char *const pixel_format_title = C_("SegaPVR", "Pixel Format");
	const bool hasPxFmt = (d->pvrType != SegaPVRPrivate::PVR_TYPE_GVR);
	if (hasPxFmt) {
		if (pxfmt) {
			d->fields->addField_string(pixel_format_title, pxfmt);
		} else {
			d->fields->addField_string(pixel_format_title,
				rp_sprintf(C_("RomData", "Unknown (0x%02X)"), px_format));
		}
	}

	const char *const image_data_type_title = C_("SegaPVR", "Image Data Type");
	if (idt) {
		d->fields->addField_string(image_data_type_title, idt);
	} else {
		d->fields->addField_string(image_data_type_title,
			rp_sprintf(C_("RomData", "Unknown (0x%02X)"), img_data_type));
	}

	// Global index (if present).
	if (d->gbix_len > 0) {
		d->fields->addField_string_numeric(C_("SegaPVR", "Global Index"),
			d->gbix, RomFields::FB_DEC, 0);
	}

	// Finished reading the field data.
	return static_cast<int>(d->fields->count());
}

/**
 * Load metadata properties.
 * Called by RomData::metaData() if the field data hasn't been loaded yet.
 * @return Number of metadata properties read on success; negative POSIX error code on error.
 */
int SegaPVR::loadMetaData(void)
{
	RP_D(SegaPVR);
	if (d->metaData != nullptr) {
		// Metadata *has* been loaded...
		return 0;
	} else if (!d->file) {
		// File isn't open.
		return -EBADF;
	} else if (!d->isValid || d->pvrType < 0) {
		// Unknown PVR image type.
		return -EIO;
	}

	// Create the metadata object.
	d->metaData = new RomMetaData();
	d->metaData->reserve(2);	// Maximum of 2 metadata properties.

	// PVR header.
	const PVR_Header *const pvrHeader = &d->pvrHeader;

	// Dimensions.
	// TODO: Don't add height for 1D textures?
	d->metaData->addMetaData_integer(Property::Width, pvrHeader->width);
	d->metaData->addMetaData_integer(Property::Height, pvrHeader->height);

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
int SegaPVR::loadInternalImage(ImageType imageType, const rp_image **pImage)
{
	ASSERT_loadInternalImage(imageType, pImage);

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
