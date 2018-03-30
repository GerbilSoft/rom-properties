/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * ValveVTF3.hpp: Valve VTF3 (PS3) image reader.                           *
 *                                                                         *
 * Copyright (c) 2017-2018 by David Korth.                                 *
 *                                                                         *
 * This program is free software; you can redistribute it and/or modify it *
 * under the terms of the GNU General Public License as published by the   *
 * Free Software Foundation; either version 2 of the License, or (at your  *
 * option) any later version.                                              *
 *                                                                         *
 * This program is distributed in the hope that it will be useful, but     *
 * WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 * GNU General Public License for more details.                            *
 *                                                                         *
 * You should have received a copy of the GNU General Public License       *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.   *
 ***************************************************************************/

#include "ValveVTF3.hpp"
#include "librpbase/RomData_p.hpp"

#include "vtf3_structs.h"

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
#include <cerrno>
#include <cstring>

// C++ includes.
#include <memory>
#include <string>
#include <vector>
using std::string;
using std::unique_ptr;
using std::vector;

namespace LibRomData {

ROMDATA_IMPL(ValveVTF3)
ROMDATA_IMPL_IMG_TYPES(ValveVTF3)

class ValveVTF3Private : public RomDataPrivate
{
	public:
		ValveVTF3Private(ValveVTF3 *q, IRpFile *file);
		~ValveVTF3Private();

	private:
		typedef RomDataPrivate super;
		RP_DISABLE_COPY(ValveVTF3Private)

	public:
		// VTF3 header.
		VTF3HEADER vtf3Header;

		// Decoded image.
		rp_image *img;

