/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * DownloadsTab.cpp: Downloads tab for rp-config.                          *
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
#include "DownloadsTab.hpp"
#include "resource.h"

// librpbase
#include "librpbase/TextFuncs.hpp"
#include "librpbase/config/Config.hpp"
using LibRpBase::Config;

// libi18n
#include "libi18n/i18n.h"

// C includes. (C++ namespace)
#include <cassert>

// C++ includes.
#include <string>
using std::wstring;

class DownloadsTabPrivate
{
	public:
		DownloadsTabPrivate();

	private:
		RP_DISABLE_COPY(DownloadsTabPrivate)

	public:
		// Property for "D pointer".
		// This points to the DownloadsTabPrivate object.
		static const wchar_t D_PTR_PROP[];

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
		static inline const wchar_t *bstCheckedToBoolString(unsigned int value) {
			return (value == BST_CHECKED ? L"true" : L"false");
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

/** DownloadsTabPrivate **/

DownloadsTabPrivate::DownloadsTabPrivate()
	: hPropSheetPage(nullptr)
	, hWndPropSheet(nullptr)
	, changed(false)
{ }

// Property for "D pointer".
// This points to the DownloadsTabPrivate object.
const wchar_t DownloadsTabPrivate::D_PTR_PROP[] = L"DownloadsTabPrivate";

/**
 * Reset the configuration.
 */
void DownloadsTabPrivate::reset(void)
{
	assert(hWndPropSheet != nullptr);
	if (!hWndPropSheet)
		return;

	// NOTE: This may re-check the configuration timestamp.
	const Config *const config = Config::instance();

	CheckDlgButton(hWndPropSheet, IDC_EXTIMGDL, boolToBstChecked(config->extImgDownloadEnabled()));
	CheckDlgButton(hWndPropSheet, IDC_INTICONSMALL, boolToBstChecked(config->useIntIconForSmallSizes()));
	CheckDlgButton(hWndPropSheet, IDC_HIGHRESDL, boolToBstChecked(config->downloadHighResScans()));

	// No longer changed.
	changed = false;
}

void DownloadsTabPrivate::loadDefaults(void)
{
	assert(hWndPropSheet != nullptr);
	if (!hWndPropSheet)
		return;

	// TODO: Get the defaults from Config.
	// For now, hard-coding everything here.
	static const bool extImgDownloadEnabled_default = true;
	static const bool useIntIconForSmallSizes_default = true;
	static const bool downloadHighResScans_default = true;
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

	if (isDefChanged) {
		this->changed = true;
		PropSheet_Changed(GetParent(hWndPropSheet), hWndPropSheet);
	}
}

/**
 * Save the configuration.
 */
void DownloadsTabPrivate::save(void)
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

	const wchar_t *bstr = bstCheckedToBoolString(IsDlgButtonChecked(hWndPropSheet, IDC_EXTIMGDL));
	WritePrivateProfileString(L"Downloads", L"ExtImageDownload", bstr, RP2W_c(filename));

	bstr = bstCheckedToBoolString(IsDlgButtonChecked(hWndPropSheet, IDC_INTICONSMALL));
	WritePrivateProfileString(L"Downloads", L"UseIntIconForSmallSizes", bstr, RP2W_c(filename));

	bstr = bstCheckedToBoolString(IsDlgButtonChecked(hWndPropSheet, IDC_HIGHRESDL));
	WritePrivateProfileString(L"Downloads", L"DownloadHighResScans", bstr, RP2W_c(filename));

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
INT_PTR CALLBACK DownloadsTabPrivate::dlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
		case WM_INITDIALOG: {
			// Get the pointer to the property sheet page object. This is 
			// contained in the LPARAM of the PROPSHEETPAGE structure.
			LPPROPSHEETPAGE pPage = reinterpret_cast<LPPROPSHEETPAGE>(lParam);
			if (!pPage)
				return TRUE;

			// Get the pointer to the DownloadsTabPrivate object.
			DownloadsTabPrivate *const d = reinterpret_cast<DownloadsTabPrivate*>(pPage->lParam);
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
			// DownloadsTabPrivate object.
			RemoveProp(hDlg, D_PTR_PROP);
			return TRUE;
		}

		case WM_NOTIFY: {
			DownloadsTabPrivate *const d = static_cast<DownloadsTabPrivate*>(
				GetProp(hDlg, D_PTR_PROP));
			if (!d) {
				// No DownloadsTabPrivate. Can't do anything...
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
			DownloadsTabPrivate *const d = static_cast<DownloadsTabPrivate*>(
				GetProp(hDlg, D_PTR_PROP));
			if (!d) {
				// No DownloadsTabPrivate. Can't do anything...
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
			DownloadsTabPrivate *const d = static_cast<DownloadsTabPrivate*>(
				GetProp(hDlg, D_PTR_PROP));
			if (!d) {
				// No DownloadsTabPrivate. Can't do anything...
				return FALSE;
			}

			// Reset the tab.
			d->reset();
			break;
		}

		case WM_RP_PROP_SHEET_DEFAULTS: {
			DownloadsTabPrivate *const d = static_cast<DownloadsTabPrivate*>(
				GetProp(hDlg, D_PTR_PROP));
			if (!d) {
				// No DownloadsTabPrivate. Can't do anything...
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
UINT CALLBACK DownloadsTabPrivate::callbackProc(HWND hWnd, UINT uMsg, LPPROPSHEETPAGE ppsp)
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

/** DownloadsTab **/

DownloadsTab::DownloadsTab(void)
	: d_ptr(new DownloadsTabPrivate())
{ }

DownloadsTab::~DownloadsTab()
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
HPROPSHEETPAGE DownloadsTab::getHPropSheetPage(void)
{
	RP_D(DownloadsTab);
	assert(d->hPropSheetPage == nullptr);
	if (d->hPropSheetPage) {
		// Property sheet has already been created.
		return nullptr;
	}

	// tr: Tab title.
	const wstring wsTabTitle = RP2W_c(C_("DownloadsTab", "Downloads"));

	PROPSHEETPAGE psp;
	psp.dwSize = sizeof(psp);	
	psp.dwFlags = PSP_USECALLBACK | PSP_USETITLE;
	psp.hInstance = HINST_THISCOMPONENT;
	psp.pszTemplate = MAKEINTRESOURCE(IDD_CONFIG_DOWNLOADS);
	psp.pszIcon = nullptr;
	psp.pszTitle = wsTabTitle.c_str();
	psp.pfnDlgProc = DownloadsTabPrivate::dlgProc;
	psp.lParam = reinterpret_cast<LPARAM>(d);
	psp.pcRefParent = nullptr;
	psp.pfnCallback = DownloadsTabPrivate::callbackProc;

	d->hPropSheetPage = CreatePropertySheetPage(&psp);
	return d->hPropSheetPage;
}

/**
 * Reset the contents of this tab.
 */
void DownloadsTab::reset(void)
{
	RP_D(DownloadsTab);
	d->reset();
}

/**
 * Load the default configuration.
 * This does NOT save, and will only emit modified()
 * if it's different from the current configuration.
 */
void DownloadsTab::loadDefaults(void)
{
	RP_D(DownloadsTab);
	d->loadDefaults();
}

/**
 * Save the contents of this tab.
 */
void DownloadsTab::save(void)
{
	RP_D(DownloadsTab);
	if (d->changed) {
		d->save();
	}
}
