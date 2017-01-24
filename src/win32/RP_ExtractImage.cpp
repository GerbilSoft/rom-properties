/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * RP_ExtractImage.hpp: IExtractImage implementation.                      *
 *                                                                         *
 * Copyright (c) 2016 by David Korth.                                      *
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
#include "RegKey.hpp"
#include "RpImageWin32.hpp"

// libromdata
#include "libromdata/RomData.hpp"
#include "libromdata/RomDataFactory.hpp"
#include "libromdata/RpWin32.hpp"
#include "libromdata/file/RpFile.hpp"
#include "libromdata/img/rp_image.hpp"
using namespace LibRomData;

// TCreateThumbnail is a templated class,
// so we have to #include the .cpp file here.
#include "libromdata/img/TCreateThumbnail.cpp"

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
// Workaround for RP_D() expecting the no-underscore naming convention.
#define RP_ExtractImagePrivate RP_ExtractImage_Private

class RP_ExtractImage_Private : public TCreateThumbnail<HBITMAP>
{
	public:
		RP_ExtractImage_Private();
		virtual ~RP_ExtractImage_Private();

	private:
		typedef TCreateThumbnail<HBITMAP> super;
		RP_DISABLE_COPY(RP_ExtractImage_Private)

	public:
		// ROM filename from IPersistFile::Load().
		rp_string filename;

		// Requested thumbnail size from IExtractImage::GetLocation().
		SIZE bmSize;

	public:
		/** TCreateThumbnail functions. **/

		/**
		 * Wrapper function to convert rp_image* to ImgClass.
		 * @param img rp_image
		 * @return ImgClass
		 */
		virtual HBITMAP rpImageToImgClass(const rp_image *img) const final;

		/**
		 * Wrapper function to check if an ImgClass is valid.
		 * @param imgClass ImgClass
		 * @return True if valid; false if not.
		 */
		virtual bool isImgClassValid(const HBITMAP &imgClass) const final;

		/**
		 * Wrapper function to get a "null" ImgClass.
		 * @return "Null" ImgClass.
		 */
		virtual HBITMAP getNullImgClass(void) const final;

		/**
		 * Free an ImgClass object.
		 * This may be no-op for e.g. HBITMAP.
		 * @param imgClass ImgClass object.
		 */
		virtual void freeImgClass(HBITMAP &imgClass) const final;

		/**
		 * Get an ImgClass's size.
		 * @param imgClass ImgClass object.
		 * @retrun Size.
		 */
		virtual ImgSize getImgSize(const HBITMAP &imgClass) const final;

		/**
		 * Rescale an ImgClass using nearest-neighbor scaling.
		 * @param imgClass ImgClass object.
		 * @param sz New size.
		 * @return Rescaled ImgClass.
		 */
		virtual HBITMAP rescaleImgClass(const HBITMAP &imgClass, const ImgSize &sz) const final;

		/**
		 * Get the proxy for the specified URL.
		 * @return Proxy, or empty string if no proxy is needed.
		 */
		virtual rp_string proxyForUrl(const rp_string &url) const final;
};

/** RP_ExtractImage_Private **/

