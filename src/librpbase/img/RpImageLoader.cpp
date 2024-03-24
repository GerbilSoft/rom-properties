/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * RpImageLoader.cpp: Image loader class.                                  *
 *                                                                         *
 * Copyright (c) 2016-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "config.librpbase.h"

#include "RpImageLoader.hpp"
#include "librpfile/IRpFile.hpp"

// Other rom-properties libraries
using namespace LibRpFile;
using namespace LibRpTexture;

// Image loaders
#include "RpPng.hpp"
#ifdef HAVE_JPEG
#  include "RpJpeg.hpp"
#endif /* HAVE_JPEG */

// RpImageLoader isn't used by libromdata directly,
// so use some linker hax to force linkage.
extern "C" {
	extern unsigned char RP_LibRpBase_RpImageLoader_ForceLinkage;
	unsigned char RP_LibRpBase_RpImageLoader_ForceLinkage;
}

// C++ STL classes
using std::array;

namespace LibRpBase { namespace RpImageLoader {

// Magic numbers
static constexpr array<uint8_t, 8> png_magic = {{0x89, 'P', 'N', 'G', '\r', '\n', 0x1A, '\n'}};
#ifdef HAVE_JPEG
static constexpr array<uint8_t, 4> jpeg_magic = {{'J','F','I','F'}};
static constexpr array<uint8_t, 4> exif_magic = {{'E','x','i','f'}};
#endif /* HAVE_JPEG */

/** RpImageLoader **/

/**
 * Load an image from an IRpFile.
 * @param file IRpFile to load from.
 * @return rp_image*, or nullptr on error.
 */
rp_image_ptr load(const IRpFilePtr &file)
{
	file->rewind();

	// Check the file header to see what kind of image this is.
	uint8_t buf[256];
	size_t sz = file->read(buf, sizeof(buf));
	if (sz >= sizeof(png_magic)) {
		// Check for PNG.
		if (!memcmp(buf, png_magic.data(), png_magic.size())) {
			// Found a PNG image.
			return RpPng::load(file);
		}
#ifdef HAVE_JPEG
		else if (buf[0] == 0xFF && buf[1] != 0xFF && buf[2] == 0xFF) {
			// This may be a JPEG.
			// Check for the JFIF and Exif magic numbers.
			if (!memcmp(&buf[6], jpeg_magic.data(), jpeg_magic.size()) ||
			    !memcmp(&buf[6], exif_magic.data(), exif_magic.size()))
			{
				// Found a JPEG image.
				return RpJpeg::load(file);
			}
		}
#endif /* HAVE_JPEG */
	}

	// Unsupported image format.
	return nullptr;
}

} }
