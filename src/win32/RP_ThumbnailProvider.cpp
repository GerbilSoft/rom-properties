/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * RP_ThumbnailProvider.hpp: IThumbnailProvider implementation.            *
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

// Reference: http://www.codeproject.com/Articles/338268/COM-in-C
#include "stdafx.h"
#include "RP_ThumbnailProvider.hpp"
#include "RegKey.hpp"
#include "RpImageWin32.hpp"

// libromdata
#include "libromdata/RomData.hpp"
#include "libromdata/RomDataFactory.hpp"
#include "libromdata/RpWin32.hpp"
#include "libromdata/file/RpFile.hpp"
#include "libromdata/img/rp_image.hpp"
#include "libromdata/img/RpImageLoader.hpp"
using namespace LibRomData;

// RpFile_IStream
#include "RpFile_IStream.hpp"

// C includes. (C++ namespace)
#include <cassert>
#include <cstdio>
#include <cstring>

// C++ includes.
#include <memory>
#include <string>
using std::unique_ptr;
using std::wstring;

// CLSID
const CLSID CLSID_RP_ThumbnailProvider =
	{0x4723df58, 0x463e, 0x4590, {0x8f, 0x4a, 0x8d, 0x9d, 0xd4, 0xf4, 0x35, 0x5a}};

/** RP_ThumbnailProvider_Private **/
#include "RP_ThumbnailProvider_p.hpp"

// TCreateThumbnail is a templated class,
// so we have to #include the .cpp file here.
#include "libromdata/img/TCreateThumbnail.cpp"

/** RP_ThumbnailProvider_Private **/

RP_ThumbnailProvider_Private::RP_ThumbnailProvider_Private()
	: file(nullptr)
	, pstream(nullptr)
	, grfMode(0)
{ }

RP_ThumbnailProvider_Private::~RP_ThumbnailProvider_Private()
{
	// pstream is owned by file,
	// so don't Release() it here.
	delete file;
}

/**
 * Wrapper function to convert rp_image* to ImgClass.
 * @param img rp_image
 * @return ImgClass.
 */
HBITMAP RP_ThumbnailProvider_Private::rpImageToImgClass(const rp_image *img) const
{
	return RpImageWin32::toHBITMAP_alpha(img);
}

/**
 * Wrapper function to check if an ImgClass is valid.
 * @param imgClass ImgClass
 * @return True if valid; false if not.
 */
bool RP_ThumbnailProvider_Private::isImgClassValid(const HBITMAP &imgClass) const
{
	return (imgClass != nullptr);
}

/**
 * Wrapper function to get a "null" ImgClass.
 * @return "Null" ImgClass.
 */
HBITMAP RP_ThumbnailProvider_Private::getNullImgClass(void) const
{
	return nullptr;
}

/**
 * Free an ImgClass object.
 * This may be no-op for e.g. HBITMAP.
 * @param imgClass ImgClass object.
 */
void RP_ThumbnailProvider_Private::freeImgClass(HBITMAP &imgClass) const
{
	DeleteObject(imgClass);
}

/**
 * Rescale an ImgClass using nearest-neighbor scaling.
 * @param imgClass ImgClass object.
 * @param sz New size.
 * @return Rescaled ImgClass.
 */
HBITMAP RP_ThumbnailProvider_Private::rescaleImgClass(const HBITMAP &imgClass, const ImgSize &sz) const
{
	// Convert the HBITMAP to rp_image.
	unique_ptr<rp_image> img(RpImageWin32::fromHBITMAP(imgClass));
	if (!img) {
		// Error converting to rp_image.
		return nullptr;
	}

	// Resize the image.
	// TODO: "nearest" parameter.
	const SIZE win_sz = {sz.width, sz.height};
	return RpImageWin32::toHBITMAP_alpha(img.get(), win_sz, true);
}

/**
 * Get the proxy for the specified URL.
 * @return Proxy, or empty string if no proxy is needed.
 */
rp_string RP_ThumbnailProvider_Private::proxyForUrl(const rp_string &url) const
{
	// libcachemgr uses urlmon on Windows, which
	// always uses the system proxy.
	return rp_string();
}

/** RP_ThumbnailProvider **/

RP_ThumbnailProvider::RP_ThumbnailProvider()
	: d_ptr(new RP_ThumbnailProvider_Private())
{ }

RP_ThumbnailProvider::~RP_ThumbnailProvider()
{
	delete d_ptr;
}

/** IUnknown **/
// Reference: https://msdn.microsoft.com/en-us/library/office/cc839627.aspx

IFACEMETHODIMP RP_ThumbnailProvider::QueryInterface(REFIID riid, LPVOID *ppvObj)
{
	static const QITAB rgqit[] = {
		QITABENT(RP_ThumbnailProvider, IInitializeWithStream),
		QITABENT(RP_ThumbnailProvider, IThumbnailProvider),
		{ 0, 0 }
	};
	return pQISearch(this, rgqit, riid, ppvObj);
}

/** IInitializeWithStream **/
// Reference: https://msdn.microsoft.com/en-us/library/windows/desktop/bb761812(v=vs.85).aspx [Initialize()]

IFACEMETHODIMP RP_ThumbnailProvider::Initialize(IStream *pstream, DWORD grfMode)
{
	// Ignoring grfMode for now. (always read-only)
	((void)grfMode);

	// Create an IRpFile wrapper for the IStream.
	IRpFile *file = new RpFile_IStream(pstream);
	if (file->lastError() != 0) {
		// Error initializing the IRpFile.
		delete file;
		return E_FAIL;
	}

	RP_D(RP_ThumbnailProvider);
	if (d->file) {
		// Delete the old file first.
		IRpFile *old_file = d->file;
		d->file = file;
		delete old_file;
	} else {
		// No old file to delete.
		d->file = file;
	}

	// Save the IStream and grfMode.
	d->pstream = pstream;
	d->grfMode = grfMode;
	return S_OK;
}

/** IThumbnailProvider **/
// Reference: https://msdn.microsoft.com/en-us/library/windows/desktop/bb774612(v=vs.85).aspx [GetThumbnail()]

IFACEMETHODIMP RP_ThumbnailProvider::GetThumbnail(UINT cx, HBITMAP *phbmp, WTS_ALPHATYPE *pdwAlpha)
{
	// Verify parameters:
	// - A stream must have been set by calling IInitializeWithStream::Initialize().
	// - phbmp and pdwAlpha must not be nullptr.
	RP_D(RP_ThumbnailProvider);
	if (!d->file || !phbmp || !pdwAlpha) {
		return E_INVALIDARG;
	}
	*phbmp = nullptr;
	*pdwAlpha = WTSAT_ARGB;

	int ret = d->getThumbnail(d->file, cx, *phbmp);
	if (ret != 0) {
		// ROM is not supported. Use the fallback.
		return d->Fallback(cx, phbmp, pdwAlpha);
	}
	return S_OK;
}
