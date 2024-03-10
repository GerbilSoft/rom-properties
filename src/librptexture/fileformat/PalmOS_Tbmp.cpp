/***************************************************************************
 * ROM Properties Page shell extension. (librptexture)                     *
 * PalmOS_Tbmp.cpp: Palm OS Tbmp texture reader.                           *
 *                                                                         *
 * Copyright (c) 2019-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "PalmOS_Tbmp.hpp"
#include "FileFormat_p.hpp"

#include "palmos_tbmp_structs.h"
#include "palmos_system_palette.h"

// Other rom-properties libraries
using namespace LibRpFile;
using LibRpBase::RomFields;

// librptexture
#include "img/rp_image.hpp"
#include "decoder/ImageDecoder_common.hpp"
#include "decoder/ImageDecoder_Linear.hpp"
#include "decoder/ImageDecoder_Linear_Gray.hpp"
#include "decoder/PixelConversion.hpp"
using LibRpTexture::ImageDecoder::PixelFormat;

// C++ STL classes
using std::unique_ptr;

namespace LibRpTexture {

// Workaround for RP_D() expecting the no-underscore naming convention.
#define PalmOS_TbmpPrivate PalmOS_Tbmp_Private

class PalmOS_Tbmp_Private final : public FileFormatPrivate
{
public:
	PalmOS_Tbmp_Private(PalmOS_Tbmp *q, const IRpFilePtr &file, off64_t bitmapTypeAddr = 0);
	~PalmOS_Tbmp_Private() final = default;

private:
	typedef FileFormatPrivate super;
	RP_DISABLE_COPY(PalmOS_Tbmp_Private)

public:
	/** TextureInfo **/
	static const char *const exts[];
	static const char *const mimeTypes[];
	static const TextureInfo textureInfo;

public:
	// BitmapType struct
	PalmOS_BitmapType_t bitmapType;
	// Starting address
	off64_t bitmapTypeAddr;

	// Decoded image
	rp_image_ptr img;

public:
	/**
	 * Decompress a scanline-compressed bitmap.
	 * @param compr_data		[in] Compressed bitmap data
	 * @param compr_data_len	[in] Length of compr_data
	 * @return Buffer containing the decompressed bitmap (rowBytes * height), or nullptr on error.
	 */
	uint8_t *decompress_scanline(const uint8_t *compr_data, size_t compr_data_len);

	/**
	 * Decompress an RLE-compressed bitmap.
	 * @param compr_data		[in] Compressed bitmap data
	 * @param compr_data_len	[in] Length of compr_data
	 * @return Buffer containing the decompressed bitmap (rowBytes * height), or nullptr on error.
	 */
	uint8_t *decompress_RLE(const uint8_t *compr_data, size_t compr_data_len);

	/**
	 * Decompress a PackBits-compressed bitmap. (8-bpp version)
	 * @param compr_data		[in] Compressed bitmap data
	 * @param compr_data_len	[in] Length of compr_data
	 * @return Buffer containing the decompressed bitmap (rowBytes * height), or nullptr on error.
	 */
	uint8_t *decompress_PackBits8(const uint8_t *compr_data, size_t compr_data_len);

	/**
	 * Load the PalmOS_Tbmp image.
	 * @return Image, or nullptr on error.
	 */
	rp_image_const_ptr loadTbmp(void);
};

FILEFORMAT_IMPL(PalmOS_Tbmp)

/** PalmOS_Tbmp_Private **/

/* TextureInfo */
const char *const PalmOS_Tbmp_Private::exts[] = {
	".tbmp",

	nullptr
};
const char *const PalmOS_Tbmp_Private::mimeTypes[] = {
	// Unofficial MIME types.
	// TODO: Get these upstreamed on FreeDesktop.org.
	// TODO: Add additional MIME types for XPR1/XPR2. (archive files)
	"image/x-palm-tbmp",

	nullptr
};
const TextureInfo PalmOS_Tbmp_Private::textureInfo = {
	exts, mimeTypes
};

