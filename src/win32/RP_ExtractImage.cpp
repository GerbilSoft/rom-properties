/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * RP_ExtractImage.hpp: IExtractImage implementation.                      *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

// Reference: http://www.codeproject.com/Articles/338268/COM-in-C
#include "stdafx.h"
#include "RP_ExtractImage.hpp"
#include "RpImageWin32.hpp"

// librpbase, librpfile, librptexture, libromdata
using namespace LibRpBase;
using namespace LibRpFile;
using LibRpTexture::rp_image;
using LibRomData::RomDataFactory;

// C++ STL classes.
using std::string;
using std::tstring;

// CLSID
const CLSID CLSID_RP_ExtractImage =
	{0x84573bc0, 0x9502, 0x42f8, {0x80, 0x66, 0xCC, 0x52, 0x7D, 0x07, 0x79, 0xE5}};

/** RP_ExtractImage_Private **/
#include "RP_ExtractImage_p.hpp"

RP_ExtractImage_Private::RP_ExtractImage_Private()
	: filename(nullptr)
	, romData(nullptr)
	, dwRecClrDepth(0)
	, dwFlags(0)
{
	rgSize.cx = 0;
	rgSize.cy = 0;
}

RP_ExtractImage_Private::~RP_ExtractImage_Private()
{
	free(filename);
	UNREF(romData);
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
// Reference: https://docs.microsoft.com/en-us/office/client-developer/outlook/mapi/implementing-iunknown-in-c-plus-plus

IFACEMETHODIMP RP_ExtractImage::QueryInterface(_In_ REFIID riid, _Outptr_ LPVOID *ppvObj)
{
#ifdef _MSC_VER
#  pragma warning(push)
#  pragma warning(disable: 4365 4838)
#endif /* _MSC_VER */
	static const QITAB rgqit[] = {
		QITABENT(RP_ExtractImage, IPersist),
		QITABENT(RP_ExtractImage, IPersistFile),
		QITABENT(RP_ExtractImage, IExtractImage),
		QITABENT(RP_ExtractImage, IExtractImage2),
		{ 0, 0 }
	};
#ifdef _MSC_VER
#  pragma warning(pop)
#endif /* _MSC_VER */
	return LibWin32Common::rp_QISearch(this, rgqit, riid, ppvObj);
}

/** IPersistFile **/
// Reference: https://docs.microsoft.com/en-us/windows/win32/shell/handlers

IFACEMETHODIMP RP_ExtractImage::GetClassID(_Out_ CLSID *pClassID)
{
	if (!pClassID) {
		return E_POINTER;
	}
	*pClassID = CLSID_RP_ExtractImage;
	return S_OK;
}

IFACEMETHODIMP RP_ExtractImage::IsDirty(void)
{
	return E_NOTIMPL;
}

IFACEMETHODIMP RP_ExtractImage::Load(_In_ LPCOLESTR pszFileName, DWORD dwMode)
{
	RP_UNUSED(dwMode);	// TODO

	// If we already have a RomData object, unref() it first.
	RP_D(RP_ExtractImage);
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

	// NOTE: Since this is the registered image extractor
	// for the file type, we have to implement our own
	// fallbacks for unsupported files. Hence, we'll
	// continue even if d->romData is nullptr;
	return S_OK;
}

IFACEMETHODIMP RP_ExtractImage::Save(_In_ LPCOLESTR pszFileName, BOOL fRemember)
{
	RP_UNUSED(pszFileName);
	RP_UNUSED(fRemember);
	return E_NOTIMPL;
}

IFACEMETHODIMP RP_ExtractImage::SaveCompleted(_In_ LPCOLESTR pszFileName)
{
	RP_UNUSED(pszFileName);
	return E_NOTIMPL;
}

IFACEMETHODIMP RP_ExtractImage::GetCurFile(_Outptr_ LPOLESTR *ppszFileName)
{
	if (!ppszFileName)
		return E_POINTER;

	RP_D(const RP_ExtractImage);
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
		const wstring wfilename = U82W_c(d->filename);
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

/** IExtractImage **/
// References:
// - https://docs.microsoft.com/en-us/windows/win32/api/shobjidl_core/nn-shobjidl_core-iextractimage
// - http://www.codeproject.com/Articles/2887/Create-Thumbnail-Extractor-objects-for-your-MFC-do

IFACEMETHODIMP RP_ExtractImage::GetLocation(
	_Out_writes_(cchMax) LPWSTR pszPathBuffer, DWORD cchMax,
	_Out_ DWORD *pdwPriority, _In_ const SIZE *prgSize,
	DWORD dwRecClrDepth, _Inout_ DWORD *pdwFlags)
{
	((void)pszPathBuffer);
	((void)cchMax);

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

	// Priority flag: Only used on Windows XP when using
	// IEIFLAG_ASYNC, but we should set it regardless.
	// MSDN says it cannot be NULL, but we'll check for NULL
	// anyway because it's not useful nowadays.
	if (pdwPriority) {
		*pdwPriority = IEIT_PRIORITY_NORMAL;
	}

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

IFACEMETHODIMP RP_ExtractImage::Extract(_Outptr_ HBITMAP *phBmpImage)
{
	// Make sure a filename was set by calling IPersistFile::Load().
	RP_D(RP_ExtractImage);
	if (!d->filename || d->filename[0] == '\0') {
		return E_UNEXPECTED;
	}

	// phBmpImage must be valid.
	if (!phBmpImage) {
		return E_INVALIDARG;
	}
	*phBmpImage = nullptr;

	if (!d->romData) {
		// ROM is not supported. Use the fallback.
		return d->Fallback(phBmpImage);
	}

	// ROM is supported. Get the image.
	// NOTE: Using width only. (TODO: both width/height?)
	CreateThumbnail::GetThumbnailOutParams_t outParams;
	int ret = d->thumbnailer.getThumbnail(d->romData, d->rgSize.cx, &outParams);
	if (ret != 0) {
		// ROM is not supported. Use the fallback.
		return d->Fallback(phBmpImage);
	}
	*phBmpImage = outParams.retImg;
	return S_OK;
}

/** IExtractImage2 **/

/**
 * Get the timestamp of the file.
 * @param pDateStamp	[out] Pointer to FILETIME to store the timestamp in.
 * @return COM error code.
 */
IFACEMETHODIMP RP_ExtractImage::GetDateStamp(_Out_ FILETIME *pDateStamp)
{
	RP_D(RP_ExtractImage);
	if (!pDateStamp) {
		// No FILETIME pointer specified.
		return E_POINTER;
	} else if (!d->filename || d->filename[0] == '\0') {
		// Filename was not set in GetLocation().
		return E_INVALIDARG;
	}

	// Open the file and get the last write time.
	// NOTE: LibRpBase::FileSystem::get_mtime() exists,
	// but its resolution is seconds, less than FILETIME.
	HANDLE hFile = CreateFile(U82T_s(d->filename),
		GENERIC_READ, FILE_SHARE_READ, nullptr,
		OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
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
