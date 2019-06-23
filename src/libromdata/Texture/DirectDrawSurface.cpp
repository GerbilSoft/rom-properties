/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * DirectDrawSurface.hpp: DirectDraw Surface image reader.                 *
 *                                                                         *
 * Copyright (c) 2017-2019 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "DirectDrawSurface.hpp"
#include "librpbase/RomData_p.hpp"

#include "dds_structs.h"
#include "data/DX10Formats.hpp"

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
#include <cstring>

// C++ includes.
#include <string>
#include <vector>
using std::string;
using std::vector;

namespace LibRomData {

ROMDATA_IMPL(DirectDrawSurface)
ROMDATA_IMPL_IMG_TYPES(DirectDrawSurface)

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
		DDS_HEADER_DXT10 dxt10Header;
		DDS_HEADER_XBOX xb1Header;

		// Texture data start address.
		unsigned int texDataStartAddr;

		// Decoded image.
		rp_image *img;

		/**
		 * Load the image.
		 * @return Image, or nullptr on error.
		 */
		const rp_image *loadImage(void);

	public:
		// Supported uncompressed RGB formats.
		struct RGB_Format_Table_t {
			uint32_t Rmask;
			uint32_t Gmask;
			uint32_t Bmask;
			uint32_t Amask;
			char desc[15];
			uint8_t px_format;	// ImageDecoder::PixelFormat
		};
		static const RGB_Format_Table_t rgb_fmt_tbl_16[];	// 16-bit RGB
		static const RGB_Format_Table_t rgb_fmt_tbl_24[];	// 24-bit RGB
		static const RGB_Format_Table_t rgb_fmt_tbl_32[];	// 32-bit RGB
		static const RGB_Format_Table_t rgb_fmt_tbl_luma[];	// Luminance
		static const RGB_Format_Table_t rgb_fmt_tbl_alpha[];	// Alpha

		// Image format identifiers.
		uint8_t pxf_uncomp;	// Pixel format for uncompressed images. (If 0, compressed.)
		uint8_t bytespp;	// Bytes per pixel. (Uncompressed only; set to 0 for compressed.)
		uint8_t dxgi_format;	// DXGI_FORMAT for compressed images. (If 0, uncompressed.)
		uint8_t dxgi_alpha;	// DDS_DXT10_MISC_FLAGS2 - alpha format.

		/**
		 * Get the format name of an uncompressed DirectDraw surface pixel format.
		 * @param ddspf DDS_PIXELFORMAT
		 * @return Format name, or nullptr if not supported.
		 */
		static const char *getPixelFormatName(const DDS_PIXELFORMAT &ddspf);

		/**
		 * Get the pixel formats of the DDS texture.
		 * DDS texture headers must have been loaded.
		 *
		 * If uncompressed, this sets pxf_uncomp and bytespp.
		 * If compressed, this sets dxgi_format.
		 *
		 * @return 0 on success; negative POSIX error code on error.
		 */
		int updatePixelFormat(void);
};

/** DirectDrawSurfacePrivate **/

// Supported 16-bit uncompressed RGB formats.
const DirectDrawSurfacePrivate::RGB_Format_Table_t DirectDrawSurfacePrivate::rgb_fmt_tbl_16[] = {
	// 5-bit per channel, plus alpha.
	{0x7C00, 0x03E0, 0x001F, 0x8000, "ARGB1555", ImageDecoder::PXF_ARGB1555},
	{0x001F, 0x03E0, 0x007C, 0x8000, "ABGR1555", ImageDecoder::PXF_ABGR1555},
	{0xF800, 0x07C0, 0x003E, 0x0001, "RGBA5551", ImageDecoder::PXF_RGBA5551},
	{0x003E, 0x03E0, 0x00F8, 0x0001, "BGRA5551", ImageDecoder::PXF_BGRA5551},

	// 5-bit per RB channel, 6-bit per G channel, without alpha.
	{0xF800, 0x07E0, 0x001F, 0x0000, "RGB565", ImageDecoder::PXF_RGB565},
	{0x001F, 0x07E0, 0xF800, 0x0000, "BGR565", ImageDecoder::PXF_BGR565},

	// 5-bit per channel, without alpha.
	// (Technically 15-bit, but DDS usually lists it as 16-bit.)
	{0x7C00, 0x03E0, 0x001F, 0x0000, "RGB555", ImageDecoder::PXF_RGB555},
	{0x001F, 0x03E0, 0x7C00, 0x0000, "BGR555", ImageDecoder::PXF_BGR555},

	// 4-bit per channel formats. (uncommon nowadays) (alpha)
	{0x0F00, 0x00F0, 0x000F, 0xF000, "ARGB4444", ImageDecoder::PXF_ARGB4444},
	{0x000F, 0x00F0, 0x0F00, 0xF000, "ABGR4444", ImageDecoder::PXF_ABGR4444},
	{0xF000, 0x0F00, 0x00F0, 0x000F, "RGBA4444", ImageDecoder::PXF_RGBA4444},
	{0x00F0, 0x0F00, 0xF000, 0x000F, "BGRA4444", ImageDecoder::PXF_BGRA4444},
	// 4-bit per channel formats. (uncommon nowadays) (no alpha)
	{0x0F00, 0x00F0, 0x000F, 0x0000, "xRGB4444", ImageDecoder::PXF_xRGB4444},
	{0x000F, 0x00F0, 0x0F00, 0x0000, "xBGR4444", ImageDecoder::PXF_xBGR4444},
	{0xF000, 0x0F00, 0x00F0, 0x0000, "RGBx4444", ImageDecoder::PXF_RGBx4444},
	{0x00F0, 0x0F00, 0xF000, 0x0000, "BGRx4444", ImageDecoder::PXF_BGRx4444},

	// Other uncommon 16-bit formats.
	{0x00E0, 0x001C, 0x0003, 0xFF00, "ARGB8332", ImageDecoder::PXF_ARGB8332},

	// end
	{0, 0, 0, 0, "", 0}
};

// Supported 24-bit uncompressed RGB formats.
const DirectDrawSurfacePrivate::RGB_Format_Table_t DirectDrawSurfacePrivate::rgb_fmt_tbl_24[] = {
	{0x00FF0000, 0x0000FF00, 0x000000FF, 0x00000000, "RGB888", ImageDecoder::PXF_RGB888},
	{0x000000FF, 0x0000FF00, 0x00FF0000, 0x00000000, "BGR888", ImageDecoder::PXF_BGR888},

	// end
	{0, 0, 0, 0, "", 0}
};

