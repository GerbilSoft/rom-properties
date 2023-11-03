/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * ConfigDialog.cpp: Configuration dialog.                                 *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "librpbase/config.librpbase.h"
#include "ConfigDialog.hpp"
#include "res/resource.h"

// librpbase
using namespace LibRpBase;

// Property sheet icon.
// Extracted from imageres.dll or shell32.dll.
#include "PropSheetIcon.hpp"

// Controls for registration.
#include "LanguageComboBox.hpp"

// C++ STL classes
using std::tstring;

#include "libi18n/config.libi18n.h"
#if defined(_MSC_VER) && defined(ENABLE_NLS)
// MSVC: Exception handling for /DELAYLOAD.
#  include "libwin32common/DelayLoadHelper.h"
// DelayLoad test implementation.
DELAYLOAD_TEST_FUNCTION_IMPL1(textdomain, nullptr);
#endif /* defined(_MSC_VER) && defined(ENABLE_NLS) */

// rp_image backend registration
#include "librptexture/img/GdiplusHelper.hpp"
#include "librptexture/img/RpGdiplusBackend.hpp"
using namespace LibRpTexture;

// Property sheet tabs
#include "ImageTypesTab.hpp"
#include "SystemsTab.hpp"
#include "OptionsTab.hpp"
#include "CacheTab.hpp"
#include "AchievementsTab.hpp"
#ifdef ENABLE_DECRYPTION
#  include "KeyManagerTab.hpp"
#endif /* ENABLE_DECRYPTION */
#include "AboutTab.hpp"

// Win32 dark mode
#include "libwin32darkmode/DarkMode.hpp"

class ConfigDialogPrivate
{
public:
	ConfigDialogPrivate();
	~ConfigDialogPrivate();

private:
	RP_DISABLE_COPY(ConfigDialogPrivate)

public:
	// Property sheet variables.
#ifdef ENABLE_DECRYPTION
	static const unsigned int TAB_COUNT = 7;
#else
	static const unsigned int TAB_COUNT = 6;
#endif
	std::array<ITab*, TAB_COUNT> tabs;
	std::array<HPROPSHEETPAGE, TAB_COUNT> hpsp;
	PROPSHEETHEADER psh;

	// Property Sheet callback.
	static int CALLBACK callbackProc(HWND hDlg, UINT uMsg, LPARAM lParam);

	// Subclass procedure for the Property Sheet.
	static LRESULT CALLBACK subclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);

	// Create Property Sheet.
	static INT_PTR CreatePropertySheet(void);
};

/** ConfigDialogPrivate **/

ConfigDialogPrivate::ConfigDialogPrivate()
{
	// Make sure we have all required window classes available.
	// Reference: https://docs.microsoft.com/en-us/windows/win32/api/commctrl/ns-commctrl-initcommoncontrolsex
	INITCOMMONCONTROLSEX initCommCtrl;
	initCommCtrl.dwSize = sizeof(initCommCtrl);
	initCommCtrl.dwICC =
		ICC_LISTVIEW_CLASSES |
		ICC_LINK_CLASS |
		ICC_TAB_CLASSES |
		ICC_PROGRESS_CLASS;

	// TODO: Also ICC_STANDARD_CLASSES on XP+?
	InitCommonControlsEx(&initCommCtrl);

	// Initialize our custom controls.
	LanguageComboBoxRegister();

	// Initialize the property sheet tabs.
	tabs[0] = new ImageTypesTab();
	tabs[1] = new SystemsTab();
	tabs[2] = new OptionsTab();
	tabs[3] = new CacheTab();
	tabs[4] = new AchievementsTab();
#ifdef ENABLE_DECRYPTION
	tabs[5] = new KeyManagerTab();
#endif /* ENABLE_DECRYPTION */
	tabs[TAB_COUNT-1] = new AboutTab();

	// Get the HPROPSHEETPAGEs.
	for (unsigned int i = 0; i < TAB_COUNT; i++) {
		hpsp[i] = tabs[i]->getHPropSheetPage();
	}

	// "ROM chip" icon
	const PropSheetIcon *const psi = PropSheetIcon::instance();

	// Create the property sheet
	psh.dwSize = sizeof(psh);
	psh.dwFlags = PSH_USECALLBACK | PSH_NOCONTEXTHELP | PSH_USEHICON;
	psh.hwndParent = nullptr;
	psh.hInstance = HINST_THISCOMPONENT;
	psh.hIcon = psi->getSmallIcon();	// Small icon only!
	psh.pszCaption = nullptr;			// will be set in WM_SHOWWINDOW
	psh.nPages = static_cast<UINT>(hpsp.size());
	psh.nStartPage = 0;
	psh.phpage = hpsp.data();
	psh.pfnCallback = this->callbackProc;
	psh.hbmWatermark = nullptr;
	psh.hplWatermark = nullptr;
	psh.hbmHeader = nullptr;

	// Property sheet will be displayed when ConfigDialog::exec() is called.
}

