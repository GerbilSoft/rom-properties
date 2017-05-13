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
#include "resource.h"

#include "libromdata/common.h"
#include "libromdata/config/Config.hpp"
using LibRomData::Config;

#include "RegKey.hpp"
#include "PropSheetIcon.hpp"

// C includes.
#include <stdlib.h>

// DllMain.cpp
extern HINSTANCE g_hInstance;

// Property sheet tabs.
#include "ImageTypesTab.hpp"
#include "DownloadsTab.hpp"
#include "CacheTab.hpp"

class ConfigDialogPrivate
{
	public:
		ConfigDialogPrivate();
		~ConfigDialogPrivate();

	private:
		RP_DISABLE_COPY(ConfigDialogPrivate)

	public:
		// Property for "D pointer".
		// This points to the ConfigDialogPrivate object.
		static const wchar_t D_PTR_PROP[];

	public:
		// Property sheet variables.
		ITab *tabs[3];	// TODO: Add Downloads.
		HPROPSHEETPAGE hpsp[3];
		PROPSHEETHEADER psh;

		// Property Sheet callback.
		static int CALLBACK callbackProc(HWND hDlg, UINT uMsg, LPARAM lParam);

		// Create Property Sheet.
		static INT_PTR CreatePropertySheet(void);
};

/** ConfigDialogPrivate **/

// Property for "D pointer".
// This points to the ConfigDialogPrivate object.
const wchar_t ConfigDialogPrivate::D_PTR_PROP[] = L"ConfigDialogPrivate";

ConfigDialogPrivate::ConfigDialogPrivate()
{
	// Make sure we have all required window classes available.
	// Reference: https://msdn.microsoft.com/en-us/library/windows/desktop/bb775507(v=vs.85).aspx
	INITCOMMONCONTROLSEX initCommCtrl;
	initCommCtrl.dwSize = sizeof(initCommCtrl);
	initCommCtrl.dwICC = ICC_LISTVIEW_CLASSES | ICC_LINK_CLASS | ICC_TAB_CLASSES | ICC_PROGRESS_CLASS;
	// TODO: Also ICC_STANDARD_CLASSES on XP+?
	InitCommonControlsEx(&initCommCtrl);

	// Initialize the property sheet tabs.

	// Image type priority.
	tabs[0] = new ImageTypesTab();
	hpsp[0] = tabs[0]->getHPropSheetPage();
	// Download configuration.
	tabs[1] = new DownloadsTab();
	hpsp[1] = tabs[1]->getHPropSheetPage();
	// Thumbnail cache.
	// References:
	// - http://stackoverflow.com/questions/23677175/clean-windows-thumbnail-cache-programmatically
	// - https://www.codeproject.com/Articles/2408/Clean-Up-Handler
	tabs[2] = new CacheTab();
	hpsp[2] = tabs[2]->getHPropSheetPage();

	// Create the property sheet.
	psh.dwSize = sizeof(psh);
	psh.dwFlags = PSH_USECALLBACK | PSH_NOCONTEXTHELP | PSH_USEHICON;
	psh.hwndParent = nullptr;
	psh.hInstance = g_hInstance;
	psh.hIcon = PropSheetIcon::getSmallIcon();	// Small icon only!
	psh.pszCaption = L"ROM Properties Page Configuration";
	psh.nPages = 3;
	psh.nStartPage = 0;
	psh.phpage = hpsp;
	psh.pfnCallback = this->callbackProc;
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
 * Property Sheet callback procedure.
 * @param hDlg Property sheet dialog.
 * @param uMsg
 * @param lParam
 * @return 0
 */
int CALLBACK ConfigDialogPrivate::callbackProc(HWND hDlg, UINT uMsg, LPARAM lParam)
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

			// NOTE: PropertySheet's pszIcon only uses the small icon.
			// Set the large icon here.
			HICON hIcon = PropSheetIcon::getLargeIcon();
			if (hIcon) {
				SendMessage(hDlg, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(hIcon));
			}
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
int CALLBACK rp_show_config_dialog(
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
