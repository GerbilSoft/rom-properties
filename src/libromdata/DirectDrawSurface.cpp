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
using namespace LibRpBase;

// C includes. (C++ namespace)
#include <cassert>
#include <cstring>

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
		DDS_HEADER_DXT10 dxt10Header;

		// Texture data start address.
		uint32_t texDataStartAddr;

		// Decoded image.
		rp_image *img;

		/**
		 * Load the image.
		 * @return Image, or nullptr on error.
		 */
		const rp_image *loadImage(void);

	public:
		// TODO: Convert const rp_char* to rp_char[10].
		// Can't do that right now due to issues with _RP() on
		// MSVC versions older than 2015.

		// Supported uncompressed RGB formats.
		struct RGB_Format_Table_t {
			uint32_t Rmask;
			uint32_t Gmask;
			uint32_t Bmask;
			uint32_t Amask;
			const rp_char *desc;
			uint8_t px_format;	// ImageDecoder::PixelFormat
		};
		static const RGB_Format_Table_t rgb_fmt_tbl_16[];	// 16-bit RGB
		static const RGB_Format_Table_t rgb_fmt_tbl_24[];	// 24-bit RGB
		static const RGB_Format_Table_t rgb_fmt_tbl_32[];	// 32-bit RGB
		static const RGB_Format_Table_t rgb_fmt_tbl_luma[];	// Luminance
		static const RGB_Format_Table_t rgb_fmt_tbl_alpha[];	// Alpha

		/**
		 * Get the format name of an uncompressed DirectDraw surface pixel format.
		 * @param ddspf DDS_PIXELFORMAT
		 * @return Format name, or nullptr if not supported.
		 */
		static const rp_char *getPixelFormatName(const DDS_PIXELFORMAT &ddspf);

		/**
		 * Get the ImageDecoder::PixelFormat of an RGB(A) DirectDraw surface pixel format.
		 * @param ddspf	[in] DDS_PIXELFORMAT
		 * @param bpp	[out,opt] Bytes per pixel. (15/16 becomes 2)
		 * @return ImageDecoder::PixelFormat, or ImageDecoder::PXF_UNKNOWN if not supported.
		 */
		static ImageDecoder::PixelFormat getPixelFormat(const DDS_PIXELFORMAT &ddspf, unsigned int *bytespp);
};

/** DirectDrawSurfacePrivate **/

// Supported 16-bit uncompressed RGB formats.
const DirectDrawSurfacePrivate::RGB_Format_Table_t DirectDrawSurfacePrivate::rgb_fmt_tbl_16[] = {
	// 5-bit per channel, plus alpha.
	{0x7C00, 0x03E0, 0x001F, 0x8000, _RP("ARGB1555"), ImageDecoder::PXF_ARGB1555},
	{0x001F, 0x03E0, 0x007C, 0x8000, _RP("ABGR1555"), ImageDecoder::PXF_ABGR1555},
	{0xF800, 0x07C0, 0x003E, 0x0001, _RP("RGBA5551"), ImageDecoder::PXF_RGBA5551},
	{0x003E, 0x03E0, 0x00F8, 0x0001, _RP("BGRA5551"), ImageDecoder::PXF_BGRA5551},

	// 5-bit per RB channel, 6-bit per G channel, without alpha.
	{0xF800, 0x07E0, 0x001F, 0x0000, _RP("RGB565"), ImageDecoder::PXF_RGB565},
	{0x001F, 0x07E0, 0xF800, 0x0000, _RP("BGR565"), ImageDecoder::PXF_BGR565},

	// 5-bit per channel, without alpha.
	// (Technically 15-bit, but DDS usually lists it as 16-bit.)
	{0x7C00, 0x03E0, 0x001F, 0x0000, _RP("RGB555"), ImageDecoder::PXF_RGB555},
	{0x001F, 0x03E0, 0x7C00, 0x0000, _RP("BGR555"), ImageDecoder::PXF_BGR555},

	// 4-bit per channel formats. (uncommon nowadays) (alpha)
	{0x0F00, 0x00F0, 0x000F, 0xF000, _RP("ARGB4444"), ImageDecoder::PXF_ARGB4444},
	{0x000F, 0x00F0, 0x0F00, 0xF000, _RP("ABGR4444"), ImageDecoder::PXF_ABGR4444},
	{0xF000, 0x0F00, 0x00F0, 0x000F, _RP("RGBA4444"), ImageDecoder::PXF_RGBA4444},
	{0x00F0, 0x0F00, 0xF000, 0x000F, _RP("BGRA4444"), ImageDecoder::PXF_BGRA4444},
	// 4-bit per channel formats. (uncommon nowadays) (no alpha)
	{0x0F00, 0x00F0, 0x000F, 0x0000, _RP("xRGB4444"), ImageDecoder::PXF_xRGB4444},
	{0x000F, 0x00F0, 0x0F00, 0x0000, _RP("xBGR4444"), ImageDecoder::PXF_xBGR4444},
	{0xF000, 0x0F00, 0x00F0, 0x0000, _RP("RGBx4444"), ImageDecoder::PXF_RGBx4444},
	{0x00F0, 0x0F00, 0xF000, 0x0000, _RP("BGRx4444"), ImageDecoder::PXF_BGRx4444},

	// Other uncommon 16-bit formats.
	{0x00E0, 0x001C, 0x0003, 0xFF00, _RP("ARGB8332"), ImageDecoder::PXF_ARGB8332},

	// end
	{0, 0, 0, 0, nullptr, 0}
};

