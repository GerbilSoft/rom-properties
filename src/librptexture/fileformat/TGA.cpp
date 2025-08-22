/***************************************************************************
 * ROM Properties Page shell extension. (librptexture)                     *
 * TGA.cpp: TrueVision TGA reader.                                         *
 *                                                                         *
 * Copyright (c) 2019-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "TGA.hpp"
#include "FileFormat_p.hpp"

#include "tga_structs.h"

// timegm()
#include "time_r.h"

// Other rom-properties libraries
using namespace LibRpFile;
using namespace LibRpText;
using LibRpBase::RomFields;

// librptexture
#include "img/rp_image.hpp"
#include "decoder/ImageDecoder_Linear.hpp"

// C++ STL classes
using std::array;
using std::string;
using std::unique_ptr;

namespace LibRpTexture {

class TGAPrivate final : public FileFormatPrivate
{
public:
	TGAPrivate(TGA *q, const IRpFilePtr &file);

private:
	typedef FileFormatPrivate super;
	RP_DISABLE_COPY(TGAPrivate)

public:
	/** TextureInfo **/
	static const array<const char*, 1+1> exts;
	static const array<const char*, 8+1> mimeTypes;
	static const TextureInfo textureInfo;

public:
	enum class TexType {
		Unknown		= -1,

		TGA1		= 0,	// Old TGA (1.0)
		TGA2		= 1,	// New TGA (2.0)

		Max
	};
	TexType texType;

	// TGA headers
	TGA_Header tgaHeader;
	TGA_ExtArea tgaExtArea;
	TGA_Footer tgaFooter;

	// Is HFlip/VFlip needed?
	// Some textures may be stored upside-down due to
	// the way GL texture coordinates are interpreted.
	// Default without orientation metadata is HFlip=false, VFlip=false
	rp_image::FlipOp flipOp;

	// Alpha channel type
	TGA_AlphaType_e alphaType;

	// Decoded image
	rp_image_ptr img;

	/**
	 * Decompress RLE image data.
	 * @param pDest Output buffer.
	 * @param dest_len Size of output buffer.
	 * @param pSrc Input buffer.
	 * @param src_len Size of input buffer.
	 * @param bytespp Bytes per pixel.
	 * @return 0 on success; non-zero on error.
	 */
	ATTR_ACCESS_SIZE(write_only, 1, 2)
	ATTR_ACCESS_SIZE(read_only, 3, 4)
	static int decompressRLE(uint8_t *pDest, size_t dest_len,
				 const uint8_t *pSrc, size_t s_len,
				 uint8_t bytespp);

	/**
	 * Load the TGA image.
	 * @return Image, or nullptr on error.
	 */
	rp_image_const_ptr loadImage(void);

	/**
	 * Convert a TGA timestamp to UNIX time.
	 * @param timestamp TGA timestamp. (little-endian)
	 * @return UNIX time, or -1 if invalid or not set.
	 */
	static time_t tgaTimeToUnixTime(const TGA_DateStamp *timestamp);
};

FILEFORMAT_IMPL(TGA)

/** TGAPrivate **/

/* TextureInfo */
const array<const char*, 1+1> TGAPrivate::exts = {{
	".tga",
	// TODO: Other obsolete file extensions?

	nullptr
}};
const array<const char*, 8+1> TGAPrivate::mimeTypes = {{
	// Unofficial MIME types from FreeDesktop.org.
	"image/x-tga",
	"image/x-targa",

	// shared-mime-info lists these MIME types as aliases.
	"image/targa",
	"image/tga",
	"image/x-icb",
	"application/tga",
	"application/x-targa",
	"application/x-tga",

	nullptr
}};
const TextureInfo TGAPrivate::textureInfo = {
	exts.data(), mimeTypes.data()
};

TGAPrivate::TGAPrivate(TGA *q, const IRpFilePtr &file)
	: super(q, file, &textureInfo)
	, texType(TexType::Unknown)
	, flipOp(rp_image::FLIP_V)	// default orientation requires vertical flip
	, alphaType(TGA_ALPHATYPE_PRESENT)
	{
	// Clear the structs.
	memset(&tgaHeader, 0, sizeof(tgaHeader));
	memset(&tgaExtArea, 0, sizeof(tgaExtArea));
	memset(&tgaFooter, 0, sizeof(tgaFooter));
}

/**
 * Decompress RLE image data.
 * @param pDest Output buffer.
 * @param dest_len Size of output buffer.
 * @param pSrc Input buffer.
 * @param src_len Size of input buffer.
 * @param bytespp Bytes per pixel.
 * @return 0 on success; non-zero on error.
 */
