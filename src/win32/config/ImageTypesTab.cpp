/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * ImageTypesTab.cpp: Image type priorities tab. (Part of ConfigDialog.)   *
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
#include "WinUI.hpp"
#include "resource.h"

#include "libromdata/RpWin32.hpp"
#include "libromdata/config/Config.hpp"
#include "libromdata/RomData.hpp"
using LibRomData::Config;
using LibRomData::RomData;

// C++ includes.
#include <string>
using std::wstring;

// Image type names.
const rp_char *const ConfigDialogPrivate::ImageTypes_imgTypeNames[RomData::IMG_EXT_MAX+1] = {
	_RP("Internal\nIcon"),
	_RP("Internal\nBanner"),
	_RP("Internal\nMedia"),
	_RP("External\nMedia"),
	_RP("External\nCover"),
	_RP("External\n3D Cover"),
	_RP("External\nFull Cover"),
	_RP("External\nBox"),
};

// System names.
const rp_char *const ConfigDialogPrivate::ImageTypes_sysNames[SYS_COUNT] = {
	_RP("amiibo"),
	_RP("Dreamcast Saves"),
	_RP("GameCube / Wii"),
	_RP("GameCube Saves"),
	_RP("Nintendo DS(i)"),
	_RP("Nintendo 3DS"),
	_RP("PlayStation Saves"),
	_RP("Wii U"),
};

/**
 * Create the grid of static text and combo boxes.
 * @param hDlg Image type priorities property sheet.
 */
void ConfigDialogPrivate::ImageTypes_createGrid(HWND hDlg)
{
	// Get the dialog margin.
	// 7x7 DLU margin is recommended by the Windows UX guidelines.
	// Reference: http://stackoverflow.com/questions/2118603/default-dialog-padding
	RECT dlgMargin = {7, 7, 8, 8};
	MapDialogRect(hDlg, &dlgMargin);

	// Get the font of the parent dialog.
	HFONT hFontDlg = GetWindowFont(GetParent(hDlg));
	assert(hFontDlg != nullptr);
	if (!hFontDlg) {
		// No font?!
		return;
	}

	// Get the dimensions of IDC_IMAGETYPES_DESC2.
	HWND lblDesc2 = GetDlgItem(hDlg, IDC_IMAGETYPES_DESC2);
	assert(lblDesc2 != nullptr);
	if (!lblDesc2) {
		// Label is missing...
		return;
	}
	RECT rect_lblDesc2;
	GetWindowRect(lblDesc2, &rect_lblDesc2);
	MapWindowPoints(HWND_DESKTOP, GetParent(lblDesc2), (LPPOINT)&rect_lblDesc2, 2);

	// Determine the size of the largest image type label.
	SIZE sz_lblImageType = {0, 0};
	for (int i = RomData::IMG_EXT_MAX; i >= 0; i--) {
		SIZE szCur;
		WinUI::measureTextSize(hDlg, hFontDlg, RP2W_c(ImageTypes_imgTypeNames[i]), &szCur);
		if (szCur.cx > sz_lblImageType.cx) {
			sz_lblImageType.cx = szCur.cx;
		}
		if (szCur.cy > sz_lblImageType.cy) {
			sz_lblImageType.cy = szCur.cy;
		}
	}

	// Determine the size of the largest system name label.
	// TODO: Height needs to match the combo boxes.
	SIZE sz_lblSysName = {0, 0};
	for (int i = SYS_COUNT-1; i >= 0; i--) {
		SIZE szCur;
		WinUI::measureTextSize(hDlg, hFontDlg, RP2W_c(ImageTypes_sysNames[i]), &szCur);
		if (szCur.cx > sz_lblSysName.cx) {
			sz_lblSysName.cx = szCur.cx;
		}
		if (szCur.cy > sz_lblSysName.cy) {
			sz_lblSysName.cy = szCur.cy;
		}
	}

	// Create the image type labels.
	POINT curPt = {rect_lblDesc2.left + sz_lblSysName.cx + dlgMargin.right,
		rect_lblDesc2.bottom + dlgMargin.bottom};
	for (unsigned int i = 0; i <= RomData::IMG_EXT_MAX; i++) {
		HWND hStatic = CreateWindowEx(WS_EX_NOPARENTNOTIFY | WS_EX_TRANSPARENT,
			WC_STATIC, RP2W_c(ImageTypes_imgTypeNames[i]),
			WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | SS_CENTER,
			curPt.x, curPt.y, sz_lblImageType.cx, sz_lblImageType.cy,
			hDlg, (HMENU)IDC_STATIC, nullptr, nullptr);
		SetWindowFont(hStatic, hFontDlg, FALSE);
		curPt.x += sz_lblImageType.cx;
	}

	// Create the system name labels.
	curPt.x = rect_lblDesc2.left;
	curPt.y += sz_lblImageType.cy;
	for (unsigned int i = 0; i < SYS_COUNT; i++) {
		HWND hStatic = CreateWindowEx(WS_EX_NOPARENTNOTIFY | WS_EX_TRANSPARENT,
			WC_STATIC, RP2W_c(ImageTypes_sysNames[i]),
			WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | SS_LEFT,
			curPt.x, curPt.y, sz_lblSysName.cx, sz_lblSysName.cy,
			hDlg, (HMENU)IDC_STATIC, nullptr, nullptr);
		SetWindowFont(hStatic, hFontDlg, FALSE);
		curPt.y += sz_lblSysName.cy;
	}
}

