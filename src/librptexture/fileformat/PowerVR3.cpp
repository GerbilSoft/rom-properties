/***************************************************************************
 * ROM Properties Page shell extension. (librptexture)                     *
 * PowerVR3.cpp: PowerVR 3.0.0 texture image reader.                       *
 *                                                                         *
 * Copyright (c) 2019-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

/**
 * References:
 * - http://cdn.imgtec.com/sdk-documentation/PVR+File+Format.Specification.pdf
 */

#include "stdafx.h"
#include "config.librptexture.h"

#include "PowerVR3.hpp"
#include "FileFormat_p.hpp"

#include "pvr3_structs.h"

// Other rom-properties libraries
#include "libi18n/i18n.h"
using namespace LibRpFile;
using LibRpBase::RomFields;

// librptexture
#include "ImageSizeCalc.hpp"
#include "img/rp_image.hpp"
#include "decoder/ImageDecoder_Linear.hpp"
#include "decoder/ImageDecoder_PVRTC.hpp"
#include "decoder/ImageDecoder_ETC1.hpp"
#include "decoder/ImageDecoder_S3TC.hpp"
#include "decoder/ImageDecoder_BC7.hpp"
#include "decoder/ImageDecoder_ASTC.hpp"

// C++ STL classes
using std::array;
using std::string;
using std::unique_ptr;
using std::vector;

namespace LibRpTexture {

class PowerVR3Private final : public FileFormatPrivate
{
public:
	PowerVR3Private(PowerVR3 *q, const IRpFilePtr &file);

private:
	typedef FileFormatPrivate super;
	RP_DISABLE_COPY(PowerVR3Private)

public:
	/** TextureInfo **/
	static const array<const char*, 1+1> exts;
	static const array<const char*, 1+1> mimeTypes;
	static const TextureInfo textureInfo;

public:
	enum class PVRType {
		Unknown = -1,

		PVR3	= 0,	// PowerVR 3.0.0
		PVR2	= 1,	// PowerVR 2.0 (Legacy format)

		Max
	};
	PVRType pvrType;

public:
	// PVR3 header
	PowerVR3_Header pvr3Header;

	// Decoded mipmaps
	// Mipmap 0 is the full image.
	vector<rp_image_ptr> mipmaps;

	// Invalid pixel format message
	mutable string invalid_pixel_format;

	// Is byteswapping needed?
	// (PVR3 file has the opposite endianness.)
	bool isByteswapNeeded;

	// Is HFlip/VFlip needed?
	// Some textures may be stored upside-down due to
	// the way GL texture coordinates are interpreted.
	// Default without orientation metadata is HFlip=false, VFlip=false
	rp_image::FlipOp flipOp;

	// Metadata
	bool orientation_valid;
	PowerVR3_Metadata_Orientation orientation;

	// Texture data start address
	unsigned int texDataStartAddr;

	/**
	 * Uncompressed format lookup table
	 * NOTE: pixel_format appears byteswapped here because trailing '\0'
	 * isn't supported by MSVC, so e.g. 'rgba' is 'abgr', and
	 * 'i\0\0\0' is '\0\0\0i'. This *does* match the LE format, though.
	 * Channel depth uses the logical format, e.g. 0x00000008 or 0x00080808.
	 */
	struct FmtLkup_t {
		uint32_t pixel_format;
		uint32_t channel_depth;
		ImageDecoder::PixelFormat pxfmt;
		uint8_t bits;	// 8, 15, 16, 24, 32
	};

	static const array<FmtLkup_t, 11> fmtLkup_tbl_U8;
	static const array<FmtLkup_t,  1> fmtLkup_tbl_U16;
#if 0
	static const array<FmtLkup_t,  0> fmtLkup_tbl_U32;
#endif

	/**
	 * Legacy format lookup table
	 * Index == PowerVR_Legacy_Pixel_Format_e
	 * TODO: Compressed formats?
	 */
	static const array<FmtLkup_t, 32> fmtLkup_tbl_legacy;

private:
	/**
	 * Calculate the expected size for the image.
	 * NOTE: Only calculates mipmap 0 (full image).
	 * @param out_fmt	[out,opt] FmtLkup_t describing the format, if available.
	 * @return Expected size, or 0 on error.
	 */
	size_t calcExpectedSizeForMip0(const FmtLkup_t **out_fmt = nullptr) const;

public:
	/**
	 * Load the image.
	 * @param mip Mipmap number (0 == full image)
	 * @return Image, or nullptr on error.
	 */
	rp_image_const_ptr loadImage(int mip);

	/**
	 * Load PowerVR3 metadata.
	 * @return 0 on success; negative POSIX error code on error.
	 */
	int loadPvr3Metadata(void);

