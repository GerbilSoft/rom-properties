/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * CreateThumbnail.cpp: TCreateThumbnail<HBITMAP> implementation.          *
 *                                                                         *
 * Copyright (c) 2016-2017 by David Korth.                                 *
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

#include "stdafx.h"
#include "CreateThumbnail.hpp"
#include "RpImageWin32.hpp"

#include "librpbase/img/rp_image.hpp"
#include "librpbase/img/RpGdiplusBackend.hpp"
using namespace LibRpBase;

// C++ includes.
#include <memory>
using std::unique_ptr;

/**
 * Wrapper function to convert rp_image* to ImgClass.
 * @param img rp_image
 * @return ImgClass.
 */
HBITMAP CreateThumbnail::rpImageToImgClass(const rp_image *img) const
{
	// IExtractIcon returns an icon with alpha transparency.
	// We're returning an HBITMAP here. It's converted into
	// an HICON later.

	// We should be using the RpGdiplusBackend.
	const RpGdiplusBackend *backend =
		dynamic_cast<const RpGdiplusBackend*>(img->backend());
	assert(backend != nullptr);
	if (!backend) {
		// Incorrect backend set.
		return nullptr;
	}

	// Windows doesn't like non-square icons.
	// Add extra transparent columns/rows before
	// converting to HBITMAP.
	unique_ptr<rp_image> tmp_img;
	if (!img->isSquare()) {
		// Image is non-square.
		tmp_img.reset(img->squared());
		assert(tmp_img.get() != nullptr);
		if (tmp_img) {
			const RpGdiplusBackend *const tmp_backend =
				dynamic_cast<const RpGdiplusBackend*>(tmp_img->backend());
			assert(tmp_backend != nullptr);
			if (tmp_backend) {
				backend = tmp_backend;
			}
		}
	}

	// Convert to HBITMAP.
	// TODO: Const-ness stuff.
	return const_cast<RpGdiplusBackend*>(backend)->toHBITMAP_alpha();
}

/**
 * Wrapper function to check if an ImgClass is valid.
 * @param imgClass ImgClass
 * @return True if valid; false if not.
 */
bool CreateThumbnail::isImgClassValid(const HBITMAP &imgClass) const
{
	return (imgClass != nullptr);
}

/**
 * Wrapper function to get a "null" ImgClass.
 * @return "Null" ImgClass.
 */
HBITMAP CreateThumbnail::getNullImgClass(void) const
{
	return nullptr;
}

/**
 * Free an ImgClass object.
 * @param imgClass ImgClass object.
 */
void CreateThumbnail::freeImgClass(HBITMAP &imgClass) const
{
	DeleteObject(imgClass);
}

/**
 * Rescale an ImgClass using nearest-neighbor scaling.
 * @param imgClass ImgClass object.
 * @param sz New size.
 * @return Rescaled ImgClass.
 */
HBITMAP CreateThumbnail::rescaleImgClass(const HBITMAP &imgClass, const ImgSize &sz) const
{
	// Convert the HBITMAP to rp_image.
	unique_ptr<rp_image> img(RpImageWin32::fromHBITMAP(imgClass));
	if (!img) {
		// Error converting to rp_image.
		return nullptr;
	}

	// IExtractIcon returns an icon with alpha transparency.
	// We're returning an HBITMAP here. It's converted into
	// an HICON later.

	// Resize the image.
	// TODO: "nearest" parameter.
	const SIZE win_sz = {sz.width, sz.height};
	return RpImageWin32::toHBITMAP_alpha(img.get(), win_sz, true);
}

/**
 * Get the proxy for the specified URL.
 * @return Proxy, or empty string if no proxy is needed.
 */
rp_string CreateThumbnail::proxyForUrl(const rp_string &url) const
{
	// libcachemgr uses urlmon on Windows, which
	// always uses the system proxy.
	return rp_string();
}
