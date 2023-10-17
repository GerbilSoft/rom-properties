/***************************************************************************
 * ROM Properties Page shell extension. (librptexture)                     *
 * SegaPVR.cpp: Sega PVR texture reader.                                   *
 *                                                                         *
 * Copyright (c) 2017-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "SegaPVR.hpp"
#include "FileFormat_p.hpp"

#include "pvr_structs.h"

// Other rom-properties libraries
#include "libi18n/i18n.h"
using namespace LibRpFile;
using LibRpBase::RomFields;

// librptexture
#include "img/rp_image.hpp"
#include "decoder/ImageDecoder_Linear.hpp"
#include "decoder/ImageDecoder_DC.hpp"
#include "decoder/ImageDecoder_GCN.hpp"
#include "decoder/ImageDecoder_S3TC.hpp"

// C++ STL classes
using std::unique_ptr;

namespace LibRpTexture {

class SegaPVRPrivate final : public FileFormatPrivate
{
	public:
		SegaPVRPrivate(SegaPVR *q, const IRpFilePtr &file);
		~SegaPVRPrivate() final = default;

	private:
		typedef FileFormatPrivate super;
		RP_DISABLE_COPY(SegaPVRPrivate)

	public:
		/** TextureInfo **/
		static const char *const exts[];
		static const char *const mimeTypes[];
		static const TextureInfo textureInfo;

	public:
		enum class PVRType {
			Unknown = -1,

			PVR	= 0,	// Dreamcast PVR
			GVR	= 1,	// GameCube GVR
			SVR	= 2,	// PlayStation 2 SVR
			PVRX	= 3,	// Xbox PVRX (TODO)

			Max
		};
		PVRType pvrType;

	public:
		// PVR header
		PVR_Header pvrHeader;

#if SYS_BYTEORDER == SYS_LIL_ENDIAN
		static inline void byteswap_pvr(PVR_Header *pvr) {
			RP_UNUSED(pvr);
		}
		static inline void byteswap_gvr(PVR_Header *gvr);
#else /* SYS_BYTEORDER == SYS_BIG_ENDIAN */
		static inline void byteswap_pvr(PVR_Header *pvr);
		static inline void byteswap_gvr(PVR_Header *gvr) {
			RP_UNUSED(gvr);
		}
#endif

		// Global Index
		// gbix_len is 0 if it's not present.
		// Otherwise, may be 16 (common) or 12 (uncommon).
		unsigned int gbix_len;
		uint32_t gbix;

		// Decoded image
		rp_image_ptr img;

		// Invalid pixel format message
		char invalid_pixel_format[24];

	public:
		/**
		 * Get the pixel format name.
		 * @return Pixel format name, or nullptr if unknown.
		 */
		const char *pixelFormatName(void) const;

		/**
		 * Get the image data type name.
		 * @return Image data type name, or nullptr if unknown.
		 */
		const char *imageDataTypeName(void) const;

	public:
		/**
		 * Load the PVR/SVR image.
		 * @return Image, or nullptr on error.
		 */
		rp_image_const_ptr loadPvrImage(void);

		/**
		 * Load the GVR image.
		 * @return Image, or nullptr on error.
		 */
		rp_image_const_ptr loadGvrImage(void);

		/**
		 * Unswizzle a 4-bit or 8-bit SVR texture.
		 * All 4-bit and 8-bit SVR textures >=128x128 are swizzled.
		 * @param img_swz Swizzled 4-bit or 8-bit SVR texture.
		 * @return Unswizzled 4-bit or 8-bit SVR texture, or nullptr on error.
		 */
		static rp_image_ptr svr_unswizzle_4or8(const rp_image_const_ptr &img_swz);

		/**
		 * Unswizzle a 16-bit SVR texture.
		 * NOTE: The rp_image must have been converted to ARGB32 format.
		 * @param img_swz Swizzled 16-bit SVR texture.
		 * @return Unswizzled 16-bit SVR texture, or nullptr on error.
		 */
		static rp_image_ptr svr_unswizzle_16(const rp_image_const_ptr &img_swz);
};

FILEFORMAT_IMPL(SegaPVR)

/** SegaPVRPrivate **/

/* TextureInfo */
const char *const SegaPVRPrivate::exts[] = {
	".pvr",	// Sega Dreamcast PVR
	".gvr",	// GameCube GVR
	".svr",	// PlayStation 2 SVR

	nullptr
};
const char *const SegaPVRPrivate::mimeTypes[] = {
	// NOTE: Ordering matches PVRType.

	// Unofficial MIME types.
	// TODO: Get these upstreamed on FreeDesktop.org.
	"image/x-sega-pvr",
	"image/x-sega-gvr",
	"image/x-sega-svr",
	"image/x-sega-pvrx",

	nullptr
};
const TextureInfo SegaPVRPrivate::textureInfo = {
	exts, mimeTypes
};

SegaPVRPrivate::SegaPVRPrivate(SegaPVR *q, const IRpFilePtr &file)
	: super(q, file, &textureInfo)
	, pvrType(PVRType::Unknown)
	, gbix_len(0)
	, gbix(0)
	, img(nullptr)
{
	// Clear the PVR header structs.
	memset(&pvrHeader, 0, sizeof(pvrHeader));
	memset(invalid_pixel_format, 0, sizeof(invalid_pixel_format));
}

#if SYS_BYTEORDER == SYS_BIG_ENDIAN
/**
 * Byteswap a PVR/SVR/PVRX header to host-endian.
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
 * Get the pixel format name.
 * @return Pixel format name, or nullptr if unknown.
 */
