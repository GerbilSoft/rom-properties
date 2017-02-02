/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * RP_ExtractImage.hpp: IExtractImage implementation.                      *
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
#include "RP_ExtractImage.hpp"
#include "RpImageWin32.hpp"

// libromdata
#include "libromdata/RomData.hpp"
#include "libromdata/RomDataFactory.hpp"
#include "libromdata/RpWin32.hpp"
#include "libromdata/file/RpFile.hpp"
#include "libromdata/img/rp_image.hpp"
using namespace LibRomData;

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
const CLSID CLSID_RP_ExtractImage =
	{0x84573bc0, 0x9502, 0x42f8, {0x80, 0x66, 0xCC, 0x52, 0x7D, 0x07, 0x79, 0xE5}};

/** RP_ExtractImage_Private **/
#include "RP_ExtractImage_p.hpp"

// TCreateThumbnail is a templated class,
// so we have to #include the .cpp file here.
#include "libromdata/img/TCreateThumbnail.cpp"

RP_ExtractImage_Private::RP_ExtractImage_Private()
	: dwRecClrDepth(0)
	, dwFlags(0)
{
	rgSize.cx = 0;
	rgSize.cy = 0;
}

RP_ExtractImage_Private::~RP_ExtractImage_Private()
{ }

/**
 * Wrapper function to convert rp_image* to ImgClass.
 * @param img rp_image
 * @return ImgClass.
 */
HBITMAP RP_ExtractImage_Private::rpImageToImgClass(const rp_image *img) const
{
	// NOTE: IExtractImage doesn't support alpha transparency,
	// so blend the image with COLOR_WINDOW. This works for the
	// most part, at least with Windows Explorer, but the cached
	// Thumbs.db images won't reflect color scheme changes.
	// NOTE 2: GetSysColor() has swapped R and B channels
	// compared to GDI+.
	COLORREF bgColor = GetSysColor(COLOR_WINDOW);
	bgColor = (bgColor & 0x00FF00) | 0xFF000000 |
		  ((bgColor & 0xFF) << 16) |
		  ((bgColor >> 16) & 0xFF);
	return RpImageWin32::toHBITMAP(img, bgColor);
}

/**
 * Wrapper function to check if an ImgClass is valid.
 * @param imgClass ImgClass
 * @return True if valid; false if not.
 */
bool RP_ExtractImage_Private::isImgClassValid(const HBITMAP &imgClass) const
{
	return (imgClass != nullptr);
}

/**
 * Wrapper function to get a "null" ImgClass.
 * @return "Null" ImgClass.
 */
HBITMAP RP_ExtractImage_Private::getNullImgClass(void) const
{
	return nullptr;
}

/**
 * Free an ImgClass object.
 * This may be no-op for e.g. HBITMAP.
 * @param imgClass ImgClass object.
 */
void RP_ExtractImage_Private::freeImgClass(HBITMAP &imgClass) const
{
	DeleteObject(imgClass);
}

/**
 * Rescale an ImgClass using nearest-neighbor scaling.
 * @param imgClass ImgClass object.
 * @param sz New size.
 * @return Rescaled ImgClass.
 */
HBITMAP RP_ExtractImage_Private::rescaleImgClass(const HBITMAP &imgClass, const ImgSize &sz) const
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
	// NOTE 2: GetSysColor() has swapped R and B channels
	// compared to GDI+.
	COLORREF bgColor = GetSysColor(COLOR_WINDOW);
	bgColor = (bgColor & 0x00FF00) | 0xFF000000 |
		  ((bgColor & 0xFF) << 16) |
		  ((bgColor >> 16) & 0xFF);

	// Resize the image.
	// TODO: "nearest" parameter.
	const SIZE win_sz = {sz.width, sz.height};
	return RpImageWin32::toHBITMAP(img.get(), bgColor, win_sz, true);
}

/**
 * Get the proxy for the specified URL.
 * @return Proxy, or empty string if no proxy is needed.
 */
rp_string RP_ExtractImage_Private::proxyForUrl(const rp_string &url) const
{
	// libcachemgr uses urlmon on Windows, which
	// always uses the system proxy.
	return rp_string();
}

/** RP_ExtractImage **/

RP_ExtractImage::RP_ExtractImage()
	: d_ptr(new RP_ExtractImage_Private())
{ }

RP_ExtractImage::~RP_ExtractImage()
{
	delete d_ptr;
}

/** IUnknown **/
// Reference: https://msdn.microsoft.com/en-us/library/office/cc839627.aspx

IFACEMETHODIMP RP_ExtractImage::QueryInterface(REFIID riid, LPVOID *ppvObj)
{
	static const QITAB rgqit[] = {
		QITABENT(RP_ExtractImage, IPersist),
		QITABENT(RP_ExtractImage, IPersistFile),
		QITABENT(RP_ExtractImage, IExtractImage),
		QITABENT(RP_ExtractImage, IExtractImage2),
		{ 0 }
	};
	return pQISearch(this, rgqit, riid, ppvObj);
}

/** IPersistFile **/
// Reference: https://msdn.microsoft.com/en-us/library/windows/desktop/cc144067(v=vs.85).aspx#unknown_28177

IFACEMETHODIMP RP_ExtractImage::GetClassID(CLSID *pClassID)
{
	if (!pClassID) {
		return E_FAIL;
	}
	*pClassID = CLSID_RP_ExtractImage;
	return S_OK;
}

IFACEMETHODIMP RP_ExtractImage::IsDirty(void)
{
	return E_NOTIMPL;
}

