/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * ConfigDialog.cpp: Configuration dialog.                                 *
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
#include "resource.h"

#include "RegKey.hpp"
#include "libromdata/common.h"
#include "libromdata/config/Config.hpp"
using LibRomData::Config;

// C includes.
#include <stdlib.h>

// C++ includes.
#include <string>
using std::wstring;

// IEmptyVolumeCacheCallBack implementation.
#include "RP_EmptyVolumeCacheCallback.hpp"

// COM smart pointer typedefs.
_COM_SMARTPTR_TYPEDEF(IEmptyVolumeCache, IID_IEmptyVolumeCache);

// Property for "D pointer".
// This points to the ConfigDialogPrivate object.
#define D_PTR_PROP L"ConfigDialogPrivate"

/** ConfigDialogPrivate **/

class ConfigDialogPrivate
{
	public:
		ConfigDialogPrivate(bool isVista);

	private:
		RP_DISABLE_COPY(ConfigDialogPrivate)

	private:
		bool m_isVista;

	public:
		inline bool isVista(void) const
		{
			return m_isVista;
		}

		// Config instance.
		Config *config;

		/**
		 * Convert a bool value to BST_CHCEKED or BST_UNCHECKED.
		 * @param value Bool value.
		 * @return BST_CHECKED or BST_UNCHECKED.
		 */
		static inline int boolToBstChecked(bool value) {
			return (value ? BST_CHECKED : BST_UNCHECKED);
		}

		/**
		 * Convert BST_CHECKED or BST_UNCHECKED to a bool string.
		 * @param value BST_CHECKED or BST_UNCHECKED.
		 * @return Bool string.
		 */
		static inline const wchar_t *bstCheckedToBoolString(unsigned int value) {
			return (value == BST_CHECKED ? L"true" : L"false");
		}

