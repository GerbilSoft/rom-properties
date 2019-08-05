/***************************************************************************
 * ROM Properties Page shell extension. (librptexture)                     *
 * RpImageLoader.hpp: Image loader class.                                  *
 *                                                                         *
 * Copyright (c) 2016-2019 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_LIBRPTEXTURE_IMG_RPIMAGELOADER_HPP__
#define __ROMPROPERTIES_LIBRPTEXTURE_IMG_RPIMAGELOADER_HPP__

#include "common.h"

namespace LibRpBase {
	class IRpFile;
}

namespace LibRpTexture {

class rp_image;

class RpImageLoader
{
	private:
		// RpImageLoader is a static class.
		RpImageLoader();
		~RpImageLoader();
		RP_DISABLE_COPY(RpImageLoader)

	public:
		/**
		 * Load an image from an IRpFile.
		 *
		 * This image is NOT checked for issues; do not use
		 * with untrusted images!
		 *
		 * @param file IRpFile to load from.
		 * @return rp_image*, or nullptr on error.
		 */
		static rp_image *loadUnchecked(LibRpBase::IRpFile *file);

		/**
		 * Load an image from an IRpFile.
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

#endif /* __ROMPROPERTIES_LIBRPTEXTURE_IMG_RP_IMAGE_LOADER_HPP__ */