IFACEMETHODIMP RP_ExtractImage::Load(LPCOLESTR pszFileName, DWORD dwMode)
{
	UNUSED(dwMode);	// TODO

	// pszFileName is the file being worked on.
	RP_D(RP_ExtractImage);
	d->filename = W2RP_c(pszFileName);
	return S_OK;
}

IFACEMETHODIMP RP_ExtractImage::Save(LPCOLESTR pszFileName, BOOL fRemember)
{
	UNUSED(pszFileName);
	UNUSED(fRemember);
	return E_NOTIMPL;
}

IFACEMETHODIMP RP_ExtractImage::SaveCompleted(LPCOLESTR pszFileName)
{
	UNUSED(pszFileName);
	return E_NOTIMPL;
}

IFACEMETHODIMP RP_ExtractImage::GetCurFile(LPOLESTR *ppszFileName)
{
	UNUSED(ppszFileName);
	return E_NOTIMPL;
}

/** IExtractImage **/
// References:
// - https://msdn.microsoft.com/en-us/library/windows/desktop/bb761848(v=vs.85).aspx
// - http://www.codeproject.com/Articles/2887/Create-Thumbnail-Extractor-objects-for-your-MFC-do

IFACEMETHODIMP RP_ExtractImage::GetLocation(LPWSTR pszPathBuffer,
	DWORD cchMax, DWORD *pdwPriority, const SIZE *prgSize,
	DWORD dwRecClrDepth, DWORD *pdwFlags)
{
	// TODO: If the image is cached on disk, return a filename.
	if (!prgSize || !pdwFlags) {
		// Invalid arguments.
		return E_INVALIDARG;
	} else if ((*pdwFlags & IEIFLAG_ASYNC) && !pdwPriority) {
		// NOTE: On Windows XP, pdwPriority must not be NULL,
		// even if IEIFLAG_ASYNC isn't set. Later versions
		// simply ignore this parameter, so we're only checking
		// it if IEIFLAG_ASYNC is set.

		// pdwPriority must be specified if IEIFLAG_ASYNC is set.
		return E_INVALIDARG;
	}

	// Save the image size for later.
	RP_D(RP_ExtractImage);
	d->rgSize = *prgSize;
	d->dwRecClrDepth = dwRecClrDepth;
	d->dwFlags = *pdwFlags;

	// Disable the border around the thumbnail.
	// NOTE: Might not work on Vista+.
	*pdwFlags |= IEIFLAG_NOBORDER;

#ifndef NDEBUG
	// Debug version. Don't cache images.
	// (Windows XP and earlier.)
	*pdwFlags |= IEIFLAG_CACHE;
#endif /* NDEBUG */

	// If IEIFLAG_ASYNC is specified, return E_PENDING to let
	// the calling process know it can call Extract() from a
	// background thread. If this isn't done, then Explorer
	// will lock up until all images are downloaded.
	// NOTE: Explorer in Windows Vista and later always seems to
	// call Extract() from a background thread.

	// FIXME: Returning E_PENDING seems to cause a crash in
	// WinXP shell32.dll: CExtractImageTask::~CExtractImageTask.
	//return (*pdwFlags & IEIFLAG_ASYNC) ? E_PENDING : S_OK;
	return S_OK;
}

IFACEMETHODIMP RP_ExtractImage::Extract(HBITMAP *phBmpImage)
{
	// Verify parameters:
	// - A filename must have been set by calling IPersistFile::Load().
	// - phBmpImage must not be nullptr.
	RP_D(RP_ExtractImage);
	if (d->filename.empty() || !phBmpImage) {
		return E_INVALIDARG;
	}
	*phBmpImage = nullptr;

	// NOTE: Using width only. (TODO: both width/height?)
	int ret = d->getThumbnail(d->filename.c_str(), d->rgSize.cx, *phBmpImage);
	if (ret != 0) {
		// ROM is not supported. Use the fallback.
		return d->Fallback(phBmpImage);
	}
	return S_OK;
}

/** IExtractImage2 **/

/**
 * Get the timestamp of the file.
 * @param pDateStamp	[out] Pointer to FILETIME to store the timestamp in.
 * @return COM error code.
 */
IFACEMETHODIMP RP_ExtractImage::GetDateStamp(FILETIME *pDateStamp)
{
	RP_D(RP_ExtractImage);
	if (!pDateStamp) {
		// No FILETIME pointer specified.
		return E_POINTER;
	} else if (d->filename.empty()) {
		// Filename was not set in GetLocation().
		return E_INVALIDARG;
	}

	// open the file and get last write time
	HANDLE hFile = CreateFile(RP2W_s(d->filename),
		GENERIC_READ, FILE_SHARE_READ, NULL,
		OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (!hFile) {
		// Could not open the file.
		// TODO: Return STG_E_FILENOTFOUND?
		return E_FAIL;
	}

	FILETIME ftLastWriteTime;
	BOOL bRet = GetFileTime(hFile, nullptr, nullptr, &ftLastWriteTime);
	CloseHandle(hFile);
	if (!bRet) {
		// Failed to retrieve the timestamp.
		return E_FAIL;
	}

	SYSTEMTIME stUTC, stLocal;
	FileTimeToSystemTime(&ftLastWriteTime, &stUTC);
	SystemTimeToTzSpecificLocalTime(nullptr, &stUTC, &stLocal);

	*pDateStamp = ftLastWriteTime;
	return NOERROR; 
}