PalmOS_Tbmp_Private::PalmOS_Tbmp_Private(PalmOS_Tbmp *q, const IRpFilePtr &file, off64_t bitmapTypeAddr)
	: super(q, file, &textureInfo)
	, bitmapTypeAddr(bitmapTypeAddr)
{
	// Clear the structs and arrays.
	memset(&bitmapType, 0, sizeof(bitmapType));
}

/**
 * Decompress a scanline-compressed bitmap.
 * @param compr_data		[in] Compressed bitmap data
 * @param compr_data_len	[in] Length of compr_data
 * @return Buffer containing the decompressed bitmap (rowBytes * height), or nullptr on error.
 */
uint8_t *PalmOS_Tbmp_Private::decompress_scanline(const uint8_t *compr_data, size_t compr_data_len)
{
	const uint8_t *const compr_data_end = &compr_data[compr_data_len];

	const int height = be16_to_cpu(bitmapType.height);
	const unsigned int rowBytes = be16_to_cpu(bitmapType.rowBytes);
	const size_t icon_data_len = (size_t)rowBytes * (size_t)height;

	unique_ptr<uint8_t[]> decomp_buf(new uint8_t[icon_data_len]);
	uint8_t *dest = decomp_buf.get();
	const uint8_t *lastrow = dest;
	for (int y = 0; y < height; y++) {
		for (unsigned int x = 0; x < rowBytes; x += 8) {
			// First byte is a diffmask indicating which bytes in
			// an 8-byte group are the same as the previous row.
			// NOTE: Assumed to be 0 for the first row.
			assert(compr_data < compr_data_end);
			if (compr_data >= compr_data_end)
				return nullptr;

			uint8_t diffmask = *compr_data++;
			if (y == 0)
				diffmask = 0xFF;

			// Process 8 bytes.
			unsigned int bytecount = std::min(rowBytes - x, 8U);
			for (unsigned int b = 0; b < bytecount; b++, diffmask <<= 1) {
				uint8_t px;
				if (!(diffmask & (1U << 7))) {
					// Read a byte from the last row.
					px = lastrow[x + b];
				} else {
					// Read a byte from the source data.
					assert(compr_data < compr_data_end);
					if (compr_data >= compr_data_end)
						return nullptr;
					px = *compr_data++;
				}
				*dest++ = px;
			}
		}

		if (y > 0)
			lastrow += rowBytes;
	}

	// Bitmap has been decompressed.
	return decomp_buf.release();
}

/**
 * Decompress an RLE-compressed bitmap.
 * @param compr_data		[in] Compressed bitmap data
 * @param compr_data_len	[in] Length of compr_data
 * @return Buffer containing the decompressed bitmap (rowBytes * height), or nullptr on error.
 */
uint8_t *PalmOS_Tbmp_Private::decompress_RLE(const uint8_t *compr_data, size_t compr_data_len)
{
	const uint8_t *const compr_data_end = &compr_data[compr_data_len];

	const int height = dimensions[1];
	const unsigned int rowBytes = be16_to_cpu(bitmapType.rowBytes);
	const size_t icon_data_len = (size_t)rowBytes * (size_t)height;

	unique_ptr<uint8_t[]> decomp_buf(new uint8_t[icon_data_len]);
	const uint8_t *const dest_end = &decomp_buf[icon_data_len];
	uint8_t *dest = decomp_buf.get();
	for (int y = 0; y < height; y++) {
		for (unsigned int x = 0; x < rowBytes; ) {
			assert(compr_data < compr_data_end);
			if (compr_data >= compr_data_end)
				return nullptr;

			// Read the RLE count byte.
			uint8_t b_count = *compr_data++;
			assert(b_count != 0);
			if (b_count == 0) {
				// Invalid: RLE count cannot be 0.
				return nullptr;
			}
			assert(b_count + x <= rowBytes);
			if (b_count + x > rowBytes) {
				// Invalid: RLE exceeds the scanline boundary.
				return nullptr;
			}

			// Read the RLE data byte.
			assert(compr_data < compr_data_end);
			if (compr_data >= compr_data_end)
				return nullptr;
			uint8_t b_data = *compr_data++;

			// Write the decompressed data bytes.
			assert(dest + b_count <= dest_end);
			if (dest + b_count > dest_end) {
				// Invalid: Decompressed data goes out of bounds.
				return nullptr;
			}
			memset(dest, b_data, b_count);
			dest += b_count;
			x += b_count;
		}
	}

	// Sanity check: We should be at the end of the bitmap.
	assert(dest == dest_end);
	if (dest != dest_end)
		return nullptr;

	// Bitmap has been decompressed.
	return decomp_buf.release();
}

