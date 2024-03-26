/***************************************************************************
 * ROM Properties Page shell extension. (librptexture)                     *
 * XboxXPR.cpp: Microsoft Xbox XPR0 texture reader.                        *
 *                                                                         *
 * Copyright (c) 2019-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "XboxXPR.hpp"
#include "FileFormat_p.hpp"

#include "xbox_xpr_structs.h"

// Other rom-properties libraries
#include "libi18n/i18n.h"
using namespace LibRpFile;
using LibRpBase::RomFields;
using LibRpText::rp_sprintf;

// librptexture
#include "img/rp_image.hpp"
#include "decoder/ImageDecoder_Linear.hpp"
#include "decoder/ImageDecoder_S3TC.hpp"

// C++ STL classes
using std::array;

namespace LibRpTexture {

class XboxXPRPrivate final : public FileFormatPrivate
{
	public:
		XboxXPRPrivate(XboxXPR *q, const IRpFilePtr &file);
		~XboxXPRPrivate() final = default;

	private:
		typedef FileFormatPrivate super;
		RP_DISABLE_COPY(XboxXPRPrivate)

	public:
		/** TextureInfo **/
		static const char *const exts[];
		static const char *const mimeTypes[];
		static const TextureInfo textureInfo;

	public:
		enum class XPRType {
			Unknown = -1,

			XPR0	= 0,	// XPR0
			XPR1	= 1,	// XPR1 (archive)
			XPR2	= 2,	// XPR2 (archive)

			Max
		};
		XPRType xprType;

		// XPR0 header
		Xbox_XPR0_Header xpr0Header;

		// Decoded image
		rp_image_ptr img;

		// Invalid pixel format message
		char invalid_pixel_format[24];

		/**
		 * Generate swizzle masks for unswizzling ARGB textures.
		 * Based on Cxbx-Reloaded's unswizzling code:
		 * https://github.com/Cxbx-Reloaded/Cxbx-Reloaded/blob/5d79c0b66e58bf38d39ea28cb4de954209d1e8ad/src/devices/video/swizzle.cpp
		 * Original license: LGPLv2 (GPLv2 for contributions after 2012/01/13)
		 *
		 * This should be pretty straightforward.
		 * It creates a bit pattern like ..zyxzyxzyx from ..xxx, ..yyy and ..zzz
		 * If there are no bits left from any component it will pack the other masks
		 * more tighly (Example: zzxzxzyx = Fewer x than z and even fewer y)
		 *
		 * rom-properties modification: Removed depth, since we're only
		 * handling 2D textures.
		 *
		 * @param width
		 * @param height
		 * @param mask_x
		 * @param mask_y
		 */
		static void generate_swizzle_masks(
			unsigned int width, unsigned int height,
			uint32_t *mask_x, uint32_t *mask_y);

		/**
		 * This fills a pattern with a value if your value has bits abcd and your
		 * pattern is 11010100100 this will return: 0a0b0c00d00
		 *
		 * Based on Cxbx-Reloaded's unswizzling code:
		 * https://github.com/Cxbx-Reloaded/Cxbx-Reloaded/blob/5d79c0b66e58bf38d39ea28cb4de954209d1e8ad/src/devices/video/swizzle.cpp
		 * Original license: LGPLv2 (GPLv2 for contributions after 2012/01/13)
		 *
		 * @param pattern
		 * @param value
		 * @return
		 */
		static uint32_t fill_pattern(uint32_t pattern, uint32_t value);

		/**
		 * Get a swizzled texture offset.
		 * Based on Cxbx-Reloaded's unswizzling code:
		 * https://github.com/Cxbx-Reloaded/Cxbx-Reloaded/blob/5d79c0b66e58bf38d39ea28cb4de954209d1e8ad/src/devices/video/swizzle.cpp
		 * Original license: LGPLv2 (GPLv2 for contributions after 2012/01/13)
		 *
		 * rom-properties modification: Removed depth, since we're only
		 * handling 2D textures.
		 *
		 * @param x
		 * @param y
		 * @param mask_x
		 * @param mask_y
		 * @param bytes_per_pixel
		 * @return
		 */
		static inline unsigned int get_swizzled_offset(
			unsigned int x, unsigned int y,
			uint32_t mask_x, uint32_t mask_y,
			unsigned int bytes_per_pixel);

		/**
		 * Unswizzle an ARGB texture.
		 * Based on Cxbx-Reloaded's unswizzling code:
		 * https://github.com/Cxbx-Reloaded/Cxbx-Reloaded/blob/5d79c0b66e58bf38d39ea28cb4de954209d1e8ad/src/devices/video/swizzle.cpp
		 * Original license: LGPLv2 (GPLv2 for contributions after 2012/01/13)
		 *
		 * rom-properties modification: Removed depth, since we're only
		 * handling 2D textures. Also removed slice_pitch, since we don't
		 * have any slices here.
		 *
		 * @param src_buf
		 * @param width
		 * @param height
		 * @param dst_buf
		 * @param row_pitch
		 * @param slice_pitch
		 * @param bytes_per_pixel
		 */
		static void unswizzle_box(const uint8_t *src_buf,
			unsigned int width, unsigned int height,
			uint8_t *dst_buf, unsigned int row_pitch,
			unsigned int bytes_per_pixel);

		/**
		 * Load the XboxXPR image.
		 * @return Image, or nullptr on error.
		 */
		rp_image_const_ptr loadXboxXPR0Image(void);
};

