/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * CacheTab.cpp: Thumbnail Cache tab for rp-config.                        *
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
#include "CacheTab.hpp"

// libwin32common
#include "libwin32common/RegKey.hpp"
using LibWin32Common::RegKey;

#include "resource.h"

// IEmptyVolumeCacheCallBack implementation.
#include "RP_EmptyVolumeCacheCallback.hpp"

// COM smart pointer typedefs.
_COM_SMARTPTR_TYPEDEF(IEmptyVolumeCache, IID_IEmptyVolumeCache);

// C includes. (C++ namespace)
#include <cassert>

// C++ includes.
#include <string>
using std::wstring;

class CacheTabPrivate
{
	public:
		CacheTabPrivate();

	private:
		RP_DISABLE_COPY(CacheTabPrivate)

	public:
		// Property for "D pointer".
		// This points to the CacheTabPrivate object.
		static const wchar_t D_PTR_PROP[];

	public:
		/**
		 * Initialize controls on the XP version.
		 */
		void initControlsXP(void);

		/**
		 * Clear the Thumbnail Cache. (Windows Vista and later.)
		 * @return 0 on success; non-zero on error.
		 */
		int clearCacheVista(void);

	public:
		/**
		 * Dialog procedure.
		 * @param hDlg
		 * @param uMsg
		 * @param wParam
		 * @param lParam
		 */
		static INT_PTR CALLBACK dlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

		/**
		 * Property sheet callback procedure.
		 * @param hWnd
		 * @param uMsg
		 * @param ppsp
		 */
		static UINT CALLBACK callbackProc(HWND hWnd, UINT uMsg, LPPROPSHEETPAGE ppsp);

	public:
		// Property sheet.
		HPROPSHEETPAGE hPropSheetPage;
		HWND hWndPropSheet;

		// Is this Windows Vista or later?
		bool isVista;
};

/** CacheTabPrivate **/