		// Downloads tab functions.
		void reset_Downloads(HWND hDlg);
		void save_Downloads(HWND hDlg);
		static INT_PTR CALLBACK DlgProc_Downloads(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
		static UINT CALLBACK CallbackProc_Downloads(HWND hWnd, UINT uMsg, LPPROPSHEETPAGE ppsp);

		// Thumbnail Cache tab functions.
		static int clearCacheVista(HWND hDlg);
		static INT_PTR CALLBACK DlgProc_Cache(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
		static UINT CALLBACK CallbackProc_Cache(HWND hWnd, UINT uMsg, LPPROPSHEETPAGE ppsp);

		// Create Property Sheet.
		static INT_PTR CreatePropertySheet(void);
};

ConfigDialogPrivate::ConfigDialogPrivate(bool isVista)
	: m_isVista(isVista)
	, config(Config::instance())
{ }

/** Property sheet callback functions. **/

/**
 * Reset the Downloads tab to the current configuration.
 * @param hDlg Downloads property sheet.
 */
void ConfigDialogPrivate::reset_Downloads(HWND hDlg)
{
	HWND hWnd = GetDlgItem(hDlg, IDC_EXTIMGDL);
	if (hWnd) {
		Button_SetCheck(hWnd, boolToBstChecked(config->extImgDownloadEnabled()));
	}
	hWnd = GetDlgItem(hDlg, IDC_INTICONSMALL);
	if (hWnd) {
		Button_SetCheck(hWnd, boolToBstChecked(config->useIntIconForSmallSizes()));
	}
	hWnd = GetDlgItem(hDlg, IDC_HIGHRESDL);
	if (hWnd) {
		Button_SetCheck(hWnd, boolToBstChecked(config->downloadHighResScans()));
	}
}

/**
 * Save the Downloads tab configuration.
 * @param hDlg Downloads property sheet.
 */
void ConfigDialogPrivate::save_Downloads(HWND hDlg)
{
	const rp_char *filename = config->filename();
	if (!filename) {
		// No configuration filename...
		return;
	}

	HWND hWnd = GetDlgItem(hDlg, IDC_EXTIMGDL);
	if (hWnd) {
		const wchar_t *bstr = bstCheckedToBoolString(Button_GetCheck(hWnd));
		WritePrivateProfileString(L"Downloads", L"ExtImageDownload", bstr, RP2W_c(filename));
	}
	hWnd = GetDlgItem(hDlg, IDC_INTICONSMALL);
	if (hWnd) {
		const wchar_t *bstr = bstCheckedToBoolString(Button_GetCheck(hWnd));
		WritePrivateProfileString(L"Downloads", L"UseIntIconForSmallSizes", bstr, RP2W_c(filename));
	}
	hWnd = GetDlgItem(hDlg, IDC_HIGHRESDL);
	if (hWnd) {
		const wchar_t *bstr = bstCheckedToBoolString(Button_GetCheck(hWnd));
		WritePrivateProfileString(L"Downloads", L"DownloadHighResScans", bstr, RP2W_c(filename));
	}
}

/**
 * DlgProc for IDD_CONFIG_DOWNLOADS.
 * @param hDlg
 * @param uMsg
 * @param wParam
 * @param lParam
 */
INT_PTR CALLBACK ConfigDialogPrivate::DlgProc_Downloads(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
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
			SetProp(hDlg,D_PTR_PROP, reinterpret_cast<HANDLE>(d));

			// Reset the values.
			d->reset_Downloads(hDlg);
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
			// A checkbox has been adjusted.
			// Page has been modified.
			PropSheet_Changed(GetParent(hDlg), hDlg);
			break;
		}

		case WM_NOTIFY: {
			ConfigDialogPrivate *d = static_cast<ConfigDialogPrivate*>(
				GetProp(hDlg, D_PTR_PROP));
			if (!d) {
				// No ConfigDialogPrivate. Can't do anything...
				return FALSE;
			}

			LPPSHNOTIFY lppsn = reinterpret_cast<LPPSHNOTIFY>(lParam);
			switch (lppsn->hdr.code) {
				case PSN_APPLY:
					// Save settings.
					d->save_Downloads(hDlg);
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
 * Property sheet callback function for IDD_CONFIG_DOWNLOADS.
 * @param hDlg
 * @param uMsg
 * @param wParam
 * @param lParam
 */
UINT CALLBACK ConfigDialogPrivate::CallbackProc_Downloads(HWND hWnd, UINT uMsg, LPPROPSHEETPAGE ppsp)
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

// Create the property sheet.
// TEMPORARY version to test things out.
INT_PTR ConfigDialogPrivate::CreatePropertySheet(void)
{
	extern HINSTANCE g_hInstance;

	// Make sure we have all required window classes available.
	// Reference: https://msdn.microsoft.com/en-us/library/windows/desktop/bb775507(v=vs.85).aspx
	INITCOMMONCONTROLSEX initCommCtrl;
	initCommCtrl.dwSize = sizeof(initCommCtrl);
	initCommCtrl.dwICC = ICC_LISTVIEW_CLASSES | ICC_LINK_CLASS | ICC_TAB_CLASSES | ICC_PROGRESS_CLASS;
	// TODO: Also ICC_STANDARD_CLASSES on XP+?
	InitCommonControlsEx(&initCommCtrl);

	// Create three property sheet pages.
	// TODO: Select only one caching page depending on OS version.
	PROPSHEETPAGE psp[3];

	// Download configuration.
	psp[0].dwSize = sizeof(psp);
	psp[0].dwFlags = PSP_USECALLBACK | PSP_USETITLE;
	psp[0].hInstance = g_hInstance;
	psp[0].pszTemplate = MAKEINTRESOURCE(IDD_CONFIG_DOWNLOADS);
	psp[0].pszIcon = nullptr;
	psp[0].pszTitle = L"Downloads";
	psp[0].pfnDlgProc = DlgProc_Downloads;
	psp[0].pcRefParent = nullptr;
	psp[0].pfnCallback = CallbackProc_Downloads;

	// Thumbnail cache.
	// References:
	// - http://stackoverflow.com/questions/23677175/clean-windows-thumbnail-cache-programmatically
	// - https://www.codeproject.com/Articles/2408/Clean-Up-Handler
	psp[1].dwSize = sizeof(psp);
	psp[1].dwFlags = PSP_USECALLBACK | PSP_USETITLE;
	psp[1].hInstance = g_hInstance;
	psp[1].pszIcon = nullptr;
	psp[1].pszTitle = L"Thumbnail Cache";
	psp[1].pfnDlgProc = DlgProc_Cache;
	psp[1].pcRefParent = nullptr;
	psp[1].pfnCallback = CallbackProc_Cache;

	// Determine which dialog we should use.
	RegKey hKey(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\VolumeCaches\\Thumbnail Cache", KEY_READ, false);
	bool isVista;
	if (hKey.isOpen()) {
		// Vista+ Thumbnail Cache cleaner is available.
		isVista = true;
		psp[1].pszTemplate = MAKEINTRESOURCE(IDD_CONFIG_CACHE);
		hKey.close();
	} else {
		// Not available. Use manual cache cleaning.
		isVista = false;
		psp[1].pszTemplate = MAKEINTRESOURCE(IDD_CONFIG_CACHE_XP);
	}

	// Create a ConfigDialogPrivate and make it available to the property sheet pages.
	ConfigDialogPrivate *const d = new ConfigDialogPrivate(isVista);
	psp[0].lParam = reinterpret_cast<LPARAM>(d);
	psp[1].lParam = reinterpret_cast<LPARAM>(d);

	// Create the property sheet pages.
	// NOTE: PropertySheet() is supposed to be able to take an
	// array of PROPSHEETPAGE, but it isn't working...
	HPROPSHEETPAGE hpsp[3];
	hpsp[0] = CreatePropertySheetPage(&psp[0]);
	hpsp[1] = CreatePropertySheetPage(&psp[1]);

	// Create the property sheet.
	PROPSHEETHEADER psh;
	psh.dwSize = sizeof(psh);
	psh.dwFlags = 0;
	psh.hwndParent = nullptr;
	psh.hInstance = g_hInstance;
	psh.hIcon = nullptr;
	psh.pszCaption = L"ROM Properties Page Configuration";
	psh.nPages = 2;
	psh.nStartPage = 0;
	psh.phpage = hpsp;
	psh.pfnCallback = nullptr;	// TODO
	psh.hbmWatermark = nullptr;
	psh.hplWatermark = nullptr;
	psh.hbmHeader = nullptr;

	INT_PTR ret = PropertySheet(&psh);
	delete d;
	return ret;
}

/**
 * Exported function for the rp-config stub.
 * @param hWnd		[in] Parent window handle.
 * @param hInstance	[in] rp-config instance.
 * @param pszCmdLine	[in] Command line.
 * @param nCmdShow	[in] nCmdShow
 * @return 0 on success; non-zero on error.
 */
extern "C"
int __declspec(dllexport) CALLBACK rp_show_config_dialog(
	HWND hWnd, HINSTANCE hInstance, LPSTR pszCmdLine, int nCmdShow)
{
	// TODO: nCmdShow.

	// Make sure COM is initialized.
	// NOTE: Using apartment threading for OLE compatibility.
	// TODO: What if COM is already initialized?
	HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
	if (FAILED(hr)) {
		// Failed to initialize COM.
		return EXIT_FAILURE;
	}

	ConfigDialogPrivate::CreatePropertySheet();

	// Uninitialize COM.
	CoUninitialize();

	return EXIT_SUCCESS;
}
