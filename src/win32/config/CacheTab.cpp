/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * CacheTab.cpp: Thumbnail Cache tab. (Part of ConfigDialog.)              *
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

#include "stdafx.h"

#include "ConfigDialog_p.hpp"
#include "resource.h"

#include "libromdata/RpWin32.hpp"
#include "libromdata/config/Config.hpp"
using LibRomData::Config;

#include "RegKey.hpp"

// C++ includes.
#include <string>
using std::wstring;

// IEmptyVolumeCacheCallBack implementation.
#include "RP_EmptyVolumeCacheCallback.hpp"

// COM smart pointer typedefs.
_COM_SMARTPTR_TYPEDEF(IEmptyVolumeCache, IID_IEmptyVolumeCache);

/**
 * Clear the thumbnail cache. (Vista+)
 * @param hDlg IDD_CONFIG_CACHE dialog.
 * @return 0 on success; non-zero on error.
 */
int ConfigDialogPrivate::clearCacheVista(HWND hDlg)
{
	HWND hStatusLabel = GetDlgItem(hDlg, IDC_CACHE_STATUS);
	HWND hProgressBar = GetDlgItem(hDlg, IDC_CACHE_PROGRESS);
	ShowWindow(hStatusLabel, SW_SHOW);
	ShowWindow(hProgressBar, SW_SHOW);

	// Reset the progress bar.
	SendMessage(hProgressBar, PBM_SETSTATE, PBST_NORMAL, 0);
	SendMessage(hProgressBar, PBM_SETRANGE, 0, MAKELONG(0, 100));
	SendMessage(hProgressBar, PBM_SETPOS, 0, 0);

	// Get all available drive letters.
	wchar_t wbuf[128];
	DWORD driveLetters = GetLogicalDrives();
	driveLetters &= 0x3FFFFFF;	// Mask out non-drive letter bits.
	if (driveLetters == 0) {
		// Error retrieving drive letters...
		// NOTE: PBM_SETSTATE is Vista+, which is fine here because
		// this is only run on Vista+.
		DWORD dwErr = GetLastError();
		swprintf(wbuf, _countof(wbuf), L"ERROR: GetLogicalDrives() failed. (GetLastError() == 0x%08X)", dwErr);
		SetWindowText(hStatusLabel, wbuf);
		SendMessage(hProgressBar, PBM_SETSTATE, PBST_ERROR, 0);
		return 1;
	}
	// Ignore all drives that aren't fixed HDDs.
	wchar_t szDrivePath[4] = L"X:\\";
	unsigned int driveCount = 0;
	for (unsigned int bit = 0; bit < 26; bit++) {
		const uint32_t mask = (1 << bit);
		if (!(driveLetters & mask))
			continue;
		szDrivePath[0] = L'A' + bit;
		if (GetDriveType(szDrivePath) != DRIVE_FIXED) {
			// Not a fixed HDD.
			driveLetters &= ~mask;
		} else {
			// This is a fixed HDD.
			driveCount++;
		}
	}
	if (driveLetters == 0) {
		// No fixed hard drives detected...
		SetWindowText(hStatusLabel, L"ERROR: No fixed HDDs or SSDs detected.");
		SendMessage(hProgressBar, PBM_SETSTATE, PBST_ERROR, 0);
		return 2;
	}

	// Open the registry key for the thumbnail cache cleaner.
	RegKey hKey(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\VolumeCaches\\Thumbnail Cache", KEY_READ, false);
	if (!hKey.isOpen()) {
		// Failed to open the registry key.
		swprintf(wbuf, _countof(wbuf), L"ERROR: Thumbnail Cache cleaner not found. (res == %ld)", hKey.lOpenRes());
		SetWindowText(hStatusLabel, wbuf);
		SendMessage(hProgressBar, PBM_SETSTATE, PBST_ERROR, 0);
		return 3;
	}

	// Get the CLSID.
	wstring s_clsidThumbnailCacheCleaner = hKey.read(nullptr);
	if (s_clsidThumbnailCacheCleaner.size() != 38) {
		// Not a CLSID.
		SetWindowText(hStatusLabel, L"ERROR: Thumbnail Cache cleaner CLSID is invalid.");
		SendMessage(hProgressBar, PBM_SETSTATE, PBST_ERROR, 0);
		return 4;
	}
	CLSID clsidThumbnailCacheCleaner;
	HRESULT hr = CLSIDFromString(s_clsidThumbnailCacheCleaner.c_str(), &clsidThumbnailCacheCleaner);
	if (FAILED(hr)) {
		// Failed to convert the CLSID from string.
		SetWindowText(hStatusLabel, L"ERROR: Thumbnail Cache cleaner CLSID is invalid.");
		SendMessage(hProgressBar, PBM_SETSTATE, PBST_ERROR, 0);
		return 5;
	}

	// Attempt to clear the cache on all non-removable hard drives.
	// TODO: Check mount points?
	// Reference: http://stackoverflow.com/questions/23677175/clean-windows-thumbnail-cache-programmatically
	IEmptyVolumeCachePtr pCleaner;
	hr = CoCreateInstance(clsidThumbnailCacheCleaner, nullptr,
		CLSCTX_INPROC_SERVER | CLSCTX_LOCAL_SERVER, IID_PPV_ARGS(&pCleaner));
	if (FAILED(hr)) {
		// Failed...
		swprintf(wbuf, _countof(wbuf), L"ERROR: CoCreateInstance() failed. (hr == 0x%08X)", hr);
		SetWindowText(hStatusLabel, wbuf);
		SendMessage(hProgressBar, PBM_SETSTATE, PBST_ERROR, 0);
		return 6;
	}

	// TODO: Disable user input until we're done?

	// Initialize the progress bar.
	SendMessage(hProgressBar, PBM_SETRANGE, 0, MAKELONG(0, driveCount * 100));
	SendMessage(hProgressBar, PBM_SETPOS, 0, 0);

	// Thumbnail cache callback.
	RP_EmptyVolumeCacheCallback *pCallback = new RP_EmptyVolumeCacheCallback(hDlg);

	pCallback->m_baseProgress = 0;
	unsigned int clearCount = 0;	// Number of drives actually cleared. (S_OK)
	for (unsigned int bit = 0; bit < 26; bit++) {
		const uint32_t mask = (1 << bit);
		if (!(driveLetters & mask))
			continue;

		// TODO: Add operator HKEY() to RegKey?
		LPWSTR pwszDisplayName = nullptr;
		LPWSTR pwszDescription = nullptr;
		DWORD dwFlags = 0;
		szDrivePath[0] = L'A' + bit;
		hr = pCleaner->Initialize(hKey.handle(), szDrivePath, &pwszDisplayName, &pwszDescription, &dwFlags);

		// Free the display name and description,
		// since we don't need them.
		if (pwszDisplayName) {
			CoTaskMemFree(pwszDisplayName);
		}
		if (pwszDescription) {
			CoTaskMemFree(pwszDescription);
		}

		// Did initialization succeed?
		if (hr == S_FALSE) {
			// Nothing to delete.
			pCallback->m_baseProgress += 100;
			SendMessage(hProgressBar, PBM_SETPOS, pCallback->m_baseProgress, 0);
			continue;
		} else if (hr != S_OK) {
			// Some error occurred.
			// TODO: Continue with other drives?
			swprintf(wbuf, _countof(wbuf), L"ERROR: IEmptyVolumeCache::Initialize() failed on drive %c. (hr == 0x%08X)", szDrivePath[0], hr);
			SetWindowText(hStatusLabel, wbuf);
			SendMessage(hProgressBar, PBM_SETSTATE, PBST_ERROR, 0);
			pCallback->Release();
			return 7;
		}

		// Clear the thumbnails.
		hr = pCleaner->Purge(-1LL, pCallback);
		if (FAILED(hr)) {
			// Cleanup failed. (TODO: Figure out why!)
			// TODO: Indicate the drive letter and error code.
			swprintf(wbuf, _countof(wbuf), L"ERROR: IEmptyVolumeCache::Purge() failed on drive %c. (hr == 0x%08X)", szDrivePath[0], hr);
			SetWindowText(hStatusLabel, wbuf);
			SendMessage(hProgressBar, PBM_SETSTATE, PBST_ERROR, 0);
			pCallback->Release();
		}

		// Next drive.
		clearCount++;
		pCallback->m_baseProgress += 100;
		SendMessage(hProgressBar, PBM_SETPOS, pCallback->m_baseProgress, 0);
	}

	// TODO: SPI_SETICONS to clear the icon cache?
	const wchar_t *success_message;
	if (clearCount > 0) {
		success_message = L"System thumbnail cache cleared successfully.";
	} else {
		success_message = L"System thumbnail cache is already empty. Nothing to do here.";
	}
	SetWindowText(hStatusLabel, success_message);
	pCallback->Release();
	return 0;
}

/**
 * DlgProc for IDD_CONFIG_CACHE / IDD_CONFIG_CACHE_XP.
 * @param hDlg
 * @param uMsg
 * @param wParam
 * @param lParam
 */
INT_PTR CALLBACK ConfigDialogPrivate::DlgProc_Cache(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
		case WM_INITDIALOG: {
			// Get the pointer to the property sheet page object. This is 
			// contained in the LPARAM of the PROPSHEETPAGE structure.
			LPPROPSHEETPAGE pPage = reinterpret_cast<LPPROPSHEETPAGE>(lParam);
			if (!pPage)
				return TRUE;

			// Get the pointer to the ConfigDialogPrivate object.
			ConfigDialogPrivate *const d = reinterpret_cast<ConfigDialogPrivate*>(pPage->lParam);
			if (!d)
				return TRUE;

			// Store the D object pointer with this particular page dialog.
			SetProp(hDlg, D_PTR_PROP, reinterpret_cast<HANDLE>(d));

			// On XP, we need to initialize some widgets.
			// TODO: InitDialog().
			if (!d->isVista()) {
				Button_SetCheck(GetDlgItem(hDlg, IDC_CACHE_XP_FIND_DRIVES), BST_CHECKED);
				ShowWindow(GetDlgItem(hDlg, IDC_CACHE_XP_PATH), SW_HIDE);
				ShowWindow(GetDlgItem(hDlg, IDC_CACHE_XP_BROWSE), SW_HIDE);
			}

			// TODO: Initialize the dialog.
			return TRUE;
		}

		case WM_DESTROY: {
			// Remove the D_PTR_PROP property from the page. 
			// The D_PTR_PROP property stored the pointer to the 
			// ConfigDialogPrivate object.
			RemoveProp(hDlg, D_PTR_PROP);
			return TRUE;
		}

		case WM_COMMAND: {
			switch (LOWORD(wParam)) {
				case IDC_CACHE_CLEAR_SYS_THUMBS:
					// Clear the system thumbnail cache. (Vista+)
					clearCacheVista(hDlg);
					return TRUE;
				case IDC_CACHE_XP_CLEAR_SYS_THUMBS:
					// Clear the system thumbnail cache. (XP)
					// TODO
					break;
				default:
					break;
			}
			break;
		}

		default:
			break;
	}

	return FALSE; // Let system deal with other messages
}

/**
 * Property sheet callback function for IDD_CONFIG_CACHE / IDD_CONFIG_CACHE_XP.
 * @param hDlg
 * @param uMsg
 * @param wParam
 * @param lParam
 */
UINT CALLBACK ConfigDialogPrivate::CallbackProc_Cache(HWND hWnd, UINT uMsg, LPPROPSHEETPAGE ppsp)
{
	switch (uMsg) {
		case PSPCB_CREATE: {
			// Must return TRUE to enable the page to be created.
			return TRUE;
		}

		case PSPCB_RELEASE: {
			// TODO: Do something here?
			break;
		}

		default:
			break;
	}

	return FALSE;
}