	/**
	 * Convert a legacy PowerVR pixel format to PowerVR3.
	 * This applies the specified format to this->pvr3Header.
	 * @param pixel_format Legacy PowerVR pixel format
	 */
	void legacyPowerVRPixelFormatToPVR3(PowerVR_Legacy_Pixel_Format_e pixel_format);
};

FILEFORMAT_IMPL(PowerVR3)

/** PowerVR3Private **/

/* TextureInfo */
const array<const char*, 1+1> PowerVR3Private::exts = {{
	".pvr",		// NOTE: Same as SegaPVR.

	nullptr
}};
const array<const char*, 1+1> PowerVR3Private::mimeTypes = {{
	// Unofficial MIME types.
	// TODO: Get these upstreamed on FreeDesktop.org.
	"image/x-pvr",

	nullptr
}};
const TextureInfo PowerVR3Private::textureInfo = {
	exts.data(), mimeTypes.data()
};

// Uncompressed format lookup table (UBYTE, UBYTE_NORM)
const array<PowerVR3Private::FmtLkup_t, 11> PowerVR3Private::fmtLkup_tbl_U8 = {{
	//{   'i', 0x00000008, ImageDecoder::PixelFormat::I8,		 8},
	//{   'r', 0x00000008, ImageDecoder::PixelFormat::R8,		 8},
	{   'a', 0x00000008, ImageDecoder::PixelFormat::A8,		 8},
	{  'al', 0x00000808, ImageDecoder::PixelFormat::A8L8,		16},
	{  'gr', 0x00000808, ImageDecoder::PixelFormat::GR88,		16},
	{ 'bgr', 0x00080808, ImageDecoder::PixelFormat::BGR888,		24},
	{'abgr', 0x08080808, ImageDecoder::PixelFormat::ABGR8888,	32},
	{'rgba', 0x08080808, ImageDecoder::PixelFormat::RGBA8888,	32},
	{ 'bgr', 0x00050605, ImageDecoder::PixelFormat::BGR565,		16},
	{'abgr', 0x04040404, ImageDecoder::PixelFormat::ABGR4444,	16},
	{'abgr', 0x01050505, ImageDecoder::PixelFormat::ABGR1555,	16},
	{ 'rgb', 0x00080808, ImageDecoder::PixelFormat::RGB888,		24},
	{'argb', 0x08080808, ImageDecoder::PixelFormat::ARGB8888,	32},
#if 0
	// TODO: Depth/stencil formats.
	{'\0\0\0d', 0x00000008},
	{'\0\0\0d', 0x00000010},
	{'\0\0\0d', 0x00000018},
	{'\0\0\0d', 0x00000020},
	{ '\0\0sd', 0x00000810},
	{ '\0\0sd', 0x00000818},
	{ '\0\0sd', 0x00000820},
	{'\0\0\0s', 0x00000008},
#endif
#if 0
	// TODO: "Weird" formats.
	{   'abgr', 0x10101010, ImageDecoder::PixelFormat::A16B16G16R16,	64},
	{  '\0bgr', 0x00101010, ImageDecoder::PixelFormat::B16G16R16,		48},
	{  '\0rgb', 0x000B0B0A, ImageDecoder::PixelFormat::R11G11B10,		32},	// NOTE: May be float.
#endif
}};

// Uncompressed format lookup table (USHORT, USHORT_NORM)
const array<PowerVR3Private::FmtLkup_t, 1> PowerVR3Private::fmtLkup_tbl_U16 = {{
	//{'\0\0\0r', 0x00000010, ImageDecoder::PixelFormat::R16,		16},
	{ '\0\0gr', 0x00001010, ImageDecoder::PixelFormat::G16R16,	32},
#if 0
	// TODO: High-bit-depth luminance.
	{ '\0\0al', 0x00001010, ImageDecoder::PixelFormat::A16L16,	32},
#endif
}};

#if 0
// Uncompressed format lookup table (UINT, UINT_NORM)
const array<PowerVR3Private::FmtLkup_t, 0> PowerVR3Private::fmtLkup_tbl_U32 = {{
	//{'\0\0\0r', 0x00000020, ImageDecoder::PixelFormat::R32,			32},
	//{ '\0\0gr', 0x00002020, ImageDecoder::PixelFormat::G32R32,		32},
	//{  '\0bgr', 0x00202020, ImageDecoder::PixelFormat::B32G32R32,		32},
	//{   'abgr', 0x20202020, ImageDecoder::PixelFormat::A32B32G32R32,	32},
#if 0
	// TODO: High-bit-depth luminance.
	{'\0\0\0l', 0x00000020, ImageDecoder::PixelFormat::L32,		32},
	{ '\0\0al', 0x00002020, ImageDecoder::PixelFormat::A32L32,	32},
#endif
}};
#endif

/**
 * Legacy format lookup table
 * Index == PowerVR_Legacy_Pixel_Format_e
 * TODO: Compressed formats?
 */
const array<PowerVR3Private::FmtLkup_t, 32> PowerVR3Private::fmtLkup_tbl_legacy = {{
	// TODO: Test these formats.

	// MGL Formats
	{   'argb', 0x04040404, ImageDecoder::PixelFormat::ARGB8888, 16},	// MGL_ARGB_4444 = 0x00
	{   'argb', 0x01050505, ImageDecoder::PixelFormat::ARGB1555, 16},	// MGL_ARGB_1555
	{  '\0rgb', 0x00050605, ImageDecoder::PixelFormat::RGB565,   16},	// MGL_RGB_565
	{  '\0rgb', 0x00050505, ImageDecoder::PixelFormat::RGB555,   15},	// MGL_RGB_555
	{  '\0rgb', 0x00080808, ImageDecoder::PixelFormat::RGB888,   24},	// MGL_RGB_888
	{   'argb', 0x08080808, ImageDecoder::PixelFormat::ARGB8888, 32},	// MGL_ARGB_8888
	{   'argb', 0x08030302, ImageDecoder::PixelFormat::ARGB8332, 16},	// MGL_ARGB_8332
	{'\0\0\0i', 0x00000008, ImageDecoder::PixelFormat::L8,        0},	// MGL_I_8 [TODO] [may need I8?]
	{ '\0\0ia', 0x00000808, ImageDecoder::PixelFormat::IA8,       0},	// MGL_AI_88 [TODO]
	{        0,          0, ImageDecoder::PixelFormat::Unknown,   0},	// MGL_1_BPP [TODO]
	{        0,          0, ImageDecoder::PixelFormat::Unknown,   0},	// MGL_VY1UY0 [TODO]
	{        0,          0, ImageDecoder::PixelFormat::Unknown,   0},	// MGL_Y1VY0U [TODO]
	{        0,          0, ImageDecoder::PixelFormat::Unknown,   0},	// MGL_PVRTC2 [TODO]
	{        0,          0, ImageDecoder::PixelFormat::Unknown,   0},	// MGL_PVRTC4 [TODO]
	{        0,          0, ImageDecoder::PixelFormat::Unknown,   0},
	{        0,          0, ImageDecoder::PixelFormat::Unknown,   0},

	// openGL Formats
	{   'rgba', 0x04040404, ImageDecoder::PixelFormat::RGBA4444, 16},	// GL_RGBA_4444 = 0x10
	{   'rgba', 0x05050501, ImageDecoder::PixelFormat::RGBA5551, 16},	// GL_RGBA_5551
	{  '\0rgb', 0x00050605, ImageDecoder::PixelFormat::RGB565,   16},	// GL_RGB_565
	{  '\0rgb', 0x00050505, ImageDecoder::PixelFormat::RGB555,   15},	// GL_RGB_555
	{  '\0rgb', 0x00080808, ImageDecoder::PixelFormat::RGB888,   24},	// GL_RGB_888
	{'\0\0\0l', 0x00000008, ImageDecoder::PixelFormat::L8,        0},	// GL_I_8
	{ '\0\0la', 0x00000808, ImageDecoder::PixelFormat::L8A8,      0},	// GL_AI_88
	{        0,          0, ImageDecoder::PixelFormat::Unknown,   0},	// GL_PVRTC2 [TODO]
	{        0,          0, ImageDecoder::PixelFormat::Unknown,   0},	// GL_PVRTC4 [TODO]
	{   'bgra', 0x08080808, ImageDecoder::PixelFormat::BGRA8888, 32},	// GL_BGRA_8888
	{        0,          0, ImageDecoder::PixelFormat::Unknown,   0},	// GL_A_8 [TODO]
	{        0,          0, ImageDecoder::PixelFormat::Unknown,   0},	// GL_PVRTCII4 [TODO]
	{        0,          0, ImageDecoder::PixelFormat::Unknown,   0},	// GL_PVRTCII2 [TODO]
	{        0,          0, ImageDecoder::PixelFormat::Unknown,   0},
	{        0,          0, ImageDecoder::PixelFormat::Unknown,   0},
	{        0,          0, ImageDecoder::PixelFormat::Unknown,   0},
}};

PowerVR3Private::PowerVR3Private(PowerVR3 *q, const IRpFilePtr &file)
	: super(q, file, &textureInfo)
	, pvrType(PVRType::Unknown)
	, isByteswapNeeded(false)
	, flipOp(rp_image::FLIP_NONE)
	, orientation_valid(false)
	, texDataStartAddr(0)
{
	// Clear the PowerVR3 header struct.
	memset(&pvr3Header, 0, sizeof(pvr3Header));
	memset(&orientation, 0, sizeof(orientation));
}

/**
 * Calculate the expected size for the image.
 * NOTE: Only calculates mipmap 0 (full image).
 * @param out_fmt	[out,opt] FmtLkup_t describing the format, if available.
 * @return Expected size, or 0 on error.
 */
size_t PowerVR3Private::calcExpectedSizeForMip0(const FmtLkup_t **out_fmt) const
{
	const int height = (pvr3Header.height > 0 ? pvr3Header.height : 1);

	if (pvr3Header.channel_depth != 0) {
		// Uncompressed format
		// Find a supported format that matches.

		// Only unsigned byte formats are supported right now.
		// TODO: How do we handle "normalized" versions?
		if (pvr3Header.channel_type != PVR3_CHTYPE_UBYTE &&
		    pvr3Header.channel_type != PVR3_CHTYPE_UBYTE_NORM)
		{
			// Not unsigned byte.
			if (out_fmt) {
				*out_fmt = nullptr;
			}
			return 0;
		}

		const FmtLkup_t *fmtLkup = nullptr;
		switch (pvrType) {
			default:
				// Invalid PVR type...
				if (out_fmt) {
					*out_fmt = nullptr;
				}
				return 0;

			case PVRType::PVR3:
				// TODO: Check fmtLkup_tbl_U16 too? Need some test files...
				for (const auto &p : fmtLkup_tbl_U8) {
					if (p.pixel_format == pvr3Header.pixel_format &&
					    p.channel_depth == pvr3Header.channel_depth)
					{
						fmtLkup = &p;
						break;
					}
				}
				break;

			case PVRType::PVR2:
				for (const auto &p : fmtLkup_tbl_legacy) {
					if (p.pixel_format == pvr3Header.pixel_format &&
					    p.channel_depth == pvr3Header.channel_depth)
					{
						fmtLkup = &p;
						break;
					}
				}
				break;
		}

		if (!fmtLkup) {
			// Not found.
			if (out_fmt) {
				*out_fmt = nullptr;
			}
			return 0;
		}

		// Convert to bytes, rounding up.
		const unsigned int bytespp = ((fmtLkup->bits + 7) & ~7) / 8;

		// TODO: Minimum row width?
		// TODO: Does 'rgb' use 24-bit or 32-bit?
		if (out_fmt) {
			*out_fmt = fmtLkup;
		}
		return ImageSizeCalc::T_calcImageSize(pvr3Header.width, height, bytespp);
	}

	// Compressed format
	size_t expected_size = 0;
	array<int8_t, 2> fmts = {{PVR3_CHTYPE_UBYTE_NORM, PVR3_CHTYPE_UBYTE}};
	switch (pvr3Header.pixel_format) {
#ifdef ENABLE_PVRTC
		case PVR3_PXF_PVRTC_2bpp_RGB:
		case PVR3_PXF_PVRTC_2bpp_RGBA:
			// 2bpp formats (PVRTC)
			// NOTE: Image dimensions must be a power of 2 for PVRTC-I.
			expected_size = ImageSizeCalc::T_calcImageSizePVRTC_PoT<true>(pvr3Header.width, height);
			break;

		case PVR3_PXF_PVRTCII_2bpp:
			// 2bpp formats (PVRTC-II)
			// NOTE: Width and height must be rounded to the nearest tile. (8x4)
			// FIXME: Our PVRTC-II decoder requires power-of-2 textures right now.
			//expected_size = ALIGN_BYTES(8, width) *
			//                ALIGN_BYTES(4, height) / 4;
			expected_size = ImageSizeCalc::T_calcImageSizePVRTC_PoT<true>(pvr3Header.width, height);
			break;

		case PVR3_PXF_PVRTC_4bpp_RGB:
		case PVR3_PXF_PVRTC_4bpp_RGBA:
			// 4bpp formats (PVRTC)
			// NOTE: Image dimensions must be a power of 2 for PVRTC-I.
			expected_size = ImageSizeCalc::T_calcImageSizePVRTC_PoT<false>(pvr3Header.width, height);
			break;

		case PVR3_PXF_PVRTCII_4bpp:
			// 4bpp formats (PVRTC-II)
			// NOTE: Width and height must be rounded to the nearest tile. (4x4)
			// FIXME: Our PVRTC-II decoder requires power-of-2 textures right now.
			//expected_size = ALIGN_BYTES(4, width) *
			//                ALIGN_BYTES(4, height) / 2;
			expected_size = ImageSizeCalc::T_calcImageSizePVRTC_PoT<false>(pvr3Header.width, height);
			break;
#endif /* ENABLE_PVRTC */

		case PVR3_PXF_ETC1:
		case PVR3_PXF_DXT1:
		case PVR3_PXF_BC4:
		case PVR3_PXF_ETC2_RGB:
		case PVR3_PXF_ETC2_RGB_A1:
		case PVR3_PXF_EAC_R11:
			// 4bpp formats
			expected_size = ImageSizeCalc::T_calcImageSize(pvr3Header.width, height) / 2;
			break;

		case PVR3_PXF_DXT2:
		case PVR3_PXF_DXT3:
		case PVR3_PXF_DXT4:
		case PVR3_PXF_DXT5:
		case PVR3_PXF_BC5:
		case PVR3_PXF_BC6:
		case PVR3_PXF_BC7:
		case PVR3_PXF_ETC2_RGBA:
		case PVR3_PXF_EAC_RG11:
			// 8bpp formats
			expected_size = ImageSizeCalc::T_calcImageSize(pvr3Header.width, height);
			break;

		case PVR3_PXF_R9G9B9E5:
			// Uncompressed "special" 32bpp formats.
			// NOTE: This is a floating-point format.
			fmts[0] = PVR3_CHTYPE_FLOAT;
			fmts[1] = -1;
			expected_size = ImageSizeCalc::T_calcImageSize(pvr3Header.width, height, sizeof(uint32_t));
			break;

		default:
#ifdef ENABLE_ASTC
			if (pvr3Header.pixel_format >= PVR3_PXF_ASTC_4x4 &&
			    pvr3Header.pixel_format <= PVR3_PXF_ASTC_12x12)
			{
				// TODO: PVR3 ASTC 3D formats.
				static_assert(PVR3_PXF_ASTC_12x12 - PVR3_PXF_ASTC_4x4 + 1 == ARRAY_SIZE(ImageDecoder::astc_lkup_tbl),
					"ASTC lookup table size is wrong!");
				const unsigned int astc_idx = pvr3Header.pixel_format - PVR3_PXF_ASTC_4x4;
				expected_size = ImageSizeCalc::calcImageSizeASTC(pvr3Header.width, height,
					ImageDecoder::astc_lkup_tbl[astc_idx][0],
					ImageDecoder::astc_lkup_tbl[astc_idx][1]);
				break;
			}
#endif /* ENABLE_ASTC */

			// TODO: Other formats that aren't actually compressed.
			//assert(!"Unsupported PowerVR3 compressed format.");
			break;
	}

	// No FmtLkup_t for compressed formats.
	if (out_fmt) {
		*out_fmt = nullptr;
	}

	// Make sure the channel type is correct.
	bool isOK = false;
	for (const int8_t fmt : fmts) {
		if (fmt < 0) {
			break;
		}

		if (pvr3Header.channel_type == static_cast<uint8_t>(fmt)) {
			// Found a match.
			isOK = true;
			break;
		}
	}

	if (!isOK) {
		// Channel type is incorrect.
		expected_size = 0;
	}

	return expected_size;
}

/**
 * Load the image.
 * @param mip Mipmap number (0 == full image)
 * @return Image, or nullptr on error.
 */
rp_image_const_ptr PowerVR3Private::loadImage(int mip)
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