// Supported 24-bit uncompressed RGB formats.
const DirectDrawSurfacePrivate::RGB_Format_Table_t DirectDrawSurfacePrivate::rgb_fmt_tbl_24[] = {
	{0x00FF0000, 0x0000FF00, 0x000000FF, 0x00000000, _RP("RGB888"), ImageDecoder::PXF_RGB888},
	{0x000000FF, 0x0000FF00, 0x00FF0000, 0x00000000, _RP("BGR888"), ImageDecoder::PXF_BGR888},

	// end
	{0, 0, 0, 0, nullptr, 0}
};

// Supported 32-bit uncompressed RGB formats.
const DirectDrawSurfacePrivate::RGB_Format_Table_t DirectDrawSurfacePrivate::rgb_fmt_tbl_32[] = {
	// Alpha
	{0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000, _RP("ARGB8888"), ImageDecoder::PXF_ARGB8888},
	{0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000, _RP("ABGR8888"), ImageDecoder::PXF_ABGR8888},
	{0x00FF0000, 0x0000FF00, 0x000000FF, 0x000000FF, _RP("RGBA8888"), ImageDecoder::PXF_RGBA8888},
	{0x000000FF, 0x0000FF00, 0x00FF0000, 0x000000FF, _RP("BGRA8888"), ImageDecoder::PXF_BGRA8888},

	// No alpha
	{0x00FF0000, 0x0000FF00, 0x000000FF, 0x00000000, _RP("xRGB8888"), ImageDecoder::PXF_xRGB8888},
	{0x000000FF, 0x0000FF00, 0x00FF0000, 0x00000000, _RP("xBGR8888"), ImageDecoder::PXF_xBGR8888},
	{0x00FF0000, 0x0000FF00, 0x000000FF, 0x00000000, _RP("RGBx8888"), ImageDecoder::PXF_RGBx8888},
	{0x000000FF, 0x0000FF00, 0x00FF0000, 0x00000000, _RP("BGRx8888"), ImageDecoder::PXF_BGRx8888},

	// Uncommon 32-bit formats.
	{0x0000FFFF, 0xFFFF0000, 0x00000000, 0x00000000, _RP("G16R16"), ImageDecoder::PXF_G16R16},

	{0x3FF00000, 0x000FFC00, 0x000003FF, 0xC0000000, _RP("A2R10G10B10"), ImageDecoder::PXF_A2R10G10B10},
	{0x000003FF, 0x000FFC00, 0x3FF00000, 0xC0000000, _RP("A2B10G10R10"), ImageDecoder::PXF_A2B10G10R10},

	// end
	{0, 0, 0, 0, nullptr, 0}
};

// Supported luminance formats.
const DirectDrawSurfacePrivate::RGB_Format_Table_t DirectDrawSurfacePrivate::rgb_fmt_tbl_luma[] = {
	// 8-bit
	{0x00FF, 0x0000, 0x0000, 0x0000, _RP("L8"), ImageDecoder::PXF_L8},
	{0x000F, 0x0000, 0x0000, 0x00F0, _RP("A4L4"), ImageDecoder::PXF_A4L4},

	// 16-bit
	{0xFFFF, 0x0000, 0x0000, 0x0000, _RP("L16"), ImageDecoder::PXF_L16},
	{0x00FF, 0x0000, 0x0000, 0xFF00, _RP("A8L8"), ImageDecoder::PXF_A8L8},

	// end
	{0, 0, 0, 0, nullptr, 0}
};