FILEFORMAT_IMPL(XboxXPR)

/** XboxXPRPrivate **/

/* TextureInfo */
const char *const XboxXPRPrivate::exts[] = {
	".xbx", ".xpr",

	nullptr
};
const char *const XboxXPRPrivate::mimeTypes[] = {
	// Unofficial MIME types.
	// TODO: Get these upstreamed on FreeDesktop.org.
	// TODO: Add additional MIME types for XPR1/XPR2. (archive files)
	"image/x-xbox-xpr0",

	nullptr
};
const TextureInfo XboxXPRPrivate::textureInfo = {
	exts, mimeTypes
};

XboxXPRPrivate::XboxXPRPrivate(XboxXPR *q, const IRpFilePtr &file)
	: super(q, file, &textureInfo)
	, xprType(XPRType::Unknown)
{
	// Clear the structs and arrays.
	memset(&xpr0Header, 0, sizeof(xpr0Header));
	memset(invalid_pixel_format, 0, sizeof(invalid_pixel_format));
}

/**
 * Generate swizzle masks for unswizzling ARGB textures.
 * Based on Cxbx-Reloaded's unswizzling code:
 * https://github.com/Cxbx-Reloaded/Cxbx-Reloaded/blob/5d79c0b66e58bf38d39ea28cb4de954209d1e8ad/src/devices/video/swizzle.cpp
 * Original license: LGPLv2 (GPLv2 for contributions after 2012/01/13)
 *
 * This should be pretty straightforward.
 * It creates a bit pattern like ..zyxzyxzyx from ..xxx, ..yyy and ..zzz
 * If there are no bits left from any component it will pack the other masks
 * more tighly (Example: zzxzxzyx = Fewer x than z and even fewer y)
 *
 * rom-properties modification: Removed depth, since we're only
 * handling 2D textures.
 *
 * @param width
 * @param height
 * @param mask_x
 * @param mask_y
 */
void XboxXPRPrivate::generate_swizzle_masks(
	unsigned int width, unsigned int height,
	uint32_t *mask_x, uint32_t *mask_y)
{
	uint32_t x = 0, y = 0;
	uint32_t bit = 1;
	uint32_t mask_bit = 1;
	bool done;
	do {
		done = true;
		if (bit < width) { x |= mask_bit; mask_bit <<= 1; done = false; }
		if (bit < height) { y |= mask_bit; mask_bit <<= 1; done = false; }
		bit <<= 1;
	} while(!done);
	assert((x ^ y) == (mask_bit - 1));
	*mask_x = x;
	*mask_y = y;
}

