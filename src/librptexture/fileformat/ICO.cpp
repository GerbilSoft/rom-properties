/***************************************************************************
 * ROM Properties Page shell extension. (librptexture)                     *
 * ICO.cpp: Windows icon and cursor image reader.                          *
 *                                                                         *
 * Copyright (c) 2017-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "ICO.hpp"
#include "FileFormat_p.hpp"

#include "ico_structs.h"

// Other rom-properties libraries
#include "libi18n/i18n.h"
using namespace LibRpFile;
using LibRpBase::RomFields;

// librptexture
#include "img/rp_image.hpp"
#include "decoder/ImageDecoder_Linear.hpp"
#include "decoder/ImageDecoder_Linear_Gray.hpp"

// C++ STL classes
using std::array;

// Uninitialized vector class
#include "uvector.h"

namespace LibRpTexture {

class ICOPrivate final : public FileFormatPrivate
{
public:
	ICOPrivate(ICO *q, const IRpFilePtr &file);

private:
	typedef FileFormatPrivate super;
	RP_DISABLE_COPY(ICOPrivate)

public:
	/** TextureInfo **/
	static const array<const char*, 2+1> exts;
	static const array<const char*, 9+1> mimeTypes;
	static const TextureInfo textureInfo;

public:
	// Icon type
	enum class IconType {
		Unknown	= -1,

		Icon_Win1 = 0,
		Cursor_Win1 = 1,

		Icon_Win3 = 2,
		Cursor_Win3 = 3,

		Max
	};
	IconType iconType;

	// ICO header
	union {
		ICO_Win1_Header win1;
		ICONHEADER win3;
	} icoHeader;

	// Icon directory (Win3.x)
	rp::uvector<ICONDIRENTRY> iconDirectory;

	// "Best" icon in the icon directory
	const ICONDIRENTRY *pBestIcon;

	// Decoded image
	rp_image_ptr img;

public:
	/**
	 * Load the icon directory. (Windows 3.x)
	 * This function will also select the "best" icon to use.
	 * @return 0 on success; negative POSIX error code on error.
	 */
	int loadIconDirectory_Win3(void);

private:
	/**
	 * Load the image. (Windows 1.0 icon format)
	 * @return Image, or nullptr on error.
	 */
	rp_image_const_ptr loadImage_Win1(void);

	/**
	 * Load the image. (Windows 3.x icon format)
	 * @return Image, or nullptr on error.
	 */
	rp_image_const_ptr loadImage_Win3(void);

public:
	/**
	 * Load the image.
	 * @return Image, or nullptr on error.
	 */
	rp_image_const_ptr loadImage(void);
};

FILEFORMAT_IMPL(ICO)

/** ICOPrivate **/

/* TextureInfo */
const array<const char*, 2+1> ICOPrivate::exts = {{
	".ico",
	".cur",

	nullptr
}};
const array<const char*, 9+1> ICOPrivate::mimeTypes = {{
	// Official MIME types.
	"image/vnd.microsoft.icon",

	// Unofficial MIME types.
	"application/ico",
	"image/ico",		// NOTE: Used by Microsoft
	"image/icon",
	"image/x-ico",
	"image/x-icon",		// NOTE: Used by Microsoft
	"text/ico",

	// Unofficial MIME types.
	// TODO: Get these upstreamed on FreeDesktop.org.
	"image/vnd.microsoft.cursor",
	"image/x-cursor",

	nullptr
}};
const TextureInfo ICOPrivate::textureInfo = {
	exts.data(), mimeTypes.data()
};

ICOPrivate::ICOPrivate(ICO *q, const IRpFilePtr &file)
	: super(q, file, &textureInfo)
	, iconType(IconType::Unknown)
	, pBestIcon(nullptr)
{
	// Clear the ICO header union.
	memset(&icoHeader, 0, sizeof(icoHeader));
}

/**
 * Load the icon directory. (Windows 3.x)
 * This function will also select the "best" icon to use.
 * @return 0 on success; negative POSIX error code on error.
 */
