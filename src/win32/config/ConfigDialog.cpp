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
#include "librpbase/config.librpbase.h"
#include "ConfigDialog.hpp"
#include "resource.h"

// librpbase
#include "librpbase/config/Config.hpp"
using LibRpBase::Config;

// Property sheet icon.
// Extracted from imageres.dll or shell32.dll.
#include "PropSheetIcon.hpp"

// C includes.
#include <stdlib.h>

// C includes. (C++ namespace)
#include <cassert>

// Property sheet tabs.
#include "ImageTypesTab.hpp"
#include "DownloadsTab.hpp"
#include "CacheTab.hpp"
#ifdef ENABLE_DECRYPTION
#include "KeyManagerTab.hpp"
#endif /* ENABLE_DECRYPTION */
#include "AboutTab.hpp"

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
#ifdef ENABLE_DECRYPTION
		static const unsigned int TAB_COUNT = 5;
#else
		static const unsigned int TAB_COUNT = 4;
#endif
		ITab *tabs[TAB_COUNT];
		HPROPSHEETPAGE hpsp[TAB_COUNT];
		PROPSHEETHEADER psh;

		// Property Sheet callback.
		static int CALLBACK callbackProc(HWND hDlg, UINT uMsg, LPARAM lParam);

		// Subclass procedure for the Property Sheet.
		static LRESULT CALLBACK subclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);

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
	initCommCtrl.dwICC =
		ICC_LISTVIEW_CLASSES |
		ICC_LINK_CLASS |
		ICC_TAB_CLASSES |
		ICC_PROGRESS_CLASS;

	// TODO: Also ICC_STANDARD_CLASSES on XP+?
	InitCommonControlsEx(&initCommCtrl);

	// Load RICHED20.DLL for RICHEDIT_CLASS.
	// TODO: What if this fails?
	HMODULE hRichEd20 = LoadLibrary(L"RICHED20.DLL");

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
#ifdef ENABLE_DECRYPTION
	// Key Manager.
	tabs[3] = new KeyManagerTab();
	hpsp[3] = tabs[3]->getHPropSheetPage();
#endif /* ENABLE_DECRYPTION */

	// About.
	tabs[TAB_COUNT-1] = new AboutTab();
	hpsp[TAB_COUNT-1] = tabs[TAB_COUNT-1]->getHPropSheetPage();

	// Create the property sheet.
	psh.dwSize = sizeof(psh);
	psh.dwFlags = PSH_USECALLBACK | PSH_NOCONTEXTHELP | PSH_USEHICON;
	psh.hwndParent = nullptr;
	psh.hInstance = HINST_THISCOMPONENT;
	psh.hIcon = PropSheetIcon::getSmallIcon();	// Small icon only!
	psh.pszCaption = L"ROM Properties Page Configuration";
	psh.nPages = ARRAY_SIZE(hpsp);
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

			// Subclass the property sheet so we can create the
			// "Reset" button in WM_SHOWWINDOW.
			SetWindowSubclass(hDlg, subclassProc, 0, 0);
			break;
		}

		default:
			break;
	}

	return 0;
}

/**
 * Subclass procedure for the Property Sheet.
 * @param hWnd		Control handle.
 * @param uMsg		Message.
 * @param wParam	WPARAM
 * @param lParam	LPARAM
 * @param uIdSubclass	Subclass ID. (usually the control ID)
 * @param dwRefData
 */
