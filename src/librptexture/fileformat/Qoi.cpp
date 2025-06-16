/***************************************************************************
 * ROM Properties Page shell extension. (librptexture)                     *
 * Qoi.cpp: Quite OK Image Format image reader.                            *
 *                                                                         *
 * Copyright (c) 2017-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "Qoi.hpp"
#include "FileFormat_p.hpp"

// Other rom-properties libraries
#include "libi18n/i18n.h"

#define QOI_IMPLEMENTATION 1
#define QOI_NO_STDIO 1
#include "decoder/qoi.h"

// Other rom-properties libraries
using namespace LibRpFile;
using LibRpBase::RomFields;

// C++ STL classes
using std::array;

namespace LibRpTexture {

// Qoi header, with magic number.
// NOTE: qoi_desc should be 14 bytes, but it's padded.
// qoi.h doesn't load the struct directly, so this isn't an issue.
#define QOI_MAGIC_NUMBER 'qoif'
struct QoiHeader {
	uint32_t magic;
	qoi_desc desc;
};
ASSERT_STRUCT(QoiHeader, 16);

class QoiPrivate final : public FileFormatPrivate
{
public:
	QoiPrivate(Qoi *q, const IRpFilePtr &file);

private:
	typedef FileFormatPrivate super;
	RP_DISABLE_COPY(QoiPrivate)

public:
	/** TextureInfo **/
	static const array<const char*, 1+1> exts;
	static const array<const char*, 2+1> mimeTypes;
	static const TextureInfo textureInfo;

public:
	// Qoi header
	QoiHeader qoiHeader;

	// Decoded image
	rp_image_ptr img;

	/**
	 * Load the image.
	 * @return Image, or nullptr on error.
	 */
	rp_image_const_ptr loadImage(void);
};

FILEFORMAT_IMPL(Qoi)

/** QoiPrivate **/

/* TextureInfo */
const array<const char*, 1+1> QoiPrivate::exts = {{
	".qoi",

	nullptr
}};
const array<const char*, 2+1> QoiPrivate::mimeTypes = {{
	// Unofficial MIME types.
	// TODO: Get these upstreamed on FreeDesktop.org.
	"image/x-qoi",

	// Official MIME types. (Not registered yet!)
	"image/qoi",

	nullptr
}};
const TextureInfo QoiPrivate::textureInfo = {
	exts.data(), mimeTypes.data()
};

QoiPrivate::QoiPrivate(Qoi *q, const IRpFilePtr &file)
	: super(q, file, &textureInfo)
{
	// Clear the Qoi header struct.
	memset(&qoiHeader, 0, sizeof(qoiHeader));
}

/**
 * Load the image.
 * @return Image, or nullptr on error.
 */
rp_image_const_ptr QoiPrivate::loadImage(void)
{
	if (img) {
		// Image has already been loaded.
		return img;
	} else if (!this->isValid || !this->file) {
		// Can't load the image.
		return {};
	}

	// Sanity check: Maximum image dimensions of 32768x32768.
	assert(qoiHeader.desc.width > 0);
	assert(qoiHeader.desc.width <= 32768);
	assert(qoiHeader.desc.height > 0);
	assert(qoiHeader.desc.height <= 32768);
	if (qoiHeader.desc.width == 0 || qoiHeader.desc.width > 32768 ||
	    qoiHeader.desc.height == 0 || qoiHeader.desc.height > 32768)
	{
		// Invalid image dimensions.
		return {};
	}

	if (file->size() > 128*1024*1024) {
		// Sanity check: Qoi files shouldn't be more than 128 MB.
		return {};
	}
	const uint32_t file_sz = static_cast<uint32_t>(file->size());

	// Qoi stores either 24-bit RGB or 32-bit RGBA image data.
	// We want to read it in as 32-bit RGBA.

	// Read in the entire file. (TODO: mmap?)
	auto buf = aligned_uptr<uint8_t>(16, file_sz);
	size_t size = file->seekAndRead(0, buf.get(), file_sz);
	if (size != file_sz) {
		// Seek and/or read error.
		return {};
	}

	// Decode the image.
	qoi_desc tmp_desc;
	void *pixels = qoi_decode(buf.get(), file_sz, &tmp_desc, 4);
	if (!pixels) {
		// Error decoding the image.
		return {};
	}
	buf.reset();

	// Copy the decoded image into an rp_image.
	rp_image *const tmp_img = new rp_image(qoiHeader.desc.width, qoiHeader.desc.height, rp_image::Format::ARGB32);
	uint32_t *px_dest = static_cast<uint32_t*>(tmp_img->bits());
	const uint32_t *px_src = static_cast<uint32_t*>(pixels);
	const int src_stride = qoiHeader.desc.width;
	const int src_bytes = src_stride * sizeof(*px_src);
	const int dest_stride = tmp_img->stride() / sizeof(*px_dest);

	for (unsigned int y = qoiHeader.desc.height; y > 0; y--) {
		memcpy(px_dest, px_src, src_bytes);
		px_dest += dest_stride;
		px_src += src_stride;
	}
	free(pixels);

	img.reset(tmp_img);
	return img;
}