int TGAPrivate::decompressRLE(uint8_t *pDest, size_t dest_len,
			      const uint8_t *pSrc, size_t src_len,
			      uint8_t bytespp)
{
	// TGA 2.0 says RLE packets must not cross scanlines.
	// TGA 1.0 allowed this, so we'll allow it for compatibility.
	// TODO: Make bytespp a template parameter?

	// Process RLE packets until we run out of source data or
	// space in the destination buffer.
	const uint8_t *const pSrcEnd = pSrc + src_len;
	uint8_t *const pDestEnd = pDest + dest_len;

	for (; pSrc < pSrcEnd && pDest < pDestEnd; ) {
		// Check the next packet.
		const uint8_t pkt = *pSrc++;
		// Low 7 bits indicate number of pixels.
		// [0,127]; add 1 for [1,128].
		unsigned int count = (pkt & 0x7F) + 1;
		if (pDest + (count * bytespp) > pDestEnd) {
			// Buffer overflow!
			return -EIO;
		}

		if (pkt & 0x80) {
			// High bit is set. This is an RLE packet.
			// One pixel is duplicated `count` number of times.

			// Based on Duff's Device.
			for (; count > 0; count--) {
				const uint8_t *pSrcTmp = pSrc;
				switch (bytespp) {
					case 4:		*pDest++ = *pSrcTmp++;	// fall-through
					case 3:		*pDest++ = *pSrcTmp++;	// fall-through
					case 2:		*pDest++ = *pSrcTmp++;	// fall-through
					case 1:		*pDest++ = *pSrcTmp++;	// fall-through
					default:	break;
				}
			}
			pSrc += bytespp;
		} else {
			// High bit is clear. This is a raw packet.
			// `count` number of pixels follow.
			const unsigned int cpysize = count * bytespp;
			memcpy(pDest, pSrc, cpysize);
			pDest += cpysize;
			pSrc += cpysize;
		}
	}

	// In case we didn't have enough data, clear the rest of
	// the destination buffer.
	if (pDest < pDestEnd) {
		memset(pDest, 0, pDestEnd - pDest);
	}
	return 0;
}

/**
 * Load the TGA image.
 * @return Image, or nullptr on error.
 */
