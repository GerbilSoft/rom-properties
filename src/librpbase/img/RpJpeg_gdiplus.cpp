/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * RpJpeg_gdiplus.cpp: JPEG image handler. (GDI+ version)                  *
 *                                                                         *
 * Copyright (c) 2016-2019 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "RpJpeg.hpp"

// librpfile
#include "librpfile/IRpFile.hpp"
#include "librpfile/win32/IStreamWrapper.hpp"
using LibRpBase::IRpFile;
using LibRpBase::IStreamWrapper;

// librptexture
#include "librptexture/img/rp_image.hpp"
#include "librptexture/img/RpGdiplusBackend.hpp"
using LibRpTexture::rp_image;
using LibRpTexture::RpGdiplusBackend;

// Gdiplus for JPEG decoding.
// NOTE: Gdiplus requires min/max.
#include <algorithm>
namespace Gdiplus {
	using std::min;
	using std::max;
}
#include <gdiplus.h>
#include "librptexture/img/GdiplusHelper.hpp"

namespace LibRpBase {

/** RpJpeg **/

/**
 * Load a JPEG image from an IRpFile.
 *
 * This image is NOT checked for issues; do not use
 * with untrusted images!
 *
 * @param file IRpFile to load from.
 * @return rp_image*, or nullptr on error.
 */
rp_image *RpJpeg::loadUnchecked(IRpFile *file)
{
	if (!file)
		return nullptr;

	// Initialize GDI+.
	// TODO: Don't init/shutdowno n every image.
	ScopedGdiplus gdip;
	if (!gdip.isValid()) {
		// Failed to initialized GDI+.
		return nullptr;
	}

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
	return new rp_image(backend);
}

/**
 * Load a JPEG image from an IRpFile.
 *
 * This image is verified with various tools to ensure
 * it doesn't have any errors.
 *
 * @param file IRpFile to load from.
 * @return rp_image*, or nullptr on error.
 */
rp_image *RpJpeg::load(IRpFile *file)
{
	if (!file)
		return nullptr;

	// FIXME: Add a JPEG equivalent of pngcheck().
	return loadUnchecked(file);
}

}