/** Qoi **/

/**
 * Read a Quite OK Image Format image file.
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
Qoi::Qoi(const IRpFilePtr &file)
	: super(new QoiPrivate(this, file))
{
	RP_D(Qoi);
	d->mimeType = "image/x-qoi";	// unofficial, not on fd.o
	d->textureFormatName = "Quite OK Image Format";

	if (!d->file) {
		// Could not ref() the file handle.
		return;
	}

	// Read the Qoi header.
	d->file->rewind();
	size_t size = d->file->read(&d->qoiHeader, sizeof(d->qoiHeader));
	if (size != sizeof(d->qoiHeader)) {
		d->file.reset();
		return;
	}

	// Verify the Qoi magic.
	if (d->qoiHeader.magic != cpu_to_be32(QOI_MAGIC_NUMBER)) {
		// Incorrect magic.
		d->file.reset();
		return;
	}

	// File is valid.
	d->isValid = true;

#if SYS_BYTEORDER == SYS_LIL_ENDIAN
	// Header is stored in big-endian, so it always
	// needs to be byteswapped on little-endian.
	// NOTE: Magic number is *not* byteswapped.
	d->qoiHeader.desc.width		= be32_to_cpu(d->qoiHeader.desc.width);
	d->qoiHeader.desc.height	= be32_to_cpu(d->qoiHeader.desc.height);
#endif

	// Cache the dimensions for the FileFormat base class.
	d->dimensions[0] = d->qoiHeader.desc.width;
	d->dimensions[1] = d->qoiHeader.desc.height;
}

/** Property accessors **/

/**
 * Get the pixel format, e.g. "RGB888" or "DXT1".
 * @return Pixel format, or nullptr if unavailable.
 */
const char *Qoi::pixelFormat(void) const
{
	RP_D(const Qoi);
	if (!d->isValid) {
		return nullptr;
	}

	return (d->qoiHeader.desc.channels == 3) ? "RGB888" : "ARGB32";
}

#ifdef ENABLE_LIBRPBASE_ROMFIELDS
/**
 * Get property fields for rom-properties.
 * @param fields RomFields object to which fields should be added.
 * @return Number of fields added, or 0 on error.
 */
int Qoi::getFields(RomFields *fields) const
{
	assert(fields != nullptr);
	if (!fields)
		return 0;

	RP_D(const Qoi);
	if (!d->isValid) {
		// Unknown file type.
		return -EIO;
	}

	const int initial_count = fields->count();
	fields->reserve(initial_count + 1);	// Maximum of 1 field.

	// Qoi description struct
	const qoi_desc *const desc = &d->qoiHeader.desc;

	// Colorspace
	const char *colorspace;
	switch (desc->colorspace) {
		default:
			colorspace = nullptr;
			break;
		case 0:
			colorspace = C_("Qoi|Colorspace", "sRGB with linear alpha");
			break;
		case 1:
			colorspace = C_("Qoi|Colorspace", "all channels linear");
			break;
	}
	if (colorspace) {
		fields->addField_string(C_("Qoi", "Colorspace"), colorspace);
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
rp_image_const_ptr Qoi::image(void) const
{
	RP_D(const Qoi);
	if (!d->isValid) {
		// Unknown file type.
		return {};
	}

	// Load the image.
	return const_cast<QoiPrivate*>(d)->loadImage();
}

} // namespace LibRpTexture
