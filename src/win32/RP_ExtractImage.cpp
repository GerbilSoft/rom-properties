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

RP_ExtractImage::RP_ExtractImage()
{
	m_bmSize.cx = 0;
	m_bmSize.cy = 0;
}

/** IUnknown **/
// Reference: https://msdn.microsoft.com/en-us/library/office/cc839627.aspx

IFACEMETHODIMP RP_ExtractImage::QueryInterface(REFIID riid, LPVOID *ppvObj)
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

IFACEMETHODIMP RP_ExtractImage::GetLocation(LPWSTR pszPathBuffer,
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

IFACEMETHODIMP RP_ExtractImage::Extract(HBITMAP *phBmpImage)
{
	// TODO: Handle m_bmSize?
	*phBmpImage = nullptr;

	// Make sure a filename was set by calling IPersistFile::Load().
	if (m_filename.empty())
		return E_INVALIDARG;

	// Get the RomData object.
	unique_ptr<IRpFile> file(new RpFile(m_filename, RpFile::FM_OPEN_READ));
	if (!file || !file->isOpen()) {
		return E_FAIL;	// TODO: More specific error?
	}

	// Get the appropriate RomData class for this ROM.
	// RomData class *must* support at least one image type.
	unique_ptr<RomData> romData(RomDataFactory::getInstance(file.get(), true));
	file.reset(nullptr);	// file is dup()'d by RomData.

	if (!romData) {
		// ROM is not supported.
		return S_FALSE;
	}

	// ROM is supported. Get the image.
	// TODO: Customize which ones are used per-system.
	// For now, check EXT MEDIA, then INT ICON.

	bool needs_delete = false;	// External images need manual deletion.
	const rp_image *img = nullptr;

	uint32_t imgbf = romData->supportedImageTypes();
	if (imgbf & RomData::IMGBF_EXT_MEDIA) {
		// External media scan.
		img = RpImageWin32::getExternalImage(romData.get(), RomData::IMG_EXT_MEDIA);
		needs_delete = (img != nullptr);
	}

	if (!img) {
		// No external media scan.
		// Try an internal image.
		if (imgbf & RomData::IMGBF_INT_ICON) {
			// Internal icon.
			img = RpImageWin32::getInternalImage(romData.get(), RomData::IMG_INT_ICON);
		}
	}

	if (img) {
		// Image loaded. Convert it to HBITMAP.
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
		*phBmpImage = RpImageWin32::toHBITMAP(img, bgColor);

		if (needs_delete) {
			// Delete the image.
			delete const_cast<rp_image*>(img);
		}
	}

	return (*phBmpImage != nullptr ? S_OK : E_FAIL);
}

/** IPersistFile **/
// Reference: https://msdn.microsoft.com/en-us/library/windows/desktop/cc144067(v=vs.85).aspx#unknown_28177

IFACEMETHODIMP RP_ExtractImage::GetClassID(CLSID *pClassID)
{
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
	m_filename = W2RP_c(pszFileName);
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