/**
 * This fills a pattern with a value if your value has bits abcd and your
 * pattern is 11010100100 this will return: 0a0b0c00d00
 *
 * Based on Cxbx-Reloaded's unswizzling code:
 * https://github.com/Cxbx-Reloaded/Cxbx-Reloaded/blob/5d79c0b66e58bf38d39ea28cb4de954209d1e8ad/src/devices/video/swizzle.cpp
 * Original license: LGPLv2 (GPLv2 for contributions after 2012/01/13)
 *
 * @param pattern
 * @param value
 * @return
 */
uint32_t XboxXPRPrivate::fill_pattern(uint32_t pattern, uint32_t value)
{
	uint32_t result = 0;
	uint32_t bit = 1;
	while(value) {
		if (pattern & bit) {
			/* Copy bit to result */
			result |= value & 1 ? bit : 0;
			value >>= 1;
		}
		bit <<= 1;
	}
	return result;
}

/**
 * Get a swizzled texture offset.
 * Based on Cxbx-Reloaded's unswizzling code:
 * https://github.com/Cxbx-Reloaded/Cxbx-Reloaded/blob/5d79c0b66e58bf38d39ea28cb4de954209d1e8ad/src/devices/video/swizzle.cpp
 * Original license: LGPLv2 (GPLv2 for contributions after 2012/01/13)
 *
 * rom-properties modification: Removed depth, since we're only
 * handling 2D textures.
 *
 * @param x
 * @param y
 * @param mask_x
 * @param mask_y
 * @param bytes_per_pixel
 * @return
 */
inline unsigned int XboxXPRPrivate::get_swizzled_offset(
	unsigned int x, unsigned int y,
	uint32_t mask_x, uint32_t mask_y,
	unsigned int bytes_per_pixel)
{
	return bytes_per_pixel * (fill_pattern(mask_x, x)
	                       |  fill_pattern(mask_y, y));
}

/**
 * Unswizzle an ARGB texture.
 * Based on Cxbx-Reloaded's unswizzling code:
 * https://github.com/Cxbx-Reloaded/Cxbx-Reloaded/blob/5d79c0b66e58bf38d39ea28cb4de954209d1e8ad/src/devices/video/swizzle.cpp
 * Original license: LGPLv2 (GPLv2 for contributions after 2012/01/13)
 *
 * rom-properties modification: Removed depth, since we're only
 * handling 2D textures. Also removed slice_pitch, since we don't
 * have any slices here.
 *
 * @param src_buf
 * @param width
 * @param height
 * @param dst_buf
 * @param row_pitch
 * @param slice_pitch
 * @param bytes_per_pixel
 */
void XboxXPRPrivate::unswizzle_box(const uint8_t *src_buf,
	unsigned int width, unsigned int height,
	uint8_t *dst_buf, unsigned int row_pitch,
	unsigned int bytes_per_pixel)
{
	uint32_t mask_x, mask_y;
	generate_swizzle_masks(width, height, &mask_x, &mask_y);

	for (unsigned int y = 0; y < height; y++) {
		for (unsigned int x = 0; x < width; x++) {
			const uint8_t *src = src_buf +
				get_swizzled_offset(x, y, mask_x, mask_y, bytes_per_pixel);
			uint8_t *dst = dst_buf + y * row_pitch + x * bytes_per_pixel;
			memcpy(dst, src, bytes_per_pixel);
		}
	}
}

/**
 * Load the XPR0 image.
 * @return Image, or nullptr on error.
 */
