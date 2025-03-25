/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * OptionsTab.cpp: Options tab for rp-config.                              *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "config.win32.h"

#include "OptionsTab.hpp"
#include "res/resource.h"

// LanguageComboBox
#include "LanguageComboBox.hpp"

// librpbase, librpfile
#include "librpbase/SystemRegion.hpp"
using namespace LibRpBase;
using namespace LibRpFile;

// libwin32ui
#include "libwin32ui/LoadResource_i18n.hpp"
using LibWin32UI::LoadDialog_i18n;

// Win32 dark mode
#include "libwin32darkmode/DarkMode.hpp"
#include "libwin32darkmode/DarkModeCtrl.hpp"

// Netowrk status
#ifdef ENABLE_NETWORKING
#  include "NetworkStatus.h"
#endif /* ENABLE_NETWORKING */

// C++ STL classes
using std::array;
using std::tstring;

class OptionsTabPrivate
{
public:
	OptionsTabPrivate();
	~OptionsTabPrivate();

private:
	RP_DISABLE_COPY(OptionsTabPrivate)

protected:
	/**
	 * Convert a bool value to BST_CHCEKED or BST_UNCHECKED.
	 * @param value Bool value.
	 * @return BST_CHECKED or BST_UNCHECKED.
	 */
	static inline constexpr int boolToBstChecked(bool value)
	{
		return (value) ? BST_CHECKED : BST_UNCHECKED;
	}

	/**
	 * Convert BST_CHECKED or BST_UNCHECKED to a bool string.
	 * @param value BST_CHECKED or BST_UNCHECKED.
	 * @return Bool string.
	 */
	static inline constexpr LPCTSTR bstCheckedToBoolString(unsigned int value)
	{
		return (value == BST_CHECKED) ? _T("true") : _T("false");
	}