/**
 * Decompress a PackBits-compressed bitmap. (8-bpp version)
 * @param compr_data		[in] Compressed bitmap data
 * @param compr_data_len	[in] Length of compr_data
 * @return Buffer containing the decompressed bitmap (rowBytes * height), or nullptr on error.
 */
uint8_t *PalmOS_Tbmp_Private::decompress_PackBits8(const uint8_t *compr_data, size_t compr_data_len)
{
	// Reference: https://en.wikipedia.org/wiki/PackBits
	const uint8_t *const compr_data_end = &compr_data[compr_data_len];

	const int height = dimensions[1];
	const unsigned int rowBytes = be16_to_cpu(bitmapType.rowBytes);
	const size_t icon_data_len = (size_t)rowBytes * (size_t)height;

	unique_ptr<uint8_t[]> decomp_buf(new uint8_t[icon_data_len]);
	uint8_t *dest = decomp_buf.get();
	for (int y = 0; y < height; y++) {
		for (unsigned int x = 0; x < rowBytes; ) {
			// First byte is a signed control byte.
			assert(compr_data < compr_data_end);
			if (compr_data >= compr_data_end)
				return nullptr;
			const int8_t cbyte = static_cast<int8_t>(*compr_data++);

			if (cbyte == -128) {
				// No operation. Skip this byte.
				continue;
			} else if (cbyte < 0) {
				// One byte, repeated (1 - n) times.
				// NOTE: Limited to the remaining bytes in the current row.
				// TODO: Assert if too many bytes?
				int reps = 1 - static_cast<int>(cbyte);
				if (x + static_cast<unsigned int>(reps) >= rowBytes) {
					reps = static_cast<int>(rowBytes - x);
				}

				assert(compr_data < compr_data_end);
				if (compr_data >= compr_data_end)
					return nullptr;
				const uint8_t data = *compr_data++;
				memset(dest, data, reps);
				dest += reps;
				x += reps;
			} else /*if (cbyte > 0)*/ {
				// (1 + n) bytes of data to copy.
				unsigned int reps = 1 + static_cast<unsigned int>(cbyte);

				assert(compr_data + reps <= compr_data_end);
				if (compr_data + reps > compr_data_end)
					return nullptr;

				// NOTE: Limited to the remaining bytes in the current row.
				// TODO: Assert if too many bytes?
				const unsigned int to_copy = std::min(reps, rowBytes - x);
				memcpy(dest, compr_data, to_copy);
				compr_data += reps;
				dest += to_copy;
				x += to_copy;
			}
		}
	}

	// Bitmap has been decompressed.
	return decomp_buf.release();
}

/**
 * Load the Palm OS Tbmp bitmap image.
 * @return Image, or nullptr on error.
 */