rp_image_const_ptr XboxXPRPrivate::loadXboxXPR0Image(void)
{
	if (img) {
		// Image has already been loaded.
		return img;
	} else if (!this->file) {
		// Can't load the image.
		return nullptr;
	}

	// Sanity check: XPR0 files shouldn't be more than 16 MB.
	assert(file->size() <= 16*1024*1024);
	if (file->size() > 16*1024*1024) {
		return nullptr;
	}

	// XPR0 textures are always square and encoded using DXT1.
	// TODO: Maybe other formats besides DXT1?

	// DXT1 is 8 bytes per 4x4 pixel block.
	// Divide the image area by 2.
	const uint32_t file_sz = static_cast<uint32_t>(file->size());
	const uint32_t data_offset = le32_to_cpu(xpr0Header.data_offset);

	// Sanity check: Image dimensions must be non-zero.
	// Not checking maximum; the 4-bit shift amount has a
	// maximum of pow(2,15), which is 32768 (our maximum).
	const int width  = dimensions[0];
	const int height = dimensions[1];
	assert(width > 0);
	assert(height > 0);
	if (width == 0 || height == 0) {
		// Invalid image dimensions.
		return nullptr;
	}

	// Mode table
	// Index is XPR0_Pixel_Format_e.
	// Reference: https://github.com/Cxbx-Reloaded/Cxbx-Reloaded/blob/c709f9e3054ad8e1dae62816f25bef06248415c4/src/core/hle/D3D8/XbConvert.cpp#L871
	// TODO: Test these formats.
	// Tested formats: ARGB4444, ARGB8888, DXT1, DXT2
	struct xpr_mode_t {
		uint8_t bpp;	// Bits per pixel (4, 8, 16, 32; 0 for invalid)
				// TODO: Use a shift amount instead?
		ImageDecoder::PixelFormat pxf;	// ImageDecoder::PixelFormat
		uint8_t dxtn;	// DXTn version (pxf must be PixelFormat::Unknown)
		bool swizzled;	// True if the format needs to be unswizzled
				// DXTn is automatically unswizzled by the DXTn
				// functions, so those should be false.
	};
	static constexpr array<xpr_mode_t, 0x42> xpr_mode_tbl = {{
		{ 8, ImageDecoder::PixelFormat::L8,		0, true},	// 0x00: L8
		{ 0, ImageDecoder::PixelFormat::Unknown,	0, true},	// 0x01: AL8 (TODO)
		{16, ImageDecoder::PixelFormat::ARGB1555, 	0, true},	// 0x02: ARGB1555
		{16, ImageDecoder::PixelFormat::RGB555,		0, true},	// 0x03: RGB555
		{16, ImageDecoder::PixelFormat::ARGB4444,	0, true},	// 0x04: ARGB4444
		{16, ImageDecoder::PixelFormat::RGB565,		0, true},	// 0x05: RGB565
		{32, ImageDecoder::PixelFormat::ARGB8888,	0, true},	// 0x06: ARGB8888
		{32, ImageDecoder::PixelFormat::xRGB8888,	0, true},	// 0x07: xRGB8888
		{ 0, ImageDecoder::PixelFormat::Unknown,	0, false},	// 0x08: undefined
		{ 0, ImageDecoder::PixelFormat::Unknown,	0, false},	// 0x09: undefined
		{ 0, ImageDecoder::PixelFormat::Unknown,	0, false},	// 0x0A: undefined
		{ 0, ImageDecoder::PixelFormat::Unknown,	0, true},	// 0x0B: P8 (TODO)
		{ 4, ImageDecoder::PixelFormat::Unknown,	1, false},	// 0x0C: DXT1
		{ 0, ImageDecoder::PixelFormat::Unknown,	0, false},	// 0x0D: undefined
		{ 8, ImageDecoder::PixelFormat::Unknown,	2, false},	// 0x0E: DXT2
		{ 8, ImageDecoder::PixelFormat::Unknown,	4, false},	// 0x0F: DXT4

		{16, ImageDecoder::PixelFormat::ARGB1555,	0, false},	// 0x10: Linear ARGB1555
		{16, ImageDecoder::PixelFormat::RGB565,		0, false},	// 0x11: Linear RGB565
		{32, ImageDecoder::PixelFormat::ARGB8888,	0, false},	// 0x12: Linear ARGB8888
		{ 8, ImageDecoder::PixelFormat::L8,		0, false},	// 0x13: Linear L8
		{ 0, ImageDecoder::PixelFormat::Unknown,	0, false},	// 0x14: undefined
		{ 0, ImageDecoder::PixelFormat::Unknown,	0, false},	// 0x15: undefined
		{ 0, ImageDecoder::PixelFormat::Unknown,	0, false},	// 0x16: Linear R8B8 (TODO)
		{ 0, ImageDecoder::PixelFormat::Unknown,	0, false},	// 0x17: Linear G8B8 (TODO)
		{ 0, ImageDecoder::PixelFormat::Unknown,	0, false},	// 0x18: undefined
		{ 8, ImageDecoder::PixelFormat::A8,		0, true},	// 0x19: A8
		{16, ImageDecoder::PixelFormat::A8L8,		0, true},	// 0x1A: A8L8
		{ 0, ImageDecoder::PixelFormat::Unknown,	0, false},	// 0x1B: Linear AL8 (TODO)
		{16, ImageDecoder::PixelFormat::RGB555,		0, false},	// 0x1C: Linear RGB555
		{16, ImageDecoder::PixelFormat::ARGB4444,	0, false},	// 0x1D: Linear ARGB4444
		{32, ImageDecoder::PixelFormat::xRGB8888,	0, false},	// 0x1E: Linear xRGB8888
		{ 8, ImageDecoder::PixelFormat::A8,		0, false},	// 0x1F: Linear A8

		{16, ImageDecoder::PixelFormat::A8L8,		0, false},	// 0x20: Linear A8L8
		{ 0, ImageDecoder::PixelFormat::Unknown,	0, false},	// 0x21: undefined
		{ 0, ImageDecoder::PixelFormat::Unknown,	0, false},	// 0x22: undefined
		{ 0, ImageDecoder::PixelFormat::Unknown,	0, false},	// 0x23: undefined
		{ 0, ImageDecoder::PixelFormat::Unknown,	0, true},	// 0x24: YUY2 (TODO)
		{ 0, ImageDecoder::PixelFormat::Unknown,	0, true},	// 0x25: YUY2 (TODO)
		{ 0, ImageDecoder::PixelFormat::Unknown,	0, false},	// 0x26: undefined
		{ 0, ImageDecoder::PixelFormat::Unknown,	0, true},	// 0x27: L6V5U5 (TODO)
		{ 0, ImageDecoder::PixelFormat::Unknown,	0, true},	// 0x28: V8U8 (TODO)
		{ 0, ImageDecoder::PixelFormat::Unknown,	0, true},	// 0x29: R8B8 (TODO)
		{ 0, ImageDecoder::PixelFormat::Unknown,	0, true},	// 0x2A: D24S8 (TODO)
		{ 0, ImageDecoder::PixelFormat::Unknown,	0, true},	// 0x2B: F24S8 (TODO)
		{ 0, ImageDecoder::PixelFormat::Unknown,	0, true},	// 0x2C: D16 (TODO)
		{ 0, ImageDecoder::PixelFormat::Unknown,	0, true},	// 0x2D: F16 (TODO)
		{ 0, ImageDecoder::PixelFormat::Unknown,	0, false},	// 0x2E: Linear D24S8 (TODO)
		{ 0, ImageDecoder::PixelFormat::Unknown,	0, false},	// 0x2F: Linear F24S8 (TODO)

		{ 0, ImageDecoder::PixelFormat::Unknown,	0, false},	// 0x30: Linear D16 (TODO)
		{ 0, ImageDecoder::PixelFormat::Unknown,	0, false},	// 0x31: Linear F16 (TODO)
		{16, ImageDecoder::PixelFormat::L16,		0, true},	// 0x32: L16
		{ 0, ImageDecoder::PixelFormat::Unknown,	0, true},	// 0x33: V16U16 (TODO)
		{ 0, ImageDecoder::PixelFormat::Unknown,	0, false},	// 0x34: undefined
		{ 0, ImageDecoder::PixelFormat::L16,		0, false},	// 0x35: Linear L16
		{ 0, ImageDecoder::PixelFormat::Unknown,	0, false},	// 0x36: Linear V16U16 (TODO)
		{ 0, ImageDecoder::PixelFormat::Unknown,	0, false},	// 0x37: Linear L6V5U5 (TODO)
		{16, ImageDecoder::PixelFormat::RGBA5551,	0, true},	// 0x38: RGBA5551
		{16, ImageDecoder::PixelFormat::RGBA4444,	0, true},	// 0x39: RGBA4444
		{32, ImageDecoder::PixelFormat::ABGR8888,	0, true},	// 0x3A: QWVU8888 (same as ABGR8888)
		{32, ImageDecoder::PixelFormat::BGRA8888,	0, true},	// 0x3B: BGRA8888
		{32, ImageDecoder::PixelFormat::RGBA8888,	0, true},	// 0x3C: RGBA8888
		{16, ImageDecoder::PixelFormat::RGBA5551,	0, false},	// 0x3D: Linear RGBA5551
		{16, ImageDecoder::PixelFormat::RGBA4444,	0, false},	// 0x3E: Linear RGBA4444
		{32, ImageDecoder::PixelFormat::ABGR8888,	0, false},	// 0x3F: Linear ABGR8888

		{32, ImageDecoder::PixelFormat::BGRA8888,	0, false},	// 0x40: Linear BGRA8888
		{32, ImageDecoder::PixelFormat::RGBA8888,	0, false},	// 0x41: Linear RGBA8888
	}};

	if (xpr0Header.pixel_format >= xpr_mode_tbl.size()) {
		// Invalid pixel format.
		return nullptr;
	}

	// Determine the expected size based on the pixel format.
	const auto &mode = xpr_mode_tbl[xpr0Header.pixel_format];
	const size_t expected_size = width * height * mode.bpp / 8U;

	if (expected_size > file_sz - data_offset) {
		// File is too small.
		return nullptr;
	}

	// Read the image data.
	auto buf = aligned_uptr<uint8_t>(16, expected_size);
	size_t size = file->seekAndRead(data_offset, buf.get(), expected_size);
	if (size != expected_size) {
		// Seek and/or read error.
		return nullptr;
	}

	if (mode.dxtn != 0) {
		// DXTn
		switch (mode.dxtn) {
			case 1:
				// NOTE: Assuming we have transparent pixels.
				img = ImageDecoder::fromDXT1_A1(width, height,
					buf.get(), expected_size);
				break;
			case 2:
				img = ImageDecoder::fromDXT2(width, height,
					buf.get(), expected_size);
				break;
			case 4:
				img = ImageDecoder::fromDXT4(width, height,
					buf.get(), expected_size);
				break;
			default:
				assert(!"Unsupported DXTn format.");
				return nullptr;
		}
	} else {
		switch (mode.bpp) {
			case 8:
				img = ImageDecoder::fromLinear8(
					mode.pxf, width, height,
					buf.get(), expected_size);
				break;
			case 16:
				img = ImageDecoder::fromLinear16(
					mode.pxf, width, height,
					reinterpret_cast<const uint16_t*>(buf.get()), expected_size);
				break;
			case 32:
				img = ImageDecoder::fromLinear32(
					mode.pxf, width, height,
					reinterpret_cast<const uint32_t*>(buf.get()), expected_size);
				break;
			case 0:
			default:
				assert(!"Unsupported bpp value.");
				return nullptr;
		}
	}

	if (!img) {
		// Unable to decode the image.
		return nullptr;
	}

	if (mode.swizzled) {
		// Image is swizzled.
		// Unswizzling code is based on Cxbx-Reloaded:
		// https://github.com/Cxbx-Reloaded/Cxbx-Reloaded/blob/5d79c0b66e58bf38d39ea28cb4de954209d1e8ad/src/devices/video/swizzle.cpp

		// Image dimensions must be a multiple of 4.
		assert(width % 4 == 0);
		assert(height % 4 == 0);
		if (width % 4 != 0 || height % 4 != 0) {
			// Not a multiple of 4.
			// Return the image as-is.
			return img;
		}

		// Assuming we don't have any extra bytes of stride,
		// since the image must be a multiple of 4px wide.
		// 4px ARGB32 is 16 bytes.
		assert(img->stride() == img->row_bytes());
		if (img->stride() != img->row_bytes()) {
			// We have extra bytes.
			// Can't unswizzle this image right now.
			// Return the image as-is.
			return img;
		}

		// Assuming img is ARGB32, since we're converting it
		// from either a 16-bit or 32-bit ARGB format.
		rp_image_ptr imgunswz = std::make_shared<rp_image>(width, height, rp_image::Format::ARGB32);
		unswizzle_box(static_cast<const uint8_t*>(img->bits()),
			width, height,
			static_cast<uint8_t*>(imgunswz->bits()),
			img->stride(), sizeof(uint32_t));
		img = imgunswz;
	}

	return img;
}