// Supported alpha formats.
const DirectDrawSurfacePrivate::RGB_Format_Table_t DirectDrawSurfacePrivate::rgb_fmt_tbl_alpha[] = {
	// 8-bit
	{0x0000, 0x0000, 0x0000, 0x00FF, _RP("A8"), ImageDecoder::PXF_A8},

	// end
	{0, 0, 0, 0, nullptr, 0}
};

/**
 * Get the format name of an uncompressed DirectDraw surface pixel format.
 * @param ddspf DDS_PIXELFORMAT
 * @return Format name, or nullptr if not supported.
 */
const rp_char *DirectDrawSurfacePrivate::getPixelFormatName(const DDS_PIXELFORMAT &ddspf)
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

	for (; entry->desc != nullptr; entry++) {
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
 * Get the ImageDecoder::PixelFormat of an uncompressed DirectDraw surface pixel format.
 * @param ddspf	[in] DDS_PIXELFORMAT
 * @param bpp	[out,opt] Bytes per pixel. (15/16 becomes 2)
 * @return ImageDecoder::PixelFormat, or ImageDecoder::PXF_UNKNOWN if not supported.
 */
ImageDecoder::PixelFormat DirectDrawSurfacePrivate::getPixelFormat(const DDS_PIXELFORMAT &ddspf, unsigned int *bytespp)
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
				if (bytespp) {
					*bytespp = 0;
				}
				return ImageDecoder::PXF_UNKNOWN;
		}
	} else if (ddspf.dwFlags & DDPF_LUMINANCE) {
		// Luminance.
		entry = rgb_fmt_tbl_luma;
	} else if (ddspf.dwFlags & DDPF_ALPHA) {
		// Alpha.
		entry = rgb_fmt_tbl_alpha;
	} else {
		// Unsupported.
		if (bytespp) {
			*bytespp = 0;
		}
		return ImageDecoder::PXF_UNKNOWN;
	}

	if (!entry) {
		// No table...
		if (bytespp) {
			*bytespp = 0;
		}
		return ImageDecoder::PXF_UNKNOWN;
	}

	for (; entry->desc != nullptr; entry++) {
		if (ddspf.dwRBitMask == entry->Rmask &&
		    ddspf.dwGBitMask == entry->Gmask &&
		    ddspf.dwBBitMask == entry->Bmask &&
		    ddspf.dwABitMask == entry->Amask)
		{
			// Found a match!
			if (bytespp) {
				*bytespp = (ddspf.dwRGBBitCount == 15 ? 2 : (ddspf.dwRGBBitCount / 8));
			}
			return (ImageDecoder::PixelFormat)entry->px_format;
		}
	}

	// Format not found.
	if (bytespp) {
		*bytespp = 0;
	}
	return ImageDecoder::PXF_UNKNOWN;
}