rp_image_const_ptr TGAPrivate::loadImage(void)
{
	if (img) {
		// Image has already been loaded.
		return img;
	} else if (!this->file) {
		// Can't load the image.
		return nullptr;
	}

        // Sanity check: Maximum image dimensions of 32768x32768.
        assert(tgaHeader.img.width > 0);
	assert(tgaHeader.img.width <= 32768);
        assert(tgaHeader.img.height > 0);
	assert(tgaHeader.img.height <= 32768);
	if (tgaHeader.img.width == 0  || tgaHeader.img.width > 32768 ||
	    tgaHeader.img.height == 0 || tgaHeader.img.height > 32768)
	{
		// Invalid image dimensions.
		return nullptr;
	}

	// Image data starts immediately after the TGA header.
	const unsigned int img_data_offset = static_cast<unsigned int>(sizeof(tgaHeader) + tgaHeader.id_length);
	if (file->seek(img_data_offset) != 0) {
		// Seek error.
		return nullptr;
	}

	// Is the image colormapped (palette)?
	const bool isColorMapImage =
		 (tgaHeader.image_type & ~TGA_IMAGETYPE_RLE_FLAG) == TGA_IMAGETYPE_COLORMAP ||
		 (tgaHeader.image_type == TGA_IMAGETYPE_HUFFMAN_COLORMAP) ||
		 (tgaHeader.image_type == TGA_IMAGETYPE_HUFFMAN_4PASS_COLORMAP);

	unsigned int cmap_size = 0;	// size of the color map in the TGA file
	size_t pal_siz = 0;		// size of pal_data[] (usually 256*bytespp)
	unique_ptr<uint8_t[]> pal_data;
	if (tgaHeader.color_map_type >= 1) {
		const unsigned int cmap_bytespp = (tgaHeader.cmap.bpp == 15 ? 2 : (tgaHeader.cmap.bpp / 8));
		cmap_size = tgaHeader.cmap.len * cmap_bytespp;

		if (isColorMapImage) {
			// Load the color map. (up to 256 colors only)
			if (tgaHeader.cmap.idx0 + tgaHeader.cmap.len > 256) {
				// Too many colors.
				return nullptr;
			}

			pal_siz = 256U * cmap_bytespp;
			pal_data.reset(new uint8_t[pal_siz]);

			// If we don't have a full 256-color palette,
			// zero-initialize it. (TODO: Optimize this?)
			if (tgaHeader.cmap.idx0 != 0 || tgaHeader.cmap.len < 256) {
				memset(pal_data.get(), 0, pal_siz);
			}

			// Read the palette.
			size_t size = file->read(&pal_data[tgaHeader.cmap.idx0], cmap_size);
			if (size != cmap_size) {
				// Read error.
				return nullptr;
			}
		} else {
			// Color map is present, but this is not a colormap image.
			// Skip over the color map.
			file->seek_cur(cmap_size);
		}
	}

	// Allocate a buffer for the image.
	// NOTE: Assuming scanlines are not padded. (pitch == width)
	const unsigned int bytespp = (tgaHeader.img.bpp == 15 ? 2 : (tgaHeader.img.bpp / 8));
	const size_t img_siz = static_cast<size_t>(tgaHeader.img.width) * static_cast<size_t>(tgaHeader.img.height) * bytespp;
	auto img_data = aligned_uptr<uint8_t>(16, img_siz);

	if (tgaHeader.image_type == TGA_IMAGETYPE_HUFFMAN_COLORMAP) {
		// TODO: Huffman+Delta compression.
		return nullptr;
	} else if (tgaHeader.image_type == TGA_IMAGETYPE_HUFFMAN_4PASS_COLORMAP) {
		// TODO: Huffman+Delta, 4-pass compression.
		return nullptr;
	} else if (tgaHeader.image_type & TGA_IMAGETYPE_RLE_FLAG) {
		// Image is compressed. We'll need to decompress it.
		// Read the rest of the file into memory.
		const off64_t fileSize = file->size();
		if (fileSize > TGA_MAX_SIZE ||
		    fileSize < static_cast<off64_t>(img_data_offset + sizeof(tgaFooter) + cmap_size))
		{
			return nullptr;
		}

		const size_t rle_size = static_cast<size_t>(fileSize) - img_data_offset - cmap_size;
		unique_ptr<uint8_t[]> rle_data(new uint8_t[rle_size]);
		size_t size = file->read(rle_data.get(), rle_size);
		if (size != rle_size) {
			// Read error.
			return nullptr;
		}

		// Decompress the RLE image.
		int ret = decompressRLE(img_data.get(), img_siz, rle_data.get(), rle_size, bytespp);
		if (ret != 0) {
			// Error decompressing the RLE image.
			return nullptr;
		}
	} else {
		// Image is not compressed. Read it directly.
		size_t size = file->read(img_data.get(), img_siz);
		if (size != static_cast<size_t>(img_siz)) {
			// Read error.
			return nullptr;
		}
	}

	// Decode the image.
	// NOTE: gdk-pixbuf assumes alpha is present if:
	// - Truecolor: bpp == 16 or bpp == 32
	// - Colormap: cmap_bpp == 32
	// QtImageFormats assumes alpha is always present.
	// TODO: Handle premultiplied alpha.
	rp_image_ptr imgtmp;
	switch (tgaHeader.image_type & ~TGA_IMAGETYPE_RLE_FLAG) {
		case TGA_IMAGETYPE_COLORMAP:
		case TGA_IMAGETYPE_HUFFMAN_COLORMAP:
		case TGA_IMAGETYPE_HUFFMAN_4PASS_COLORMAP: {
			// Palette
			// TODO: attr_dir number of bits for alpha?

			// Determine the pixel format.
			ImageDecoder::PixelFormat px_fmt;
			switch (tgaHeader.cmap.bpp) {
				case 15:	px_fmt = ImageDecoder::PixelFormat::RGB555; break;
				case 16: {
					// ARGB1555
					// TODO: Verify that it's ARGB1555 and not RGB565.
					// FIXME: Is ARGB1555 supported? We have some 16bpp cmap test images
					// that are expecting the high bit to be ignored.
					switch (alphaType) {
						default:
						case TGA_ALPHATYPE_NONE:
						case TGA_ALPHATYPE_UNDEFINED_IGNORE:
						case TGA_ALPHATYPE_UNDEFINED_RETAIN:
							px_fmt = ImageDecoder::PixelFormat::RGB555;
							break;

						case TGA_ALPHATYPE_PRESENT:
						case TGA_ALPHATYPE_PREMULTIPLIED:
							px_fmt = ImageDecoder::PixelFormat::ARGB1555;
							break;
					}
					break;
				}
				case 24:	px_fmt = ImageDecoder::PixelFormat::RGB888; break;
				case 32: {
					switch (alphaType) {
						default:
						case TGA_ALPHATYPE_NONE:
						case TGA_ALPHATYPE_UNDEFINED_IGNORE:
						case TGA_ALPHATYPE_UNDEFINED_RETAIN:
							px_fmt = ImageDecoder::PixelFormat::xRGB8888;
							break;

						case TGA_ALPHATYPE_PRESENT:
						case TGA_ALPHATYPE_PREMULTIPLIED:
							px_fmt = ImageDecoder::PixelFormat::ARGB8888;
							break;
					}
					break;
				}
				default:	px_fmt = ImageDecoder::PixelFormat::Unknown; break;
			}
			assert(px_fmt != ImageDecoder::PixelFormat::Unknown);

			// Decode the image.
			imgtmp = ImageDecoder::fromLinearCI8(px_fmt,
				tgaHeader.img.width, tgaHeader.img.height,
				img_data.get(), img_siz,
				pal_data.get(), pal_siz);
			break;
		}

		case TGA_IMAGETYPE_TRUECOLOR:
			// Truecolor
			// TODO: attr_dir number of bits for alpha?
			switch (tgaHeader.img.bpp) {
				case 15:
					// RGB555
					imgtmp = ImageDecoder::fromLinear16(
						ImageDecoder::PixelFormat::RGB555,
						tgaHeader.img.width, tgaHeader.img.height,
						reinterpret_cast<const uint16_t*>(img_data.get()), img_siz);
					break;

				case 16: {
					// ARGB1555
					// TODO: Verify that it's ARGB1555 and not RGB565.
					ImageDecoder::PixelFormat px_fmt;
					switch (alphaType) {
						default:
						case TGA_ALPHATYPE_NONE:
						case TGA_ALPHATYPE_UNDEFINED_IGNORE:
						case TGA_ALPHATYPE_UNDEFINED_RETAIN:
							px_fmt = ImageDecoder::PixelFormat::RGB555;
							break;

						case TGA_ALPHATYPE_PRESENT:
						case TGA_ALPHATYPE_PREMULTIPLIED:
							px_fmt = ImageDecoder::PixelFormat::ARGB1555;
							break;
					}

					imgtmp = ImageDecoder::fromLinear16(px_fmt,
						tgaHeader.img.width, tgaHeader.img.height,
						reinterpret_cast<const uint16_t*>(img_data.get()), img_siz);
					break;
				}

				case 24:
					// RGB888
					imgtmp = ImageDecoder::fromLinear24(
						ImageDecoder::PixelFormat::RGB888,
						tgaHeader.img.width, tgaHeader.img.height,
						img_data.get(), img_siz);
					break;

				case 32: {
					// ARGB8888
					ImageDecoder::PixelFormat px_fmt;
					switch (alphaType) {
						default:
						case TGA_ALPHATYPE_NONE:
						case TGA_ALPHATYPE_UNDEFINED_IGNORE:
						case TGA_ALPHATYPE_UNDEFINED_RETAIN:
							px_fmt = ImageDecoder::PixelFormat::xRGB8888;
							break;

						case TGA_ALPHATYPE_PRESENT:
						case TGA_ALPHATYPE_PREMULTIPLIED:
							px_fmt = ImageDecoder::PixelFormat::ARGB8888;
							break;
					}

					imgtmp = ImageDecoder::fromLinear32(px_fmt,
						tgaHeader.img.width, tgaHeader.img.height,
						reinterpret_cast<const uint32_t*>(img_data.get()), img_siz);
					break;
				}

				default:
					assert(!"Unsupported truecolor TGA bpp.");
					break;
			}
			break;

		case TGA_IMAGETYPE_GRAYSCALE: {
			// Grayscale
			switch (tgaHeader.img.bpp) {
				case 8: {
					assert(alphaType < TGA_ALPHATYPE_PRESENT);
					assert((tgaHeader.img.attr_dir & 0x0F) == 0);
					const bool ok = ((alphaType < TGA_ALPHATYPE_PRESENT) && ((tgaHeader.img.attr_dir & 0x0F) == 0));
					if (!ok)
						break;

					// Create a grayscale palette.
					uint32_t palette[256];
					uint32_t gray = 0xFF000000U;
					for (unsigned int i = 0; i < 256; i++, gray += 0x010101U) {
						palette[i] = gray;
					}

					// Decode the image.
					imgtmp = ImageDecoder::fromLinearCI8(
						ImageDecoder::PixelFormat::Host_ARGB32,
						tgaHeader.img.width, tgaHeader.img.height,
						img_data.get(), img_siz,
						palette, sizeof(palette));
					break;
				}

				case 16: {
					assert(alphaType >= TGA_ALPHATYPE_PRESENT);
					assert((tgaHeader.img.attr_dir & 0x0F) == 0);
					const bool ok = ((alphaType >= TGA_ALPHATYPE_PRESENT) && ((tgaHeader.img.attr_dir & 0x0F) == 8));
					if (!ok)
						break;

					// Decode the image.
					// TODO: Verify; handle premultiplied alpha.
					imgtmp = ImageDecoder::fromLinear16(
						ImageDecoder::PixelFormat::IA8,
						tgaHeader.img.width, tgaHeader.img.height,
						reinterpret_cast<const uint16_t*>(img_data.get()), img_siz);
					break;
				}

				default:
					assert(!"Unsupported grayscale TGA bpp.");
					break;
			}
			break;
		}

		default:
			assert(!"Unsupported TGA format.");
			break;
	}

	// Post-processing: Check if a flip is needed.
	if (imgtmp && flipOp != rp_image::FLIP_NONE) {
		rp_image_ptr flipimg = imgtmp->flip(flipOp);
		if (flipimg) {
			imgtmp = std::move(flipimg);
		}
	}

	img = imgtmp;
	return imgtmp;
}

