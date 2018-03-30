/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * RpPngWriter.hpp: PNG image writer.                                      *
 *                                                                         *
 * Copyright (c) 2016-2018 by David Korth.                                 *
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

#ifndef __ROMPROPERTIES_LIBRPBASE_IMG_RPPNGWRITER_HPP__
#define __ROMPROPERTIES_LIBRPBASE_IMG_RPPNGWRITER_HPP__

#include "../common.h"
#include "rp_image.hpp"

// C++ includes.
#include <string>
#include <vector>

namespace LibRpBase {

class IRpFile;
struct IconAnimData;

class RpPngWriterPrivate;
class RpPngWriter
{
	public:
		/**
		 * Write a raw image to a PNG file.
		 *
		 * Check isOpen() after constructing to verify that
		 * the file was opened.
		 *
		 * NOTE: If the write fails, the caller will need
		 * to delete the file.
		 *
		 * NOTE 2: If the write fails, the caller will need
		 * to delete the file.
		 *
		 * @param filename	[in] Filename.
		 * @param width 	[in] Image width.
		 * @param height 	[in] Image height.
		 * @param format 	[in] Image format.
		 */
		RpPngWriter(const char *filename, int width, int height, rp_image::Format format);

		/**
		 * Write a raw image to a PNG file.
		 * IRpFile must be open for writing.
		 *
		 * Check isOpen() after constructing to verify that
		 * the file was opened.
		 *
		 * NOTE: If the write fails, the caller will need
		 * to delete the file.
		 *
		 * NOTE 2: If the write fails, the caller will need
		 * to delete the file.
		 *
		 * @param file	[in] IRpFile open for writing.
		 * @param width 	[in] Image width.
		 * @param height 	[in] Image height.
		 * @param format 	[in] Image format.
		 */
		RpPngWriter(IRpFile *file, int width, int height, rp_image::Format format);

		/**
		 * Write an image to a PNG file.
		 *
		 * Check isOpen() after constructing to verify that
		 * the file was opened.
		 *
		 * NOTE: If the write fails, the caller will need
		 * to delete the file.
		 *
		 * NOTE 2: If the write fails, the caller will need
		 * to delete the file.
		 *
		 * @param filename	[in] Filename.
		 * @param img		[in] rp_image.
		 */
		RpPngWriter(const char *filename, const rp_image *img);

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
		 * NOTE 2: If the write fails, the caller will need
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
		 * NOTE 2: If the write fails, the caller will need
		 * to delete the file.
		 *
		 * @param file		[in] IRpFile open for writing.
		 * @param iconAnimData	[in] Animated image data.
		 */
		RpPngWriter(const char *filename, const IconAnimData *iconAnimData);

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
		 * Close the PNG file.
		 */
		void close(void);

		/**
		 * Write the PNG IHDR.
		 * This must be called before writing any other image data.
		 * @return 0 on success; negative POSIX error code on error.
		 */
		int write_IHDR(void);

		/**
		 * Write the PNG IHDR.
		 * This must be called before writing any other image data.
		 *
		 * This function sets the cached sBIT before writing IHDR.
		 * It should only be used for raw images. Use write_IHDR()
		 * for rp_image and IconAnimData.
		 *
		 * @param sBIT		[in] sBIT metadata.
		 * @param palette	[in,opt] Palette for CI8 images.
		 * @param palette_len	[in,opt] Number of entries in `palette`.
		 * @return 0 on success; negative POSIX error code on error.
		 */
		int write_IHDR(const rp_image::sBIT_t *sBIT, const uint32_t *palette = nullptr, int palette_len = 0);

		// Key/value pair.
		// - Key: Must be Latin-1.
		// - Value: May be UTF-8.
		typedef std::vector<std::pair<const char*, std::string> > kv_vector;

		/**
		 * Write an array of text chunks.
		 * This is needed for e.g. the XDG thumbnailing specification.
		 *
		 * If called before write_IHDR(), tEXt chunks will be written
		 * before the IDAT chunk.
		 *
		 * If called after write_IHDR(), tEXt chunks will be written
		 * after the IDAT chunk.
		 *
		 * NOTE: Key must be Latin-1. Value may be UTF-8.
		 * If value is ASCII or exclusively uses Latin-1 code points,
		 * it will be saved as Latin-1.
		 *
		 * Strings that are 40 bytes or longer will be saved as zTXt.
		 *
		 * @param kv Vector of key/value pairs.
		 * @return 0 on success; negative POSIX error code on error.
		 */
		int write_tEXt(const kv_vector &kv);

		/**
		 * Write raw image data to the PNG image.
		 *
		 * This must be called after any other modifier functions.
		 *
		 * If constructed using a filename instead of IRpFile,
		 * this will automatically close the file.
		 *
		 * NOTE: This version is *only* for raw images!
		 *
		 * @param row_pointers PNG row pointers. Array must have cache.height elements.
		 * @param is_abgr If true, image data is ABGR instead of ARGB.
		 * @return 0 on success; negative POSIX error code on error.
		 */
		int write_IDAT(const uint8_t *const *row_pointers, bool is_abgr = false);

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
