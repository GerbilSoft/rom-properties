/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * RpJpeg_gdiplus.cpp: JPEG image handler. (GDI+ version)                  *
 *                                                                         *
 * Copyright (c) 2016-2026 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "RpJpeg.hpp"

// librpfile
#include "librpfile/IRpFile.hpp"
#include "librpfile/win32/IStreamWrapper.hpp"
using namespace LibRpFile;

// librptexture
#include "librptexture/img/rp_image.hpp"
#include "librptexture/img/RpGdiplusBackend.hpp"
using namespace LibRpTexture;

// Gdiplus for JPEG decoding.
// NOTE: Gdiplus requires min/max.
#include <algorithm>
namespace Gdiplus {
	using std::min;
	using std::max;
}
#include <comdef.h>
#include <gdiplus.h>

namespace LibRpBase { namespace RpJpeg {

/**
 * Load a JPEG image from an IRpFile.
 * @param file IRpFile to load from.
 * @return rp_image*, or nullptr on error.
 */
rp_image_ptr load(const IRpFilePtr &file)
{
	rp_image_ptr img;
	if (!file) {
		return img;
	}

	// Rewind the file.
	file->rewind();

	// Load the image using IStreamWrapper.
	IStreamWrapper *const stream = new IStreamWrapper(file.get());
	Gdiplus::Bitmap *const pGdipBmp = Gdiplus::Bitmap::FromStream(stream, FALSE);
	stream->Release();
	if (!pGdipBmp) {
		// Could not load the image.
		return img;
	}

	// Create an rp_image using the GDI+ bitmap.
	// NOTE: Assigning to `img` for named-return-value optimization.
	RpGdiplusBackend *const backend = new RpGdiplusBackend(pGdipBmp);
	img = std::make_shared<rp_image>(backend);
	return img;
}

} }