/**
 * Convert a TGA timestamp to UNIX time.
 * @param timestamp TGA timestamp. (little-endian)
 * @return UNIX time, or -1 if invalid or not set.
 */
time_t TGAPrivate::tgaTimeToUnixTime(const TGA_DateStamp *timestamp)
{
	assert(timestamp != nullptr);
	if (!timestamp)
		return -1;

	// Convert TGA time to Unix time.
	// NOTE: struct tm has some oddities:
	// - tm_year: year - 1900
	// - tm_mon: 0 == January
	struct tm tgatime;

	// Copy and byteswap everything first.
	tgatime.tm_year	= le16_to_cpu(timestamp->year) - 1900;
	tgatime.tm_mon	= le16_to_cpu(timestamp->month) - 1;
	tgatime.tm_mday = le16_to_cpu(timestamp->day);
	tgatime.tm_hour	= le16_to_cpu(timestamp->hour);
	tgatime.tm_min	= le16_to_cpu(timestamp->min);
	tgatime.tm_sec	= le16_to_cpu(timestamp->sec);

	// tm_wday and tm_yday are output variables.
	tgatime.tm_wday = 0;
	tgatime.tm_yday = 0;
	tgatime.tm_isdst = 0;

	// If conversion fails, this will return -1.
	return timegm(&tgatime);
}

