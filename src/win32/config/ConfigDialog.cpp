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

#include "ConfigDialog.hpp"
#include "ConfigDialog_p.hpp"
#include "resource.h"

#include "libromdata/common.h"
#include "libromdata/config/Config.hpp"
using LibRomData::Config;

#include "RegKey.hpp"

// C includes.
#include <stdlib.h>

// DllMain.cpp
extern HINSTANCE g_hInstance;

// Property sheet tabs.
#include "ImageTypesTab.hpp"

/** ConfigDialogPrivate **/

// Property for "D pointer".
// This points to the ConfigDialogPrivate object.
const wchar_t ConfigDialogPrivate::D_PTR_PROP[] = L"ConfigDialogPrivate";

ConfigDialogPrivate::ConfigDialogPrivate()
	: m_isVista(false)
	, config(Config::instance())
	, changed_Downloads(false)
{
	// Initialize the property sheet tabs.
	memset(tabs, 0, sizeof(tabs));

	// Make sure we have all required window classes available.
	// Reference: https://msdn.microsoft.com/en-us/library/windows/desktop/bb775507(v=vs.85).aspx
	INITCOMMONCONTROLSEX initCommCtrl;
	initCommCtrl.dwSize = sizeof(initCommCtrl);
	initCommCtrl.dwICC = ICC_LISTVIEW_CLASSES | ICC_LINK_CLASS | ICC_TAB_CLASSES | ICC_PROGRESS_CLASS;
	// TODO: Also ICC_STANDARD_CLASSES on XP+?
	InitCommonControlsEx(&initCommCtrl);

	// Create three property sheet pages.
	PROPSHEETPAGE psp[3];

	// Image type priority.
	tabs[0] = new ImageTypesTab();
	hpsp[0] = tabs[0]->getHPropSheetPage();

	// Download configuration.
	psp[1].dwSize = sizeof(psp);
	psp[1].dwFlags = PSP_USECALLBACK | PSP_USETITLE;
	psp[1].hInstance = g_hInstance;
	psp[1].pszTemplate = MAKEINTRESOURCE(IDD_CONFIG_DOWNLOADS);
	psp[1].pszIcon = nullptr;
	psp[1].pszTitle = L"Downloads";
	psp[1].pfnDlgProc = DlgProc_Downloads;
	psp[1].pcRefParent = nullptr;
	psp[1].pfnCallback = CallbackProc_Downloads;

	// Thumbnail cache.
	// References:
	// - http://stackoverflow.com/questions/23677175/clean-windows-thumbnail-cache-programmatically
	// - https://www.codeproject.com/Articles/2408/Clean-Up-Handler
	psp[2].dwSize = sizeof(psp);
	psp[2].dwFlags = PSP_USECALLBACK | PSP_USETITLE;
	psp[2].hInstance = g_hInstance;
	psp[2].pszIcon = nullptr;
	psp[2].pszTitle = L"Thumbnail Cache";
	psp[2].pfnDlgProc = DlgProc_Cache;
	psp[2].pcRefParent = nullptr;
	psp[2].pfnCallback = CallbackProc_Cache;

	// Determine which dialog we should use.
	RegKey hKey(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\VolumeCaches\\Thumbnail Cache", KEY_READ, false);
	if (hKey.isOpen()) {
		// Vista+ Thumbnail Cache cleaner is available.
		m_isVista = true;
		psp[2].pszTemplate = MAKEINTRESOURCE(IDD_CONFIG_CACHE);
		hKey.close();
	} else {
		// Not available. Use manual cache cleaning.
		m_isVista = false;
		psp[2].pszTemplate = MAKEINTRESOURCE(IDD_CONFIG_CACHE_XP);
	}

	// Create a ConfigDialogPrivate and make it available to the property sheet pages.
	psp[1].lParam = reinterpret_cast<LPARAM>(this);
	psp[2].lParam = reinterpret_cast<LPARAM>(this);

	// Create the property sheet pages.
	// NOTE: PropertySheet() is supposed to be able to take an
	// array of PROPSHEETPAGE, but it isn't working...
	hpsp[1] = CreatePropertySheetPage(&psp[1]);
	hpsp[2] = CreatePropertySheetPage(&psp[2]);

	// Create the property sheet.
	psh.dwSize = sizeof(psh);
	psh.dwFlags = PSH_USECALLBACK | PSH_NOCONTEXTHELP;
	psh.hwndParent = nullptr;
	psh.hInstance = g_hInstance;
	psh.hIcon = nullptr;
	psh.pszCaption = L"ROM Properties Page Configuration";
	psh.nPages = 3;
	psh.nStartPage = 0;
	psh.phpage = hpsp;
	psh.pfnCallback = ConfigDialogPrivate::CallbackProc;
	psh.hbmWatermark = nullptr;
	psh.hplWatermark = nullptr;
	psh.hbmHeader = nullptr;

	// Property sheet will be displayed when ConfigDialog::exec() is called.
}

ConfigDialogPrivate::~ConfigDialogPrivate()
{
	for (int i = ARRAY_SIZE(tabs)-1; i >= 0; i--) {
		delete tabs[i];
	}
}

/**
 * Property Sheet callback.
 * @param hDlg Property sheet dialog.
 * @param uMsg
 * @param lParam
 * @return 0
 */
int CALLBACK ConfigDialogPrivate::CallbackProc(HWND hDlg, UINT uMsg, LPARAM lParam)
{
	switch (uMsg) {
		case PSCB_INITIALIZED: {
			// Property sheet has been initialized.
			// Add the system menu and minimize box.
			LONG style = GetWindowLong(hDlg, GWL_STYLE);
			style |= WS_MINIMIZEBOX | WS_SYSMENU;
			SetWindowLong(hDlg, GWL_STYLE, style);

			// Remove the context help box.
			// NOTE: Setting WS_MINIMIZEBOX does this,
			// but we should remove the style anyway.
			LONG exstyle = GetWindowLong(hDlg, GWL_EXSTYLE);
			exstyle &= ~WS_EX_CONTEXTHELP;
			SetWindowLong(hDlg, GWL_EXSTYLE, exstyle);
			break;
		}

		default:
			break;
	}

	return 0;
}

/** ConfigDialog **/

ConfigDialog::ConfigDialog()
	: d_ptr(new ConfigDialogPrivate())
{ }

ConfigDialog::~ConfigDialog()
{
	delete d_ptr;
}

/**
 * Run the property sheet.
 * @return PropertySheet() return value.
 */
INT_PTR ConfigDialog::exec(void)
{
	RP_D(ConfigDialog);
	return PropertySheet(&d->psh);
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

	ConfigDialog *cfg = new ConfigDialog();
	INT_PTR ret = cfg->exec();

	// Uninitialize COM.
	CoUninitialize();

	return EXIT_SUCCESS;
}
