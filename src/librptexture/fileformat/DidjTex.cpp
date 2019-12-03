/***************************************************************************
 * ROM Properties Page shell extension. (librptexture)                     *
 * DidjTex.hpp: Leapster Didj .tex reader.                                 *
 *                                                                         *
 * Copyright (c) 2019 by David Korth.                                      *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "DidjTex.hpp"
#include "FileFormat_p.hpp"

#include "didj_tex_structs.h"

// librpbase
#include "librpbase/common.h"
#include "librpbase/byteswap.h"
#include "librpbase/aligned_malloc.h"
#include "librpbase/RomFields.hpp"
#include "librpbase/TextFuncs.hpp"
#include "librpbase/file/IRpFile.hpp"
using LibRpBase::IRpFile;
using LibRpBase::rp_sprintf;

// librptexture
#include "img/rp_image.hpp"
#include "decoder/ImageDecoder.hpp"

// zlib
#include <zlib.h>

// C includes. (C++ namespace)
#include <cassert>
#include <cstdio>
#include <cstring>

// C++ includes.
#include <memory>
using std::unique_ptr;

namespace LibRpTexture {

FILEFORMAT_IMPL(DidjTex)

class DidjTexPrivate : public FileFormatPrivate
{
	public:
		DidjTexPrivate(DidjTex *q, IRpFile *file);
		~DidjTexPrivate();

	private:
		typedef FileFormatPrivate super;
		RP_DISABLE_COPY(DidjTexPrivate)

	public:
		// .tex header.
		Didj_Tex_Header texHeader;

		// Decoded image.
		rp_image *img;

		// Invalid pixel format message.
		char invalid_pixel_format[24];

		/**
		 * Load the DidjTex image.
		 * @return Image, or nullptr on error.
		 */
		const rp_image *loadDidjTexImage(void);
};

/** DidjTexPrivate **/

DidjTexPrivate::DidjTexPrivate(DidjTex *q, IRpFile *file)
	: super(q, file)
	, img(nullptr)
{
	// Clear the structs and arrays.
	memset(&texHeader, 0, sizeof(texHeader));
	memset(invalid_pixel_format, 0, sizeof(invalid_pixel_format));
}

DidjTexPrivate::~DidjTexPrivate()
{
	delete img;
}

/**
 * Load the .tex image.
 * @return Image, or nullptr on error.
 */