	// NOTE: Only the first surface/face is supported at the moment,
	// but we need to ensure we skip all of them when selecting a
	// mipmap level other than 0.
	unsigned int num_surfaces = pvr3Header.num_surfaces;
	assert(num_surfaces <= 128);
	if (num_surfaces == 0) {
		num_surfaces = 1;
	} else if (num_surfaces > 128) {
		// Too many surfaces.
		return nullptr;
	}
	unsigned int num_faces = pvr3Header.num_faces;
	assert(num_faces <= 128);
	if (num_faces == 0) {
		num_faces = 1;
	} else if (num_faces > 128) {
		// Too many faces.
		return nullptr;
	}
	// TODO: Skip the multiply if both surfaces and faces are 1?
	const unsigned int prod_surfaces_faces = num_surfaces * num_faces;

	// Sanity check: Maximum image dimensions of 32768x32768.
	// NOTE: `height == 0` is allowed here. (1D texture)
	assert(pvr3Header.width > 0);
	assert(pvr3Header.width <= 32768);
	assert(pvr3Header.height <= 32768);
	if (pvr3Header.width == 0 || pvr3Header.width > 32768 ||
	    pvr3Header.height > 32768)
	{
		// Invalid image dimensions.
		return nullptr;
	}

	// Texture cannot start inside of the PowerVR3 header.
	assert(texDataStartAddr >= sizeof(pvr3Header));
	if (texDataStartAddr < sizeof(pvr3Header)) {
		// Invalid texture data start address.
		return nullptr;
	}