/** TGA **/

/**
 * Read a Leapster Didj .tex image file.
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
TGA::TGA(const IRpFilePtr &file)
	: super(new TGAPrivate(this, file))
{
	RP_D(TGA);
	d->mimeType = TGAPrivate::mimeTypes[0];	// unofficial
	d->textureFormatName = "TrueVision TGA";

	if (!d->file) {
		// Could not ref() the file handle.
		return;
	}

	// Sanity check: TGA file shouldn't be larger than 16 MB,
	// and it must be larger than tgaHeader and tgaFooter.
	const off64_t fileSize = d->file->size();
	assert(fileSize >= static_cast<off64_t>(sizeof(d->tgaHeader) + sizeof(d->tgaFooter)));
	assert(fileSize <= TGA_MAX_SIZE);
	if (fileSize < static_cast<off64_t>(sizeof(d->tgaHeader) + sizeof(d->tgaFooter)) ||
	    fileSize > TGA_MAX_SIZE)
	{
		d->file.reset();
		return;
	}

	// Read the .tga footer to verify if this is TGA 1.0 or 2.0.
	size_t size = d->file->seekAndRead(fileSize - sizeof(d->tgaFooter), &d->tgaFooter, sizeof(d->tgaFooter));
	if (size != sizeof(d->tgaFooter)) {
		// Could not read the TGA footer.
		// The file is likely too small to be valid.
		d->file.reset();
		return;
	}

	// Check if it's TGA1 or TGA2.
	if (!memcmp(d->tgaFooter.signature, TGA_SIGNATURE, sizeof(d->tgaFooter.signature))) {
		// TGA2 signature found.
		// Extension Area and Developer Area may be present.
		// These would be located *after* the image data.
		d->texType = TGAPrivate::TexType::TGA2;
	} else {
		// No signature. Assume TGA1.
		d->texType = TGAPrivate::TexType::TGA1;
	}

	// Read the .tga header.
	d->file->rewind();
	size = d->file->read(&d->tgaHeader, sizeof(d->tgaHeader));
	if (size != sizeof(d->tgaHeader)) {
		// Seek and/or read error.
		d->file.reset();
		return;
	}

	// Assume alpha may be present unless the TGA2 extension area says otherwise.
	// (...except for 8-bit grayscale)
	if (unlikely(((d->tgaHeader.image_type & ~TGA_IMAGETYPE_RLE_FLAG) == TGA_IMAGETYPE_GRAYSCALE) && d->tgaHeader.img.bpp == 8)) {
		d->alphaType = TGA_ALPHATYPE_NONE;
	} else {
		d->alphaType = TGA_ALPHATYPE_PRESENT;
	}

	if (d->texType == TGAPrivate::TexType::TGA2) {
		// Check for an extension area.
		const uint32_t ext_offset = le32_to_cpu(d->tgaFooter.ext_offset);
		if (ext_offset != 0 && fileSize > static_cast<off64_t>(sizeof(d->tgaExtArea)) &&
		    (ext_offset < (fileSize - static_cast<off64_t>(sizeof(d->tgaExtArea)))))
		{
			// We have an extension area.
			size = d->file->seekAndRead(ext_offset, &d->tgaExtArea, sizeof(d->tgaExtArea));
			if (size == sizeof(d->tgaExtArea) &&
			    d->tgaExtArea.size == cpu_to_le16(sizeof(d->tgaExtArea)))
			{
				// Extension area read successfully.
				d->alphaType = static_cast<TGA_AlphaType_e>(d->tgaExtArea.attributes_type);
			} else {
				// Error reading the extension area.
				d->tgaExtArea.size = 0;
			}
		}

		// TODO: Developer area?
	}

	// Looks like it's valid.
	d->isValid = true;

#if SYS_BYTEORDER == SYS_BIG_ENDIAN
	// Byteswap the header.
	d->tgaHeader.cmap.idx0		= le16_to_cpu(d->tgaHeader.cmap.idx0);
	d->tgaHeader.cmap.len		= le16_to_cpu(d->tgaHeader.cmap.len);
	d->tgaHeader.img.x_origin	= le16_to_cpu(d->tgaHeader.img.x_origin);
	d->tgaHeader.img.y_origin	= le16_to_cpu(d->tgaHeader.img.y_origin);
	d->tgaHeader.img.width		= le16_to_cpu(d->tgaHeader.img.width);
	d->tgaHeader.img.height		= le16_to_cpu(d->tgaHeader.img.height);
#endif /* SYS_BYTEORDER == SYS_BIG_ENDIAN */

	// Cache the texture dimensions.
	d->dimensions[0] = d->tgaHeader.img.width;
	d->dimensions[1] = d->tgaHeader.img.height;
	d->dimensions[2] = 0;

	// Is a flip operation required?
	// H-flip: Default is no; if set, flip.
	d->flipOp = rp_image::FLIP_NONE;
	if (d->tgaHeader.img.attr_dir & TGA_ORIENTATION_X_MASK) {
		d->flipOp = rp_image::FLIP_H;
	}
	// V-flip: Default is yes; if set, don't flip.
	if (!(d->tgaHeader.img.attr_dir & TGA_ORIENTATION_Y_MASK)) {
		d->flipOp = static_cast<rp_image::FlipOp>(d->flipOp | rp_image::FLIP_V);
	}
}

