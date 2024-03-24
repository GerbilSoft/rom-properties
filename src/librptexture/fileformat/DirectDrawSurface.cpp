/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * DirectDrawSurface.hpp: DirectDraw Surface image reader.                 *
 *                                                                         *
 * Copyright (c) 2017-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "config.librptexture.h"

#include "DirectDrawSurface.hpp"
#include "FileFormat_p.hpp"

#include "dds_structs.h"
#include "data/DX10Formats.hpp"

// Other rom-properties libraries
#include "libi18n/i18n.h"
#include "librptext/fourCC.hpp"
using namespace LibRpFile;
using LibRpBase::RomFields;
using LibRpText::rp_sprintf;

// librptexture
#include "ImageSizeCalc.hpp"
#include "img/rp_image.hpp"
#include "decoder/ImageDecoder_Linear.hpp"
#include "decoder/ImageDecoder_S3TC.hpp"
#include "decoder/ImageDecoder_BC7.hpp"
#include "decoder/ImageDecoder_PVRTC.hpp"
#include "decoder/ImageDecoder_ASTC.hpp"

// C++ STL classes
using std::string;
using std::vector;

namespace LibRpTexture {

class DirectDrawSurfacePrivate final : public FileFormatPrivate
{
	public:
		DirectDrawSurfacePrivate(DirectDrawSurface *q, const IRpFilePtr &file);
		~DirectDrawSurfacePrivate() final = default;

	private:
		typedef FileFormatPrivate super;
		RP_DISABLE_COPY(DirectDrawSurfacePrivate)

	public:
		/** TextureInfo **/
		static const char *const exts[];
		static const char *const mimeTypes[];
		static const TextureInfo textureInfo;

	public:
		// DDS header
		DDS_HEADER ddsHeader;
		DDS_HEADER_DXT10 dxt10Header;
		DDS_HEADER_XBOX xb1Header;

		// Texture data start address
		unsigned int texDataStartAddr;

		// Decoded mipmaps
		// Mipmap 0 is the full image.
		vector<rp_image_ptr> mipmaps;

		// Pixel format message
		// NOTE: Used for both valid and invalid pixel formats
		// due to various bit specifications.
		char pixel_format[32];

		/**
		 * Calculate the expected image size.
		 *
		 * Width/height must be specified here for mipmap support.
		 * Other values are determined from the DDS header.
		 *
		 * @param width		[in]
		 * @param height	[in]
		 * @param mip		[in] Mipmap level (for stride calculation)
		 * @param pStride	[out] Stride (for uncompressed formats)
		 * @return Expected image size, or 0 on error
		 */
		unsigned int calcExpectedSize(int width, int height, int mip, unsigned int *pStride);

		/**
		 * Load the image.
		 * @param mip Mipmap number. (0 == full image)
		 * @return Image, or nullptr on error.
		 */
		rp_image_const_ptr loadImage(int mip);

	public:
		// Supported uncompressed RGB formats.
		struct RGB_Format_Table_t {
			uint32_t Rmask;
			uint32_t Gmask;
			uint32_t Bmask;
			uint32_t Amask;
			char desc[15];
			ImageDecoder::PixelFormat px_format;	// ImageDecoder::PixelFormat
		};
		ASSERT_STRUCT(RGB_Format_Table_t, sizeof(uint32_t)*4 + 15 + 1);

		static const RGB_Format_Table_t rgb_fmt_tbl_8[];	// 8-bit RGB
		static const RGB_Format_Table_t rgb_fmt_tbl_16[];	// 16-bit RGB
		static const RGB_Format_Table_t rgb_fmt_tbl_24[];	// 24-bit RGB
		static const RGB_Format_Table_t rgb_fmt_tbl_32[];	// 32-bit RGB
		static const RGB_Format_Table_t rgb_fmt_tbl_luma[];	// Luminance
		static const RGB_Format_Table_t rgb_fmt_tbl_alpha[];	// Alpha

		/**
		 * Get an RGB_Format_Table_t entry from an RGB format table.
		 * @param ddspf DirectDraw Surface pixel format
		 * @return Pointer to RGB_Format_Table_t, or nullptr if not found.
		 */
		static const RGB_Format_Table_t *getRGBFormatTableEntry(const DDS_PIXELFORMAT &ddspf);

		// Image format identifiers.
		ImageDecoder::PixelFormat pxf_uncomp;	// Pixel format for uncompressed images. (If 0, compressed.)
		uint8_t bytespp;	// Bytes per pixel. (Uncompressed only; set to 0 for compressed.)
		uint8_t dxgi_format;	// DXGI_FORMAT for compressed images. (If 0, uncompressed.)
		uint8_t dxgi_alpha;	// DDS_DXT10_MISC_FLAGS2 - alpha format.