/**
 * Reset the image type priorities tab to the current configuration.
 * @param hDlg Image type priorities property sheet.
 */
void ConfigDialogPrivate::ImageTypes_reset(HWND hDlg)
{
	// TODO

	// No longer changed.
	changed_ImageTypes = false;
}

/**
 * Save the image type priorities tab configuration.
 * @param hDlg Image type priorities property sheet.
 */
void ConfigDialogPrivate::ImageTypes_save(HWND hDlg)
{
	const rp_char *filename = config->filename();
	if (!filename) {
		// No configuration filename...
		return;
	}

	// TODO

	// No longer changed.
	changed_ImageTypes = false;
}

/**
 * DlgProc for IDD_CONFIG_IMAGETYPES.
 * @param hDlg
 * @param uMsg
 * @param wParam
 * @param lParam
 */
INT_PTR CALLBACK ConfigDialogPrivate::ImageTypes_DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
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
			SetProp(hDlg, D_PTR_PROP, reinterpret_cast<HANDLE>(d));

			// Create the control grid.
			d->ImageTypes_createGrid(hDlg);

			// Reset the values.
			d->ImageTypes_reset(hDlg);
			return TRUE;
		}

		case WM_DESTROY: {
			// Remove the D_PTR_PROP property from the page.
			// The D_PTR_PROP property stored the pointer to the
			// ConfigDialogPrivate object.
			RemoveProp(hDlg, D_PTR_PROP);
			return TRUE;
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
					if (d->changed_ImageTypes) {
						d->ImageTypes_save(hDlg);
					}
					break;

				case NM_CLICK:
				case NM_RETURN:
					// SysLink control notification.
					if (lppsn->hdr.hwndFrom == GetDlgItem(hDlg, IDC_IMAGETYPES_CREDITS)) {
						// Open the URL.
						PNMLINK pNMLink = reinterpret_cast<PNMLINK>(lParam);
						ShellExecute(nullptr, L"open", pNMLink->item.szUrl, nullptr, nullptr, SW_SHOW);
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
 * Property sheet callback function for IDD_CONFIG_IMAGETYPES.
 * @param hDlg
 * @param uMsg
 * @param wParam
 * @param lParam
 */
UINT CALLBACK ConfigDialogPrivate::ImageTypes_CallbackProc(HWND hWnd, UINT uMsg, LPPROPSHEETPAGE ppsp)
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