rp_image_const_ptr PalmOS_Tbmp_Private::loadTbmp(void)
{
	const uint8_t version = bitmapType.version;

	static const uint8_t header_size_tbl[] = {
		PalmOS_BitmapType_v0_SIZE,
		PalmOS_BitmapType_v1_SIZE,
		PalmOS_BitmapType_v2_SIZE,
		PalmOS_BitmapType_v3_SIZE,
	};
	assert(version < ARRAY_SIZE(header_size_tbl));
	if (version >= ARRAY_SIZE(header_size_tbl)) {
		// Version is not supported...
		return {};
	}
	off64_t addr = bitmapTypeAddr + header_size_tbl[version];

	// Decode the icon.
	const int width = dimensions[0];
	const int height = dimensions[1];
	assert(width > 0);
	assert(width <= 256);
	assert(height > 0);
	assert(height <= 256);
	if (width <= 0 || width > 256 ||
	    height <= 0 || height > 256)
	{
		// Icon size is probably out of range.
		return {};
	}
	const unsigned int rowBytes = be16_to_cpu(bitmapType.rowBytes);
	size_t icon_data_len = (size_t)rowBytes * (size_t)height;
	const uint16_t flags = be16_to_cpu(bitmapType.flags);

	PalmOS_BitmapDirectInfoType_t bitmapDirectInfoType;
	if (flags & PalmOS_BitmapType_Flags_directColor) {
		// Direct Color flag is set. Must be v2 or v3, and pixelSize must be 16.
		assert(version >= 2);
		assert(bitmapType.pixelSize == 16);
		if (version < 2 || bitmapType.pixelSize != 16)
			return {};

		if (version == 2) {
			// Read the BitmapDirectInfoType field.
			size_t size = file->seekAndRead(addr, &bitmapDirectInfoType, sizeof(bitmapDirectInfoType));
			if (size != sizeof(bitmapDirectInfoType)) {
				// Seek and/or read error.
				return {};
			}
			addr += sizeof(bitmapDirectInfoType);
		}
	}

	uint8_t compr_type;
	uint32_t compr_data_len;
	if (version >= 2 && (flags & PalmOS_BitmapType_Flags_compressed)) {
		// Bitmap data is compressed. Read the compressed size field.
		uint8_t cbuf[4];
		size_t size = file->seekAndRead(addr, cbuf, sizeof(cbuf));
		if (size != sizeof(cbuf)) {
			// Seek and/or read error.
			return {};
		}

		compr_type = bitmapType.v2.compressionType;
		if (version >= 3) {
			// v3: 32-bit size
			compr_data_len = (cbuf[0] << 24) | (cbuf[1] << 16) | (cbuf[2] << 8) | cbuf[3];
			addr += sizeof(uint32_t);
		} else {
			// v2: 16-bit size
			compr_data_len = (cbuf[0] << 8) | cbuf[1];
			addr += sizeof(uint16_t);
		}
	} else {
		// Not compressed.
		compr_type = PalmOS_BitmapType_CompressionType_None;
		compr_data_len = icon_data_len;
	}

	// Sanity check: compr_data_len should *always* be <= icon_data_len.
	assert(compr_data_len <= icon_data_len);
	if (compr_data_len > icon_data_len)
		return {};

	// NOTE: Allocating enough memory for the uncompressed bitmap,
	// but only reading enough data for the compressed bitmap.
	// (If the bitmap is not compressed, the sizes are the same.)
	unique_ptr<uint8_t[]> icon_data(new uint8_t[icon_data_len]);
	size_t size = file->seekAndRead(addr, icon_data.get(), compr_data_len);
	if (size != compr_data_len) {
		// Seek and/or read error.
		return {};
	}

	rp_image_ptr img;
	switch (bitmapType.pixelSize) {
		default:
			assert(!"Pixel size is not supported yet!");
			break;

		case 0:	// NOTE: for v0 only
		case 1:
			// 1-bpp monochrome
			img = ImageDecoder::fromLinearMono(width, height, icon_data.get(), icon_data_len, static_cast<int>(rowBytes));
			break;

		case 2:
			// 2-bpp grayscale
			// TODO: Use $00/$88/$CC/$FF palette instead of $00/$80/$C0/$FF?
			img = ImageDecoder::fromLinearGray2bpp(width, height, icon_data.get(), icon_data_len, static_cast<int>(rowBytes));
			break;

		case 4: {
			// 4-bpp grayscale
			// NOTE: Using a function intended for 16-color images,
			// so we'll have to provide our own palette.
			uint32_t palette[16];
			uint32_t gray = 0xFFFFFFFFU;
			for (unsigned int i = 0; i < ARRAY_SIZE(palette); i++, gray -= 0x111111U) {
				palette[i] = gray;
			}

			img = ImageDecoder::fromLinearCI4(PixelFormat::Host_ARGB32, true,
					width, height,
					icon_data.get(), icon_data_len,
					palette, sizeof(palette), rowBytes);
			if (img) {
				// Set the sBIT metadata.
				// NOTE: Setting the grayscale value, though we're
				// not saving grayscale PNGs at the moment.
				static const rp_image::sBIT_t sBIT = {4,4,4,4,0};
				img->set_sBIT(&sBIT);
			}
			break;
		}

		case 8: {
			// 8-bpp indexed (palette)
			// NOTE: Must be v2 or higher.
			// NOTE 2: SpaceWarColor v2.1 and later has an 8-bpp icon bitmap
			// marked as v1. We'll allow that for now...
			assert(version >= 1);
			if (version < 1)
				break;

			// TODO: Handle various flags.
			if (flags & (PalmOS_BitmapType_Flags_hasColorTable |
			             PalmOS_BitmapType_Flags_directColor |
			             PalmOS_BitmapType_Flags_indirectColorTable))
			{
				// Flag is not supported.
				break;
			}

			// Decompress certain types of images.
			switch (compr_type) {
				default:
					// Not supported...
					assert(!"Unsupported bitmap compression type.");
					return {};

				case PalmOS_BitmapType_CompressionType_None:
					// Not actually compressed...
					break;

				case PalmOS_BitmapType_CompressionType_ScanLine: {
					// Scanline compression
					icon_data.reset(decompress_scanline(icon_data.get(), compr_data_len));
					if (!icon_data) {
						// Decompression failed.
						return {};
					}
					break;
				}

				case PalmOS_BitmapType_CompressionType_PackBits: {
					// PackBits compression
					icon_data.reset(decompress_PackBits8(icon_data.get(), compr_data_len));
					if (!icon_data) {
						// Decompression failed.
						return {};
					}
					break;
				}

				case PalmOS_BitmapType_CompressionType_RLE: {
					// RLE compression
					icon_data.reset(decompress_RLE(icon_data.get(), compr_data_len));
					if (!icon_data) {
						// Decompression failed.
						return {};
					}
					break;
				}
			}

			img = ImageDecoder::fromLinearCI8(PixelFormat::Host_ARGB32,
					width, height,
					icon_data.get(), icon_data_len,
					PalmOS_system_palette, sizeof(PalmOS_system_palette), rowBytes);
			if (img) {
				bool did_tRNS = false;
				if (flags & PalmOS_BitmapType_Flags_hasTransparency) {
					// Get the transparent palette index.
					const uint8_t tr_idx = (version <= 2)
						? bitmapType.v2.transparentIndex
						: static_cast<uint8_t>(be32_to_cpu(bitmapType.v3.transparentValue));

					// Set the transparent index and adjust the palette.
					img->set_tr_idx(tr_idx);
					assert(tr_idx < img->palette_len());
					if (tr_idx < img->palette_len()) {
						uint32_t *const palette = img->palette();
						palette[tr_idx] = 0x00000000;
						did_tRNS = true;
					}
				}

				if (!did_tRNS) {
					// Remove the alpha channel from the sBIT metadata.
					static const rp_image::sBIT_t sBIT = {8,8,8,0,0};
					img->set_sBIT(&sBIT);
				}
			}
			break;
		}

		case 16: {
			// 16-bpp (RGB565)
			// NOTE: Must be v2 or higher.
			assert(version >= 2);
			if (version < 2)
				break;

			// TODO: Handle various flags.
			if (flags & (PalmOS_BitmapType_Flags_hasColorTable |
			             PalmOS_BitmapType_Flags_indirect |
			             /*PalmOS_BitmapType_Flags_directColor |*/
			             PalmOS_BitmapType_Flags_indirectColorTable))
			{
				// Flag is not supported.
				break;
			}

			// TODO: Validate the BitmapDirectInfoType field.

			// Decompress certain types of images.
			switch (compr_type) {
				default:
					// Not supported...
					assert(!"Unsupported bitmap compression type.");
					return {};

				case PalmOS_BitmapType_CompressionType_None:
					// Not actually compressed...
					break;

				case PalmOS_BitmapType_CompressionType_ScanLine: {
					// Scanline compression
					// NOTE: No changes for 16-bpp compared to 8-bpp.
					icon_data.reset(decompress_scanline(icon_data.get(), compr_data_len));
					if (!icon_data) {
						// Decompression failed.
						return {};
					}
					break;
				}
			}

			// v2: Image is encoded using RGB565 BE.
			// v3: Check pixelFormat.
			const uint8_t pixelFormat = (version == 3)
				? bitmapType.v3.pixelFormat
				: static_cast<uint8_t>(PalmOS_BitmapType_PixelFormat_RGB565_BE);
			switch (pixelFormat) {
				default:
				case PalmOS_BitmapType_PixelFormat_Indexed:
				case PalmOS_BitmapType_PixelFormat_Indexed_LE:
					// Not supported...
					assert(!"pixelFormat not supported!");
					break;

				case PalmOS_BitmapType_PixelFormat_RGB565_BE:
					// RGB565, big-endian (standard for v2; default for v3)
#if SYS_BYTEORDER != SYS_BIG_ENDIAN
					// Byteswap the data.
					assert(icon_data_len % 2 == 0);
					rp_byte_swap_16_array(reinterpret_cast<uint16_t*>(icon_data.get()), icon_data_len);
#endif /* SYS_BYTEORDER != SYS_BIG_ENDIAN */
					break;

				case PalmOS_BitmapType_PixelFormat_RGB565_LE:
					// RGB565, big-endian (standard for v2; default for v3)
#if SYS_BYTEORDER != SYS_LIL_ENDIAN
					// Byteswap the data.
					assert(icon_data_len % 2 == 0);
					rp_byte_swap_16_array(reinterpret_cast<uint16_t*>(icon_data.get()), icon_data_len);
#endif /* SYS_BYTEORDER != SYS_LIL_ENDIAN */
					break;
			}

			img = ImageDecoder::fromLinear16(PixelFormat::RGB565,
					width, height,
					reinterpret_cast<const uint16_t*>(icon_data.get()), icon_data_len);

			if (img && (flags & PalmOS_BitmapType_Flags_hasTransparency)) {
				// Apply transparency.
				argb32_t key;
				switch (version) {
					default:
						assert(!"Incorrect version for transparency?");
						break;
					case 2: {
						// v2 uses a transparency color in the BitmapDirectInfoType field.
						// Need to mask and extend the bits.
						PalmOS_RGBColorType_t tc = bitmapDirectInfoType.transparentcolor;
						key.a = 0xFF;
						key.r = (tc.r & 0xF8) | (tc.r >> 5);
						key.g = (tc.g & 0xFC) | (tc.g >> 6);
						key.b = (tc.b & 0xF8) | (tc.b >> 5);
						break;
					}
					case 3:
						// v3 stores a 16-bit RGB565 value in the transparentValue field.
						// TODO: Is this always RGB565 BE, or can it be RGB565 LE?
						key.u32 = PixelConversion::RGB565_to_ARGB32(be32_to_cpu(bitmapType.v3.transparentValue));
						break;
				}
				img->apply_chroma_key(key.u32);
			}
			break;
		}
	}

	return img;
}

