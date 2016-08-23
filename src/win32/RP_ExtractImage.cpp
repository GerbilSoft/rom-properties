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
#include "libromdata/rp_image.hpp"
#include "libromdata/RpFile.hpp"
using namespace LibRomData;

// C includes. (C++ namespace)
#include <cassert>
#include <cstdio>
#include <cstring>

// C++ includes.
#include <string>
using std::wstring;

// CLSID
const CLSID CLSID_RP_ExtractImage =
	{0x84573bc0, 0x9502, 0x42f8, {0x80, 0x66, 0xCC, 0x52, 0x7D, 0x07, 0x79, 0xE5}};

RP_ExtractImage::RP_ExtractImage()
	: super()
{
	m_bmSize.cx = 0;
	m_bmSize.cy = 0;
}

/** IUnknown **/
// Reference: https://msdn.microsoft.com/en-us/library/office/cc839627.aspx

STDMETHODIMP RP_ExtractImage::QueryInterface(REFIID riid, LPVOID *ppvObj)
{
	// Always set out parameter to NULL, validating it first.
	if (!ppvObj)
		return E_INVALIDARG;
	*ppvObj = NULL;

	// Check if this interface is supported.
	// NOTE: static_cast<> is required due to vtable shenanigans.
	// Also, IID_IUnknown must always return the same pointer.
	// References:
	// - http://stackoverflow.com/questions/1742848/why-exactly-do-i-need-an-explicit-upcast-when-implementing-queryinterface-in-a
	// - http://stackoverflow.com/a/2812938
	if (riid == IID_IUnknown || riid == IID_IExtractImage) {
		*ppvObj = static_cast<IExtractImage*>(this);
	} else if (riid == IID_IPersistFile) {
		*ppvObj = static_cast<IPersistFile*>(this);
	} else {
		// Interface is not supported.
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
LONG RP_ExtractImage::Register(void)
{
	static const wchar_t description[] = L"ROM Properties Page - Image Extractor";
	extern const wchar_t RP_ProgID[];

	// Convert the CLSID to a string.
	wchar_t clsid_str[48];	// maybe only 40 is needed?
	LONG lResult = StringFromGUID2(__uuidof(RP_ExtractImage), clsid_str, sizeof(clsid_str)/sizeof(clsid_str[0]));
	if (lResult <= 0)
		return ERROR_INVALID_PARAMETER;

	// Register the COM object.
	lResult = RegKey::RegisterComObject(__uuidof(RP_ExtractImage), RP_ProgID, description);
	if (lResult != ERROR_SUCCESS)
		return lResult;

	// Register as an "approved" shell extension.
	lResult = RegKey::RegisterApprovedExtension(__uuidof(RP_ExtractImage), description);
	if (lResult != ERROR_SUCCESS)
		return lResult;

	// Register as the icon handler for this ProgID.
	// Create/open the ProgID key.
	RegKey hkcr_ProgID(HKEY_CLASSES_ROOT, RP_ProgID, KEY_WRITE, true);
	if (!hkcr_ProgID.isOpen())
		return hkcr_ProgID.lOpenRes();

	// Create/open the "ShellEx" key.
	RegKey hkcr_ShellEx(hkcr_ProgID, L"ShellEx", KEY_WRITE, true);
	if (!hkcr_ShellEx.isOpen())
		return hkcr_ShellEx.lOpenRes();
	// Create/open the IExtractImage key.
	RegKey hkcr_IExtractImage(hkcr_ShellEx, L"{BB2E617C-0920-11D1-9A0B-00C04FC2D6C1}", KEY_WRITE, true);
	if (!hkcr_IExtractImage.isOpen())
		return hkcr_IExtractImage.lOpenRes();
	// Set the default value to this CLSID.
	lResult = hkcr_IExtractImage.write(nullptr, clsid_str);
	if (lResult != ERROR_SUCCESS)
		return lResult;
	hkcr_IExtractImage.close();
	hkcr_ShellEx.close();

	// COM object registered.
	return ERROR_SUCCESS;
}

/**
 * Unregister the COM object.
 * @return ERROR_SUCCESS on success; Win32 error code on error.
 */
LONG RP_ExtractImage::Unregister(void)
{
	extern const wchar_t RP_ProgID[];

	// Unegister the COM object.
	LONG lResult = RegKey::UnregisterComObject(__uuidof(RP_ExtractImage), RP_ProgID);
	if (lResult != ERROR_SUCCESS)
		return lResult;

	// TODO
	return ERROR_SUCCESS;
}

/** IExtractImage **/
// References:
// - https://msdn.microsoft.com/en-us/library/windows/desktop/bb761848(v=vs.85).aspx
// - http://www.codeproject.com/Articles/2887/Create-Thumbnail-Extractor-objects-for-your-MFC-do

STDMETHODIMP RP_ExtractImage::GetLocation(LPWSTR pszPathBuffer,
	DWORD cchMax, DWORD *pdwPriority, const SIZE *prgSize,
	DWORD dwRecClrDepth, DWORD *pdwFlags)
{
	// TODO: If the image is cached on disk, return a filename.

	// Save the image size for later.
	m_bmSize = *prgSize;

	// Disable the border around the thumbnail.
	// NOTE: Might not work on Vista+.
	*pdwFlags |= IEIFLAG_NOBORDER;

#ifndef NDEBUG
	// Debug version. Don't cache images.
	// (Windows XP and earlier.)
	*pdwFlags |= IEIFLAG_CACHE;
#endif /* NDEBUG */

	// TODO: On Windows XP, check for IEIFLAG_ASYNC.
	// If specified, run the thumbnailing process in the background?
	return S_OK;
}

STDMETHODIMP RP_ExtractImage::Extract(HBITMAP *phBmpImage)
{
	// TODO: Actually get a bitmap.
	// For now, just draw a square with alternating cyan and yellow pixels.

	// Make sure a filename was set by calling IPersistFile::Load().
	if (m_filename.empty())
		return E_INVALIDARG;

	// Create an image.
	rp_image *img = new rp_image(m_bmSize.cx, m_bmSize.cy, rp_image::FORMAT_CI8);
	uint32_t *palette = img->palette();
	memset(palette, 0, img->palette_len() * sizeof(uint32_t));
	palette[0] = 0xFFFF00FF;	// Magenta
	palette[1] = 0xFFFFFF00;	// Cyan
	palette[2] = 0xFF00FFFF;	// Yellow
	for (int y = 0; y < m_bmSize.cy; y++) {
		uint8_t *scanline = (uint8_t*)img->scanLine(y);
		for (int x = m_bmSize.cx; x > 0; x -= 2) {
			scanline[0] = (y%2 ? 1 : 2);
			scanline[1] = (y%2 ? 2 : 1);
			scanline += 2;
		}
	}

	// Convert it to an HBITMAP.
	HBITMAP hBitmap = RpImageWin32::toHBITMAP(img);
	if (hBitmap) {
		// Bitmap has been created.
		*phBmpImage = hBitmap;
		delete img;
		return S_OK;
	}

	// Error creating the bitmap.
	delete img;
	return E_FAIL;
}

/** IPersistFile **/
// Reference: https://msdn.microsoft.com/en-us/library/windows/desktop/cc144067(v=vs.85).aspx#unknown_28177

STDMETHODIMP RP_ExtractImage::GetClassID(CLSID *pClassID)
{
	*pClassID = CLSID_RP_ExtractImage;
	return S_OK;
}

STDMETHODIMP RP_ExtractImage::IsDirty(void)
{
	return E_NOTIMPL;
}

STDMETHODIMP RP_ExtractImage::Load(LPCOLESTR pszFileName, DWORD dwMode)
{
	UNUSED(dwMode);	// TODO

	// pszFileName is the file being worked on.
#ifdef RP_UTF16
	m_filename = reinterpret_cast<const rp_char*>(pszFileName);
#else
	// FIXME: Not supported.
	#error RP_ExtractImage requires UTF-16.
#endif
	return S_OK;
}

STDMETHODIMP RP_ExtractImage::Save(LPCOLESTR pszFileName, BOOL fRemember)
{
	UNUSED(pszFileName);
	UNUSED(fRemember);
	return E_NOTIMPL;
}

STDMETHODIMP RP_ExtractImage::SaveCompleted(LPCOLESTR pszFileName)
{
	UNUSED(pszFileName);
	return E_NOTIMPL;
}

STDMETHODIMP RP_ExtractImage::GetCurFile(LPOLESTR *ppszFileName)
{
	UNUSED(ppszFileName);
	return E_NOTIMPL;
}
