/***************************************************************************
 * ROM Properties Page shell extension. (librptexture)                     *
 * ValveVTF3.hpp: Valve VTF3 (PS3) image reader.                           *
 *                                                                         *
 * Copyright (c) 2017-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "ValveVTF3.hpp"
#include "FileFormat_p.hpp"

#include "vtf3_structs.h"

// Other rom-properties libraries
using namespace LibRpFile;
using LibRpBase::RomFields;

// librptexture
#include "ImageSizeCalc.hpp"
#include "img/rp_image.hpp"
#include "decoder/ImageDecoder_S3TC.hpp"

namespace LibRpTexture {

class ValveVTF3Private final : public FileFormatPrivate
{
	public:
		ValveVTF3Private(ValveVTF3 *q, const IRpFilePtr &file);
		~ValveVTF3Private() final = default;

	private:
		typedef FileFormatPrivate super;
		RP_DISABLE_COPY(ValveVTF3Private)

	public:
		/** TextureInfo **/
		static const char *const exts[];
		static const char *const mimeTypes[];
		static const TextureInfo textureInfo;

	public:
		// VTF3 header
		VTF3HEADER vtf3Header;

		// Decoded image
		rp_image_ptr img;

		/**
		 * Load the image.
		 * @return Image, or nullptr on error.
		 */
		rp_image_const_ptr loadImage(void);

#if SYS_BYTEORDER == SYS_BIG_ENDIAN
		/**
		 * Byteswap a float. (TODO: Move to byteswap_rp.h?)
		 * @param f Float to byteswap.
		 * @return Byteswapped flaot.
		 */
		static inline float __swabf(float f)
		{
			union {
				uint32_t u32;
				float f;
			} u32_f;
			u32_f.f = f;
			u32_f.u32 = __swab32(u32_f.u32);
			return u32_f.f;
		}
#endif /* SYS_BYTEORDER == SYS_BIG_ENDIAN */
};

FILEFORMAT_IMPL(ValveVTF3)

/** ValveVTF3Private **/

/* TextureInfo */
const char *const ValveVTF3Private::exts[] = {
	".vtf",
	//".vtx",	// TODO: Some files might use the ".vtx" extension.

	nullptr
};
const char *const ValveVTF3Private::mimeTypes[] = {
	// Unofficial MIME types.
	// TODO: Get these upstreamed on FreeDesktop.org.
	"image/x-vtf3",

	nullptr
};
const TextureInfo ValveVTF3Private::textureInfo = {
	exts, mimeTypes
};

ValveVTF3Private::ValveVTF3Private(ValveVTF3 *q, const IRpFilePtr &file)
	: super(q, file, &textureInfo)
{
	// Clear the VTF3 header struct.
	memset(&vtf3Header, 0, sizeof(vtf3Header));
}

/**
 * Load the image.
 * @return Image, or nullptr on error.
 */