/** PalmOS_Tbmp **/

/**
 * Read a Palm OS Tbmp image file.
 *
 * A ROM image must be opened by the caller. The file handle
 * will be ref()'d and must be kept open in order to load
 * data from the ROM image.
 *
 * To close the file, either delete this object or call close().
 *
 * NOTE: Check isValid() to determine if this is a valid ROM.
 *
 * @param file Open image file.
 */
PalmOS_Tbmp::PalmOS_Tbmp(const IRpFilePtr &file)
	: super(new PalmOS_Tbmp_Private(this, file, 0))
{
	init();
}

/**
 * Read a Palm OS Tbmp image file.
 *
 * A ROM image must be opened by the caller. The file handle
 * will be ref()'d and must be kept open in order to load
 * data from the ROM image.
 *
 * To close the file, either delete this object or call close().
 *
 * NOTE: Check isValid() to determine if this is a valid ROM.
 *
 * @param file Open file.
 * @param bitmapTypeAddr Starting address of the BitmapType header in the file.
 */
PalmOS_Tbmp::PalmOS_Tbmp(const IRpFilePtr &file, off64_t bitmapTypeAddr)
	: super(new PalmOS_Tbmp_Private(this, file, bitmapTypeAddr))
{
	init();
}

/**
 * Internal initialization function.
 */
