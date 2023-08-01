/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * RP_ShellIconOverlayIdentifier.cpp: IShellIconOverlayIdentifier          *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

// Reference: http://www.codeproject.com/Articles/338268/COM-in-C
#include "stdafx.h"
#include "RP_ShellIconOverlayIdentifier.hpp"
#include "res/resource.h"

// librpbase, librpfile, libromdata
using namespace LibRpBase;
using namespace LibRpFile;
using LibRomData::RomDataFactory;

// C++ STL classes.
using std::string;

// CLSID
const CLSID CLSID_RP_ShellIconOverlayIdentifier =
	{0x02c6Af01, 0x3c99, 0x497d, {0xb3, 0xfc, 0xe3, 0x8c, 0xe5, 0x26, 0x78, 0x6b}};

/** RP_ShellIconOverlayIdentifier_Private **/
#include "RP_ShellIconOverlayIdentifier_p.hpp"

// FIXME: Crashing when scrolling through %TEMP%...

RP_ShellIconOverlayIdentifier_Private::RP_ShellIconOverlayIdentifier_Private()
#if 0
	: romData(nullptr)
#endif
	: hShell32_dll(nullptr)
	, pfnSHGetStockIconInfo(nullptr)
{
	// Get SHGetStockIconInfo().
	hShell32_dll = LoadLibraryEx(_T("shell32.dll"), nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
	if (hShell32_dll) {
		pfnSHGetStockIconInfo = (PFNSHGETSTOCKICONINFO)GetProcAddress(hShell32_dll, "SHGetStockIconInfo");
	}
}

RP_ShellIconOverlayIdentifier_Private::~RP_ShellIconOverlayIdentifier_Private()
{
	if (hShell32_dll) {
		FreeLibrary(hShell32_dll);
	}
}

/** RP_PropertyStore **/

RP_ShellIconOverlayIdentifier::RP_ShellIconOverlayIdentifier()
	: d_ptr(new RP_ShellIconOverlayIdentifier_Private())
{ }

RP_ShellIconOverlayIdentifier::~RP_ShellIconOverlayIdentifier()
{
	delete d_ptr;
}

/** IUnknown **/
// Reference: https://docs.microsoft.com/en-us/office/client-developer/outlook/mapi/implementing-iunknown-in-c-plus-plus

IFACEMETHODIMP RP_ShellIconOverlayIdentifier::QueryInterface(_In_ REFIID riid, _Outptr_ LPVOID *ppvObj)
{
#ifdef _MSC_VER
#  pragma warning(push)
#  pragma warning(disable: 4365 4838)
#endif /* _MSC_VER */
	static const QITAB rgqit[] = {
		QITABENT(RP_ShellIconOverlayIdentifier, IShellIconOverlayIdentifier),
		{ 0, 0 }
	};
#ifdef _MSC_VER
#  pragma warning(pop)
#endif /* _MSC_VER */
	return LibWin32Common::rp_QISearch(this, rgqit, riid, ppvObj);
}

/** IShellIconOverlayIdentifier **/
// Reference: https://docs.microsoft.com/en-us/windows/desktop/shell/how-to-implement-icon-overlay-handlers

IFACEMETHODIMP RP_ShellIconOverlayIdentifier::IsMemberOf(_In_ PCWSTR pwszPath, DWORD dwAttrib)
{
	if (!pwszPath) {
		return E_POINTER;
	}

	const Config *const config = Config::instance();
	if (!config->showDangerousPermissionsOverlayIcon()) {
		// Overlay icon is disabled.
		return S_FALSE;
	}

	// Don't check the file if it's "slow", unavailable, or a directory.
	if (dwAttrib & (SFGAO_ISSLOW | SFGAO_GHOSTED | SFGAO_FOLDER)) {
		// Don't bother checking this file.
		return S_FALSE;
	}

	// Convert the filename to UTF-8.
	const string u8filename = W2U8(pwszPath);

	// Check for "bad" file systems.
	// TODO: Combine with the above "slow" check?
	if (FileSystem::isOnBadFS(u8filename.c_str(),
	    config->enableThumbnailOnNetworkFS()))
	{
		// This file is on a "bad" file system.
		return S_FALSE;
	}

	// Attempt to create a RomData object.
	RomData *const romData = RomDataFactory::create(u8filename.c_str(), RomDataFactory::RDA_HAS_DPOVERLAY);
	if (!romData) {
		// ROM is not supported.
		// TODO: Return E_FAIL if the file couldn't be opened?
		return S_FALSE;
	}

	HRESULT hr = (romData->hasDangerousPermissions() ? S_OK : S_FALSE);
	romData->unref();
	return hr;
}

IFACEMETHODIMP RP_ShellIconOverlayIdentifier::GetOverlayInfo(_Out_writes_(cchMax) PWSTR pwszIconFile, int cchMax, _Out_ int *pIndex, _Out_ DWORD *pdwFlags)
{
	if (!pwszIconFile || !pIndex || !pdwFlags) {
		return E_POINTER;
	} else if (cchMax < 1) {
		return E_INVALIDARG;
	}

	// Get the "dangerous" permissions overlay.
	HRESULT hr;

	RP_D(const RP_ShellIconOverlayIdentifier);
	if (d->pfnSHGetStockIconInfo) {
		// SHGetStockIconInfo() is available.
		// FIXME: Icon size is a bit too large in some cases.
		SHSTOCKICONINFO sii;
		sii.cbSize = sizeof(sii);
		hr = d->pfnSHGetStockIconInfo(SIID_SHIELD, SHGSI_ICONLOCATION, &sii);
		if (SUCCEEDED(hr)) {
			// Copy the returned filename and index.
			wcscpy_s(pwszIconFile, cchMax, sii.szPath);
			*pIndex = sii.iIcon;
			*pdwFlags = ISIOI_ICONFILE | ISIOI_ICONINDEX;
		} else {
			// Unable to get the filename.
			pwszIconFile[0] = L'\0';
			*pIndex = 0;
			*pdwFlags = 0;
		}
	} else {
		// Use our own shield icon.
		// Based on Windows 7's shield icon from imageres.dll.
		// FIXME: Windows XP requires the overlay icon to be the
		// same size as the regular icon, but with transparency.
		hr = E_FAIL;
#if 0
		// [TODO: Removed; rework this when needed.]
		TCHAR szDllFilename[MAX_PATH];
		SetLastError(ERROR_SUCCESS);	// required for XP
		DWORD dwResult = GetModuleFileName(HINST_THISCOMPONENT,
			szDllFilename, _countof(szDllFilename));
		if (dwResult == 0 || dwResult >= _countof(szDllFilename) || GetLastError() != ERROR_SUCCESS) {
			// Cannot get the DLL filename.
			// TODO: Windows XP doesn't SetLastError() if the
			// filename is too big for the buffer.
			hr = E_FAIL;
		} else {
			wcscpy_s(pwszIconFile, cchMax, szDllFilename);
			*pIndex = -IDI_SHIELD;
			*pdwFlags = ISIOI_ICONFILE | ISIOI_ICONINDEX;

			// Assume we're successful.
			hr = S_OK;
		}
#endif
	}

	return hr;
}

IFACEMETHODIMP RP_ShellIconOverlayIdentifier::GetPriority(_Out_ int *pPriority)
{
	if (!pPriority) {
		return E_POINTER;
	}

	const Config *const config = Config::instance();
	if (!config->showDangerousPermissionsOverlayIcon()) {
		// Overlay icon is disabled.
		return S_FALSE;
	}

	// Use the higest priority for the UAC icon.
	*pPriority = 0;
	return S_OK;
}