DirectDrawSurfacePrivate::DirectDrawSurfacePrivate(DirectDrawSurface *q, IRpFile *file)
	: super(q, file)
	, texDataStartAddr(0)
	, img(nullptr)
{
	// Clear the DDS header structs.
	memset(&ddsHeader, 0, sizeof(ddsHeader));
	memset(&dxt10Header, 0, sizeof(dxt10Header));
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

	// NOTE: Mipmaps are stored *after* the main image.
	// Hence, no mipmap processing is necessary.
	const DDS_PIXELFORMAT &ddspf = ddsHeader.ddspf;
	if (ddspf.dwFlags & DDPF_FOURCC) {
		// Compressed RGB data.

		// NOTE: dwPitchOrLinearSize is not necessarily correct.
		// Calculate the expected size.
		uint32_t expected_size;
		switch (ddspf.dwFourCC) {
			case DDPF_FOURCC_DXT1:
			case DDPF_FOURCC_ATI1:
			case DDPF_FOURCC_BC4U:
				// 16 pixels compressed into 64 bits. (4bpp)
				expected_size = (ddsHeader.dwWidth * ddsHeader.dwHeight) / 2;
				break;

			case DDPF_FOURCC_DXT2:
			case DDPF_FOURCC_DXT3:
			case DDPF_FOURCC_DXT4:
			case DDPF_FOURCC_DXT5:
			case DDPF_FOURCC_ATI2:
			case DDPF_FOURCC_BC5U:
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
		unique_ptr<uint8_t[]> buf(new uint8_t[expected_size]);
		size_t size = file->read(buf.get(), expected_size);
		if (size != expected_size) {
			// Read error.
			return nullptr;
		}

		switch (ddspf.dwFourCC) {
			case DDPF_FOURCC_DXT1:
				img = ImageDecoder::fromDXT1(
					ddsHeader.dwWidth, ddsHeader.dwHeight,
					buf.get(), expected_size);
				break;

			case DDPF_FOURCC_DXT2:
				img = ImageDecoder::fromDXT2(
					ddsHeader.dwWidth, ddsHeader.dwHeight,
					buf.get(), expected_size);
				break;

			case DDPF_FOURCC_DXT3:
				img = ImageDecoder::fromDXT3(
					ddsHeader.dwWidth, ddsHeader.dwHeight,
					buf.get(), expected_size);
				break;

			case DDPF_FOURCC_DXT4:
				img = ImageDecoder::fromDXT4(
					ddsHeader.dwWidth, ddsHeader.dwHeight,
					buf.get(), expected_size);
				break;

			case DDPF_FOURCC_DXT5:
				img = ImageDecoder::fromDXT5(
					ddsHeader.dwWidth, ddsHeader.dwHeight,
					buf.get(), expected_size);
				break;

			case DDPF_FOURCC_ATI1:
			case DDPF_FOURCC_BC4U:
				img = ImageDecoder::fromBC4(
					ddsHeader.dwWidth, ddsHeader.dwHeight,
					buf.get(), expected_size);
				break;

			case DDPF_FOURCC_ATI2:
			case DDPF_FOURCC_BC5U:
				img = ImageDecoder::fromBC5(
					ddsHeader.dwWidth, ddsHeader.dwHeight,
					buf.get(), expected_size);
				break;

			default:
				// Not supported.
				return nullptr;
		}
	} else {
		// Uncompressed linear image data.
		unsigned int bytespp = 0;
		ImageDecoder::PixelFormat px_format = getPixelFormat(ddspf, &bytespp);
		if (px_format == ImageDecoder::PXF_UNKNOWN || bytespp == 0) {
			// Unknown pixel format.
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
		// TODO: unique_ptr<> helper that uses aligned_malloc() and aligned_free()?
		uint8_t *buf = static_cast<uint8_t*>(aligned_malloc(16, expected_size));
		size_t size = file->read(buf, expected_size);
		if (size != expected_size) {
			// Read error.
			aligned_free(buf);
			return nullptr;
		}

		switch (bytespp) {
			case sizeof(uint8_t):
				// 8-bit image. (Usually luminance or alpha.)
				img = ImageDecoder::fromLinear8(px_format,
					ddsHeader.dwWidth, ddsHeader.dwHeight,
					buf, expected_size, stride);
				break;

			case sizeof(uint16_t):
				// 16-bit RGB image.
				img = ImageDecoder::fromLinear16(px_format,
					ddsHeader.dwWidth, ddsHeader.dwHeight,
					reinterpret_cast<const uint16_t*>(buf),
					expected_size, stride);
				break;

			case 24/8:
				// 24-bit RGB image.
				img = ImageDecoder::fromLinear24(
					px_format, ddsHeader.dwWidth, ddsHeader.dwHeight,
					buf, expected_size, stride);
				break;

			case sizeof(uint32_t):
				// 32-bit RGB image.
				img = ImageDecoder::fromLinear32(px_format,
					ddsHeader.dwWidth, ddsHeader.dwHeight,
					reinterpret_cast<const uint32_t*>(buf),
					expected_size, stride);
				break;

			default:
				// TODO: Implement other formats.
				assert(!"Unsupported pixel format.");
				break;
		}

		aligned_free(buf);
	}

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
	uint8_t header[4+sizeof(DDS_HEADER)+sizeof(DDS_HEADER_DXT10)];
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
		// Is this a DXT10 image?
		const DDS_HEADER *const pSrcHeader = reinterpret_cast<const DDS_HEADER*>(&header[4]);
		if (le32_to_cpu(pSrcHeader->ddspf.dwFourCC) == DDPF_FOURCC_DX10) {
			if (size < sizeof(header)) {
				// DXT10 header wasn't read.
				d->isValid = false;
				return;
			}

			// Save the DXT10 header.
			memcpy(&d->dxt10Header, &header[4+sizeof(DDS_HEADER)], sizeof(d->dxt10Header));

#if SYS_BYTEORDER == SYS_BIG_ENDIAN
			// Byteswap the DXT10 header.
			d->dxt10Header.dxgiFormat = (DXGI_FORMAT)le32_to_cpu((uint32_t)d->dxt10Header.dxgiFormat);
			d->dxt10Header.resourceDimension = (D3D10_RESOURCE_DIMENSION)le32_to_cpu((uint32_t)d->dxt10Header.resourceDimension);
			d->dxt10Header.miscFlag   = le32_to_cpu(d->dxt10Header.miscFlag);
			d->dxt10Header.arraySize  = le32_to_cpu(d->dxt10Header.arraySize);
			d->dxt10Header.miscFlags2 = le32_to_cpu(d->dxt10Header.miscFlags2);
#endif /* SYS_BYTEORDER == SYS_BIG_ENDIAN */

			// Texture data start address.
			d->texDataStartAddr = sizeof(header);
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
const rp_char *DirectDrawSurface::systemName(unsigned int type) const
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

	// DDS header.
	const DDS_HEADER *const ddsHeader = &d->ddsHeader;
	d->fields->reserve(8);	// Maximum of 8 fields.

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

	// Mipmap count.
	// NOTE: DDSD_MIPMAPCOUNT might not be accurate, so ignore it.
	d->fields->addField_string_numeric(_RP("Mipmap Count"),
		ddsHeader->dwMipMapCount, RomFields::FB_DEC, 0);

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
		const rp_char *pxfmt = d->getPixelFormatName(ddspf);
		if (pxfmt) {
			d->fields->addField_string(_RP("Pixel Format"), pxfmt);
		} else {
			d->fields->addField_string(_RP("Pixel Format"),
				rp_sprintf("RGB (%u-bit)", ddspf.dwRGBBitCount));
		}
	} else if (ddspf.dwFlags & DDPF_ALPHA) {
		// Alpha channel.
		const rp_char *pxfmt = d->getPixelFormatName(ddspf);
		if (pxfmt) {
			d->fields->addField_string(_RP("Pixel Format"), pxfmt);
		} else {
			d->fields->addField_string(_RP("Pixel Format"),
				rp_sprintf("Alpha (%u-bit)", ddspf.dwRGBBitCount));
		}
	} else if (ddspf.dwFlags & DDPF_YUV) {
		// YUV. (TODO: Determine the format.)
		d->fields->addField_string(_RP("Pixel Format"),
			rp_sprintf("YUV (%u-bit)", ddspf.dwRGBBitCount));
	} else if (ddspf.dwFlags & DDPF_LUMINANCE) {
		// Luminance.
		const rp_char *pxfmt = d->getPixelFormatName(ddspf);
		if (pxfmt) {
			d->fields->addField_string(_RP("Pixel Format"), pxfmt);
		} else {
			d->fields->addField_string(_RP("Pixel Format"),
				rp_sprintf("%s (%u-bit)",
					((ddspf.dwFlags & DDPF_ALPHAPIXELS)
						? "Luminance + Alpha"
						: "Luminance"),
					ddspf.dwRGBBitCount));
		}
	} else {
		// Unknown pixel format.
		d->fields->addField_string(_RP("Pixel Format"), _RP("Unknown"));
	}

	if (ddspf.dwFourCC == DDPF_FOURCC_DX10) {
		// DX10 texture.
		const DDS_HEADER_DXT10 *const dxt10Header = &d->dxt10Header;

		// Texture format.
		static const rp_char *const dx10_texFormat_tbl[] = {
			nullptr, _RP("R32G32B32A32_TYPELESS"),				// 0,1
			_RP("R32G32B32A32_FLOAT"), _RP("R32G32B32A32_UINT"),		// 2,3
			_RP("R32G32B32A32_SINT"), _RP("R32G32B32_TYPELESS"),		// 4,5
			_RP("R32G32B32_FLOAT"),	_RP("R32G32B32_UINT"),			// 6,7
			_RP("R32G32B32_SINT"), _RP("R16G16B16A16_TYPELESS"),		// 8,9
			_RP("R16G16B16A16_FLOAT"), _RP("R16G16B16A16_UNORM"),		// 10,11
			_RP("R16G16B16A16_UINT"), _RP("R16G16B16A16_SNORM"),		// 12,13
			_RP("R16G16B16A16_SINT"), _RP("R32G32_TYPELESS"),		// 14,15
			_RP("R32G32_FLOAT"), _RP("R32G32_UINT"),			// 16,17
			_RP("R32G32_SINT"), _RP("R32G8X24_TYPELESS"),			// 18,19
			_RP("D32_FLOAT_S8X24_UINT"), _RP("R32_FLOAT_X8X24_TYPELESS"),	// 20,21
			_RP("X32_TYPELESS_G8X24_UINT"),	_RP("R10G10B10A2_TYPELESS"),	// 22,23
			_RP("R10G10B10A2_UNORM"), _RP("R10G10B10A2_UINT"),		// 24,25
			_RP("R11G11B10_FLOAT"), _RP("R8G8B8A8_TYPELESS"),		// 26,27
			_RP("R8G8B8A8_UNORM"), _RP("R8G8B8A8_UNORM_SRGB"),		// 28,29
			_RP("R8G8B8A8_UINT"), _RP("R8G8B8A8_SNORM"),			// 30,31
			_RP("R8G8B8A8_SINT"), _RP("R16G16_TYPELESS"),			// 32,33
			_RP("R16G16_FLOAT"), _RP("R16G16_UNORM"),			// 34,35
			_RP("R16G16_UINT"), _RP("R16G16_SNORM"),			// 36,37
			_RP("R16G16_SINT"), _RP("R32_TYPELESS"),			// 38,39
			_RP("D32_FLOAT"), _RP("R32_FLOAT"),				// 40,41
			_RP("R32_UINT"), _RP("R32_SINT"),				// 42,43
			_RP("R24G8_TYPELESS"), _RP("D24_UNORM_S8_UINT"),		// 44,45
			_RP("R24_UNORM_X8_TYPELESS"), _RP("X24_TYPELESS_G8_UINT"),	// 46,47
			_RP("R8G8_TYPELESS"), _RP("R8G8_UNORM"),			// 48,49
			_RP("R8G8_UINT"), _RP("R8G8_SNORM"),				// 50,51
			_RP("R8G8_SINT"), _RP("R16_TYPELESS"),				// 52,53
			_RP("R16_FLOAT"), _RP("D16_UNORM"),				// 54,55
			_RP("R16_UNORM"), _RP("R16_UINT"),				// 56,57
			_RP("R16_SNORM"), _RP("R16_SINT"),				// 58,59
			_RP("R8_TYPELESS"), _RP("R8_UNORM"),				// 60,61
			_RP("R8_UINT"), _RP("R8_SNORM"),				// 62,63
			_RP("R8_SINT"), _RP("A8_UNORM"),				// 64,65
			_RP("R1_UNORM"), _RP("R9G9B9E5_SHAREDEXP"),			// 66,67
			_RP("R8G8_B8G8_UNORM"), _RP("G8R8_G8B8_UNORM"),			// 68,69
			_RP("BC1_TYPELESS"), _RP("BC1_UNORM"),				// 70,71
			_RP("BC1_UNORM_SRGB"), _RP("BC2_TYPELESS"),			// 72,73
			_RP("BC2_UNORM"), _RP("BC2_UNORM_SRGB"),			// 74,75
			_RP("BC3_TYPELESS"), _RP("BC3_UNORM"),				// 76,77
			_RP("BC3_UNORM_SRGB"), _RP("BC4_TYPELESS"),			// 78,79
			_RP("BC4_UNORM"), _RP("BC4_SNORM"),				// 80,81
			_RP("BC5_TYPELESS"), _RP("BC5_UNORM"),				// 82,83
			_RP("BC5_SNORM"), _RP("B5G6R5_UNORM"),				// 84,85
			_RP("B5G5R5A1_UNORM"), _RP("B8G8R8A8_UNORM"),			// 86,87
			_RP("B8G8R8X8_UNORM"), _RP("R10G10B10_XR_BIAS_A2_UNORM"),	// 88,89
			_RP("B8G8R8A8_TYPELESS"), _RP("B8G8R8A8_UNORM_SRGB"),		// 90,91
			_RP("B8G8R8X8_TYPELESS"), _RP("B8G8R8X8_UNORM_SRGB"),		// 92,93
			_RP("BC6H_TYPELESS"), _RP("BC6H_UF16"),				// 94,95
			_RP("BC6H_SF16"), _RP("BC7_TYPELESS"),				// 96,97
			_RP("BC7_UNORM"), _RP("BC7_UNORM_SRGB"),			// 98,99
			_RP("AYUV"), _RP("Y410"), _RP("Y416"), _RP("NV12"),		// 100-103
			_RP("P010"), _RP("P016"), _RP("420_OPAQUE"), _RP("YUY2"),	// 104-107
			_RP("Y210"), _RP("Y216"), _RP("NV11"), _RP("AI44"),		// 108-111
			_RP("IA44"), _RP("P8"), _RP("A8P8"), _RP("B4G4R4A4_UNORM"),	// 112-115
			nullptr, nullptr, nullptr, nullptr,				// 116-119
			nullptr, nullptr, nullptr, nullptr,				// 120-123
			nullptr, nullptr, nullptr, nullptr,				// 124-127
			nullptr, nullptr,						// 128,129
			_RP("P208"), _RP("V208"), _RP("V408"),				// 130-132
		};

		const rp_char *texFormat = nullptr;
		if (dxt10Header->dxgiFormat > 0 && dxt10Header->dxgiFormat < ARRAY_SIZE(dx10_texFormat_tbl)) {
			texFormat = dx10_texFormat_tbl[dxt10Header->dxgiFormat];
		} else if (dxt10Header->dxgiFormat == DXGI_FORMAT_FORCE_UINT) {
			texFormat = _RP("FORCE_UINT");
		}
		d->fields->addField_string(_RP("DX10 Format"),
			(texFormat ? texFormat : rp_sprintf("Unknown (0x%08X)", dxt10Header->dxgiFormat)));
	}

	// dwFlags
	static const rp_char *const dwFlags_names[] = {
		_RP("Caps"), _RP("Height"), _RP("Width"), _RP("Pitch"),		// 0x1-0x8
		nullptr, nullptr, nullptr, nullptr,				// 0x10-0x80
		nullptr, nullptr, nullptr, nullptr,				// 0x100-0x800
		_RP("Pixel Format"), nullptr, nullptr, nullptr,			// 0x1000-0x8000
		nullptr, _RP("Mipmap Count"), nullptr, _RP("Linear Size"),	// 0x10000-0x80000
		nullptr, nullptr, nullptr, _RP("Depth")				// 0x100000-0x800000
	};
	vector<rp_string> *v_dwFlags_names = RomFields::strArrayToVector(
		dwFlags_names, ARRAY_SIZE(dwFlags_names));
	d->fields->addField_bitfield(_RP("Flags"),
		v_dwFlags_names, 3, ddsHeader->dwFlags);

	// dwCaps
	static const rp_char *const dwCaps_names[] = {
		nullptr, nullptr, nullptr, _RP("Complex"),	// 0x1-0x8
		nullptr, nullptr, nullptr, nullptr,		// 0x10-0x80
		nullptr, nullptr, nullptr, nullptr,		// 0x100-0x800
		_RP("Texture"), nullptr, nullptr, nullptr,	// 0x1000-0x8000
		nullptr, nullptr, nullptr, nullptr,		// 0x10000-0x80000
		nullptr, nullptr, _RP("Mipmap")			// 0x100000-0x400000
	};
	vector<rp_string> *v_dwCaps_names = RomFields::strArrayToVector(
		dwCaps_names, ARRAY_SIZE(dwCaps_names));
	d->fields->addField_bitfield(_RP("Caps"),
		v_dwCaps_names, 3, ddsHeader->dwCaps);

	// dwCaps2
	static const rp_char *const dwCaps2_names[] = {
		nullptr, nullptr, nullptr, nullptr,		// 0x1-0x8
		nullptr, nullptr, nullptr, nullptr,		// 0x10-0x80
		nullptr, _RP("Cubemap"), _RP("+X"), _RP("-X"),	// 0x100-0x800
		_RP("+Y"), _RP("-Y"), _RP("+Z"), _RP("-Z"),	// 0x1000-0x8000
		nullptr, nullptr, nullptr, nullptr,		// 0x10000-0x80000
		nullptr, _RP("Volume")				// 0x100000-0x200000
	};
	vector<rp_string> *v_dwCaps2_names = RomFields::strArrayToVector(
		dwCaps2_names, ARRAY_SIZE(dwCaps2_names));
	d->fields->addField_bitfield(_RP("Caps2"),
		v_dwCaps2_names, 4, ddsHeader->dwCaps2);

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
