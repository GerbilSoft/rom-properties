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

/** RP_ExtractIcon_Private **/
// Workaround for RP_D() expecting the no-underscore naming convention.
#define RP_ExtractIconPrivate RP_ExtractIcon_Private

class RP_ExtractIcon_Private
{
	public:
		RP_ExtractIcon_Private();
		~RP_ExtractIcon_Private();

	private:
		RP_DISABLE_COPY(RP_ExtractIcon_Private)

	public:
		// ROM filename from IPersistFile::Load().
		rp_string filename;

		// RomData object. Loaded in IPersistFile::Load().
		LibRomData::RomData *romData;
};

RP_ExtractIcon_Private::RP_ExtractIcon_Private()
	: romData(nullptr)
{ }

RP_ExtractIcon_Private::~RP_ExtractIcon_Private()
{
	if (romData) {
		romData->unref();
	}
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
	// Always set out parameter to NULL, validating it first.
	if (!ppvObj)
		return E_INVALIDARG;

	// Check if this interface is supported.
	// NOTE: static_cast<> is required due to vtable shenanigans.
	// Also, IID_IUnknown must always return the same pointer.
	// References:
	// - http://stackoverflow.com/questions/1742848/why-exactly-do-i-need-an-explicit-upcast-when-implementing-queryinterface-in-a
	// - http://stackoverflow.com/a/2812938
	if (riid == IID_IUnknown || riid == IID_IExtractIcon) {
		*ppvObj = static_cast<IExtractIcon*>(this);
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
LONG RP_ExtractIcon::RegisterCLSID(void)
{
	static const wchar_t description[] = L"ROM Properties Page - Icon Extractor";
	extern const wchar_t RP_ProgID[];

	// Convert the CLSID to a string.
	wchar_t clsid_str[48];	// maybe only 40 is needed?
	LONG lResult = StringFromGUID2(__uuidof(RP_ExtractIcon), clsid_str, sizeof(clsid_str)/sizeof(clsid_str[0]));
	if (lResult <= 0) {
		return ERROR_INVALID_PARAMETER;
	}

	// Register the COM object.
	lResult = RegKey::RegisterComObject(__uuidof(RP_ExtractIcon), RP_ProgID, description);
	if (lResult != ERROR_SUCCESS) {
		return lResult;
	}

	// Register as an "approved" shell extension.
	lResult = RegKey::RegisterApprovedExtension(__uuidof(RP_ExtractIcon), description);
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
LONG RP_ExtractIcon::RegisterFileType(RegKey &hkey_Assoc)
{
	extern const wchar_t RP_ProgID[];

	// Convert the CLSID to a string.
	wchar_t clsid_str[48];	// maybe only 40 is needed?
	LONG lResult = StringFromGUID2(__uuidof(RP_ExtractIcon), clsid_str, sizeof(clsid_str)/sizeof(clsid_str[0]));
	if (lResult <= 0) {
		return ERROR_INVALID_PARAMETER;
	}

	// Register as the icon handler for this file association.

	// Create/open the "ShellEx" key.
	RegKey hkcr_ShellEx(hkey_Assoc, L"ShellEx", KEY_WRITE, true);
	if (!hkcr_ShellEx.isOpen()) {
		return hkcr_ShellEx.lOpenRes();
	}

	// Create/open the "IconHandler" key.
	RegKey hkcr_IconHandler(hkcr_ShellEx, L"IconHandler", KEY_WRITE, true);
	if (!hkcr_IconHandler.isOpen()) {
		return hkcr_IconHandler.lOpenRes();
	}
	// Set the default value to this CLSID.
	lResult = hkcr_IconHandler.write(nullptr, clsid_str);
	if (lResult != ERROR_SUCCESS) {
		return lResult;
	}

	// Create/open the "DefaultIcon" key.
	RegKey hkcr_DefaultIcon(hkey_Assoc, L"DefaultIcon", KEY_WRITE, true);
	if (!hkcr_DefaultIcon.isOpen()) {
		return hkcr_DefaultIcon.lOpenRes();
	}
	// Set the default value to "%1".
	lResult = hkcr_DefaultIcon.write(nullptr, L"%1");

	// File type handler registered.
	return lResult;
}

/**
 * Unregister the COM object.
 * @return ERROR_SUCCESS on success; Win32 error code on error.
 */
LONG RP_ExtractIcon::UnregisterCLSID(void)
{
	extern const wchar_t RP_ProgID[];

	// Unegister the COM object.
	return RegKey::UnregisterComObject(__uuidof(RP_ExtractIcon), RP_ProgID);
}

/**
 * Unregister the file type handler.
 * @param hkey_Assoc File association key to register under.
 * @return ERROR_SUCCESS on success; Win32 error code on error.
 */
LONG RP_ExtractIcon::UnregisterFileType(RegKey &hkey_Assoc)
{
	extern const wchar_t RP_ProgID[];

	// Convert the CLSID to a string.
	wchar_t clsid_str[48];	// maybe only 40 is needed?
	LONG lResult = StringFromGUID2(__uuidof(RP_ExtractIcon), clsid_str, sizeof(clsid_str)/sizeof(clsid_str[0]));
	if (lResult <= 0) {
		return ERROR_INVALID_PARAMETER;
	}

	// Unregister as the icon handler for this file association.

	// Open the "ShellEx" key.
	RegKey hkcr_ShellEx(hkey_Assoc, L"ShellEx", KEY_WRITE, false);
	if (!hkcr_ShellEx.isOpen()) {
		// ERROR_FILE_NOT_FOUND is acceptable here.
		if (hkcr_ShellEx.lOpenRes() == ERROR_FILE_NOT_FOUND) {
			return ERROR_SUCCESS;
		}
		return hkcr_ShellEx.lOpenRes();
	}

	// Open the "IconHandler" key.
	RegKey hkcr_IconHandler(hkcr_ShellEx, L"IconHandler", KEY_READ, false);
	if (!hkcr_IconHandler.isOpen()) {
		// ERROR_FILE_NOT_FOUND is acceptable here.
		if (hkcr_IconHandler.lOpenRes() == ERROR_FILE_NOT_FOUND) {
			return ERROR_SUCCESS;
		}
		return hkcr_IconHandler.lOpenRes();
	}
	// Check if the default value matches the CLSID.
	wstring str_IconHandler = hkcr_IconHandler.read(nullptr);
	if (str_IconHandler == clsid_str) {
		// Default value matches.
		// Remove the subkey.
		hkcr_IconHandler.close();
		lResult = hkcr_ShellEx.deleteSubKey(L"IconHandler");
		if (lResult != ERROR_SUCCESS) {
			return lResult;
		}
	} else {
		// Default value does not match.
		// We're done here.
		return hkcr_IconHandler.lOpenRes();
	}

	// Open the "DefaultIcon" key.
	RegKey hkcr_DefaultIcon(hkey_Assoc, L"DefaultIcon", KEY_READ, false);
	if (!hkcr_DefaultIcon.isOpen()) {
		// ERROR_FILE_NOT_FOUND is acceptable here.
		if (hkcr_DefaultIcon.lOpenRes() == ERROR_FILE_NOT_FOUND) {
			return ERROR_SUCCESS;
		}
		return hkcr_DefaultIcon.lOpenRes();
	}
	// Check if the default value is "%1".
	wstring wstr_DefaultIcon = hkcr_DefaultIcon.read(nullptr);
	if (wstr_DefaultIcon == L"%1") {
		// Default value matches.
		// Remove the subkey.
		hkcr_DefaultIcon.close();
		lResult = hkey_Assoc.deleteSubKey(L"DefaultIcon");
		if (lResult != ERROR_SUCCESS) {
			return lResult;
		}
	} else {
		// Default value doesn't match.
		// We're done here.
		return hkcr_DefaultIcon.lOpenRes();
	}

	// File type handler unregistered.
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
	if (!pszIconFile || !piIndex || cchMax == 0) {
		return E_INVALIDARG;
	}
	UNUSED(uFlags);

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
	*pwFlags = GIL_NOTFILENAME | GIL_DONTCACHE;
	return S_OK;
}

IFACEMETHODIMP RP_ExtractIcon::Extract(LPCTSTR pszFile, UINT nIconIndex,
	HICON *phiconLarge, HICON *phiconSmall, UINT nIconSize)
{
	// NOTE: pszFile and nIconIndex were set in GetIconLocation().
	UNUSED(pszFile);
	UNUSED(nIconIndex);

	// TODO: Use nIconSize?
	UNUSED(nIconSize);

	// Make sure a filename was set by calling IPersistFile::Load().
	RP_D(RP_ExtractIcon);
	if (d->filename.empty()) {
		return E_INVALIDARG;
	}

	// phiconLarge must be valid.
	if (!phiconLarge) {
		return E_INVALIDARG;
	}

	if (!d->romData) {
		// ROM is not supported.
		// NOTE: S_FALSE causes icon shenanigans.
		return E_FAIL;
	}

	// ROM is supported. Get the image.
	// TODO: Customize which ones are used per-system.
	// For now, check EXT MEDIA, then INT ICON.

	*phiconLarge = nullptr;
	bool needs_delete = false;	// External images need manual deletion.
	const rp_image *img = nullptr;

	uint32_t imgbf = d->romData->supportedImageTypes();
	if (imgbf & RomData::IMGBF_EXT_MEDIA) {
		// External media scan.
		img = RpImageWin32::getExternalImage(d->romData, RomData::IMG_EXT_MEDIA);
		needs_delete = (img != nullptr);
	}

	if (!img) {
		// No external media scan.
		// Try an internal image.
		if (imgbf & RomData::IMGBF_INT_ICON) {
			// Internal icon.
			img = RpImageWin32::getInternalImage(d->romData, RomData::IMG_INT_ICON);
		}
	}

	if (img) {
		// TODO: If the image is non-square, make it square.
		// Convert the image to HICON.
		HICON hIcon = RpImageWin32::toHICON(img);
		if (hIcon != nullptr) {
			// Icon converted.
			bool iconWasSet = false;
			if (phiconLarge) {
				*phiconLarge = hIcon;
				iconWasSet = true;
			}
			if (phiconSmall) {
				// NULL out the small icon.
				*phiconSmall = nullptr;
			}

			if (!iconWasSet) {
				DeleteObject(hIcon);
			}
		}

		if (needs_delete) {
			// Delete the image.
			delete const_cast<rp_image*>(img);
		}
	}

	// NOTE: S_FALSE causes icon shenanigans.
	return (*phiconLarge != nullptr ? S_OK : E_FAIL);
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
	// TODO: RpQFile wrapper.
	// For now, using RpFile, which is an stdio wrapper.
	unique_ptr<IRpFile> file(new RpFile(d->filename, RpFile::FM_OPEN_READ));
	if (!file || !file->isOpen()) {
		return E_FAIL;
	}

	// Get the appropriate RomData class for this ROM.
	// RomData class *must* support at least one image type.
	d->romData = RomDataFactory::getInstance(file.get(), true);
	if (!d->romData) {
		// Not supported.
		return E_FAIL;
	}

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