	const off64_t fileSize = file->size();
	if (fileSize > 128*1024*1024) {
		// Sanity check: PowerVR3 files shouldn't be more than 128 MB.
		return nullptr;
	}

	// Seek to the start of the texture data.
	int ret = file->seek(texDataStartAddr);
	if (ret != 0) {
		// Seek error.
		return nullptr;
	}

	// Handle a 1D texture as a "width x 1" 2D texture.
	// NOTE: Handling a 3D texture as a single 2D texture.
	int width = pvr3Header.width;
	int height = (pvr3Header.height > 0 ? pvr3Header.height : 1);

	// Calculate the expected size.
	const FmtLkup_t *fmtLkup = nullptr;
	size_t expected_size = calcExpectedSizeForMip0(&fmtLkup);
	if (unlikely(expected_size == 0)) {
		// Unable to calculate the expected size.
		return nullptr;
	}

	// If we're requesting a mipmap level higher than 0 (full image),
	// adjust the start address, expected size, and dimensions.
	unsigned int start_addr = texDataStartAddr;
	for (int adjmip = mip; adjmip > 0; adjmip--) {
		width /= 2;
		height /= 2;

		assert(width > 0);
		assert(height > 0);
		if (width <= 0 || height <= 0) {
			// Mipmap size calculation error...
			return nullptr;
		}

		// TODO: Skip the multiply if both surfaces and faces are 1?
		start_addr += static_cast<unsigned int>(expected_size * prod_surfaces_faces);
		expected_size /= 4;
	}

	// Verify file size.
	if ((start_addr + expected_size) > static_cast<size_t>(fileSize)) {
		// File is too small.
		return nullptr;
	}

	// Read the texture data.
	auto buf = aligned_uptr<uint8_t>(16, expected_size);
	size_t size = file->seekAndRead(start_addr, buf.get(), expected_size);
	if (size != expected_size) {
		// Seek and/or read error.
		return nullptr;
	}

	// Decode the image.
	rp_image_ptr img;
	if (pvr3Header.channel_depth != 0) {
		// Uncompressed format
		assert(fmtLkup != nullptr);
		if (!fmtLkup) {
			// Shouldn't happen...
			return nullptr;
		}

		// TODO: Is the row stride required to be a specific multiple?
		switch (fmtLkup->bits) {
			case 8:
				// 8-bit
				img = ImageDecoder::fromLinear8(
					static_cast<ImageDecoder::PixelFormat>(fmtLkup->pxfmt),
					width, height, buf.get(), expected_size);
				break;

			case 15:
			case 16:
				// 15/16-bit
				img = ImageDecoder::fromLinear16(
					static_cast<ImageDecoder::PixelFormat>(fmtLkup->pxfmt),
					width, height,
					reinterpret_cast<const uint16_t*>(buf.get()), expected_size);
				break;

			case 24:
				// 24-bit
				img = ImageDecoder::fromLinear24(
					static_cast<ImageDecoder::PixelFormat>(fmtLkup->pxfmt),
					width, height, buf.get(), expected_size);
				break;

			case 32:
				// 32-bit
				img = ImageDecoder::fromLinear32(
					static_cast<ImageDecoder::PixelFormat>(fmtLkup->pxfmt),
					width, height,
					reinterpret_cast<const uint32_t*>(buf.get()), expected_size);
				break;

			default:
				// Not supported...
				//assert(!"Unsupported PowerVR3 uncompressed format.");
				return nullptr;
		}
	} else {
		// Compressed format
		switch (pvr3Header.pixel_format) {
#ifdef ENABLE_PVRTC
			case PVR3_PXF_PVRTC_2bpp_RGB:
				// PVRTC, 2bpp, no alpha.
				img = ImageDecoder::fromPVRTC(width, height, buf.get(), expected_size,
					ImageDecoder::PVRTC_2BPP | ImageDecoder::PVRTC_ALPHA_NONE);
				break;

			case PVR3_PXF_PVRTC_2bpp_RGBA:
				// PVRTC, 2bpp, has alpha.
				img = ImageDecoder::fromPVRTC(width, height, buf.get(), expected_size,
					ImageDecoder::PVRTC_2BPP | ImageDecoder::PVRTC_ALPHA_YES);
				break;

			case PVR3_PXF_PVRTC_4bpp_RGB:
				// PVRTC, 4bpp, no alpha.
				img = ImageDecoder::fromPVRTC(width, height, buf.get(), expected_size,
					ImageDecoder::PVRTC_4BPP | ImageDecoder::PVRTC_ALPHA_NONE);
				break;

			case PVR3_PXF_PVRTC_4bpp_RGBA:
				// PVRTC, 4bpp, has alpha.
				img = ImageDecoder::fromPVRTC(width, height, buf.get(), expected_size,
					ImageDecoder::PVRTC_4BPP | ImageDecoder::PVRTC_ALPHA_YES);
				break;

			case PVR3_PXF_PVRTCII_2bpp:
				// PVRTC-II, 2bpp.
				// NOTE: Assuming this has alpha.
				img = ImageDecoder::fromPVRTCII(width, height, buf.get(), expected_size,
					ImageDecoder::PVRTC_2BPP | ImageDecoder::PVRTC_ALPHA_YES);
				break;

			case PVR3_PXF_PVRTCII_4bpp:
				// PVRTC-II, 4bpp.
				// NOTE: Assuming this has alpha.
				img = ImageDecoder::fromPVRTCII(width, height, buf.get(), expected_size,
					ImageDecoder::PVRTC_4BPP | ImageDecoder::PVRTC_ALPHA_YES);
				break;
#endif /* ENABLE_PVRTC */

			case PVR3_PXF_ETC1:
				// ETC1-compressed texture.
				img = ImageDecoder::fromETC1(width, height, buf.get(), expected_size);
				break;

			case PVR3_PXF_ETC2_RGB:
				// ETC2-compressed RGB texture.
				img = ImageDecoder::fromETC2_RGB(width, height, buf.get(), expected_size);
				break;

			case PVR3_PXF_ETC2_RGB_A1:
				// ETC2-compressed RGB texture
				// with punchthrough alpha.
				img = ImageDecoder::fromETC2_RGB_A1(width, height, buf.get(), expected_size);
				break;

			case PVR3_PXF_ETC2_RGBA:
				// ETC2-compressed RGB texture
				// with EAC-compressed alpha channel.
				img = ImageDecoder::fromETC2_RGBA(width, height, buf.get(), expected_size);
				break;

			case PVR3_PXF_EAC_R11:
				// EAC-compressed R11 texture.
				img = ImageDecoder::fromEAC_R11(width, height, buf.get(), expected_size);
				break;

			case PVR3_PXF_EAC_RG11:
				// EAC-compressed RG11 texture.
				img = ImageDecoder::fromEAC_RG11(width, height, buf.get(), expected_size);
				break;

			case PVR3_PXF_DXT1:
				// DXT1-compressed texture.
				img = ImageDecoder::fromDXT1(width, height, buf.get(), expected_size);
				break;

			case PVR3_PXF_DXT2:
				// DXT2-compressed texture.
				img = ImageDecoder::fromDXT2(width, height, buf.get(), expected_size);
				break;

			case PVR3_PXF_DXT3:
				// DXT3-compressed texture.
				img = ImageDecoder::fromDXT3(width, height, buf.get(), expected_size);
				break;

			case PVR3_PXF_DXT4:
				// DXT4-compressed texture.
				img = ImageDecoder::fromDXT4(width, height, buf.get(), expected_size);
				break;

			case PVR3_PXF_DXT5:
				// DXT2-compressed texture.
				img = ImageDecoder::fromDXT5(width, height, buf.get(), expected_size);
				break;

			case PVR3_PXF_BC4:
				// RGTC, one component. (BC4)
				img = ImageDecoder::fromBC4(width, height, buf.get(), expected_size);
				break;

			case PVR3_PXF_BC5:
				// RGTC, two components. (BC5)
				img = ImageDecoder::fromBC5(width, height, buf.get(), expected_size);
				break;

			case PVR3_PXF_BC7:
				// BC7-compressed texture.
				img = ImageDecoder::fromBC7(width, height, buf.get(), expected_size);
				break;

			case PVR3_PXF_R9G9B9E5:
				// RGB9_E5 (technically uncompressed...)
				img = ImageDecoder::fromLinear32(
					ImageDecoder::PixelFormat::RGB9_E5, width, height,
					reinterpret_cast<const uint32_t*>(buf.get()), expected_size);
				break;

			default:
#ifdef ENABLE_ASTC
				if (pvr3Header.pixel_format >= PVR3_PXF_ASTC_4x4 &&
				    pvr3Header.pixel_format <= PVR3_PXF_ASTC_12x12)
				{
					// TODO: PVR3 ASTC 3D formats.
					static_assert(PVR3_PXF_ASTC_12x12 - PVR3_PXF_ASTC_4x4 + 1 == ARRAY_SIZE(ImageDecoder::astc_lkup_tbl),
						"ASTC lookup table size is wrong!");
					const unsigned int astc_idx = pvr3Header.pixel_format - PVR3_PXF_ASTC_4x4;
					img = ImageDecoder::fromASTC(width, height, buf.get(), expected_size,
						ImageDecoder::astc_lkup_tbl[astc_idx][0],
						ImageDecoder::astc_lkup_tbl[astc_idx][1]);
					break;
				}
#endif /* ENABLE_ASTC */

				// TODO: Other formats that aren't actually compressed.
				//assert(!"Unsupported PowerVR3 compressed format.");
				return nullptr;
		}
	}

