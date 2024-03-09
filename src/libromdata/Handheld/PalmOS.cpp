/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * PalmOS.cpp: Palm OS application reader.                                 *
 *                                                                         *
 * Copyright (c) 2016-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

// References:
// - https://en.wikipedia.org/wiki/PRC_(Palm_OS)
// - https://web.mit.edu/pilot/pilot-docs/V1.0/cookbook.pdf
// - https://web.mit.edu/Tytso/www/pilot/prc-format.html
// - https://stuff.mit.edu/afs/sipb/user/yonah/docs/Palm%20OS%20Companion.pdf
// - https://stuff.mit.edu/afs/sipb/user/yonah/docs/Palm%20OS%20Reference.pdf
// - https://www.cs.trinity.edu/~jhowland/class.files.cs3194.html/palm-docs/Constructor%20for%20Palm%20OS.pdf
// - https://www.cs.uml.edu/~fredm/courses/91.308-spr05/files/palmdocs/uiguidelines.pdf

#include "stdafx.h"
#include "PalmOS.hpp"
#include "palmos_structs.h"
#include "palmos_system_palette.h"

// Other rom-properties libraries
#include "librptext/fourCC.hpp"
#include "librptexture/decoder/ImageDecoder_common.hpp"
#include "librptexture/decoder/ImageDecoder_Linear.hpp"
#include "librptexture/decoder/ImageDecoder_Linear_Gray.hpp"
#include "librptexture/decoder/PixelConversion.hpp"
using namespace LibRpBase;
using namespace LibRpFile;
using namespace LibRpText;
using namespace LibRpTexture;
using LibRpTexture::ImageDecoder::PixelFormat;

// C++ STL classes
using std::map;
using std::string;
using std::unique_ptr;
using std::vector;

// Uninitialized vector class
#include "uvector.h"

namespace LibRomData {

class PalmOSPrivate final : public RomDataPrivate
{
public:
	PalmOSPrivate(const IRpFilePtr &file);
	~PalmOSPrivate() final = default;

private:
	typedef RomDataPrivate super;
	RP_DISABLE_COPY(PalmOSPrivate)

public:
	/** RomDataInfo **/
	static const char *const exts[];
	static const char *const mimeTypes[];
	static const RomDataInfo romDataInfo;

public:
	// Application header
	PalmOS_PRC_Header_t prcHeader;

	// Resource headers
	// NOTE: Kept in big-endian format.
	rp::uvector<PalmOS_PRC_ResHeader_t> resHeaders;

	// Icon
	rp_image_ptr img_icon;

	/**
	 * Find a resource header.
	 * @param type Resource type
	 * @param id ID, or 0 for "first available"
	 * @return Pointer to resource header, or nullptr if not found.
	 */
	const PalmOS_PRC_ResHeader_t *findResHeader(uint32_t type, uint16_t id = 0) const;

	/**
	 * Decompress a scanline-compressed bitmap.
	 * @param bitmapType		[in] PalmOS_BitmapType_t
	 * @param compr_data		[in] Compressed bitmap data
	 * @param compr_data_len	[in] Length of compr_data
	 * @return Buffer containing the decompressed bitmap (rowBytes * height), or nullptr on error.
	 */
	uint8_t *decompress_scanline(const PalmOS_BitmapType_t *bitmapType, const uint8_t *compr_data, size_t compr_data_len);

	/**
	 * Decompress an RLE-compressed bitmap.
	 * @param bitmapType		[in] PalmOS_BitmapType_t
	 * @param compr_data		[in] Compressed bitmap data
	 * @param compr_data_len	[in] Length of compr_data
	 * @return Buffer containing the decompressed bitmap (rowBytes * height), or nullptr on error.
	 */
	uint8_t *decompress_RLE(const PalmOS_BitmapType_t *bitmapType, const uint8_t *compr_data, size_t compr_data_len);

	/**
	 * Decompress a PackBits-compressed bitmap. (8-bpp version)
	 * @param bitmapType		[in] PalmOS_BitmapType_t
	 * @param compr_data		[in] Compressed bitmap data
	 * @param compr_data_len	[in] Length of compr_data
	 * @return Buffer containing the decompressed bitmap (rowBytes * height), or nullptr on error.
	 */
	uint8_t *decompress_PackBits8(const PalmOS_BitmapType_t *bitmapType, const uint8_t *compr_data, size_t compr_data_len);

