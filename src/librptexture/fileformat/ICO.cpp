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

	// Decoded image
	rp_image_ptr img;

private:
	/**
	 * Load the image. (Windows 1.0 icon format)
	 * @return Image, or nullptr on error.
	 */
	rp_image_const_ptr loadImage_Win1(void);

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
{
	// Clear the ICO header union.
	memset(&icoHeader, 0, sizeof(icoHeader));
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
	}
	// TODO: Load icons.
	return {};
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

		case ICO_WIN1_FORMAT_MAYBE_WIN3:
			// Maybe a Win3 icon?
			// FIXME: Not supported right now.
			d->file.reset();
			return;

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
			break;

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

	// File is valid.
	d->isValid = true;

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
			// TODO: Need to pick the "best" icon for Win3.x.
			d->file.reset();
			return;
	}
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

	// Win1.x only supports monochrome.
	// TODO: Win3.x
	return "1bpp";
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
