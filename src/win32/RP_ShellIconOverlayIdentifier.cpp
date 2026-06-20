/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * RP_ShellIconOverlayIdentifier.cpp: IShellIconOverlayIdentifier          *
 *                                                                         *
 * Copyright (c) 2016-2026 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

// Reference: http://www.codeproject.com/Articles/338268/COM-in-C
#include "RP_ShellIconOverlayIdentifier.hpp"
#include "res/resource.h"

// Other rom-properties libraries
#include "librpbase/config/Config.hpp"
#include "librpfile/FileSystem.hpp"
#include "libromdata/RomDataFactory.hpp"
using namespace LibRpBase;
using namespace LibRpFile;
using namespace LibRomData;

// C++ STL classes
#include <memory>
using std::string;
using std::tstring;
using std::unique_ptr;

/** RP_ShellIconOverlayIdentifier_Private **/
#include "RP_ShellIconOverlayIdentifier_p.hpp"

// FIXME: Crashing when scrolling through %TEMP%...
// FIXME: regsvr32 doesn't force Explorer to reload overlay handlers... (Explorer needs to be restarted!)

// UAC shield icon variables
std::once_flag RP_ShellIconOverlayIdentifier_Private::uac_once_flag;
tstring RP_ShellIconOverlayIdentifier_Private::uac_shield_filename;
int RP_ShellIconOverlayIdentifier_Private::uac_shield_index = 0;

/**
 * Initialize the UAC shield icon variables.
 *
 * Internal function; must be called using std::call_once().
 */
void RP_ShellIconOverlayIdentifier_Private::initUacShieldIconVars(void)
{
	// shell32.dll might be delay-loaded to avoid a gdi32.dll penalty.
	// Call SHGetFolderPath() with invalid parameters to load it into
	// memory before using GetModuleHandle().
	{
		TCHAR szPathDummy[MAX_PATH];
		SHGetFolderPath(nullptr, 0, nullptr, 0, szPathDummy);
	}

	// Get SHGetStockIconInfo().
	HMODULE hShell32_dll = GetModuleHandle(_T("shell32.dll"));
	if (hShell32_dll) {
		typedef HRESULT (STDAPICALLTYPE *pfnSHGetStockIconInfo_t)(_In_ SHSTOCKICONID siid, _In_ UINT uFlags, _Out_ SHSTOCKICONINFO *psii);
		pfnSHGetStockIconInfo_t pfnSHGetStockIconInfo =
			reinterpret_cast<pfnSHGetStockIconInfo_t>(
				GetProcAddress(hShell32_dll, "SHGetStockIconInfo"));
		if (pfnSHGetStockIconInfo) {
			// SHGetStockIconInfo() is available.
			// FIXME: Icon size is a bit too large in some cases.
			SHSTOCKICONINFO sii;
			sii.cbSize = sizeof(sii);
			HRESULT hr = pfnSHGetStockIconInfo(SIID_SHIELD, SHGSI_ICONLOCATION, &sii);
			if (SUCCEEDED(hr)) {
				// Copy the returned filename and index.
				uac_shield_filename.assign(sii.szPath);
				uac_shield_index = sii.iIcon;
			}
		}
	}

	if (uac_shield_filename.empty()) {
		// SHGetStockIconInfo() either failed or is not available.
		// Use our own shield icon.
		// Based on Windows 7's shield icon from imageres.dll.
		// FIXME: Windows XP requires the overlay icon to be the
		// same size as the regular icon, but with transparency.

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
		} else {
			// Copy the returned filename and our known IDI_SHIELD icon index.
			uac_shield_filename.assign(szDllFilename);
			uac_shield_index = -IDI_SHIELD;
		}
#endif
	}
}

/** RP_PropertyStore **/

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
	if (!config->getBoolConfigOption(Config::BoolConfig::Options_ShowDangerousPermissionsOverlayIcon)) {
		// Overlay icon is disabled.
		return S_FALSE;
	}

	// Don't check the file if it's "slow", unavailable, or a directory.
	// TODO: Allow directories for e.g. Wii U NUS packages?
	if (dwAttrib & (SFGAO_ISSLOW | SFGAO_GHOSTED | SFGAO_FOLDER)) {
		// Don't bother checking this file.
		return S_FALSE;
	}

	// Check for "bad" file systems.
	// TODO: Combine with the above "slow" check?
	if (FileSystem::isOnBadFS(pwszPath, config->getBoolConfigOption(Config::BoolConfig::Options_EnableThumbnailOnNetworkFS))) {
		// This file is on a "bad" file system.
		return S_FALSE;
	}

	// Attempt to create a RomData object.
	const RomDataPtr romData = RomDataFactory::create(pwszPath, RomDataFactory::RDA_HAS_DPOVERLAY);
	if (!romData) {
		// ROM is not supported.
		// TODO: Return E_FAIL if the file couldn't be opened?
		return S_FALSE;
	}

	return (romData->hasDangerousPermissions()) ? S_OK : S_FALSE;
}

IFACEMETHODIMP RP_ShellIconOverlayIdentifier::GetOverlayInfo(_Out_writes_(cchMax) PWSTR pwszIconFile, int cchMax, _Out_ int *pIndex, _Out_ DWORD *pdwFlags)
{
	if (!pwszIconFile || !pIndex || !pdwFlags) {
		return E_POINTER;
	} else if (cchMax < 1) {
		return E_INVALIDARG;
	}

	// Initialize the UAC shield icon variables.
	using d = RP_ShellIconOverlayIdentifier_Private;
	std::call_once(d::uac_once_flag, d::initUacShieldIconVars);

	// Get the "dangerous" permissions overlay.
	HRESULT hr;

	if (!d::uac_shield_filename.empty()) {
		// We have a valid UAC shield icon.
		wcscpy_s(pwszIconFile, cchMax, d::uac_shield_filename.c_str());
		*pIndex = d::uac_shield_index;
		*pdwFlags = ISIOI_ICONFILE | ISIOI_ICONINDEX;
		hr = S_OK;
	} else {
		// No icon was loaded...
		pwszIconFile[0] = _T('\0');
		*pIndex = 0;
		*pdwFlags = 0;
		hr = E_FAIL;
	}

	return hr;
}

IFACEMETHODIMP RP_ShellIconOverlayIdentifier::GetPriority(_Out_ int *pPriority)
{
	if (!pPriority) {
		return E_POINTER;
	}

	const Config *const config = Config::instance();
	if (!config->getBoolConfigOption(Config::BoolConfig::Options_ShowDangerousPermissionsOverlayIcon)) {
		// Overlay icon is disabled.
		*pPriority = 100;
		return S_FALSE;
	}

	// Use the higest priority for the UAC icon.
	*pPriority = 0;
	return S_OK;
}