/** XboxXPR **/

/**
 * Read a Microsoft Xbox XPR0 image file.
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
XboxXPR::XboxXPR(const IRpFilePtr &file)
	: super(new XboxXPRPrivate(this, file))
{
	RP_D(XboxXPR);
	// TODO: XPR1/XPR2?
	d->mimeType = "image/x-xbox-xpr0";	// unofficial, not on fd.o
	d->textureFormatName = "Microsoft Xbox XPR0";

	if (!d->file) {
		// Could not ref() the file handle.
		return;
	}

	// Read the XPR0 header.
	d->file->rewind();
	size_t size = d->file->read(&d->xpr0Header, sizeof(d->xpr0Header));
	if (size != sizeof(d->xpr0Header)) {
		d->file.reset();
		return;
	}

	// Verify the XPR0 magic.
	if (d->xpr0Header.magic == cpu_to_be32(XBOX_XPR0_MAGIC)) {
		// This is an XPR0 image.
		d->xprType = XboxXPRPrivate::XPRType::XPR0;
		d->isValid = true;
	} else if (d->xpr0Header.magic == cpu_to_be32(XBOX_XPR1_MAGIC)) {
		// This is an XPR1 archive.
		d->xprType = XboxXPRPrivate::XPRType::XPR1;
		// NOT SUPPORTED YET
		d->isValid = false;
	} else if (d->xpr0Header.magic == cpu_to_be32(XBOX_XPR2_MAGIC)) {
		// This is an XPR2 archive.
		d->xprType = XboxXPRPrivate::XPRType::XPR2;
		// NOT SUPPORTED YET
		d->isValid = false;
	}

	// Make sure this is an XPR texture.
	const uint16_t type = le16_to_cpu(d->xpr0Header.type);
	if ((type & XPR0_TYPE_MASK) != XPR0_TYPE_TEXTURE) {
		// Only textures are supported.
		d->isValid = false;
	}

	if (!d->isValid) {
		d->file.reset();
		return;
	}

	// Cache the texture dimensions.
	const uint8_t xpr_w = (d->xpr0Header.width_pow2 >> 4);
	const uint8_t xpr_h = (d->xpr0Header.height_pow2 & 0x0F);
	if (likely(xpr_w != 0 && xpr_h != 0)) {
		// Use the standard width and height.
		d->dimensions[0] = 1 << xpr_w;
		d->dimensions[1] = 1 << xpr_h;
	} else {
		// Either width or height are 0. Try the NPOT sizes instead.
		d->dimensions[0] = (d->xpr0Header.width_npot + 1) * 16;
		d->dimensions[1] = (d->xpr0Header.height_npot + 1) * 16;
	}
	d->dimensions[2] = 0;

	// TODO: Does XPR0 support mipmaps?
}

/** Property accessors **/

