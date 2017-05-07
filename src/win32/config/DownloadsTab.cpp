/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * DownloadsTab.cpp: Downloads tab. (Part of ConfigDialog.)                *
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

#include "ConfigDialog_p.hpp"
#include "resource.h"

#include "libromdata/RpWin32.hpp"
#include "libromdata/config/Config.hpp"
using LibRomData::Config;

/**
 * Reset the Downloads tab to the current configuration.
 * @param hDlg Downloads property sheet.
 */
void ConfigDialogPrivate::reset_Downloads(HWND hDlg)
{
	HWND hWnd = GetDlgItem(hDlg, IDC_EXTIMGDL);
	if (hWnd) {
		Button_SetCheck(hWnd, boolToBstChecked(config->extImgDownloadEnabled()));
	}
	hWnd = GetDlgItem(hDlg, IDC_INTICONSMALL);
	if (hWnd) {
		Button_SetCheck(hWnd, boolToBstChecked(config->useIntIconForSmallSizes()));
	}
	hWnd = GetDlgItem(hDlg, IDC_HIGHRESDL);
	if (hWnd) {
		Button_SetCheck(hWnd, boolToBstChecked(config->downloadHighResScans()));
	}

	// No longer changed.
	changed_Downloads = false;
}

/**
 * Save the Downloads tab configuration.
 * @param hDlg Downloads property sheet.
 */
void ConfigDialogPrivate::save_Downloads(HWND hDlg)
{
	const rp_char *filename = config->filename();
	if (!filename) {
		// No configuration filename...
		return;
	}

	HWND hWnd = GetDlgItem(hDlg, IDC_EXTIMGDL);
	if (hWnd) {
		const wchar_t *bstr = bstCheckedToBoolString(Button_GetCheck(hWnd));
		WritePrivateProfileString(L"Downloads", L"ExtImageDownload", bstr, RP2W_c(filename));
	}
	hWnd = GetDlgItem(hDlg, IDC_INTICONSMALL);
	if (hWnd) {
		const wchar_t *bstr = bstCheckedToBoolString(Button_GetCheck(hWnd));
		WritePrivateProfileString(L"Downloads", L"UseIntIconForSmallSizes", bstr, RP2W_c(filename));
	}
	hWnd = GetDlgItem(hDlg, IDC_HIGHRESDL);
	if (hWnd) {
		const wchar_t *bstr = bstCheckedToBoolString(Button_GetCheck(hWnd));
		WritePrivateProfileString(L"Downloads", L"DownloadHighResScans", bstr, RP2W_c(filename));
	}

	// No longer changed.
	changed_Downloads = false;
}

/**
 * DlgProc for IDD_CONFIG_DOWNLOADS.
 * @param hDlg
 * @param uMsg
 * @param wParam
 * @param lParam
 */
INT_PTR CALLBACK ConfigDialogPrivate::DlgProc_Downloads(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
		case WM_INITDIALOG: {
			// Get the pointer to the property sheet page object. This is 
			// contained in the LPARAM of the PROPSHEETPAGE structure.
			LPPROPSHEETPAGE pPage = reinterpret_cast<LPPROPSHEETPAGE>(lParam);
			if (!pPage)
				return TRUE;

			// Get the pointer to the ConfigDialogPrivate object.
			ConfigDialogPrivate *const d = reinterpret_cast<ConfigDialogPrivate*>(pPage->lParam);
			if (!d)
				return TRUE;

			// Store the D object pointer with this particular page dialog.
			SetProp(hDlg,D_PTR_PROP, reinterpret_cast<HANDLE>(d));

			// Reset the values.
			d->reset_Downloads(hDlg);
			return TRUE;
		}

		case WM_DESTROY: {
			// Remove the D_PTR_PROP property from the page. 
			// The D_PTR_PROP property stored the pointer to the 
			// ConfigDialogPrivate object.
			RemoveProp(hDlg, D_PTR_PROP);
			return TRUE;
		}

		case WM_COMMAND: {
			ConfigDialogPrivate *d = static_cast<ConfigDialogPrivate*>(
				GetProp(hDlg, D_PTR_PROP));
			if (!d) {
				// No ConfigDialogPrivate. Can't do anything...
				return FALSE;
			}

			// A checkbox has been adjusted.
			// Page has been modified.
			PropSheet_Changed(GetParent(hDlg), hDlg);
			d->changed_Downloads = true;
			break;
		}

		case WM_NOTIFY: {
			ConfigDialogPrivate *d = static_cast<ConfigDialogPrivate*>(
				GetProp(hDlg, D_PTR_PROP));
			if (!d) {
				// No ConfigDialogPrivate. Can't do anything...
				return FALSE;
			}

			LPPSHNOTIFY lppsn = reinterpret_cast<LPPSHNOTIFY>(lParam);
			switch (lppsn->hdr.code) {
				case PSN_APPLY:
					// Save settings.
					if (d->changed_Downloads) {
						d->save_Downloads(hDlg);
					}
					break;
				default:
					break;
			}
			break;
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
UINT CALLBACK ConfigDialogPrivate::CallbackProc_Downloads(HWND hWnd, UINT uMsg, LPPROPSHEETPAGE ppsp)
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