// Supported 32-bit uncompressed RGB formats.
const DirectDrawSurfacePrivate::RGB_Format_Table_t DirectDrawSurfacePrivate::rgb_fmt_tbl_32[] = {
	// Alpha
	{0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000, "ARGB8888", ImageDecoder::PXF_ARGB8888},
	{0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000, "ABGR8888", ImageDecoder::PXF_ABGR8888},
	{0x00FF0000, 0x0000FF00, 0x000000FF, 0x000000FF, "RGBA8888", ImageDecoder::PXF_RGBA8888},
	{0x000000FF, 0x0000FF00, 0x00FF0000, 0x000000FF, "BGRA8888", ImageDecoder::PXF_BGRA8888},

	// No alpha
	{0x00FF0000, 0x0000FF00, 0x000000FF, 0x00000000, "xRGB8888", ImageDecoder::PXF_xRGB8888},
	{0x000000FF, 0x0000FF00, 0x00FF0000, 0x00000000, "xBGR8888", ImageDecoder::PXF_xBGR8888},
	{0x00FF0000, 0x0000FF00, 0x000000FF, 0x00000000, "RGBx8888", ImageDecoder::PXF_RGBx8888},
	{0x000000FF, 0x0000FF00, 0x00FF0000, 0x00000000, "BGRx8888", ImageDecoder::PXF_BGRx8888},

	// Uncommon 32-bit formats.
	{0x0000FFFF, 0xFFFF0000, 0x00000000, 0x00000000, "G16R16", ImageDecoder::PXF_G16R16},

	{0x3FF00000, 0x000FFC00, 0x000003FF, 0xC0000000, "A2R10G10B10", ImageDecoder::PXF_A2R10G10B10},
	{0x000003FF, 0x000FFC00, 0x3FF00000, 0xC0000000, "A2B10G10R10", ImageDecoder::PXF_A2B10G10R10},

	// end
	{0, 0, 0, 0, "", 0}
};

// Supported luminance formats.
const DirectDrawSurfacePrivate::RGB_Format_Table_t DirectDrawSurfacePrivate::rgb_fmt_tbl_luma[] = {
	// 8-bit
	{0x00FF, 0x0000, 0x0000, 0x0000, "L8",   ImageDecoder::PXF_L8},
	{0x000F, 0x0000, 0x0000, 0x00F0, "A4L4", ImageDecoder::PXF_A4L4},

	// 16-bit
	{0xFFFF, 0x0000, 0x0000, 0x0000, "L16",  ImageDecoder::PXF_L16},
	{0x00FF, 0x0000, 0x0000, 0xFF00, "A8L8", ImageDecoder::PXF_A8L8},

	// end
	{0, 0, 0, 0, "", 0}
};

// Supported alpha formats.
const DirectDrawSurfacePrivate::RGB_Format_Table_t DirectDrawSurfacePrivate::rgb_fmt_tbl_alpha[] = {
	// 8-bit
	{0x0000, 0x0000, 0x0000, 0x00FF, "A8", ImageDecoder::PXF_A8},

	// end
	{0, 0, 0, 0, "", 0}
};

/**
 * Get the format name of an uncompressed DirectDraw surface pixel format.
 * @param ddspf DDS_PIXELFORMAT
 * @return Format name, or nullptr if not supported.
 */
const char *DirectDrawSurfacePrivate::getPixelFormatName(const DDS_PIXELFORMAT &ddspf)
{
#ifndef NDEBUG
	static const unsigned int FORMATS = DDPF_ALPHA | DDPF_FOURCC | DDPF_RGB | DDPF_YUV | DDPF_LUMINANCE;
	assert(((ddspf.dwFlags & FORMATS) == DDPF_RGB) ||
	       ((ddspf.dwFlags & FORMATS) == DDPF_LUMINANCE) ||
	       ((ddspf.dwFlags & FORMATS) == DDPF_ALPHA));
#endif /* !NDEBUG */

	const RGB_Format_Table_t *entry = nullptr;
	if (ddspf.dwFlags & DDPF_RGB) {
		switch (ddspf.dwRGBBitCount) {
			case 15:
			case 16:
				// 16-bit.
				entry = rgb_fmt_tbl_16;
				break;
			case 24:
				// 24-bit.
				entry = rgb_fmt_tbl_24;
				break;
			case 32:
				// 32-bit.
				entry = rgb_fmt_tbl_32;
				break;
			default:
				// Unsupported.
				return nullptr;
		}
	} else if (ddspf.dwFlags & DDPF_LUMINANCE) {
		// Luminance.
		entry = rgb_fmt_tbl_luma;
	} else if (ddspf.dwFlags & DDPF_ALPHA) {
		// Alpha.
		entry = rgb_fmt_tbl_alpha;
	} else {
		// Unsupported.
		return nullptr;
	}

	if (!entry) {
		// No table...
		return nullptr;
	}

	for (; entry->desc[0] != '\0'; entry++) {
		if (ddspf.dwRBitMask == entry->Rmask &&
		    ddspf.dwGBitMask == entry->Gmask &&
		    ddspf.dwBBitMask == entry->Bmask &&
		    ddspf.dwABitMask == entry->Amask)
		{
			// Found a match!
			return entry->desc;
		}
	}

	// Format not found.
	return nullptr;
}

/**
 * Get the pixel formats of the DDS texture.
 * DDS texture headers must have been loaded.
 *
 * If uncompressed, this sets pxf_uncomp and bytespp.
 * If compressed, this sets dxgi_format.
 *
 * @return 0 on success; negative POSIX error code on error.
 */
