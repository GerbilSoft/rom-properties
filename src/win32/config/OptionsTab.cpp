/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * OptionsTab.cpp: Options tab for rp-config.                              *
 *                                                                         *
 * Copyright (c) 2016-2020 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "OptionsTab.hpp"
#include "res/resource.h"

// librpbase
using LibRpBase::Config;

// C++ STL classes.
using std::tstring;

class OptionsTabPrivate
{
	public:
		OptionsTabPrivate();

	private:
		RP_DISABLE_COPY(OptionsTabPrivate)

	public:
		// Property for "D pointer".
		// This points to the OptionsTabPrivate object.
		static const TCHAR D_PTR_PROP[];

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
};

/** OptionsTabPrivate **/

OptionsTabPrivate::OptionsTabPrivate()
	: hPropSheetPage(nullptr)
	, hWndPropSheet(nullptr)
	, changed(false)
{ }

// Property for "D pointer".
// This points to the OptionsTabPrivate object.
const TCHAR OptionsTabPrivate::D_PTR_PROP[] = _T("OptionsTabPrivate");

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

	CheckDlgButton(hWndPropSheet, IDC_EXTIMGDL, boolToBstChecked(config->extImgDownloadEnabled()));
	CheckDlgButton(hWndPropSheet, IDC_INTICONSMALL, boolToBstChecked(config->useIntIconForSmallSizes()));
	CheckDlgButton(hWndPropSheet, IDC_HIGHRESDL, boolToBstChecked(config->downloadHighResScans()));
	CheckDlgButton(hWndPropSheet, IDC_STOREFILEORIGININFO, boolToBstChecked(config->storeFileOriginInfo()));
	CheckDlgButton(hWndPropSheet, IDC_DANGEROUSPERMISSIONS,
		boolToBstChecked(config->showDangerousPermissionsOverlayIcon()));
	CheckDlgButton(hWndPropSheet, IDC_ENABLETHUMBNAILONNETWORKFS,
		boolToBstChecked(config->enableThumbnailOnNetworkFS()));

	// No longer changed.
	changed = false;
}

