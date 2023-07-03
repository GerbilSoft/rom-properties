/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * RP_ExtractIcon.cpp: IExtractIcon implementation.                        *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

// Reference: http://www.codeproject.com/Articles/338268/COM-in-C
#include "stdafx.h"
#include "RP_ExtractIcon.hpp"
#include "RpImageWin32.hpp"

// librpbase, librpfile, librptexture, libromdata
using namespace LibRpBase;
using namespace LibRpFile;
using LibRpTexture::rp_image;
using LibRomData::RomDataFactory;

// C++ STL classes.
using std::wstring;

// CLSID
const CLSID CLSID_RP_ExtractIcon =
	{0xe51bc107, 0xe491, 0x4b29, {0xa6, 0xa3, 0x2a, 0x43, 0x09, 0x25, 0x98, 0x02}};

/** RP_ExtractIcon_Private **/
#include "RP_ExtractIcon_p.hpp"

RP_ExtractIcon_Private::RP_ExtractIcon_Private()
	: filename(nullptr)
	, romData(nullptr)
{ }

RP_ExtractIcon_Private::~RP_ExtractIcon_Private()
{
	free(filename);
	UNREF(romData);
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
// Reference: https://docs.microsoft.com/en-us/office/client-developer/outlook/mapi/implementing-iunknown-in-c-plus-plus

IFACEMETHODIMP RP_ExtractIcon::QueryInterface(_In_ REFIID riid, _Outptr_ LPVOID *ppvObj)
{
#ifdef _MSC_VER
#  pragma warning(push)
#  pragma warning(disable: 4365 4838)
#endif /* _MSC_VER */
	static const QITAB rgqit[] = {
		QITABENT(RP_ExtractIcon, IPersist),
		QITABENT(RP_ExtractIcon, IPersistFile),
		QITABENT(RP_ExtractIcon, IExtractIconW),
		QITABENT(RP_ExtractIcon, IExtractIconA),
		{ 0, 0 }
	};
#ifdef _MSC_VER
#  pragma warning(pop)
#endif /* _MSC_VER */
	return LibWin32Common::rp_QISearch(this, rgqit, riid, ppvObj);
}

/** IPersistFile **/
// Reference: https://docs.microsoft.com/en-us/windows/win32/shell/handlers

IFACEMETHODIMP RP_ExtractIcon::GetClassID(_Out_ CLSID *pClassID)
{
	if (!pClassID) {
		return E_POINTER;
	}
	*pClassID = CLSID_RP_ExtractIcon;
	return S_OK;
}

IFACEMETHODIMP RP_ExtractIcon::IsDirty(void)
{
	return E_NOTIMPL;
}

IFACEMETHODIMP RP_ExtractIcon::Load(_In_ LPCOLESTR pszFileName, DWORD dwMode)
{
	RP_UNUSED(dwMode);	// TODO

	// If we already have a RomData object, unref() it first.
	RP_D(RP_ExtractIcon);
	UNREF_AND_NULL(d->romData);

	// pszFileName is the file being worked on.
	// TODO: If the file was already loaded, don't reload it.
	free(d->filename);
	d->filename = strdup(W2U8(pszFileName).c_str());
	if (!d->filename) {
		return E_OUTOFMEMORY;
	}

	// Check for "bad" file systems.
	const Config *const config = Config::instance();
	if (FileSystem::isOnBadFS(d->filename, config->enableThumbnailOnNetworkFS())) {
		// This file is on a "bad" file system.
		return E_FAIL;
	}

	// Attempt to open the ROM file.
	RpFile *const file = new RpFile(d->filename, RpFile::FM_OPEN_READ_GZ);
	if (!file->isOpen()) {
		// Unable to open the file.
		file->unref();
		return E_FAIL;
	}

	// Get the appropriate RomData class for this ROM.
	// RomData class *must* support at least one image type.
	d->romData = RomDataFactory::create(file, RomDataFactory::RDA_HAS_THUMBNAIL);
	file->unref();

	// NOTE: Since this is the registered icon handler
	// for the file type, we have to implement our own
	// fallbacks for unsupported files. Hence, we'll
	// continue even if d->romData is nullptr;
	return S_OK;
}

IFACEMETHODIMP RP_ExtractIcon::Save(_In_ LPCOLESTR pszFileName, BOOL fRemember)
{
	RP_UNUSED(pszFileName);
	RP_UNUSED(fRemember);
	return E_NOTIMPL;
}

IFACEMETHODIMP RP_ExtractIcon::SaveCompleted(_In_ LPCOLESTR pszFileName)
{
	RP_UNUSED(pszFileName);
	return E_NOTIMPL;
}

IFACEMETHODIMP RP_ExtractIcon::GetCurFile(_In_ LPOLESTR *ppszFileName)
{
	if (!ppszFileName)
		return E_POINTER;

	RP_D(const RP_ExtractIcon);
	if (!d->filename) {
		// No filename. Create an empty string.
		LPWSTR psz = static_cast<LPWSTR>(CoTaskMemAlloc(sizeof(wchar_t)));
		if (!psz) {
			*ppszFileName = nullptr;
			return E_OUTOFMEMORY;
		}
		*psz = L'\0';
		*ppszFileName = psz;
	} else {
		// Convert the filename to UTF-16 first and then copy it.
		wstring wfilename = U82W_c(d->filename);
		const size_t cb = (wfilename.size() + 1) * sizeof(wchar_t);
		LPWSTR psz = static_cast<LPWSTR>(CoTaskMemAlloc(cb));
		if (!psz) {
			*ppszFileName = nullptr;
			return E_OUTOFMEMORY;
		}
		memcpy(psz, wfilename.c_str(), cb);
		*ppszFileName = psz;
	}

	return S_OK;
}

/** IExtractIconW **/
// Reference: https://docs.microsoft.com/en-us/windows/win32/api/shlobj_core/nn-shlobj_core-iextracticonw

IFACEMETHODIMP RP_ExtractIcon::GetIconLocation(UINT uFlags,
	_Out_writes_(cchMax) LPWSTR pszIconFile, UINT cchMax,
	_Out_ int *piIndex, _Out_ UINT *pwFlags)
{
	// TODO: If the icon is cached on disk, return a filename.
	// TODO: Enable ASYNC?
	// - https://docs.microsoft.com/en-us/windows/win32/api/shlobj_core/nf-shlobj_core-iextracticonw-geticonlocation
	RP_UNUSED(uFlags);
	if (!pszIconFile || !piIndex || cchMax == 0) {
		return E_INVALIDARG;
	}

	// If the file wasn't set via IPersistFile::Load(), that's an error.
	RP_D(RP_ExtractIcon);
	if (!d->filename || d->filename[0] == '\0') {
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
	pszIconFile[0] = L'\0';
	*piIndex = 0;
	*pwFlags = GIL_NOTFILENAME | GIL_DONTCACHE;
	return S_OK;
}

IFACEMETHODIMP RP_ExtractIcon::Extract(_In_ LPCWSTR pszFile, UINT nIconIndex,
	_Outptr_opt_ HICON *phiconLarge, _Outptr_opt_ HICON *phiconSmall, UINT nIconSize)
{
	// NOTE: pszFile and nIconIndex were set in GetIconLocation().
	// TODO: Validate them to make sure they're the same values
	// we returned in GetIconLocation()?
	RP_UNUSED(pszFile);
	RP_UNUSED(nIconIndex);

	// TODO: Use nIconSize outside of fallback?

	// Make sure a filename was set by calling IPersistFile::Load().
	RP_D(RP_ExtractIcon);
	if (!d->filename || d->filename[0] == '\0') {
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
	CreateThumbnail::GetThumbnailOutParams_t outParams;
	int ret = d->thumbnailer.getThumbnail(d->romData, LOWORD(nIconSize), &outParams);
	if (ret != 0) {
		// Thumbnail not available. Use the fallback.
		if (outParams.retImg) {
			DeleteBitmap(outParams.retImg);
		}
		LONG lResult = d->Fallback(phiconLarge, phiconSmall, nIconSize);
		// NOTE: S_FALSE causes icon shenanigans.
		return (lResult == ERROR_SUCCESS ? S_OK : E_FAIL);
	}

	// Convert the HBITMAP to an HICON.
	HICON hIcon = RpImageWin32::toHICON(outParams.retImg);
	DeleteBitmap(outParams.retImg);
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
			DestroyIcon(hIcon);
		}
	} else {
		// Error converting to HICON.
		// Use the fallback.
		LONG lResult = d->Fallback(phiconLarge, phiconSmall, nIconSize);
		// NOTE: S_FALSE causes icon shenanigans.
		return (lResult == ERROR_SUCCESS ? S_OK : E_FAIL);
	}

	// NOTE: S_FALSE causes icon shenanigans.
	// TODO: Always return success here due to fallback?
	return (*phiconLarge != nullptr ? S_OK : E_FAIL);
}

/** IExtractIconA **/
// Reference: https://docs.microsoft.com/en-us/windows/win32/api/shlobj_core/nn-shlobj_core-iextracticona

IFACEMETHODIMP RP_ExtractIcon::GetIconLocation(UINT uFlags,
	_Out_writes_(cchMax) LPSTR pszIconFile, UINT cchMax,
	_Out_ int *piIndex, _Out_ UINT *pwFlags)
{
	// NOTE: pszIconFile is always blanked out in the IExtractIconW
	// interface, so no conversion is necessary. We still need a
	// temporary buffer, though.
	if (!pszIconFile || !piIndex || cchMax == 0) {
		// We're still expecting valid parameters...
		return E_INVALIDARG;
	}
	wchar_t buf[16];
	HRESULT hr = GetIconLocation(uFlags, buf, _countof(buf), piIndex, pwFlags);
	pszIconFile[0] = 0;	// Blank it out.
	return hr;
}

IFACEMETHODIMP RP_ExtractIcon::Extract(_In_ LPCSTR pszFile, UINT nIconIndex,
	_Outptr_opt_ HICON *phiconLarge, _Outptr_opt_ HICON *phiconSmall, UINT nIconSize)
{
	// NOTE: The IExtractIconW interface doesn't use pszFile,
	// so no conversion is necessary.
	((void)pszFile);
	return Extract(L"", nIconIndex, phiconLarge, phiconSmall, nIconSize);
}
