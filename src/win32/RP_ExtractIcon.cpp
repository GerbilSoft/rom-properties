/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * RP_ExtractIcon.cpp: IExtractIcon implementation.                        *
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
#include "RP_ExtractIcon.hpp"
#include "RpImageWin32.hpp"

// libromdata
#include "libromdata/RomData.hpp"
#include "libromdata/RomDataFactory.hpp"
#include "libromdata/RpWin32.hpp"
#include "libromdata/file/RpFile.hpp"
#include "libromdata/img/rp_image.hpp"
#include "libromdata/img/RpGdiplusBackend.hpp"
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
const CLSID CLSID_RP_ExtractIcon =
	{0xe51bc107, 0xe491, 0x4b29, {0xa6, 0xa3, 0x2a, 0x43, 0x09, 0x25, 0x98, 0x02}};

/** RP_ExtractIcon_Private **/
#include "RP_ExtractIcon_p.hpp"

// TCreateThumbnail is a templated class,
// so we have to #include the .cpp file here.
#include "libromdata/img/TCreateThumbnail.cpp"

RP_ExtractIcon_Private::RP_ExtractIcon_Private()
	: romData(nullptr)
{ }

RP_ExtractIcon_Private::~RP_ExtractIcon_Private()
{
	if (romData) {
		romData->unref();
	}
}

/**
 * Wrapper function to convert rp_image* to ImgClass.
 * @param img rp_image
 * @return ImgClass.
 */
HBITMAP RP_ExtractIcon_Private::rpImageToImgClass(const rp_image *img) const
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
bool RP_ExtractIcon_Private::isImgClassValid(const HBITMAP &imgClass) const
{
	return (imgClass != nullptr);
}

/**
 * Wrapper function to get a "null" ImgClass.
 * @return "Null" ImgClass.
 */
HBITMAP RP_ExtractIcon_Private::getNullImgClass(void) const
{
	return nullptr;
}

/**
 * Free an ImgClass object.
 * @param imgClass ImgClass object.
 */
void RP_ExtractIcon_Private::freeImgClass(HBITMAP &imgClass) const
{
	DeleteObject(imgClass);
}

/**
 * Rescale an ImgClass using nearest-neighbor scaling.
 * @param imgClass ImgClass object.
 * @param sz New size.
 * @return Rescaled ImgClass.
 */
HBITMAP RP_ExtractIcon_Private::rescaleImgClass(const HBITMAP &imgClass, const ImgSize &sz) const
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
rp_string RP_ExtractIcon_Private::proxyForUrl(const rp_string &url) const
{
	// libcachemgr uses urlmon on Windows, which
	// always uses the system proxy.
	return rp_string();
}

/** RP_ExtractIcon **/

RP_ExtractIcon::RP_ExtractIcon()
	: d_ptr(new RP_ExtractIcon_Private())
{ }

RP_ExtractIcon::~RP_ExtractIcon()
{
	delete d_ptr;
}

/** IUnknown **/
// Reference: https://msdn.microsoft.com/en-us/library/office/cc839627.aspx

IFACEMETHODIMP RP_ExtractIcon::QueryInterface(REFIID riid, LPVOID *ppvObj)
{
	static const QITAB rgqit[] = {
		QITABENT(RP_ExtractIcon, IPersist),
		QITABENT(RP_ExtractIcon, IPersistFile),
		QITABENT(RP_ExtractIcon, IExtractIconW),
		QITABENT(RP_ExtractIcon, IExtractIconA),
		{ 0, 0 }
	};
	return pQISearch(this, rgqit, riid, ppvObj);
}

/** IPersistFile **/
// Reference: https://msdn.microsoft.com/en-us/library/windows/desktop/cc144067(v=vs.85).aspx#unknown_28177

IFACEMETHODIMP RP_ExtractIcon::GetClassID(CLSID *pClassID)
{
	if (!pClassID) {
		return E_FAIL;
	}
	*pClassID = CLSID_RP_ExtractIcon;
	return S_OK;
}