void PalmOS_Tbmp::init(void)
{
	RP_D(PalmOS_Tbmp);
	d->mimeType = "image/x-palm-tbmp";	// unofficial, not on fd.o
	d->textureFormatName = "Palm OS Tbmp";

	if (!d->file) {
		// Could not ref() the file handle.
		return;
	}

	// Read the BitmapType header.
	size_t size = d->file->seekAndRead(d->bitmapTypeAddr, &d->bitmapType, sizeof(d->bitmapType));
	if (size != sizeof(d->bitmapType)) {
		d->file.reset();
		return;
	}

	// Check for v1 with pixelSize == 255.
	// If found, this is an extra 16-byte header located before some bitmaps.
	if (d->bitmapType.version == 1 && d->bitmapType.pixelSize == 255) {
		// Skip the 16 bytes.
		d->bitmapTypeAddr += 16;
		size = d->file->seekAndRead(d->bitmapTypeAddr, &d->bitmapType, sizeof(d->bitmapType));
		if (size != sizeof(d->bitmapType)) {
			d->file.reset();
			return;
		}
	}

	// TODO: Verify that this is in fact a Palm OS Tbmp image?
	d->isValid = true;

	// Cache the texture dimensions.
	d->dimensions[0] = be16_to_cpu(d->bitmapType.width);
	d->dimensions[1] = be16_to_cpu(d->bitmapType.height);
}