		/**
		 * Get the format name of an uncompressed DirectDraw surface pixel format.
		 * @param ddspf DDS_PIXELFORMAT
		 * @return Format name, or nullptr if not supported.
		 */
		static inline const char *getPixelFormatName(const DDS_PIXELFORMAT &ddspf);

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

FILEFORMAT_IMPL(DirectDrawSurface)

/** DirectDrawSurfacePrivate **/

/* TextureInfo */
const char *const DirectDrawSurfacePrivate::exts[] = {
	".dds",	// DirectDraw Surface

	nullptr
};
const char *const DirectDrawSurfacePrivate::mimeTypes[] = {
	// Vendor-specific MIME types.
	// TODO: Get these upstreamed on FreeDesktop.org.
	"image/vnd.ms-dds",

	// Unofficial MIME types from FreeDesktop.org.
	"image/x-dds",

	nullptr
};
const TextureInfo DirectDrawSurfacePrivate::textureInfo = {
	exts, mimeTypes
};

// Supported 16-bit uncompressed RGB formats
const DirectDrawSurfacePrivate::RGB_Format_Table_t DirectDrawSurfacePrivate::rgb_fmt_tbl_8[] = {
	// 3-bit for R and G; 2-bit for B
	{0xE0, 0x1C, 0x03, 0x00, "RGB332", ImageDecoder::PixelFormat::RGB332},
};

// Supported 16-bit uncompressed RGB formats
const DirectDrawSurfacePrivate::RGB_Format_Table_t DirectDrawSurfacePrivate::rgb_fmt_tbl_16[] = {
	// 5-bit per channel, plus alpha.
	{0x7C00, 0x03E0, 0x001F, 0x8000, "ARGB1555", ImageDecoder::PixelFormat::ARGB1555},
	{0x001F, 0x03E0, 0x007C, 0x8000, "ABGR1555", ImageDecoder::PixelFormat::ABGR1555},
	{0xF800, 0x07C0, 0x003E, 0x0001, "RGBA5551", ImageDecoder::PixelFormat::RGBA5551},
	{0x003E, 0x03E0, 0x00F8, 0x0001, "BGRA5551", ImageDecoder::PixelFormat::BGRA5551},

	// 5-bit per RB channel, 6-bit per G channel, without alpha.
	{0xF800, 0x07E0, 0x001F, 0x0000, "RGB565", ImageDecoder::PixelFormat::RGB565},
	{0x001F, 0x07E0, 0xF800, 0x0000, "BGR565", ImageDecoder::PixelFormat::BGR565},

	// 5-bit per channel, without alpha.
	// (Technically 15-bit, but DDS usually lists it as 16-bit.)
	{0x7C00, 0x03E0, 0x001F, 0x0000, "RGB555", ImageDecoder::PixelFormat::RGB555},
	{0x001F, 0x03E0, 0x7C00, 0x0000, "BGR555", ImageDecoder::PixelFormat::BGR555},

	// 4-bit per channel formats. (uncommon nowadays) (alpha)
	{0x0F00, 0x00F0, 0x000F, 0xF000, "ARGB4444", ImageDecoder::PixelFormat::ARGB4444},
	{0x000F, 0x00F0, 0x0F00, 0xF000, "ABGR4444", ImageDecoder::PixelFormat::ABGR4444},
	{0xF000, 0x0F00, 0x00F0, 0x000F, "RGBA4444", ImageDecoder::PixelFormat::RGBA4444},
	{0x00F0, 0x0F00, 0xF000, 0x000F, "BGRA4444", ImageDecoder::PixelFormat::BGRA4444},
	// 4-bit per channel formats. (uncommon nowadays) (no alpha)
	{0x0F00, 0x00F0, 0x000F, 0x0000, "xRGB4444", ImageDecoder::PixelFormat::xRGB4444},
	{0x000F, 0x00F0, 0x0F00, 0x0000, "xBGR4444", ImageDecoder::PixelFormat::xBGR4444},
	{0xF000, 0x0F00, 0x00F0, 0x0000, "RGBx4444", ImageDecoder::PixelFormat::RGBx4444},
	{0x00F0, 0x0F00, 0xF000, 0x0000, "BGRx4444", ImageDecoder::PixelFormat::BGRx4444},

	// Other uncommon 16-bit formats.
	{0x00E0, 0x001C, 0x0003, 0xFF00, "ARGB8332", ImageDecoder::PixelFormat::ARGB8332},
};

// Supported 24-bit uncompressed RGB formats
const DirectDrawSurfacePrivate::RGB_Format_Table_t DirectDrawSurfacePrivate::rgb_fmt_tbl_24[] = {
	{0x00FF0000, 0x0000FF00, 0x000000FF, 0x00000000, "RGB888", ImageDecoder::PixelFormat::RGB888},
	{0x000000FF, 0x0000FF00, 0x00FF0000, 0x00000000, "BGR888", ImageDecoder::PixelFormat::BGR888},
};

// Supported 32-bit uncompressed RGB formats
const DirectDrawSurfacePrivate::RGB_Format_Table_t DirectDrawSurfacePrivate::rgb_fmt_tbl_32[] = {
	// Alpha
	{0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000, "ARGB8888", ImageDecoder::PixelFormat::ARGB8888},
	{0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000, "ABGR8888", ImageDecoder::PixelFormat::ABGR8888},
	{0x00FF0000, 0x0000FF00, 0x000000FF, 0x000000FF, "RGBA8888", ImageDecoder::PixelFormat::RGBA8888},
	{0x000000FF, 0x0000FF00, 0x00FF0000, 0x000000FF, "BGRA8888", ImageDecoder::PixelFormat::BGRA8888},

	// No alpha
	{0x00FF0000, 0x0000FF00, 0x000000FF, 0x00000000, "xRGB8888", ImageDecoder::PixelFormat::xRGB8888},
	{0x000000FF, 0x0000FF00, 0x00FF0000, 0x00000000, "xBGR8888", ImageDecoder::PixelFormat::xBGR8888},
	{0x00FF0000, 0x0000FF00, 0x000000FF, 0x00000000, "RGBx8888", ImageDecoder::PixelFormat::RGBx8888},
	{0x000000FF, 0x0000FF00, 0x00FF0000, 0x00000000, "BGRx8888", ImageDecoder::PixelFormat::BGRx8888},

	// Uncommon 32-bit formats.
	{0x0000FFFF, 0xFFFF0000, 0x00000000, 0x00000000, "G16R16", ImageDecoder::PixelFormat::G16R16},

	{0x3FF00000, 0x000FFC00, 0x000003FF, 0xC0000000, "A2R10G10B10", ImageDecoder::PixelFormat::A2R10G10B10},
	{0x000003FF, 0x000FFC00, 0x3FF00000, 0xC0000000, "A2B10G10R10", ImageDecoder::PixelFormat::A2B10G10R10},
};

// Supported luminance formats.
const DirectDrawSurfacePrivate::RGB_Format_Table_t DirectDrawSurfacePrivate::rgb_fmt_tbl_luma[] = {
	// 8-bit
	// TODO: Verify A4L4.
	{0x00FF, 0x0000, 0x0000, 0x0000, "L8",   ImageDecoder::PixelFormat::L8},
	{0x00FF, 0x00FF, 0x00FF, 0x0000, "L8",   ImageDecoder::PixelFormat::L8},
	{0xFF000000, 0xFF000000, 0xFF000000, 0x00000000, "L8",   ImageDecoder::PixelFormat::L8},	// from Pillow
	{0x000F, 0x0000, 0x0000, 0x00F0, "A4L4", ImageDecoder::PixelFormat::A4L4},
	{0x000F, 0x000F, 0x000F, 0x00F0, "A4L4", ImageDecoder::PixelFormat::A4L4},

	// 16-bit
	// TODO: Verify L16.
	{0xFFFF, 0x0000, 0x0000, 0x0000, "L16",  ImageDecoder::PixelFormat::L16},
	{0xFFFF, 0xFFFF, 0xFFFF, 0x0000, "L16",  ImageDecoder::PixelFormat::L16},
	{0x00FF, 0x0000, 0x0000, 0xFF00, "A8L8", ImageDecoder::PixelFormat::A8L8},
	{0x00FF, 0x00FF, 0x00FF, 0xFF00, "A8L8", ImageDecoder::PixelFormat::A8L8},			// from Pillow
};

// Supported alpha formats.
const DirectDrawSurfacePrivate::RGB_Format_Table_t DirectDrawSurfacePrivate::rgb_fmt_tbl_alpha[] = {
	// 8-bit
	{0x0000, 0x0000, 0x0000, 0x00FF, "A8", ImageDecoder::PixelFormat::A8},
};

/**
 * Get an RGB_Format_Table_t entry from an RGB format table.
 * @param ddspf DirectDraw Surface pixel format
 * @return Pointer to RGB_Format_Table_t, or nullptr if not found.
 */
const DirectDrawSurfacePrivate::RGB_Format_Table_t *DirectDrawSurfacePrivate::getRGBFormatTableEntry(const DDS_PIXELFORMAT &ddspf)
{
#ifndef NDEBUG
	static const unsigned int FORMATS = DDPF_ALPHA | DDPF_FOURCC | DDPF_RGB | DDPF_YUV | DDPF_LUMINANCE;
	assert(((ddspf.dwFlags & FORMATS) == DDPF_RGB) ||
	       ((ddspf.dwFlags & FORMATS) == DDPF_LUMINANCE) ||
	       ((ddspf.dwFlags & FORMATS) == DDPF_ALPHA));
#endif /* !NDEBUG */

	const RGB_Format_Table_t *entry = nullptr;
	const RGB_Format_Table_t *pTbl_end = nullptr;
	if (ddspf.dwFlags & DDPF_RGB) {
		switch (ddspf.dwRGBBitCount) {
			case 8:
				// 8-bit
				entry = rgb_fmt_tbl_8;
				pTbl_end = &rgb_fmt_tbl_8[ARRAY_SIZE(rgb_fmt_tbl_8)];
				break;
			case 15:
			case 16:
				// 16-bit
				entry = rgb_fmt_tbl_16;
				pTbl_end = &rgb_fmt_tbl_16[ARRAY_SIZE(rgb_fmt_tbl_16)];
				break;
			case 24:
				// 24-bit
				entry = rgb_fmt_tbl_24;
				pTbl_end = &rgb_fmt_tbl_24[ARRAY_SIZE(rgb_fmt_tbl_24)];
				break;
			case 32:
				// 32-bit
				entry = rgb_fmt_tbl_32;
				pTbl_end = &rgb_fmt_tbl_32[ARRAY_SIZE(rgb_fmt_tbl_32)];
				break;
			default:
				// Not supported
				return nullptr;
		}
	} else if (ddspf.dwFlags & DDPF_LUMINANCE) {
		// Luminance
		entry = rgb_fmt_tbl_luma;
		pTbl_end = &rgb_fmt_tbl_luma[ARRAY_SIZE(rgb_fmt_tbl_luma)];
	} else if (ddspf.dwFlags & DDPF_ALPHA) {
		// Alpha
		entry = rgb_fmt_tbl_alpha;
		pTbl_end = &rgb_fmt_tbl_alpha[ARRAY_SIZE(rgb_fmt_tbl_alpha)];
	} else {
		// Not supported
		return nullptr;
	}

	if (!entry) {
		// No table...
		return nullptr;
	}

	for (; entry < pTbl_end; entry++) {
		if (ddspf.dwRBitMask == entry->Rmask &&
		    ddspf.dwGBitMask == entry->Gmask &&
		    ddspf.dwBBitMask == entry->Bmask &&
		    ddspf.dwABitMask == entry->Amask)
		{
			// Found a match!
			return entry;
		}
	}

	// Not found.
	return nullptr;
}

/**
 * Get the format name of an uncompressed DirectDraw surface pixel format.
 * @param ddspf DDS_PIXELFORMAT
 * @return Format name, or nullptr if not supported.
 */
inline const char *DirectDrawSurfacePrivate::getPixelFormatName(const DDS_PIXELFORMAT &ddspf)
{
	const RGB_Format_Table_t *const entry = getRGBFormatTableEntry(ddspf);
	return (likely(entry != nullptr)) ? entry->desc : nullptr;
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
	assert(pxf_uncomp == ImageDecoder::PixelFormat::Unknown);
	assert(bytespp == 0);
	assert(dxgi_format == 0);
	assert(dxgi_alpha == DDS_ALPHA_MODE_UNKNOWN);

	pxf_uncomp = ImageDecoder::PixelFormat::Unknown;
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
		struct fourCC_dxgi_tbl_t {
			uint32_t dwFourCC;
			uint8_t dxgi_format;
			uint8_t dxgi_alpha;
		};
		static const std::array<fourCC_dxgi_tbl_t, 28> fourCC_dxgi_tbl = {{
			{DDPF_FOURCC_DXT1, DXGI_FORMAT_BC1_UNORM, DDS_ALPHA_MODE_STRAIGHT},
			{DDPF_FOURCC_DXT2, DXGI_FORMAT_BC2_UNORM, DDS_ALPHA_MODE_PREMULTIPLIED},
			{DDPF_FOURCC_DXT3, DXGI_FORMAT_BC2_UNORM, DDS_ALPHA_MODE_STRAIGHT},
			{DDPF_FOURCC_DXT4, DXGI_FORMAT_BC3_UNORM, DDS_ALPHA_MODE_PREMULTIPLIED},
			{DDPF_FOURCC_DXT5, DXGI_FORMAT_BC3_UNORM, DDS_ALPHA_MODE_STRAIGHT},

			{DDPF_FOURCC_ATI1, DXGI_FORMAT_BC4_UNORM, DDS_ALPHA_MODE_STRAIGHT},
			{DDPF_FOURCC_BC4U, DXGI_FORMAT_BC4_UNORM, DDS_ALPHA_MODE_STRAIGHT},
			{DDPF_FOURCC_BC4S, DXGI_FORMAT_BC4_UNORM, DDS_ALPHA_MODE_STRAIGHT},

			{DDPF_FOURCC_ATI2, DXGI_FORMAT_BC5_UNORM, DDS_ALPHA_MODE_STRAIGHT},
			{DDPF_FOURCC_BC5U, DXGI_FORMAT_BC5_UNORM, DDS_ALPHA_MODE_STRAIGHT},
			{DDPF_FOURCC_BC5S, DXGI_FORMAT_BC5_SNORM, DDS_ALPHA_MODE_STRAIGHT},

			// TODO: PVRTC no-alpha formats?
			{DDPF_FOURCC_PTC2, DXGI_FORMAT_FAKE_PVRTC_2bpp, DDS_ALPHA_MODE_STRAIGHT},
			{DDPF_FOURCC_PTC4, DXGI_FORMAT_FAKE_PVRTC_4bpp, DDS_ALPHA_MODE_STRAIGHT},

			{DDPF_FOURCC_ASTC4x4, DXGI_FORMAT_ASTC_4X4_UNORM, DDS_ALPHA_MODE_STRAIGHT},
			{DDPF_FOURCC_ASTC5x4, DXGI_FORMAT_ASTC_5X4_UNORM, DDS_ALPHA_MODE_STRAIGHT},
			{DDPF_FOURCC_ASTC5x5, DXGI_FORMAT_ASTC_5X5_UNORM, DDS_ALPHA_MODE_STRAIGHT},
			{DDPF_FOURCC_ASTC6x5, DXGI_FORMAT_ASTC_6X5_UNORM, DDS_ALPHA_MODE_STRAIGHT},
			{DDPF_FOURCC_ASTC6x6, DXGI_FORMAT_ASTC_6X6_UNORM, DDS_ALPHA_MODE_STRAIGHT},
			{DDPF_FOURCC_ASTC8x5, DXGI_FORMAT_ASTC_8X5_UNORM, DDS_ALPHA_MODE_STRAIGHT},
			{DDPF_FOURCC_ASTC8x6, DXGI_FORMAT_ASTC_8X6_UNORM, DDS_ALPHA_MODE_STRAIGHT},
			{DDPF_FOURCC_ASTC8x8, DXGI_FORMAT_ASTC_8X8_UNORM, DDS_ALPHA_MODE_STRAIGHT},
			{DDPF_FOURCC_ASTC10x5, DXGI_FORMAT_ASTC_10X5_UNORM, DDS_ALPHA_MODE_STRAIGHT},
			{DDPF_FOURCC_ASTC10x6, DXGI_FORMAT_ASTC_10X6_UNORM, DDS_ALPHA_MODE_STRAIGHT},
			{DDPF_FOURCC_ASTC10x8, DXGI_FORMAT_ASTC_10X8_UNORM, DDS_ALPHA_MODE_STRAIGHT},
			{DDPF_FOURCC_ASTC10x10, DXGI_FORMAT_ASTC_10X10_UNORM, DDS_ALPHA_MODE_STRAIGHT},
			{DDPF_FOURCC_ASTC12x10, DXGI_FORMAT_ASTC_12X10_UNORM, DDS_ALPHA_MODE_STRAIGHT},
			{DDPF_FOURCC_ASTC12x12, DXGI_FORMAT_ASTC_12X12_UNORM, DDS_ALPHA_MODE_STRAIGHT},

			// RXGB: DXT5 with swizzled channels
			{DDPF_FOURCC_RXGB, DXGI_FORMAT_BC3_UNORM, DDS_ALPHA_MODE_STRAIGHT},
		}};

		const uint32_t dwFourCC = ddspf.dwFourCC;	// to use by-value lambda capture
		auto fourCC_iter = std::find_if(fourCC_dxgi_tbl.cbegin(), fourCC_dxgi_tbl.cend(),
			[dwFourCC](const fourCC_dxgi_tbl_t &p) noexcept -> bool {
				return (p.dwFourCC == dwFourCC);
			});
		if (fourCC_iter != fourCC_dxgi_tbl.cend()) {
			// Found a match.
			dxgi_format = fourCC_iter->dxgi_format;
			dxgi_alpha = fourCC_iter->dxgi_alpha;
		}

		// TODO: Check DX10/XBOX before the other FourCCs?
		if (dxgi_format == 0 && (dwFourCC == DDPF_FOURCC_DX10 || dwFourCC == DDPF_FOURCC_XBOX)) {
			// Check the DX10 format.
			// TODO: Handle typeless, signed, sRGB, float.
			dxgi_format = dxt10Header.dxgiFormat;
			dxgi_alpha = dxt10Header.miscFlags2;

			struct dx10_tbl_t {
				uint8_t dxgi_format;
				ImageDecoder::PixelFormat pxf_uncomp;
				uint8_t bytespp;
			};
			static const std::array<dx10_tbl_t, 25> dx10_tbl = {{
				{DXGI_FORMAT_R10G10B10A2_TYPELESS,	ImageDecoder::PixelFormat::A2B10G10R10, 4},
				{DXGI_FORMAT_R10G10B10A2_UNORM,		ImageDecoder::PixelFormat::A2B10G10R10, 4},
				{DXGI_FORMAT_R10G10B10A2_UINT,		ImageDecoder::PixelFormat::A2B10G10R10, 4},

				{DXGI_FORMAT_R8G8B8A8_TYPELESS,		ImageDecoder::PixelFormat::ABGR8888, 4},
				{DXGI_FORMAT_R8G8B8A8_UNORM,		ImageDecoder::PixelFormat::ABGR8888, 4},
				{DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,	ImageDecoder::PixelFormat::ABGR8888, 4},
				{DXGI_FORMAT_R8G8B8A8_UINT,		ImageDecoder::PixelFormat::ABGR8888, 4},
				//{DXGI_FORMAT_R8G8B8A8_SNORM,		ImageDecoder::PixelFormat::ABGR8888, 4},
				//{DXGI_FORMAT_R8G8B8A8_SINT,		ImageDecoder::PixelFormat::ABGR8888, 4},

				{DXGI_FORMAT_R16G16_TYPELESS,		ImageDecoder::PixelFormat::G16R16, 4},
				{DXGI_FORMAT_R16G16_FLOAT,		ImageDecoder::PixelFormat::G16R16, 4},
				{DXGI_FORMAT_R16G16_UNORM,		ImageDecoder::PixelFormat::G16R16, 4},
				{DXGI_FORMAT_R16G16_UINT,		ImageDecoder::PixelFormat::G16R16, 4},
				//{DXGI_FORMAT_R16G16_SNORM,		ImageDecoder::PixelFormat::G16R16, 4},
				//{DXGI_FORMAT_R16G16_SINT,		ImageDecoder::PixelFormat::G16R16, 4},

				{DXGI_FORMAT_R8G8_TYPELESS,		ImageDecoder::PixelFormat::GR88, 2},
				{DXGI_FORMAT_R8G8_UNORM,		ImageDecoder::PixelFormat::GR88, 2},
				{DXGI_FORMAT_R8G8_UINT,			ImageDecoder::PixelFormat::GR88, 2},
				//{DXGI_FORMAT_R8G8_SNORM,		ImageDecoder::PixelFormat::GR88, 2},
				//{DXGI_FORMAT_R8G8_SINT,		ImageDecoder::PixelFormat::GR88, 2},

				{DXGI_FORMAT_A8_UNORM,			ImageDecoder::PixelFormat::A8, 1},

				{DXGI_FORMAT_R9G9B9E5_SHAREDEXP,	ImageDecoder::PixelFormat::RGB9_E5, 4},

				{DXGI_FORMAT_B5G6R5_UNORM,		ImageDecoder::PixelFormat::RGB565, 2},
				{DXGI_FORMAT_B5G5R5A1_UNORM,		ImageDecoder::PixelFormat::ARGB1555, 2},

				{DXGI_FORMAT_B8G8R8A8_UNORM,		ImageDecoder::PixelFormat::ARGB8888, 4},
				{DXGI_FORMAT_B8G8R8A8_TYPELESS,		ImageDecoder::PixelFormat::ARGB8888, 4},
				{DXGI_FORMAT_B8G8R8A8_UNORM_SRGB,	ImageDecoder::PixelFormat::ARGB8888, 4},

				{DXGI_FORMAT_B8G8R8X8_UNORM,		ImageDecoder::PixelFormat::xRGB8888, 4},
				{DXGI_FORMAT_B8G8R8X8_TYPELESS,		ImageDecoder::PixelFormat::xRGB8888, 4},
				{DXGI_FORMAT_B8G8R8X8_UNORM_SRGB,	ImageDecoder::PixelFormat::xRGB8888, 4},

				{DXGI_FORMAT_B4G4R4A4_UNORM,		ImageDecoder::PixelFormat::ARGB4444, 2},
			}};

			// If the dxgi_format is not listed in the table, we'll use it
			// as-is, assuming it's compressed.
			const uint8_t dxgi_format_tmp = dxgi_format;	// needed for lambda capture
			auto dx10_iter = std::find_if(dx10_tbl.cbegin(), dx10_tbl.cend(),
				[dxgi_format_tmp](const dx10_tbl_t &p) noexcept -> bool {
					return (p.dxgi_format == dxgi_format_tmp);
				});
			if (dx10_iter != dx10_tbl.cend()) {
				// Found a match.
				pxf_uncomp = dx10_iter->pxf_uncomp;
				bytespp = dx10_iter->bytespp;
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
		const RGB_Format_Table_t *const entry = getRGBFormatTableEntry(ddspf);
		if (!entry) {
			// No table...
			dxgi_alpha = DDS_ALPHA_MODE_UNKNOWN;
			return -ENOTSUP;
		}

		// Found a match!
		pxf_uncomp = entry->px_format;
		bytespp = (ddspf.dwRGBBitCount == 15 ? 2 : (ddspf.dwRGBBitCount / 8));
		dxgi_alpha = (ddspf.dwABitMask != 0 ? DDS_ALPHA_MODE_STRAIGHT : DDS_ALPHA_MODE_OPAQUE);

		// TODO: For DDPF_LUMINANCE, we're always setting DDS_ALPHA_MODE_OPAQUE.
		// Set it to standard alpha if it's Luma+Alpha?
		if (ddspf.dwFlags & DDPF_LUMINANCE) {
			dxgi_alpha = DDS_ALPHA_MODE_OPAQUE;
		}
	}

	return ret;
}

DirectDrawSurfacePrivate::DirectDrawSurfacePrivate(DirectDrawSurface *q, const IRpFilePtr &file)
	: super(q, file, &textureInfo)
	, texDataStartAddr(0)
	, pxf_uncomp(ImageDecoder::PixelFormat::Unknown)
	, bytespp(0)
	, dxgi_format(0)
	, dxgi_alpha(DDS_ALPHA_MODE_UNKNOWN)
{
	// Clear the DDS header structs.
	memset(&ddsHeader, 0, sizeof(ddsHeader));
	memset(&dxt10Header, 0, sizeof(dxt10Header));
	memset(&xb1Header, 0, sizeof(xb1Header));
	memset(pixel_format, 0, sizeof(pixel_format));
}

/**
 * Calculate the expected image size.
 *
 * Width/height must be specified here for mipmap support.
 * Other values are determined from the DDS header.
 *
 * @param width		[in]
 * @param height	[in]
 * @param mip		[in] Mipmap level (for stride calculation)
 * @param pStride	[out] Stride (for uncompressed formats)
 * @return Expected image size, or 0 on error
 */
unsigned int DirectDrawSurfacePrivate::calcExpectedSize(int width, int height, int mip, unsigned int *pStride)
{
	if (pxf_uncomp != ImageDecoder::PixelFormat::Unknown) {
		// Uncompressed linear image data.
		assert(pxf_uncomp != ImageDecoder::PixelFormat::Unknown);
		assert(bytespp != 0);
		if (pxf_uncomp == ImageDecoder::PixelFormat::Unknown || bytespp == 0) {
			// Pixel format wasn't updated...
			return 0;
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
			stride = width * bytespp;
		} else if (mip > 0) {
			// Adjust the stride for mipmaps.
			stride >>= static_cast<unsigned int>(mip * 2);
		}

		if (stride > static_cast<unsigned int>(width * 16)) {
			// Stride is too large.
			return 0;
		}

		if (pStride) {
			*pStride = stride;
		}
		return static_cast<unsigned int>(height) * stride;
	}

	// Compressed RGB data.

	// NOTE: dwPitchOrLinearSize is not necessarily correct.
	// Calculate the expected size.
	unsigned int expected_size = 0;
	if (pStride) {
		*pStride = 0;
	}
	switch (dxgi_format) {
#ifdef ENABLE_PVRTC
		case DXGI_FORMAT_FAKE_PVRTC_2bpp:
			// 32 pixels compressed into 64 bits. (2bpp)
			// NOTE: Image dimensions must be a power of 2 for PVRTC-I.
			expected_size = ImageSizeCalc::T_calcImageSizePVRTC_PoT<true>(width, height);
			break;

		case DXGI_FORMAT_FAKE_PVRTC_4bpp:
			// 16 pixels compressed into 64 bits. (4bpp)
			// NOTE: Image dimensions must be a power of 2 for PVRTC-I.
			expected_size = ImageSizeCalc::T_calcImageSizePVRTC_PoT<false>(width, height);
			break;
#endif /* ENABLE_PVRTC */

		case DXGI_FORMAT_BC1_TYPELESS:
		case DXGI_FORMAT_BC1_UNORM:
		case DXGI_FORMAT_BC1_UNORM_SRGB:
		case DXGI_FORMAT_BC4_TYPELESS:
		case DXGI_FORMAT_BC4_UNORM:
		//case DXGI_FORMAT_BC4_SNORM:
			// 16 pixels compressed into 64 bits. (4bpp)
			// NOTE: Width and height must be rounded to the nearest tile. (4x4)
			expected_size = ImageSizeCalc::T_calcImageSize(ALIGN_BYTES(4, width), ALIGN_BYTES(4, height)) / 2;
			break;

		case DXGI_FORMAT_BC2_TYPELESS:
		case DXGI_FORMAT_BC2_UNORM:
		case DXGI_FORMAT_BC2_UNORM_SRGB:
		case DXGI_FORMAT_BC3_TYPELESS:
		case DXGI_FORMAT_BC3_UNORM:
		case DXGI_FORMAT_BC3_UNORM_SRGB:
		case DXGI_FORMAT_BC5_TYPELESS:
		case DXGI_FORMAT_BC5_UNORM:
		//case DXGI_FORMAT_BC5_SNORM:
		case DXGI_FORMAT_BC6H_TYPELESS:
		case DXGI_FORMAT_BC6H_UF16:
		//case DXGI_FORMAT_BC6H_SF16:
		case DXGI_FORMAT_BC7_TYPELESS:
		case DXGI_FORMAT_BC7_UNORM:
		case DXGI_FORMAT_BC7_UNORM_SRGB:
			// 16 pixels compressed into 128 bits. (8bpp)
			// NOTE: Width and height must be rounded to the nearest tile. (4x4)
			expected_size = ImageSizeCalc::T_calcImageSize(ALIGN_BYTES(4, width), ALIGN_BYTES(4, height));
			break;

		case DXGI_FORMAT_R9G9B9E5_SHAREDEXP:
			// Uncompressed "special" 32bpp formats.
			expected_size = ImageSizeCalc::T_calcImageSize(width, height, sizeof(uint32_t));
			break;

		default:
#ifdef ENABLE_ASTC
			if (dxgi_format >= DXGI_FORMAT_ASTC_4X4_TYPELESS &&
			    dxgi_format <= DXGI_FORMAT_ASTC_12X12_UNORM_SRGB)
			{
				static_assert(((DXGI_FORMAT_ASTC_12X12_TYPELESS - DXGI_FORMAT_ASTC_4X4_TYPELESS) / 4) + 1 ==
					ARRAY_SIZE(ImageDecoder::astc_lkup_tbl), "ASTC lookup table size is wrong!");
				const unsigned int astc_idx = (dxgi_format - DXGI_FORMAT_ASTC_4X4_TYPELESS) / 4;
				expected_size = ImageSizeCalc::calcImageSizeASTC(
					width, height,
					ImageDecoder::astc_lkup_tbl[astc_idx][0],
					ImageDecoder::astc_lkup_tbl[astc_idx][1]);
			}
#endif /* ENABLE_ASTC */
			break;
	}

	return expected_size;
}

/**
 * Load the image.
 * @param mip Mipmap number. (0 == full image)
 * @return Image, or nullptr on error.
 */
rp_image_const_ptr DirectDrawSurfacePrivate::loadImage(int mip)
{
	assert(mip >= 0);
	assert(mip < static_cast<int>(mipmaps.size()));
	if (mip < 0 || mip >= static_cast<int>(mipmaps.size())) {
		// Invalid mipmap number.
		return nullptr;
	}

	if (!mipmaps.empty() && mipmaps[mip] != nullptr) {
		// Image has already been loaded.
		return mipmaps[mip];
	} else if (!this->isValid || !this->file) {
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

	int width = ddsHeader.dwWidth;
	int height = ddsHeader.dwHeight;

	// If we're requesting a mipmap level higher than 0 (full image),
	// adjust the start address, expected size, and dimensions.
	// FIXME: Do cubemaps affect anything?
	unsigned int stride = 0;
	unsigned int start_addr = texDataStartAddr;
	unsigned int expected_size = calcExpectedSize(width, height, 0, &stride);
	assert(expected_size != 0);
	if (expected_size == 0) {
		// Could not calculate the expected size.
		return nullptr;
	}

	for (int adjmip = 1; adjmip <= mip; adjmip++) {
		width /= 2;
		height /= 2;

		assert(width > 0);
		assert(height > 0);
		if (width <= 0 || height <= 0) {
			// Mipmap size calculation error...
			return nullptr;
		}

		start_addr += expected_size;
		expected_size = calcExpectedSize(width, height, mip, &stride);
	}

	// Seek to the start of the texture data.
	int ret = file->seek(start_addr);
	if (ret != 0) {
		// Seek error.
		return nullptr;
	}

	// TODO: Handle DX10 alpha processing.
	// Currently, we're assuming straight alpha for formats
	// that have an alpha channel, except for DXT2 and DXT4,
	// which use premultiplied alpha.

	// TODO: Handle sRGB.
	// TODO: Handle signed textures.

	// NOTE: Mipmaps are stored *after* the main image.
	// Hence, no mipmap processing is necessary.
	rp_image_ptr img;
	if (pxf_uncomp == ImageDecoder::PixelFormat::Unknown) {
		// Compressed RGB data.

		// Verify file size.
		if (expected_size >= file_sz + start_addr) {
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
						width, height,
						buf.get(), expected_size);
				} else {
					// No alpha channel.
					img = ImageDecoder::fromDXT1(
						width, height,
						buf.get(), expected_size);
				}
				break;

			case DXGI_FORMAT_BC2_TYPELESS:
			case DXGI_FORMAT_BC2_UNORM:
			case DXGI_FORMAT_BC2_UNORM_SRGB:
				if (likely(dxgi_alpha != DDS_ALPHA_MODE_PREMULTIPLIED)) {
					// Standard alpha: DXT3
					img = ImageDecoder::fromDXT3(
						width, height,
						buf.get(), expected_size);
				} else {
					// Premultiplied alpha: DXT2
					img = ImageDecoder::fromDXT2(
						width, height,
						buf.get(), expected_size);
				}
				break;

			case DXGI_FORMAT_BC3_TYPELESS:
			case DXGI_FORMAT_BC3_UNORM:
			case DXGI_FORMAT_BC3_UNORM_SRGB:
				if (likely(dxgi_alpha != DDS_ALPHA_MODE_PREMULTIPLIED)) {
					// Standard alpha: DXT5
					img = ImageDecoder::fromDXT5(
						width, height,
						buf.get(), expected_size);
					switch (ddsHeader.ddspf.dwFourCC) {
						default:
							break;
						case DDPF_FOURCC_RXGB:
							// RxGB -> xRGB
							img->swizzle("agb1");
							break;
						case DDPF_FOURCC_DXT5:
							if (ddsHeader.ddspf.dwFlags & DDPF_NORMAL) {
								// DXT5nm (nVidia-specific)
								// Swap the Red and Alpha channels.
								img->swizzle("agbr");
							}
							break;
					}
				} else {
					// Premultiplied alpha: DXT4
					img = ImageDecoder::fromDXT4(
						width, height,
						buf.get(), expected_size);
				}
				break;

			case DXGI_FORMAT_BC4_TYPELESS:
			case DXGI_FORMAT_BC4_UNORM:
			//case DXGI_FORMAT_BC4_SNORM:
				img = ImageDecoder::fromBC4(
					width, height,
					buf.get(), expected_size);
				break;

			case DXGI_FORMAT_BC5_TYPELESS:
			case DXGI_FORMAT_BC5_UNORM:
			//case DXGI_FORMAT_BC5_SNORM:
				img = ImageDecoder::fromBC5(
					width, height,
					buf.get(), expected_size);
				break;

			case DXGI_FORMAT_BC6H_TYPELESS:
			case DXGI_FORMAT_BC6H_UF16:
			//case DXGI_FORMAT_BC6H_SF16:
				// TODO
				break;

			case DXGI_FORMAT_BC7_TYPELESS:
			case DXGI_FORMAT_BC7_UNORM:
			case DXGI_FORMAT_BC7_UNORM_SRGB:
				img = ImageDecoder::fromBC7(
					width, height,
					buf.get(), expected_size);
				break;

#ifdef ENABLE_PVRTC
			case DXGI_FORMAT_FAKE_PVRTC_2bpp:
				// PVRTC, 2bpp, has alpha.
				img = ImageDecoder::fromPVRTC(
					width, height,
					buf.get(), expected_size,
					ImageDecoder::PVRTC_2BPP | ImageDecoder::PVRTC_ALPHA_YES);
				break;

			case DXGI_FORMAT_FAKE_PVRTC_4bpp:
				// PVRTC, 4bpp, has alpha.
				img = ImageDecoder::fromPVRTC(
					width, height,
					buf.get(), expected_size,
					ImageDecoder::PVRTC_4BPP | ImageDecoder::PVRTC_ALPHA_YES);
				break;
#endif /* ENABLE_PVRTC */

			case DXGI_FORMAT_R9G9B9E5_SHAREDEXP:
				// RGB9_E5 (technically uncompressed...)
				img = ImageDecoder::fromLinear32(
					ImageDecoder::PixelFormat::RGB9_E5,
					width, height,
					reinterpret_cast<const uint32_t*>(buf.get()),
					expected_size);
				break;

			default:
#ifdef ENABLE_ASTC
				if (dxgi_format >= DXGI_FORMAT_ASTC_4X4_TYPELESS &&
				    dxgi_format <= DXGI_FORMAT_ASTC_12X12_UNORM_SRGB)
				{
					static_assert(((DXGI_FORMAT_ASTC_12X12_TYPELESS - DXGI_FORMAT_ASTC_4X4_TYPELESS) / 4) + 1 ==
						ARRAY_SIZE(ImageDecoder::astc_lkup_tbl), "ASTC lookup table size is wrong!");
					const unsigned int astc_idx = (dxgi_format - DXGI_FORMAT_ASTC_4X4_TYPELESS) / 4;
					img = ImageDecoder::fromASTC(
						width, height,
						buf.get(), expected_size,
						ImageDecoder::astc_lkup_tbl[astc_idx][0],
						ImageDecoder::astc_lkup_tbl[astc_idx][1]);
					break;
				}
#endif /* ENABLE_ASTC */

				// Not supported.
				break;
		}
	} else {
		// Uncompressed linear image data.
		assert(pxf_uncomp != ImageDecoder::PixelFormat::Unknown);
		assert(bytespp != 0);
		if (pxf_uncomp == ImageDecoder::PixelFormat::Unknown || bytespp == 0) {
			// Pixel format wasn't updated...
			return nullptr;
		}

		// Verify file size.
		if (expected_size >= file_sz + start_addr) {
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
					pxf_uncomp, width, height,
					buf.get(), expected_size, stride);
				break;

			case sizeof(uint16_t):
				// 16-bit RGB image.
				img = ImageDecoder::fromLinear16(
					pxf_uncomp, width, height,
					reinterpret_cast<const uint16_t*>(buf.get()),
					expected_size, stride);
				break;

			case 24/8:
				// 24-bit RGB image.
				img = ImageDecoder::fromLinear24(
					pxf_uncomp, width, height,
					buf.get(), expected_size, stride);
				break;

			case sizeof(uint32_t):
				// 32-bit RGB image.
				img = ImageDecoder::fromLinear32(
					pxf_uncomp, width, height,
					reinterpret_cast<const uint32_t*>(buf.get()),
					expected_size, stride);
				break;

			default:
				// TODO: Implement other formats.
				assert(!"Unsupported pixel format.");
				break;
		}
	}