RP_ExtractImage_Private::RP_ExtractImage_Private()
{
	bmSize.cx = 0;
	bmSize.cy = 0;
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
 * Get an ImgClass's size.
 * @param imgClass ImgClass object.
 * @retrun Size.
 */
RP_ExtractImage_Private::ImgSize RP_ExtractImage_Private::getImgSize(const HBITMAP &imgClass) const
{
	BITMAP bm;
	if (GetObject(imgClass, sizeof(bm), &bm) == 0) {
		// Error retrieving the bitmap information.
		static const ImgSize sz = {0, 0};
		return sz;
	}

	const ImgSize sz = {bm.bmWidth, bm.bmHeight};
	return sz;
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
{
	RP_D(RP_ExtractImage);
	d->bmSize.cx = 0;
	d->bmSize.cy = 0;
}

RP_ExtractImage::~RP_ExtractImage()
{
	delete d_ptr;
}

/** IUnknown **/
// Reference: https://msdn.microsoft.com/en-us/library/office/cc839627.aspx

IFACEMETHODIMP RP_ExtractImage::QueryInterface(REFIID riid, LPVOID *ppvObj)
{
	// Always set out parameter to NULL, validating it first.
	if (!ppvObj)
		return E_INVALIDARG;

	// Check if this interface is supported.
	// NOTE: static_cast<> is required due to vtable shenanigans.
	// Also, IID_IUnknown must always return the same pointer.
	// References:
	// - http://stackoverflow.com/questions/1742848/why-exactly-do-i-need-an-explicit-upcast-when-implementing-queryinterface-in-a
	// - http://stackoverflow.com/a/2812938
	if (riid == IID_IUnknown || riid == IID_IExtractImage) {
		*ppvObj = static_cast<IExtractImage*>(this);
	} else if (riid == IID_IExtractImage2) {
		*ppvObj = static_cast<IExtractImage2*>(this);
	} else if (riid == IID_IPersist) {
		*ppvObj = static_cast<IPersist*>(this);
	} else if (riid == IID_IPersistFile) {
		*ppvObj = static_cast<IPersistFile*>(this);
	} else {
		// Interface is not supported.
		*ppvObj = nullptr;
		return E_NOINTERFACE;
	}

	// Make sure we count this reference.
	AddRef();
	return NOERROR;
}

/**
 * Register the COM object.
 * @return ERROR_SUCCESS on success; Win32 error code on error.
 */
LONG RP_ExtractImage::RegisterCLSID(void)
{
	static const wchar_t description[] = L"ROM Properties Page - Image Extractor";
	extern const wchar_t RP_ProgID[];

	// Convert the CLSID to a string.
	wchar_t clsid_str[48];	// maybe only 40 is needed?
	LONG lResult = StringFromGUID2(__uuidof(RP_ExtractImage), clsid_str, sizeof(clsid_str)/sizeof(clsid_str[0]));
	if (lResult <= 0) {
		return ERROR_INVALID_PARAMETER;
	}

	// Register the COM object.
	lResult = RegKey::RegisterComObject(__uuidof(RP_ExtractImage), RP_ProgID, description);
	if (lResult != ERROR_SUCCESS) {
		return lResult;
	}

	// Register as an "approved" shell extension.
	lResult = RegKey::RegisterApprovedExtension(__uuidof(RP_ExtractImage), description);
	if (lResult != ERROR_SUCCESS) {
		return lResult;
	}

	// COM object registered.
	return ERROR_SUCCESS;
}

/**
 * Register the file type handler.
 * @param hkey_Assoc File association key to register under.
 * @return ERROR_SUCCESS on success; Win32 error code on error.
 */
LONG RP_ExtractImage::RegisterFileType(RegKey &hkey_Assoc)
{
	extern const wchar_t RP_ProgID[];

	// Convert the CLSID to a string.
	wchar_t clsid_str[48];	// maybe only 40 is needed?
	LONG lResult = StringFromGUID2(__uuidof(RP_ExtractImage), clsid_str, sizeof(clsid_str)/sizeof(clsid_str[0]));
	if (lResult <= 0) {
		return ERROR_INVALID_PARAMETER;
	}

	// Register as the image handler for this file association.

	// Create/open the "ShellEx" key.
	RegKey hkcr_ShellEx(hkey_Assoc, L"ShellEx", KEY_WRITE, true);
	if (!hkcr_ShellEx.isOpen()) {
		return hkcr_ShellEx.lOpenRes();
	}
	// Create/open the IExtractImage key.
	RegKey hkcr_IExtractImage(hkcr_ShellEx, L"{BB2E617C-0920-11D1-9A0B-00C04FC2D6C1}", KEY_WRITE, true);
	if (!hkcr_IExtractImage.isOpen()) {
		return hkcr_IExtractImage.lOpenRes();
	}
	// Set the default value to this CLSID.
	lResult = hkcr_IExtractImage.write(nullptr, clsid_str);
	if (lResult != ERROR_SUCCESS) {
		return lResult;
	}

	// File type handler registered.
	return ERROR_SUCCESS;
}

/**
 * Unregister the COM object.
 * @return ERROR_SUCCESS on success; Win32 error code on error.
 */
LONG RP_ExtractImage::UnregisterCLSID(void)
{
	extern const wchar_t RP_ProgID[];

	// Unegister the COM object.
	return RegKey::UnregisterComObject(__uuidof(RP_ExtractImage), RP_ProgID);
}

/**
 * Unregister the file type handler.
 * @param hkey_Assoc File association key to register under.
 * @return ERROR_SUCCESS on success; Win32 error code on error.
 */
LONG RP_ExtractImage::UnregisterFileType(RegKey &hkey_Assoc)
{
	extern const wchar_t RP_ProgID[];

	// Convert the CLSID to a string.
	wchar_t clsid_str[48];	// maybe only 40 is needed?
	LONG lResult = StringFromGUID2(__uuidof(RP_ExtractImage), clsid_str, sizeof(clsid_str)/sizeof(clsid_str[0]));
	if (lResult <= 0) {
		return ERROR_INVALID_PARAMETER;
	}

	// Unregister as the image handler for this file association.

	// Open the "ShellEx" key.
	RegKey hkcr_ShellEx(hkey_Assoc, L"ShellEx", KEY_READ, false);
	if (!hkcr_ShellEx.isOpen()) {
		// ERROR_FILE_NOT_FOUND is acceptable here.
		if (hkcr_ShellEx.lOpenRes() == ERROR_FILE_NOT_FOUND) {
			return ERROR_SUCCESS;
		}
		return hkcr_ShellEx.lOpenRes();
	}
	// Open the IExtractImage key.
	RegKey hkcr_IExtractImage(hkcr_ShellEx, L"{BB2E617C-0920-11D1-9A0B-00C04FC2D6C1}", KEY_READ, false);
	if (!hkcr_IExtractImage.isOpen()) {
		// ERROR_FILE_NOT_FOUND is acceptable here.
		if (hkcr_IExtractImage.lOpenRes() == ERROR_FILE_NOT_FOUND) {
			return ERROR_SUCCESS;
		}
		return hkcr_IExtractImage.lOpenRes();
	}

	// Check if the default value matches the CLSID.
	wstring wstr_IExtractImage = hkcr_IExtractImage.read(nullptr);
	if (wstr_IExtractImage == clsid_str) {
		// Default value matches.
		// Remove the subkey.
		hkcr_IExtractImage.close();
		lResult = hkcr_ShellEx.deleteSubKey(L"{BB2E617C-0920-11D1-9A0B-00C04FC2D6C1}");
		if (lResult != ERROR_SUCCESS) {
			return lResult;
		}
	} else {
		// Default value doesn't match.
		// We're done here.
		return hkcr_IExtractImage.lOpenRes();
	}

	// File type handler unregistered.
	return ERROR_SUCCESS;
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
		// pdwPriority must be specified if IEIFLAG_ASYNC is set.
		return E_INVALIDARG;
	}

	// Save the image size for later.
	RP_D(RP_ExtractImage);
	d->bmSize = *prgSize;

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
	int ret = d->getThumbnail(d->filename.c_str(), d->bmSize.cx, *phBmpImage);
	// TODO: More specific error?
	return (ret == 0 ? S_OK : E_FAIL);
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