ConfigDialogPrivate::~ConfigDialogPrivate()
{
	// Delete the tabs.
	for (ITab *pTab : tabs) {
		delete pTab;
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
	RP_UNUSED(lParam);

	switch (uMsg) {
		case PSCB_INITIALIZED: {
			// Property sheet has been initialized.

			// Add the system menu and minimize box.
			LONG style = GetWindowLong(hDlg, GWL_STYLE);
			style |= WS_MINIMIZEBOX | WS_SYSMENU;
			SetWindowLong(hDlg, GWL_STYLE, style);

			// Restore the default system menu.
			// Not only is this needed to restore the default entries,
			// it's needed to make the Minimize button work on Windows 8.1
			// and Windows 10 (and possibly Windows 8.0 as well).
			// References:
			// - http://ntcoder.com/bab/2008/03/27/making-a-property-sheet-window-resizable/
			// - https://www.experts-exchange.com/articles/1521/Using-a-Property-Sheet-as-your-Main-Window.html
			GetSystemMenu(hDlg, false);
			GetSystemMenu(hDlg, true);

			// Remove the context help box.
			// NOTE: Setting WS_MINIMIZEBOX does this,
			// but we should remove the style anyway.
			LONG exstyle = GetWindowLong(hDlg, GWL_EXSTYLE);
			exstyle &= ~WS_EX_CONTEXTHELP;
			SetWindowLong(hDlg, GWL_EXSTYLE, exstyle);

			// NOTE: PropertySheet's pszIcon only uses the small icon.
			// Set the large icon here.
			const PropSheetIcon *const psi = PropSheetIcon::instance();
			HICON hIcon = psi->getLargeIcon();
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
	RP_UNUSED(dwRefData);

	switch (uMsg) {
		case WM_NCDESTROY:
			// Remove the window subclass.
			// Reference: https://devblogs.microsoft.com/oldnewthing/20031111-00/?p=41883
			RemoveWindowSubclass(hWnd, subclassProc, uIdSubclass);
			break;

		case WM_SHOWWINDOW: {
			//  NOTE: This should be in WM_CREATE, but we don't receive WM_CREATE here.
			if (g_darkModeSupported) {
				_SetWindowTheme(hWnd, L"CFD", NULL);
				_AllowDarkModeForWindow(hWnd, true);
				RefreshTitleBarThemeColor(hWnd);
			}

			// Check for RTL.
			if (LibWin32UI::isSystemRTL() != 0) {
				// Set the dialog to allow automatic right-to-left adjustment.
				LONG_PTR lpExStyle = GetWindowLongPtr(hWnd, GWL_EXSTYLE);
				lpExStyle |= WS_EX_LAYOUTRTL;
				SetWindowLongPtr(hWnd, GWL_EXSTYLE, lpExStyle);
			}

			// tr: Dialog title.
			const tstring tsTitle = U82T_c(C_("ConfigDialog", "ROM Properties Page configuration"));
			SetWindowText(hWnd, tsTitle.c_str());

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

			// TODO: Verify RTL positioning.
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

			HWND hBtnReset = CreateWindowEx(0, WC_BUTTON,
				// tr: "Reset" button.
				U82T_c(C_("ConfigDialog", "&Reset")),
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
			HWND hBtnDefaults = CreateWindowEx(0, WC_BUTTON,
				// tr: "Defaults" button.
				U82T_c(C_("ConfigDialog", "Defaults")),
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
					for (int i = TAB_COUNT-1; i >= 0; i--) {
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

					// KDE Plasma 5's System Settings keeps focus on the "Defaults" button,
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

		case WM_DEVICECHANGE: {
			// Forward WM_DEVICECHANGE to CacheTab.
			// NOTE: PropSheet_IndexToHwnd() may return nullptr if
			// CacheTab hasn't been initialized yet.
			HWND hwndCacheTab = PropSheet_IndexToHwnd(hWnd, 2);
			if (hwndCacheTab) {
				SendMessage(hwndCacheTab, uMsg, wParam, lParam);
			}
			break;
		}

		case PSM_CHANGED:
			// A property sheet is telling us that something has changed.
			// Enable the "Reset" button.
			EnableWindow(GetDlgItem(hWnd, IDC_RP_RESET), TRUE);
			break;

		case WM_RP_PROP_SHEET_ENABLE_DEFAULTS:
			// Enable/disable the "Defaults" button.
			EnableWindow(GetDlgItem(hWnd, IDC_RP_DEFAULTS), (BOOL)wParam);
			break;

		case WM_SETTINGCHANGE:
			if (IsColorSchemeChangeMessage(lParam)) {
				g_darkModeEnabled = _ShouldAppsUseDarkMode() && !IsHighContrast();
				RefreshTitleBarThemeColor(hWnd);
			}

		default:
			break;
	}

	return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

/** ConfigDialog **/

ConfigDialog::ConfigDialog()
	: d_ptr(new ConfigDialogPrivate())
{}

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
	// TODO: Handle hWnd and nCmdShow?
	RP_UNUSED(hInstance);
	RP_UNUSED(pszCmdLine);

#if defined(_MSC_VER) && defined(ENABLE_NLS)
	// Delay load verification.
	// TODO: Only if linked with /DELAYLOAD?
	if (DelayLoad_test_textdomain() != 0) {
		// Delay load failed.
		// TODO: Use a CMake macro for the soversion?
		MessageBox(hWnd,
			LIBGNUINTL_DLL _T(" could not be loaded.\n\n")
			_T("This build of rom-properties has localization enabled,\n")
			_T("which requires the use of GNU texttext.\n\n")
			_T("Please redownload rom-properties and copy the\n")
			LIBGNUINTL_DLL _T(" file to the installation directory."),
			LIBGNUINTL_DLL _T(" not found"),
			MB_ICONSTOP);
		return EXIT_FAILURE;
	}
#endif /* defined(_MSC_VER) && defined(ENABLE_NLS) */

	// Make sure COM is initialized.
	// NOTE: Using apartment threading for OLE compatibility.
	// TODO: What if COM is already initialized?
	HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
	if (FAILED(hr)) {
		// Failed to initialize COM.
		return EXIT_FAILURE;
	}

	// Initialize GDI+.
	ULONG_PTR gdipToken = GdiplusHelper::InitGDIPlus();
	assert(gdipToken != 0);
	if (gdipToken == 0) {
		return EXIT_FAILURE;
	}

	// Initialize i18n.
	rp_i18n_init();

	// Enable dark mode if it's available.
	InitDarkMode();

	ConfigDialog *cfg = new ConfigDialog();
	INT_PTR ret = cfg->exec();
	delete cfg;

	// Shut down GDI+.
	GdiplusHelper::ShutdownGDIPlus(gdipToken);

	// Uninitialize COM.
	CoUninitialize();

	return EXIT_SUCCESS;
}
