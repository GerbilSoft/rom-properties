/***************************************************************************
 * ROM Properties Page shell extension. (librptexture)                     *
 * ASTC.hpp: ASTC image reader.                                            *
 *                                                                         *
 * Copyright (c) 2017-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "ASTC.hpp"
#include "FileFormat_p.hpp"

#include "astc_structs.h"

// librpbase, librpfile
using LibRpBase::RomFields;
using LibRpFile::IRpFile;

// librptexture
#include "ImageSizeCalc.hpp"
#include "img/rp_image.hpp"
#include "decoder/ImageDecoder_ASTC.hpp"

namespace LibRpTexture {

class ASTCPrivate final : public FileFormatPrivate
{
	public:
		ASTCPrivate(ASTC *q, IRpFile *file);
		~ASTCPrivate() final;

	private:
		typedef FileFormatPrivate super;
		RP_DISABLE_COPY(ASTCPrivate)

	public:
		/** TextureInfo **/
		static const char *const exts[];
		static const char *const mimeTypes[];
		static const TextureInfo textureInfo;

	public:
		// ASTC header.
		ASTC_Header astcHeader;

		// Decoded image.
		rp_image *img;

		// Pixel format message
		char pixel_format[20];

		/**
		 * Load the image.
		 * @return Image, or nullptr on error.
		 */
		const rp_image *loadImage(void);
};

FILEFORMAT_IMPL(ASTC)

/** ASTCPrivate **/

/* TextureInfo */
const char *const ASTCPrivate::exts[] = {
	".astc",
	".dds",	// Some .dds files are actually ASTC.

	nullptr
};
const char *const ASTCPrivate::mimeTypes[] = {
	// Official MIME types.
	"image/astc",

	nullptr
};
const TextureInfo ASTCPrivate::textureInfo = {
	exts, mimeTypes
};

ASTCPrivate::ASTCPrivate(ASTC *q, IRpFile *file)
	: super(q, file, &textureInfo)
	, img(nullptr)
{
	// Clear the structs and arrays.
	memset(&astcHeader, 0, sizeof(astcHeader));
	memset(pixel_format, 0, sizeof(pixel_format));
}

ASTCPrivate::~ASTCPrivate()
{
	UNREF(img);
}

/**
 * Load the image.
 * @return Image, or nullptr on error.
 */
const rp_image *ASTCPrivate::loadImage(void)
{
	if (img) {
		// Image has already been loaded.
		return img;
	} else if (!this->file || !this->isValid) {
		// Can't load the image.
		return nullptr;
	}

	// NOTE: We can't use astcHeader's width/height fields because
	// they're 24-bit values. Use dimensions[] instead.

	// Sanity check: Maximum image dimensions of 32768x32768.
	// NOTE: `height == 0` is allowed here. (1D texture)
	assert(dimensions[0] > 0);
	assert(dimensions[0] <= 32768);
	assert(dimensions[1] <= 32768);
	if (dimensions[0] == 0 || dimensions[0] > 32768 ||
	    dimensions[1] > 32768)
	{
		// Invalid image dimensions.
		return nullptr;
	}

	if (dimensions[2] > 1) {
		// 3D textures are not supported.
		return nullptr;
	}

	if (file->size() > 128*1024*1024) {
		// Sanity check: VTF files shouldn't be more than 128 MB.
		return nullptr;
	}
	const uint32_t file_sz = static_cast<uint32_t>(file->size());

	// Handle a 1D texture as a "width x 1" 2D texture.
	// NOTE: Handling a 3D texture as a single 2D texture.
	const int height = (dimensions[1] > 0 ? dimensions[1] : 1);

	// Calculate the expected size.
	const unsigned int expected_size = ImageSizeCalc::calcImageSizeASTC(
		dimensions[0], height,
		astcHeader.blockdimX, astcHeader.blockdimY);
	if (expected_size == 0 || expected_size > file_sz) {
		// Invalid image size.
		return nullptr;
	}

	// The ASTC file format does not support mipmaps.
	// The texture data is located directly after the header.
	const unsigned int texDataStartAddr = static_cast<unsigned int>(sizeof(astcHeader));

	// Seek to the start of the texture data.
	int ret = file->seek(texDataStartAddr);
	if (ret != 0) {
		// Seek error.
		return nullptr;
	}

	// Read the texture data.
	auto buf = aligned_uptr<uint8_t>(16, expected_size);
	size_t size = file->read(buf.get(), expected_size);
	if (size != expected_size) {
		// Read error.
		return nullptr;
	}

	// Decode the image.
	img = ImageDecoder::fromASTC(
		dimensions[0], height,
		buf.get(), expected_size,
		astcHeader.blockdimX, astcHeader.blockdimY);
	return img;
}

