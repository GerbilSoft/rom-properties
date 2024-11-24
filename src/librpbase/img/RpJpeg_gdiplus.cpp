/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * RpJpeg_gdiplus.cpp: JPEG image handler. (GDI+ version)                  *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
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
#include <gdiplus.h>

namespace LibRpBase { namespace RpJpeg {

/**
 * Load a JPEG image from an IRpFile.
 * @param file IRpFile to load from.
 * @return rp_image*, or nullptr on error.
 */
rp_image_ptr load(IRpFile *file)
{
	if (!file)
		return nullptr;

	// Rewind the file.
	file->rewind();

	// Load the image using IStreamWrapper.
	IStreamWrapper *const stream = new IStreamWrapper(file);
	Gdiplus::Bitmap *const pGdipBmp = Gdiplus::Bitmap::FromStream(stream, FALSE);
	stream->Release();
	if (!pGdipBmp) {
		// Could not load the image.
		return nullptr;
	}

	// Create an rp_image using the GDI+ bitmap.
	RpGdiplusBackend *const backend = new RpGdiplusBackend(pGdipBmp);
	return std::make_shared<rp_image>(backend);
}

} }
