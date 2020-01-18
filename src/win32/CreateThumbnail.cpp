/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * CreateThumbnail.cpp: TCreateThumbnail<HBITMAP> implementation.          *
 *                                                                         *
 * Copyright (c) 2016-2020 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "CreateThumbnail.hpp"
#include "RpImageWin32.hpp"

// librptexture
#include "librptexture/img/RpGdiplusBackend.hpp"
using namespace LibRpTexture;

// C++ STL classes.
using std::string;
using std::unique_ptr;

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
	DeleteBitmap(imgClass);
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

	// IExtractIcon and IThumbnailProvider both support alpha transparency.
	// We're returning HBITMAP here, which works for IThumbnailProvider.
	// Our IExtractIcon implementation converts it to HICON later.

	// Resize the image.
	// TODO: "nearest" parameter.
	const SIZE win_sz = {sz.width, sz.height};
	return RpImageWin32::toHBITMAP_alpha(img.get(), win_sz, true);
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
 * Get the proxy for the specified URL.
 * @return Proxy, or empty string if no proxy is needed.
 */
string CreateThumbnail::proxyForUrl(const string &url) const
{
	// rp-download uses WinInet on Windows, which
	// always uses the system proxy.
	return string();
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

	// NOTE: IExtractImage doesn't support alpha transparency,
	// so blend the image with COLOR_WINDOW. This works for the
	// most part, at least with Windows Explorer, but the cached
	// Thumbs.db images won't reflect color scheme changes.
	return const_cast<RpGdiplusBackend*>(backend)->toHBITMAP(
		LibWin32Common::GetSysColor_ARGB32(COLOR_WINDOW));
}

/**
 * Rescale an ImgClass using nearest-neighbor scaling.
 * @param imgClass ImgClass object.
 * @param sz New size.
 * @return Rescaled ImgClass.
 */
HBITMAP CreateThumbnailNoAlpha::rescaleImgClass(const HBITMAP &imgClass, const ImgSize &sz) const
{
	// Convert the HBITMAP to rp_image.
	unique_ptr<rp_image> img(RpImageWin32::fromHBITMAP(imgClass));
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
	return RpImageWin32::toHBITMAP(img.get(),
		LibWin32Common::GetSysColor_ARGB32(COLOR_WINDOW),
		win_sz, true);
}