	// TODO: Handle sRGB.
	// TODO: Handle premultiplied alpha, aside from DXT2 and DXT4.

	// Post-processing: Check if a flip is needed.
	if (img && flipOp != rp_image::FLIP_NONE) {
		rp_image_ptr flipimg = img->flip(flipOp);
		if (flipimg) {
			img = std::move(flipimg);
		}
	}

	mipmaps[mip] = img;
	return img;
}

/**
 * Load PowerVR3 metadata.
 * @return 0 on success; negative POSIX error code on error.
 */
int PowerVR3Private::loadPvr3Metadata(void)
{
	if (pvr3Header.metadata_size == 0) {
		// No metadata.
		return 0;
	} else if (pvr3Header.metadata_size <= sizeof(PowerVR3_Metadata_Block_Header_t)) {
		// Metadata is present, but it's too small...
		return -EIO;
	}

	// Sanity check: Metadata shouldn't be more than 128 KB.
	assert(pvr3Header.metadata_size <= 128*1024);
	if (pvr3Header.metadata_size > 128*1024) {
		return -ENOMEM;
	}

	// Parse the additional metadata.
	int ret = 0;
	unique_ptr<uint8_t[]> buf(new uint8_t[pvr3Header.metadata_size]);
	size_t size = file->seekAndRead(sizeof(pvr3Header), buf.get(), pvr3Header.metadata_size);
	if (size != pvr3Header.metadata_size) {
		return -EIO;
	}

	uint8_t *p = buf.get();
	uint8_t *const p_end = p + pvr3Header.metadata_size;
	while (p < p_end - sizeof(PowerVR3_Metadata_Block_Header_t)) {
		PowerVR3_Metadata_Block_Header_t *const pHdr =
			reinterpret_cast<PowerVR3_Metadata_Block_Header_t*>(p);
		p += sizeof(*pHdr);

		// Byteswap the header, if necessary.
		if (isByteswapNeeded) {
			pHdr->fourCC = __swab32(pHdr->fourCC);
			pHdr->key    = __swab32(pHdr->key);
			pHdr->size   = __swab32(pHdr->size);
		}

		// Check the fourCC.
		if (pHdr->fourCC != PVR3_VERSION_HOST) {
			// Not supported.
			p += pHdr->size;
			continue;
		}

		// Check the key.
		switch (pHdr->key) {
			case PVR3_META_ORIENTATION: {
				// Logical orientation.
				if (p > p_end - sizeof(orientation)) {
					// Out of bounds...
					p = p_end;
					break;
				}

				// Copy the orientation bytes.
				memcpy(&orientation, p, sizeof(orientation));
				orientation_valid = true;
				p += sizeof(orientation);

				// Set the flip operation.
				// TODO: Z flip?
				flipOp = rp_image::FLIP_NONE;
				if (orientation.x != 0) {
					flipOp = rp_image::FLIP_H;
				}
				if (orientation.y != 0) {
					flipOp = static_cast<rp_image::FlipOp>(flipOp | rp_image::FLIP_V);
				}
				break;
			}

			default:
			case PVR3_META_TEXTURE_ATLAS:
			case PVR3_META_NORMAL_MAP:
			case PVR3_META_CUBE_MAP:
			case PVR3_META_BORDER:
			case PVR3_META_PADDING:
				// TODO: Not supported.
				p += pHdr->size;
				break;
		}
	}

	// Metadata parsed.
	return ret;
}

/**
 * Convert a legacy PowerVR pixel format to PowerVR3.
 * This applies the specified format to this->pvr3Header.
 * @param pixel_format Legacy PowerVR pixel format
 */