void OptionsTabPrivate::loadDefaults(void)
{
	assert(hWndPropSheet != nullptr);
	if (!hWndPropSheet)
		return;

	// TODO: Get the defaults from Config.
	// For now, hard-coding everything here.
	static const bool extImgDownloadEnabled_default = true;
	static const bool useIntIconForSmallSizes_default = true;
	static const bool downloadHighResScans_default = true;
	static const bool storeFileOriginInfo_default = true;
	static const bool showDangerousPermissionsOverlayIcon_default = true;
	static const bool enableThumbnailOnNetworkFS_default = false;
	bool isDefChanged = false;

	bool cur = bstCheckedToBool(IsDlgButtonChecked(hWndPropSheet, IDC_EXTIMGDL));
	if (cur != extImgDownloadEnabled_default) {
		CheckDlgButton(hWndPropSheet, IDC_EXTIMGDL, boolToBstChecked(extImgDownloadEnabled_default));
		isDefChanged = true;
	}
	cur = bstCheckedToBool(IsDlgButtonChecked(hWndPropSheet, IDC_INTICONSMALL));
	if (cur != useIntIconForSmallSizes_default) {
		CheckDlgButton(hWndPropSheet, IDC_INTICONSMALL, boolToBstChecked(useIntIconForSmallSizes_default));
		isDefChanged = true;
	}
	cur = bstCheckedToBool(IsDlgButtonChecked(hWndPropSheet, IDC_HIGHRESDL));
	if (cur != downloadHighResScans_default) {
		CheckDlgButton(hWndPropSheet, IDC_HIGHRESDL, boolToBstChecked(downloadHighResScans_default));
		isDefChanged = true;
	}
	cur = bstCheckedToBool(IsDlgButtonChecked(hWndPropSheet, IDC_STOREFILEORIGININFO));
	if (cur != storeFileOriginInfo_default) {
		CheckDlgButton(hWndPropSheet, IDC_STOREFILEORIGININFO,
			boolToBstChecked(storeFileOriginInfo_default));
		isDefChanged = true;
	}
	cur = bstCheckedToBool(IsDlgButtonChecked(hWndPropSheet, IDC_DANGEROUSPERMISSIONS));
	if (cur != downloadHighResScans_default) {
		CheckDlgButton(hWndPropSheet, IDC_DANGEROUSPERMISSIONS,
			boolToBstChecked(showDangerousPermissionsOverlayIcon_default));
		isDefChanged = true;
	}
	cur = bstCheckedToBool(IsDlgButtonChecked(hWndPropSheet, IDC_ENABLETHUMBNAILONNETWORKFS));
	if (cur != enableThumbnailOnNetworkFS_default) {
		CheckDlgButton(hWndPropSheet, IDC_ENABLETHUMBNAILONNETWORKFS,
			boolToBstChecked(enableThumbnailOnNetworkFS_default));
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

	const TCHAR *btstr = bstCheckedToBoolString(IsDlgButtonChecked(hWndPropSheet, IDC_EXTIMGDL));
	WritePrivateProfileString(_T("Downloads"), _T("ExtImageDownload"), btstr, tfilename.c_str());

	btstr = bstCheckedToBoolString(IsDlgButtonChecked(hWndPropSheet, IDC_INTICONSMALL));
	WritePrivateProfileString(_T("Downloads"), _T("UseIntIconForSmallSizes"), btstr, tfilename.c_str());

	btstr = bstCheckedToBoolString(IsDlgButtonChecked(hWndPropSheet, IDC_HIGHRESDL));
	WritePrivateProfileString(_T("Downloads"), _T("DownloadHighResScans"), btstr, tfilename.c_str());

	btstr = bstCheckedToBoolString(IsDlgButtonChecked(hWndPropSheet, IDC_STOREFILEORIGININFO));
	WritePrivateProfileString(_T("Downloads"), _T("StoreFileOriginInfo"), btstr, tfilename.c_str());

	btstr = bstCheckedToBoolString(IsDlgButtonChecked(hWndPropSheet, IDC_DANGEROUSPERMISSIONS));
	WritePrivateProfileString(_T("Options"), _T("ShowDangerousPermissionsOverlayIcon"), btstr, tfilename.c_str());

	btstr = bstCheckedToBoolString(IsDlgButtonChecked(hWndPropSheet, IDC_ENABLETHUMBNAILONNETWORKFS));
	WritePrivateProfileString(_T("Options"), _T("EnableThumbnailOnNetworkFS"), btstr, tfilename.c_str());

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
			SetProp(hDlg, D_PTR_PROP, reinterpret_cast<HANDLE>(d));

			// Reset the configuration.
			d->reset();
			return TRUE;
		}

		case WM_DESTROY: {
			// Remove the D_PTR_PROP property from the page. 
			// The D_PTR_PROP property stored the pointer to the 
			// OptionsTabPrivate object.
			RemoveProp(hDlg, D_PTR_PROP);
			return TRUE;
		}

		case WM_NOTIFY: {
			OptionsTabPrivate *const d = static_cast<OptionsTabPrivate*>(
				GetProp(hDlg, D_PTR_PROP));
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
			OptionsTabPrivate *const d = static_cast<OptionsTabPrivate*>(
				GetProp(hDlg, D_PTR_PROP));
			if (!d) {
				// No OptionsTabPrivate. Can't do anything...
				return FALSE;
			}

			if (HIWORD(wParam) != BN_CLICKED)
				break;

			// A checkbox has been adjusted.
			// Page has been modified.
			PropSheet_Changed(GetParent(hDlg), hDlg);
			d->changed = true;
			break;
		}

		case WM_RP_PROP_SHEET_RESET: {
			OptionsTabPrivate *const d = static_cast<OptionsTabPrivate*>(
				GetProp(hDlg, D_PTR_PROP));
			if (!d) {
				// No OptionsTabPrivate. Can't do anything...
				return FALSE;
			}

			// Reset the tab.
			d->reset();
			break;
		}

		case WM_RP_PROP_SHEET_DEFAULTS: {
			OptionsTabPrivate *const d = static_cast<OptionsTabPrivate*>(
				GetProp(hDlg, D_PTR_PROP));
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
	psp.pResource = LoadDialog_i18n(IDD_CONFIG_OPTIONS);
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