const rp_image *DidjTexPrivate::loadDidjTexImage(void)
{
	if (img) {
		// Image has already been loaded.
		return img;
	} else if (!this->file) {
		// Can't load the image.
		return nullptr;
	}

	// Sanity checks:
	// - .tex files shouldn't be more than 128 KB.
	// - Uncompressed size shouldn't be more than 1 MB.
	const unsigned int uncompr_size = le32_to_cpu(texHeader.uncompr_size);
	assert(file->size() <= 128*1024);
	assert(uncompr_size <= 1*1024*1024);
	if (file->size() > 128*1024 || uncompr_size > 1*1024*1024) {
		return nullptr;
	}

	// Load the compressed data.
	// NOTE: Compressed size is validated in the constructor.
	const unsigned int compr_size = le32_to_cpu(texHeader.compr_size);
	unique_ptr<uint8_t[]> compr_data(new uint8_t[compr_size]);
	size_t size = file->seekAndRead(sizeof(texHeader), compr_data.get(), compr_size);
	if (size != compr_size) {
		// Seek and/or read error.
		return nullptr;
	}

	// Decompress the data.
	unique_ptr<uint8_t[]> uncompr_data(new uint8_t[uncompr_size]);

	// Initialize zlib.
	z_stream strm;
	memset(&strm, 0, sizeof(strm));
	int ret = inflateInit(&strm);
	if (ret != Z_OK) {
		// Error initializing inflate.
		return nullptr;
	}

	strm.avail_in = compr_size;
	strm.next_in = compr_data.get();
	strm.avail_out = uncompr_size;
	strm.next_out = uncompr_data.get();
	do {
		ret = inflate(&strm, Z_NO_FLUSH);
		switch (ret) {
			case Z_OK:
			case Z_STREAM_END:
				break;

			case Z_NEED_DICT:
			case Z_DATA_ERROR:
			case Z_MEM_ERROR:	// TODO: Handle this?
				// Error decompressing...
				inflateEnd(&strm);
				return nullptr;
		}
	} while (strm.avail_out > 0 && ret != Z_STREAM_END);

	// Buffer should be full and we should be at the end of the stream.
	assert(strm.avail_out == 0);
	assert(ret == Z_STREAM_END);
	if (strm.avail_out != 0 || ret != Z_STREAM_END) {
		// Error decompressing...
		inflateEnd(&strm);
		return nullptr;
	}

	// Finished decompressing.
	inflateEnd(&strm);

	// Decode the image.
	rp_image *imgtmp = nullptr;
	const unsigned int width_pow2 = le32_to_cpu(texHeader.width_pow2);
	const unsigned int height_pow2 = le32_to_cpu(texHeader.height_pow2);
	switch (le32_to_cpu(texHeader.px_format)) {
		case DIDJ_PIXEL_FORMAT_4BPP_RGB565: {
			// 4bpp with RGB565 palette.
			const int pal_siz = static_cast<int>(16 * sizeof(uint16_t));
			const int img_siz = (width_pow2 * height_pow2) / 2;
			assert(static_cast<unsigned int>(pal_siz + img_siz) == uncompr_size);
			if (static_cast<unsigned int>(pal_siz + img_siz) != uncompr_size) {
				// Incorrect size.
				return nullptr;
			}

			const uint16_t *const pal_buf = reinterpret_cast<const uint16_t*>(uncompr_data.get());
			const uint8_t *const img_buf = &uncompr_data[pal_siz];
			imgtmp = ImageDecoder::fromLinearCI4(ImageDecoder::PXF_RGB565, false,
				width_pow2, height_pow2,
				img_buf, img_siz, pal_buf, pal_siz);
			break;
		}

		default:
			//assert(!"Format not supported.");
			return nullptr;
	}

	// TODO: Truncate the image if the pow2 size doesn't match the real size?

	img = imgtmp;
	return img;
}

/** DidjTex **/

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
DidjTex::DidjTex(IRpFile *file)
	: super(new DidjTexPrivate(this, file))
{
	RP_D(DidjTex);

	if (!d->file) {
		// Could not ref() the file handle.
		return;
	}

	// Read the .tex header.
	d->file->rewind();
	size_t size = d->file->read(&d->texHeader, sizeof(d->texHeader));
	if (size != sizeof(d->texHeader)) {
		d->file->unref();
		d->file = nullptr;
		return;
	}

	// TODO: Add an isTextureSupported() function to FileFormat.

	// Check heuristics.
	if (d->texHeader.magic != cpu_to_le32(DIDJ_TEX_HEADER_MAGIC) ||
	    d->texHeader.num_images != cpu_to_le32(1))
	{
		// Incorrect values.
		d->isValid = false;
		d->file->unref();
		d->file = nullptr;
		return;
	}

	// Verify compressed filesize.
	const int64_t filesize = d->file->size();
	if (static_cast<int64_t>(le32_to_cpu(d->texHeader.compr_size) + sizeof(d->texHeader)) != filesize) {
		// Incorrect compressed filesize.
		d->isValid = false;
		d->file->unref();
		d->file = nullptr;
		return;
	}

	// Looks like it's valid.
	d->isValid = true;

	// Cache the texture dimensions.
	d->dimensions[0] = le32_to_cpu(d->texHeader.width);
	d->dimensions[1] = le32_to_cpu(d->texHeader.height);
	d->dimensions[2] = 0;
}