	// Check if we need to unswizzle a GIMP-DDS texture.
	if (img && !memcmp(ddsHeader.gimp.magic, DDS_GIMP_MAGIC, sizeof(ddsHeader.gimp.magic))) {
		// TODO: Verify that the image format is ARGB32.
		switch (be32_to_cpu(ddsHeader.gimp.fourCC.u32)) {
			default:
				// Not supported...
				assert(!"Unsupported GIMP-DDS swizzle format.");
				break;
			case 0:
				// No swizzling.
				break;
			case DDS_GIMP_SWIZZLE_FOURCC_AEXP:
				img->unswizzle_AExp();
				break;
			case DDS_GIMP_SWIZZLE_FOURCC_YCG1:
				img->unswizzle_YCoCg();
				break;
			case DDS_GIMP_SWIZZLE_FOURCC_YCG2:
				img->unswizzle_YCoCg_scaled();
				break;
		}
	}

	// TODO: Untile textures for XBOX format.
	mipmaps[mip] = img;
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
DirectDrawSurface::DirectDrawSurface(const IRpFilePtr &file)
	: super(new DirectDrawSurfacePrivate(this, file))
{
	RP_D(DirectDrawSurface);
	d->mimeType = "image/x-dds";	// unofficial
	d->textureFormatName = "DirectDraw Surface";

	if (!d->file) {
		// Could not ref() the file handle.
		return;
	}

	// Read the DDS magic number and header.
	uint8_t header[4+sizeof(DDS_HEADER)+sizeof(DDS_HEADER_DXT10)+sizeof(DDS_HEADER_XBOX)];
	d->file->rewind();
	size_t size = d->file->read(header, sizeof(header));
	if (size < 4+sizeof(DDS_HEADER)) {
		d->file.reset();
		return;
	}

	// Check if this DDS texture is supported.
	const DetectInfo info = {
		{0, static_cast<uint32_t>(size), header},
		nullptr,	// ext (not needed for DirectDrawSurface)
		file->size()	// szFile
	};
	d->isValid = (isRomSupported_static(&info) >= 0);

	if (!d->isValid) {
		d->file.reset();
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
			d->file.reset();
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

		// Make sure the dxgiFormat is not one of our "fake" formats.
		// If it is, assume the texture isn't supported for now.
		assert(d->dxt10Header.dxgiFormat < DXGI_FORMAT_FAKE_START ||
		       d->dxt10Header.dxgiFormat > DXGI_FORMAT_FAKE_END);
		if (d->dxt10Header.dxgiFormat >= DXGI_FORMAT_FAKE_START &&
		    d->dxt10Header.dxgiFormat <= DXGI_FORMAT_FAKE_END)
		{
			// "Fake" format...
			d->file.reset();
			d->isValid = false;
			return;
		}

		// Texture data start address.
		d->texDataStartAddr = headerSize;
	} else {
		// No DXT10 header.
		d->texDataStartAddr = 4+sizeof(DDS_HEADER);
	}

#if SYS_BYTEORDER == SYS_BIG_ENDIAN
	// Byteswap and copy the DDS header fields.
	d->ddsHeader.dwSize			= le32_to_cpu(pSrcHeader->dwSize);
	d->ddsHeader.dwFlags			= le32_to_cpu(pSrcHeader->dwFlags);
	d->ddsHeader.dwHeight			= le32_to_cpu(pSrcHeader->dwHeight);
	d->ddsHeader.dwWidth			= le32_to_cpu(pSrcHeader->dwWidth);
	d->ddsHeader.dwPitchOrLinearSize	= le32_to_cpu(pSrcHeader->dwPitchOrLinearSize);
	d->ddsHeader.dwDepth			= le32_to_cpu(pSrcHeader->dwDepth);
	d->ddsHeader.dwMipMapCount		= le32_to_cpu(pSrcHeader->dwMipMapCount);

	// NOTE: Not byteswapping NVTT or GIMP headers. Copy as-is.
	// These fields are byteswapped when needed.
	memcpy(&d->ddsHeader.dwReserved1, &pSrcHeader->dwReserved1, sizeof(d->ddsHeader.dwReserved1));

	// Copy the DDS pixel format.
	// NOTE: FourCC is NOT byteswapped here; it's handled separately.
	DDS_PIXELFORMAT &ddspf = d->ddsHeader.ddspf;
	const DDS_PIXELFORMAT &ddspf_src = pSrcHeader->ddspf;
	ddspf.dwSize		= le32_to_cpu(ddspf_src.dwSize);
	ddspf.dwFlags		= le32_to_cpu(ddspf_src.dwFlags);
	ddspf.dwFourCC		= ddspf_src.dwFourCC;	// NO byteswap!
	ddspf.dwRGBBitCount	= le32_to_cpu(ddspf_src.dwRGBBitCount);
	ddspf.dwRBitMask	= le32_to_cpu(ddspf_src.dwRBitMask);
	ddspf.dwGBitMask	= le32_to_cpu(ddspf_src.dwGBitMask);
	ddspf.dwBBitMask	= le32_to_cpu(ddspf_src.dwBBitMask);
	ddspf.dwABitMask	= le32_to_cpu(ddspf_src.dwABitMask);

	// Copy the capabilities values.
	d->ddsHeader.dwCaps			= le32_to_cpu(pSrcHeader->dwCaps);
	d->ddsHeader.dwCaps2			= le32_to_cpu(pSrcHeader->dwCaps2);
	d->ddsHeader.dwCaps3			= le32_to_cpu(pSrcHeader->dwCaps3);
	d->ddsHeader.dwCaps4			= le32_to_cpu(pSrcHeader->dwCaps4);
#else /* SYS_BYTEORDER == SYS_LIL_ENDIAN */
	// Copy the DDS header without byteswapping.
	memcpy(&d->ddsHeader, pSrcHeader, sizeof(d->ddsHeader));

	// FourCC is considered to be big-endian.
	d->ddsHeader.ddspf.dwFourCC = be32_to_cpu(d->ddsHeader.ddspf.dwFourCC);
#endif

	// Update the pixel format.
	d->updatePixelFormat();

	// Cache the dimensions for the FileFormat base class.
	d->dimensions[0] = d->ddsHeader.dwWidth;
	d->dimensions[1] = d->ddsHeader.dwHeight;
	if (d->ddsHeader.dwFlags & DDSD_DEPTH) {
		d->dimensions[2] = d->ddsHeader.dwDepth;
	}

	// Save the mipmap count.
	d->mipmapCount = d->ddsHeader.dwMipMapCount;
	assert(d->mipmapCount >= 0);
	assert(d->mipmapCount <= 128);
	if (d->mipmapCount > 128) {
		// Too many mipmaps.
		d->isValid = false;
		d->file.reset();
		return;
	}
	d->mipmaps.resize(d->mipmapCount > 0 ? d->mipmapCount : 1);
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

/** Property accessors **/

/**
 * Get the pixel format, e.g. "RGB888" or "DXT1".
 * @return Pixel format, or nullptr if unavailable.
 */
const char *DirectDrawSurface::pixelFormat(void) const
{
	RP_D(const DirectDrawSurface);
	if (!d->isValid)
		return nullptr;

	if (d->pixel_format[0] != '\0') {
		// We already determined the pixel format.
		return d->pixel_format;
	}

	// Pixel format.
	DirectDrawSurfacePrivate *const d_nc = const_cast<DirectDrawSurfacePrivate*>(d);
	const DDS_PIXELFORMAT &ddspf = d->ddsHeader.ddspf;
	if (ddspf.dwFlags & DDPF_FOURCC) {
		// Compressed RGB data.
		// NOTE: If DX10, see dxgi_format.
		LibRpText::fourCCtoString(d_nc->pixel_format, sizeof(d_nc->pixel_format), ddspf.dwFourCC);
		return d_nc->pixel_format;
	}

	const char *const pxfmt = d->getPixelFormatName(ddspf);
	if (pxfmt) {
		// Got the pixel format name.
		strcpy(d_nc->pixel_format, pxfmt);
		return d_nc->pixel_format;
	}

	// Manually determine the pixel format.
	if (ddspf.dwFlags & DDPF_RGB) {
		// Uncompressed RGB data
		snprintf(d_nc->pixel_format, sizeof(d_nc->pixel_format),
			 "RGB (%u-bit)", ddspf.dwRGBBitCount);
	} else if (ddspf.dwFlags & DDPF_ALPHA) {
		// Alpha channel
		snprintf(d_nc->pixel_format, sizeof(d_nc->pixel_format),
			C_("DirectDrawSurface", "Alpha (%u-bit)"), ddspf.dwRGBBitCount);
	} else if (ddspf.dwFlags & DDPF_YUV) {
		// YUV (TODO: Determine the format.)
		snprintf(d_nc->pixel_format, sizeof(d_nc->pixel_format),
			C_("DirectDrawSurface", "YUV (%u-bit)"), ddspf.dwRGBBitCount);
	} else if (ddspf.dwFlags & DDPF_LUMINANCE) {
		// Luminance
		if (ddspf.dwFlags & DDPF_ALPHAPIXELS) {
			snprintf(d_nc->pixel_format, sizeof(d_nc->pixel_format),
				C_("DirectDrawSurface", "Luminance + Alpha (%u-bit)"),
				ddspf.dwRGBBitCount);
		} else {
			snprintf(d_nc->pixel_format, sizeof(d_nc->pixel_format),
				C_("DirectDrawSurface", "Luminance (%u-bit)"),
				ddspf.dwRGBBitCount);
		}
	} else {
		// Unknown pixel format
		strncpy(d_nc->pixel_format,
			C_("FileFormat", "Unknown"), sizeof(d_nc->pixel_format));
		d_nc->pixel_format[sizeof(d_nc->pixel_format)-1] = '\0';
	}

	return d->pixel_format;
}

#ifdef ENABLE_LIBRPBASE_ROMFIELDS
/**
 * Get property fields for rom-properties.
 * @param fields RomFields object to which fields should be added.
 * @return Number of fields added, or 0 on error.
 */
int DirectDrawSurface::getFields(RomFields *fields) const
{
	assert(fields != nullptr);
	if (!fields)
		return 0;

	RP_D(const DirectDrawSurface);
	if (!d->isValid) {
		// Unknown file type.
		return -EIO;
	}

	const int initial_count = fields->count();
	fields->reserve(initial_count + 11);	// Maximum of 11 fields

	// DDS header
	const DDS_HEADER *const ddsHeader = &d->ddsHeader;

	// Pitch (uncompressed)
	// Linear size (compressed)
	const char *const pitch_name = (ddsHeader->dwFlags & DDSD_LINEARSIZE)
		? C_("DirectDrawSurface", "Linear Size")
		: C_("DirectDrawSurface", "Pitch");
	fields->addField_string_numeric(
		dpgettext_expr(RP_I18N_DOMAIN, "DirectDrawSurface", pitch_name),
		ddsHeader->dwPitchOrLinearSize, RomFields::Base::Dec, 0);

	if (d->dxgi_format != 0) {
		// DX10 texture format.
		const char *const texFormat = DX10Formats::lookup_dxgiFormat(d->dxgi_format);
		fields->addField_string(C_("DirectDrawSurface", "DX10 Format"),
			(texFormat ? texFormat :
				rp_sprintf(C_("FileFormat", "Unknown (0x%08X)"), d->dxgi_format)));
	}

	// nVidia Texture Tools header
	if (ddsHeader->nvtt.dwNvttMagic == cpu_to_be32(NVTT_MAGIC)) {
		const uint32_t nvtt_version = le32_to_cpu(ddsHeader->nvtt.dwNvttVersion);
		fields->addField_string(C_("DirectDrawSurface", "NVTT Version"),
			rp_sprintf("%u.%u.%u",
				   (nvtt_version >> 16) & 0xFF,
				   (nvtt_version >>  8) & 0xFF,
				    nvtt_version        & 0xFF));
	}

	// GIMP-DDS header
	if (!memcmp(ddsHeader->gimp.magic, DDS_GIMP_MAGIC, sizeof(ddsHeader->gimp.magic))) {
		// Show the FourCC if it's set.
		// TODO: Show descriptive versions.
		if (ddsHeader->gimp.fourCC.u32 != 0) {
			char buf[8];
			memcpy(buf, ddsHeader->gimp.fourCC.c, 4);
			buf[4] = '\0';
			fields->addField_string(C_("DirectDrawSurface", "GIMP-DDS FourCC"), buf);
		}
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
	fields->addField_bitfield(C_("DirectDrawSurface", "Flags"),
		v_dwFlags_names, 3, ddsHeader->dwFlags);

	// ddspf.dwFlags (high bits; nVidia-specific)
	const uint32_t pf_flags_high = (ddsHeader->ddspf.dwFlags >> 30);
	static const char *const ddspf_dwFlags_names[] = {
		"sRGB",	// Not translatable
		NOP_C_("DirectDrawSurface|ddspf", "Normal Map"),
	};
	vector<string> *const v_ddspf_dwFlags_names = RomFields::strArrayToVector_i18n(
		"DirectDrawSurface|ddspf", ddspf_dwFlags_names, ARRAY_SIZE(ddspf_dwFlags_names));
	fields->addField_bitfield(C_("DirectDrawSurface", "PF Flags"),
		v_ddspf_dwFlags_names, 4, pf_flags_high);

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
	fields->addField_bitfield(C_("DirectDrawSurface", "Caps"),
		v_dwCaps_names, 3, ddsHeader->dwCaps);

	// dwCaps2 (rshifted by 8)
	static const char *const dwCaps2_names[] = {
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
	fields->addField_bitfield(C_("DirectDrawSurface", "Caps2"),
		v_dwCaps2_names, 4, (ddsHeader->dwCaps2 >> 8));

	if (ddsHeader->ddspf.dwFourCC == DDPF_FOURCC_XBOX) {
		// Xbox One texture.
		const DDS_HEADER_XBOX *const xb1Header = &d->xb1Header;

		fields->addField_string_numeric(C_("DirectDrawSurface", "Tile Mode"), xb1Header->tileMode);
		fields->addField_string_numeric(C_("DirectDrawSurface", "Base Alignment"), xb1Header->baseAlignment);
		// TODO: Not needed?
		fields->addField_string_numeric(C_("DirectDrawSurface", "Data Size"), xb1Header->dataSize);
		// TODO: Parse this.
		fields->addField_string_numeric(C_("DirectDrawSurface", "XDK Version"),
			xb1Header->xdkVer, RomFields::Base::Hex, 4, RomFields::STRF_MONOSPACE);
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
rp_image_const_ptr DirectDrawSurface::image(void) const
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
rp_image_const_ptr DirectDrawSurface::mipmap(int mip) const
{
	RP_D(const DirectDrawSurface);
	if (!d->isValid) {
		// Unknown file type.
		return nullptr;
	}

	// Load the image at the specified mipmap level.
	return const_cast<DirectDrawSurfacePrivate*>(d)->loadImage(mip);
}

}
