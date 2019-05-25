/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * RpPng.hpp: PNG image handler.                                           *
 *                                                                         *
 * Copyright (c) 2016-2018 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_LIBRPBASE_IMG_RPPNG_HPP__
#define __ROMPROPERTIES_LIBRPBASE_IMG_RPPNG_HPP__

#include "../common.h"

namespace LibRpBase {

class IRpFile;
class rp_image;
struct IconAnimData;

class RpPng
{
	private:
		// RpPng is a static class.
		RpPng();
		~RpPng();
		RP_DISABLE_COPY(RpPng)

	public:
		/**
		 * Load a PNG image from an IRpFile.
		 *
		 * This image is NOT checked for issues; do not use
		 * with untrusted images!
		 *
		 * @param file IRpFile to load from.
		 * @return rp_image*, or nullptr on error.
		 */
		static rp_image *loadUnchecked(IRpFile *file);

		/**
		 * Load a PNG image from an IRpFile.
		 *
		 * This image is verified with various tools to ensure
		 * it doesn't have any errors.
		 *
		 * @param file IRpFile to load from.
		 * @return rp_image*, or nullptr on error.
		 */
		static rp_image *load(IRpFile *file);

		/**
		 * Save an image in PNG format to an IRpFile.
		 * IRpFile must be open for writing.
		 *
		 * NOTE: If the write fails, the caller will need
		 * to delete the file.
		 *
		 * @param file IRpFile to write to.
		 * @param img rp_image to save.
		 * @return 0 on success; negative POSIX error code on error.
		 */
		static int save(IRpFile *file, const rp_image *img);

		/**
		 * Save an image in PNG format to a file.
		 *
		 * @param filename Destination filename.
		 * @param img rp_image to save.
		 * @return 0 on success; negative POSIX error code on error.
		 */
		static int save(const char *filename, const rp_image *img);

		/**
		 * Save an animated image in APNG format to an IRpFile.
		 * IRpFile must be open for writing.
		 *
		 * If the animated image contains a single frame,
		 * a standard PNG image will be written.
		 *
		 * NOTE: If the image has multiple frames and APNG
		 * write support is unavailable, -ENOTSUP will be
		 * returned. The caller should then save the image
		 * as a standard PNG file.
		 *
		 * NOTE 2: If the write fails, the caller will need
		 * to delete the file.
		 *
		 * @param file IRpFile to write to.
		 * @param iconAnimData Animated image data to save.
		 * @return 0 on success; negative POSIX error code on error.
		 */
		static int save(IRpFile *file, const IconAnimData *iconAnimData);

		/**
		 * Save an animated image in APNG format to a file.
		 *
		 * If the animated image contains a single frame,
		 * a standard PNG image will be written.
		 *
		 * NOTE: If the image has multiple frames and APNG
		 * write support is unavailable, -ENOTSUP will be
		 * returned. The caller should then save the image
		 * as a standard PNG file.
		 *
		 * @param filename Destination filename.
		 * @param iconAnimData Animated image data to save.
		 * @return 0 on success; negative POSIX error code on error.
		 */
		static int save(const char *filename, const IconAnimData *iconAnimData);
};

}

#endif /* __ROMPROPERTIES_LIBRPBASE_IMG_RPPNG_HPP__ */
