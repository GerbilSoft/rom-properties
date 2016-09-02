/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * RpImageLoader.cpp: Image loader class.                                  *
 *                                                                         *
 * Copyright (c) 2016 by David Korth.                                      *
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

#include "RpImageLoader.hpp"
#include "rp_image.hpp"
#include "../file/IRpFile.hpp"

// Image loaders.
#include "RpPng.hpp"

namespace LibRomData {

/**
 * Load an image from an IRpFile.
 *
 * This image is NOT checked for issues; do not use
 * with untrusted images!
 *
 * @param file IRpFile to load from.
 * @return rp_image*, or nullptr on error.
 */
rp_image *RpImageLoader::loadUnchecked(IRpFile *file)
{
	file->rewind();

	// Image magic numbers.
	const uint8_t png_magic[8] = {0x89, 'P', 'N', 'G', '\r', '\n', 0x1A, '\n'};

	// Check the file header to see what kind of image this is.
	uint8_t buf[256];
	size_t sz = file->read(buf, sizeof(buf));
	if (sz >= sizeof(png_magic)) {
		// Check for PNG.
		if (!memcmp(buf, png_magic, sizeof(png_magic))) {
			// Found a PNG image.
			return RpPng::loadUnchecked(file);
		}
	}

	// Unsupported image format.
	return nullptr;
}

/**
 * Load an image from an IRpFile.
 *
 * This image is verified with various tools to ensure
 * it doesn't have any errors.
 *
 * @param file IRpFile to load from.
 * @return rp_image*, or nullptr on error.
 */
rp_image *RpImageLoader::load(IRpFile *file)
{
	// FIXME: Actually implement checked loads.
	return loadUnchecked(file);
}

}
