/***************************************************************************
 * ROM Properties Page shell extension. (librptexture)                     *
 * FileFormat.hpp: Texture file format base class.                         *
 *                                                                         *
 * Copyright (c) 2016-2020 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_LIBRPTEXTURE_FILEFORMAT_FILEFORMAT_HPP__
#define __ROMPROPERTIES_LIBRPTEXTURE_FILEFORMAT_FILEFORMAT_HPP__

#include "librptexture/config.librptexture.h"

// TODO: Move to librpfile or similar?
#include "librpbase/common.h"

// Common declarations.
#include "FileFormat_decl.hpp"

#ifdef ENABLE_LIBRPBASE_ROMFIELDS
namespace LibRpBase {
	class RomFields;
}
#endif /* ENABLE_LIBRPBASE_ROMFIELDS */

// C includes.
#include <stdint.h>

namespace LibRpTexture {

class rp_image;

class FileFormatPrivate;
class FileFormat
{
	protected:
		explicit FileFormat(FileFormatPrivate *d);

	protected:
		/**
		 * FileFormat destructor is protected.
		 * Use unref() instead.
		 */
		virtual ~FileFormat();

	private:
		RP_DISABLE_COPY(FileFormat)
	protected:
		friend class FileFormatPrivate;
		FileFormatPrivate *const d_ptr;

	public:
		/**
		 * Take a reference to this FileFormat* object.
		 * @return this
		 */
		FileFormat *ref(void);

		/**
		 * Unreference this FileFormat* object.
		 * If ref_cnt reaches 0, the FileFormat* object is deleted.
		 */
		void unref(void);

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
		virtual const char *const *supportedFileExtensions(void) const = 0;

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
		virtual const char *const *supportedMimeTypes(void) const = 0;

	public:
		/** Property accessors **/

		/**
		 * Get the texture format name.
		 * @return Texture format name, or nullptr on error.
		 */
		virtual const char *textureFormatName(void) const = 0;

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
		virtual const rp_image *image(void) const = 0;

		/**
		 * Get the image for the specified mipmap.
		 * Mipmap 0 is the largest image.
		 * @param mip Mipmap number.
		 * @return Image, or nullptr on error.
		 */
		virtual const rp_image *mipmap(int mip) const = 0;
};

}

#endif /* __ROMPROPERTIES_LIBRPTEXTURE_FILEFORMAT_FILEFORMAT_HPP__ */
