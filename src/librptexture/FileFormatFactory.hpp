/***************************************************************************
 * ROM Properties Page shell extension. (librptexture)                     *
 * FileFormatFactory.hpp: FileFormat factory class.                        *
 *                                                                         *
 * Copyright (c) 2016-2018 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_LIBRPTEXTURE_FILEFORMATFACTORY_HPP__
#define __ROMPROPERTIES_LIBRPTEXTURE_FILEFORMATFACTORY_HPP__

#include "librpbase/common.h"

// C++ includes.
#include <utility>
#include <vector>

namespace LibRpBase {
	class IRpFile;
}

namespace LibRpTexture {

class FileFormat;
class FileFormatFactory
{
	private:
		FileFormatFactory();
		~FileFormatFactory();
	private:
		RP_DISABLE_COPY(FileFormatFactory)

	public:
		/**
		 * Create a FileFormat subclass for the specified texture file.
		 *
		 * NOTE: FileFormat::isValid() is checked before returning a
		 * created FileFormat instance, so returned objects can be
		 * assumed to be valid as long as they aren't nullptr.
		 *
		 * If imgbf is non-zero, at least one of the specified image
		 * types must be supported by the FileFormat subclass in order to
		 * be returned.
		 *
		 * @param file Texture file.
		 * @return FileFormat subclass, or nullptr if the texture file isn't supported.
		 */
		static LibRpTexture::FileFormat *create(LibRpBase::IRpFile *file);

		/**
		 * Get all supported file extensions.
		 * Used for Win32 COM registration.
		 *
		 * @return All supported file extensions, including the leading dot.
		 */
		static const std::vector<const char*> &supportedFileExtensions(void);

		/**
		 * Get all supported MIME types.
		 * Used for KFileMetaData.
		 *
		 * @return All supported MIME types.
		 */
		static const std::vector<const char*> &supportedMimeTypes(void);
};

}

#endif /* __ROMPROPERTIES_LIBFileFormat_FileFormatFACTORY_HPP__ */