/**
 * Get the pixel format, e.g. "RGB888" or "DXT1".
 * @return Pixel format, or nullptr if unavailable.
 */
const char *XboxXPR::pixelFormat(void) const
{
	RP_D(const XboxXPR);
	if (!d->isValid || (int)d->xprType < 0) {
		// Not supported.
		return nullptr;
	}

	static constexpr array<const char*, 0x65> pxfmt_tbl = {{
		// 0x00
		"L8", "AL8", "ARGB1555", "RGB555",
		"ARGB4444", "RGB565", "ARGB8888", "xRGB8888",

		// 0x08
		nullptr, nullptr, nullptr, "P8",
		"DXT1", nullptr, "DXT2", "DXT4",

		// 0x10
		"Linear ARGB1555", "Linear RGB565",
		"Linear ARGB8888", "Linear L8",
		nullptr, nullptr,
		"Linear R8B8", "Linear G8B8",

		// 0x18
		nullptr, "A8", "A8L8", "Linear AL8",
		"Linear RGB555", "Linear ARGB4444",
		"Linear xRGB8888", "Linear A8",

		// 0x20
		"Linear A8L8", nullptr, nullptr, nullptr,
		"YUY2", "UYVY", nullptr, "L6V5U5",

		// 0x28
		"V8U8", "R8B8", "D24S8", "F24S8",
		"D16", "F16", "Linear D24S8", "Linear F24S8",

		// 0x30
		"Linear D16", "Linear F16", "L16", "V16U16",
		nullptr, "Linear L16", "Linear V16U16", "Linear L6V5U5",

		// 0x38
		"RGBA5551", "RGBA4444", "QWVU8888", "BGRA8888",
		"RGBA8888", "Linear RGBA5551",
		"Linear RGBA4444", "Linear ABGR8888",

		// 0x40
		"Linear BGRA8888", "Linear RGBA8888", nullptr, nullptr,
		nullptr, nullptr, nullptr, nullptr,

		// 0x48
		nullptr, nullptr, nullptr, nullptr,
		nullptr, nullptr, nullptr, nullptr,

		// 0x50
		nullptr, nullptr, nullptr, nullptr,
		nullptr, nullptr, nullptr, nullptr,
		nullptr, nullptr, nullptr, nullptr,
		nullptr, nullptr, nullptr, nullptr,

		// 0x60
		nullptr, nullptr, nullptr, "Vertex Data",
		"Index16",
	}};

	if (d->xpr0Header.pixel_format < pxfmt_tbl.size()) {
		return pxfmt_tbl[d->xpr0Header.pixel_format];
	}

	// Invalid pixel format.
	// Store an error message instead.
	// TODO: Localization?
	if (d->invalid_pixel_format[0] == '\0') {
		snprintf(const_cast<XboxXPRPrivate*>(d)->invalid_pixel_format,
			sizeof(d->invalid_pixel_format),
			"Unknown (0x%02X)", d->xpr0Header.pixel_format);
	}
	return d->invalid_pixel_format;
}