/** Class-specific functions that can be used even if isValid() is false. **/

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
const char *const *DidjTex::supportedFileExtensions_static(void)
{
	static const char *const exts[] = {
		".tex",		// NOTE: Too generic...

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
const char *const *DidjTex::supportedMimeTypes_static(void)
{
	static const char *const mimeTypes[] = {
		// Unofficial MIME types.
		// TODO: Get these upstreamed on FreeDesktop.org.
		"image/x-didj-texture",

		nullptr
	};
	return mimeTypes;
}

/** Property accessors **/

/**
 * Get the texture format name.
 * @return Texture format name, or nullptr on error.
 */
const char *DidjTex::textureFormatName(void) const
{
	RP_D(const DidjTex);
	if (!d->isValid)
		return nullptr;

	return "Leapster Didj .tex";
}

/**
 * Get the pixel format, e.g. "RGB888" or "DXT1".
 * @return Pixel format, or nullptr if unavailable.
 */
const char *DidjTex::pixelFormat(void) const
{
	RP_D(const DidjTex);
	if (!d->isValid) {
		// Not supported.
		return nullptr;
	}

	// TODO: Verify other formats.
	static const char *const pxfmt_tbl[] = {
		// 0x00
		nullptr, nullptr, nullptr, nullptr,
		nullptr, nullptr, nullptr, nullptr,

		// 0x08
		nullptr, "4bpp with RGB565 palette",
	};

	if (d->texHeader.px_format < ARRAY_SIZE(pxfmt_tbl) &&
	    pxfmt_tbl[d->texHeader.px_format] != nullptr)
	{
		return pxfmt_tbl[d->texHeader.px_format];
	}

	// Invalid pixel format.
	// Store an error message instead.
	// TODO: Localization?
	if (d->invalid_pixel_format[0] == '\0') {
		snprintf(const_cast<DidjTexPrivate*>(d)->invalid_pixel_format,
			sizeof(d->invalid_pixel_format),
			"Unknown (0x%08X)", d->texHeader.px_format);
	}
	return d->invalid_pixel_format;
}

/**
 * Get the mipmap count.
 * @return Number of mipmaps. (0 if none; -1 if format doesn't support mipmaps)
 */
int DidjTex::mipmapCount(void) const
{
	// TODO: Does .tex support mipmaps?
	return -1;
}

#ifdef ENABLE_LIBRPBASE_ROMFIELDS
/**
 * Get property fields for rom-properties.
 * @param fields RomFields object to which fields should be added.
 * @return Number of fields added, or 0 on error.
 */
int DidjTex::getFields(LibRpBase::RomFields *fields) const
{
	// TODO: Localization.
#define C_(ctx, str) str
#define NOP_C_(ctx, str) str

	assert(fields != nullptr);
	if (!fields)
		return 0;

	RP_D(DidjTex);
	if (!d->isValid) {
		// Not valid.
		return -EIO;
	}

	const int initial_count = fields->count();
	fields->reserve(initial_count + 1);	// Maximum of 1 field. (TODO)

	// Internal dimensions.
	// Usually a power of two.
	fields->addField_dimensions(C_("DidjTex", "Internal Dimensions"),
		le32_to_cpu(d->texHeader.width_pow2), le32_to_cpu(d->texHeader.height_pow2), 0);

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
const rp_image *DidjTex::image(void) const
{
	RP_D(const DidjTex);
	if (!d->isValid) {
		// Not valid.
		return nullptr;
	}

	// Load the image.
	return const_cast<DidjTexPrivate*>(d)->loadDidjTexImage();
}

/**
 * Get the image for the specified mipmap.
 * Mipmap 0 is the largest image.
 * @param mip Mipmap number.
 * @return Image, or nullptr on error.
 */
const rp_image *DidjTex::mipmap(int mip) const
{
	// Allowing mipmap 0 for compatibility.
	if (mip == 0) {
		return image();
	}
	return nullptr;
}

}