rp_image_const_ptr ValveVTF3Private::loadImage(void)
{
	if (img) {
		// Image has already been loaded.
		return img;
	} else if (!this->isValid || !this->file) {
		// Can't load the image.
		return nullptr;
	}

	// Sanity check: Maximum image dimensions of 32768x32768.
	// NOTE: `height == 0` is allowed here. (1D texture)
	assert(vtf3Header.width > 0);
	assert(vtf3Header.width <= 32768);
	assert(vtf3Header.height <= 32768);
	if (vtf3Header.width == 0 || vtf3Header.width > 32768 ||
	    vtf3Header.height > 32768)
	{
		// Invalid image dimensions.
		return nullptr;
	}

	if (file->size() > 128*1024*1024) {
		// Sanity check: VTF files shouldn't be more than 128 MB.
		return nullptr;
	}
	const uint32_t file_sz = static_cast<uint32_t>(file->size());

	// Handle a 1D texture as a "width x 1" 2D texture.
	// NOTE: Handling a 3D texture as a single 2D texture.
	const int height = (vtf3Header.height > 0 ? vtf3Header.height : 1);

	// Calculate the expected size.
	size_t expected_size = ImageSizeCalc::T_calcImageSize(vtf3Header.width, height);
	if (!(vtf3Header.flags & VTF3_FLAG_ALPHA)) {
		// Image does not have an alpha channel,
		// which means it's DXT1 and thus 4bpp.
		expected_size /= 2;
	}

	if (expected_size == 0 || expected_size > file_sz) {
		// Invalid image size.
		return nullptr;
	}

	// TODO: Adjust for mipmaps.
	// For now, assuming the main texture is at the end of the file.
	const unsigned int texDataStartAddr = static_cast<unsigned int>(file_sz - expected_size);

	// Texture cannot start inside of the VTF header.
	assert(texDataStartAddr >= sizeof(vtf3Header));
	if (texDataStartAddr < sizeof(vtf3Header)) {
		// Invalid texture data start address.
		return nullptr;
	}

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
	if (vtf3Header.flags & VTF3_FLAG_ALPHA) {
		// Image has an alpha channel.
		// Encoded using DXT5.
		img = ImageDecoder::fromDXT5(
			vtf3Header.width, height,
			buf.get(), expected_size);
	} else {
		// Image does not have an alpha channel.
		// Encoded using DXT1.
		img = ImageDecoder::fromDXT1(
			vtf3Header.width, height,
			buf.get(), expected_size);
	}

	return img;
}

/** ValveVTF3 **/

/**
 * Read a Valve VTF3 (PS3) image file.
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
ValveVTF3::ValveVTF3(const IRpFilePtr &file)
	: super(new ValveVTF3Private(this, file))
{
	RP_D(ValveVTF3);
	d->mimeType = "image/x-vtf3";	// unofficial, not on fd.o
	d->textureFormatName = "Valve VTF3 (PS3)";

	if (!d->file) {
		// Could not ref() the file handle.
		return;
	}

	// Read the VTF3 header.
	d->file->rewind();
	size_t size = d->file->read(&d->vtf3Header, sizeof(d->vtf3Header));
	if (size != sizeof(d->vtf3Header)) {
		d->file.reset();
		return;
	}

	// Verify the VTF magic.
	if (d->vtf3Header.signature != cpu_to_be32(VTF3_SIGNATURE)) {
		// Incorrect magic.
		d->file.reset();
		return;
	}

	// File is valid.
	d->isValid = true;

#if SYS_BYTEORDER == SYS_LIL_ENDIAN
	// Header is stored in big-endian, so it always
	// needs to be byteswapped on little-endian.
	// NOTE: Signature is *not* byteswapped.
	d->vtf3Header.flags		= be32_to_cpu(d->vtf3Header.flags);
	d->vtf3Header.width		= be16_to_cpu(d->vtf3Header.width);
	d->vtf3Header.height		= be16_to_cpu(d->vtf3Header.height);
#endif

	// Cache the dimensions for the FileFormat base class.
	d->dimensions[0] = d->vtf3Header.width;
	d->dimensions[1] = d->vtf3Header.height;
}

/** Property accessors **/

/**
 * Get the pixel format, e.g. "RGB888" or "DXT1".
 * @return Pixel format, or nullptr if unavailable.
 */
const char *ValveVTF3::pixelFormat(void) const
{
	RP_D(const ValveVTF3);
	if (!d->isValid)
		return nullptr;

	// Only two formats are supported.
	return (d->vtf3Header.flags & VTF3_FLAG_ALPHA) ? "DXT5" : "DXT1";
}

#ifdef ENABLE_LIBRPBASE_ROMFIELDS
/**
 * Get property fields for rom-properties.
 * @param fields RomFields object to which fields should be added.
 * @return Number of fields added, or 0 on error.
 */
int ValveVTF3::getFields(RomFields *fields) const
{
	assert(fields != nullptr);
	if (!fields)
		return 0;

	RP_D(const ValveVTF3);
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
rp_image_const_ptr ValveVTF3::image(void) const
{
	RP_D(const ValveVTF3);
	if (!d->isValid) {
		// Unknown file type.
		return nullptr;
	}

	// Load the image.
	return const_cast<ValveVTF3Private*>(d)->loadImage();
}

}
