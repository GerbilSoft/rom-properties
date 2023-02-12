/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * RpTextureWrapper.hpp: librptexture file format wrapper.                 *
 *                                                                         *
 * Copyright (c) 2019-2021 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "RpTextureWrapper.hpp"

// librpbase, librpfile
using namespace LibRpBase;
using LibRpFile::IRpFile;

// librptexture
#include "librptexture/FileFormatFactory.hpp"
#include "librptexture/fileformat/FileFormat.hpp"
using LibRpTexture::rp_image;
using LibRpTexture::FileFormat;
using LibRpTexture::FileFormatFactory;

// C++ STL classes.
using std::vector;

namespace LibRomData {

class RpTextureWrapperPrivate final : public RomDataPrivate
{
	public:
		RpTextureWrapperPrivate(RpTextureWrapper *q, IRpFile *file);
		~RpTextureWrapperPrivate();

	private:
		typedef RomDataPrivate super;
		RP_DISABLE_COPY(RpTextureWrapperPrivate)

	public:
		/** RomDataInfo **/
		static const char *const exts[];
		static const char *const mimeTypes[];
		static const RomDataInfo romDataInfo;

	public:
		// librptexture file format object.
		FileFormat *texture;
};

ROMDATA_IMPL(RpTextureWrapper)
ROMDATA_IMPL_IMG_TYPES(RpTextureWrapper)

/** RpTextureWrapperPrivate **/

/* RomDataInfo */
// NOTE: RomDataFactory queries extensions and MIME types from
// FileFormatFactory directly, so these aren't used.
const char *const RpTextureWrapperPrivate::exts[] = {
	nullptr
};
const char *const RpTextureWrapperPrivate::mimeTypes[] = {
	nullptr
};
const RomDataInfo RpTextureWrapperPrivate::romDataInfo = {
	"RpTextureWrapper", exts, mimeTypes
};

RpTextureWrapperPrivate::RpTextureWrapperPrivate(RpTextureWrapper *q, IRpFile *file)
	: super(q, file, &romDataInfo)
	, texture(nullptr)
{ }

RpTextureWrapperPrivate::~RpTextureWrapperPrivate()
{
	UNREF(texture);
}

/** RpTextureWrapper **/

/**
 * Read a texture file supported by librptexture.
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
RpTextureWrapper::RpTextureWrapper(IRpFile *file)
	: super(new RpTextureWrapperPrivate(this, file))
{
	// This class handles texture files.
	RP_D(RpTextureWrapper);
	d->fileType = FileType::TextureFile;

	if (!d->file) {
		// Could not ref() the file handle.
		return;
	}

	// Create a FileFormat instance.
	d->texture = FileFormatFactory::create(d->file);
	if (!d->texture) {
		// Not a valid texture.
		UNREF_AND_NULL_NOCHK(d->file);
		return;
	}

	d->mimeType = d->texture->mimeType();
	d->isValid = true;
}

/**
 * Close the opened file.
 */
void RpTextureWrapper::close(void)
{
	RP_D(RpTextureWrapper);

	// NOTE: Don't delete these. They have rp_image objects
	// that may be used by the UI later.
	if (d->texture) {
		d->texture->close();
	}

	// Call the superclass function.
	super::close();
}

/** ROM detection functions. **/

/**
 * Is a ROM image supported by this class?
 * @param info DetectInfo containing ROM detection information.
 * @return Class-specific system ID (>= 0) if supported; -1 if not.
 */
int RpTextureWrapper::isRomSupported_static(const DetectInfo *info)
{
	assert(info != nullptr);
	assert(info->header.pData != nullptr);
	assert(info->header.addr == 0);
	if (!info || !info->header.pData ||
	    info->header.addr != 0 ||
	    info->header.size < sizeof(uint32_t))
	{
		// Either no detection information was specified,
		// or the header is too small.
		return -1;
	}

	// TODO: FileFormatFactory::isTextureSupported()
	// For now, delegate to FileFormatFactory::create().
	return 0;
}

/**
 * Get the name of the system the loaded ROM is designed for.
 * @param type System name type. (See the SystemName enum.)
 * @return System name, or nullptr if type is invalid.
 */
const char *RpTextureWrapper::systemName(unsigned int type) const
{
	RP_D(const RpTextureWrapper);
	if (!d->isValid || !isSystemNameTypeValid(type))
		return nullptr;

	// TODO: Short names and whatnot from FileFormat.
	return d->texture->textureFormatName();
}

/**
 * Get a bitfield of image types this class can retrieve.
 * @return Bitfield of supported image types. (ImageTypesBF)
 */
uint32_t RpTextureWrapper::supportedImageTypes_static(void)
{
	return IMGBF_INT_IMAGE;
}

/**
 * Get a list of all available image sizes for the specified image type.
 * @param imageType Image type.
 * @return Vector of available image sizes, or empty vector if no images are available.
 */
vector<RomData::ImageSizeDef> RpTextureWrapper::supportedImageSizes(ImageType imageType) const
{
	ASSERT_supportedImageSizes(imageType);

	RP_D(const RpTextureWrapper);
	if (!d->isValid || imageType != IMG_INT_IMAGE) {
		return vector<ImageSizeDef>();
	}

	// Return the image's size.
	const ImageSizeDef imgsz[] = {{nullptr,
		static_cast<uint16_t>(d->texture->width()),
		static_cast<uint16_t>(d->texture->height()),
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
uint32_t RpTextureWrapper::imgpf(ImageType imageType) const
{
	ASSERT_imgpf(imageType);

	RP_D(const RpTextureWrapper);
	if (imageType != IMG_INT_IMAGE) {
		// Only IMG_INT_IMAGE is supported by RpTextureWrapper.
		return 0;
	}

	// If both dimensions of the texture are 64 or less,
	// specify nearest-neighbor scaling.
	uint32_t ret = 0;
	if (d->texture->width() <= 64 && d->texture->height() <= 64) {
		// 64x64 or smaller.
		ret = IMGPF_RESCALE_NEAREST;
	}

	// Are rescale dimensions specified?
	int rescale_dimensions[2];
	if (d->texture->getRescaleDimensions(rescale_dimensions) == 0) {
		ret |= IMGPF_RESCALE_RFT_DIMENSIONS_2;
	}

	return ret;
}

/**
 * Load field data.
 * Called by RomData::fields() if the field data hasn't been loaded yet.
 * @return Number of fields read on success; negative POSIX error code on error.
 */
int RpTextureWrapper::loadFieldData(void)
{
	RP_D(RpTextureWrapper);
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

	// RpTextureWrapper header
	const FileFormat *const texture = d->texture;
	d->fields->reserve(4);	// Maximum of 4 fields.

	// Dimensions
	int dimensions[3];
	int ret = texture->getDimensions(dimensions);
	if (ret == 0) {
		d->fields->addField_dimensions(C_("RpTextureWrapper", "Dimensions"),
			dimensions[0], dimensions[1], dimensions[2]);

		// Rescale dimensions (may not be present)
		// TODO: 3D rescaling?
		int rescale_dimensions[2];
		ret = texture->getRescaleDimensions(rescale_dimensions);
		if (ret == 0 && (rescale_dimensions[0] != dimensions[0] ||
		                 rescale_dimensions[1] != dimensions[1]))
		{
			d->fields->addField_dimensions(C_("RpTextureWrapper", "Rescale To"),
				rescale_dimensions[0], rescale_dimensions[1]);
		}
	}

	// Pixel format
	// NOTE: Godot 3 textures with embedded PNG/WebP doesn't
	// have the pixel format field set. We could decode the
	// image to find it, but that would be slow.
	const char *const pixelFormat = texture->pixelFormat();
	if (pixelFormat) {
		d->fields->addField_string(C_("RpTextureWrapper", "Pixel Format"), pixelFormat);
	}

	// Mipmap count
	int mipmapCount = texture->mipmapCount();
	if (mipmapCount >= 0) {
		d->fields->addField_string_numeric(C_("RpTextureWrapper", "Mipmap Count"), mipmapCount);
	}

	// Texture-specific fields.
	texture->getFields(d->fields);

	// TODO: More fields.

	// Finished reading the field data.
	return static_cast<int>(d->fields->count());
}

/**
 * Load metadata properties.
 * Called by RomData::metaData() if the field data hasn't been loaded yet.
 * @return Number of metadata properties read on success; negative POSIX error code on error.
 */
int RpTextureWrapper::loadMetaData(void)
{
	RP_D(RpTextureWrapper);
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

	// Dimensions
	int dimensions[3];
	int ret = d->texture->getDimensions(dimensions);
	if (ret == 0) {
		if (dimensions[0] > 0) {
			d->metaData->addMetaData_integer(Property::Width, dimensions[0]);
		}
		if (dimensions[1] > 0) {
			d->metaData->addMetaData_integer(Property::Height, dimensions[1]);
		}
	}

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
int RpTextureWrapper::loadInternalImage(ImageType imageType, const rp_image **pImage)
{
	ASSERT_loadInternalImage(imageType, pImage);
	RP_D(RpTextureWrapper);
	ROMDATA_loadInternalImage_single(
		IMG_INT_IMAGE,		// ourImageType
		d->file,		// file
		d->isValid,		// isValid
		0,			// romType
		nullptr,		// imgCache
		d->texture->image);	// func
}

/**
 * Get the pixel format, e.g. "RGB888" or "DXT1".
 * @return Pixel format, or nullptr if unavailable.
 */
const char *RpTextureWrapper::pixelFormat(void) const
{
	RP_D(const RpTextureWrapper);
	if (d->texture) {
		return d->texture->pixelFormat();
	}
	return nullptr;
}

}