void PowerVR3Private::legacyPowerVRPixelFormatToPVR3(PowerVR_Legacy_Pixel_Format_e pixel_format)
{
	if (static_cast<size_t>(pixel_format) > fmtLkup_tbl_legacy.size()) {
		// Out of range.
		pvr3Header.pixel_format = 0;
		pvr3Header.channel_depth = 0;
		return;
	}

	pvr3Header.pixel_format = fmtLkup_tbl_legacy[pixel_format].pixel_format;
	pvr3Header.channel_depth = fmtLkup_tbl_legacy[pixel_format].channel_depth;
}

/** PowerVR3 **/

/**
 * Read a PowerVR 3.0.0 texture image file.
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
PowerVR3::PowerVR3(const IRpFilePtr &file)
	: super(new PowerVR3Private(this, file))
{
	RP_D(PowerVR3);
	d->mimeType = "image/x-pvr";	// unofficial, not on fd.o

	if (!d->file) {
		// Could not ref() the file handle.
		return;
	}

	// Read the PowerVR3 header.
	union {
		PowerVR3_Header pvr3;
		PowerVR_Legacy_Header pvrLegacy;
		uint8_t u8[sizeof(d->pvr3Header)];
	} header;

	d->file->rewind();
	size_t size = d->file->read(header.u8, sizeof(header.u8));
	if (size != sizeof(d->pvr3Header)) {
		d->file.reset();
		return;
	}

	// Check if this PVR image is supported.
	const DetectInfo info = {
		{0, sizeof(header.u8), header.u8},
		nullptr,	// ext (not needed for PowerVR3)
		file->size()	// szFile
	};
	d->pvrType = static_cast<PowerVR3Private::PVRType>(isRomSupported_static(&info));
	d->isValid = (static_cast<int>(d->pvrType) >= 0);

	if (!d->isValid) {
		d->file.reset();
		return;
	}

	switch (d->pvrType) {
		default:
			d->file.reset();
			d->isValid = false;
			return;

		case PowerVR3Private::PVRType::PVR3:
			// Verify the PVR3 magic/version.
			if (header.pvr3.version == PVR3_VERSION_HOST) {
				// Host-endian. Byteswapping is not needed.
				d->isByteswapNeeded = false;
				memcpy(&d->pvr3Header, &header.pvr3, sizeof(d->pvr3Header));

#if SYS_BYTEORDER == SYS_BIG_ENDIAN
				// Pixel format and channel depth need to be swapped if this is
				// a big-endian file, since it's technically a 64-bit field.
				std::swap(d->pvr3Header.pixel_format, d->pvr3Header.channel_depth);
#endif /* SYS_BYTEORDER == SYS_BIG_ENDIAN */
			} else if (header.pvr3.version == PVR3_VERSION_SWAP) {
				// Swap-endian. Byteswapping is needed.
				// NOTE: Keeping `version` unswapped in case
				// the actual image data needs to be byteswapped.
				d->pvr3Header.version		= header.pvr3.version;
				d->pvr3Header.flags		= __swab32(header.pvr3.flags);

#if SYS_BYTEORDER == SYS_LIL_ENDIAN
				// Pixel format and channel depth need to be swapped if this is
				// a big-endian file, since it's technically a 64-bit field.
				d->pvr3Header.channel_depth	= __swab32(header.pvr3.pixel_format);
				d->pvr3Header.pixel_format	= __swab32(header.pvr3.channel_depth);
#else /* SYS_BYTEORDER == SYS_BIG_ENDIAN */
				// Little-endian file. Simply byteswap the two fields.
				d->pvr3Header.pixel_format	= __swab32(header.pvr3.pixel_format);
				d->pvr3Header.channel_depth	= __swab32(header.pvr3.channel_depth);
#endif

				d->pvr3Header.color_space	= __swab32(header.pvr3.color_space);
				d->pvr3Header.channel_type	= __swab32(header.pvr3.channel_type);
				d->pvr3Header.height		= __swab32(header.pvr3.height);
				d->pvr3Header.width		= __swab32(header.pvr3.width);
				d->pvr3Header.depth		= __swab32(header.pvr3.depth);
				d->pvr3Header.num_surfaces	= __swab32(header.pvr3.num_surfaces);
				d->pvr3Header.num_faces		= __swab32(header.pvr3.num_faces);
				d->pvr3Header.mipmap_count	= __swab32(header.pvr3.mipmap_count);
				d->pvr3Header.metadata_size	= __swab32(header.pvr3.metadata_size);

				// Convenience flag
				d->isByteswapNeeded = true;
			} else {
				// Invalid magic.
				d->file.reset();
				d->isValid = false;
				return;
			}

			d->textureFormatName = "PowerVR 3.0.0";
			break;

		case PowerVR3Private::PVRType::PVR2: {
			// Convert the header from PVR2 to PVR3.
			uint32_t pixel_format_and_flags;
			if (header.pvrLegacy.magic == PVR2_MAGIC_HOST) {
				// Host-endian. Byteswapping is not needed.
				d->pvr3Header.version		= header.pvrLegacy.magic;
				d->pvr3Header.flags		= 0;	// TODO

				d->pvr3Header.height		= header.pvrLegacy.height;
				d->pvr3Header.width		= header.pvrLegacy.width;

				pixel_format_and_flags = header.pvrLegacy.pixel_format_and_flags;
				const bool isCubeMap = !!(pixel_format_and_flags & PVR_LEGACY_FLAG_CUBEMAP);

				if (pixel_format_and_flags & PVR_LEGACY_FLAG_VOLUME_TEXTURE) {
					d->pvr3Header.depth = header.pvrLegacy.num_surfaces;
					if (isCubeMap) {
						d->pvr3Header.depth /= 6;
					}
					d->pvr3Header.num_surfaces = 1;
				} else {
					d->pvr3Header.depth = 1;
					d->pvr3Header.num_surfaces = header.pvrLegacy.num_surfaces;
					if (isCubeMap) {
						d->pvr3Header.num_surfaces /= 6;
					}
				}

				// Mipmap count in legacy headers is 0 for toplevel only.
				// In PVR3, it's 1 for toplevel only.
				d->pvr3Header.mipmap_count = header.pvrLegacy.mipmap_count + 1;
			} else if (header.pvrLegacy.magic == PVR2_MAGIC_SWAP) {
				// Swap-endian. Byteswapping is needed.
				// NOTE: Keeping `version` unswapped in case.
				// Host-endian. Byteswapping is not needed.
				d->pvr3Header.version		= header.pvrLegacy.magic;
				d->pvr3Header.flags		= 0;	// TODO

				d->pvr3Header.height		= __swab32(header.pvrLegacy.height);
				d->pvr3Header.width		= __swab32(header.pvrLegacy.width);

				pixel_format_and_flags = __swab32(header.pvrLegacy.pixel_format_and_flags);
				const bool isCubeMap = !!(pixel_format_and_flags & PVR_LEGACY_FLAG_CUBEMAP);

				if (pixel_format_and_flags & PVR_LEGACY_FLAG_VOLUME_TEXTURE) {
					d->pvr3Header.depth = __swab32(header.pvrLegacy.num_surfaces);
					if (isCubeMap) {
						d->pvr3Header.depth /= 6;
					}
					d->pvr3Header.num_surfaces = 1;
				} else {
					d->pvr3Header.depth = 1;
					d->pvr3Header.num_surfaces = __swab32(header.pvrLegacy.num_surfaces);
					if (isCubeMap) {
						d->pvr3Header.num_surfaces /= 6;
					}
				}

				// Mipmap count in legacy headers is 0 for toplevel only.
				// In PVR3, it's 1 for toplevel only.
				d->pvr3Header.mipmap_count = __swab32(header.pvrLegacy.mipmap_count) + 1;
			} else {
				// Invalid magic.
				d->file.reset();
				d->isValid = false;
				return;
			}

			d->pvr3Header.color_space	= PVR3_COLOR_SPACE_RGB;
			d->pvr3Header.channel_type	= PVR3_CHTYPE_UBYTE_NORM;

			// There's a bug where num_surfaces is sometimes 0.
			if (d->pvr3Header.num_surfaces == 0) {
				d->pvr3Header.num_surfaces = 1;
			}

			// No metadata. (TODO: Vertical flip?)
			d->pvr3Header.metadata_size = 0;

			// Determine the pixel format.
			d->legacyPowerVRPixelFormatToPVR3(static_cast<PowerVR_Legacy_Pixel_Format_e>(pixel_format_and_flags & PVR_LEGACY_PIXEL_FORMAT_MASK));
			if (pixel_format_and_flags & PVR_LEGACY_FLAG_VERTICAL_FLIP) {
				d->flipOp = rp_image::FlipOp::FLIP_V;
			}

			d->textureFormatName = "PowerVR 2.0";
			break;
		}
	}

	// File is valid.
	d->isValid = true;

	// Initialize the mipmap vector.
	d->mipmapCount = static_cast<int>(d->pvr3Header.mipmap_count);
	assert(d->mipmapCount >= 0);
	assert(d->mipmapCount <= 128);
	if (d->mipmapCount > 128) {
		// Too many mipmaps.
		// NOTE: PowerVR3 stores mipmaps in descending order,
		// so clamp it to 128 mipmaps.
		d->mipmapCount = 128;
	}
	d->mipmaps.resize(d->mipmapCount > 0 ? d->mipmapCount : 1);

	// Texture data start address.
	d->texDataStartAddr = sizeof(d->pvr3Header) + d->pvr3Header.metadata_size;

	// Load PowerVR metadata.
	// This function checks for the orientation block
	// and sets the HFlip/VFlip values as necessary.
	d->loadPvr3Metadata();

	// Cache the dimensions for the FileFormat base class.
	d->dimensions[0] = d->pvr3Header.width;
	if (d->pvr3Header.height > 1) {
		d->dimensions[1] = d->pvr3Header.height;
		if (d->pvr3Header.depth > 1) {
			d->dimensions[2] = d->pvr3Header.depth;
		}
	}
}