#ifdef ENABLE_LIBRPBASE_ROMFIELDS
/**
 * Get property fields for rom-properties.
 * @param fields RomFields object to which fields should be added.
 * @return Number of fields added, or 0 on error.
 */
int XboxXPR::getFields(RomFields *fields) const
{
	assert(fields != nullptr);
	if (!fields)
		return 0;

	RP_D(const XboxXPR);
	if (!d->isValid || (int)d->xprType < 0) {
		// Unknown XPR image type.
		return -EIO;
	}

	const int initial_count = fields->count();
	fields->reserve(initial_count + 1);	// Maximum of 1 field. (TODO)

	// Type
	static const char type_tbl[][8] = {
		"XPR0", "XPR1", "XPR2"
	};
	if (d->xprType > XboxXPRPrivate::XPRType::Unknown &&
	    (int)d->xprType < ARRAY_SIZE_I(type_tbl))
	{
		fields->addField_string(C_("XboxXPR", "Type"), type_tbl[(int)d->xprType]);
	} else {
		fields->addField_string(C_("XboxXPR", "Type"),
			rp_sprintf(C_("RomData", "Unknown (%d)"), (int)d->xprType));
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
rp_image_const_ptr XboxXPR::image(void) const
{
	RP_D(const XboxXPR);
	if (!d->isValid || (int)d->xprType < 0) {
		// Unknown file type.
		return nullptr;
	}

	// Load the image.
	return const_cast<XboxXPRPrivate*>(d)->loadXboxXPR0Image();
}

}
