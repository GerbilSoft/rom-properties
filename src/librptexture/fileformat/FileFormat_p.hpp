/***************************************************************************
 * ROM Properties Page shell extension. (librptexture)                     *
 * FileFormat.hpp: Texture file format base class. (PRIVATE CLASS)         *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

// C++ includes
#include <memory>

namespace LibRpFile {
	class IRpFile;
}

namespace LibRpTexture {

struct TextureInfo {
	const char *const *exts;	// Supported file extensions
	const char *const *mimeTypes;	// Supported MIME types
};

class FileFormat;
class FileFormatPrivate
{
	public:
		/**
		 * Initialize a FileFormatPrivate storage class.
		 *
		 * @param q FileFormat class
		 * @param file Texture file
		 * @param pTextureInfo FileFormat subclass information
		 */
		explicit FileFormatPrivate(FileFormat *q, const LibRpFile::IRpFilePtr &file, const TextureInfo *pTextureInfo);
	public:
		virtual ~FileFormatPrivate() = default;

	private:
		RP_DISABLE_COPY(FileFormatPrivate)
	protected:
		friend class FileFormat;
		FileFormat *const q_ptr;

	public:
		volatile int ref_cnt;		// Reference count.
		bool isValid;			// Subclass must set this to true if the ROM is valid.
		LibRpFile::IRpFilePtr file;	// Open file

	public:
		/** These fields must be set by FileFormat subclasses in their constructors. **/
		const TextureInfo *pTextureInfo;// FileFormat subclass information
		const char *mimeType;		// MIME type (ASCII) (default is nullptr)
		int dimensions[3];		// Dimensions (width, height, depth)
						// 2D textures have depth=0
		int rescale_dimensions[2];	// Rescale dimensions, (width, height)
						// Needed for e.g. ETC2 where a power-of-2 size
						// is used but the image should be rescaled before
						// displaying in a UI frontend.
		int mipmapCount;		// Mipmap count (0 == none; -1 == not supported)
};

}