		/**
		 * Load the image.
		 * @return Image, or nullptr on error.
		 */
		const rp_image *loadImage(void);

#if SYS_BYTEORDER == SYS_BIG_ENDIAN
		/**
		 * Byteswap a float. (TODO: Move to byteswap.h?)
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
#endif
};

/** ValveVTF3Private **/

ValveVTF3Private::ValveVTF3Private(ValveVTF3 *q, IRpFile *file)
	: super(q, file)
	, img(nullptr)
{
	// Clear the VTF3 header struct.
	memset(&vtf3Header, 0, sizeof(vtf3Header));
}

ValveVTF3Private::~ValveVTF3Private()
{
	delete img;
}

/**
 * Load the image.
 * @return Image, or nullptr on error.
 */
const rp_image *ValveVTF3Private::loadImage(void)
{
	if (img) {
		// Image has already been loaded.
		return img;
	} else if (!this->file || !this->isValid) {
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
	const uint32_t file_sz = (uint32_t)file->size();

	// Handle a 1D texture as a "width x 1" 2D texture.
	// NOTE: Handling a 3D texture as a single 2D texture.
	const int height = (vtf3Header.height > 0 ? vtf3Header.height : 1);

	// Calculate the expected size.
	unsigned int expected_size = vtf3Header.width * height;
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
	const unsigned int texDataStartAddr = file_sz - expected_size;

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
	// TODO: unique_ptr<> helper that uses aligned_malloc() and aligned_free()?
	uint8_t *const buf = static_cast<uint8_t*>(aligned_malloc(16, expected_size));
	if (!buf) {
		// Memory allocation failure.
		return nullptr;
	}
	size_t size = file->read(buf, expected_size);
	if (size != expected_size) {
		// Read error.
		aligned_free(buf);
		return nullptr;
	}

	// Decode the image.
	if (vtf3Header.flags & VTF3_FLAG_ALPHA) {
		// Image has an alpha channel.
		// Encoded using DXT5.
		img = ImageDecoder::fromDXT5(
			vtf3Header.width, height,
			buf, expected_size);
	} else {
		// Image does not have an alpha channel.
		// Encoded using DXT1.
		img = ImageDecoder::fromDXT1(
			vtf3Header.width, height,
			buf, expected_size);
	}

	aligned_free(buf);
	return img;
}

/** ValveVTF3 **/

/**
 * Read a Valve VTF3 (PS3) image file.
 *
 * A ROM image must be opened by the caller. The file handle
 * will be dup()'d and must be kept open in order to load
 * data from the ROM image.
 *
 * To close the file, either delete this object or call close().
 *
 * NOTE: Check isValid() to determine if this is a valid ROM.
 *
 * @param file Open ROM image.
 */
ValveVTF3::ValveVTF3(IRpFile *file)
	: super(new ValveVTF3Private(this, file))
{
	// This class handles texture files.
	RP_D(ValveVTF3);
	d->className = "ValveVTF3";
	d->fileType = FTYPE_TEXTURE_FILE;

	if (!d->file) {
		// Could not dup() the file handle.
		return;
	}

	// Read the VTF3 header.
	d->file->rewind();
	size_t size = d->file->read(&d->vtf3Header, sizeof(d->vtf3Header));
	if (size != sizeof(d->vtf3Header))
		return;

	// Check if this VTF3 texture is supported.
	DetectInfo info;
	info.header.addr = 0;
	info.header.size = (uint32_t)size;
	info.header.pData = reinterpret_cast<const uint8_t*>(&d->vtf3Header);
	info.ext = nullptr;	// Not needed for VTF3.
	info.szFile = file->size();
	d->isValid = (isRomSupported_static(&info) >= 0);

	if (d->isValid) {
#if SYS_BYTEORDER == SYS_LIL_ENDIAN
		// Header is stored in big-endian, so it always
		// needs to be byteswapped on little-endian.
		d->vtf3Header.signature		= be32_to_cpu(d->vtf3Header.signature);
		d->vtf3Header.flags		= be32_to_cpu(d->vtf3Header.flags);
		d->vtf3Header.width		= be16_to_cpu(d->vtf3Header.width);
		d->vtf3Header.height		= be16_to_cpu(d->vtf3Header.height);
#endif
	}
}

/**
 * Is a ROM image supported by this class?
 * @param info DetectInfo containing ROM detection information.
 * @return Class-specific system ID (>= 0) if supported; -1 if not.
 */
int ValveVTF3::isRomSupported_static(const DetectInfo *info)
{
	assert(info != nullptr);
	assert(info->header.pData != nullptr);
	assert(info->header.addr == 0);
	if (!info || !info->header.pData ||
	    info->header.addr != 0 ||
	    info->header.size < sizeof(VTF3HEADER))
	{
		// Either no detection information was specified,
		// or the header is too small.
		return -1;
	}

	// Verify the VTF3 signature.
	const VTF3HEADER *const vtf3Header =
		reinterpret_cast<const VTF3HEADER*>(info->header.pData);
	if (vtf3Header->signature == be32_to_cpu(VTF3_SIGNATURE)) {
		// VTF3 signature is correct.
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
const char *ValveVTF3::systemName(unsigned int type) const
{
	RP_D(const ValveVTF3);
	if (!d->isValid || !isSystemNameTypeValid(type))
		return nullptr;

	// Bits 0-1: Type. (short, long, abbreviation)
	static const char *const sysNames[4] = {
		"Valve VTF3 Texture (PS3)", "Valve VTF3", "VTF3", nullptr
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
const char *const *ValveVTF3::supportedFileExtensions_static(void)
{
	static const char *const exts[] = {
		".vtf",
		//".vtx",	// TODO: Some files might use the ".vtx" extension.
		nullptr
	};
	return exts;
}

/**
 * Get a bitfield of image types this class can retrieve.
 * @return Bitfield of supported image types. (ImageTypesBF)
 */
uint32_t ValveVTF3::supportedImageTypes_static(void)
{
	return IMGBF_INT_IMAGE;
}

/**
 * Get a list of all available image sizes for the specified image type.
 * @param imageType Image type.
 * @return Vector of available image sizes, or empty vector if no images are available.
 */
vector<RomData::ImageSizeDef> ValveVTF3::supportedImageSizes(ImageType imageType) const
{
	assert(imageType >= IMG_INT_MIN && imageType <= IMG_EXT_MAX);
	if (imageType < IMG_INT_MIN || imageType > IMG_EXT_MAX) {
		// ImageType is out of range.
		return vector<ImageSizeDef>();
	}

	RP_D(ValveVTF3);
	if (!d->isValid || imageType != IMG_INT_IMAGE) {
		return vector<ImageSizeDef>();
	}

	// Return the image's size.
	const ImageSizeDef imgsz[] = {{nullptr,
		d->vtf3Header.width,
		d->vtf3Header.height, 0}};
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
uint32_t ValveVTF3::imgpf(ImageType imageType) const
{
	assert(imageType >= IMG_INT_MIN && imageType <= IMG_EXT_MAX);
	if (imageType < IMG_INT_MIN || imageType > IMG_EXT_MAX) {
		// ImageType is out of range.
		return 0;
	}

	RP_D(ValveVTF3);
	if (imageType != IMG_INT_IMAGE) {
		// Only IMG_INT_IMAGE is supported by DDS.
		return 0;
	}

	// If both dimensions of the texture are 64 or less,
	// specify nearest-neighbor scaling.
	uint32_t ret = 0;
	if (d->vtf3Header.width <= 64 && d->vtf3Header.height <= 64) {
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
int ValveVTF3::loadFieldData(void)
{
	RP_D(ValveVTF3);
	if (d->fields->isDataLoaded()) {
		// Field data *has* been loaded...
		return 0;
	} else if (!d->file) {
		// File isn't open.
		return -EBADF;
	} else if (!d->isValid) {
		// Unknown file type.
		return -EIO;
	}

	// VTF3 header.
	const VTF3HEADER *const vtf3Header = &d->vtf3Header;
	d->fields->reserve(2);	// Maximum of 2 fields.

	// TODO: More fields.

	// Texture size.
	// TODO: 3D textures?
	if (vtf3Header->height > 0) {
		// TODO: >0 or >1?
		d->fields->addField_string(C_("ValveVTF3", "Texture Size"),
			rp_sprintf("%ux%u", vtf3Header->width,
				vtf3Header->height));
	} else {
		d->fields->addField_string_numeric(C_("ValveVTF3", "Texture Size"), vtf3Header->width);
	}

	// Image format.
	d->fields->addField_string(C_("ValveVTF3", "Image Format"),
		((vtf3Header->flags & VTF3_FLAG_ALPHA) ? "DXT5" : "DXT1"));

	// TODO: Flags.

	// Finished reading the field data.
	return (int)d->fields->count();
}

/**
 * Load an internal image.
 * Called by RomData::image().
 * @param imageType	[in] Image type to load.
 * @param pImage	[out] Pointer to const rp_image* to store the image in.
 * @return 0 on success; negative POSIX error code on error.
 */
int ValveVTF3::loadInternalImage(ImageType imageType, const rp_image **pImage)
{
	assert(imageType >= IMG_INT_MIN && imageType <= IMG_INT_MAX);
	assert(pImage != nullptr);
	if (!pImage) {
		// Invalid parameters.
		return -EINVAL;
	} else if (imageType < IMG_INT_MIN || imageType > IMG_INT_MAX) {
		// ImageType is out of range.
		*pImage = nullptr;
		return -ERANGE;
	}

	RP_D(ValveVTF3);
	if (imageType != IMG_INT_IMAGE) {
		// Only IMG_INT_IMAGE is supported by DDS.
		*pImage = nullptr;
		return -ENOENT;
	} else if (!d->file) {
		// File isn't open.
		*pImage = nullptr;
		return -EBADF;
	} else if (!d->isValid) {
		// DDS texture isn't valid.
		*pImage = nullptr;
		return -EIO;
	}

	// Load the image.
	*pImage = d->loadImage();
	return (*pImage != nullptr ? 0 : -EIO);
}

}
