/***************************************************************************
 * ROM Properties Page shell extension. (librptexture)                     *
 * RpJpeg.hpp: JPEG image handler.                                         *
 *                                                                         *
 * Copyright (c) 2016-2019 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_LIBRPTEXTURE_IMG_RPJPEG_HPP__
#define __ROMPROPERTIES_LIBRPTEXTURE_IMG_RPJPEG_HPP__

#include "librpbase/common.h"

namespace LibRpBase {
	class IRpFile;
}

namespace LibRpTexture {

class rp_image;

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
		static rp_image *loadUnchecked(LibRpBase::IRpFile *file);

		/**
		 * Load a JPEG image from an IRpFile.
		 *
		 * This image is verified with various tools to ensure
		 * it doesn't have any errors.
		 *
		 * @param file IRpFile to load from.
		 * @return rp_image*, or nullptr on error.
		 */
		static rp_image *load(LibRpBase::IRpFile *file);
};

}

#endif /* __ROMPROPERTIES_LIBRPTEXTURE_IMG_RPJPEG_HPP__ */