int DirectDrawSurfacePrivate::updatePixelFormat(void)
{
	// This should only be called once.
	assert(pxf_uncomp == 0);
	assert(bytespp == 0);
	assert(dxgi_format == 0);
	assert(dxgi_alpha == DDS_ALPHA_MODE_UNKNOWN);

	pxf_uncomp = 0;
	bytespp = 0;
	dxgi_format = 0;
	dxgi_alpha = DDS_ALPHA_MODE_STRAIGHT;	// assume a standard alpha channel

	int ret = 0;
	const DDS_PIXELFORMAT &ddspf = ddsHeader.ddspf;
#ifndef NDEBUG
	static const unsigned int FORMATS = DDPF_ALPHA | DDPF_FOURCC | DDPF_RGB | DDPF_YUV | DDPF_LUMINANCE;
	assert(((ddspf.dwFlags & FORMATS) == DDPF_FOURCC) ||
	       ((ddspf.dwFlags & FORMATS) == DDPF_RGB) ||
	       ((ddspf.dwFlags & FORMATS) == DDPF_LUMINANCE) ||
	       ((ddspf.dwFlags & FORMATS) == DDPF_ALPHA));
#endif /* !NDEBUG */

	// Check if a FourCC is specified.
	if (ddspf.dwFourCC != 0) {
		// FourCC is specified.
		static const struct {
			uint32_t dwFourCC;
			uint8_t dxgi_format;
			uint8_t dxgi_alpha;
		} fourCC_dxgi_lkup_tbl[] = {
			{DDPF_FOURCC_DXT1, DXGI_FORMAT_BC1_UNORM, DDS_ALPHA_MODE_STRAIGHT},
			{DDPF_FOURCC_DXT2, DXGI_FORMAT_BC2_UNORM, DDS_ALPHA_MODE_PREMULTIPLIED},
			{DDPF_FOURCC_DXT3, DXGI_FORMAT_BC2_UNORM, DDS_ALPHA_MODE_STRAIGHT},
			{DDPF_FOURCC_DXT4, DXGI_FORMAT_BC3_UNORM, DDS_ALPHA_MODE_PREMULTIPLIED},
			{DDPF_FOURCC_DXT5, DXGI_FORMAT_BC3_UNORM, DDS_ALPHA_MODE_STRAIGHT},

			{DDPF_FOURCC_ATI1, DXGI_FORMAT_BC4_UNORM, DDS_ALPHA_MODE_STRAIGHT},
			{DDPF_FOURCC_BC4U, DXGI_FORMAT_BC4_UNORM, DDS_ALPHA_MODE_STRAIGHT},

			{DDPF_FOURCC_ATI2, DXGI_FORMAT_BC5_UNORM, DDS_ALPHA_MODE_STRAIGHT},
			{DDPF_FOURCC_BC5U, DXGI_FORMAT_BC5_UNORM, DDS_ALPHA_MODE_STRAIGHT},

			{0, 0, 0}
		};

		for (const auto *p = fourCC_dxgi_lkup_tbl; p->dwFourCC != 0; p++) {
			if (ddspf.dwFourCC == p->dwFourCC) {
				// Found a match.
				dxgi_format = p->dxgi_format;
				dxgi_alpha = p->dxgi_alpha;
				break;
			}
		}

		// TODO: Check DX10/XBOX before the other FourCCs?
		if (dxgi_format == 0 && (ddspf.dwFourCC == DDPF_FOURCC_DX10 || ddspf.dwFourCC == DDPF_FOURCC_XBOX)) {
			// Check the DX10 format.
			// TODO: Handle typeless, signed, sRGB, float.
			dxgi_format = dxt10Header.dxgiFormat;
			dxgi_alpha = dxt10Header.miscFlags2;

			static const struct {
				uint8_t dxgi_format;
				uint8_t pxf_uncomp;
				uint8_t bytespp;
			} dx10_lkup_tbl[] = {
				{DXGI_FORMAT_R10G10B10A2_TYPELESS,	ImageDecoder::PXF_A2B10G10R10, 4},
				{DXGI_FORMAT_R10G10B10A2_UNORM,		ImageDecoder::PXF_A2B10G10R10, 4},
				{DXGI_FORMAT_R10G10B10A2_UINT,		ImageDecoder::PXF_A2B10G10R10, 4},

				{DXGI_FORMAT_R8G8B8A8_TYPELESS,		ImageDecoder::PXF_ABGR8888, 4},
				{DXGI_FORMAT_R8G8B8A8_UNORM,		ImageDecoder::PXF_ABGR8888, 4},
				{DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,	ImageDecoder::PXF_ABGR8888, 4},
				{DXGI_FORMAT_R8G8B8A8_UINT,		ImageDecoder::PXF_ABGR8888, 4},
				{DXGI_FORMAT_R8G8B8A8_SNORM,		ImageDecoder::PXF_ABGR8888, 4},
				{DXGI_FORMAT_R8G8B8A8_SINT,		ImageDecoder::PXF_ABGR8888, 4},

				{DXGI_FORMAT_R16G16_TYPELESS,		ImageDecoder::PXF_G16R16, 4},
				{DXGI_FORMAT_R16G16_FLOAT,		ImageDecoder::PXF_G16R16, 4},
				{DXGI_FORMAT_R16G16_UNORM,		ImageDecoder::PXF_G16R16, 4},
				{DXGI_FORMAT_R16G16_UINT,		ImageDecoder::PXF_G16R16, 4},
				{DXGI_FORMAT_R16G16_SNORM,		ImageDecoder::PXF_G16R16, 4},
				{DXGI_FORMAT_R16G16_SINT,		ImageDecoder::PXF_G16R16, 4},

				{DXGI_FORMAT_R8G8_TYPELESS,		ImageDecoder::PXF_GR88, 2},
				{DXGI_FORMAT_R8G8_UNORM,		ImageDecoder::PXF_GR88, 2},
				{DXGI_FORMAT_R8G8_UINT,			ImageDecoder::PXF_GR88, 2},
				{DXGI_FORMAT_R8G8_SNORM,		ImageDecoder::PXF_GR88, 2},
				{DXGI_FORMAT_R8G8_SINT,			ImageDecoder::PXF_GR88, 2},

				{DXGI_FORMAT_A8_UNORM,			ImageDecoder::PXF_A8, 1},
				{DXGI_FORMAT_B5G6R5_UNORM,		ImageDecoder::PXF_RGB565, 2},
				{DXGI_FORMAT_B5G5R5A1_UNORM,		ImageDecoder::PXF_ARGB1555, 2},

				{DXGI_FORMAT_B8G8R8A8_UNORM,		ImageDecoder::PXF_ARGB8888, 4},
				{DXGI_FORMAT_B8G8R8A8_TYPELESS,		ImageDecoder::PXF_ARGB8888, 4},
				{DXGI_FORMAT_B8G8R8A8_UNORM_SRGB,	ImageDecoder::PXF_ARGB8888, 4},

				{DXGI_FORMAT_B8G8R8X8_UNORM,		ImageDecoder::PXF_xRGB8888, 4},
				{DXGI_FORMAT_B8G8R8X8_TYPELESS,		ImageDecoder::PXF_xRGB8888, 4},
				{DXGI_FORMAT_B8G8R8X8_UNORM_SRGB,	ImageDecoder::PXF_xRGB8888, 4},

				{DXGI_FORMAT_B4G4R4A4_UNORM,		ImageDecoder::PXF_ARGB4444, 2},

				{0, 0, 0}
			};

			// If the dxgi_format is not listed in the table, we'll use it
			// as-is, assuming it's compressed.
			for (const auto *p = dx10_lkup_tbl; p->dxgi_format != 0; p++) {
				if (dxgi_format == p->dxgi_format) {
					// Found a match.
					pxf_uncomp = p->pxf_uncomp;
					bytespp = p->bytespp;
				}
			}
		}

		if (dxgi_format == 0) {
			// Unsupported FourCC.
			dxgi_alpha = DDS_ALPHA_MODE_UNKNOWN;
			ret = -ENOTSUP;
		}
	} else {
		// No FourCC.
		// Determine the pixel format by looking at the bit masks.
		const RGB_Format_Table_t *entry = nullptr;
		if (ddspf.dwFlags & DDPF_RGB) {
			switch (ddspf.dwRGBBitCount) {
				case 15:
				case 16:
					// 16-bit.
					entry = rgb_fmt_tbl_16;
					break;
				case 24:
					// 24-bit.
					entry = rgb_fmt_tbl_24;
					break;
				case 32:
					// 32-bit.
					entry = rgb_fmt_tbl_32;
					break;
				default:
					// Unsupported.
					return -ENOTSUP;
			}
		} else if (ddspf.dwFlags & DDPF_LUMINANCE) {
			// Luminance.
			entry = rgb_fmt_tbl_luma;
			// TODO: Set to standard alpha if it's Luma+Alpha?
			dxgi_alpha = DDS_ALPHA_MODE_OPAQUE;
		} else if (ddspf.dwFlags & DDPF_ALPHA) {
			// Alpha.
			entry = rgb_fmt_tbl_alpha;
		} else {
			// Unsupported.
			dxgi_alpha = DDS_ALPHA_MODE_UNKNOWN;
			return -ENOTSUP;
		}

		if (!entry) {
			// No table...
			dxgi_alpha = DDS_ALPHA_MODE_UNKNOWN;
			return -ENOTSUP;
		}

		for (; entry->desc[0] != '\0'; entry++) {
			if (ddspf.dwRBitMask == entry->Rmask &&
			    ddspf.dwGBitMask == entry->Gmask &&
			    ddspf.dwBBitMask == entry->Bmask &&
			    ddspf.dwABitMask == entry->Amask)
			{
				// Found a match!
				pxf_uncomp = entry->px_format;
				bytespp = (ddspf.dwRGBBitCount == 15 ? 2 : (ddspf.dwRGBBitCount / 8));
				dxgi_alpha = (ddspf.dwABitMask != 0 ? DDS_ALPHA_MODE_STRAIGHT : DDS_ALPHA_MODE_OPAQUE);
				return 0;
			}
		}

		// Format not found.
		dxgi_alpha = DDS_ALPHA_MODE_UNKNOWN;
		ret = -ENOTSUP;
	}

	return ret;
}

