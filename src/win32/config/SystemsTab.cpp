/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * SystemsTab.cpp: Systems tab for rp-config.                              *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "SystemsTab.hpp"
#include "res/resource.h"

// librpbase, librpfile
using namespace LibRpBase;
using namespace LibRpFile;

// libwin32ui
#include "libwin32ui/LoadResource_i18n.hpp"
using LibWin32UI::LoadDialog_i18n;

// Win32 dark mode
#include "libwin32darkmode/DarkMode.hpp"
#include "libwin32darkmode/DarkModeCtrl.hpp"

// C++ STL classes
using std::tstring;

class SystemsTabPrivate
{
public:
	SystemsTabPrivate();
	~SystemsTabPrivate();

private:
	RP_DISABLE_COPY(SystemsTabPrivate)

public:
	/**
	 * Reset the configuration.
	 */
	void reset(void);

	/**
	 * Load the default configuration.
	 * This does NOT save, and will only emit modified()
	 * if it's different from the current configuration.
	 */
	void loadDefaults(void);

	/**
	 * Save the configuration.
	 */
	void save(void);

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

	// Has the user changed anything?
	bool changed;

public:
	// Dark Mode background brush
	HBRUSH hbrBkgnd;
	bool lastDarkModeEnabled;
};

/** SystemsTabPrivate **/

SystemsTabPrivate::SystemsTabPrivate()
	: hPropSheetPage(nullptr)
	, hWndPropSheet(nullptr)
	, changed(false)
	, hbrBkgnd(nullptr)
	, lastDarkModeEnabled(false)
{}

SystemsTabPrivate::~SystemsTabPrivate()
{
	// Dark mode background brush
	if (hbrBkgnd) {
		DeleteBrush(hbrBkgnd);
	}
}

/**
 * Reset the configuration.
 */
void SystemsTabPrivate::reset(void)
{
	assert(hWndPropSheet != nullptr);
	if (!hWndPropSheet)
		return;

	// NOTE: This may re-check the configuration timestamp.
	const Config *const config = Config::instance();

	// Special handling: DMG as SGB doesn't really make sense,
	// so handle it as DMG.
	const Config::DMG_TitleScreen_Mode tsMode =
		config->dmgTitleScreenMode(Config::DMG_TitleScreen_Mode::DMG_TS_DMG);
	const HWND hwndDmgTsDMG = GetDlgItem(hWndPropSheet, IDC_SYSTEMS_DMGTS_DMG);
	switch (tsMode) {
		case Config::DMG_TitleScreen_Mode::DMG_TS_DMG:
		case Config::DMG_TitleScreen_Mode::DMG_TS_SGB:
		default:
			ComboBox_SetCurSel(hwndDmgTsDMG, 0);
			break;
		case Config::DMG_TitleScreen_Mode::DMG_TS_CGB:
			ComboBox_SetCurSel(hwndDmgTsDMG, 1);
			break;
	}

	// The SGB and CGB dropdowns have all three.
	ComboBox_SetCurSel(GetDlgItem(hWndPropSheet, IDC_SYSTEMS_DMGTS_SGB),
		(int)config->dmgTitleScreenMode(Config::DMG_TitleScreen_Mode::DMG_TS_SGB));
	ComboBox_SetCurSel(GetDlgItem(hWndPropSheet, IDC_SYSTEMS_DMGTS_CGB),
		(int)config->dmgTitleScreenMode(Config::DMG_TitleScreen_Mode::DMG_TS_CGB));

	// No longer changed.
	changed = false;
}