/** Property accessors **/

/**
 * Get the pixel format, e.g. "RGB888" or "DXT1".
 * @return Pixel format, or nullptr if unavailable.
 */
const char *TGA::pixelFormat(void) const
{
	RP_D(const TGA);
	if (!d->isValid || static_cast<int>(d->texType) < 0) {
		// Not supported.
		return nullptr;
	}

	// NOTE: gdk-pixbuf assumes alpha is present if:
	// - Truecolor: bpp == 16 or bpp == 32
	// - Colormap: cmap_bpp == 32
	// QtImageFormats assumes alpha is always present.
	const char *fmt = nullptr;
	switch (d->tgaHeader.image_type) {
		case TGA_IMAGETYPE_COLORMAP:
		case TGA_IMAGETYPE_RLE_COLORMAP:
			// Palette
			if (d->tgaHeader.cmap.len <= 256) {
				// 8bpp
				switch (d->tgaHeader.cmap.bpp) {
					case 15:
						fmt = "8bpp with RGB555 palette";
						break;
					case 16:
						// FIXME: Is ARGB1555 supported? We have some 16bpp cmap test images
						// that are expecting the high bit to be ignored.
						fmt = "8bpp with RGB555 palette";
						break;
					case 24:
						fmt = "8bpp with RGB888 palette";
						break;
					case 32:
						fmt = "8bpp with ARGB8888 palette";
						break;
					default:
						break;
				}
			} else {
				// 16bpp
				switch (d->tgaHeader.cmap.bpp) {
					case 15:
						fmt = "16bpp with RGB555 palette";
						break;
					case 16:
						// FIXME: Is ARGB1555 supported? We have some 16bpp cmap test images
						// that are expecting the high bit to be ignored.
						fmt = "16bpp with RGB555 palette";
						break;
					case 24:
						fmt = "16bpp with RGB888 palette";
						break;
					case 32:
						fmt = "16bpp with ARGB8888 palette";
						break;
					default:
						break;
				}
			}
			break;

		case TGA_IMAGETYPE_TRUECOLOR:
		case TGA_IMAGETYPE_RLE_TRUECOLOR:
			// True color
			switch (d->tgaHeader.img.bpp) {
				case 15:
					fmt = "RGB555";
					break;
				case 16:
					fmt = "ARGB1555";
					break;
				case 24:
					fmt = "RGB888";
					break;
				case 32:
					fmt = "ARGB8888";
					break;
				default:
					break;
			}
			break;

		case TGA_IMAGETYPE_GRAYSCALE:
		case TGA_IMAGETYPE_RLE_GRAYSCALE:
			// Grayscale
			switch (d->tgaHeader.img.bpp) {
				case 8:
					fmt = "8bpp grayscale";
					break;
				case 16:
					if ((d->tgaHeader.img.attr_dir & 0x0F) == 8) {
						fmt = "IA8";
					}
					break;
				default:
					break;
			}
			break;

		default:
			break;
	}

	// TODO: Indicate invalid formats?
	return fmt;
}