DirectDrawSurfacePrivate::DirectDrawSurfacePrivate(DirectDrawSurface *q, IRpFile *file)
	: super(q, file)
	, texDataStartAddr(0)
	, img(nullptr)
	, pxf_uncomp(0)
	, bytespp(0)
	, dxgi_format(0)
	, dxgi_alpha(DDS_ALPHA_MODE_UNKNOWN)
{
	// Clear the DDS header structs.
	memset(&ddsHeader, 0, sizeof(ddsHeader));
	memset(&dxt10Header, 0, sizeof(dxt10Header));
	memset(&xb1Header, 0, sizeof(xb1Header));
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

	// Sanity check: Maximum image dimensions of 32768x32768.
	assert(ddsHeader.dwWidth > 0);
	assert(ddsHeader.dwWidth <= 32768);
	assert(ddsHeader.dwHeight > 0);
	assert(ddsHeader.dwHeight <= 32768);
	if (ddsHeader.dwWidth == 0 || ddsHeader.dwWidth > 32768 ||
	    ddsHeader.dwHeight == 0 || ddsHeader.dwHeight > 32768)
	{
		// Invalid image dimensions.
		return nullptr;
	}

	// Texture cannot start inside of the DDS header.
	// TODO: Also dxt10Header for DX10?
	// TODO: ...and xb1Header for XBOX?
	assert(texDataStartAddr >= sizeof(ddsHeader));
	if (texDataStartAddr < sizeof(ddsHeader)) {
		// Invalid texture data start address.
		return nullptr;
	}

	if (file->size() > 128*1024*1024) {
		// Sanity check: DDS files shouldn't be more than 128 MB.
		return nullptr;
	}
	const uint32_t file_sz = static_cast<uint32_t>(file->size());

	// Seek to the start of the texture data.
	int ret = file->seek(texDataStartAddr);
	if (ret != 0) {
		// Seek error.
		return nullptr;
	}

	// TODO: Handle DX10 alpha processing.
	// Currently, we're assuming straight alpha for formats
	// that have an alpha channel, except for DXT2 and DXT4,
	// which use premultiplied alpha.

	// NOTE: Mipmaps are stored *after* the main image.
	// Hence, no mipmap processing is necessary.
	if (dxgi_format != 0) {
		// Compressed RGB data.

		// NOTE: dwPitchOrLinearSize is not necessarily correct.
		// Calculate the expected size.
		uint32_t expected_size;
		switch (dxgi_format) {
			case DXGI_FORMAT_BC1_TYPELESS:
			case DXGI_FORMAT_BC1_UNORM:
			case DXGI_FORMAT_BC1_UNORM_SRGB:
			case DXGI_FORMAT_BC4_TYPELESS:
			case DXGI_FORMAT_BC4_UNORM:
			case DXGI_FORMAT_BC4_SNORM:
				// 16 pixels compressed into 64 bits. (4bpp)
				expected_size = (ddsHeader.dwWidth * ddsHeader.dwHeight) / 2;
				break;

			case DXGI_FORMAT_BC2_TYPELESS:
			case DXGI_FORMAT_BC2_UNORM:
			case DXGI_FORMAT_BC2_UNORM_SRGB:
			case DXGI_FORMAT_BC3_TYPELESS:
			case DXGI_FORMAT_BC3_UNORM:
			case DXGI_FORMAT_BC3_UNORM_SRGB:
			case DXGI_FORMAT_BC5_TYPELESS:
			case DXGI_FORMAT_BC5_UNORM:
			case DXGI_FORMAT_BC5_SNORM:
			case DXGI_FORMAT_BC7_TYPELESS:
			case DXGI_FORMAT_BC7_UNORM:
			case DXGI_FORMAT_BC7_UNORM_SRGB:
				// 16 pixels compressed into 128 bits. (8bpp)
				expected_size = ddsHeader.dwWidth * ddsHeader.dwHeight;
				break;

			default:
				// Not supported.
				return nullptr;
		}

		// Verify file size.
		if (expected_size >= file_sz + texDataStartAddr) {
			// File is too small.
			return nullptr;
		}

		// Read the texture data.
		auto buf = aligned_uptr<uint8_t>(16, expected_size);
		size_t size = file->read(buf.get(), expected_size);
		if (size != expected_size) {
			// Read error.
			return nullptr;
		}

		// TODO: Handle typeless, signed, sRGB, float.
		switch (dxgi_format) {
			case DXGI_FORMAT_BC1_TYPELESS:
			case DXGI_FORMAT_BC1_UNORM:
			case DXGI_FORMAT_BC1_UNORM_SRGB:
				if (likely(dxgi_alpha != DDS_ALPHA_MODE_OPAQUE)) {
					// 1-bit alpha.
					img = ImageDecoder::fromDXT1_A1(
						ddsHeader.dwWidth, ddsHeader.dwHeight,
						buf.get(), expected_size);
				} else {
					// No alpha channel.
					img = ImageDecoder::fromDXT1(
						ddsHeader.dwWidth, ddsHeader.dwHeight,
						buf.get(), expected_size);
				}
				break;

			case DXGI_FORMAT_BC2_TYPELESS:
			case DXGI_FORMAT_BC2_UNORM:
			case DXGI_FORMAT_BC2_UNORM_SRGB:
				if (likely(dxgi_alpha != DDS_ALPHA_MODE_PREMULTIPLIED)) {
					// Standard alpha: DXT3
					img = ImageDecoder::fromDXT3(
						ddsHeader.dwWidth, ddsHeader.dwHeight,
						buf.get(), expected_size);
				} else {
					// Premultiplied alpha: DXT2
					img = ImageDecoder::fromDXT2(
						ddsHeader.dwWidth, ddsHeader.dwHeight,
						buf.get(), expected_size);
				}
				break;

			case DXGI_FORMAT_BC3_TYPELESS:
			case DXGI_FORMAT_BC3_UNORM:
			case DXGI_FORMAT_BC3_UNORM_SRGB:
				if (likely(dxgi_alpha != DDS_ALPHA_MODE_PREMULTIPLIED)) {
					// Standard alpha: DXT5
					img = ImageDecoder::fromDXT5(
						ddsHeader.dwWidth, ddsHeader.dwHeight,
						buf.get(), expected_size);
				} else {
					// Premultiplied alpha: DXT4
					img = ImageDecoder::fromDXT4(
						ddsHeader.dwWidth, ddsHeader.dwHeight,
						buf.get(), expected_size);
				}
				break;

			case DXGI_FORMAT_BC4_TYPELESS:
			case DXGI_FORMAT_BC4_UNORM:
			case DXGI_FORMAT_BC4_SNORM:
				img = ImageDecoder::fromBC4(
					ddsHeader.dwWidth, ddsHeader.dwHeight,
					buf.get(), expected_size);
				break;

			case DXGI_FORMAT_BC5_TYPELESS:
			case DXGI_FORMAT_BC5_UNORM:
			case DXGI_FORMAT_BC5_SNORM:
				img = ImageDecoder::fromBC5(
					ddsHeader.dwWidth, ddsHeader.dwHeight,
					buf.get(), expected_size);
				break;

			case DXGI_FORMAT_BC7_TYPELESS:
			case DXGI_FORMAT_BC7_UNORM:
			case DXGI_FORMAT_BC7_UNORM_SRGB:
				img = ImageDecoder::fromBC7(
					ddsHeader.dwWidth, ddsHeader.dwHeight,
					buf.get(), expected_size);
				break;

			default:
				// Not supported.
				break;
		}
	} else {
		// Uncompressed linear image data.
		assert(pxf_uncomp != 0);
		assert(bytespp != 0);
		if (pxf_uncomp == 0 || bytespp == 0) {
			// Pixel format wasn't updated...
			return nullptr;
		}

		// If DDSD_LINEARSIZE is set, the field is linear size,
		// so it needs to be divided by the image height.
		unsigned int stride = 0;
		if (ddsHeader.dwFlags & DDSD_LINEARSIZE) {
			if (ddsHeader.dwHeight != 0) {
				stride = ddsHeader.dwPitchOrLinearSize / ddsHeader.dwHeight;
			}
		} else {
			stride = ddsHeader.dwPitchOrLinearSize;
		}
		if (stride == 0) {
			// Invalid stride. Assume stride == width * bytespp.
			// TODO: Check for stride is too small but non-zero?
			stride = ddsHeader.dwWidth * bytespp;
		} else if (stride > (ddsHeader.dwWidth * 16)) {
			// Stride is too large.
			return nullptr;
		}
		const unsigned int expected_size = ddsHeader.dwHeight * stride;

		// Verify file size.
		if (expected_size >= file_sz + texDataStartAddr) {
			// File is too small.
			return nullptr;
		}

		// Read the texture data.
		auto buf = aligned_uptr<uint8_t>(16, expected_size);
		size_t size = file->read(buf.get(), expected_size);
		if (size != expected_size) {
			// Read error.
			return nullptr;
		}

		switch (bytespp) {
			case sizeof(uint8_t):
				// 8-bit image. (Usually luminance or alpha.)
				img = ImageDecoder::fromLinear8(
					(ImageDecoder::PixelFormat)pxf_uncomp,
					ddsHeader.dwWidth, ddsHeader.dwHeight,
					buf.get(), expected_size, stride);
				break;

			case sizeof(uint16_t):
				// 16-bit RGB image.
				img = ImageDecoder::fromLinear16(
					(ImageDecoder::PixelFormat)pxf_uncomp,
					ddsHeader.dwWidth, ddsHeader.dwHeight,
					reinterpret_cast<const uint16_t*>(buf.get()),
					expected_size, stride);
				break;

			case 24/8:
				// 24-bit RGB image.
				img = ImageDecoder::fromLinear24(
					(ImageDecoder::PixelFormat)pxf_uncomp,
					ddsHeader.dwWidth, ddsHeader.dwHeight,
					buf.get(), expected_size, stride);
				break;

			case sizeof(uint32_t):
				// 32-bit RGB image.
				img = ImageDecoder::fromLinear32(
					(ImageDecoder::PixelFormat)pxf_uncomp,
					ddsHeader.dwWidth, ddsHeader.dwHeight,
					reinterpret_cast<const uint32_t*>(buf.get()),
					expected_size, stride);
				break;

			default:
				// TODO: Implement other formats.
				assert(!"Unsupported pixel format.");
				break;
		}
	}

	// TODO: Untile textures for XBOX format.
	return img;
}