CacheTabPrivate::CacheTabPrivate()
	: hPropSheetPage(nullptr)
	, hWndPropSheet(nullptr)
	, isVista(false)
{
	// Determine which dialog we should use.
	RegKey hKey(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\VolumeCaches\\Thumbnail Cache", KEY_READ, false);
	if (hKey.isOpen()) {
		// Windows Vista Thumbnail Cache cleaner is available.
		isVista = true;
	} else {
		// Not available. Use manual cache cleaning.
		isVista = false;
	}
}

// Property for "D pointer".
// This points to the CacheTabPrivate object.
const wchar_t CacheTabPrivate::D_PTR_PROP[] = L"CacheTabPrivate";

/**
 * Initialize controls on the XP version.
 */
void CacheTabPrivate::initControlsXP(void)
{
	Button_SetCheck(GetDlgItem(hWndPropSheet, IDC_CACHE_XP_FIND_DRIVES), BST_CHECKED);
	ShowWindow(GetDlgItem(hWndPropSheet, IDC_CACHE_XP_PATH), SW_HIDE);
	ShowWindow(GetDlgItem(hWndPropSheet, IDC_CACHE_XP_BROWSE), SW_HIDE);
}

/**
 * Clear the Thumbnail Cache. (Windows Vista and later.)
 * @return 0 on success; non-zero on error.
 */
int CacheTabPrivate::clearCacheVista(void)
{
	HWND hStatusLabel = GetDlgItem(hWndPropSheet, IDC_CACHE_STATUS);
	HWND hProgressBar = GetDlgItem(hWndPropSheet, IDC_CACHE_PROGRESS);
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
		swprintf_s(wbuf, ARRAY_SIZE(wbuf), L"ERROR: GetLogicalDrives() failed. (GetLastError() == 0x%08X)", dwErr);
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
		swprintf_s(wbuf, ARRAY_SIZE(wbuf), L"ERROR: Thumbnail Cache cleaner not found. (res == %ld)", hKey.lOpenRes());
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
		swprintf_s(wbuf, ARRAY_SIZE(wbuf), L"ERROR: CoCreateInstance() failed. (hr == 0x%08X)", hr);
		SetWindowText(hStatusLabel, wbuf);
		SendMessage(hProgressBar, PBM_SETSTATE, PBST_ERROR, 0);
		return 6;
	}

	// TODO: Disable user input until we're done?

	// Initialize the progress bar.
	SendMessage(hProgressBar, PBM_SETRANGE, 0, MAKELONG(0, driveCount * 100));
	SendMessage(hProgressBar, PBM_SETPOS, 0, 0);

	// Thumbnail cache callback.
	RP_EmptyVolumeCacheCallback *pCallback = new RP_EmptyVolumeCacheCallback(hWndPropSheet);

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
			swprintf_s(wbuf, ARRAY_SIZE(wbuf), L"ERROR: IEmptyVolumeCache::Initialize() failed on drive %c. (hr == 0x%08X)", szDrivePath[0], hr);
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
			swprintf_s(wbuf, ARRAY_SIZE(wbuf), L"ERROR: IEmptyVolumeCache::Purge() failed on drive %c. (hr == 0x%08X)", szDrivePath[0], hr);
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
 * Dialog procedure.
 * @param hDlg
 * @param uMsg
 * @param wParam
 * @param lParam
 */
INT_PTR CALLBACK CacheTabPrivate::dlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
		case WM_INITDIALOG: {
			// Get the pointer to the property sheet page object. This is 
			// contained in the LPARAM of the PROPSHEETPAGE structure.
			LPPROPSHEETPAGE pPage = reinterpret_cast<LPPROPSHEETPAGE>(lParam);
			if (!pPage)
				return TRUE;

			// Get the pointer to the CacheTabPrivate object.
			CacheTabPrivate *const d = reinterpret_cast<CacheTabPrivate*>(pPage->lParam);
			if (!d)
				return TRUE;

			assert(d->hWndPropSheet == nullptr);
			d->hWndPropSheet = hDlg;

			// Store the D object pointer with this particular page dialog.
			SetProp(hDlg, D_PTR_PROP, reinterpret_cast<HANDLE>(d));

			// The XP version requires some control initialization.
			if (!d->isVista) {
				d->initControlsXP();
			}
			return TRUE;
		}

		case WM_DESTROY: {
			// Remove the D_PTR_PROP property from the page. 
			// The D_PTR_PROP property stored the pointer to the 
			// CacheTabPrivate object.
			RemoveProp(hDlg, D_PTR_PROP);
			return TRUE;
		}

		case WM_NOTIFY: {
			CacheTabPrivate *const d = static_cast<CacheTabPrivate*>(
				GetProp(hDlg, D_PTR_PROP));
			if (!d) {
				// No CacheTabPrivate. Can't do anything...
				return FALSE;
			}

			LPPSHNOTIFY lppsn = reinterpret_cast<LPPSHNOTIFY>(lParam);
			switch (lppsn->hdr.code) {
				case NM_CLICK:
				case NM_RETURN:
					// SysLink control notification.
					if (lppsn->hdr.hwndFrom == GetDlgItem(hDlg, IDC_IMAGETYPES_CREDITS)) {
						// Open the URL.
						PNMLINK pNMLink = reinterpret_cast<PNMLINK>(lParam);
						ShellExecute(nullptr, L"open", pNMLink->item.szUrl, nullptr, nullptr, SW_SHOW);
					}
					break;

				default:
					break;
			}
			break;
		}

		case WM_COMMAND: {
			CacheTabPrivate *const d = static_cast<CacheTabPrivate*>(
				GetProp(hDlg, D_PTR_PROP));
			if (!d) {
				// No CacheTabPrivate. Can't do anything...
				return FALSE;
			}

			switch (LOWORD(wParam)) {
				case IDC_CACHE_CLEAR_SYS_THUMBS:
					// Clear the system thumbnail cache. (Vista+)
					d->clearCacheVista();
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
 * Property sheet callback procedure.
 * @param hDlg
 * @param uMsg
 * @param wParam
 * @param lParam
 */
UINT CALLBACK CacheTabPrivate::callbackProc(HWND hWnd, UINT uMsg, LPPROPSHEETPAGE ppsp)
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

/** CacheTab **/

CacheTab::CacheTab(void)
	: d_ptr(new CacheTabPrivate())
{ }

CacheTab::~CacheTab()
{
	delete d_ptr;
}

/**
 * Create the HPROPSHEETPAGE for this tab.
 *
 * NOTE: This function can only be called once.
 * Subsequent invocations will return nullptr.
 *
 * @return HPROPSHEETPAGE.
 */
HPROPSHEETPAGE CacheTab::getHPropSheetPage(void)
{
	RP_D(CacheTab);
	assert(d->hPropSheetPage == nullptr);
	if (d->hPropSheetPage) {
		// Property sheet has already been created.
		return nullptr;
	}

	extern HINSTANCE g_hInstance;

	PROPSHEETPAGE psp;
	psp.dwSize = sizeof(psp);	
	psp.dwFlags = PSP_USECALLBACK | PSP_USETITLE;
	psp.hInstance = g_hInstance;
	psp.pszIcon = nullptr;
	psp.pszTitle = L"Thumbnail Cache";
	psp.pfnDlgProc = CacheTabPrivate::dlgProc;
	psp.lParam = reinterpret_cast<LPARAM>(d);
	psp.pcRefParent = nullptr;
	psp.pfnCallback = CacheTabPrivate::callbackProc;

	if (d->isVista) {
		psp.pszTemplate = MAKEINTRESOURCE(IDD_CONFIG_CACHE);
	} else {
		psp.pszTemplate = MAKEINTRESOURCE(IDD_CONFIG_CACHE_XP);
	}

	d->hPropSheetPage = CreatePropertySheetPage(&psp);
	return d->hPropSheetPage;
}

/**
 * Reset the contents of this tab.
 */
void CacheTab::reset(void)
{
	// Nothing to reset here...
}

/**
 * Save the contents of this tab.
 */
void CacheTab::save(void)
{
	// Nothing to save here...
}
