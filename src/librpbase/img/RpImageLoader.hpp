/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * RpImageLoader.hpp: Image loader class.                                  *
 *                                                                         *
 * Copyright (c) 2016 by David Korth.                                      *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_LIBRPBASE_IMG_RPIMAGELOADER_HPP__
#define __ROMPROPERTIES_LIBRPBASE_IMG_RPIMAGELOADER_HPP__

#include "common.h"

namespace LibRpBase {

class rp_image;
class IRpFile;

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
		static rp_image *loadUnchecked(IRpFile *file);

		/**
		 * Load an image from an IRpFile.
		 *
		 * This image is verified with various tools to ensure
		 * it doesn't have any errors.
		 *
		 * @param file IRpFile to load from.
		 * @return rp_image*, or nullptr on error.
		 */
		static rp_image *load(IRpFile *file);
};

}

#endif /* __ROMPROPERTIES_LIBRPBASE_IMG_RP_IMAGE_LOADER_HPP__ */