	/**
	 * Load the specified bitmap from a 'tAIB' resource.
	 * @param bitmapType BitmapType struct
	 * @param addr Address of the BitmapType struct
	 * @return Image, or nullptr on error.
	 */
	rp_image_ptr loadBitmap_tAIB(const PalmOS_BitmapType_t *bitmapType, uint32_t addr);

	/**
	 * Load the icon.
	 * @return Icon, or nullptr on error.
	 */
	rp_image_const_ptr loadIcon(void);

	/**
	 * Get a string resource. (max 255 bytes + NULL)
	 * @param type		[in] Resource type
	 * @param id		[in] Resource ID
	 * @return String resource, or empty string if not found.
	 */
	string load_string(uint32_t type, uint16_t id);
};

ROMDATA_IMPL(PalmOS)

/** PalmOSPrivate **/

/* RomDataInfo */
const char *const PalmOSPrivate::exts[] = {
	".prc",

	nullptr
};
const char *const PalmOSPrivate::mimeTypes[] = {
	// Vendor-specific MIME types from FreeDesktop.org.
	"application/vnd.palm",

	// Unofficial MIME types from FreeDesktop.org.
	// TODO: Get these upstreamed on FreeDesktop.org.
	"application/x-palm-database",
	"application/x-mobipocket-ebook",	// May show up on some systems, so reference it here.

	nullptr
};
const RomDataInfo PalmOSPrivate::romDataInfo = {
	"PalmOS", exts, mimeTypes
};

PalmOSPrivate::PalmOSPrivate(const IRpFilePtr &file)
	: super(file, &romDataInfo)
{
	// Clear the PRC header struct.
	memset(&prcHeader, 0, sizeof(prcHeader));
}

/**
 * Find a resource header.
 * @param type Resource type
 * @param id ID, or 0 for "first available"
 * @return Pointer to resource header, or nullptr if not found.
 */
const PalmOS_PRC_ResHeader_t *PalmOSPrivate::findResHeader(uint32_t type, uint16_t id) const
{
	if (resHeaders.empty()) {
		// No resource headers...
		return nullptr;
	}

#if SYS_BYTEORDER != SYS_BIG_ENDIAN
	// Convert type and ID to big-endian for faster parsing.
	type = cpu_to_be32(type);
	id = cpu_to_be16(id);
#endif /* SYS_BYTEORDER != SYS_BIG_ENDIAN */

	// Find the specified resource header.
	for (const auto &hdr : resHeaders) {
		if (hdr.type == type) {
			if (id == 0 || hdr.id == id) {
				// Found a matching resource.
				return &hdr;
			}
		}
	}

	// Resource not found.
	return nullptr;
}

/**
 * Decompress a scanline-compressed bitmap.
 * @param bitmapType		[in] PalmOS_BitmapType_t
 * @param compr_data		[in] Compressed bitmap data
 * @param compr_data_len	[in] Length of compr_data
 * @return Buffer containing the decompressed bitmap (rowBytes * height), or nullptr on error.
 */
uint8_t *PalmOSPrivate::decompress_scanline(const PalmOS_BitmapType_t *bitmapType, const uint8_t *compr_data, size_t compr_data_len)
{
	const uint8_t *const compr_data_end = &compr_data[compr_data_len];

	const int height = be16_to_cpu(bitmapType->height);
	const unsigned int rowBytes = be16_to_cpu(bitmapType->rowBytes);
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
 * @param bitmapType		[in] PalmOS_BitmapType_t
 * @param compr_data		[in] Compressed bitmap data
 * @param compr_data_len	[in] Length of compr_data
 * @return Buffer containing the decompressed bitmap (rowBytes * height), or nullptr on error.
 */
uint8_t *PalmOSPrivate::decompress_RLE(const PalmOS_BitmapType_t *bitmapType, const uint8_t *compr_data, size_t compr_data_len)
{
	const uint8_t *const compr_data_end = &compr_data[compr_data_len];

	const int height = be16_to_cpu(bitmapType->height);
	const unsigned int rowBytes = be16_to_cpu(bitmapType->rowBytes);
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
 * @param bitmapType		[in] PalmOS_BitmapType_t
 * @param compr_data		[in] Compressed bitmap data
 * @param compr_data_len	[in] Length of compr_data
 * @return Buffer containing the decompressed bitmap (rowBytes * height), or nullptr on error.
 */
uint8_t *PalmOSPrivate::decompress_PackBits8(const PalmOS_BitmapType_t *bitmapType, const uint8_t *compr_data, size_t compr_data_len)
{
	// Reference: https://en.wikipedia.org/wiki/PackBits
	const uint8_t *const compr_data_end = &compr_data[compr_data_len];

	const int height = be16_to_cpu(bitmapType->height);
	const unsigned int rowBytes = be16_to_cpu(bitmapType->rowBytes);
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
 * Load the specified bitmap from a 'tAIB' resource.
 * @param bitmapType BitmapType struct
 * @param addr Address of the BitmapType struct
 * @return Image, or nullptr on error.
 */
rp_image_ptr PalmOSPrivate::loadBitmap_tAIB(const PalmOS_BitmapType_t *bitmapType, uint32_t addr)
{
	const uint8_t version = bitmapType->version;

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
	addr += header_size_tbl[version];

	// Decode the icon.
	const int width = be16_to_cpu(bitmapType->width);
	const int height = be16_to_cpu(bitmapType->height);
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
	const unsigned int rowBytes = be16_to_cpu(bitmapType->rowBytes);
	size_t icon_data_len = (size_t)rowBytes * (size_t)height;
	const uint16_t flags = be16_to_cpu(bitmapType->flags);

	PalmOS_BitmapDirectInfoType_t bitmapDirectInfoType;
	if (flags & PalmOS_BitmapType_Flags_directColor) {
		// Direct Color flag is set. Must be v2 or v3, and pixelSize must be 16.
		assert(version >= 2);
		assert(bitmapType->pixelSize == 16);
		if (version < 2 || bitmapType->pixelSize != 16)
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

		compr_type = bitmapType->v2.compressionType;
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
	switch (bitmapType->pixelSize) {
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
					icon_data.reset(decompress_scanline(bitmapType, icon_data.get(), compr_data_len));
					if (!icon_data) {
						// Decompression failed.
						return {};
					}
					break;
				}

				case PalmOS_BitmapType_CompressionType_PackBits: {
					// PackBits compression
					icon_data.reset(decompress_PackBits8(bitmapType, icon_data.get(), compr_data_len));
					if (!icon_data) {
						// Decompression failed.
						return {};
					}
					break;
				}

				case PalmOS_BitmapType_CompressionType_RLE: {
					// RLE compression
					icon_data.reset(decompress_RLE(bitmapType, icon_data.get(), compr_data_len));
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
						? bitmapType->v2.transparentIndex
						: static_cast<uint8_t>(be32_to_cpu(bitmapType->v3.transparentValue));

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
					icon_data.reset(decompress_scanline(bitmapType, icon_data.get(), compr_data_len));
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
				? bitmapType->v3.pixelFormat
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
						key.u32 = PixelConversion::RGB565_to_ARGB32(be32_to_cpu(bitmapType->v3.transparentValue));
						break;
				}
				img->apply_chroma_key(key.u32);
			}
			break;
		}
	}

	return img;
}

/**
 * Load the icon.
 * @return Icon, or nullptr on error.
 */
rp_image_const_ptr PalmOSPrivate::loadIcon(void)
{
	if (img_icon) {
		// Icon has already been loaded.
		return img_icon;
	} else if (!this->isValid || !this->file) {
		// Can't load the icon.
		return {};
	}

	// TODO: Make this a general icon loading function?

	// Find the application icon resource.
	// - Type: 'tAIB'
	// - Large icon: 1000 (usually 22x22; may be up to 32x32)
	// - Small icon: 1001 (15x9)
	// TODO: Allow user selection. For now, large icon only.
	const PalmOS_PRC_ResHeader_t *const iconHdr = findResHeader(PalmOS_PRC_ResType_ApplicationIcon, 1000);
	if (!iconHdr) {
		// Not found...
		return {};
	}

	// Found the application icon.
	// Read the BitmapType struct.
	uint32_t addr = be32_to_cpu(iconHdr->addr);
	// TODO: Verify the address is after the resource header section.

	// Read all of the BitmapType struct headers.
	// - Key: Struct header address
	// - Value: PalmOS_BitmapType_t
	// TODO: Do we need to store all of them?
	map<uint32_t, PalmOS_BitmapType_t> bitmapTypeMap;

	while (addr != 0) {
		PalmOS_BitmapType_t bitmapType;
		size_t size = file->seekAndRead(addr, &bitmapType, sizeof(bitmapType));
		if (size != sizeof(bitmapType)) {
			// Failed to read the BitmapType struct.
			return {};
		}

		// Validate the bitmap version and get the next bitmap address.
		const uint32_t cur_addr = addr;
		switch (bitmapType.version) {
			default:
				// Not supported.
				assert(!"Unsupported BitmapType version.");
				return {};

			case 0:
				// v0: no chaining, so this is the last bitmap
				addr = 0;
				break;

			case 1:
				// v1: next bitmap has a relative offset in DWORDs
				if (bitmapType.pixelSize == 255) {
					// FIXME: This is the next bitmap after a v2 bitmap,
					// but there's 16 bytes of weird data between it?
					addr += 16;
					continue;
				}
				if (bitmapType.v1.nextDepthOffset != 0) {
					const unsigned int nextDepthOffset = be16_to_cpu(bitmapType.v1.nextDepthOffset);
					addr += nextDepthOffset * sizeof(uint32_t);
				} else {
					addr = 0;
				}
				break;

			case 2:
				// v2: next bitmap has a relative offset in DWORDs
				// FIXME: v2 sometimes has an extra +0x04 DWORDs offset to the next bitmap? (+0x10 bytes)
				if (bitmapType.v2.nextDepthOffset != 0) {
					const unsigned int nextDepthOffset = be16_to_cpu(bitmapType.v2.nextDepthOffset);// + 0x4;
					addr += nextDepthOffset * sizeof(uint32_t);
				} else {
					addr = 0;
				}
				break;

			case 3:
				// v3: next bitmap has a relative offset in bytes
				if (bitmapType.v3.nextBitmapOffset != 0) {
					addr += be32_to_cpu(bitmapType.v3.nextBitmapOffset);
				} else {
					addr = 0;
				}
				break;
		}

		// Sanity check: Icon must have valid dimensions.
		const int width = be16_to_cpu(bitmapType.width);
		const int height = be16_to_cpu(bitmapType.height);
		assert(width > 0);
		assert(height > 0);
		if (width > 0 && height > 0) {
			bitmapTypeMap.emplace(cur_addr, bitmapType);
		}
	}

	if (bitmapTypeMap.empty()) {
		// No bitmaps...
		return {};
	}

#if 0
	// AFL testing only: Load *all* bitmaps;
	for (auto iter = bitmapTypeMap.cbegin(); iter != bitmapTypeMap.cend(); ++iter) {
		printf("loading: %08X\n", iter->first);
		rp_image_ptr img = loadBitmap_tAIB(&(iter->second), iter->first);
		if (!img) {
			printf("FAILED: %08X\n", iter->first);
		}
	}
#endif

	// Select the "best" bitmap.
	const PalmOS_BitmapType_t *selBitmapType = nullptr;
	const auto iter_end = bitmapTypeMap.cend();
	for (auto iter = bitmapTypeMap.cbegin(); iter != iter_end; ++iter) {
		if (!selBitmapType) {
			// No bitmap selected yet.
			addr = iter->first;
			selBitmapType = &(iter->second);
			continue;
		}

		// Check if this bitmap is "better" than the currently selected one.
		const PalmOS_BitmapType_t *checkBitmapType = &(iter->second);

		// First check: Is it a newer version?
		if (checkBitmapType->version > selBitmapType->version) {
			addr = iter->first;
			selBitmapType = checkBitmapType;
			continue;
		}

		// Next check: Is the color depth higher? (bpp; pixelSize)
		if (checkBitmapType->pixelSize > selBitmapType->pixelSize) {
			addr = iter->first;
			selBitmapType = checkBitmapType;
			continue;
		}

		// TODO: v3: Does it have a higher pixel density?

		// Final check: Is it a bigger icon?
		// TODO: Check total area instead of width vs. height?
		if (checkBitmapType->width > selBitmapType->width ||
		    checkBitmapType->height > selBitmapType->height)
		{
			addr = iter->first;
			selBitmapType = checkBitmapType;
			continue;
		}
	}

	if (!selBitmapType) {
		// No bitmap was selected...
		return {};
	}

	// Load the bitmap.
	img_icon = loadBitmap_tAIB(selBitmapType, addr);
	return img_icon;
}

/**
 * Get a string resource. (max 255 bytes + NULL)
 * @param type		[in] Resource type
 * @param id		[in] Resource ID
 * @return String resource, or empty string if not found.
 */
string PalmOSPrivate::load_string(uint32_t type, uint16_t id)
{
	const PalmOS_PRC_ResHeader_t *const pRes = findResHeader(type, id);
	if (!pRes)
		return {};

	// Read up to 256 bytes at tAIN's address.
	// This resource contains a NULL-terminated string.
	char buf[256];
	size_t size = file->seekAndRead(be32_to_cpu(pRes->addr), buf, sizeof(buf));
	if (size == 0 || size > sizeof(buf)) {
		// Out of range.
		return {};
	}

	// Make sure the buffer is NULL-terminated.
	buf[size-1] = '\0';

	// Find the first NULL byte and use that as the length.
	const char *const nullpos = static_cast<const char*>(memchr(buf, '\0', size));
	if (!nullpos)
		return {};

	// TODO: Text encoding.
	const int len = static_cast<int>(nullpos - buf);
	return latin1_to_utf8(buf, len);
}

/** PalmOS **/

/**
 * Read a Palm OS application.
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
PalmOS::PalmOS(const IRpFilePtr &file)
	: super(new PalmOSPrivate(file))
{
	// This class handles resource files.
	// Defaulting to "ResourceLibrary". We'll check for other types later.
	RP_D(PalmOS);
	d->mimeType = PalmOSPrivate::mimeTypes[0];

	if (!d->file) {
		// Could not ref() the file handle.
		return;
	}

	// Read the PRC header.
	d->file->rewind();
	size_t size = d->file->read(&d->prcHeader, sizeof(d->prcHeader));
	if (size != sizeof(d->prcHeader)) {
		d->file.reset();
		return;
	}

	// Check if this application is supported.
	const char *const filename = file->filename();
	const DetectInfo info = {
		{0, sizeof(d->prcHeader), reinterpret_cast<const uint8_t*>(&d->prcHeader)},
		FileSystem::file_ext(filename),	// ext
		0		// szFile (not needed for PalmOS)
	};
	d->isValid = (isRomSupported_static(&info) >= 0);

	if (!d->isValid) {
		d->file.reset();
		return;
	}

	// Determine the file type.
	struct file_type_map_t {
		uint32_t prc_type;
		RomData::FileType fileType;
	};
	static const file_type_map_t file_type_map[] = {
		{'appl', FileType::Executable},
		{'appm', FileType::Executable},
		{'libr', FileType::SharedLibrary},
		{'JLib', FileType::SharedLibrary},
	};
	// TODO: More heuristics for detecting executables with non-standard types?
	const uint32_t type = be32_to_cpu(d->prcHeader.type);
	d->fileType = FileType::ResourceLibrary;
	for (const auto &ft : file_type_map) {
		if (ft.prc_type == type) {
			d->fileType = ft.fileType;
			break;
		}
	}

	// Load the resource headers.
	const unsigned int num_records = be16_to_cpu(d->prcHeader.num_records);
	const size_t res_size = num_records * sizeof(PalmOS_PRC_ResHeader_t);
	d->resHeaders.resize(num_records);
	size = d->file->seekAndRead(sizeof(d->prcHeader), d->resHeaders.data(), res_size);
	if (size != res_size) {
		// Seek and/or read error.
		d->resHeaders.clear();
		d->file.reset();
		d->isValid = false;
	}
}

/**
 * Is a ROM image supported by this class?
 * @param info DetectInfo containing ROM detection information.
 * @return Class-specific system ID (>= 0) if supported; -1 if not.
 */
int PalmOS::isRomSupported_static(const DetectInfo *info)
{
	assert(info != nullptr);
	assert(info->header.pData != nullptr);
	assert(info->header.addr == 0);
	if (!info || !info->ext || !info->header.pData ||
	    info->header.addr != 0 ||
	    info->header.size < sizeof(PalmOS_PRC_Header_t))
	{
		// Either no detection information was specified,
		// or the header is too small.
		return -1;
	}

	// NOTE: File extension must match, and the type field must be non-zero.
	bool ok = false;
	for (const char *const *ext = PalmOSPrivate::exts;
	     *ext != nullptr; ext++)
	{
		if (!strcasecmp(info->ext, *ext)) {
			// File extension is supported.
			ok = true;
			break;
		}
	}
	if (!ok) {
		// File extension doesn't match.
		return -1;
	}

	// Check for a non-zero type field.
	// TODO: Better heuristics.
	const PalmOS_PRC_Header_t *const prcHeader =
		reinterpret_cast<const PalmOS_PRC_Header_t*>(info->header.pData);
	if (prcHeader->type != 0) {
		// Type is non-zero.
		// TODO: More checks?
		return 0;
	}

	// Not supported.
	return -1;
}

/**
 * Get the name of the system the loaded ROM is designed for.
 * @param type System name type. (See the SystemName enum.)
 * @return System name, or nullptr if type is invalid.
 */
const char *PalmOS::systemName(unsigned int type) const
{
	RP_D(const PalmOS);
	if (!d->isValid || !isSystemNameTypeValid(type))
		return nullptr;

	// game.com has the same name worldwide, so we can
	// ignore the region selection.
	static_assert(SYSNAME_TYPE_MASK == 3,
		"PalmOS::systemName() array index optimization needs to be updated.");

	static const char *const sysNames[4] = {
		"Palm OS",
		"Palm OS",
		"Palm",
		nullptr
	};

	return sysNames[type & SYSNAME_TYPE_MASK];
}

/**
 * Get a bitfield of image types this object can retrieve.
 * @return Bitfield of supported image types. (ImageTypesBF)
 */
uint32_t PalmOS::supportedImageTypes(void) const
{
	// TODO: Check for a valid "tAIB" resource first?
	return IMGBF_INT_ICON;
}

/**
 * Get a bitfield of image types this class can retrieve.
 * @return Bitfield of supported image types. (ImageTypesBF)
 */
uint32_t PalmOS::supportedImageTypes_static(void)
{
	return IMGBF_INT_ICON;
}

/**
 * Get a list of all available image sizes for the specified image type.
 * @param imageType Image type.
 * @return Vector of available image sizes, or empty vector if no images are available.
 */
vector<RomData::ImageSizeDef> PalmOS::supportedImageSizes(ImageType imageType) const
{
	ASSERT_supportedImageSizes(imageType);

	// TODO: Check for a valid "tAIB" resource first?
	// Also, what are the valid icon sizes?
	RP_D(const PalmOS);
	if (!d->isValid || imageType != IMG_INT_ICON) {
		// Only IMG_INT_ICON is supported,
		// and/or the ROM doesn't have an icon.
		return {};
	}

	return {{nullptr, 32, 32, 0}};
}

/**
 * Get a list of all available image sizes for the specified image type.
 * @param imageType Image type.
 * @return Vector of available image sizes, or empty vector if no images are available.
 */
vector<RomData::ImageSizeDef> PalmOS::supportedImageSizes_static(ImageType imageType)
{
	ASSERT_supportedImageSizes(imageType);

	if (imageType != IMG_INT_ICON) {
		// Only IMG_INT_ICON is supported.
		return {};
	}

	// TODO: What are the valid icon sizes?
	return {{nullptr, 32, 32, 0}};
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
uint32_t PalmOS::imgpf(ImageType imageType) const
{
	ASSERT_imgpf(imageType);

	switch (imageType) {
		case IMG_INT_ICON: {
			// TODO: Check for a valid "tAIB" resource first?
			// Use nearest-neighbor scaling.
			return IMGPF_RESCALE_NEAREST;
		}
		default:
			break;
	}
	return 0;
}

/**
 * Load field data.
 * Called by RomData::fields() if the field data hasn't been loaded yet.
 * @return Number of fields read on success; negative POSIX error code on error.
 */
int PalmOS::loadFieldData(void)
{
	RP_D(PalmOS);
	if (!d->fields.empty()) {
		// Field data *has* been loaded...
		return 0;
	} else if (!d->file) {
		// File isn't open.
		return -EBADF;
	} else if (!d->isValid) {
		// ROM image isn't valid.
		return -EIO;
	}

	// TODO: Add more fields?
	// TODO: Text encoding?
	const PalmOS_PRC_Header_t *const prcHeader = &d->prcHeader;
	d->fields.reserve(6);	// Maximum of 6 fields.

	// Title
	d->fields.addField_string(C_("RomData", "Title"),
		latin1_to_utf8(prcHeader->name, sizeof(prcHeader->name)),
		RomFields::STRF_TRIM_END);

	// Type
	// TODO: Filter out non-ASCII characters.
	{
		char s_type[5];
		int ret = LibRpText::fourCCtoString(s_type, sizeof(s_type), be32_to_cpu(prcHeader->type));
		if (ret == 0) {
			d->fields.addField_string(C_("PalmOS", "Type"), s_type,
				RomFields::STRF_MONOSPACE);
		}
	}

	// Creator ID
	// TODO: Filter out non-ASCII characters.
	if (prcHeader->creator_id != 0) {
		char s_creator_id[5];
		int ret = LibRpText::fourCCtoString(s_creator_id, sizeof(s_creator_id), be32_to_cpu(prcHeader->creator_id));
		if (ret == 0) {
			d->fields.addField_string(C_("PalmOS", "Creator ID"), s_creator_id,
				RomFields::STRF_MONOSPACE);
		}
	}

	// Icon Name
	const string s_tAIN = d->load_string(PalmOS_PRC_ResType_ApplicationName, 1000);
	if (!s_tAIN.empty()) {
		d->fields.addField_string(C_("PalmOS", "Icon Name"), s_tAIN);
	}

	// Version
	const string s_tver = d->load_string(PalmOS_PRC_ResType_ApplicationVersion, 1000);
	if (!s_tver.empty()) {
		d->fields.addField_string(C_("RomData", "Version"), s_tver);
	}

	// Category
	const string s_taic = d->load_string(PalmOS_PRC_ResType_ApplicationCategory, 1000);
	if (!s_taic.empty()) {
		d->fields.addField_string(C_("PalmOS", "Category"), s_taic);
	}

	// Finished reading the field data.
	return static_cast<int>(d->fields.count());
}

/**
 * Load metadata properties.
 * Called by RomData::metaData() if the metadata hasn't been loaded yet.
 * @return Number of metadata properties read on success; negative POSIX error code on error.
 */
int PalmOS::loadMetaData(void)
{
	RP_D(PalmOS);
	if (d->metaData != nullptr) {
		// Metadata *has* been loaded...
		return 0;
	} else if (!d->file) {
		// File isn't open.
		return -EBADF;
	} else if (!d->isValid) {
		// Unknown ROM image type.
		return -EIO;
	}

	// Create the metadata object.
	d->metaData = new RomMetaData();
	d->metaData->reserve(1);	// Maximum of 1 metadata property.

	// TODO: Text encoding?
	const PalmOS_PRC_Header_t *const prcHeader = &d->prcHeader;

	// Title
	d->metaData->addMetaData_string(Property::Title,
		latin1_to_utf8(prcHeader->name, sizeof(prcHeader->name)),
		RomMetaData::STRF_TRIM_END);

	// Finished reading the metadata.
	return static_cast<int>(d->metaData->count());
}

/**
 * Load an internal image.
 * Called by RomData::image().
 * @param imageType	[in] Image type to load.
 * @param pImage	[out] Reference to rp_image_const_ptr to store the image in.
 * @return 0 on success; negative POSIX error code on error.
 */
int PalmOS::loadInternalImage(ImageType imageType, rp_image_const_ptr &pImage)
{
	ASSERT_loadInternalImage(imageType, pImage);

	RP_D(PalmOS);
	if (imageType != IMG_INT_ICON) {
		pImage.reset();
		return -ENOENT;
	} else if (d->img_icon != nullptr) {
		pImage = d->img_icon;
		return 0;
	} else if (!d->file) {
		pImage.reset();
		return -EBADF;
	} else if (!d->isValid) {
		pImage.reset();
		return -EIO;
	}

	pImage = d->loadIcon();
	return ((bool)pImage ? 0 : -EIO);
}

}
