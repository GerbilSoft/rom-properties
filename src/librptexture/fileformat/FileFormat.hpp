/***************************************************************************
 * ROM Properties Page shell extension. (librptexture)                     *
 * FileFormat.hpp: Texture file format base class.                         *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once
#define __ROMPROPERTIES_LIBRPTEXTURE_FILEFORMAT_FILEFORMAT_HPP__

#include "librptexture/config.librptexture.h"

// C includes
#include <stdint.h>
#include <sys/types.h>	// for off64_t

// C++ includes
#include <memory>

// Common macros
#include "common.h"

// Common declarations
#include "FileFormat_decl.hpp"

#ifdef ENABLE_LIBRPBASE_ROMFIELDS
namespace LibRpBase {
	class RomFields;
}
#endif /* ENABLE_LIBRPBASE_ROMFIELDS */

// librptexture
#include "../img/rp_image.hpp"

namespace LibRpTexture {

class FileFormatPrivate;
class FileFormat
{
protected:
	explicit FileFormat(FileFormatPrivate *d);

public:
	virtual ~FileFormat();

private:
	RP_DISABLE_COPY(FileFormat)
protected:
	friend class FileFormatPrivate;
	FileFormatPrivate *const d_ptr;

public:
	/**
	 * Is the texture file valid?
	 * @return True if valid; false if not.
	 */
	bool isValid(void) const;

	/**
	 * Is the texture file open?
	 * @return True if open; false if not.
	 */
	bool isOpen(void) const;

	/**
	 * Close the opened file.
	 */
	virtual void close(void);

public:
	/**
	 * ROM detection information.
	 * Used for isRomSupported() functions.
	 */
	struct DetectInfo {
		struct {
			uint32_t addr;		// Start address in the ROM.
			uint32_t size;		// Length.
			const uint8_t *pData;	// Data.
		} header;		// ROM header.
		const char *ext;	// File extension, including leading '.'
		off64_t szFile;		// File size. (Required for certain types.)
	};

public:
	/** Property accessors **/

	/**
	 * Get the texture format name.
	 * @return Texture format name, or nullptr on error.
	 */
	virtual const char *textureFormatName(void) const = 0;

	/**
	 * Get the file's MIME type.
	 * @return MIME type, or nullptr if unknown.
	 */
	const char *mimeType(void) const;

	// TODO: Supported file extensions and MIME types.

	/**
	 * Get the image width.
	 * @return Image width.
	 */
	int width(void) const;

	/**
	 * Get the image height.
	 * @return Image height.
	 */
	int height(void) const;

	/**
	 * Get the image dimensions.
	 * If the image is 2D, then 'z' will be set to zero.
	 * @param pBuf Three-element array for [x, y, z].
	 * @return 0 on success; negative POSIX error code on error.
	 */
	int getDimensions(int pBuf[3]) const;

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
	int getRescaleDimensions(int pBuf[2]) const;

	/**
	 * Get the pixel format, e.g. "RGB888" or "DXT1".
	 * @return Pixel format, or nullptr if unavailable.
	 */
	virtual const char *pixelFormat(void) const = 0;

	/**
	 * Get the mipmap count.
	 * @return Number of mipmaps. (0 if none; -1 if format doesn't support mipmaps)
	 */
	virtual int mipmapCount(void) const = 0;

#ifdef ENABLE_LIBRPBASE_ROMFIELDS
public:
	/**
	 * Get property fields for rom-properties.
	 * @param fields RomFields object to which fields should be added.
	 * @return Number of fields added, or 0 on error.
	 */
	virtual int getFields(LibRpBase::RomFields *fields) const = 0;
#endif /* ENABLE_LIBRPBASE_ROMFIELDS */

public:
	/** Image accessors **/

	/**
	 * Get the image.
	 * For textures with mipmaps, this is the largest mipmap.
	 * The image is owned by this object.
	 * @return Image, or nullptr on error.
	 */
	virtual rp_image_const_ptr image(void) const = 0;

	/**
	 * Get the image for the specified mipmap.
	 * Mipmap 0 is the largest image.
	 * @param mip Mipmap number.
	 * @return Image, or nullptr on error.
	 */
	virtual rp_image_const_ptr mipmap(int mip) const = 0;
};

typedef std::shared_ptr<FileFormat> FileFormatPtr;
typedef std::shared_ptr<const FileFormat> FileFormatConstPtr;

} //namespace LibRpTexture