void SystemsTabPrivate::loadDefaults(void)
{
	assert(hWndPropSheet != nullptr);
	if (!hWndPropSheet)
		return;

	// TODO: Get the defaults from Config.
	// For now, hard-coding everything here.
	static const int8_t idxDMG_default = 0;
	static const int8_t idxSGB_default = 1;
	static const int8_t idxCGB_default = 2;
	bool isDefChanged = false;

	HWND hwndDmgTs = GetDlgItem(hWndPropSheet, IDC_SYSTEMS_DMGTS_DMG);
	if (ComboBox_GetCurSel(hwndDmgTs) != idxDMG_default) {
		ComboBox_SetCurSel(hwndDmgTs, idxDMG_default);
		isDefChanged = true;
	}
	hwndDmgTs = GetDlgItem(hWndPropSheet, IDC_SYSTEMS_DMGTS_SGB);
	if (ComboBox_GetCurSel(hwndDmgTs) != idxSGB_default) {
		ComboBox_SetCurSel(hwndDmgTs, idxSGB_default);
		isDefChanged = true;
	}
	hwndDmgTs = GetDlgItem(hWndPropSheet, IDC_SYSTEMS_DMGTS_CGB);
	if (ComboBox_GetCurSel(hwndDmgTs) != idxCGB_default) {
		ComboBox_SetCurSel(hwndDmgTs, idxCGB_default);
		isDefChanged = true;
	}

	if (isDefChanged) {
		this->changed = true;
		PropSheet_Changed(GetParent(hWndPropSheet), hWndPropSheet);
	}
}

/**
 * Save the configuration.
 */
void SystemsTabPrivate::save(void)
{
	assert(hWndPropSheet != nullptr);
	if (!hWndPropSheet)
		return;

	// NOTE: This may re-check the configuration timestamp.
	const Config *const config = Config::instance();
	const char *const filename = config->filename();
	if (!filename) {
		// No configuration filename...
		return;
	}

	// Make sure the configuration directory exists.
	// NOTE: The filename portion MUST be kept in config_path,
	// since the last component is ignored by rmkdir().
	int ret = FileSystem::rmkdir(filename);
	if (ret != 0) {
		// rmkdir() failed.
		return;
	}

	const tstring tfilename = U82T_c(filename);

	const TCHAR s_dmg_dmg[][4] = {_T("DMG"), _T("CGB")};
	const int idxDMG = ComboBox_GetCurSel(GetDlgItem(hWndPropSheet, IDC_SYSTEMS_DMGTS_DMG));
	assert(idxDMG >= 0);
	assert(idxDMG < ARRAY_SIZE_I(s_dmg_dmg));
	if (idxDMG >= 0 && idxDMG < ARRAY_SIZE_I(s_dmg_dmg)) {
		WritePrivateProfileString(_T("DMGTitleScreenMode"), _T("DMG"), s_dmg_dmg[idxDMG], tfilename.c_str());
	}

	const TCHAR s_dmg_other[][4] = {_T("DMG"), _T("SGB"), _T("CGB")};
	const int idxSGB = ComboBox_GetCurSel(GetDlgItem(hWndPropSheet, IDC_SYSTEMS_DMGTS_SGB));
	const int idxCGB = ComboBox_GetCurSel(GetDlgItem(hWndPropSheet, IDC_SYSTEMS_DMGTS_CGB));

	assert(idxSGB >= 0);
	assert(idxSGB < ARRAY_SIZE_I(s_dmg_other));
	if (idxSGB >= 0 && idxSGB < ARRAY_SIZE_I(s_dmg_other)) {
		WritePrivateProfileString(_T("DMGTitleScreenMode"), _T("SGB"), s_dmg_other[idxSGB], tfilename.c_str());
	}
	assert(idxCGB >= 0);
	assert(idxCGB < ARRAY_SIZE_I(s_dmg_other));
	if (idxCGB >= 0 && idxCGB < ARRAY_SIZE_I(s_dmg_other)) {
		WritePrivateProfileString(_T("DMGTitleScreenMode"), _T("CGB"), s_dmg_other[idxCGB], tfilename.c_str());
	}

	// No longer changed.
	changed = false;
}

/**
 * Dialog procedure.
 * @param hDlg
 * @param uMsg
 * @param wParam
 * @param lParam
 */
