/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * XboxXPR.cpp: Microsoft Xbox XPR0 image reader.                          *
 *                                                                         *
 * Copyright (c) 2019 by David Korth.                                      *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "XboxXPR.hpp"
#include "librpbase/RomData_p.hpp"

#include "xbox_xpr_structs.h"

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
#include <vector>
using std::vector;

namespace LibRomData {

ROMDATA_IMPL(XboxXPR)
ROMDATA_IMPL_IMG_TYPES(XboxXPR)

class XboxXPRPrivate : public RomDataPrivate
{
	public:
		XboxXPRPrivate(XboxXPR *q, IRpFile *file);
		~XboxXPRPrivate();

	private:
		typedef RomDataPrivate super;
		RP_DISABLE_COPY(XboxXPRPrivate)

	public:
		enum XPRType {
			XPR_TYPE_UNKNOWN	= -1,	// Unknown image type.
			XPR_TYPE_XPR0		= 0,	// XPR0
			XPR_TYPE_XPR1		= 1,	// XPR1

			XPR_TYPE_MAX
		};

		// XPR type.
		int xprType;

		// XPR0 header.
		Xbox_XPR0_Header xpr0Header;

		// Decoded image.
		rp_image *img;

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
		const rp_image *loadXboxXPR0Image(void);
};

/** XboxXPRPrivate **/

XboxXPRPrivate::XboxXPRPrivate(XboxXPR *q, IRpFile *file)
	: super(q, file)
	, xprType(XPR_TYPE_UNKNOWN)
	, img(nullptr)
{
	// Clear the XPR0 header struct.
	memset(&xpr0Header, 0, sizeof(xpr0Header));
}

XboxXPRPrivate::~XboxXPRPrivate()
{
	delete img;
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
const rp_image *XboxXPRPrivate::loadXboxXPR0Image(void)
{
	if (img) {
		// Image has already been loaded.
		return img;
	} else if (!this->file) {
		// Can't load the image.
		return nullptr;
	}

	// Sanity check: XPR0 files shouldn't be more than 16 MB.
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
	assert((xpr0Header.width_pow2 >> 4) > 0);
	assert((xpr0Header.height_pow2 & 0x0F) > 0);
	if ((xpr0Header.width_pow2 >> 4) == 0 ||
	    (xpr0Header.height_pow2 & 0x0F) == 0)
	{
		// Invalid image dimensions.
		return nullptr;
	}

	// Mode table.
	// Index is XPR0_Pixel_Format_e.
	// Reference: https://github.com/Cxbx-Reloaded/Cxbx-Reloaded/blob/c709f9e3054ad8e1dae62816f25bef06248415c4/src/core/hle/D3D8/XbConvert.cpp#L871
	// TODO: Test these formats.
	// Tested formats: ARGB4444, ARGB8888, DXT1, DXT2
	static const struct {
		uint8_t bpp;	// Bits per pixel (4, 8, 16, 32; 0 for invalid)
				// TODO: Use a shift amount instead?
		uint8_t pxf;	// ImageDecoder::PixelFormat
		uint8_t dxtn;	// DXTn version (pxf must be PXF_UNKNOWN)
		bool swizzled;	// True if the format needs to be unswizzled
				// DXTn is automatically unswizzled by the DXTn
				// functions, so those should be false.
	} mode_tbl[] = {
		{ 8, ImageDecoder::PXF_L8,		0, true},	// 0x00: L8
		{ 0, ImageDecoder::PXF_UNKNOWN,		0, true},	// 0x01: AL8 (TODO)
		{16, ImageDecoder::PXF_ARGB1555, 	0, true},	// 0x02: ARGB1555
		{16, ImageDecoder::PXF_RGB555,		0, true},	// 0x03: RGB555
		{16, ImageDecoder::PXF_ARGB4444,	0, true},	// 0x04: ARGB4444
		{16, ImageDecoder::PXF_RGB565,		0, true},	// 0x05: RGB565
		{32, ImageDecoder::PXF_ARGB8888,	0, true},	// 0x06: ARGB8888
		{32, ImageDecoder::PXF_xRGB8888,	0, true},	// 0x07: xRGB8888
		{ 0, ImageDecoder::PXF_UNKNOWN,		0, false},	// 0x08: undefined
		{ 0, ImageDecoder::PXF_UNKNOWN,		0, false},	// 0x09: undefined
		{ 0, ImageDecoder::PXF_UNKNOWN,		0, false},	// 0x0A: undefined
		{ 0, ImageDecoder::PXF_UNKNOWN,		0, true},	// 0x0B: P8 (TODO)
		{ 4, ImageDecoder::PXF_UNKNOWN,		1, false},	// 0x0C: DXT1
		{ 0, ImageDecoder::PXF_UNKNOWN,		0, false},	// 0x0D: undefined
		{ 8, ImageDecoder::PXF_UNKNOWN,		2, false},	// 0x0E: DXT2
		{ 8, ImageDecoder::PXF_UNKNOWN,		4, false},	// 0x0F: DXT4

		{16, ImageDecoder::PXF_ARGB1555,	0, false},	// 0x10: Linear ARGB1555
		{16, ImageDecoder::PXF_RGB565,		0, false},	// 0x11: Linear RGB565
		{32, ImageDecoder::PXF_ARGB8888,	0, false},	// 0x12: Linear ARGB8888
		{ 8, ImageDecoder::PXF_L8,		0, false},	// 0x13: Linear L8
		{ 0, ImageDecoder::PXF_UNKNOWN,		0, false},	// 0x14: undefined
		{ 0, ImageDecoder::PXF_UNKNOWN,		0, false},	// 0x15: undefined
		{ 0, ImageDecoder::PXF_UNKNOWN,		0, false},	// 0x16: Linear R8B8 (TODO)
		{ 0, ImageDecoder::PXF_UNKNOWN,		0, false},	// 0x17: Linear G8B8 (TODO)
		{ 0, ImageDecoder::PXF_UNKNOWN,		0, false},	// 0x18: undefined
		{ 8, ImageDecoder::PXF_A8,		0, true},	// 0x19: A8
		{16, ImageDecoder::PXF_A8L8,		0, true},	// 0x1A: A8L8
		{ 0, ImageDecoder::PXF_UNKNOWN,		0, false},	// 0x1B: Linear AL8 (TODO)
		{16, ImageDecoder::PXF_RGB555,		0, false},	// 0x1C: Linear RGB555
		{16, ImageDecoder::PXF_ARGB4444,	0, false},	// 0x1D: Linear ARGB4444
		{32, ImageDecoder::PXF_xRGB8888,	0, false},	// 0x1E: Linear xRGB8888
		{ 8, ImageDecoder::PXF_A8,		0, false},	// 0x1F: Linear A8

		{16, ImageDecoder::PXF_A8L8,		0, false},	// 0x20: Linear A8L8
		{ 0, ImageDecoder::PXF_UNKNOWN,		0, false},	// 0x21: undefined
		{ 0, ImageDecoder::PXF_UNKNOWN,		0, false},	// 0x22: undefined
		{ 0, ImageDecoder::PXF_UNKNOWN,		0, false},	// 0x23: undefined
		{ 0, ImageDecoder::PXF_UNKNOWN,		0, true},	// 0x24: YUY2 (TODO)
		{ 0, ImageDecoder::PXF_UNKNOWN,		0, true},	// 0x25: YUY2 (TODO)
		{ 0, ImageDecoder::PXF_UNKNOWN,		0, false},	// 0x26: undefined
		{ 0, ImageDecoder::PXF_UNKNOWN,		0, true},	// 0x27: L6V5U5 (TODO)
		{ 0, ImageDecoder::PXF_UNKNOWN,		0, true},	// 0x28: V8U8 (TODO)
		{ 0, ImageDecoder::PXF_UNKNOWN,		0, true},	// 0x29: R8B8 (TODO)
		{ 0, ImageDecoder::PXF_UNKNOWN,		0, true},	// 0x2A: D24S8 (TODO)
		{ 0, ImageDecoder::PXF_UNKNOWN,		0, true},	// 0x2B: F24S8 (TODO)
		{ 0, ImageDecoder::PXF_UNKNOWN,		0, true},	// 0x2C: D16 (TODO)
		{ 0, ImageDecoder::PXF_UNKNOWN,		0, true},	// 0x2D: F16 (TODO)
		{ 0, ImageDecoder::PXF_UNKNOWN,		0, false},	// 0x2E: Linear D24S8 (TODO)
		{ 0, ImageDecoder::PXF_UNKNOWN,		0, false},	// 0x2F: Linear F24S8 (TODO)

		{ 0, ImageDecoder::PXF_UNKNOWN,		0, false},	// 0x30: Linear D16 (TODO)
		{ 0, ImageDecoder::PXF_UNKNOWN,		0, false},	// 0x31: Linear F16 (TODO)
		{16, ImageDecoder::PXF_L16,		0, true},	// 0x32: L16
		{ 0, ImageDecoder::PXF_UNKNOWN,		0, true},	// 0x33: V16U16 (TODO)
		{ 0, ImageDecoder::PXF_UNKNOWN,		0, false},	// 0x34: undefined
		{ 0, ImageDecoder::PXF_L16,		0, false},	// 0x35: Linear L16
		{ 0, ImageDecoder::PXF_UNKNOWN,		0, false},	// 0x36: Linear V16U16 (TODO)
		{ 0, ImageDecoder::PXF_UNKNOWN,		0, false},	// 0x37: Linear L6V5U5 (TODO)
		{16, ImageDecoder::PXF_RGBA5551,	0, true},	// 0x38: RGBA5551
		{16, ImageDecoder::PXF_RGBA4444,	0, true},	// 0x39: RGBA4444
		{32, ImageDecoder::PXF_ABGR8888,	0, true},	// 0x3A: QWVU8888 (same as ABGR8888)
		{32, ImageDecoder::PXF_BGRA8888,	0, true},	// 0x3B: BGRA8888
		{32, ImageDecoder::PXF_RGBA8888,	0, true},	// 0x3C: RGBA8888
		{16, ImageDecoder::PXF_RGBA5551,	0, false},	// 0x3D: Linear RGBA5551
		{16, ImageDecoder::PXF_RGBA4444,	0, false},	// 0x3E: Linear RGBA4444
		{32, ImageDecoder::PXF_ABGR8888,	0, false},	// 0x3F: Linear ABGR8888

		{32, ImageDecoder::PXF_BGRA8888,	0, false},	// 0x3F: Linear BGRA8888
		{32, ImageDecoder::PXF_RGBA8888,	0, false},	// 0x3F: Linear RGBA8888
	};

	if (xpr0Header.pixel_format >= ARRAY_SIZE(mode_tbl)) {
		// Invalid pixel format.
		return nullptr;
	}

	// Determine the expected size based on the pixel format.
	const unsigned int area_shift = (xpr0Header.width_pow2 >> 4) +
					(xpr0Header.height_pow2 & 0x0F);
	const auto &mode = mode_tbl[xpr0Header.pixel_format];
	const uint32_t expected_size = (1U << area_shift) * mode.bpp / 8U;

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

	const int width  = 1 << (xpr0Header.width_pow2 >> 4);
	const int height = 1 << (xpr0Header.height_pow2 & 0x0F);
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
					static_cast<ImageDecoder::PixelFormat>(mode.pxf),
					width, height,
					buf.get(), expected_size);
				break;
			case 16:
				img = ImageDecoder::fromLinear16(
					static_cast<ImageDecoder::PixelFormat>(mode.pxf),
					width, height,
					reinterpret_cast<const uint16_t*>(buf.get()), expected_size);
				break;
			case 32:
				img = ImageDecoder::fromLinear32(
					static_cast<ImageDecoder::PixelFormat>(mode.pxf),
					width, height,
					reinterpret_cast<const uint32_t*>(buf.get()), expected_size);
				break;
			case 0:
			default:
				assert(!"Unsupported bpp value.");
				return nullptr;
		}
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
		rp_image *const imgunswz = new rp_image(width, height, rp_image::FORMAT_ARGB32);
		unswizzle_box(static_cast<const uint8_t*>(img->bits()),
			width, height,
			static_cast<uint8_t*>(imgunswz->bits()),
			img->stride(), sizeof(uint32_t));
		delete img;
		img = imgunswz;
		return imgunswz;
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
XboxXPR::XboxXPR(IRpFile *file)
	: super(new XboxXPRPrivate(this, file))
{
	// This class handles texture files.
	RP_D(XboxXPR);
	d->className = "XboxXPR";
	d->fileType = FTYPE_TEXTURE_FILE;

	if (!d->file) {
		// Could not ref() the file handle.
		return;
	}

	// Read the XPR0 header.
	d->file->rewind();
	size_t size = d->file->read(&d->xpr0Header, sizeof(d->xpr0Header));
	if (size != sizeof(d->xpr0Header)) {
		d->file->unref();
		d->file = nullptr;
		return;
	}

	// Check if this XPR image is supported.
	DetectInfo info;
	info.header.addr = 0;
	info.header.size = sizeof(d->xpr0Header);
	info.header.pData = reinterpret_cast<const uint8_t*>(&d->xpr0Header);
	info.ext = nullptr;	// Not needed for XPR.
	info.szFile = file->size();
	d->xprType = isRomSupported_static(&info);
	d->isValid = (d->xprType >= 0);

	if (!d->isValid) {
		d->file->unref();
		d->file = nullptr;
	}
}

/**
 * Is a ROM image supported by this class?
 * @param info DetectInfo containing ROM detection information.
 * @return Class-specific system ID (>= 0) if supported; -1 if not.
 */
int XboxXPR::isRomSupported_static(const DetectInfo *info)
{
	assert(info != nullptr);
	assert(info->header.pData != nullptr);
	assert(info->header.addr == 0);
	if (!info || !info->header.pData ||
	    info->header.addr != 0 ||
	    info->header.size < sizeof(Xbox_XPR0_Header))
	{
		// Either no detection information was specified,
		// or the header is too small.
		return -1;
	}

	// Verify the XPR0 magic.
	const Xbox_XPR0_Header *const xpr0Header =
		reinterpret_cast<const Xbox_XPR0_Header*>(info->header.pData);
	if (xpr0Header->magic == cpu_to_be32(XBOX_XPR0_MAGIC)) {
		// This is an XPR0 image.
		return XboxXPRPrivate::XPR_TYPE_XPR0;
	} else if (xpr0Header->magic == cpu_to_be32(XBOX_XPR1_MAGIC)) {
		// This is an XPR1 image.
		return XboxXPRPrivate::XPR_TYPE_XPR1;
	}

	// Not supported.
	return XboxXPRPrivate::XPR_TYPE_UNKNOWN;
}

/**
 * Get the name of the system the loaded ROM is designed for.
 * @param type System name type. (See the SystemName enum.)
 * @return System name, or nullptr if type is invalid.
 */
const char *XboxXPR::systemName(unsigned int type) const
{
	RP_D(const XboxXPR);
	if (!d->isValid || !isSystemNameTypeValid(type))
		return nullptr;

	// Microsoft Xbox has the same name worldwide, so we can
	// ignore the region selection.
	static_assert(SYSNAME_TYPE_MASK == 3,
		"XboxXPR::systemName() array index optimization needs to be updated.");

	// Bits 0-1: Type. (long, short, abbreviation)
	static const char *const sysNames[4] = {
		"Microsoft Xbox", "Xbox", "Xbox", nullptr
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
const char *const *XboxXPR::supportedFileExtensions_static(void)
{
	static const char *const exts[] = {
		".xbx", ".xpr",

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
const char *const *XboxXPR::supportedMimeTypes_static(void)
{
	static const char *const mimeTypes[] = {
		// Unofficial MIME types.
		// TODO: Get these upstreamed on FreeDesktop.org.
		// TODO: Add additional MIME types for XPR1/XPR2. (archive files)
		"image/x-xbox-xpr0",

		nullptr
	};
	return mimeTypes;
}

/**
 * Get a bitfield of image types this class can retrieve.
 * @return Bitfield of supported image types. (ImageTypesBF)
 */
uint32_t XboxXPR::supportedImageTypes_static(void)
{
	return IMGBF_INT_IMAGE;
}

/**
 * Get a list of all available image sizes for the specified image type.
 * @param imageType Image type.
 * @return Vector of available image sizes, or empty vector if no images are available.
 */
vector<RomData::ImageSizeDef> XboxXPR::supportedImageSizes(ImageType imageType) const
{
	ASSERT_supportedImageSizes(imageType);

	RP_D(const XboxXPR);
	if (!d->isValid || imageType != IMG_INT_IMAGE) {
		return vector<ImageSizeDef>();
	}

	// Return the image's size.
	const ImageSizeDef imgsz[] = {{nullptr,
		static_cast<uint16_t>(1U << (d->xpr0Header.width_pow2 >> 4)),
		static_cast<uint16_t>(1U << (d->xpr0Header.height_pow2 & 0x0F)),
		0
	}};
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
uint32_t XboxXPR::imgpf(ImageType imageType) const
{
	ASSERT_imgpf(imageType);

	RP_D(const XboxXPR);
	if (imageType != IMG_INT_IMAGE) {
		// Only IMG_INT_IMAGE is supported by PVR.
		return 0;
	}

	// If both dimensions of the texture are 64 or less,
	// specify nearest-neighbor scaling.
	uint32_t ret = 0;
	if ((d->xpr0Header.width_pow2 >> 4) <= 6 ||
	    (d->xpr0Header.height_pow2 & 0x0F) <= 6)
	{
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
int XboxXPR::loadFieldData(void)
{
	RP_D(XboxXPR);
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

	// XboxXPR header
	const Xbox_XPR0_Header *const xpr0Header = &d->xpr0Header;
	d->fields->reserve(3);	// Maximum of 2 fields.

	// Type
	static const char *const type_tbl[] = {
		"XPR0", "XPR1"
	};
	if (d->xprType > XboxXPRPrivate::XPR_TYPE_UNKNOWN &&
	    d->xprType < ARRAY_SIZE(type_tbl))
	{
		d->fields->addField_string(C_("XboxXPR", "Type"), type_tbl[d->xprType]);
	} else {
		d->fields->addField_string(C_("XboxXPR", "Type"),
			rp_sprintf(C_("RomData", "Unknown (%d)"), d->xprType));
	}

	// Pixel format
	static const char *const pxfmt_tbl[] = {
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
	};
	if (xpr0Header->pixel_format < ARRAY_SIZE(pxfmt_tbl)) {
		d->fields->addField_string(C_("XboxXPR", "Pixel Format"),
			pxfmt_tbl[xpr0Header->pixel_format]);
	} else {
		d->fields->addField_string(C_("XboxXPR", "Pixel Format"),
			rp_sprintf(C_("RomData", "Unknown (0x%02X)"), xpr0Header->pixel_format));
	}

	// Texture size
	d->fields->addField_dimensions(C_("XboxXPR", "Texture Size"),
		1 << (xpr0Header->width_pow2 >> 4),
		1 << (xpr0Header->height_pow2 & 0x0F));

	// TODO: More fields.

	// Finished reading the field data.
	return static_cast<int>(d->fields->count());
}

/**
 * Load metadata properties.
 * Called by RomData::metaData() if the field data hasn't been loaded yet.
 * @return Number of metadata properties read on success; negative POSIX error code on error.
 */
int XboxXPR::loadMetaData(void)
{
	RP_D(XboxXPR);
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

	// XboxXPR header.
	const Xbox_XPR0_Header *const xpr0Header = &d->xpr0Header;

	// Dimensions.
	d->metaData->addMetaData_integer(Property::Width, 1 << (xpr0Header->width_pow2 >> 4));
	d->metaData->addMetaData_integer(Property::Height, 1 << (xpr0Header->height_pow2 & 0x0F));

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
int XboxXPR::loadInternalImage(ImageType imageType, const rp_image **pImage)
{
	ASSERT_loadInternalImage(imageType, pImage);

	RP_D(XboxXPR);
	if (imageType != IMG_INT_IMAGE) {
		// Only IMG_INT_IMAGE is supported by PVR.
		*pImage = nullptr;
		return -ENOENT;
	} else if (!d->file) {
		// File isn't open.
		*pImage = nullptr;
		return -EBADF;
	} else if (!d->isValid) {
		// Unknown file type.
		*pImage = nullptr;
		return -EIO;
	}

	// Load the image.
	switch (d->xprType) {
		case XboxXPRPrivate::XPR_TYPE_XPR0:
			*pImage = d->loadXboxXPR0Image();
			break;
		case XboxXPRPrivate::XPR_TYPE_XPR1:
			// TODO
			*pImage = nullptr;
			return -EIO;
		default:
			// Unsupported.
			*pImage = nullptr;
			return -EIO;
	}

	return (*pImage != nullptr ? 0 : -EIO);
}

}
