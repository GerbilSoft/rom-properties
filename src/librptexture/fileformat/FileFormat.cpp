/***************************************************************************
 * ROM Properties Page shell extension. (librptexture)                     *
 * FileFormat.cpp: Texture file format base class.                         *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "FileFormat.hpp"
#include "FileFormat_p.hpp"

#include "libi18n/i18n.h"

// Other rom-properties libraries
using namespace LibRpFile;

namespace LibRpTexture {

/** FileFormatPrivate **/

/**
 * Initialize a FileFormatPrivate storage class.
 *
 * @param q FileFormat class
 * @param file Texture file
 * @param pTextureInfo FileFormat subclass information
 */
FileFormatPrivate::FileFormatPrivate(FileFormat *q, const IRpFilePtr &file, const TextureInfo *pTextureInfo)
	: q_ptr(q)
	, isValid(false)
	, file(file)
	, pTextureInfo(pTextureInfo)
	, mimeType(nullptr)
	, textureFormatName(nullptr)
	, mipmapCount(-1)
{
	assert(pTextureInfo != nullptr);

	// Clear the arrays.
	dimensions.fill(0);
	rescale_dimensions.fill(0);

	// Initialize i18n.
	rp_i18n_init();
}

/** FileFormat **/

FileFormat::FileFormat(FileFormatPrivate *d)
	: d_ptr(d)
{ }

FileFormat::~FileFormat()
{
	delete d_ptr;
}

/**
 * Is this ROM valid?
 * @return True if it is; false if it isn't.
 */
bool FileFormat::isValid(void) const
{
	RP_D(const FileFormat);
	return d->isValid;
}

/**
 * Is the file open?
 * @return True if the file is open; false if it isn't.
 */
bool FileFormat::isOpen(void) const
{
	RP_D(const FileFormat);
	return (d->file != nullptr);
}

/**
 * Close the opened file.
 */
void FileFormat::close(void)
{
	// Unreference the file.
	RP_D(FileFormat);
	d->file.reset();
}

/** Property accessors **/

/**
 * Get the texture format name.
 * @return Texture format name, or nullptr on error.
 */
const char *FileFormat::textureFormatName(void) const
{
	RP_D(const FileFormat);
	return d->textureFormatName;
}

/**
 * Get the file's MIME type.
 * @return MIME type, or nullptr if unknown.
 */
const char *FileFormat::mimeType(void) const
{
	RP_D(const FileFormat);
	return d->mimeType;
}

/**
 * Get the image width.
 * @return Image width.
 */
int FileFormat::width(void) const
{
	RP_D(const FileFormat);
	return d->dimensions[0];
}

/**
 * Get the image height.
 * @return Image height.
 */
int FileFormat::height(void) const
{
	RP_D(const FileFormat);
	return d->dimensions[1];
}

/**
 * Get the image dimensions.
 * If the image is 2D, then 'z' will be set to zero.
 * @param pBuf Three-element array for [x, y, z].
 * @return 0 on success; negative POSIX error code on error.
 */
int FileFormat::getDimensions(int pBuf[3]) const
{
	RP_D(const FileFormat);
	if (!d->isValid) {
		// Not supported.
		return -EBADF;
	}

	memcpy(pBuf, d->dimensions.data(), d->dimensions.size() * sizeof(int));
	return 0;
}

/**
 * Get the image rescale dimensions.
 *
 * This is for e.g. ETC2 textures that are stored as
 * a power-of-2 size but should be rendered with a
 * smaller size.
 *
 * @param pBuf Two-element array for [x, y].
 * @return 0 on success; -ENOENT if no rescale dimensions; negative POSIX error code on error.
 */
int FileFormat::getRescaleDimensions(int pBuf[2]) const
{
	RP_D(const FileFormat);
	if (!d->isValid) {
		// Not supported.
		return -EBADF;
	}

	if (d->rescale_dimensions[0] == 0 || d->rescale_dimensions[1] == 0) {
		// No rescale dimensions.
		return -ENOENT;
	}

	memcpy(pBuf, d->rescale_dimensions.data(), d->rescale_dimensions.size() * sizeof(int));
	return 0;
}

/**
 * Get the mipmap count.
 * @return Number of mipmaps. (0 if none; -1 if format doesn't support mipmaps)
 */
int FileFormat::mipmapCount(void) const
{
	RP_D(const FileFormat);
	return d->mipmapCount;
}

/**
 * Get the image for the specified mipmap.
 * Mipmap 0 is the largest image.
 * @param mip Mipmap number.
 * @return Image, or nullptr on error.
 */
rp_image_const_ptr FileFormat::mipmap(int mip) const
{
	// The base implementation doesn't support mipmaps.
	// Mipmap 0 is the same as the main image, so we'll allow that.
	if (mip == 0) {
		return image();
	}
	return nullptr;
}

}