LRESULT CALLBACK ConfigDialogPrivate::subclassProc(
	HWND hWnd, UINT uMsg,
	WPARAM wParam, LPARAM lParam,
	UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	switch (uMsg) {
		case WM_SHOWWINDOW: {
			// Create the "Reset" and "Defaults" buttons.
			if (GetDlgItem(hWnd, IDC_RP_RESET) != nullptr) {
				// "Reset" button is already created.
				// This shouldn't happen...
				assert(!"IDC_RP_RESET is already created.");
				break;
			} else if (GetDlgItem(hWnd, IDC_RP_DEFAULTS) != nullptr) {
				// "Defaults" button is already created.
				// This shouldn't happen...
				assert(!"IDC_RP_DEFAULTS is already created.");
				break;
			}

			HWND hBtnOK = GetDlgItem(hWnd, IDOK);
			HWND hBtnCancel = GetDlgItem(hWnd, IDCANCEL);
			HWND hTabControl = PropSheet_GetTabControl(hWnd);
			if (!hBtnOK || !hBtnCancel || !hTabControl)
				break;

			RECT rect_btnOK, rect_btnCancel, rect_tabControl;
			GetWindowRect(hBtnOK, &rect_btnOK);
			GetWindowRect(hBtnCancel, &rect_btnCancel);
			GetWindowRect(hTabControl, &rect_tabControl);
			MapWindowPoints(HWND_DESKTOP, hWnd, (LPPOINT)&rect_btnOK, 2);
			MapWindowPoints(HWND_DESKTOP, hWnd, (LPPOINT)&rect_btnCancel, 2);
			MapWindowPoints(HWND_DESKTOP, hWnd, (LPPOINT)&rect_tabControl, 2);

			// Dialog font.
			HFONT hDlgFont = GetWindowFont(hWnd);

			// Create the "Reset" button.
			POINT ptBtn = {rect_tabControl.left, rect_btnOK.top};
			const SIZE szBtn = {
				rect_btnOK.right - rect_btnOK.left,
				rect_btnOK.bottom - rect_btnOK.top
			};

			HWND hBtnReset = CreateWindowEx(0,
				WC_BUTTON, L"Reset",
				WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_GROUP | BS_CENTER,
				ptBtn.x, ptBtn.y, szBtn.cx, szBtn.cy,
				hWnd, (HMENU)IDC_RP_RESET, nullptr, nullptr);
			SetWindowFont(hBtnReset, hDlgFont, FALSE);
			EnableWindow(hBtnReset, FALSE);

			// Fix up the tab order. ("Reset" should be after "Apply".)
			HWND hBtnApply = GetDlgItem(hWnd, IDC_APPLY_BUTTON);
			if (hBtnApply) {
				SetWindowPos(hBtnReset, hBtnApply,
					0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
			}

			// Create the "Defaults" button.
			ptBtn.x += szBtn.cx + (rect_btnCancel.left - rect_btnOK.right);
			HWND hBtnDefaults = CreateWindowEx(0,
				WC_BUTTON, L"Defaults",
				WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_GROUP | BS_CENTER,
				ptBtn.x, ptBtn.y, szBtn.cx, szBtn.cy,
				hWnd, (HMENU)IDC_RP_DEFAULTS, nullptr, nullptr);
			SetWindowFont(hBtnDefaults, hDlgFont, FALSE);

			// Fix up the tab order. ("Defaults" should be after "Reset".)
			SetWindowPos(hBtnDefaults, hBtnReset,
				0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
			break;
		}

		case WM_COMMAND: {
			if (HIWORD(wParam) != BN_CLICKED)
				break;

			switch (LOWORD(wParam)) {
				case IDC_APPLY_BUTTON:
					// "Apply" was clicked.
					// Disable the "Reset" button.
					EnableWindow(GetDlgItem(hWnd, IDC_RP_RESET), FALSE);
					break;

				case IDC_RP_RESET: {
					// "Reset" was clicked.
					// Reset all of the tabs.
					for (unsigned int i = 0; i < TAB_COUNT; i++) {
						HWND hwndPropSheet = PropSheet_IndexToHwnd(hWnd, i);
						if (hwndPropSheet) {
							SendMessage(hwndPropSheet, WM_RP_PROP_SHEET_RESET, 0, 0);
						}
					}

					// Set focus to the tab control.
					SetFocus(PropSheet_GetTabControl(hWnd));
					// Go to the next control.
					SendMessage(hWnd, WM_NEXTDLGCTL, 0, FALSE);

					// TODO: Clear the "changed" state in the property sheet?
					// Disable the "Apply" and "Reset" buttons.
					EnableWindow(GetDlgItem(hWnd, IDC_APPLY_BUTTON), FALSE);
					EnableWindow(GetDlgItem(hWnd, IDC_RP_RESET), FALSE);

					// Don't continue processing. Otherwise, weird things
					// will happen with the button message.
					return TRUE;
				}

				case IDC_RP_DEFAULTS: {
					// "Defaults" was clicked.
					// Load the defaults in the current tab.
					HWND hwndPropSheet = PropSheet_GetCurrentPageHwnd(hWnd);
					if (hwndPropSheet) {
						SendMessage(hwndPropSheet, WM_RP_PROP_SHEET_DEFAULTS, 0, 0);
					}

					// KDE5 System Settings keeps focus on the "Defaults" button,
					// so we'll leave the focus as-is.

					// Don't continue processing. Otherwise, weird things
					// will happen with the button message.
					return TRUE;
				}

				default:
					break;
			}

			break;
		}

		case PSM_CHANGED:
			// A property sheet is telling us that something has changed.
			// Enable the "Reset" button.
			EnableWindow(GetDlgItem(hWnd, IDC_RP_RESET), TRUE);
			break;

		case WM_NCDESTROY:
			// Remove the window subclass.
			// Reference: https://blogs.msdn.microsoft.com/oldnewthing/20031111-00/?p=41883
			RemoveWindowSubclass(hWnd, subclassProc, uIdSubclass);
			break;

		case WM_RP_PROP_SHEET_ENABLE_DEFAULTS:
			// Enable/disable the "Defaults" button.
			EnableWindow(GetDlgItem(hWnd, IDC_RP_DEFAULTS), (BOOL)wParam);
			break;

		default:
			break;
	}

	return DefSubclassProc(hWnd, uMsg, wParam, lParam);
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
	delete cfg;

	// Uninitialize COM.
	CoUninitialize();

	return EXIT_SUCCESS;
}
