/***************************************************************************
 * ROM Properties Page shell extension. (librptexture)                     *
 * FileFormat.hpp: Texture file format base class. (PRIVATE CLASS)         *
 *                                                                         *
 * Copyright (c) 2016-2019 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_LIBRPTEXTURE_FILEFORMAT_FILEFORMAT_P_HPP__
#define __ROMPROPERTIES_LIBRPTEXTURE_FILEFORMAT_FILEFORMAT_P_HPP__

namespace LibRpBase {
	class IRpFile;
}

namespace LibRpTexture {

class FileFormat;
class FileFormatPrivate
{
	public:
		/**
		 * Initialize a FileFormatPrivate storage class.
		 *
		 * @param q FileFormat class.
		 * @param file Texture file.
		 */
		explicit FileFormatPrivate(FileFormat *q, LibRpBase::IRpFile *file);

		virtual ~FileFormatPrivate();

	private:
		RP_DISABLE_COPY(FileFormatPrivate)
	protected:
		friend class FileFormat;
		FileFormat *const q_ptr;

	public:
		volatile int ref_cnt;		// Reference count.
		bool isValid;			// Subclass must set this to true if the ROM is valid.
		LibRpBase::IRpFile *file;	// Open file.

	public:
		/** These fields must be set by FileFormat subclasses in their constructors. **/
		const char *mimeType;		// MIME type. (ASCII) (default is nullptr)
		int dimensions[3];		// Dimensions. (width, height, depth)
						// 2D textures have depth=0.
};

}

#endif /* __ROMPROPERTIES_LIBRPTEXTURE_FILEFORMAT_FILEFORMAT_P_HPP__ */
