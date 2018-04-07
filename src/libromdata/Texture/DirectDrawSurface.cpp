/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * DirectDrawSurface.hpp: DirectDraw Surface image reader.                 *
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

#include "DirectDrawSurface.hpp"
#include "librpbase/RomData_p.hpp"

#include "dds_structs.h"

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
		switch (ddspf.dwFourCC) {
			case DDPF_FOURCC_DXT1:
				dxgi_format = DXGI_FORMAT_BC1_UNORM;
				break;
			case DDPF_FOURCC_DXT2:
				dxgi_format = DXGI_FORMAT_BC2_UNORM;
				dxgi_alpha = DDS_ALPHA_MODE_PREMULTIPLIED;
				break;
			case DDPF_FOURCC_DXT3:
				dxgi_format = DXGI_FORMAT_BC2_UNORM;
				break;
			case DDPF_FOURCC_DXT4:
				dxgi_format = DXGI_FORMAT_BC3_UNORM;
				dxgi_alpha = DDS_ALPHA_MODE_PREMULTIPLIED;
				break;
			case DDPF_FOURCC_DXT5:
				dxgi_format = DXGI_FORMAT_BC3_UNORM;
				break;

			case DDPF_FOURCC_ATI1:
			case DDPF_FOURCC_BC4U:
				dxgi_format = DXGI_FORMAT_BC4_UNORM;
				break;

			case DDPF_FOURCC_ATI2:
			case DDPF_FOURCC_BC5U:
				dxgi_format = DXGI_FORMAT_BC5_UNORM;
				break;

			case DDPF_FOURCC_DX10:
			case DDPF_FOURCC_XBOX:
				// Check the DX10 format.
				// TODO: Handle typeless, signed, sRGB, float.
				// TODO: Make this a lookup table with three fields?
				dxgi_alpha = dxt10Header.miscFlags2;
				switch (dxt10Header.dxgiFormat) {
					case DXGI_FORMAT_R10G10B10A2_TYPELESS:
					case DXGI_FORMAT_R10G10B10A2_UNORM:
					case DXGI_FORMAT_R10G10B10A2_UINT:
						pxf_uncomp = ImageDecoder::PXF_A2B10G10R10;
						bytespp = 4;
						break;

					case DXGI_FORMAT_R8G8B8A8_TYPELESS:
					case DXGI_FORMAT_R8G8B8A8_UNORM:
					case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
					case DXGI_FORMAT_R8G8B8A8_UINT:
					case DXGI_FORMAT_R8G8B8A8_SNORM:
					case DXGI_FORMAT_R8G8B8A8_SINT:
						pxf_uncomp = ImageDecoder::PXF_ABGR8888;
						bytespp = 4;
						break;

					case DXGI_FORMAT_R16G16_TYPELESS:
					case DXGI_FORMAT_R16G16_FLOAT:
					case DXGI_FORMAT_R16G16_UNORM:
					case DXGI_FORMAT_R16G16_UINT:
					case DXGI_FORMAT_R16G16_SNORM:
					case DXGI_FORMAT_R16G16_SINT:
						pxf_uncomp = ImageDecoder::PXF_G16R16;
						bytespp = 4;
						break;

					case DXGI_FORMAT_R8G8_TYPELESS:
					case DXGI_FORMAT_R8G8_UNORM:
					case DXGI_FORMAT_R8G8_UINT:
					case DXGI_FORMAT_R8G8_SNORM:
					case DXGI_FORMAT_R8G8_SINT:
						pxf_uncomp = ImageDecoder::PXF_GR88;
						bytespp = 2;
						break;

					case DXGI_FORMAT_A8_UNORM:
						pxf_uncomp = ImageDecoder::PXF_A8;
						bytespp = 1;
						break;

					case DXGI_FORMAT_B5G6R5_UNORM:
						pxf_uncomp = ImageDecoder::PXF_RGB565;
						bytespp = 2;
						break;

					case DXGI_FORMAT_B5G5R5A1_UNORM:
						pxf_uncomp = ImageDecoder::PXF_ARGB1555;
						bytespp = 2;
						break;

					case DXGI_FORMAT_B8G8R8A8_UNORM:
					case DXGI_FORMAT_B8G8R8A8_TYPELESS:
					case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
						pxf_uncomp = ImageDecoder::PXF_ARGB8888;
						bytespp = 4;
						break;

					case DXGI_FORMAT_B8G8R8X8_UNORM:
					case DXGI_FORMAT_B8G8R8X8_TYPELESS:
					case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
						pxf_uncomp = ImageDecoder::PXF_xRGB8888;
						bytespp = 4;
						break;

					case DXGI_FORMAT_B4G4R4A4_UNORM:
						pxf_uncomp = ImageDecoder::PXF_ARGB4444;
						bytespp = 2;
						break;

					default:
						// Use the DX10 format as-is. (Assume it's compressed.)
						dxgi_format = dxt10Header.dxgiFormat;
						break;
				}
				break;

			default:
				// Unsupported FourCC.
				ret = -ENOTSUP;
				break;
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
	const uint32_t file_sz = (uint32_t)file->size();

	// Seek to the start of the texture data.
	int ret = file->seek(texDataStartAddr);
	if (ret != 0) {
		// Seek error.
		return nullptr;
	}

	// Aligned memory buffer.
	// TODO: unique_ptr<> helper that uses aligned_malloc() and aligned_free()?
	uint8_t *buf = nullptr;

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
		buf = static_cast<uint8_t*>(aligned_malloc(16, expected_size));
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

		// TODO: Handle typeless, signed, sRGB, float.
		switch (dxgi_format) {
			case DXGI_FORMAT_BC1_TYPELESS:
			case DXGI_FORMAT_BC1_UNORM:
			case DXGI_FORMAT_BC1_UNORM_SRGB:
				if (likely(dxgi_alpha != DDS_ALPHA_MODE_OPAQUE)) {
					// 1-bit alpha.
					img = ImageDecoder::fromDXT1_A1(
						ddsHeader.dwWidth, ddsHeader.dwHeight,
						buf, expected_size);
				} else {
					// No alpha channel.
					img = ImageDecoder::fromDXT1(
						ddsHeader.dwWidth, ddsHeader.dwHeight,
						buf, expected_size);
				}
				break;

			case DXGI_FORMAT_BC2_TYPELESS:
			case DXGI_FORMAT_BC2_UNORM:
			case DXGI_FORMAT_BC2_UNORM_SRGB:
				if (likely(dxgi_alpha != DDS_ALPHA_MODE_PREMULTIPLIED)) {
					// Standard alpha: DXT3
					img = ImageDecoder::fromDXT3(
						ddsHeader.dwWidth, ddsHeader.dwHeight,
						buf, expected_size);
				} else {
					// Premultiplied alpha: DXT2
					img = ImageDecoder::fromDXT2(
						ddsHeader.dwWidth, ddsHeader.dwHeight,
						buf, expected_size);
				}
				break;

			case DXGI_FORMAT_BC3_TYPELESS:
			case DXGI_FORMAT_BC3_UNORM:
			case DXGI_FORMAT_BC3_UNORM_SRGB:
				if (likely(dxgi_alpha != DDS_ALPHA_MODE_PREMULTIPLIED)) {
					// Standard alpha: DXT5
					img = ImageDecoder::fromDXT5(
						ddsHeader.dwWidth, ddsHeader.dwHeight,
						buf, expected_size);
				} else {
					// Premultiplied alpha: DXT4
					img = ImageDecoder::fromDXT4(
						ddsHeader.dwWidth, ddsHeader.dwHeight,
						buf, expected_size);
				}
				break;

			case DXGI_FORMAT_BC4_TYPELESS:
			case DXGI_FORMAT_BC4_UNORM:
			case DXGI_FORMAT_BC4_SNORM:
				img = ImageDecoder::fromBC4(
					ddsHeader.dwWidth, ddsHeader.dwHeight,
					buf, expected_size);
				break;

			case DXGI_FORMAT_BC5_TYPELESS:
			case DXGI_FORMAT_BC5_UNORM:
			case DXGI_FORMAT_BC5_SNORM:
				img = ImageDecoder::fromBC5(
					ddsHeader.dwWidth, ddsHeader.dwHeight,
					buf, expected_size);
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
		buf = static_cast<uint8_t*>(aligned_malloc(16, expected_size));
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

		switch (bytespp) {
			case sizeof(uint8_t):
				// 8-bit image. (Usually luminance or alpha.)
				img = ImageDecoder::fromLinear8(
					(ImageDecoder::PixelFormat)pxf_uncomp,
					ddsHeader.dwWidth, ddsHeader.dwHeight,
					buf, expected_size, stride);
				break;

			case sizeof(uint16_t):
				// 16-bit RGB image.
				img = ImageDecoder::fromLinear16(
					(ImageDecoder::PixelFormat)pxf_uncomp,
					ddsHeader.dwWidth, ddsHeader.dwHeight,
					reinterpret_cast<const uint16_t*>(buf),
					expected_size, stride);
				break;

			case 24/8:
				// 24-bit RGB image.
				img = ImageDecoder::fromLinear24(
					(ImageDecoder::PixelFormat)pxf_uncomp,
					ddsHeader.dwWidth, ddsHeader.dwHeight,
					buf, expected_size, stride);
				break;

			case sizeof(uint32_t):
				// 32-bit RGB image.
				img = ImageDecoder::fromLinear32(
					(ImageDecoder::PixelFormat)pxf_uncomp,
					ddsHeader.dwWidth, ddsHeader.dwHeight,
					reinterpret_cast<const uint32_t*>(buf),
					expected_size, stride);
				break;

			default:
				// TODO: Implement other formats.
				assert(!"Unsupported pixel format.");
				break;
		}
	}

	// TODO: Untile textures for XBOX format.

	aligned_free(buf);
	return img;
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
	uint8_t header[4+sizeof(DDS_HEADER)+sizeof(DDS_HEADER_DXT10)+sizeof(DDS_HEADER_XBOX)];
	d->file->rewind();
	size_t size = d->file->read(header, sizeof(header));
	if (size < 4+sizeof(DDS_HEADER))
		return;

	// Check if this DDS texture is supported.
	DetectInfo info;
	info.header.addr = 0;
	info.header.size = (uint32_t)size;
	info.header.pData = header;
	info.ext = nullptr;	// Not needed for DDS.
	info.szFile = file->size();
	d->isValid = (isRomSupported_static(&info) >= 0);

	if (d->isValid) {
		// Is this a DXT10 texture?
		const DDS_HEADER *const pSrcHeader = reinterpret_cast<const DDS_HEADER*>(&header[4]);
		if (pSrcHeader->ddspf.dwFourCC == le32_to_cpu(DDPF_FOURCC_DX10) ||
		    pSrcHeader->ddspf.dwFourCC == le32_to_cpu(DDPF_FOURCC_XBOX))
		{
			const bool isXbox = (pSrcHeader->ddspf.dwFourCC == le32_to_cpu(DDPF_FOURCC_XBOX));
			// Verify the size.
			unsigned int headerSize;
			if (!isXbox) {
				// DX10 texture.
				headerSize = (unsigned int)(4+sizeof(DDS_HEADER)+sizeof(DDS_HEADER_DXT10));
			} else {
				// Xbox One texture.
				headerSize = (unsigned int)(4+sizeof(DDS_HEADER)+sizeof(DDS_HEADER_DXT10)+sizeof(DDS_HEADER_XBOX));
			}
			if (size < headerSize) {
				// Extra headers weren't read.
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

		// Update the pixel format.
		d->updatePixelFormat();
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
 * Get the name of the system the loaded ROM is designed for.
 * @param type System name type. (See the SystemName enum.)
 * @return System name, or nullptr if type is invalid.
 */
const char *DirectDrawSurface::systemName(unsigned int type) const
{
	RP_D(const DirectDrawSurface);
	if (!d->isValid || !isSystemNameTypeValid(type))
		return nullptr;

	// Bits 0-1: Type. (short, long, abbreviation)
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

	// DDS header.
	const DDS_HEADER *const ddsHeader = &d->ddsHeader;
	d->fields->reserve(12);	// Maximum of 12 fields.

	// Texture size.
	if (ddsHeader->dwFlags & DDSD_DEPTH) {
		d->fields->addField_string(C_("DirectDrawSurface", "Texture Size"),
			rp_sprintf("%ux%ux%u", ddsHeader->dwWidth, ddsHeader->dwHeight, ddsHeader->dwDepth));
	} else {
		d->fields->addField_string(C_("DirectDrawSurface", "Texture Size"),
			rp_sprintf("%ux%u", ddsHeader->dwWidth, ddsHeader->dwHeight));
	}

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
	if (ddspf.dwFlags & DDPF_FOURCC) {
		// Compressed RGB data.
		// TODO: Union of uint32_t and char?
		d->fields->addField_string(C_("DirectDrawSurface", "Pixel Format"),
			rp_sprintf("%c%c%c%c",
				 ddspf.dwFourCC        & 0xFF,
				(ddspf.dwFourCC >>  8) & 0xFF,
				(ddspf.dwFourCC >> 16) & 0xFF,
				(ddspf.dwFourCC >> 24) & 0xFF));
	} else if (ddspf.dwFlags & DDPF_RGB) {
		// Uncompressed RGB data.
		const char *pxfmt = d->getPixelFormatName(ddspf);
		if (pxfmt) {
			d->fields->addField_string(C_("DirectDrawSurface", "Pixel Format"), pxfmt);
		} else {
			d->fields->addField_string(C_("DirectDrawSurface", "Pixel Format"),
				rp_sprintf("RGB (%u-bit)", ddspf.dwRGBBitCount));
		}
	} else if (ddspf.dwFlags & DDPF_ALPHA) {
		// Alpha channel.
		const char *pxfmt = d->getPixelFormatName(ddspf);
		if (pxfmt) {
			d->fields->addField_string(C_("DirectDrawSurface", "Pixel Format"), pxfmt);
		} else {
			d->fields->addField_string(C_("DirectDrawSurface", "Pixel Format"),
				rp_sprintf(C_("DirectDrawSurface", "Alpha (%u-bit)"), ddspf.dwRGBBitCount));
		}
	} else if (ddspf.dwFlags & DDPF_YUV) {
		// YUV. (TODO: Determine the format.)
		d->fields->addField_string(C_("DirectDrawSurface", "Pixel Format"),
			rp_sprintf(C_("DirectDrawSurface", "YUV (%u-bit)"), ddspf.dwRGBBitCount));
	} else if (ddspf.dwFlags & DDPF_LUMINANCE) {
		// Luminance.
		const char *pxfmt = d->getPixelFormatName(ddspf);
		if (pxfmt) {
			d->fields->addField_string(C_("DirectDrawSurface", "Pixel Format"), pxfmt);
		} else {
			d->fields->addField_string(C_("DirectDrawSurface", "Pixel Format"),
				// tr: %1$s == pixel format name; %2$u == bits per pixel
				rp_sprintf_p(C_("DirectDrawSurface", "%1$s (%2$u-bit)"),
					((ddspf.dwFlags & DDPF_ALPHAPIXELS)
						? C_("DirectDrawSurface", "Luminance + Alpha")
						: C_("DirectDrawSurface", "Luminance")),
					ddspf.dwRGBBitCount));
		}
	} else {
		// Unknown pixel format.
		d->fields->addField_string(C_("DirectDrawSurface", "Pixel Format"), C_("DirectDrawSurface", "Unknown"));
	}

	if (ddspf.dwFourCC == DDPF_FOURCC_DX10 ||
	    ddspf.dwFourCC == DDPF_FOURCC_XBOX)
	{
		// DX10 texture.
		const DDS_HEADER_DXT10 *const dxt10Header = &d->dxt10Header;

		// Texture format.
		static const char *const dx10_texFormat_tbl[] = {
			nullptr, "R32G32B32A32_TYPELESS",			// 0,1
			"R32G32B32A32_FLOAT", "R32G32B32A32_UINT",		// 2,3
			"R32G32B32A32_SINT", "R32G32B32_TYPELESS",		// 4,5
			"R32G32B32_FLOAT", "R32G32B32_UINT",			// 6,7
			"R32G32B32_SINT", "R16G16B16A16_TYPELESS",		// 8,9
			"R16G16B16A16_FLOAT", "R16G16B16A16_UNORM",		// 10,11
			"R16G16B16A16_UINT", "R16G16B16A16_SNORM",		// 12,13
			"R16G16B16A16_SINT", "R32G32_TYPELESS",			// 14,15
			"R32G32_FLOAT", "R32G32_UINT",				// 16,17
			"R32G32_SINT", "R32G8X24_TYPELESS",			// 18,19
			"D32_FLOAT_S8X24_UINT", "R32_FLOAT_X8X24_TYPELESS",	// 20,21
			"X32_TYPELESS_G8X24_UINT", "R10G10B10A2_TYPELESS",	// 22,23
			"R10G10B10A2_UNORM", "R10G10B10A2_UINT",		// 24,25
			"R11G11B10_FLOAT", "R8G8B8A8_TYPELESS",			// 26,27
			"R8G8B8A8_UNORM", "R8G8B8A8_UNORM_SRGB",		// 28,29
			"R8G8B8A8_UINT", "R8G8B8A8_SNORM",			// 30,31
			"R8G8B8A8_SINT", "R16G16_TYPELESS",			// 32,33
			"R16G16_FLOAT", "R16G16_UNORM",				// 34,35
			"R16G16_UINT", "R16G16_SNORM",				// 36,37
			"R16G16_SINT", "R32_TYPELESS",				// 38,39
			"D32_FLOAT", "R32_FLOAT",				// 40,41
			"R32_UINT", "R32_SINT",					// 42,43
			"R24G8_TYPELESS", "D24_UNORM_S8_UINT",			// 44,45
			"R24_UNORM_X8_TYPELESS", "X24_TYPELESS_G8_UINT",	// 46,47
			"R8G8_TYPELESS", "R8G8_UNORM",				// 48,49
			"R8G8_UINT", "R8G8_SNORM",				// 50,51
			"R8G8_SINT", "R16_TYPELESS",				// 52,53
			"R16_FLOAT", "D16_UNORM",				// 54,55
			"R16_UNORM", "R16_UINT",				// 56,57
			"R16_SNORM", "R16_SINT",				// 58,59
			"R8_TYPELESS", "R8_UNORM",				// 60,61
			"R8_UINT", "R8_SNORM",					// 62,63
			"R8_SINT", "A8_UNORM",					// 64,65
			"R1_UNORM", "R9G9B9E5_SHAREDEXP",			// 66,67
			"R8G8_B8G8_UNORM", "G8R8_G8B8_UNORM",			// 68,69
			"BC1_TYPELESS", "BC1_UNORM",				// 70,71
			"BC1_UNORM_SRGB", "BC2_TYPELESS",			// 72,73
			"BC2_UNORM", "BC2_UNORM_SRGB",				// 74,75
			"BC3_TYPELESS", "BC3_UNORM",				// 76,77
			"BC3_UNORM_SRGB", "BC4_TYPELESS",			// 78,79
			"BC4_UNORM", "BC4_SNORM",				// 80,81
			"BC5_TYPELESS", "BC5_UNORM",				// 82,83
			"BC5_SNORM", "B5G6R5_UNORM",				// 84,85
			"B5G5R5A1_UNORM", "B8G8R8A8_UNORM",			// 86,87
			"B8G8R8X8_UNORM", "R10G10B10_XR_BIAS_A2_UNORM",		// 88,89
			"B8G8R8A8_TYPELESS", "B8G8R8A8_UNORM_SRGB",		// 90,91
			"B8G8R8X8_TYPELESS", "B8G8R8X8_UNORM_SRGB",		// 92,93
			"BC6H_TYPELESS", "BC6H_UF16",				// 94,95
			"BC6H_SF16", "BC7_TYPELESS",				// 96,97
			"BC7_UNORM", "BC7_UNORM_SRGB",				// 98,99
			"AYUV", "Y410", "Y416", "NV12",				// 100-103
			"P010", "P016", "420_OPAQUE", "YUY2",			// 104-107
			"Y210", "Y216", "NV11", "AI44",				// 108-111
			"IA44", "P8", "A8P8", "B4G4R4A4_UNORM",			// 112-115
			"XBOX_R10G10B10_7E2_A2_FLOAT",				// 116
			"XBOX_R10G10B10_6E4_A2_FLOAT",				// 117
			"XBOX_D16_UNORM_S8_UINT", "XBOX_R6_UNORM_X8_TYPELESS",	// 118-119
			"XBOX_DXGI_FORMAT_X16_TYPELESS_G8_UINT",		// 120
			nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,	// 121-126
			nullptr, nullptr, nullptr,				// 127-129
			"P208", "V208", "V408",					// 130-132
		};
		static_assert(ARRAY_SIZE(dx10_texFormat_tbl) == DXGI_FORMAT_V408+1, "dx10_texFormat_tbl size is incorrect.");

		const char *texFormat = nullptr;
		if (dxt10Header->dxgiFormat > 0 && dxt10Header->dxgiFormat < ARRAY_SIZE(dx10_texFormat_tbl)) {
			texFormat = dx10_texFormat_tbl[dxt10Header->dxgiFormat];
		} else {
			switch (dxt10Header->dxgiFormat) {
				case XBOX_DXGI_FORMAT_R10G10B10_SNORM_A2_UNORM:
					texFormat = "XBOX_R10G10B10_SNORM_A2_UNORM";
					break;
				case XBOX_DXGI_FORMAT_R4G4_UNORM:
					texFormat = "XBOX_R4G4_UNORM";
					break;
				case DXGI_FORMAT_FORCE_UINT:
					texFormat = "FORCE_UINT";
					break;
				default:
					break;
			}
		}
		d->fields->addField_string(C_("DirectDrawSurface", "DX10 Format"),
			(texFormat ? texFormat :
				rp_sprintf(C_("DirectDrawSurface", "Unknown (0x%08X)"), dxt10Header->dxgiFormat)));
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
	vector<string> *v_dwFlags_names = RomFields::strArrayToVector_i18n(
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
	vector<string> *v_dwCaps_names = RomFields::strArrayToVector_i18n(
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
	vector<string> *v_dwCaps2_names = RomFields::strArrayToVector_i18n(
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