/**
 * Is a ROM image supported by this class?
 * TODO: Add isTextureSupported() to FileFormat
 * @param info DetectInfo containing ROM detection information.
 * @return Class-specific system ID (>= 0) if supported; -1 if not.
 */
int PowerVR3::isRomSupported_static(const DetectInfo *info)
{
	assert(info != nullptr);
	assert(info->header.pData != nullptr);
	assert(info->header.addr == 0);
	if (!info || !info->header.pData ||
	    info->header.addr != 0 ||
	    info->header.size < std::max(sizeof(PowerVR3_Header), sizeof(PowerVR_Legacy_Header)))
	{
		// Either no detection information was specified,
		// or the header is too small.
		return -1;
	}

	// Check for a PVR3 texture.
	const PowerVR3_Header *const pvr3Header =
		reinterpret_cast<const PowerVR3_Header*>(info->header.pData);
	if (pvr3Header->version == PVR3_VERSION_HOST ||
	    pvr3Header->version == PVR3_VERSION_SWAP)
	{
		// This is a PVR3 texture.
		return static_cast<int>(PowerVR3Private::PVRType::PVR3);
	}

	// Check for a PVR2 texture.
	// TODO: Check for V1? (No magic number, though...)
	const PowerVR_Legacy_Header *const pvrLegacyHeader =
		reinterpret_cast<const PowerVR_Legacy_Header*>(info->header.pData);
	if (pvrLegacyHeader->magic == PVR2_MAGIC_HOST ||
	    pvrLegacyHeader->magic == PVR2_MAGIC_SWAP)
	{
		// This is a PVR2 texture.
		return static_cast<int>(PowerVR3Private::PVRType::PVR2);
	}

	// Not supported.
	return -1;
}

/** Property accessors **/

/**
 * Get the pixel format, e.g. "RGB888" or "DXT1".
 * @return Pixel format, or nullptr if unavailable.
 */
const char *PowerVR3::pixelFormat(void) const
{
	RP_D(const PowerVR3);
	if (!d->isValid) {
		return nullptr;
	}

	if (!d->invalid_pixel_format.empty()) {
		return d->invalid_pixel_format.c_str();
	}

	if (d->pvr3Header.channel_depth == 0) {
		// Compressed texture format.
		static const array<const char*, PVR3_PXF_MAX> pvr3PxFmt_tbl = {{
			// 0
			"PVRTC 2bpp RGB", "PVRTC 2bpp RGBA",
			"PVRTC 4bpp RGB", "PVRTC 4bpp RGBA",
			"PVRTC-II 2bpp", "PVRTC-II 4bpp",
			"ETC1", "DXT1", "DXT2", "DXT3", "DXT4", "DXT5",
			"BC4", "BC5", "BC6", "BC7",

			// 16
			"UYVY", "YUY2", "BW1bpp", "R9G9B9E5 Shared Exponent",
			"RGBG8888", "GRGB8888", "ETC2 RGB", "ETC2 RGBA",
			"ETC2 RGB A1", "EAC R11", "EAC RG11",

			// 27
			"ASTC_4x4", "ASTC_5x4", "ASTC_5x5", "ASTC_6x5", "ATC_6x6",

			// 32
			"ASTC_8x5", "ASTC_8x6", "ASTC_8x8", "ASTC_10x5",
			"ASTC_10x6", "ASTC_10x8", "ASTC_10x10", "ASTC_12x10",
			"ASTC_12x12",

			// 41
			"ASTC_3x3x3", "ASTC_4x3x3", "ASTC_4x4x3", "ASTC_4x4x4",
			"ASTC_5x4x4", "ASTC_5x5x4", "ASTC_5x5x5", "ASTC_6x5x5",
			"ASTC_6x6x5", "ASTC_6x6x6",
		}};
		static_assert(pvr3PxFmt_tbl.size() == PVR3_PXF_MAX, "pvr3PxFmt_tbl[] needs to be updated!");

		if (d->pvr3Header.pixel_format < pvr3PxFmt_tbl.size()) {
			return pvr3PxFmt_tbl[d->pvr3Header.pixel_format];
		}

		// Not valid.
		d->invalid_pixel_format = fmt::format(
			C_("PowerVR3", "Unknown (Compressed: 0x{:0>8X})"),
			d->pvr3Header.pixel_format);
		return d->invalid_pixel_format.c_str();
	}

	// Uncompressed pixel formats.
	// These are literal channel identifiers, e.g. 'rgba',
	// followed by a color depth value for each channel.

	// NOTE: Pixel formats are stored in literal order in
	// little-endian files, so the low byte is the first channel.
	// TODO: Verify big-endian.

	string s_pxf;	// pixel format
	string s_chcnt;	// channel count
	s_pxf.reserve(8);
	s_chcnt.reserve(8);

	uint32_t pixel_format = d->pvr3Header.pixel_format;
	uint32_t channel_depth = d->pvr3Header.channel_depth;
	for (unsigned int i = 0; i < 4; i++, pixel_format >>= 8, channel_depth >>= 8) {
		const uint8_t pxf = (pixel_format & 0xFF);
		if (pxf == 0) {
			break;
		}

		s_pxf += TOUPPER(pxf);
		s_chcnt += fmt::to_string(static_cast<unsigned int>(channel_depth & 0xFF));
	}

	if (likely(!s_pxf.empty())) {
		// Not exactly an "invalid" pixel format...
		d->invalid_pixel_format.reserve(s_pxf.size() + s_chcnt.size());
		d->invalid_pixel_format = s_pxf;
		d->invalid_pixel_format += s_chcnt;
	} else {
		d->invalid_pixel_format = C_("RomData", "Unknown");
	}
	return d->invalid_pixel_format.c_str();
}