#ifdef ENABLE_LIBRPBASE_ROMFIELDS
/**
 * Get property fields for rom-properties.
 * @param fields RomFields object to which fields should be added.
 * @return Number of fields added, or 0 on error.
 */
int TGA::getFields(RomFields *fields) const
{
	assert(fields != nullptr);
	if (!fields)
		return 0;

	RP_D(const TGA);
	if (!d->isValid || static_cast<int>(d->texType) < 0) {
		// Not valid.
		return -EIO;
	}

	const int initial_count = fields->count();
	fields->reserve(initial_count + 13);	// Maximum of 13 fields.

	// TGA header.
	const TGA_Header *const tgaHeader = &d->tgaHeader;

	// Orientation
	// Uses KTX1 format for display.
	// Default 00 orientation: H-flip NO, V-flip YES
	char s_orientation[] = "S=?,T=?";
	s_orientation[2] = (tgaHeader->img.attr_dir & TGA_ORIENTATION_X_MASK) ? 'l' : 'r';
	s_orientation[6] = (tgaHeader->img.attr_dir & TGA_ORIENTATION_Y_MASK) ? 'd' : 'u';
	fields->addField_string(C_("TGA", "Orientation"), s_orientation);

	// Compression
	const char *s_compression;
	if (tgaHeader->image_type == TGA_IMAGETYPE_HUFFMAN_COLORMAP) {
		s_compression = C_("TGA|Compression", "Huffman+Delta");
	} else if (tgaHeader->image_type == TGA_IMAGETYPE_HUFFMAN_4PASS_COLORMAP) {
		s_compression = C_("TGA|Compression", "Huffman+Delta, 4-pass");
	} else if (tgaHeader->image_type & TGA_IMAGETYPE_RLE_FLAG) {
		s_compression = "RLE";
	} else {
		s_compression = C_("TGA|Compression", "None");
	}
	fields->addField_string(C_("TGA", "Compression"), s_compression);

	// Alpha channel
	// TODO: dpgettext_expr()
	const char *s_alphaType;
	static const array<const char*, 5> alphaType_tbl = {{
		NOP_C_("TGA|AlphaType", "None"),
		NOP_C_("TGA|AlphaType", "Undefined (ignore)"),
		NOP_C_("TGA|AlphaType", "Undefined (retain)"),
		NOP_C_("TGA|AlphaType", "Present"),
		NOP_C_("TGA|AlphaType", "Premultiplied"),
	}};
	s_alphaType = alphaType_tbl[d->alphaType >= 0 && static_cast<size_t>(d->alphaType) < alphaType_tbl.size()
		? d->alphaType : TGA_ALPHATYPE_UNDEFINED_IGNORE];
	fields->addField_string(C_("TGA", "Alpha Type"), s_alphaType);

	/** Extension area fields **/

	const TGA_ExtArea *const tgaExtArea = &d->tgaExtArea;
	if (tgaExtArea->size == cpu_to_le16(sizeof(*tgaExtArea))) {
		// Author
		if (tgaExtArea->author_name[0] != '\0') {
			fields->addField_string(C_("RomData", "Author"),
				cp1252_to_utf8(tgaExtArea->author_name, sizeof(tgaExtArea->author_name)));
		}

		// Comments
		string s_comments;
		for (size_t i = 0; i < ARRAY_SIZE(tgaExtArea->author_comment); i++) {
			if (tgaExtArea->author_comment[i][0] != '\0') {
				if (!s_comments.empty()) {
					s_comments += '\n';
				}
				s_comments += cp1252_to_utf8(
					tgaExtArea->author_comment[i],
					sizeof(tgaExtArea->author_comment[i]));
			}
		}
		if (!s_comments.empty()) {
			fields->addField_string(C_("RomData", "Comments"), s_comments);
		}

		// Timestamp
		// NOTE: Copy needed to avoid an unaligned pointer.
		const TGA_DateStamp tga_dateStamp = tgaExtArea->timestamp;
		const time_t timestamp = d->tgaTimeToUnixTime(&tga_dateStamp);
		if (timestamp != -1) {
			fields->addField_dateTime(C_("TGA", "Last Saved Time"), timestamp,
				RomFields::RFT_DATETIME_HAS_DATE |
				RomFields::RFT_DATETIME_HAS_TIME |
				RomFields::RFT_DATETIME_IS_UTC);	// no timezone
		}

		// Job name/ID
		if (tgaExtArea->job_id[0] != '\0') {
			fields->addField_string(C_("TGA", "Job Name/ID"),
				cp1252_to_utf8(tgaExtArea->job_id, sizeof(tgaExtArea->job_id)));
		}

		// Job time
		// TODO: Elapsed time data type?
		if (tgaExtArea->job_time.hours != cpu_to_le16(0) ||
		    tgaExtArea->job_time.mins != cpu_to_le16(0) ||
		    tgaExtArea->job_time.secs != cpu_to_le16(0))
		{
			fields->addField_string(C_("TGA", "Job Time"),
				fmt::format(FSTR("{:d}'{:d}\"{:d}"),
					le16_to_cpu(tgaExtArea->job_time.hours),
					le16_to_cpu(tgaExtArea->job_time.mins),
					le16_to_cpu(tgaExtArea->job_time.secs)));
		}

		// Software ID
		if (tgaExtArea->software_id[0] != '\0') {
			fields->addField_string(C_("TGA", "Software ID"),
				cp1252_to_utf8(tgaExtArea->software_id, sizeof(tgaExtArea->software_id)));
		}

		// Software version
		if (tgaExtArea->sw_version.number != 0 ||
		    tgaExtArea->sw_version.letter != ' ')
		{
			array<char, 2> lstr = {{tgaExtArea->sw_version.letter, '\0'}};
			if (lstr[0] == ' ')
				lstr[0] = '\0';

			fields->addField_string(C_("TGA", "Software Version"),
				fmt::format(FSTR("{:0>1d}.{:0>2d}{:s}"),
					tgaExtArea->sw_version.number / 100,
					tgaExtArea->sw_version.number % 100, lstr.data()));
		}

		// Key color
		// TODO: RFT_COLOR field?
		if (tgaExtArea->key_color != cpu_to_le32(0)) {
			fields->addField_string_numeric(C_("TGA", "Key Color"),
				le32_to_cpu(tgaExtArea->key_color),
				RomFields::Base::Hex, 8, RomFields::STRF_MONOSPACE);
		}

		// Pixel aspect ratio
		if (tgaExtArea->pixel_aspect_ratio.denominator != cpu_to_le16(0)) {
			fields->addField_string(C_("TGA", "Pixel Aspect Ratio"),
				fmt::format(FSTR("{:d}:{:d}"),
					tgaExtArea->pixel_aspect_ratio.numerator,
					tgaExtArea->pixel_aspect_ratio.denominator));
		}

		// Gamma value
		if (tgaExtArea->gamma_value.denominator != cpu_to_le16(0)) {
			const int gamma = static_cast<int>(
				static_cast<double>(tgaExtArea->gamma_value.numerator) /
				static_cast<double>(tgaExtArea->gamma_value.denominator) * 10);
			fields->addField_string(C_("TGA", "Gamma Value"),
				fmt::format(FSTR("{:d}.{:d}"),
					static_cast<unsigned int>(gamma / 10),
					static_cast<unsigned int>(gamma % 10)));
		}
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
rp_image_const_ptr TGA::image(void) const
{
	RP_D(const TGA);
	if (!d->isValid || static_cast<int>(d->texType) < 0) {
		// Unknown file type.
		return nullptr;
	}

	// Load the image.
	return const_cast<TGAPrivate*>(d)->loadImage();
}

} // namespace LibRpTexture