IFACEMETHODIMP RP_ExtractIcon::IsDirty(void)
{
	return E_NOTIMPL;
}

IFACEMETHODIMP RP_ExtractIcon::Load(LPCOLESTR pszFileName, DWORD dwMode)
{
	UNUSED(dwMode);	// TODO

	// If we already have a RomData object, unref() it first.
	RP_D(RP_ExtractIcon);
	if (d->romData) {
		d->romData->unref();
		d->romData = nullptr;
	}

	// pszFileName is the file being worked on.
	// TODO: If the file was already loaded, don't reload it.
	d->filename = W2RP_cs(pszFileName);

	// Attempt to open the ROM file.
	unique_ptr<IRpFile> file(new RpFile(d->filename, RpFile::FM_OPEN_READ));
	if (!file || !file->isOpen()) {
		return E_FAIL;
	}

	// Get the appropriate RomData class for this ROM.
	// RomData class *must* support at least one image type.
	d->romData = RomDataFactory::create(file.get(), true);

	// NOTE: Since this is the registered icon handler
	// for the file type, we have to implement our own
	// fallbacks for unsupported files. Hence, we'll
	// continue even if d->romData is nullptr;
	return S_OK;
}

IFACEMETHODIMP RP_ExtractIcon::Save(LPCOLESTR pszFileName, BOOL fRemember)
{
	UNUSED(pszFileName);
	UNUSED(fRemember);
	return E_NOTIMPL;
}

IFACEMETHODIMP RP_ExtractIcon::SaveCompleted(LPCOLESTR pszFileName)
{
	UNUSED(pszFileName);
	return E_NOTIMPL;
}

IFACEMETHODIMP RP_ExtractIcon::GetCurFile(LPOLESTR *ppszFileName)
{
	UNUSED(ppszFileName);
	return E_NOTIMPL;
}

/** IExtractIconW **/
// Reference: https://msdn.microsoft.com/en-us/library/windows/desktop/bb761854(v=vs.85).aspx

IFACEMETHODIMP RP_ExtractIcon::GetIconLocation(UINT uFlags,
	LPWSTR pszIconFile, UINT cchMax, int *piIndex, UINT *pwFlags)
{
	// TODO: If the icon is cached on disk, return a filename.
	// TODO: Enable ASYNC?
	// - https://msdn.microsoft.com/en-us/library/windows/desktop/bb761852(v=vs.85).aspx
	if (!pszIconFile || !piIndex || cchMax == 0) {
		return E_INVALIDARG;
	}
	UNUSED(uFlags);

	// If the file wasn't set via IPersistFile::Load(), that's an error.
	RP_D(RP_ExtractIcon);
	if (d->filename.empty()) {
		return E_UNEXPECTED;
	}

	// NOTE: If caching is enabled and we don't set pszIconFile
	// and piIndex, all icons for files handled by rom-properties
	// will be the first file Explorer hands off to the extension.
	//
	// If we enable caching and set pszIconFile and piIndex, it
	// effectively disables caching anyway, since it ends up
	// calling Extract() the first time a file is encountered
	// in an Explorer session.
	//
	// TODO: Implement our own icon caching?
	// TODO: Set pszIconFile[] and piIndex to something else?
	pszIconFile[0] = 0;
	*piIndex = 0;
	*pwFlags = GIL_NOTFILENAME | GIL_DONTCACHE;
	return S_OK;
}