/** DirectDrawSurface **/

/**
 * Read a DirectDraw Surface image file.
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
DirectDrawSurface::DirectDrawSurface(IRpFile *file)
	: super(new DirectDrawSurfacePrivate(this, file))
{
	// This class handles texture files.
	RP_D(DirectDrawSurface);
	d->className = "DirectDrawSurface";
	d->fileType = FTYPE_TEXTURE_FILE;

	if (!d->file) {
		// Could not ref() the file handle.
		return;
	}

	// Read the DDS magic number and header.
	uint8_t header[4+sizeof(DDS_HEADER)+sizeof(DDS_HEADER_DXT10)+sizeof(DDS_HEADER_XBOX)];
	d->file->rewind();
	size_t size = d->file->read(header, sizeof(header));
	if (size < 4+sizeof(DDS_HEADER)) {
		d->file->unref();
		d->file = nullptr;
		return;
	}

	// Check if this DDS texture is supported.
	DetectInfo info;
	info.header.addr = 0;
	info.header.size = static_cast<uint32_t>(size);
	info.header.pData = header;
	info.ext = nullptr;	// Not needed for DDS.
	info.szFile = file->size();
	d->isValid = (isRomSupported_static(&info) >= 0);

	if (!d->isValid) {
		d->file->unref();
		d->file = nullptr;
		return;
	}

	// Is this a DXT10 texture?
	const DDS_HEADER *const pSrcHeader = reinterpret_cast<const DDS_HEADER*>(&header[4]);
	if (pSrcHeader->ddspf.dwFourCC == cpu_to_be32(DDPF_FOURCC_DX10) ||
	    pSrcHeader->ddspf.dwFourCC == cpu_to_be32(DDPF_FOURCC_XBOX))
	{
		const bool isXbox = (pSrcHeader->ddspf.dwFourCC == le32_to_cpu(DDPF_FOURCC_XBOX));
		// Verify the size.
		unsigned int headerSize;
		if (!isXbox) {
			// DX10 texture.
			headerSize = static_cast<unsigned int>(4+sizeof(DDS_HEADER)+sizeof(DDS_HEADER_DXT10));
		} else {
			// Xbox One texture.
			headerSize = static_cast<unsigned int>(4+sizeof(DDS_HEADER)+sizeof(DDS_HEADER_DXT10)+sizeof(DDS_HEADER_XBOX));
		}
		if (size < headerSize) {
			// Extra headers weren't read.
			d->file->unref();
			d->file = nullptr;
			d->isValid = false;
			return;
		}

		// Save the DXT10 header.
		memcpy(&d->dxt10Header, &header[4+sizeof(DDS_HEADER)], sizeof(d->dxt10Header));
		if (isXbox) {
			// Save the Xbox One header.
			memcpy(&d->xb1Header, &header[4+sizeof(DDS_HEADER)+sizeof(DDS_HEADER_DXT10)], sizeof(d->xb1Header));
		}

#if SYS_BYTEORDER == SYS_BIG_ENDIAN
		// Byteswap the DXT10 header.
		d->dxt10Header.dxgiFormat = (DXGI_FORMAT)le32_to_cpu((uint32_t)d->dxt10Header.dxgiFormat);
		d->dxt10Header.resourceDimension = (D3D10_RESOURCE_DIMENSION)le32_to_cpu((uint32_t)d->dxt10Header.resourceDimension);
		d->dxt10Header.miscFlag   = le32_to_cpu(d->dxt10Header.miscFlag);
		d->dxt10Header.arraySize  = le32_to_cpu(d->dxt10Header.arraySize);
		d->dxt10Header.miscFlags2 = le32_to_cpu(d->dxt10Header.miscFlags2);
		if (isXbox) {
			// Byteswap the Xbox One header.
			d->xb1Header.tileMode		= le32_to_cpu(d->xb1Header.tileMode);
			d->xb1Header.baseAlignment	= le32_to_cpu(d->xb1Header.baseAlignment);
			d->xb1Header.dataSize		= le32_to_cpu(d->xb1Header.dataSize);
			d->xb1Header.xdkVer		= le32_to_cpu(d->xb1Header.xdkVer);
		}
#endif /* SYS_BYTEORDER == SYS_BIG_ENDIAN */

		// Texture data start address.
		d->texDataStartAddr = headerSize;
	} else {
		// No DXT10 header.
		d->texDataStartAddr = 4+sizeof(DDS_HEADER);
	}

	// Save the DDS header.
	memcpy(&d->ddsHeader, pSrcHeader, sizeof(d->ddsHeader));

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
	// NOTE: FourCC is handled separately.
	DDS_PIXELFORMAT &ddspf = d->ddsHeader.ddspf;
	ddspf.dwSize		= le32_to_cpu(ddspf.dwSize);
	ddspf.dwFlags		= le32_to_cpu(ddspf.dwFlags);
	ddspf.dwRGBBitCount	= le32_to_cpu(ddspf.dwRGBBitCount);
	ddspf.dwRBitMask	= le32_to_cpu(ddspf.dwRBitMask);
	ddspf.dwGBitMask	= le32_to_cpu(ddspf.dwGBitMask);
	ddspf.dwBBitMask	= le32_to_cpu(ddspf.dwBBitMask);
	ddspf.dwABitMask	= le32_to_cpu(ddspf.dwABitMask);