const char *SegaPVRPrivate::pixelFormatName(void) const
{
	static const char *const pxfmt_tbl_pvr[] = {
		// Sega Dreamcast (PVR)
		"ARGB1555", "RGB565",		// 0x00-0x01
		"ARGB4444", "YUV422",		// 0x02-0x03
		"BUMP", "4-bit per pixel",	// 0x04-0x05
		"8-bit per pixel", nullptr,	// 0x06-0x07

		// Sony PlayStation 2 (SVR)
		"BGR5A3", "BGR888_ABGR7888",	// 0x08-0x09
	};
	static const char *const pxfmt_tbl_gvr[] = {
		// GameCube (GVR)
		"IA8", "RGB565", "RGB5A3",	// 0x00-0x02
	};
	static const char *const pxfmt_tbl_pvrx[] = {
		// Xbox (PVRX) (TODO)
		nullptr,
	};

	static const char *const *const pxfmt_tbl_ptrs[(int)PVRType::Max] = {
		pxfmt_tbl_pvr,
		pxfmt_tbl_gvr,
		pxfmt_tbl_pvr,	// SVR
		pxfmt_tbl_pvrx,
	};
	static const uint8_t pxfmt_tbl_sizes[(int)PVRType::Max] = {
		static_cast<uint8_t>(ARRAY_SIZE(pxfmt_tbl_pvr)),
		static_cast<uint8_t>(ARRAY_SIZE(pxfmt_tbl_gvr)),
		static_cast<uint8_t>(ARRAY_SIZE(pxfmt_tbl_pvr)),	// SVR
		static_cast<uint8_t>(ARRAY_SIZE(pxfmt_tbl_pvrx)),
	};

	if ((int)pvrType < 0 || pvrType >= PVRType::Max) {
		// Invalid PVR type.
		return nullptr;
	}

	// GVR has pixel format and image data type located at a different offset.
	const uint8_t px_format = (pvrType == PVRType::GVR)
		? pvrHeader.gvr.px_format
		: pvrHeader.pvr.px_format;

	// NOTE: GameCube pxfmt makes no sense.
	// Maybe we should use the image data type instead?
	// For now, we'll end up returning nullptr here.
	const char *const *const p_pxfmt_tbl = pxfmt_tbl_ptrs[(int)pvrType];
	const uint8_t pxfmt_tbl_sz = pxfmt_tbl_sizes[(int)pvrType];

	if (px_format < pxfmt_tbl_sz) {
		return p_pxfmt_tbl[px_format];
	}

	// Unknown image data type
	return nullptr;
}

/**
 * Get the image data type name.
 * @return Image data type name, or nullptr if unknown.
 */
const char *SegaPVRPrivate::imageDataTypeName(void) const
{
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
		"Rectangle (Swizzled)",		// 0x61
		"8-bit (external palette)",	// 0x62
		nullptr,			// 0x63
		"8-bit (external palette)",	// 0x64
		nullptr,			// 0x65
		"4-bit (BGR5A3), Rectangle",	// 0x66
		"4-bit (BGR5A3), Square",	// 0x67
		"4-bit (ABGR8), Rectangle",	// 0x68
		"4-bit (ABGR8), Square",	// 0x69
		"8-bit (BGR5A3), Rectangle",	// 0x6A
		"8-bit (BGR5A3), Square",	// 0x6B
		"8-bit (ABGR8), Rectangle",	// 0x6C
		"8-bit (ABGR8), Square",	// 0x6D
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

	static const char *const *const idt_tbl_ptrs[(int)PVRType::Max] = {
		idt_tbl_pvr,
		idt_tbl_gvr,
		idt_tbl_svr,
		idt_tbl_pvrx,
	};
	static const uint8_t idt_tbl_sizes[(int)PVRType::Max] = {
		static_cast<uint8_t>(ARRAY_SIZE(idt_tbl_pvr)),
		static_cast<uint8_t>(ARRAY_SIZE(idt_tbl_gvr)),
		static_cast<uint8_t>(ARRAY_SIZE(idt_tbl_svr)),
		static_cast<uint8_t>(ARRAY_SIZE(idt_tbl_pvrx)),
	};

	if ((int)pvrType < 0 || pvrType >= PVRType::Max) {
		// Invalid PVR type.
		return nullptr;
	}

	// GVR has these values located at a different offset.
	// TODO: Verify PVRX.
	uint8_t img_data_type;
	switch (pvrType) {
		default:
		case SegaPVRPrivate::PVRType::PVR:
		case SegaPVRPrivate::PVRType::SVR:
		case SegaPVRPrivate::PVRType::PVRX:	// TODO
			img_data_type = pvrHeader.pvr.img_data_type;
			break;
		case SegaPVRPrivate::PVRType::GVR:
			img_data_type = pvrHeader.gvr.img_data_type;
			break;
	}

	// NOTE: For GameCube, this is essentially the pixel format.
	const char *const *const p_idt_tbl = idt_tbl_ptrs[(int)pvrType];
	const uint8_t idt_tbl_sz = idt_tbl_sizes[(int)pvrType];

	if (pvrType == SegaPVRPrivate::PVRType::SVR) {
		// SVR image data type
		if (img_data_type >= SVR_IMG_MIN && img_data_type <= SVR_IMG_MAX) {
			return idt_tbl_svr[img_data_type - SVR_IMG_MIN];
		}
	} else {
		// Other image data type
		if (img_data_type < idt_tbl_sz) {
			return p_idt_tbl[img_data_type];
		}
	}

	// Unknown image data type
	return nullptr;
}