INT_PTR CALLBACK SystemsTabPrivate::dlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
		case WM_INITDIALOG: {
			// Get the pointer to the property sheet page object. This is 
			// contained in the LPARAM of the PROPSHEETPAGE structure.
			LPPROPSHEETPAGE pPage = reinterpret_cast<LPPROPSHEETPAGE>(lParam);
			if (!pPage)
				return TRUE;

			// Get the pointer to the SystemsTabPrivate object.
			SystemsTabPrivate *const d = reinterpret_cast<SystemsTabPrivate*>(pPage->lParam);
			if (!d)
				return TRUE;

			assert(d->hWndPropSheet == nullptr);
			d->hWndPropSheet = hDlg;

			// Store the D object pointer with this particular page dialog.
			SetWindowLongPtr(hDlg, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(d));

			//  NOTE: This should be in WM_CREATE, but we don't receive WM_CREATE here.
			DarkMode_InitDialog(hDlg);
			d->lastDarkModeEnabled = g_darkModeEnabled;

			// Populate the combo boxes.
			HWND hwndDmgTs = GetDlgItem(hDlg, IDC_SYSTEMS_DMGTS_DMG);
			ComboBox_AddString(hwndDmgTs, _T("Game Boy"));
			ComboBox_AddString(hwndDmgTs, _T("Game Boy Color"));
			hwndDmgTs = GetDlgItem(hDlg, IDC_SYSTEMS_DMGTS_SGB);
			ComboBox_AddString(hwndDmgTs, _T("Game Boy"));
			ComboBox_AddString(hwndDmgTs, _T("Super Game Boy"));
			ComboBox_AddString(hwndDmgTs, _T("Game Boy Color"));
			hwndDmgTs = GetDlgItem(hDlg, IDC_SYSTEMS_DMGTS_CGB);
			ComboBox_AddString(hwndDmgTs, _T("Game Boy"));
			ComboBox_AddString(hwndDmgTs, _T("Super Game Boy"));
			ComboBox_AddString(hwndDmgTs, _T("Game Boy Color"));

			// Set window themes for Win10's dark mode.
			// FIXME: Not working for BS_GROUPBOX.
			if (g_darkModeSupported) {
				DarkMode_InitButton_Dlg(hDlg, IDC_SYSTEMS_DMGTS_GROUPBOX);
				DarkMode_InitComboBox_Dlg(hDlg, IDC_SYSTEMS_DMGTS_DMG);
				DarkMode_InitComboBox_Dlg(hDlg, IDC_SYSTEMS_DMGTS_SGB);
				DarkMode_InitComboBox_Dlg(hDlg, IDC_SYSTEMS_DMGTS_CGB);
			}

			// Reset the configuration.
			d->reset();
			return TRUE;
		}

		case WM_NOTIFY: {
			auto *const d = reinterpret_cast<SystemsTabPrivate*>(GetWindowLongPtr(hDlg, GWLP_USERDATA));
			if (!d) {
				// No SystemsTabPrivate. Can't do anything...
				return FALSE;
			}

			NMHDR *pHdr = reinterpret_cast<NMHDR*>(lParam);
			switch (pHdr->code) {
				case PSN_APPLY:
					// Save settings.
					if (d->changed) {
						d->save();
					}
					break;

				case PSN_SETACTIVE:
					// Enable the "Defaults" button.
					RpPropSheet_EnableDefaults(GetParent(hDlg), true);
					break;

				default:
					break;
			}
			break;
		}

		case WM_COMMAND: {
			auto *const d = reinterpret_cast<SystemsTabPrivate*>(GetWindowLongPtr(hDlg, GWLP_USERDATA));
			if (!d) {
				// No SystemsTabPrivate. Can't do anything...
				return FALSE;
			}

			if (HIWORD(wParam) != CBN_SELCHANGE)
				break;

			// A combobox's selection has been changed.
			// Page has been modified.
			PropSheet_Changed(GetParent(hDlg), hDlg);
			d->changed = true;
			break;
		}

		case WM_RP_PROP_SHEET_RESET: {
			auto *const d = reinterpret_cast<SystemsTabPrivate*>(GetWindowLongPtr(hDlg, GWLP_USERDATA));
			if (!d) {
				// No SystemsTabPrivate. Can't do anything...
				return FALSE;
			}

			// Reset the tab.
			d->reset();
			break;
		}

		case WM_RP_PROP_SHEET_DEFAULTS: {
			auto *const d = reinterpret_cast<SystemsTabPrivate*>(GetWindowLongPtr(hDlg, GWLP_USERDATA));
			if (!d) {
				// No SystemsTabPrivate. Can't do anything...
				return FALSE;
			}

			// Load the defaults.
			d->loadDefaults();
			break;
		}

		/** Dark Mode **/

		case WM_CTLCOLORDLG:
		case WM_CTLCOLORSTATIC:
			if (g_darkModeSupported && g_darkModeEnabled) {
				auto *const d = reinterpret_cast<SystemsTabPrivate*>(GetWindowLongPtr(hDlg, GWLP_USERDATA));
				if (!d) {
					// No SystemsTabPrivate. Can't do anything...
					return FALSE;
				}

				HDC hdc = reinterpret_cast<HDC>(wParam);
				SetTextColor(hdc, g_darkTextColor);
				SetBkColor(hdc, g_darkBkColor);
				if (!d->hbrBkgnd) {
					d->hbrBkgnd = CreateSolidBrush(g_darkBkColor);
				}
				return reinterpret_cast<INT_PTR>(d->hbrBkgnd);
			}
			break;

		case WM_SETTINGCHANGE:
			if (g_darkModeSupported && IsColorSchemeChangeMessage(lParam)) {
				SendMessage(hDlg, WM_THEMECHANGED, 0, 0);
			}
			break;

		case WM_THEMECHANGED:
			if (g_darkModeSupported) {
				auto *const d = reinterpret_cast<SystemsTabPrivate*>(GetWindowLongPtr(hDlg, GWLP_USERDATA));
				if (!d) {
					// No SystemsTabPrivate. Can't do anything...
					return FALSE;
				}

				UpdateDarkModeEnabled();
				if (d->lastDarkModeEnabled != g_darkModeEnabled) {
					d->lastDarkModeEnabled = g_darkModeEnabled;
					InvalidateRect(hDlg, NULL, true);

					// Propagate WM_THEMECHANGED to window controls that don't
					// automatically handle Dark Mode changes, e.g. ComboBox and Button.
					SendMessage(GetDlgItem(hDlg, IDC_SYSTEMS_DMGTS_DMG), WM_THEMECHANGED, 0, 0);
					SendMessage(GetDlgItem(hDlg, IDC_SYSTEMS_DMGTS_SGB), WM_THEMECHANGED, 0, 0);
					SendMessage(GetDlgItem(hDlg, IDC_SYSTEMS_DMGTS_CGB), WM_THEMECHANGED, 0, 0);
				}
			}
			break;

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
UINT CALLBACK SystemsTabPrivate::callbackProc(HWND hWnd, UINT uMsg, LPPROPSHEETPAGE ppsp)
{
	RP_UNUSED(hWnd);
	RP_UNUSED(ppsp);

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

/** SystemsTab **/

SystemsTab::SystemsTab(void)
	: d_ptr(new SystemsTabPrivate())
{}

SystemsTab::~SystemsTab()
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
HPROPSHEETPAGE SystemsTab::getHPropSheetPage(void)
{
	RP_D(SystemsTab);
	assert(d->hPropSheetPage == nullptr);
	if (d->hPropSheetPage) {
		// Property sheet has already been created.
		return nullptr;
	}

	// tr: Tab title.
	const tstring tsTabTitle = U82T_c(C_("SystemsTab", "Systems"));

	PROPSHEETPAGE psp;
	psp.dwSize = sizeof(psp);
	psp.dwFlags = PSP_USECALLBACK | PSP_USETITLE | PSP_DLGINDIRECT;
	psp.hInstance = HINST_THISCOMPONENT;
	psp.pResource = LoadDialog_i18n(HINST_THISCOMPONENT, IDD_CONFIG_SYSTEMS);
	psp.pszIcon = nullptr;
	psp.pszTitle = tsTabTitle.c_str();
	psp.pfnDlgProc = SystemsTabPrivate::dlgProc;
	psp.lParam = reinterpret_cast<LPARAM>(d);
	psp.pcRefParent = nullptr;
	psp.pfnCallback = SystemsTabPrivate::callbackProc;

	d->hPropSheetPage = CreatePropertySheetPage(&psp);
	return d->hPropSheetPage;
}

/**
 * Reset the contents of this tab.
 */
void SystemsTab::reset(void)
{
	RP_D(SystemsTab);
	d->reset();
}

/**
 * Load the default configuration.
 * This does NOT save, and will only emit modified()
 * if it's different from the current configuration.
 */
void SystemsTab::loadDefaults(void)
{
	RP_D(SystemsTab);
	d->loadDefaults();
}

/**
 * Save the contents of this tab.
 */
void SystemsTab::save(void)
{
	RP_D(SystemsTab);
	if (d->changed) {
		d->save();
	}
}