IFACEMETHODIMP RP_ExtractIcon::Extract(LPCWSTR pszFile, UINT nIconIndex,
	HICON *phiconLarge, HICON *phiconSmall, UINT nIconSize)
{
	// TODO: Use TCreateThumbnail()?

	// NOTE: pszFile and nIconIndex were set in GetIconLocation().
	// TODO: Validate them to make sure they're the same values
	// we returned in GetIconLocation()?
	UNUSED(pszFile);
	UNUSED(nIconIndex);

	// TODO: Use nIconSize outside of fallback?

	// Make sure a filename was set by calling IPersistFile::Load().
	RP_D(RP_ExtractIcon);
	if (d->filename.empty()) {
		return E_UNEXPECTED;
	}

	// phiconLarge must be valid.
	if (!phiconLarge) {
		return E_INVALIDARG;
	}

	if (!d->romData) {
		// ROM is not supported. Use the fallback.
		LONG lResult = d->Fallback(phiconLarge, phiconSmall, nIconSize);
		// NOTE: S_FALSE causes icon shenanigans.
		return (lResult == ERROR_SUCCESS ? S_OK : E_FAIL);
	}

	// ROM is supported. Get the image.
	// TODO: Small icon?
	HBITMAP hBmpImage = nullptr;
	int ret = d->getThumbnail(d->romData, LOWORD(nIconSize), hBmpImage);
	if (ret != 0 || !hBmpImage) {
		// Thumbnail not available. Use the fallback.
		if (hBmpImage) {
			DeleteObject(hBmpImage);
			hBmpImage = nullptr;
		}
		LONG lResult = d->Fallback(phiconLarge, phiconSmall, nIconSize);
		// NOTE: S_FALSE causes icon shenanigans.
		return (lResult == ERROR_SUCCESS ? S_OK : E_FAIL);
	}

	// Convert the HBITMAP to an HICON.
	HICON hIcon = RpImageWin32::toHICON(hBmpImage);
	if (hIcon != nullptr) {
		// Icon converted.
		bool iconWasSet = false;
		if (phiconLarge) {
			*phiconLarge = hIcon;
			iconWasSet = true;
		}
		if (phiconSmall) {
			// TODO: Support the small icon?
			// NULL out the small icon.
			*phiconSmall = nullptr;
		}

		if (!iconWasSet) {
			// Not returning the icon.
			// Delete it to prevent a resource leak.
			DeleteObject(hIcon);
		}
	} else {
		// Error converting to HICON.
		// Use the fallback.
		DeleteObject(hBmpImage);
		hBmpImage = nullptr;
		LONG lResult = d->Fallback(phiconLarge, phiconSmall, nIconSize);
		// NOTE: S_FALSE causes icon shenanigans.
		return (lResult == ERROR_SUCCESS ? S_OK : E_FAIL);
	}

	// NOTE: S_FALSE causes icon shenanigans.
	// TODO: Always return success here due to fallback?
	return (*phiconLarge != nullptr ? S_OK : E_FAIL);
}

/** IExtractIconA **/
// Reference: https://msdn.microsoft.com/en-us/library/windows/desktop/bb761854(v=vs.85).aspx

IFACEMETHODIMP RP_ExtractIcon::GetIconLocation(UINT uFlags,
	LPSTR pszIconFile, UINT cchMax, int *piIndex, UINT *pwFlags)
{
	// NOTE: pszIconFile is always blanked out in the IExtractIconW
	// interface, so no conversion is necessary. We still need a
	// temporary buffer, though.
	if (!pszIconFile || !piIndex || cchMax == 0) {
		// We're still expecting valid parameters...
		return E_INVALIDARG;
	}
	wchar_t buf[16];
	HRESULT hr = GetIconLocation(uFlags, buf, ARRAY_SIZE(buf), piIndex, pwFlags);
	pszIconFile[0] = 0;	// Blank it out.
	return hr;
}

IFACEMETHODIMP RP_ExtractIcon::Extract(LPCSTR pszFile, UINT nIconIndex,
	HICON *phiconLarge, HICON *phiconSmall, UINT nIconSize)
{
	// NOTE: The IExtractIconW interface doesn't use pszFile,
	// so no conversion is necessary.
	return Extract(L"", nIconIndex, phiconLarge, phiconSmall, nIconSize);
}
