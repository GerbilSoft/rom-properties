/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * RpJpeg.hpp: JPEG image handler.                                         *
 *                                                                         *
 * Copyright (c) 2016-2019 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_LIBRPBASE_IMG_RPJPEG_HPP__
#define __ROMPROPERTIES_LIBRPBASE_IMG_RPJPEG_HPP__

#include "../common.h"

namespace LibRpTexture {
	class rp_image;
}

namespace LibRpBase {

class IRpFile;

class RpJpeg
{
	private:
		// RpJpeg is a static class.
		RpJpeg();
		~RpJpeg();
		RP_DISABLE_COPY(RpJpeg)

	public:
		/**
		 * Load a JPEG image from an IRpFile.
		 *
		 * This image is NOT checked for issues; do not use
		 * with untrusted images!
		 *
		 * @param file IRpFile to load from.
		 * @return rp_image*, or nullptr on error.
		 */
		static LibRpTexture::rp_image *loadUnchecked(IRpFile *file);

		/**
		 * Load a JPEG image from an IRpFile.
		 *
		 * This image is verified with various tools to ensure
		 * it doesn't have any errors.
		 *
		 * @param file IRpFile to load from.
		 * @return rp_image*, or nullptr on error.
		 */
		static LibRpTexture::rp_image *load(IRpFile *file);
};

}

#endif /* __ROMPROPERTIES_LIBRPBASE_IMG_RPJPEG_HPP__ */