int ICOPrivate::loadIconDirectory_Win3(void)
{
	// TODO: Windows Vista uses BITMAPINFOHEADER to select an icon.
	// Don't remember the reference for this, probably The Old New Thing...

	// Load the icon directory.
	const unsigned int count = le16_to_cpu(icoHeader.win3.idCount);
	if (count == 0) {
		// No icons???
		return -ENOENT;
	}

	const size_t fullsize = count * sizeof(ICONDIRENTRY);
	iconDirectory.resize(count);
	size_t size = file->seekAndRead(sizeof(ICONHEADER), iconDirectory.data(), fullsize);
	if (size != fullsize) {
		// Seek and/or read error.
		return -EIO;
	}

	// NOTE: Picking the first icon for now.
	// TODO: Pick the largest, then highest color depth.
	pBestIcon = &iconDirectory[0];
	return 0;
}

/**
 * Load the image. (Windows 1.0 icon format)
 * @return Image, or nullptr on error.
 */
rp_image_const_ptr ICOPrivate::loadImage_Win1(void)
{
	// Icon data is located immediately after the header.
	// Each icon is actually two icons: a 1bpp icon, then a 1bpp mask.

	// NOTE: If the file has *both* DIB and DDB, then the DIB is first,
	// followed by the DDB, with its own icon header. Not handling this
	// for now; only reading the DIB.

	const int width = le16_to_cpu(icoHeader.win1.width);
	const int height = le16_to_cpu(icoHeader.win1.height);
	const unsigned int stride = le16_to_cpu(icoHeader.win1.stride);

	// Single icon size
	const size_t icon_size = static_cast<size_t>(height) * stride;

	// Load the icon data.
	rp::uvector<uint8_t> icon_data;
	icon_data.resize(icon_size * 2);
	size_t size = file->seekAndRead(sizeof(icoHeader.win1), icon_data.data(), icon_size * 2);
	if (size != icon_size * 2) {
		// Seek and/or read error.
		return {};
	}

	// Convert the icon.
	img = ImageDecoder::fromLinearMono_WinIcon(width, height, icon_data.data(), icon_size * 2, stride);
	return img;
}

/**
 * Load the image. (Windows 3.x icon format)
 * @return Image, or nullptr on error.
 */