	/**
	 * Convert BST_CHECKED or BST_UNCHECKED to a bool.
	 * @param value BST_CHECKED or BST_UNCHECKED.
	 * @return bool.
	 */
	static inline constexpr bool bstCheckedToBool(unsigned int value)
	{
		return (value == BST_CHECKED);
	}

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
	 * Update grpExtImgDl's control sensitivity.
	 */
	void updateGrpExtImgDl(void);

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

/** OptionsTabPrivate **/

OptionsTabPrivate::OptionsTabPrivate()
	: hPropSheetPage(nullptr)
	, hWndPropSheet(nullptr)
	, changed(false)
	, hbrBkgnd(nullptr)
	, lastDarkModeEnabled(false)
{}

OptionsTabPrivate::~OptionsTabPrivate()
{
	// Dark mode background brush
	if (hbrBkgnd) {
		DeleteBrush(hbrBkgnd);
	}
}

/**
 * Reset the configuration.
 */
void OptionsTabPrivate::reset(void)
{
	assert(hWndPropSheet != nullptr);
	if (!hWndPropSheet)
		return;

	// NOTE: This may re-check the configuration timestamp.
	const Config *const config = Config::instance();

	// Downloads
	CheckDlgButton(hWndPropSheet, IDC_OPTIONS_CHKEXTIMGDL, boolToBstChecked(config->getBoolConfigOption(Config::BoolConfig::Downloads_ExtImgDownloadEnabled)));
	CheckDlgButton(hWndPropSheet, IDC_OPTIONS_INTICONSMALL, boolToBstChecked(config->getBoolConfigOption(Config::BoolConfig::Downloads_UseIntIconForSmallSizes)));
	CheckDlgButton(hWndPropSheet, IDC_OPTIONS_STOREFILEORIGININFO, boolToBstChecked(config->getBoolConfigOption(Config::BoolConfig::Downloads_StoreFileOriginInfo)));

	// Image bandwidth options
	ComboBox_SetCurSel(GetDlgItem(hWndPropSheet, IDC_OPTIONS_CBO_UNMETERED_DL), static_cast<int>(config->imgBandwidthUnmetered()));
	ComboBox_SetCurSel(GetDlgItem(hWndPropSheet, IDC_OPTIONS_CBO_METERED_DL), static_cast<int>(config->imgBandwidthMetered()));
	// Update sensitivity
	updateGrpExtImgDl();

	// PAL language code
	HWND cboGameTDBPAL = GetDlgItem(hWndPropSheet, IDC_OPTIONS_PALLANGUAGEFORGAMETDB);
	LanguageComboBox_SetSelectedLC(cboGameTDBPAL, config->palLanguageForGameTDB());

	// Options

	// FIXME: Re-enable IDC_OPTIONS_DANGEROUSPERMISSIONS once the
	// "dangerous" permissions overlay is working on Windows.
#if 0
	CheckDlgButton(hWndPropSheet, IDC_OPTIONS_DANGEROUSPERMISSIONS,
		boolToBstChecked(config->getBoolConfigOption(Config::BoolConfig::Options_ShowDangerousPermissionsOverlayIcon)));
#else
	CheckDlgButton(hWndPropSheet, IDC_OPTIONS_DANGEROUSPERMISSIONS, BST_UNCHECKED);
	EnableWindow(GetDlgItem(hWndPropSheet, IDC_OPTIONS_DANGEROUSPERMISSIONS), FALSE);
#endif

	CheckDlgButton(hWndPropSheet, IDC_OPTIONS_ENABLETHUMBNAILONNETWORKFS,
		boolToBstChecked(config->getBoolConfigOption(Config::BoolConfig::Options_EnableThumbnailOnNetworkFS)));

	// Thumbnail directory packages
	CheckDlgButton(hWndPropSheet, IDC_OPTIONS_THUMBNAILDIRECTORYPACKAGES,
		boolToBstChecked(config->getBoolConfigOption(Config::BoolConfig::Options_ThumbnailDirectoryPackages)));

	// Show XAttrView
	CheckDlgButton(hWndPropSheet, IDC_OPTIONS_SHOWXATTRVIEW,
		boolToBstChecked(config->getBoolConfigOption(Config::BoolConfig::Options_ShowXAttrView)));

	// No longer changed.
	changed = false;
}

void OptionsTabPrivate::loadDefaults(void)
{
	assert(hWndPropSheet != nullptr);
	if (!hWndPropSheet)
		return;
	// Has any value changed due to resetting to defaults?
	bool isDefChanged = false;

	// Downloads
	bool bcur = bstCheckedToBool(IsDlgButtonChecked(hWndPropSheet, IDC_OPTIONS_CHKEXTIMGDL));
	bool bdef = Config::getBoolConfigOption_default(Config::BoolConfig::Downloads_ExtImgDownloadEnabled);
	if (bcur != bdef) {
		CheckDlgButton(hWndPropSheet, IDC_OPTIONS_CHKEXTIMGDL, boolToBstChecked(bdef));
		isDefChanged = true;
		// Update sensitivity
		updateGrpExtImgDl();
	}
	bcur = bstCheckedToBool(IsDlgButtonChecked(hWndPropSheet, IDC_OPTIONS_INTICONSMALL));
	bdef = Config::getBoolConfigOption_default(Config::BoolConfig::Downloads_UseIntIconForSmallSizes);
	if (bcur != bdef) {
		CheckDlgButton(hWndPropSheet, IDC_OPTIONS_INTICONSMALL, boolToBstChecked(bdef));
		isDefChanged = true;
	}
	bcur = bstCheckedToBool(IsDlgButtonChecked(hWndPropSheet, IDC_OPTIONS_STOREFILEORIGININFO));
	bdef = Config::getBoolConfigOption_default(Config::BoolConfig::Downloads_StoreFileOriginInfo);
	if (bcur != bdef) {
		CheckDlgButton(hWndPropSheet, IDC_OPTIONS_STOREFILEORIGININFO, boolToBstChecked(bdef));
		isDefChanged = true;
	}
	HWND cboGameTDBPAL = GetDlgItem(hWndPropSheet, IDC_OPTIONS_PALLANGUAGEFORGAMETDB);
	uint32_t u32cur = LanguageComboBox_GetSelectedLC(cboGameTDBPAL);
	uint32_t u32def = Config::palLanguageForGameTDB_default();
	if (u32cur != u32def) {
		LanguageComboBox_SetSelectedLC(cboGameTDBPAL, u32def);
		isDefChanged = true;
	}

	// Image bandwidth options
	HWND cboUnmeteredDL = GetDlgItem(hWndPropSheet, IDC_OPTIONS_CBO_UNMETERED_DL);
	int icur = ComboBox_GetCurSel(cboUnmeteredDL);
	int idef = static_cast<int>(Config::imgBandwidthUnmetered_default());
	if (icur != idef) {
		ComboBox_SetCurSel(cboUnmeteredDL, idef);
		isDefChanged = true;
	}
	HWND cboMeteredDL = GetDlgItem(hWndPropSheet, IDC_OPTIONS_CBO_METERED_DL);
	icur = ComboBox_GetCurSel(cboMeteredDL);
	idef = static_cast<int>(Config::imgBandwidthMetered_default());
	if (icur != idef) {
		ComboBox_SetCurSel(cboMeteredDL, idef);
		isDefChanged = true;
	}

	// Options

#if 0
	// FIXME: Uncomment this once the "dangerous" permissions overlay
	// is working on Windows.
	bcur = bstCheckedToBool(IsDlgButtonChecked(hWndPropSheet, IDC_OPTIONS_DANGEROUSPERMISSIONS));
	bdef = Config::getBoolConfigOption_default(Config::BoolConfig::Options_ShowDangerousPermissionsOverlayIcon);
	if (bcur != bdef) {
		CheckDlgButton(hWndPropSheet, IDC_OPTIONS_DANGEROUSPERMISSIONS, boolToBstChecked(bdef));
		isDefChanged = true;
	}
#endif

	bcur = bstCheckedToBool(IsDlgButtonChecked(hWndPropSheet, IDC_OPTIONS_ENABLETHUMBNAILONNETWORKFS));
	bdef = Config::getBoolConfigOption_default(Config::BoolConfig::Options_EnableThumbnailOnNetworkFS);
	if (bcur != bdef) {
		CheckDlgButton(hWndPropSheet, IDC_OPTIONS_ENABLETHUMBNAILONNETWORKFS, boolToBstChecked(bdef));
		isDefChanged = true;
	}

	bcur = bstCheckedToBool(IsDlgButtonChecked(hWndPropSheet, IDC_OPTIONS_THUMBNAILDIRECTORYPACKAGES));
	bdef = Config::getBoolConfigOption_default(Config::BoolConfig::Options_ThumbnailDirectoryPackages);
	if (bcur != bdef) {
		CheckDlgButton(hWndPropSheet, IDC_OPTIONS_THUMBNAILDIRECTORYPACKAGES, boolToBstChecked(bdef));
		isDefChanged = true;
	}

	bcur = bstCheckedToBool(IsDlgButtonChecked(hWndPropSheet, IDC_OPTIONS_SHOWXATTRVIEW));
	bdef = Config::getBoolConfigOption_default(Config::BoolConfig::Options_ShowXAttrView);
	if (bcur != bdef) {
		CheckDlgButton(hWndPropSheet, IDC_OPTIONS_SHOWXATTRVIEW, boolToBstChecked(bdef));
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
void OptionsTabPrivate::save(void)
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

	// Downloads
	const TCHAR *btstr = bstCheckedToBoolString(IsDlgButtonChecked(hWndPropSheet, IDC_OPTIONS_CHKEXTIMGDL));
	WritePrivateProfileString(_T("Downloads"), _T("ExtImageDownload"), btstr, tfilename.c_str());

	btstr = bstCheckedToBoolString(IsDlgButtonChecked(hWndPropSheet, IDC_OPTIONS_INTICONSMALL));
	WritePrivateProfileString(_T("Downloads"), _T("UseIntIconForSmallSizes"), btstr, tfilename.c_str());

	btstr = bstCheckedToBoolString(IsDlgButtonChecked(hWndPropSheet, IDC_OPTIONS_STOREFILEORIGININFO));
	WritePrivateProfileString(_T("Downloads"), _T("StoreFileOriginInfo"), btstr, tfilename.c_str());

	uint32_t lc = LanguageComboBox_GetSelectedLC(GetDlgItem(hWndPropSheet, IDC_OPTIONS_PALLANGUAGEFORGAMETDB));
	WritePrivateProfileString(_T("Downloads"), _T("PalLanguageForGameTDB"),
		SystemRegion::lcToTString(lc).c_str(), tfilename.c_str());

	// Image bandwidth options
	const char *const sUnmetered = Config::imgBandwidthToConfSetting(
		static_cast<Config::ImgBandwidth>(ComboBox_GetCurSel(GetDlgItem(hWndPropSheet, IDC_OPTIONS_CBO_UNMETERED_DL))));
	const char *const sMetered = Config::imgBandwidthToConfSetting(
		static_cast<Config::ImgBandwidth>(ComboBox_GetCurSel(GetDlgItem(hWndPropSheet, IDC_OPTIONS_CBO_METERED_DL))));
	WritePrivateProfileString(_T("Downloads"), _T("ImgBandwidthUnmetered"), U82T_c(sUnmetered), tfilename.c_str());
	WritePrivateProfileString(_T("Downloads"), _T("ImgBandwidthMetered"), U82T_c(sMetered), tfilename.c_str());
	// TODO: Remove the old key.

	// Options
	// FIXME: Uncomment this once the "dangerous" permissions overlay
	// is working on Windows.
	/*
	btstr = bstCheckedToBoolString(IsDlgButtonChecked(hWndPropSheet, IDC_DANGEROUSPERMISSIONS));
	WritePrivateProfileString(_T("Options"), _T("ShowDangerousPermissionsOverlayIcon"), btstr, tfilename.c_str());
	*/

	btstr = bstCheckedToBoolString(IsDlgButtonChecked(hWndPropSheet, IDC_OPTIONS_ENABLETHUMBNAILONNETWORKFS));
	WritePrivateProfileString(_T("Options"), _T("EnableThumbnailOnNetworkFS"), btstr, tfilename.c_str());

	btstr = bstCheckedToBoolString(IsDlgButtonChecked(hWndPropSheet, IDC_OPTIONS_THUMBNAILDIRECTORYPACKAGES));
	WritePrivateProfileString(_T("Options"), _T("ThumbnailDirectoryPackages"), btstr, tfilename.c_str());

	btstr = bstCheckedToBoolString(IsDlgButtonChecked(hWndPropSheet, IDC_OPTIONS_SHOWXATTRVIEW));
	WritePrivateProfileString(_T("Options"), _T("ShowXAttrView"), btstr, tfilename.c_str());

	// No longer changed.
	changed = false;
}

/**
 * Update grpExtImgDl's control sensitivity.
 */
void OptionsTabPrivate::updateGrpExtImgDl(void)
{
#ifdef ENABLE_NETWORKING
	// Enable the unmetered dropdown if chkExtImgDl is checked.
	bool enable = bstCheckedToBool(IsDlgButtonChecked(hWndPropSheet, IDC_OPTIONS_CHKEXTIMGDL));
	EnableWindow(GetDlgItem(hWndPropSheet, IDC_OPTIONS_LBL_UNMETERED_DL), enable);
	EnableWindow(GetDlgItem(hWndPropSheet, IDC_OPTIONS_CBO_UNMETERED_DL), enable);

	// Enable the metered dropdown if chkExtImgDl is checked,
	// *and* metered/unmetered detection is available.
	if (!rp_win32_can_identify_if_metered()) {
		enable = false;
	}
	EnableWindow(GetDlgItem(hWndPropSheet, IDC_OPTIONS_LBL_METERED_DL), enable);
	EnableWindow(GetDlgItem(hWndPropSheet, IDC_OPTIONS_CBO_METERED_DL), enable);
#else /* !ENABLE_NETWORKING */
	// No-network build: Disable *all* controls related to downloads.
	for (uint16_t i = IDC_OPTIONS_GRPDOWNLOADS; i <= IDC_OPTIONS_PALLANGUAGEFORGAMETDB; i++) {
		EnableWindow(GetDlgItem(hWndPropSheet, i), FALSE);
	}
#endif /* ENABLE_NETWORKING */
}

/**
 * Dialog procedure.
 * @param hDlg
 * @param uMsg
 * @param wParam
 * @param lParam
 */
INT_PTR CALLBACK OptionsTabPrivate::dlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
		case WM_INITDIALOG: {
			// Get the pointer to the property sheet page object. This is 
			// contained in the LPARAM of the PROPSHEETPAGE structure.
			LPPROPSHEETPAGE pPage = reinterpret_cast<LPPROPSHEETPAGE>(lParam);
			if (!pPage)
				return TRUE;

			// Get the pointer to the OptionsTabPrivate object.
			OptionsTabPrivate *const d = reinterpret_cast<OptionsTabPrivate*>(pPage->lParam);
			if (!d)
				return TRUE;

			assert(d->hWndPropSheet == nullptr);
			d->hWndPropSheet = hDlg;

			// Store the D object pointer with this particular page dialog.
			SetWindowLongPtr(hDlg, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(d));

			// NOTE: This should be in WM_CREATE, but we don't receive WM_CREATE here.
			DarkMode_InitDialog(hDlg);
			d->lastDarkModeEnabled = g_darkModeEnabled;

			// Populate the combo boxes.
			const tstring s_dl_None = TC_("OptionsTab", "Don't download any images");
			const tstring s_dl_NormalRes = TC_("OptionsTab", "Download normal-resolution images");
			const tstring s_dl_HighRes = TC_("OptionsTab", "Download high-resolution images");
			HWND cboUnmeteredDL = GetDlgItem(hDlg, IDC_OPTIONS_CBO_UNMETERED_DL);
			ComboBox_AddString(cboUnmeteredDL, s_dl_None.c_str());
			ComboBox_AddString(cboUnmeteredDL, s_dl_NormalRes.c_str());
			ComboBox_AddString(cboUnmeteredDL, s_dl_HighRes.c_str());
			HWND cboMeteredDL = GetDlgItem(hDlg, IDC_OPTIONS_CBO_METERED_DL);
			ComboBox_AddString(cboMeteredDL, s_dl_None.c_str());
			ComboBox_AddString(cboMeteredDL, s_dl_NormalRes.c_str());
			ComboBox_AddString(cboMeteredDL, s_dl_HighRes.c_str());

			// Initialize the PAL language dropdown.
			// TODO: "Force PAL" option.
			HWND cboLanguage = GetDlgItem(hDlg, IDC_OPTIONS_PALLANGUAGEFORGAMETDB);
			assert(cboLanguage != nullptr);
			if (cboLanguage) {
				LanguageComboBox_SetForcePAL(cboLanguage, true);
				LanguageComboBox_SetLCs(cboLanguage, Config::get_all_pal_lcs());
			}

			// Set window themes for Win10's dark mode.
			if (g_darkModeSupported) {
				DarkMode_InitButton_Dlg(hDlg, IDC_OPTIONS_GRPDOWNLOADS);
				DarkMode_InitButton_Dlg(hDlg, IDC_OPTIONS_GRPEXTIMGDL);
				DarkMode_InitButton_Dlg(hDlg, IDC_OPTIONS_CHKEXTIMGDL);
				DarkMode_InitComboBox(cboUnmeteredDL);
				DarkMode_InitComboBox(cboMeteredDL);
				DarkMode_InitButton_Dlg(hDlg, IDC_OPTIONS_INTICONSMALL);
				DarkMode_InitButton_Dlg(hDlg, IDC_OPTIONS_STOREFILEORIGININFO);
				DarkMode_InitComboBoxEx(cboLanguage);
				DarkMode_InitButton_Dlg(hDlg, IDC_OPTIONS_GRPOPTIONS);
				DarkMode_InitButton_Dlg(hDlg, IDC_OPTIONS_DANGEROUSPERMISSIONS);
				DarkMode_InitButton_Dlg(hDlg, IDC_OPTIONS_ENABLETHUMBNAILONNETWORKFS);
				DarkMode_InitButton_Dlg(hDlg, IDC_OPTIONS_THUMBNAILDIRECTORYPACKAGES);
				DarkMode_InitButton_Dlg(hDlg, IDC_OPTIONS_SHOWXATTRVIEW);
			}

			// Reset the configuration. 338
			d->reset();
			return TRUE;
		}

		case WM_NOTIFY: {
			auto *const d = reinterpret_cast<OptionsTabPrivate*>(GetWindowLongPtr(hDlg, GWLP_USERDATA));
			if (!d) {
				// No OptionsTabPrivate. Can't do anything...
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
			auto *const d = reinterpret_cast<OptionsTabPrivate*>(GetWindowLongPtr(hDlg, GWLP_USERDATA));
			if (!d) {
				// No OptionsTabPrivate. Can't do anything...
				return FALSE;
			}

			if (HIWORD(wParam) != BN_CLICKED &&
			    HIWORD(wParam) != CBN_SELCHANGE)
			{
				// Unexpected notification.
				break;
			}

			if (wParam == MAKELONG(IDC_OPTIONS_CHKEXTIMGDL, BN_CLICKED)) {
				// chkExtImgDl was clicked.
				d->updateGrpExtImgDl();
			}

			// A checkbox has been adjusted, or a dropdown
			// box has had its selection changed.
			// Page has been modified.
			PropSheet_Changed(GetParent(hDlg), hDlg);
			d->changed = true;
			break;
		}

		case WM_RP_PROP_SHEET_RESET: {
			auto *const d = reinterpret_cast<OptionsTabPrivate*>(GetWindowLongPtr(hDlg, GWLP_USERDATA));
			if (!d) {
				// No OptionsTabPrivate. Can't do anything...
				return FALSE;
			}

			// Reset the tab.
			d->reset();
			break;
		}

		case WM_RP_PROP_SHEET_DEFAULTS: {
			auto *const d = reinterpret_cast<OptionsTabPrivate*>(GetWindowLongPtr(hDlg, GWLP_USERDATA));
			if (!d) {
				// No OptionsTabPrivate. Can't do anything...
				return FALSE;
			}

			// Load the defaults.
			d->loadDefaults();
			break;
		}

		/** Dark Mode **/

		case WM_CTLCOLORDLG:
		case WM_CTLCOLORSTATIC:
			if (g_darkModeEnabled) {
				auto *const d = reinterpret_cast<OptionsTabPrivate*>(GetWindowLongPtr(hDlg, GWLP_USERDATA));
				if (!d) {
					// No OptionsTabPrivate. Can't do anything...
					return FALSE;
				}

				HDC hdc = reinterpret_cast<HDC>(wParam);
				SetTextColor(hdc, g_darkTextColor);
				SetBkColor(hdc, g_darkSubDlgBkColor);
				if (!d->hbrBkgnd) {
					d->hbrBkgnd = CreateSolidBrush(g_darkSubDlgBkColor);
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
				auto *const d = reinterpret_cast<OptionsTabPrivate*>(GetWindowLongPtr(hDlg, GWLP_USERDATA));
				if (!d) {
					// No OptionsTabPrivate. Can't do anything...
					return FALSE;
				}

				UpdateDarkModeEnabled();
				if (d->lastDarkModeEnabled != g_darkModeEnabled) {
					d->lastDarkModeEnabled = g_darkModeEnabled;
					InvalidateRect(hDlg, NULL, true);

					// Propagate WM_THEMECHANGED to window controls that don't
					// automatically handle Dark Mode changes, e.g. ComboBox and Button.
					SendMessage(GetDlgItem(hDlg, IDC_OPTIONS_CBO_UNMETERED_DL), WM_THEMECHANGED, 0, 0);
					SendMessage(GetDlgItem(hDlg, IDC_OPTIONS_CBO_METERED_DL), WM_THEMECHANGED, 0, 0);

					// ComboBoxEx needs extra handling.
					// TODO: Move this to LanguageComboBox's window procedure?
					HWND cboGameTDBPAL = GetDlgItem(hDlg, IDC_OPTIONS_PALLANGUAGEFORGAMETDB);
					assert(cboGameTDBPAL != nullptr);
					SendMessage(cboGameTDBPAL, WM_THEMECHANGED, 0, 0);
					HWND hCombo = reinterpret_cast<HWND>(SendMessage(cboGameTDBPAL, CBEM_GETCOMBOCONTROL, 0, 0));
					SendMessage(hCombo, WM_THEMECHANGED, 0, 0);
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
UINT CALLBACK OptionsTabPrivate::callbackProc(HWND hWnd, UINT uMsg, LPPROPSHEETPAGE ppsp)
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

/** OptionsTab **/

OptionsTab::OptionsTab(void)
	: d_ptr(new OptionsTabPrivate())
{}

OptionsTab::~OptionsTab()
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
HPROPSHEETPAGE OptionsTab::getHPropSheetPage(void)
{
	RP_D(OptionsTab);
	assert(d->hPropSheetPage == nullptr);
	if (d->hPropSheetPage) {
		// Property sheet has already been created.
		return nullptr;
	}

	// tr: Tab title.
	const tstring tsTabTitle = TC_("OptionsTab", "Options");

	PROPSHEETPAGE psp;
	psp.dwSize = sizeof(psp);
	psp.dwFlags = PSP_USECALLBACK | PSP_USETITLE | PSP_DLGINDIRECT;
	psp.hInstance = HINST_THISCOMPONENT;
	psp.pResource = LoadDialog_i18n(HINST_THISCOMPONENT, IDD_CONFIG_OPTIONS);
	psp.pszIcon = nullptr;
	psp.pszTitle = tsTabTitle.c_str();
	psp.pfnDlgProc = OptionsTabPrivate::dlgProc;
	psp.lParam = reinterpret_cast<LPARAM>(d);
	psp.pcRefParent = nullptr;
	psp.pfnCallback = OptionsTabPrivate::callbackProc;

	d->hPropSheetPage = CreatePropertySheetPage(&psp);
	return d->hPropSheetPage;
}

/**
 * Reset the contents of this tab.
 */
void OptionsTab::reset(void)
{
	RP_D(OptionsTab);
	d->reset();
}

/**
 * Load the default configuration.
 * This does NOT save, and will only emit modified()
 * if it's different from the current configuration.
 */
void OptionsTab::loadDefaults(void)
{
	RP_D(OptionsTab);
	d->loadDefaults();
}

/**
 * Save the contents of this tab.
 */
void OptionsTab::save(void)
{
	RP_D(OptionsTab);
	if (d->changed) {
		d->save();
	}
}
