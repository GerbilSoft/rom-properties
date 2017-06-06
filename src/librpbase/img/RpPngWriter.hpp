/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * RpPngWriter.hpp: PNG image writer.                                      *
 *                                                                         *
 * Copyright (c) 2016-2017 by David Korth.                                 *
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
 * You should have received a copy of the GNU General Public License along *
 * with this program; if not, write to the Free Software Foundation, Inc., *
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.           *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_LIBRPBASE_IMG_RPPNGWRITER_HPP__
#define __ROMPROPERTIES_LIBRPBASE_IMG_RPPNGWRITER_HPP__

#include "librpbase/config.librpbase.h"
#include "librpbase/common.h"

namespace LibRpBase {

class IRpFile;
class rp_image;
class IconAnimData;

class RpPngWriterPrivate;
class RpPngWriter
{
	public:
		/**
		 * Write an image to a PNG file.
		 *
		 * Check isOpen() after constructing to verify that
		 * the file was opened.
		 *
		 * @param filename	[in] Filename.
		 * @param img		[in] rp_image.
		 */
		RpPngWriter(const rp_char *filename, const rp_image *img);

		/**
		 * Write an image to a PNG file.
		 * IRpFile must be open for writing.
		 *
		 * Check isOpen() after constructing to verify that
		 * the file was opened.
		 *
		 * NOTE: If the write fails, the caller will need
		 * to delete the file.
		 *
		 * @param file	[in] IRpFile open for writing.
		 * @param img	[in] rp_image.
		 */
		RpPngWriter(IRpFile *file, const rp_image *img);

		/**
		 * Write an animated image to an APNG file.
		 *
		 * Check isOpen() after constructing to verify that
		 * the file was opened.
		 *
		 * If the animated image contains a single frame,
		 * a standard PNG image will be written.
		 *
		 * NOTE: If the image has multiple frames and APNG
		 * write support is unavailable, -ENOTSUP will be
		 * set as the last error. The caller should then save
		 * the image as a standard PNG file.
		 *
		 * @param file		[in] IRpFile open for writing.
		 * @param iconAnimData	[in] Animated image data.
		 */
		RpPngWriter(const rp_char *filename, const IconAnimData *iconAnimData);

		/**
		 * Write an animated image to an APNG file.
		 * IRpFile must be open for writing.
		 *
		 * Check isOpen() after constructing to verify that
		 * the file was opened.
		 *
		 * If the animated image contains a single frame,
		 * a standard PNG image will be written.
		 *
		 * NOTE: If the image has multiple frames and APNG
		 * write support is unavailable, -ENOTSUP will be
		 * set as the last error. The caller should then save
		 * the image as a standard PNG file.
		 *
		 * NOTE 2: If the write fails, the caller will need
		 * to delete the file.
		 *
		 * @param file		[in] IRpFile open for writing.
		 * @param iconAnimData	[in] Animated image data.
		 */
		RpPngWriter(IRpFile *file, const IconAnimData *iconAnimData);

		~RpPngWriter();

	private:
		friend class RpPngWriterPrivate;
		RpPngWriterPrivate *const d_ptr;
		RP_DISABLE_COPY(RpPngWriter)

	public:
		/**
		 * Is the PNG file open?
		 * @return True if the PNG file is open; false if not.
		 */
		bool isOpen(void) const;

		/**
		 * Get the last error.
		 * @return Last POSIX error, or 0 if no error.
		 */
		int lastError(void) const;

		/**
		 * Write the PNG IHDR.
		 * This must be called before writing any other image data.
		 * @return 0 on success; negative POSIX error code on error.
		 */
		int write_IHDR(void);

		/**
		 * Write the rp_image data to the PNG image.
		 *
		 * This must be called after any other modifier functions.
		 *
		 * If constructed using a filename instead of IRpFile,
		 * this will automatically close the file.
		 *
		 * @return 0 on success; negative POSIX error code on error.
		 */
		int write_IDAT(void);
};

}

#endif /* __ROMPROPERTIES_LIBRPBASE_IMG_RPPNGWRITER_HPP__ */