/**
 * Load the PVR/SVR image.
 * @return Image, or nullptr on error.
 */
rp_image_const_ptr SegaPVRPrivate::loadPvrImage(void)
{
	if (img) {
		// Image has already been loaded.
		return img;
	} else if (!file || !file->isOpen()) {
		// File isn't open.
		return nullptr;
	} else if (pvrType != PVRType::PVR && pvrType != PVRType::SVR) {
		// Wrong PVR type for this function.
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
	size_t expected_size = 0;

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

			// Make sure this is in fact a square textuer.
			assert(pvrHeader.width == pvrHeader.height);
			if (pvrHeader.width != pvrHeader.height) {
				return nullptr;
			}

			// Make sure the texture width is a power of two.
			assert(isPow2(pvrHeader.width));
			if (!isPow2(pvrHeader.width)) {
				return nullptr;
			}

			// Get the log2 of the texture width.
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

	// SVR palette buffer
	rp::uvector<uint8_t> svr_pal_buf;

	// Determine the image size.
	switch (pvrHeader.pvr.img_data_type) {
		case PVR_IMG_SQUARE_TWIDDLED_MIPMAP:
		case PVR_IMG_SQUARE_TWIDDLED_MIPMAP_ALT:
		case PVR_IMG_SQUARE_TWIDDLED:
		case PVR_IMG_RECTANGLE:
		case SVR_IMG_RECTANGLE:
		case SVR_IMG_RECTANGLE_SWIZZLED:
			switch (pvrHeader.pvr.px_format) {
				case PVR_PX_ARGB1555:
				case PVR_PX_RGB565:
				case PVR_PX_ARGB4444:
				case SVR_PX_BGR5A3:
					expected_size = ((pvrHeader.width * pvrHeader.height) * 2);
					break;

				case SVR_PX_BGR888_ABGR7888:
					expected_size = ((pvrHeader.width * pvrHeader.height) * 4);
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
				ImageDecoder::calcDreamcastSmallVQPaletteEntries_NoMipmaps(pvrHeader.width) * 2;
			expected_size = pal_siz + ((pvrHeader.width * pvrHeader.height) / 4);
			break;
		}

		case PVR_IMG_SMALL_VQ_MIPMAP: {
			// Small VQ images have up to 1024 palette entries based on width,
			// and the image data is 2bpp.
			// Skip the palette, since that's handled later.
			const unsigned int pal_siz =
				ImageDecoder::calcDreamcastSmallVQPaletteEntries_WithMipmaps(pvrHeader.width) * 2;
			svr_pal_buf.resize(pal_siz);
			mipmap_size += pal_siz;
			expected_size = ((pvrHeader.width * pvrHeader.height) / 4);
			break;
		}

		case SVR_IMG_INDEX4_BGR5A3_RECTANGLE:
		case SVR_IMG_INDEX4_BGR5A3_SQUARE:
		case SVR_IMG_INDEX4_ABGR8_RECTANGLE:
		case SVR_IMG_INDEX4_ABGR8_SQUARE: {
			// 16-color palette is located at the beginning of the data.
			// TODO: Require SQUARE to have identical width/height?

			// NOTE: Puyo Tools sometimes uses the wrong image data type
			// for the palette format. Use pixel format instead.
			unsigned int pal_siz;
			switch (pvrHeader.pvr.px_format) {
				case SVR_PX_BGR5A3:
					pal_siz = 16 * 2;
					break;
				case SVR_PX_BGR888_ABGR7888:
					pal_siz = 16 * 4;
					break;
				default:
					assert(!"Unsupported pixel format for SVR.");
					return nullptr;
			}
			svr_pal_buf.resize(pal_siz);
			mipmap_size = pal_siz;
			expected_size = ((pvrHeader.width * pvrHeader.height) / 2);
			break;
		}

		case SVR_IMG_INDEX8_BGR5A3_RECTANGLE:
		case SVR_IMG_INDEX8_BGR5A3_SQUARE:
		case SVR_IMG_INDEX8_ABGR8_RECTANGLE:
		case SVR_IMG_INDEX8_ABGR8_SQUARE: {
			// 256-color palette is located at the beginning of the data.
			// TODO: Require SQUARE to have identical width/height?

			// NOTE: Puyo Tools sometimes uses the wrong image data type
			// for the palette format. Use pixel format instead.
			unsigned int pal_siz;
			switch (pvrHeader.pvr.px_format) {
				case SVR_PX_BGR5A3:
					pal_siz = 256 * 2;
					break;
				case SVR_PX_BGR888_ABGR7888:
					pal_siz = 256 * 4;
					break;
				default:
					assert(!"Unsupported pixel format for SVR.");
					return nullptr;
			}
			svr_pal_buf.resize(pal_siz);
			mipmap_size = pal_siz;
			expected_size = (pvrHeader.width * pvrHeader.height);
			break;
		}

		default:
			// TODO: Other formats.
			return nullptr;
	}

	if ((pvrDataStart + mipmap_size + expected_size) > file_sz) {
		// File is too small.
#ifdef _DEBUG
		if (pvrHeader.pvr.img_data_type == PVR_IMG_SMALL_VQ ||
		    pvrHeader.pvr.img_data_type == PVR_IMG_SMALL_VQ_MIPMAP)
		{
			assert(!"PVR Small VQ file is too small.");
		}
#endif /* _DEBUG */
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
	bool is32bit = false;
	switch (pvrHeader.pvr.px_format) {
		case PVR_PX_ARGB1555:
			px_format = ImageDecoder::PixelFormat::ARGB1555;
			break;
		case PVR_PX_RGB565:
			px_format = ImageDecoder::PixelFormat::RGB565;
			break;
		case PVR_PX_ARGB4444:
			px_format = ImageDecoder::PixelFormat::ARGB4444;
			break;
		case SVR_PX_BGR5A3:
			// TODO: Verify that this works for SVR.
			px_format = ImageDecoder::PixelFormat::BGR5A3;
			break;
		case SVR_PX_BGR888_ABGR7888:
			px_format = ImageDecoder::PixelFormat::BGR888_ABGR7888;
			is32bit = true;
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
				reinterpret_cast<const uint16_t*>(buf.get()), expected_size);
			break;

		case PVR_IMG_RECTANGLE:
		case SVR_IMG_RECTANGLE:
		case SVR_IMG_RECTANGLE_SWIZZLED:
			if (is32bit) {
				img = ImageDecoder::fromLinear32(px_format,
					pvrHeader.width, pvrHeader.height,
					reinterpret_cast<const uint32_t*>(buf.get()), expected_size);
			} else {
				img = ImageDecoder::fromLinear16(px_format,
					pvrHeader.width, pvrHeader.height,
					reinterpret_cast<uint16_t*>(buf.get()), expected_size);
			}

			if (pvrHeader.pvr.img_data_type == SVR_IMG_RECTANGLE_SWIZZLED) {
				// If RGB5A3 and >=64x64, this texture is probably swizzled.
				if (pvrHeader.pvr.px_format == SVR_PX_BGR5A3 &&
				    pvrHeader.width >= 64 && pvrHeader.height >= 64)
				{
					// Need to unswizzle the texture.
					const rp_image_ptr img_unswz = svr_unswizzle_16(img);
					if (img_unswz) {
						img = img_unswz;
					}
				}
				break;
			}
			break;

		case PVR_IMG_VQ: {
			// VQ images have a 1024-entry palette.
			static const size_t pal_siz = 1024U * 2;
			const uint16_t *const pal_buf = reinterpret_cast<const uint16_t*>(buf.get());
			const uint8_t *const img_buf = buf.get() + pal_siz;
			const size_t img_siz = expected_size - pal_siz;

			img = ImageDecoder::fromDreamcastVQ16(px_format,
				false, false,
				pvrHeader.width, pvrHeader.height,
				img_buf, img_siz, pal_buf, pal_siz);
			break;
		}

		case PVR_IMG_VQ_MIPMAP: {
			// VQ images have a 1024-entry palette.
			// This is stored before the mipmaps, so we need to read it manually.
			static const unsigned int pal_siz = 1024*2;
			unique_ptr<uint16_t[]> pal_buf(new uint16_t[pal_siz/2]);
			size = file->seekAndRead(pvrDataStart, pal_buf.get(), pal_siz);
			if (size != pal_siz) {
				// Read error.
				break;
			}

			img = ImageDecoder::fromDreamcastVQ16(px_format,
				false, true,
				pvrHeader.width, pvrHeader.height,
				buf.get(), expected_size, pal_buf.get(), pal_siz);
			break;
		}

		case PVR_IMG_SMALL_VQ: {
			// Small VQ images have up to 1024 palette entries based on width.
			const size_t pal_siz =
				ImageDecoder::calcDreamcastSmallVQPaletteEntries_NoMipmaps(pvrHeader.width) * 2;
			const uint16_t *const pal_buf = reinterpret_cast<const uint16_t*>(buf.get());
			const uint8_t *const img_buf = buf.get() + pal_siz;
			const size_t img_siz = expected_size - pal_siz;

			img = ImageDecoder::fromDreamcastVQ16(px_format,
				true, false,
				pvrHeader.width, pvrHeader.height,
				img_buf, img_siz, pal_buf, pal_siz);
			break;
		}

		case PVR_IMG_SMALL_VQ_MIPMAP: {
			// Small VQ images have up to 1024 palette entries based on width.
			// This is stored before the mipmaps, so we need to read it manually.
			size = file->seekAndRead(pvrDataStart, svr_pal_buf.data(), svr_pal_buf.size());
			if (size != svr_pal_buf.size()) {
				// Read error.
				break;
			}

			img = ImageDecoder::fromDreamcastVQ16(px_format,
				true, true,
				pvrHeader.width, pvrHeader.height,
				buf.get(), expected_size,
				reinterpret_cast<const uint16_t*>(svr_pal_buf.data()), (int)svr_pal_buf.size());
			break;
		}

		case SVR_IMG_INDEX4_BGR5A3_RECTANGLE:
		case SVR_IMG_INDEX4_BGR5A3_SQUARE:
		case SVR_IMG_INDEX4_ABGR8_RECTANGLE:
		case SVR_IMG_INDEX4_ABGR8_SQUARE: {
			assert(!svr_pal_buf.empty());
			if (svr_pal_buf.empty()) {
				// Invalid palette buffer size.
				return nullptr;
			}

			// Palette is located immediately after the PVR header.
			size = file->seekAndRead(pvrDataStart, svr_pal_buf.data(), svr_pal_buf.size());
			if (size != svr_pal_buf.size()) {
				// Seek and/or read error.
				return nullptr;
			}

			// FIXME: Puyo Tools has palette bit swapping in
			// swizzled textures, sort of like 8-bit textures.
			// Find a >=128x128 4-bit texture to test this with.

			// Least-significant nybble is first.
			img = ImageDecoder::fromLinearCI4(px_format, false,
				pvrHeader.width, pvrHeader.height,
				buf.get(), expected_size,
				svr_pal_buf.data(), (int)svr_pal_buf.size());

			// Puyo Tools: Minimum swizzle size for 4-bit is 128x128.
			if (pvrHeader.width >= 128 && pvrHeader.height >= 128) {
				// Need to unswizzle the texture.
				const rp_image_ptr img_unswz = svr_unswizzle_4or8(img);
				if (img_unswz) {
					img = img_unswz;
				}
			}
			break;
		}

		case SVR_IMG_INDEX8_BGR5A3_RECTANGLE:
		case SVR_IMG_INDEX8_BGR5A3_SQUARE:
		case SVR_IMG_INDEX8_ABGR8_RECTANGLE:
		case SVR_IMG_INDEX8_ABGR8_SQUARE: {
			assert(!svr_pal_buf.empty());
			if (svr_pal_buf.empty()) {
				// Invalid palette buffer size.
				return nullptr;
			}

			// Palette is located immediately after the PVR header.
			size = file->seekAndRead(pvrDataStart, svr_pal_buf.data(), svr_pal_buf.size());
			if (size != svr_pal_buf.size()) {
				// Seek and/or read error.
				return nullptr;
			}

			// NOTE: Bits 3 and 4 in each image data byte is swapped.
			// Why? Who the hell knows.

			// We need to swap the image data instead of the palette entries
			// in order to maintain the original palette ordering.
			// TODO: Expand to uint64_t and/or SSE2?
			assert(expected_size % 4 == 0);
			uint32_t *bits = reinterpret_cast<uint32_t*>(buf.get());
			uint32_t *const bits_end = bits + (expected_size / sizeof(uint32_t));
			for (; bits < bits_end; bits++) {
				const uint32_t sw = (*bits & 0xE7E7E7E7);
				const uint32_t b3 = (*bits & 0x10101010) >> 1;
				const uint32_t b4 = (*bits & 0x08080808) << 1;
				*bits = (sw | b3 | b4);
			}

			// Least-significant nybble is first.
			img = ImageDecoder::fromLinearCI8(px_format,
				pvrHeader.width, pvrHeader.height,
				buf.get(), expected_size,
				svr_pal_buf.data(), (int)svr_pal_buf.size());

			// Puyo Tools: Minimum swizzle size for 8-bit is 128x64.
			if (pvrHeader.width >= 128 && pvrHeader.height >= 64) {
				// Need to unswizzle the texture.
				const rp_image_ptr img_unswz = svr_unswizzle_4or8(img);
				if (img_unswz) {
					img = img_unswz;
				}
			}
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
rp_image_const_ptr SegaPVRPrivate::loadGvrImage(void)
{
	if (img) {
		// Image has already been loaded.
		return img;
	} else if (!this->file || this->pvrType != PVRType::GVR) {
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
	const unsigned int file_sz = static_cast<unsigned int>(file->size());

	const unsigned int pvrDataStart = gbix_len + sizeof(PVR_Header);
	unsigned int expected_size = ((unsigned int)pvrHeader.width * (unsigned int)pvrHeader.height);

	switch (pvrHeader.gvr.img_data_type) {
		case GVR_IMG_I4:
		case GVR_IMG_DXT1:
			// 4bpp
			expected_size /= 2;
			break;
		case GVR_IMG_I8:
		case GVR_IMG_IA4:
			// 8bpp; no adjustments needed.
			break;
		case GVR_IMG_IA8:
		case GVR_IMG_RGB565:
		case GVR_IMG_RGB5A3:
			// 16bpp
			expected_size *= 2;
			break;
		case GVR_IMG_ARGB8888:
			// 32bpp
			expected_size *= 4;
			break;

		case GVR_IMG_CI4:
			expected_size /= 2;
			//expected_size += 16;	// palette?
			break;
		case GVR_IMG_CI8:
			// 8bpp; no adjustments needed.
			break;

		default:
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
	auto buf = aligned_uptr<uint8_t>(16, expected_size);
	size_t size = file->read(buf.get(), expected_size);
	if (size != expected_size) {
		// Read error.
		return nullptr;
	}

	switch (pvrHeader.gvr.img_data_type) {
		case GVR_IMG_I8:
			// FIXME: Untested.
			img = ImageDecoder::fromGcnI8(
				pvrHeader.width, pvrHeader.height,
				buf.get(), expected_size);
			break;

		case GVR_IMG_IA8:
			// FIXME: Untested.
			img = ImageDecoder::fromGcn16(
				ImageDecoder::PixelFormat::IA8,
				pvrHeader.width, pvrHeader.height,
				reinterpret_cast<uint16_t*>(buf.get()), expected_size);
			break;

		case GVR_IMG_RGB565:
			// FIXME: Untested.
			img = ImageDecoder::fromGcn16(
				ImageDecoder::PixelFormat::RGB565,
				pvrHeader.width, pvrHeader.height,
				reinterpret_cast<uint16_t*>(buf.get()), expected_size);
			break;

		case GVR_IMG_RGB5A3:
			img = ImageDecoder::fromGcn16(
				ImageDecoder::PixelFormat::RGB5A3,
				pvrHeader.width, pvrHeader.height,
				reinterpret_cast<uint16_t*>(buf.get()), expected_size);
			break;

		case GVR_IMG_CI4: {
			// TODO: Figure out the palette location.
			// For now, use a grayscale RGB5A3 palette.
			uint16_t rgb5a3[16];
			for (unsigned int i = 0; i < 16; i++) {
				rgb5a3[i] = cpu_to_be16(0x8000 | (i*2) | ((i*2)<<5) | ((i*2)<<10));
			}
			img = ImageDecoder::fromGcnCI4(
				pvrHeader.width, pvrHeader.height,
				buf.get(), expected_size,
				rgb5a3, sizeof(rgb5a3));
			break;
		}

		case GVR_IMG_CI8: {
			// TODO: Figure out the palette location.
			// For now, use a grayscale RGB5A3 palette.
			uint16_t rgb5a3[256];
			for (unsigned int i = 0; i < 256; i++) {
				const unsigned int val = (i >> 3);
				rgb5a3[i] = cpu_to_be16(0x8000 | val | (val<<5) | (val<<10));
			}
			// FIXME: Untested.
			img = ImageDecoder::fromGcnCI8(
				pvrHeader.width, pvrHeader.height,
				buf.get(), expected_size,
				rgb5a3, sizeof(rgb5a3));
			break;
		}

		case GVR_IMG_DXT1:
			// TODO: Determine if color 3 should be black or transparent.
			img = ImageDecoder::fromDXT1_GCN(
				pvrHeader.width, pvrHeader.height,
				buf.get(), expected_size);
			break;

		default:
			// TODO: Other types.
			break;
	}

	return img;
}

/**
 * Unswizzle a 4-bit or 8-bit SVR texture.
 * All 4-bit and 8-bit SVR textures >=128x128 are swizzled.
 * @param img_swz Swizzled 4-bit or 8-bit SVR texture.
 * @return Unswizzled 4-bit or 8-bit SVR texture, or nullptr on error.
 */
rp_image_ptr SegaPVRPrivate::svr_unswizzle_4or8(const rp_image_const_ptr &img_swz)
{
	// TODO: Move to ImageDecoder if more PS2 formats are added.

	// NOTE: The original code is for 4-bit textures, but 8-bit
	// textures use the same algorithm. Since we've already
	// decoded the 4-bit pixels to 8-bit, we can use the same
	// function for both.

	// References:
	// - https://forum.xentax.com/viewtopic.php?f=18&t=3516
	// - https://gist.github.com/Fireboyd78/1546f5c86ebce52ce05e7837c697dc72

	// Original Delphi version by Dageron:
	// - https://gta.nick7.com/ps2/swizzling/unswizzle_delphi.txt

	static const uint8_t interlaceMatrix[] = {
		0x00, 0x10, 0x02, 0x12,
		0x11, 0x01, 0x13, 0x03,
	};
	static const int8_t matrix[] = {0, 1, -1, 0};
	static const int8_t tileMatrix[] = {4, -4};

	// Only CI8 formats are supported here.
	assert(img_swz != nullptr);
	assert(img_swz->isValid());
	assert(img_swz->format() == rp_image::Format::CI8);
	if (!img_swz || !img_swz->isValid() ||
	    img_swz->format() != rp_image::Format::CI8)
	{
		return nullptr;
	}

	const int width = img_swz->width();
	const int height = img_swz->height();

	// Texture dimensions must be a multiple of 4.
	assert(width % 4 == 0);
	assert(height % 4 == 0);
	if (width % 4 != 0 || height % 4 != 0) {
		// Unable to unswizzle this texture.
		return nullptr;
	}

	rp_image_ptr img = std::make_shared<rp_image>(width, height, img_swz->format());
	if (!img->isValid()) {
		// Could not allocate the image.
		return nullptr;
	}

	// Strides must be equal to the image width.
	assert(img_swz->stride() == width);
	assert(img->stride() == width);
	if (img_swz->stride() != width || img->stride() != width) {
		return nullptr;
	}

	// Copy the palette.
	const unsigned int palette_len = std::min(img_swz->palette_len(), img->palette_len());
	memcpy(img->palette(), img_swz->palette(), palette_len * sizeof(uint32_t));

	const uint8_t *src_pixels = static_cast<const uint8_t*>(img_swz->bits());
	for (int y = 0; y < height; y++) {
		const bool oddRow = (y & 1);
		const int num1 = (y / 4) & 1;
		const int num3 = (y % 4);
		const int yy = y + matrix[num3];

		int num7 = y * width;
		if (oddRow) {
			num7 -= width;
		}

		uint8_t *const destLine = static_cast<uint8_t*>(img->scanLine(yy));
		for (int x = 0; x < width; x++) {
			const int num2 = (x / 4) & 1;

			int num4 = ((x / 4) % 4);
			if (oddRow) {
				num4 += 4;
			}

			const int num5 = ((x * 4) % 16);
			const int num6 = ((x / 16) * 32);

			const int xx = x + num1 * tileMatrix[num2];

			const int i = interlaceMatrix[num4] + num5 + num6 + num7;

			destLine[xx] = src_pixels[i];
		}
	}

	return img;
}

/**
 * Unswizzle a 16-bit SVR texture.
 * NOTE: The rp_image must have been converted to ARGB32 format.
 * @param img_swz Swizzled 16-bit SVR texture.
 * @return Unswizzled 16-bit SVR texture, or nullptr on error.
 */
rp_image_ptr SegaPVRPrivate::svr_unswizzle_16(const rp_image_const_ptr &img_swz)
{
	// TODO: Move to ImageDecoder if more PS2 formats are added.

	// FIXME: This code is *wrong*, but it's better than leaving it
	// completely unswizzled...

	// References:
	// - https://forum.xentax.com/viewtopic.php?f=18&t=3516
	// - https://gist.github.com/Fireboyd78/1546f5c86ebce52ce05e7837c697dc72

	// Original Delphi version by Dageron:
	// - https://gta.nick7.com/ps2/swizzling/unswizzle_delphi.txt

	static const uint8_t interlaceMatrix[] = {
		0x00, 0x10, 0x02, 0x12,
		0x11, 0x01, 0x13, 0x03,
	};
	static const int8_t matrix[] = {0, 1, -1, 0};
	static const int8_t tileMatrix[] = {4, -4};

	// Only ARGB32 formats are supported here.
	assert(img_swz != nullptr);
	assert(img_swz->isValid());
	assert(img_swz->format() == rp_image::Format::ARGB32);
	if (!img_swz || !img_swz->isValid() ||
	    img_swz->format() != rp_image::Format::ARGB32)
	{
		return nullptr;
	}

	const int width = img_swz->width();
	const int height = img_swz->height();

	// Texture dimensions must be a multiple of 4.
	assert(width % 4 == 0);
	assert(height % 4 == 0);
	if (width % 4 != 0 || height % 4 != 0) {
		// Unable to unswizzle this texture.
		return nullptr;
	}

	rp_image_ptr img = std::make_shared<rp_image>(width, height, img_swz->format());
	if (!img->isValid()) {
		// Could not allocate the image.
		return nullptr;
	}

	// Strides must be equal to the image width.
	assert(img_swz->stride()/(int)sizeof(uint32_t) == width);
	assert(img->stride()/(int)sizeof(uint32_t) == width);
	if (img_swz->stride()/(int)sizeof(uint32_t) != width ||
	    img->stride()/(int)sizeof(uint32_t) != width)
	{
		return nullptr;
	}

	const uint32_t *src_pixels = static_cast<const uint32_t*>(img_swz->bits());
	for (int y = 0; y < height; y++) {
		const bool oddRow = (y & 1);
		const int num1 = (y / 4) & 1;
		const int num3 = (y % 4);
		const int yy = y + matrix[num3];

		int num7 = y * width;
		if (oddRow) {
			num7 -= width;
		}

		uint32_t *const destLine = static_cast<uint32_t*>(img->scanLine(yy));
		for (int x = 0; x < width; x++) {
			const int num2 = (x / 4) & 1;

			int num4 = ((x / 4) % 4);
			if (oddRow) {
				num4 += 4;
			}

			const int num5 = ((x * 4) % 16);
			const int num6 = ((x / 16) * 32);

			const int xx = x + num1 * tileMatrix[num2];

			const int i = interlaceMatrix[num4] + num5 + num6 + num7;

			destLine[xx] = src_pixels[i];
		}
	}

	return img;
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
SegaPVR::SegaPVR(const IRpFilePtr &file)
	: super(new SegaPVRPrivate(this, file))
{
	RP_D(SegaPVR);

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
		d->file.reset();
		return;
	}

	// Check if this PVR image is supported.
	const DetectInfo info = {
		{0, static_cast<uint32_t>(sz_header), header},
		nullptr,	// ext (not needed for SegaPVR)
		file->size()	// szFile
	};
	d->pvrType = static_cast<SegaPVRPrivate::PVRType>(isRomSupported_static(&info));
	d->isValid = ((int)d->pvrType >= 0);

	if (!d->isValid) {
		d->file.reset();
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

		if (d->pvrType == SegaPVRPrivate::PVRType::GVR) {
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
			d->pvrType = SegaPVRPrivate::PVRType::Unknown;
			d->isValid = false;
			d->file.reset();
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
		case SegaPVRPrivate::PVRType::PVR:
		case SegaPVRPrivate::PVRType::SVR:
		case SegaPVRPrivate::PVRType::PVRX:
			// Little-endian.
			d->byteswap_pvr(&d->pvrHeader);
			break;

		case SegaPVRPrivate::PVRType::GVR:
			// Big-endian.
			d->byteswap_gvr(&d->pvrHeader);
			break;

		default:
			// Should not get here...
			assert(!"Invalid PVR type.");
			d->pvrType = SegaPVRPrivate::PVRType::Unknown;
			d->isValid = false;
			d->file.reset();
			return;
	}

	// Cache the dimensions for the FileFormat base class.
	d->dimensions[0] = d->pvrHeader.width;
	d->dimensions[1] = d->pvrHeader.height;

	// Set the MIME type and texture format name.
	static const char *const sysNames[(int)SegaPVRPrivate::PVRType::Max] = {
		"Sega Dreamcast PVR",
		"Sega GVR for GameCube",
		"Sega SVR for PlayStation 2",
		"Sega PVRX for Xbox",
	};

	static_assert(ARRAY_SIZE(d->mimeTypes)-1 == ARRAY_SIZE(sysNames), "d->mimeTypes[] and sysNames[] are out of sync");
	if ((int)d->pvrType < ARRAY_SIZE_I(d->mimeTypes)-1) {
		d->mimeType = d->mimeTypes[(int)d->pvrType];
		d->textureFormatName = sysNames[(int)d->pvrType];
	}

	// TODO: Calculate the number of mipmaps.
	d->mipmapCount = 0;
}

/**
 * Is a ROM image supported by this class?
 * TODO: Add isTextureSupported() to FileFormat
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
	SegaPVRPrivate::PVRType pvrType;
	if (pvrHeader->magic == cpu_to_be32(PVR_MAGIC_PVRT)) {
		// Sega Dreamcast PVR.
		if ((pvrHeader->pvr.px_format >= SVR_PX_MIN &&
		     pvrHeader->pvr.px_format <= SVR_PX_MAX) ||
		    (pvrHeader->pvr.img_data_type >= SVR_IMG_MIN &&
		     pvrHeader->pvr.img_data_type <= SVR_IMG_MAX))
		{
			// Pixel format and/or image data type is SVR.
			pvrType = SegaPVRPrivate::PVRType::SVR;
		} else {
			// All other types are PVR.
			pvrType = SegaPVRPrivate::PVRType::PVR;
		}
	} else if (pvrHeader->magic == cpu_to_be32(PVR_MAGIC_GVRT)) {
		// GameCube GVR.
		pvrType = SegaPVRPrivate::PVRType::GVR;
	} else if (pvrHeader->magic == cpu_to_be32(PVR_MAGIC_PVRX)) {
		// Xbox PVRX.
		pvrType = SegaPVRPrivate::PVRType::PVRX;
	} else {
		// Unknown.
		pvrType = SegaPVRPrivate::PVRType::Unknown;
	}

	return static_cast<int>(pvrType);
}

/** Property accessors **/

/**
 * Get the pixel format, e.g. "RGB888" or "DXT1".
 * @return Pixel format, or nullptr if unavailable.
 */
const char *SegaPVR::pixelFormat(void) const
{
	RP_D(const SegaPVR);
	if (!d->isValid || (int)d->pvrType < 0) {
		// Not supported.
		return nullptr;
	}

	// Get the pixel format name.
	const char *pxfmt = d->pixelFormatName();
	if (!pxfmt) {
		// No pixel format name.
		// Try the image data type instead.
		pxfmt = d->imageDataTypeName();
	}

	if (pxfmt) {
		return pxfmt;
	}

	// Invalid pixel format.
	// Store an error message instead.
	// TODO: Localization?
	if (d->invalid_pixel_format[0] == '\0') {
		// GVR has pixel format and image data type located at a different offset.
		// NOTE: Using image data type for GCN.
		uint8_t val;
		switch (d->pvrType) {
			default:
			case SegaPVRPrivate::PVRType::PVR:
			case SegaPVRPrivate::PVRType::SVR:
			case SegaPVRPrivate::PVRType::PVRX:	// TODO
				val = d->pvrHeader.pvr.px_format;
				break;
			case SegaPVRPrivate::PVRType::GVR:
				val = d->pvrHeader.gvr.img_data_type;
				break;
		}

		snprintf(const_cast<SegaPVRPrivate*>(d)->invalid_pixel_format,
			sizeof(d->invalid_pixel_format),
			"Unknown (0x%02X)", val);
	}
	return d->invalid_pixel_format;
}

#ifdef ENABLE_LIBRPBASE_ROMFIELDS
/**
 * Get property fields for rom-properties.
 * @param fields RomFields object to which fields should be added.
 * @return Number of fields added, or 0 on error.
 */
int SegaPVR::getFields(RomFields *fields) const
{
	assert(fields != nullptr);
	if (!fields)
		return 0;

	RP_D(const SegaPVR);
	if (!d->isValid || (int)d->pvrType < 0) {
		// Unknown PVR image type.
		return -EIO;
	}

	const int initial_count = fields->count();
	fields->reserve(initial_count + 2);	// Maximum of 2 fields. (TODO)

	// Image data type (only if pxfmt is *not* nullptr)
	const char *pxfmt = d->pixelFormatName();
	if (pxfmt != nullptr) {
		const char *idt = d->imageDataTypeName();
		if (idt != nullptr) {
			fields->addField_string(C_("SegaPVR", "Image Data Type"), idt);
		}
	}

	// Global index (if present)
	if (d->gbix_len > 0) {
		fields->addField_string_numeric(C_("SegaPVR", "Global Index"),
			d->gbix, RomFields::Base::Dec, 0);
	}

	// Finished reading the field data.
	return (fields->count() - initial_count);
}
#endif /* ENABLE_LIBRPBASE_ROMFIELDS */

/** Image accessors **/

/**
 * Get the image.
 * For textures with mipmaps, this is the largest mipmap.
 * The image is owned by this object.
 * @return Image, or nullptr on error.
 */
rp_image_const_ptr SegaPVR::image(void) const
{
	// The full image is mipmap 0.
	return this->mipmap(0);
}

/**
 * Get the image for the specified mipmap.
 * Mipmap 0 is the largest image.
 * @param mip Mipmap number.
 * @return Image, or nullptr on error.
 */
rp_image_const_ptr SegaPVR::mipmap(int mip) const
{
	RP_D(const SegaPVR);
	if (!d->isValid) {
		// Unknown file type.
		return nullptr;
	}

	// FIXME: Support decoding mipmaps.
	if (mip != 0) {
		return nullptr;
	}

	// Load the image.
	switch (d->pvrType) {
		case SegaPVRPrivate::PVRType::PVR:
		case SegaPVRPrivate::PVRType::SVR:
			return const_cast<SegaPVRPrivate*>(d)->loadPvrImage();
		case SegaPVRPrivate::PVRType::GVR:
			return const_cast<SegaPVRPrivate*>(d)->loadGvrImage();
		default:
			// Not supported yet.
			break;
	}

	// Not supported...
	return nullptr;
}

}