#else /* SYS_BYTEORDER == SYS_LIL_ENDIAN */
	// FourCC is considered to be big-endian.
	d->ddsHeader.ddspf.dwFourCC = be32_to_cpu(d->ddsHeader.ddspf.dwFourCC);
#endif

	// Update the pixel format.
	d->updatePixelFormat();
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
	const uint32_t *const pData32 = reinterpret_cast<const uint32_t*>(info->header.pData);
	if (pData32[0] == cpu_to_be32(DDS_MAGIC)) {
		// DDS magic is present.
		// Check the structure sizes.
		const DDS_HEADER *const ddsHeader =
			reinterpret_cast<const DDS_HEADER*>(&info->header.pData[4]);
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
 * Get the name of the system the loaded ROM is designed for.
 * @param type System name type. (See the SystemName enum.)
 * @return System name, or nullptr if type is invalid.
 */
const char *DirectDrawSurface::systemName(unsigned int type) const
{
	RP_D(const DirectDrawSurface);
	if (!d->isValid || !isSystemNameTypeValid(type))
		return nullptr;

	// DirectDraw Surface has the same name worldwide, so we can
	// ignore the region selection.
	static_assert(SYSNAME_TYPE_MASK == 3,
		"DirectDrawSurface::systemName() array index optimization needs to be updated.");

	// Bits 0-1: Type. (long, short, abbreviation)
	static const char *const sysNames[4] = {
		"DirectDraw Surface", "DirectDraw Surface", "DDS", nullptr
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
const char *const *DirectDrawSurface::supportedFileExtensions_static(void)
{
	static const char *const exts[] = {
		".dds",	// DirectDraw Surface
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
const char *const *DirectDrawSurface::supportedMimeTypes_static(void)
{
	static const char *const mimeTypes[] = {
		// Unofficial MIME types from FreeDesktop.org.
		"image/x-dds",

		nullptr
	};
	return mimeTypes;
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
 * Get a list of all available image sizes for the specified image type.
 * @param imageType Image type.
 * @return Vector of available image sizes, or empty vector if no images are available.
 */
vector<RomData::ImageSizeDef> DirectDrawSurface::supportedImageSizes(ImageType imageType) const
{
	ASSERT_supportedImageSizes(imageType);

	RP_D(const DirectDrawSurface);
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
	ASSERT_imgpf(imageType);

	RP_D(const DirectDrawSurface);
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

	// DDS header.
	const DDS_HEADER *const ddsHeader = &d->ddsHeader;
	d->fields->reserve(13);	// Maximum of 13 fields.

	// Texture size.
	d->fields->addField_dimensions(C_("DirectDrawSurface", "Texture Size"),
		ddsHeader->dwWidth, ddsHeader->dwHeight,
		(ddsHeader->dwFlags & DDSD_DEPTH) ? ddsHeader->dwDepth : 0);

	// Pitch (uncompressed)
	// Linear size (compressed)
	const char *const pitch_name = (ddsHeader->dwFlags & DDSD_LINEARSIZE)
		? C_("DirectDrawSurface", "Linear Size")
		: C_("DirectDrawSurface", "Pitch");
	d->fields->addField_string_numeric(
		dpgettext_expr(RP_I18N_DOMAIN, "DirectDrawSurface", pitch_name),
		ddsHeader->dwPitchOrLinearSize, RomFields::FB_DEC, 0);

	// Mipmap count.
	// NOTE: DDSD_MIPMAPCOUNT might not be accurate, so ignore it.
	d->fields->addField_string_numeric(C_("DirectDrawSurface", "Mipmap Count"),
		ddsHeader->dwMipMapCount, RomFields::FB_DEC, 0);

	// Pixel format.
	const DDS_PIXELFORMAT &ddspf = ddsHeader->ddspf;
	const char *const pixel_format_title = C_("DirectDrawSurface", "Pixel Format");
	if (ddspf.dwFlags & DDPF_FOURCC) {
		// Compressed RGB data.
		char cFourCC[5];
		cFourCC[0] = (ddspf.dwFourCC >> 24) & 0xFF;
		cFourCC[1] = (ddspf.dwFourCC >> 16) & 0xFF;
		cFourCC[2] = (ddspf.dwFourCC >>  8) & 0xFF;
		cFourCC[3] =  ddspf.dwFourCC        & 0xFF;
		cFourCC[4] = '\0';
		d->fields->addField_string(pixel_format_title, cFourCC);
	} else if (ddspf.dwFlags & DDPF_RGB) {
		// Uncompressed RGB data.
		const char *pxfmt = d->getPixelFormatName(ddspf);
		if (pxfmt) {
			d->fields->addField_string(pixel_format_title, pxfmt);
		} else {
			d->fields->addField_string(pixel_format_title,
				rp_sprintf("RGB (%u-bit)", ddspf.dwRGBBitCount));
		}
	} else if (ddspf.dwFlags & DDPF_ALPHA) {
		// Alpha channel.
		const char *pxfmt = d->getPixelFormatName(ddspf);
		if (pxfmt) {
			d->fields->addField_string(pixel_format_title, pxfmt);
		} else {
			d->fields->addField_string(pixel_format_title,
				rp_sprintf(C_("DirectDrawSurface", "Alpha (%u-bit)"), ddspf.dwRGBBitCount));
		}
	} else if (ddspf.dwFlags & DDPF_YUV) {
		// YUV. (TODO: Determine the format.)
		d->fields->addField_string(pixel_format_title,
			rp_sprintf(C_("DirectDrawSurface", "YUV (%u-bit)"), ddspf.dwRGBBitCount));
	} else if (ddspf.dwFlags & DDPF_LUMINANCE) {
		// Luminance.
		const char *pxfmt = d->getPixelFormatName(ddspf);
		if (pxfmt) {
			d->fields->addField_string(pixel_format_title, pxfmt);
		} else {
			d->fields->addField_string(pixel_format_title,
				// tr: %1$s == pixel format name; %2$u == bits per pixel
				rp_sprintf_p(C_("DirectDrawSurface", "%1$s (%2$u-bit)"),
					((ddspf.dwFlags & DDPF_ALPHAPIXELS)
						? C_("DirectDrawSurface", "Luminance + Alpha")
						: C_("DirectDrawSurface", "Luminance")),
					ddspf.dwRGBBitCount));
		}
	} else {
		// Unknown pixel format.
		d->fields->addField_string(pixel_format_title,
			C_("RomData", "Unknown"));
	}

	if (d->dxgi_format != 0) {
		// DX10 texture format.
		const char *const texFormat = DX10Formats::lookup_dxgiFormat(d->dxgi_format);
		d->fields->addField_string(C_("DirectDrawSurface", "DX10 Format"),
			(texFormat ? texFormat :
				rp_sprintf(C_("RomData", "Unknown (0x%08X)"), d->dxgi_format)));
	}

	// nVidia Texture Tools header
	if (ddsHeader->nvtt.dwNvttMagic == cpu_to_be32(NVTT_MAGIC)) {
		const uint32_t nvtt_version = le32_to_cpu(ddsHeader->nvtt.dwNvttVersion);
		d->fields->addField_string(C_("DirectDrawSurface", "NVTT Version"),
			rp_sprintf(C_("DirectDrawSurface", "%u.%u.%u"),
				   (nvtt_version >> 16) & 0xFF,
				   (nvtt_version >>  8) & 0xFF,
				    nvtt_version        & 0xFF));
	}

	// dwFlags
	static const char *const dwFlags_names[] = {
		// 0x1-0x8
		NOP_C_("DirectDrawSurface|dwFlags", "Caps"),
		NOP_C_("DirectDrawSurface|dwFlags", "Height"),
		NOP_C_("DirectDrawSurface|dwFlags", "Width"),
		NOP_C_("DirectDrawSurface|dwFlags", "Pitch"),
		// 0x10-0x80
		nullptr, nullptr, nullptr, nullptr,
		// 0x100-0x800
		nullptr, nullptr, nullptr, nullptr,
		// 0x1000-0x8000
		NOP_C_("DirectDrawSurface|dwFlags", "Pixel Format"),
		nullptr, nullptr, nullptr,
		// 0x10000-0x80000
		nullptr,
		NOP_C_("DirectDrawSurface|dwFlags", "Mipmap Count"),
		nullptr,
		NOP_C_("DirectDrawSurface|dwFlags", "Linear Size"),
		// 0x100000-0x800000
		nullptr, nullptr, nullptr,
		NOP_C_("DirectDrawSurface|dwFlags", "Depth"),
	};
	vector<string> *const v_dwFlags_names = RomFields::strArrayToVector_i18n(
		"DirectDrawSurface|dwFlags", dwFlags_names, ARRAY_SIZE(dwFlags_names));
	d->fields->addField_bitfield(C_("DirectDrawSurface", "Flags"),
		v_dwFlags_names, 3, ddsHeader->dwFlags);

	// dwCaps
	static const char *const dwCaps_names[] = {
		// 0x1-0x8
		nullptr, nullptr, nullptr,
		NOP_C_("DirectDrawSurface|dwCaps", "Complex"),
		// 0x10-0x80
		nullptr, nullptr, nullptr, nullptr,
		// 0x100-0x800
		nullptr, nullptr, nullptr, nullptr,
		// 0x1000-0x8000
		NOP_C_("DirectDrawSurface|dwCaps", "Texture"),
		nullptr, nullptr, nullptr,
		// 0x10000-0x80000
		nullptr, nullptr, nullptr, nullptr,
		// 0x100000-0x400000
		nullptr, nullptr,
		NOP_C_("DirectDrawSurface|dwCaps", "Mipmap"),
	};
	vector<string> *const v_dwCaps_names = RomFields::strArrayToVector_i18n(
		"DirectDrawSurface|dwFlags", dwCaps_names, ARRAY_SIZE(dwCaps_names));
	d->fields->addField_bitfield(C_("DirectDrawSurface", "Caps"),
		v_dwCaps_names, 3, ddsHeader->dwCaps);

	// dwCaps2
	static const char *const dwCaps2_names[] = {
		// 0x1-0x8
		nullptr, nullptr, nullptr, nullptr,
		// 0x10-0x80
		nullptr, nullptr, nullptr, nullptr,
		// 0x100-0x800
		nullptr,
		NOP_C_("DirectDrawSurface|dwCaps2", "Cubemap"),
		NOP_C_("DirectDrawSurface|dwCaps2", "+X"),
		NOP_C_("DirectDrawSurface|dwCaps2", "-X"),
		// 0x1000-0x8000
		NOP_C_("DirectDrawSurface|dwCaps2", "+Y"),
		NOP_C_("DirectDrawSurface|dwCaps2", "-Y"),
		NOP_C_("DirectDrawSurface|dwCaps2", "+Z"),
		NOP_C_("DirectDrawSurface|dwCaps2", "-Z"),
		// 0x10000-0x80000
		nullptr, nullptr, nullptr, nullptr,
		// 0x100000-0x200000
		nullptr,
		NOP_C_("DirectDrawSurface|dwCaps2", "Volume"),
	};
	vector<string> *const v_dwCaps2_names = RomFields::strArrayToVector_i18n(
		"DirectDrawSurface|dwCaps2", dwCaps2_names, ARRAY_SIZE(dwCaps2_names));
	d->fields->addField_bitfield(C_("DirectDrawSurface", "Caps2"),
		v_dwCaps2_names, 4, ddsHeader->dwCaps2);

	if (ddspf.dwFourCC == DDPF_FOURCC_XBOX) {
		// Xbox One texture.
		const DDS_HEADER_XBOX *const xb1Header = &d->xb1Header;

		d->fields->addField_string_numeric(C_("DirectDrawSurface", "Tile Mode"), xb1Header->tileMode);
		d->fields->addField_string_numeric(C_("DirectDrawSurface", "Base Alignment"), xb1Header->baseAlignment);
		// TODO: Not needed?
		d->fields->addField_string_numeric(C_("DirectDrawSurface", "Data Size"), xb1Header->dataSize);
		// TODO: Parse this.
		d->fields->addField_string_numeric(C_("DirectDrawSurface", "XDK Version"),
			xb1Header->xdkVer, RomFields::FB_HEX, 4, RomFields::STRF_MONOSPACE);
	}

	// Finished reading the field data.
	return static_cast<int>(d->fields->count());
}

/**
 * Load metadata properties.
 * Called by RomData::metaData() if the field data hasn't been loaded yet.
 * @return Number of metadata properties read on success; negative POSIX error code on error.
 */
int DirectDrawSurface::loadMetaData(void)
{
	RP_D(DirectDrawSurface);
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

	// DDS header.
	const DDS_HEADER *const ddsHeader = &d->ddsHeader;

	// Dimensions.
	// TODO: Don't add dwHeight for 1D textures?
	d->metaData->addMetaData_integer(Property::Width, ddsHeader->dwWidth);
	d->metaData->addMetaData_integer(Property::Height, ddsHeader->dwHeight);

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
int DirectDrawSurface::loadInternalImage(ImageType imageType, const rp_image **pImage)
{
	ASSERT_loadInternalImage(imageType, pImage);

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