/** Property accessors **/

/**
 * Get the pixel format, e.g. "RGB888" or "DXT1".
 * @return Pixel format, or nullptr if unavailable.
 */
const char *PalmOS_Tbmp::pixelFormat(void) const
{
	RP_D(const PalmOS_Tbmp);
	if (!d->isValid) {
		// Not supported.
		return nullptr;
	}

	const char *px_fmt;
	switch (d->bitmapType.pixelSize) {
		default:
			assert(!"Unsupported pixel size.");
			px_fmt = nullptr;
			break;
		case 1:
			px_fmt = "1-bpp mono";
			break;
		case 2:
			px_fmt = "2-bpp grayscale";
			break;
		case 4:
			px_fmt = "4-bpp grayscale";
			break;
		case 8:
			// TODO: Transparency; compression formats
			px_fmt = "CI8";
			break;

		case 16: {
			// TODO: Transparency; compression formats; RGB565 BE or LE
			const uint8_t pixelFormat = (d->bitmapType.version == 3)
				? d->bitmapType.v3.pixelFormat
				: static_cast<uint8_t>(PalmOS_BitmapType_PixelFormat_RGB565_BE);
			switch (pixelFormat) {
				default:
				case PalmOS_BitmapType_PixelFormat_Indexed:
				case PalmOS_BitmapType_PixelFormat_Indexed_LE:
					// Not supported...
					assert(!"pixelFormat not supported!");
					px_fmt = nullptr;
					break;

				case PalmOS_BitmapType_PixelFormat_RGB565_BE:
					// RGB565, big-endian (standard for v2; default for v3)
					px_fmt = "RGB565 (big-endian)";
					break;

				case PalmOS_BitmapType_PixelFormat_RGB565_LE:
					// RGB565, big-endian (standard for v2; default for v3)
					px_fmt = "RGB565 (little-endian)";
					break;
			}
			px_fmt = "RGB565 (big-endian)";
			break;
		}
	}

	return px_fmt;
}

#ifdef ENABLE_LIBRPBASE_ROMFIELDS
/**
 * Get property fields for rom-properties.
 * @param fields RomFields object to which fields should be added.
 * @return Number of fields added, or 0 on error.
 */
int PalmOS_Tbmp::getFields(RomFields *fields) const
{
	assert(fields != nullptr);
	if (!fields)
		return 0;

	// NOTE: No fields right now...
	return 0;
}
#endif /* ENABLE_LIBRPBASE_ROMFIELDS */

/** Image accessors **/

/**
 * Get the image.
 * For textures with mipmaps, this is the largest mipmap.
 * The image is owned by this object.
 * @return Image, or nullptr on error.
 */
rp_image_const_ptr PalmOS_Tbmp::image(void) const
{
	RP_D(const PalmOS_Tbmp);
	if (!d->isValid) {
		// Unknown file type.
		return nullptr;
	}

	// Load the image.
	return const_cast<PalmOS_Tbmp_Private*>(d)->loadTbmp();
}

}
