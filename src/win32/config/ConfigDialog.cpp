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

// Property for "external pointer".
// This links the property sheet to the COM object.
#define EXT_POINTER_PROP L"RP_ShellPropSheetExt"

/** Property sheet callback functions. **/

/**
 * DlgProc for IDD_CONFIG_DOWNLOADS.
 * @param hDlg
 * @param uMsg
 * @param wParam
 * @param lParam
 */
static INT_PTR CALLBACK DlgProc_Downloads(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
		case WM_INITDIALOG: {
			// Get the pointer to the property sheet page object. This is 
			// contained in the LPARAM of the PROPSHEETPAGE structure.
			LPPROPSHEETPAGE pPage = reinterpret_cast<LPPROPSHEETPAGE>(lParam);
			if (!pPage)
				return TRUE;

			// Store the object pointer with this particular page dialog.
			// TODO
			SetProp(hDlg, EXT_POINTER_PROP, nullptr);

			// TODO: Initialize the dialog.
			return TRUE;
		}

		case WM_DESTROY: {
			// Remove the EXT_POINTER_PROP property from the page. 
			// The EXT_POINTER_PROP property stored the pointer to the 
			// FilePropSheetExt object.
			RemoveProp(hDlg, EXT_POINTER_PROP);
			return TRUE;
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
static UINT CALLBACK CallbackProc_Downloads(HWND hWnd, UINT uMsg, LPPROPSHEETPAGE ppsp)
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
 * DlgProc for IDD_CONFIG_CACHE / IDD_CONFIG_CACHE_XP.
 * @param hDlg
 * @param uMsg
 * @param wParam
 * @param lParam
 */
static INT_PTR CALLBACK DlgProc_Cache(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
		case WM_INITDIALOG: {
			// Get the pointer to the property sheet page object. This is 
			// contained in the LPARAM of the PROPSHEETPAGE structure.
			LPPROPSHEETPAGE pPage = reinterpret_cast<LPPROPSHEETPAGE>(lParam);
			if (!pPage)
				return TRUE;

			// Store the object pointer with this particular page dialog.
			// TODO
			SetProp(hDlg, EXT_POINTER_PROP, nullptr);

			// TODO: Initialize the dialog.
			return TRUE;
		}

		case WM_DESTROY: {
			// Remove the EXT_POINTER_PROP property from the page. 
			// The EXT_POINTER_PROP property stored the pointer to the 
			// FilePropSheetExt object.
			RemoveProp(hDlg, EXT_POINTER_PROP);
			return TRUE;
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
static UINT CALLBACK CallbackProc_Cache(HWND hWnd, UINT uMsg, LPPROPSHEETPAGE ppsp)
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
static INT_PTR createPropertySheet(void)
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
	psp[0].lParam = 0;	// TODO

	// Thumbnail cache.
	// TODO: References:
	// - http://stackoverflow.com/questions/23677175/clean-windows-thumbnail-cache-programmatically
	// - https://www.codeproject.com/Articles/2408/Clean-Up-Handler
	// TODO: Check for the Thumbnail Cache cleanup object:
	// - HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\Explorer\VolumeCaches\Thumbnail Cache
	// If present, use the Vista+ dialog.
	// If not present, use the XP dialog.
	// For now, always using the Vista version.
	psp[1].dwSize = sizeof(psp);
	psp[1].dwFlags = PSP_USECALLBACK | PSP_USETITLE;
	psp[1].hInstance = g_hInstance;
	psp[1].pszTemplate = MAKEINTRESOURCE(IDD_CONFIG_CACHE);
	psp[1].pszIcon = nullptr;
	psp[1].pszTitle = L"Thumbnail Cache";
	psp[1].pfnDlgProc = DlgProc_Cache;
	psp[1].pcRefParent = nullptr;
	psp[1].pfnCallback = CallbackProc_Cache;
	psp[1].lParam = 0;	// TODO

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

	return PropertySheet(&psh);
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
	createPropertySheet();
	return 0;
}
