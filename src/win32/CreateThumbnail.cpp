/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * CreateThumbnail.cpp: TCreateThumbnail<HBITMAP> implementation.          *
 *                                                                         *
 * Copyright (c) 2016-2022 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "CreateThumbnail.hpp"
#include "RpImageWin32.hpp"
#include "NetworkStatus.h"

// librptexture
#include "librptexture/img/RpGdiplusBackend.hpp"
using namespace LibRpTexture;

// C++ STL classes.
using std::string;

// TCreateThumbnail is a templated class,
// so we have to #include the .cpp file here.
#include "libromdata/img/TCreateThumbnail.cpp"

// Explicitly instantiate TCreateThumbnail<HBITMAP>.
template class LibRomData::TCreateThumbnail<HBITMAP>;

/** CreateThumbnail **/

/**
 * Wrapper function to convert rp_image* to ImgClass.
 * @tparam alpha If true, use alpha transparency. Otherwise, use COLOR_WINDOW.
 * @param img rp_image
 * @return ImgClass.
 */
HBITMAP CreateThumbnail::rpImageToImgClass(const rp_image *img) const
{
	// IExtractIcon and IThumbnailProvider both support alpha transparency.
	// We're returning HBITMAP here, which works for IThumbnailProvider.
	// Our IExtractIcon implementation converts it to HICON later.

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
	// TODO: Disable this for RP_ExtractImage and RP_ThumbnailProvider?
	rp_image *tmp_img = nullptr;
	if (!img->isSquare()) {
		// Image is non-square.
		tmp_img = img->squared();
		assert(tmp_img != nullptr);
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
	HBITMAP hbmp = const_cast<RpGdiplusBackend*>(backend)->toHBITMAP_alpha();
	UNREF(tmp_img);
	return hbmp;
}

/**
 * Rescale an ImgClass using the specified scaling method.
 * @param imgClass ImgClass object.
 * @param sz New size.
 * @param method Scaling method.
 * @return Rescaled ImgClass.
 */
HBITMAP CreateThumbnail::rescaleImgClass(const HBITMAP &imgClass, ImgSize sz, ScalingMethod method) const
{
	// Convert the HBITMAP to rp_image.
	rp_image *const img = RpImageWin32::fromHBITMAP(imgClass);
	if (!img) {
		// Error converting to rp_image.
		return nullptr;
	}

	// IExtractIcon and IThumbnailProvider both support alpha transparency.
	// We're returning HBITMAP here, which works for IThumbnailProvider.
	// Our IExtractIcon implementation converts it to HICON later.

	// Resize the image.
	const SIZE win_sz = {sz.width, sz.height};
	HBITMAP hbmp = RpImageWin32::toHBITMAP_alpha(img, win_sz, (method == ScalingMethod::Nearest));
	img->unref();
	return hbmp;
}

/**
 * Get the size of the specified ImgClass.
 * @param imgClass	[in] ImgClass object.
 * @param pOutSize	[out] Pointer to ImgSize to store the image size.
 * @return 0 on success; non-zero on error.
 */
int CreateThumbnail::getImgClassSize(const HBITMAP &imgClass, ImgSize *pOutSize) const
{
	BITMAP bmp;
	int ret = GetObject(static_cast<HGDIOBJ>(imgClass), sizeof(bmp), &bmp);
	assert(ret != 0);
	if (ret) {
		pOutSize->width = bmp.bmWidth;
		pOutSize->height = bmp.bmHeight;
		return 0;
	}
	return -1;
}

/**
 * Is the system using a metered connection?
 *
 * Note that if the system doesn't support identifying if the
 * connection is metered, it will be assumed that the network
 * connection is unmetered.
 *
 * @return True if metered; false if not.
 */
bool CreateThumbnail::isMetered(void)
{
	return rp_win32_is_metered();
}

/** CreateThumbnailNoAlpha **/

/**
 * Wrapper function to convert rp_image* to ImgClass.
 * @tparam alpha If true, use alpha transparency. Otherwise, use COLOR_WINDOW.
 * @param img rp_image
 * @return ImgClass.
 */
HBITMAP CreateThumbnailNoAlpha::rpImageToImgClass(const rp_image *img) const
{
	// IExtractImage doesn't support alpha transparency, so we'll
	// use COLOR_WINDOW as the background color.

	// We should be using the RpGdiplusBackend.
	const RpGdiplusBackend *backend =
		dynamic_cast<const RpGdiplusBackend*>(img->backend());
	assert(backend != nullptr);
	if (!backend) {
		// Incorrect backend set.
		return nullptr;
	}

	// Convert to HBITMAP.
	// TODO: Const-ness stuff.

	// NOTE: IExtractImage doesn't support alpha transparency,
	// so blend the image with COLOR_WINDOW. This works for the
	// most part, at least with Windows Explorer, but the cached
	// Thumbs.db images won't reflect color scheme changes.
	HBITMAP hbmp = const_cast<RpGdiplusBackend*>(backend)->toHBITMAP(
		LibWin32UI::GetSysColor_ARGB32(COLOR_WINDOW));
	return hbmp;
}

/**
 * Rescale an ImgClass using the specified scaling method.
 * @param imgClass ImgClass object.
 * @param sz New size.
 * @param method Scaling method.
 * @return Rescaled ImgClass.
 */
HBITMAP CreateThumbnailNoAlpha::rescaleImgClass(const HBITMAP &imgClass, ImgSize sz, ScalingMethod method) const
{
	// Convert the HBITMAP to rp_image.
	rp_image *const img = RpImageWin32::fromHBITMAP(imgClass);
	if (!img) {
		// Error converting to rp_image.
		return nullptr;
	}

	// NOTE: IExtractImage doesn't support alpha transparency,
	// so blend the image with COLOR_WINDOW. This works for the
	// most part, at least with Windows Explorer, but the cached
	// Thumbs.db images won't reflect color scheme changes.

	// Resize the image.
	// TODO: "nearest" parameter.
	const SIZE win_sz = {sz.width, sz.height};
	HBITMAP hbmp = RpImageWin32::toHBITMAP(img,
		LibWin32UI::GetSysColor_ARGB32(COLOR_WINDOW),
		win_sz, (method == ScalingMethod::Nearest));
	img->unref();
	return hbmp;
}