rp_image_const_ptr ICOPrivate::loadImage_Win3(void)
{
	// Get the icon image header.
	// May be BITMAPCOREHEADER or BITMAPINFOHEADER.
	// TODO: PNG header?

	union {
		uint32_t size;
		BITMAPCOREHEADER bch;
		BITMAPINFOHEADER bih;
	} header;
	unsigned int addr = le32_to_cpu(pBestIcon->dwImageOffset);
	size_t size = file->seekAndRead(addr, &header, sizeof(header));
	if (size != sizeof(header)) {
		// Seek and/or read error.
		return {};
	}

	// Check the header size.
	const unsigned int header_size = le32_to_cpu(header.size);
	addr += header_size;
	switch (header_size) {
		default:
			// Not supported...
			return {};

		case BITMAPCOREHEADER_SIZE:
			// TODO: Convert to BITMAPINFOHEADER.
			return {};

		case BITMAPINFOHEADER_SIZE:
		case BITMAPV2INFOHEADER_SIZE:
		case BITMAPV3INFOHEADER_SIZE:
		case BITMAPV4HEADER_SIZE:
		case BITMAPV5HEADER_SIZE:
			break;

		case 0x474E5089:	// "\x89PNG"
			// TODO: Read the PNG header.
			return {};
	}

	// NOTE: For standard icons (non-alpha, not PNG), the height is
	// actually doubled. The top of the bitmap is the icon image,
	// and the bottom is the monochrome mask.
	// NOTE 2: If height > 0, the entire bitmap is upside-down.

	// Make sure width and height are valid.
	// Height cannot be 0 or an odd number.
	// NOTE: Negative height is allowed for "right-side up".
	const unsigned int width = le32_to_cpu((unsigned int)(header.bih.biWidth));
	const int orig_height = static_cast<int>(le32_to_cpu(header.bih.biHeight));
	const bool is_upside_down = (orig_height > 0);
	const unsigned int height = abs(orig_height);
	const unsigned int half_height = height / 2;
	if (width <= 0 || height == 0 || (height & 1)) {
		// Invalid bitmap size.
		return {};
	}

	// Only supporting 16-color images for now.
	// TODO: Handle BI_BITFIELDS?
	if (le32_to_cpu(header.bih.biPlanes) > 1) {
		// Cannot handle planar bitmaps.
		return {};
	}

	// Row must be 32-bit aligned.
	// FIXME: Including for 24-bit images?
	const unsigned int bitcount = le32_to_cpu(header.bih.biBitCount);
	unsigned int stride = width;
	switch (bitcount) {
		default:
			// Unsupported bitcount.
			return {};
		case 1:
			stride /= 8;
			break;
		case 2:
			stride /= 4;
			break;
		case 4:
			stride /= 2;
			break;
		case 8:
			break;
		case 16:
			stride *= 2;
			break;
		case 24:
			stride *= 3;
			break;
		case 32:
			stride *= 4;
			break;
	}
	stride = ALIGN_BYTES(4, stride);

	// Mask row is 1bpp and must also be 32-bit aligned.
	unsigned int mask_stride = ALIGN_BYTES(4, width / 8);

	// For 8bpp or less, a color table is present.
	// NOTE: Need to manually set the alpha channel to 0xFF.
	rp::uvector<uint32_t> pal_data;
	if (bitcount <= 8) {
		const unsigned int palette_count = (1U << bitcount);
		const size_t palette_size = palette_count * sizeof(uint32_t);
		pal_data.resize(palette_count);
		size = file->seekAndRead(addr, pal_data.data(), palette_size);
		if (size != palette_size) {
			// Seek and/or read error.
			return {};
		}
		// TODO: 32-bit alignment?
		addr += palette_size;

		// Convert to host-endian and set the A channel to 0xFF.
		for (uint32_t &p : pal_data) {
			p = le32_to_cpu(p) | 0xFF000000U;
		}
	}

	// Calculate the icon, mask, and total image size.
	unsigned int icon_size = stride * half_height;
	unsigned int mask_size = mask_stride * half_height;

	size_t biSizeImage = icon_size + mask_size;
	rp::uvector<uint8_t> img_data;
	img_data.resize(biSizeImage);
	size = file->seekAndRead(addr, img_data.data(), biSizeImage);
	if (size != biSizeImage) {
		// Seek and/or read error.
		return {};
	}

	// Convert the main image first.
	// TODO: Apply the mask.
	const uint8_t *icon_data, *mask_data;
	if (is_upside_down) {
		mask_data = img_data.data();
		icon_data = mask_data;// + mask_size;
	} else {
		icon_data = img_data.data();
		mask_data = icon_data + icon_size;
	}

	switch (bitcount) {
		default:
			// Not supported yet...
			return {};

		case 4:
			img = ImageDecoder::fromLinearCI4(ImageDecoder::PixelFormat::Host_ARGB32, true,
				width, half_height,
				icon_data, icon_size,
				pal_data.data(), pal_data.size() * sizeof(uint32_t),
				stride);
			break;
	}

	if (is_upside_down) {
		rp_image_ptr flipimg = img->flip(rp_image::FLIP_V);
		if (flipimg) {
			img = std::move(flipimg);
		}
	}

	// TODO: Apply the icon mask.

	return img;
}

/**
 * Load the image.
 * @return Image, or nullptr on error.
 */
rp_image_const_ptr ICOPrivate::loadImage(void)
{
	if (img) {
		// Image has already been loaded.
		return img;
	} else if (!this->isValid || !this->file) {
		// Can't load the image.
		return {};
	}

	switch (iconType) {
		default:
			// Not supported...
			return {};

		case IconType::Icon_Win1:
		case IconType::Cursor_Win1:
			// Windows 1.0 icon or cursor
			return loadImage_Win1();

		case IconType::Icon_Win3:
		case IconType::Cursor_Win3:
			// Windows 3.x icon or cursor
			return loadImage_Win3();
	}
}

/** ICO **/