#ifdef ENABLE_LIBRPBASE_ROMFIELDS
/**
 * Get property fields for rom-properties.
 * @param fields RomFields object to which fields should be added.
 * @return Number of fields added, or 0 on error.
 */
int PowerVR3::getFields(RomFields *fields) const
{
	assert(fields != nullptr);
	if (!fields) {
		return 0;
	}

	RP_D(const PowerVR3);
	if (!d->isValid) {
		// Unknown file type.
		return -EIO;
	}

	const PowerVR3_Header *const pvr3Header = &d->pvr3Header;
	const int initial_count = fields->count();
	fields->reserve(initial_count + 8);	// Maximum of 8 fields.

	const char *s_version;
	switch (d->pvrType) {
		default:
			s_version = nullptr;
			break;
		case PowerVR3Private::PVRType::PVR3:
			s_version = "3.0.0";
			break;
		case PowerVR3Private::PVRType::PVR2:
			s_version = "2.0";
			break;
	}
	if (s_version) {
		fields->addField_string(C_("PowerVR3", "Version"), s_version);
	}

	// Endianness
#if SYS_BYTEORDER == SYS_LIL_ENDIAN
#  define HOST_ENDIAN C_("PowerVR3", "Little-Endian")
#  define SWAP_ENDIAN C_("PowerVR3", "Big-Endian")
#else /* SYS_BYTEORDER != SYS_LIL_ENDIAN */
#  define HOST_ENDIAN C_("PowerVR3", "Big-Endian")
#  define SWAP_ENDIAN C_("PowerVR3", "Little-Endian")
#endif
	const char *const endian_str = (pvr3Header->version == PVR3_VERSION_HOST)
		? HOST_ENDIAN
		: SWAP_ENDIAN;
	fields->addField_string(C_("PowerVR3", "Endianness"), endian_str);

	// Flags
	// NOTE: "Compressed" is listed in the PowerVR Native SDK,
	// but I'm not sure what it's used for...
	static const array<const char*, 2> flags_names = {{
		NOP_C_("PowerVR3|Flags", "Compressed"),
		NOP_C_("PowerVR3|Flags", "Premultipled Alpha"),
	}};
	vector<string> *const v_flags_names = RomFields::strArrayToVector_i18n("PowerVR3|Flags", flags_names);
	fields->addField_bitfield(C_("PowerVR3", "Flags"),
		v_flags_names, 3, pvr3Header->flags);

	// Color space
	static const array<const char*, PVR3_COLOR_SPACE_MAX> pvr3_colorspace_tbl = {{
		NOP_C_("PowerVR3|ColorSpace", "Linear RGB"),
		"sRGB",	// Not translatable
	}};
	static_assert(pvr3_colorspace_tbl.size() == PVR3_COLOR_SPACE_MAX, "pvr3_colorspace_tbl[] needs to be updated!");
	if (pvr3Header->color_space < pvr3_colorspace_tbl.size()) {
		fields->addField_string(C_("PowerVR3", "Color Space"),
			pgettext_expr("PowerVR3|ColorSpace",
				pvr3_colorspace_tbl[pvr3Header->color_space]));
	} else {
		fields->addField_string_numeric(C_("PowerVR3", "Color Space"),
			pvr3Header->color_space);
	}

	// Channel type
	static const array<const char*, PVR3_CHTYPE_MAX> pvr3_chtype_tbl = {{
		NOP_C_("PowerVR3|ChannelType", "Unsigned Byte (normalized)"),
		NOP_C_("PowerVR3|ChannelType", "Signed Byte (normalized)"),
		NOP_C_("PowerVR3|ChannelType", "Unsigned Byte"),
		NOP_C_("PowerVR3|ChannelType", "Signed Byte"),
		NOP_C_("PowerVR3|ChannelType", "Unsigned Short (normalized)"),
		NOP_C_("PowerVR3|ChannelType", "Signed Short (normalized)"),
		NOP_C_("PowerVR3|ChannelType", "Unsigned Short"),
		NOP_C_("PowerVR3|ChannelType", "Signed Short"),
		NOP_C_("PowerVR3|ChannelType", "Unsigned Integer (normalized)"),
		NOP_C_("PowerVR3|ChannelType", "Signed Integer (normalized)"),
		NOP_C_("PowerVR3|ChannelType", "Unsigned Integer"),
		NOP_C_("PowerVR3|ChannelType", "Signed Integer"),
		NOP_C_("PowerVR3|ChannelType", "Float"),
	}};
	static_assert(pvr3_chtype_tbl.size() == PVR3_CHTYPE_MAX, "pvr3_chtype_tbl[] needs to be updated!");
	if (pvr3Header->channel_type < pvr3_chtype_tbl.size()) {
		fields->addField_string(C_("PowerVR3", "Channel Type"),
			pvr3_chtype_tbl[pvr3Header->channel_type]);
			//pgettext_expr("PowerVR3|ChannelType", pvr3_chtype_tbl[pvr3Header->channel_type]));
	} else {
		fields->addField_string_numeric(C_("PowerVR3", "Channel Type"),
			pvr3Header->channel_type);
	}

	// Other numeric fields.
	fields->addField_string_numeric(C_("PowerVR3", "# of Surfaces"),
		pvr3Header->num_surfaces);
	fields->addField_string_numeric(C_("PowerVR3", "# of Faces"),
		pvr3Header->num_faces);

	// Orientation.
	if (d->orientation_valid) {
		// Using KTX-style formatting.
		// TODO: Is 1D set using height or width?
		char s_orientation[] = "S=?,T=?,R=?";
		s_orientation[2] = (d->orientation.x != 0) ? 'l' : 'r';
		if (pvr3Header->height > 1) {
			s_orientation[6] = (d->orientation.y != 0) ? 'u' : 'd';
			if (pvr3Header->depth > 1) {
				s_orientation[10] = (d->orientation.z != 0) ? 'o' : 'i';
			} else {
				s_orientation[7] = '\0';
			}
		} else {
			s_orientation[3] = '\0';
		}
		fields->addField_string(C_("PowerVR3", "Orientation"), s_orientation);
	}

	// TODO: Additional fields.

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
rp_image_const_ptr PowerVR3::image(void) const
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
rp_image_const_ptr PowerVR3::mipmap(int mip) const
{
	RP_D(const PowerVR3);
	if (!d->isValid) {
		// Unknown file type.
		return nullptr;
	}

	// Load the image.
	return const_cast<PowerVR3Private*>(d)->loadImage(mip);
}

} // namespace LibRpTexture