/** ASTC **/

/**
 * Read an ASTC image file.
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
ASTC::ASTC(IRpFile *file)
	: super(new ASTCPrivate(this, file))
{
	RP_D(ASTC);
	d->mimeType = "image/astc";	// official

	if (!d->file) {
		// Could not ref() the file handle.
		return;
	}

	// Read the ASTC header.
	d->file->rewind();
	size_t size = d->file->read(&d->astcHeader, sizeof(d->astcHeader));
	if (size != sizeof(d->astcHeader)) {
		UNREF_AND_NULL_NOCHK(d->file);
		return;
	}

	// Verify the ASTC magic.
	if (d->astcHeader.magic != cpu_to_le32(ASTC_MAGIC)) {
		// Incorrect magic.
		UNREF_AND_NULL_NOCHK(d->file);
		return;
	}

	// File is valid.
	d->isValid = true;

	// Cache the dimensions for the FileFormat base class.
	d->dimensions[0] = (d->astcHeader.width[2] << 16) |
	                   (d->astcHeader.width[1] <<  8) |
	                    d->astcHeader.width[0];
	d->dimensions[1] = (d->astcHeader.height[2] << 16) |
	                   (d->astcHeader.height[1] <<  8) |
	                    d->astcHeader.height[0];

	const unsigned int depth = (d->astcHeader.depth[2] << 16) |
	                           (d->astcHeader.depth[1] <<  8) |
	                            d->astcHeader.depth[0];
	if (depth > 1) {
		d->dimensions[2] = depth;
	}
}

/** Property accessors **/

/**
 * Get the texture format name.
 * @return Texture format name, or nullptr on error.
 */
const char *ASTC::textureFormatName(void) const
{
	RP_D(const ASTC);
	if (!d->isValid)
		return nullptr;

	return "ASTC";
}

/**
 * Get the pixel format, e.g. "RGB888" or "DXT1".
 * @return Pixel format, or nullptr if unavailable.
 */
const char *ASTC::pixelFormat(void) const
{
	RP_D(const ASTC);
	if (!d->isValid)
		return nullptr;

	if (d->pixel_format[0] == '\0') {
		if (d->dimensions[2] <= 1) {
			snprintf(const_cast<ASTCPrivate*>(d)->pixel_format,
				sizeof(d->pixel_format),
				"ASTC_%dx%d",
				d->astcHeader.blockdimX, d->astcHeader.blockdimY);
		} else {
			snprintf(const_cast<ASTCPrivate*>(d)->pixel_format,
				sizeof(d->pixel_format),
				"ASTC_%dx%dx%d",
				d->astcHeader.blockdimX, d->astcHeader.blockdimY, d->astcHeader.blockdimZ);
		}
	}
	return d->pixel_format;
}

/**
 * Get the mipmap count.
 * @return Number of mipmaps. (0 if none; -1 if format doesn't support mipmaps)
 */
int ASTC::mipmapCount(void) const
{
	RP_D(const ASTC);
	if (!d->isValid)
		return -1;

	// The ASTC file format does not support mipmaps.
	return -1;
}

#ifdef ENABLE_LIBRPBASE_ROMFIELDS
/**
 * Get property fields for rom-properties.
 * @param fields RomFields object to which fields should be added.
 * @return Number of fields added, or 0 on error.
 */
int ASTC::getFields(RomFields *fields) const
{
	assert(fields != nullptr);
	if (!fields)
		return 0;

	RP_D(const ASTC);
	if (!d->isValid) {
		// Unknown file type.
		return -EIO;
	}

	// TODO: Add fields?
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
const rp_image *ASTC::image(void) const
{
	RP_D(const ASTC);
	if (!d->isValid) {
		// Unknown file type.
		return nullptr;
	}

	// Load the image.
	return const_cast<ASTCPrivate*>(d)->loadImage();
}

/**
 * Get the image for the specified mipmap.
 * Mipmap 0 is the largest image.
 * @param mip Mipmap number.
 * @return Image, or nullptr on error.
 */
const rp_image *ASTC::mipmap(int mip) const
{
	// Allowing mipmap 0 for compatibility.
	if (mip == 0) {
		return image();
	}
	return nullptr;
}

}