/**
 * Read a Windows icon or cursor image file.
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
ICO::ICO(const IRpFilePtr &file)
	: super(new ICOPrivate(this, file))
{
	RP_D(ICO);

	if (!d->file) {
		// Could not ref() the file handle.
		return;
	}

	// Read the ICO header.
	d->file->rewind();
	size_t size = d->file->read(&d->icoHeader, sizeof(d->icoHeader));
	if (size != sizeof(d->icoHeader)) {
		d->file.reset();
		return;
	}

	// Determine the icon type.
	switch (le16_to_cpu(d->icoHeader.win1.format)) {
		default:
			// Not recognized...
			d->file.reset();
			return;

		case ICO_WIN1_FORMAT_MAYBE_WIN3: {
			switch (le16_to_cpu(d->icoHeader.win3.idType)) {
				default:
					// Not recognized...
					d->file.reset();
					return;
				case ICO_WIN3_TYPE_ICON:
					d->iconType = ICOPrivate::IconType::Icon_Win3;
					d->mimeType = "image/vnd.microsoft.icon";
					d->textureFormatName = "Windows 3.x Icon";
					break;
				case ICO_WIN3_TYPE_CURSOR:
					d->iconType = ICOPrivate::IconType::Cursor_Win3;
					d->mimeType = "image/vnd.microsoft.cursor";
					d->textureFormatName = "Windows 3.x Icon";
					break;
			}

			// Load the icon directory and select the best image.
			int ret = d->loadIconDirectory_Win3();
			if (ret != 0) {
				// Failed to load the icon directory.
				d->file.reset();
				return;
			}
			break;
		}

		case ICO_WIN1_FORMAT_ICON_DIB:
		case ICO_WIN1_FORMAT_ICON_DDB:
		case ICO_WIN1_FORMAT_ICON_BOTH:
			d->iconType = ICOPrivate::IconType::Icon_Win1;
			// TODO: Different MIME type for Windows 1.x?
			d->mimeType = "image/vnd.microsoft.icon";
			d->textureFormatName = "Windows 1.x Icon";
			break;

		case ICO_WIN1_FORMAT_CURSOR_DIB:
		case ICO_WIN1_FORMAT_CURSOR_DDB:
		case ICO_WIN1_FORMAT_CURSOR_BOTH:
			d->iconType = ICOPrivate::IconType::Cursor_Win1;
			// TODO: Different MIME type for Windows 1.x?
			d->mimeType = "image/vnd.microsoft.cursor";
			d->textureFormatName = "Windows 1.x Cursor";
			break;
	}

	// Cache the dimensions for the FileFormat base class.
	switch (d->iconType) {
		default:
			// Shouldn't get here...
			assert(!"Invalid case!");
			d->file.reset();
			return;

		case ICOPrivate::IconType::Icon_Win1:
		case ICOPrivate::IconType::Cursor_Win1:
			d->dimensions[0] = le16_to_cpu(d->icoHeader.win1.width);
			d->dimensions[1] = le16_to_cpu(d->icoHeader.win1.height);
			break;

		case ICOPrivate::IconType::Icon_Win3:
		case ICOPrivate::IconType::Cursor_Win3:
			// TODO: Need to check BITMAPINFOHEADER, BITMAPCOREHEADER, or PNG header.
			if (!d->pBestIcon) {
				// No "best" icon...
				d->file.reset();
				return;
			}

			d->dimensions[0] = d->pBestIcon->bWidth;
			d->dimensions[1] = d->pBestIcon->bHeight;
			break;
	}

	// File is valid.
	d->isValid = true;
}

/** Property accessors **/

/**
 * Get the pixel format, e.g. "RGB888" or "DXT1".
 * @return Pixel format, or nullptr if unavailable.
 */
const char *ICO::pixelFormat(void) const
{
	RP_D(const ICO);
	if (!d->isValid) {
		return nullptr;
	}

	switch (d->iconType) {
		default:
			assert(!"Invalid icon type?");
			return nullptr;

		case ICOPrivate::IconType::Icon_Win1:
		case ICOPrivate::IconType::Cursor_Win1:
			// Windows 1.x only supports monochrome.
			return "1bpp";

		case ICOPrivate::IconType::Icon_Win3:
		case ICOPrivate::IconType::Cursor_Win3:
			// Check what the "best" icon is.
			// TODO: Check BITMAPCOREHEADER, BITMAPINFOHEADER, or the PNG header.
			return "Win3.x (TODO)";
	}
}

#ifdef ENABLE_LIBRPBASE_ROMFIELDS
/**
 * Get property fields for rom-properties.
 * @param fields RomFields object to which fields should be added.
 * @return Number of fields added, or 0 on error.
 */
int ICO::getFields(RomFields *fields) const
{
	assert(fields != nullptr);
	if (!fields)
		return 0;

	RP_D(const ICO);
	if (!d->isValid) {
		// Unknown file type.
		return -EIO;
	}

	const int initial_count = fields->count();
	fields->reserve(initial_count + 1);	// Maximum of 1 field.

	// TODO: ICO/CUR fields?
	// and "color" for Win1.x cursors

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
rp_image_const_ptr ICO::image(void) const
{
	RP_D(const ICO);
	if (!d->isValid) {
		// Unknown file type.
		return {};
	}

	// Load the image.
	return const_cast<ICOPrivate*>(d)->loadImage();
}

} // namespace LibRpTexture
