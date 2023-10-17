/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * OptionsTab.cpp: Options tab for rp-config.                              *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
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

// Netowrk status
#include "NetworkStatus.h"

// C++ STL classes
using std::tstring;

class OptionsTabPrivate
{
	public:
		OptionsTabPrivate();

	private:
		RP_DISABLE_COPY(OptionsTabPrivate)

	protected:
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
		static inline const TCHAR *bstCheckedToBoolString(unsigned int value) {
			return (value == BST_CHECKED ? _T("true") : _T("false"));
		}

		/**
		 * Convert BST_CHECKED or BST_UNCHECKED to a bool.
		 * @param value BST_CHECKED or BST_UNCHECKED.
		 * @return bool.
		 */
		static inline bool bstCheckedToBool(unsigned int value) {
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

		// PAL language codes for GameTDB.
		static const uint32_t pal_lc[];
};

/** OptionsTabPrivate **/

// PAL language codes for GameTDB.
// NOTE: 'au' is technically not a language code, but
// GameTDB handles it as a separate language.
// TODO: Combine with the KDE version.
// NOTE: Win32 LanguageComboBox uses a NULL-terminated pal_lc[] array.
const uint32_t OptionsTabPrivate::pal_lc[] = {'au', 'de', 'en', 'es', 'fr', 'it', 'nl', 'pt', 'ru', 0};

OptionsTabPrivate::OptionsTabPrivate()
	: hPropSheetPage(nullptr)
	, hWndPropSheet(nullptr)
	, changed(false)
{ }

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
	CheckDlgButton(hWndPropSheet, IDC_OPTIONS_CHKEXTIMGDL, boolToBstChecked(config->extImgDownloadEnabled()));
	CheckDlgButton(hWndPropSheet, IDC_OPTIONS_INTICONSMALL, boolToBstChecked(config->useIntIconForSmallSizes()));
	CheckDlgButton(hWndPropSheet, IDC_OPTIONS_STOREFILEORIGININFO, boolToBstChecked(config->storeFileOriginInfo()));

	// Image bandwidth options
	ComboBox_SetCurSel(GetDlgItem(hWndPropSheet, IDC_OPTIONS_CBO_UNMETERED_DL), static_cast<int>(config->imgBandwidthUnmetered()));
	ComboBox_SetCurSel(GetDlgItem(hWndPropSheet, IDC_OPTIONS_CBO_METERED_DL), static_cast<int>(config->imgBandwidthMetered()));
	// Update sensitivity
	updateGrpExtImgDl();

	// Options
	// FIXME: Uncomment this once the "dangerous" permissions overlay
	// is working on Windows.
	/*
	CheckDlgButton(hWndPropSheet, IDC_OPTIONS_DANGEROUSPERMISSIONS,
		boolToBstChecked(config->showDangerousPermissionsOverlayIcon()));
	*/
	CheckDlgButton(hWndPropSheet, IDC_OPTIONS_ENABLETHUMBNAILONNETWORKFS,
		boolToBstChecked(config->enableThumbnailOnNetworkFS()));

	// FIXME: Remove this once the "dangerous" permissions overlay
	// is working on Windows.
	CheckDlgButton(hWndPropSheet, IDC_OPTIONS_DANGEROUSPERMISSIONS, BST_UNCHECKED);
	EnableWindow(GetDlgItem(hWndPropSheet, IDC_OPTIONS_DANGEROUSPERMISSIONS), FALSE);

