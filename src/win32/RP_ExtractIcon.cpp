/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * RP_ExtractIcon.cpp: IExtractIcon implementation.                        *
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
#include "RP_ExtractIcon.hpp"
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
const CLSID CLSID_RP_ExtractIcon =
	{0xe51bc107, 0xe491, 0x4b29, {0xa6, 0xa3, 0x2a, 0x43, 0x09, 0x25, 0x98, 0x02}};

/** IUnknown **/
// Reference: https://msdn.microsoft.com/en-us/library/office/cc839627.aspx

IFACEMETHODIMP RP_ExtractIcon::QueryInterface(REFIID riid, LPVOID *ppvObj)
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
	if (riid == IID_IUnknown || riid == IID_IExtractIcon) {
		*ppvObj = static_cast<IExtractIcon*>(this);
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
LONG RP_ExtractIcon::Register(void)
{
	static const wchar_t description[] = L"ROM Properties Page - Icon Extractor";
	extern const wchar_t RP_ProgID[];

	// Convert the CLSID to a string.
	wchar_t clsid_str[48];	// maybe only 40 is needed?
	LONG lResult = StringFromGUID2(__uuidof(RP_ExtractIcon), clsid_str, sizeof(clsid_str)/sizeof(clsid_str[0]));
	if (lResult <= 0)
		return ERROR_INVALID_PARAMETER;

	// Register the COM object.
	lResult = RegKey::RegisterComObject(__uuidof(RP_ExtractIcon), RP_ProgID, description);
	if (lResult != ERROR_SUCCESS)
		return lResult;

	// Register as an "approved" shell extension.
	lResult = RegKey::RegisterApprovedExtension(__uuidof(RP_ExtractIcon), description);
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
	// Create/open the "IconHandler" key.
	RegKey hkcr_IconHandler(hkcr_ShellEx, L"IconHandler", KEY_WRITE, true);
	if (!hkcr_IconHandler.isOpen())
		return hkcr_IconHandler.lOpenRes();
	// Set the default value to this CLSID.
	lResult = hkcr_IconHandler.write(nullptr, clsid_str);
	if (lResult != ERROR_SUCCESS)
		return lResult;
	hkcr_IconHandler.close();
	hkcr_ShellEx.close();

	// Create/open the "DefaultIcon" key.
	RegKey hkcr_DefaultIcon(hkcr_ProgID, L"DefaultIcon", KEY_WRITE, true);
	if (!hkcr_DefaultIcon.isOpen())
		return SELFREG_E_CLASS;
	// Set the default value to "%1".
	lResult = hkcr_DefaultIcon.write(nullptr, L"%1");
	if (lResult != ERROR_SUCCESS)
		return SELFREG_E_CLASS;
	hkcr_DefaultIcon.close();
	hkcr_ProgID.close();

	// COM object registered.
	return ERROR_SUCCESS;
}

/**
 * Unregister the COM object.
 * @return ERROR_SUCCESS on success; Win32 error code on error.
 */
LONG RP_ExtractIcon::Unregister(void)
{
	extern const wchar_t RP_ProgID[];

	// Unegister the COM object.
	LONG lResult = RegKey::UnregisterComObject(__uuidof(RP_ExtractIcon), RP_ProgID);
	if (lResult != ERROR_SUCCESS)
		return lResult;

	// TODO
	return ERROR_SUCCESS;
}

/** IExtractIcon **/
// Reference: https://msdn.microsoft.com/en-us/library/windows/desktop/bb761854(v=vs.85).aspx

IFACEMETHODIMP RP_ExtractIcon::GetIconLocation(UINT uFlags,
	LPTSTR pszIconFile, UINT cchMax, int *piIndex, UINT *pwFlags)
{
	// TODO: If the icon is cached on disk, return a filename.
	// TODO: Enable ASYNC?
	// - https://msdn.microsoft.com/en-us/library/windows/desktop/bb761852(v=vs.85).aspx
	UNUSED(uFlags);
	UNUSED(pszIconFile);
	UNUSED(cchMax);
	UNUSED(piIndex);

#ifndef NDEBUG
	// Debug version. Don't cache icons.
	*pwFlags = GIL_NOTFILENAME | GIL_DONTCACHE;
#else /* !NDEBUG */
	// Release version. Cache icons.
	*pwFlags = GIL_NOTFILENAME;
#endif /* NDEBUG */

	return S_OK;
}

IFACEMETHODIMP RP_ExtractIcon::Extract(LPCTSTR pszFile, UINT nIconIndex,
	HICON *phiconLarge, HICON *phiconSmall, UINT nIconSize)
{
	// NOTE: pszFile should be nullptr here.
	// TODO: Fail if it's not nullptr?

	// TODO: Use nIconSize?
	UNUSED(pszFile);
	UNUSED(nIconIndex);
	UNUSED(nIconSize);

	// Make sure a filename was set by calling IPersistFile::Load().
	if (m_filename.empty()) {
		return E_INVALIDARG;
	}

	// phiconLarge must be valid.
	if (!phiconLarge) {
		return E_INVALIDARG;
	}

	// Attempt to open the ROM file.
	// TODO: RpQFile wrapper.
	// For now, using RpFile, which is an stdio wrapper.
	unique_ptr<IRpFile> file(new RpFile(m_filename, RpFile::FM_OPEN_READ));
	if (!file || !file->isOpen()) {
		return E_FAIL;
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

	*phiconLarge = nullptr;
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
		// Convert the image to HICON.
		HICON hIcon = RpImageWin32::toHICON(img);
		if (hIcon != nullptr) {
			// Icon converted.
			if (phiconLarge) {
				*phiconLarge = hIcon;
			} else {
				DeleteObject(hIcon);
			}

			if (phiconSmall) {
				// FIXME: is this valid?
				*phiconSmall = nullptr;
			}
		}

		if (needs_delete) {
			// Delete the image.
			delete const_cast<rp_image*>(img);
		}
	}

	return (*phiconLarge != nullptr ? S_OK : S_FALSE);
}

/** IPersistFile **/
// Reference: https://msdn.microsoft.com/en-us/library/windows/desktop/cc144067(v=vs.85).aspx#unknown_28177

IFACEMETHODIMP RP_ExtractIcon::GetClassID(CLSID *pClassID)
{
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

	// pszFileName is the file being worked on.
	m_filename = W2RP_cs(pszFileName);
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