	// PAL language code
	HWND cboGameTDBPAL = GetDlgItem(hWndPropSheet, IDC_OPTIONS_PALLANGUAGEFORGAMETDB);
	LanguageComboBox_SetSelectedLC(cboGameTDBPAL, config->palLanguageForGameTDB());

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
	bool bdef = Config::extImgDownloadEnabled_default();
	if (bcur != bdef) {
		CheckDlgButton(hWndPropSheet, IDC_OPTIONS_CHKEXTIMGDL, boolToBstChecked(bdef));
		isDefChanged = true;
		// Update sensitivity
		updateGrpExtImgDl();
	}
	bcur = bstCheckedToBool(IsDlgButtonChecked(hWndPropSheet, IDC_OPTIONS_INTICONSMALL));
	bdef = Config::useIntIconForSmallSizes_default();
	if (bcur != bdef) {
		CheckDlgButton(hWndPropSheet, IDC_OPTIONS_INTICONSMALL, boolToBstChecked(bdef));
		isDefChanged = true;
	}
	bcur = bstCheckedToBool(IsDlgButtonChecked(hWndPropSheet, IDC_OPTIONS_STOREFILEORIGININFO));
	bdef = Config::storeFileOriginInfo_default();
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
	// FIXME: Uncomment this once the "dangerous" permissions overlay
	// is working on Windows.
	/*
	bcur = bstCheckedToBool(IsDlgButtonChecked(hWndPropSheet, IDC_OPTIONS_DANGEROUSPERMISSIONS));
	bdef = Config::showDangerousPermissionsOverlayIcon_default();
	if (bcur != bdef) {
		CheckDlgButton(hWndPropSheet, IDC_OPTIONS_DANGEROUSPERMISSIONS, boolToBstChecked(bdef));
		isDefChanged = true;
	}
	*/
	bcur = bstCheckedToBool(IsDlgButtonChecked(hWndPropSheet, IDC_OPTIONS_ENABLETHUMBNAILONNETWORKFS));
	bdef = Config::enableThumbnailOnNetworkFS_default();
	if (bcur != bdef) {
		CheckDlgButton(hWndPropSheet, IDC_OPTIONS_ENABLETHUMBNAILONNETWORKFS, boolToBstChecked(bdef));
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
	// TODO: Consolidate this.
	const TCHAR *sUnmetered, *sMetered;
	switch (static_cast<Config::ImgBandwidth>(ComboBox_GetCurSel(GetDlgItem(hWndPropSheet, IDC_OPTIONS_CBO_UNMETERED_DL)))) {
		case Config::ImgBandwidth::None:
			sUnmetered = _T("None");
			break;
		case Config::ImgBandwidth::NormalRes:
			sUnmetered = _T("NormalRes");
			break;
		case Config::ImgBandwidth::HighRes:
		default:
			sUnmetered = _T("HighRes");
			break;
	}
	switch (static_cast<Config::ImgBandwidth>(ComboBox_GetCurSel(GetDlgItem(hWndPropSheet, IDC_OPTIONS_CBO_METERED_DL)))) {
		case Config::ImgBandwidth::None:
			sMetered = _T("None");
			break;
		case Config::ImgBandwidth::NormalRes:
		default:
			sMetered = _T("NormalRes");
			break;
		case Config::ImgBandwidth::HighRes:
			sMetered = _T("HighRes");
			break;
	}
	WritePrivateProfileString(_T("Downloads"), _T("ImgBandwidthUnmetered"), sUnmetered, tfilename.c_str());
	WritePrivateProfileString(_T("Downloads"), _T("ImgBandwidthMetered"), sMetered, tfilename.c_str());
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

	// No longer changed.
	changed = false;
}

/**
 * Update grpExtImgDl's control sensitivity.
 */
void OptionsTabPrivate::updateGrpExtImgDl(void)
{
	// The dropdowns will be enabled if:
	// - chkExtImgDl is enabled
	// - Metered/unmetered detection is available
	bool enable = bstCheckedToBool(IsDlgButtonChecked(hWndPropSheet, IDC_OPTIONS_CHKEXTIMGDL));
	if (!rp_win32_can_identify_if_metered()) {
		enable = false;
	}

	EnableWindow(GetDlgItem(hWndPropSheet, IDC_OPTIONS_LBL_UNMETERED_DL), enable);
	EnableWindow(GetDlgItem(hWndPropSheet, IDC_OPTIONS_CBO_UNMETERED_DL), enable);
	EnableWindow(GetDlgItem(hWndPropSheet, IDC_OPTIONS_LBL_METERED_DL), enable);
	EnableWindow(GetDlgItem(hWndPropSheet, IDC_OPTIONS_CBO_METERED_DL), enable);
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

			// Populate the combo boxes.
			const tstring s_dl_None = U82T_c(C_("OptionsTab", "Don't download any images"));
			const tstring s_dl_NormalRes = U82T_c(C_("OptionsTab", "Download normal-resolution images"));
			const tstring s_dl_HighRes = U82T_c(C_("OptionsTab", "Download high-resolution images"));
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
				LanguageComboBox_SetLCs(cboLanguage, pal_lc);
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
{ }

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
	const tstring tsTabTitle = U82T_c(C_("OptionsTab", "Options"));

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
