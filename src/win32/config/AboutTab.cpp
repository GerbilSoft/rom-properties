/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * AboutTab.cpp: About tab for rp-config.                                  *
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
#include "AboutTab.hpp"
#include "resource.h"

// librpbase
#include "librpbase/TextFuncs.hpp"
#include "librpbase/config/AboutTabText.hpp"
using LibRpBase::AboutTabText;

// Property sheet icon.
// Extracted from imageres.dll or shell32.dll.
#include "PropSheetIcon.hpp"

// C includes. (C++ namespace)
#include <cassert>

// C++ includes.
#include <string>
using std::wstring;

class AboutTabPrivate
{
	public:
		AboutTabPrivate();
		~AboutTabPrivate();

	private:
		RP_DISABLE_COPY(AboutTabPrivate)

	public:
		// Property for "D pointer".
		// This points to the AboutTabPrivate object.
		static const wchar_t D_PTR_PROP[];

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

		// Bold font.
		HFONT hFontBold;

		/**
		 * Initialize the bold font.
		 * @param hFont Base font.
		 */
		void initBoldFont(HFONT hFont);

	protected:
		/**
		 * Initialize the program title text.
		 */
		void initProgramTitleText(void);

	public:
		/**
		 * Initialize the dialog.
		 */
		void init(void);
};

/** AboutTabPrivate **/

AboutTabPrivate::AboutTabPrivate()
	: hPropSheetPage(nullptr)
	, hWndPropSheet(nullptr)
	, hFontBold(nullptr)
{ }

AboutTabPrivate::~AboutTabPrivate()
{
	if (hFontBold) {
		DeleteFont(hFontBold);
	}
}

// Property for "D pointer".
// This points to the AboutTabPrivate object.
const wchar_t AboutTabPrivate::D_PTR_PROP[] = L"AboutTabPrivate";

/**
 * Dialog procedure.
 * @param hDlg
 * @param uMsg
 * @param wParam
 * @param lParam
 */
INT_PTR CALLBACK AboutTabPrivate::dlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
		case WM_INITDIALOG: {
			// Get the pointer to the property sheet page object. This is 
			// contained in the LPARAM of the PROPSHEETPAGE structure.
			LPPROPSHEETPAGE pPage = reinterpret_cast<LPPROPSHEETPAGE>(lParam);
			if (!pPage)
				return TRUE;

			// Get the pointer to the AboutTabPrivate object.
			AboutTabPrivate *const d = reinterpret_cast<AboutTabPrivate*>(pPage->lParam);
			if (!d)
				return TRUE;

			assert(d->hWndPropSheet == nullptr);
			d->hWndPropSheet = hDlg;

			// Store the D object pointer with this particular page dialog.
			SetProp(hDlg, D_PTR_PROP, reinterpret_cast<HANDLE>(d));

			// Initialize the dialog.
			d->init();
			return TRUE;
		}

		case WM_DESTROY: {
			// Remove the D_PTR_PROP property from the page. 
			// The D_PTR_PROP property stored the pointer to the 
			// AboutTabPrivate object.
			RemoveProp(hDlg, D_PTR_PROP);
			return TRUE;
		}

		case WM_NOTIFY: {
			AboutTabPrivate *const d = static_cast<AboutTabPrivate*>(
				GetProp(hDlg, D_PTR_PROP));
			if (!d) {
				// No AboutTabPrivate. Can't do anything...
				return FALSE;
			}

			NMHDR *pHdr = reinterpret_cast<NMHDR*>(lParam);
			switch (pHdr->code) {
				case NM_CLICK:
				case NM_RETURN:
					// SysLink control notification.
					// TODO
					break;

				case PSN_SETACTIVE:
					// Disable the "Defaults" button.
					RpPropSheet_EnableDefaults(GetParent(hDlg), false);
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
 * Property sheet callback procedure.
 * @param hDlg
 * @param uMsg
 * @param wParam
 * @param lParam
 */
UINT CALLBACK AboutTabPrivate::callbackProc(HWND hWnd, UINT uMsg, LPPROPSHEETPAGE ppsp)
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

/**
 * Initialize the bold font.
 * @param hFont Base font.
 */
void AboutTabPrivate::initBoldFont(HFONT hFont)
{
	// TODO: Combine with RP_ShellPropSheetExt's version.
	assert(hFont != nullptr);
	if (!hFont || hFontBold) {
		// No base font, or the bold font
		// is already initialized.
		return;
	}

	// Create the bold font.
	LOGFONT lfFontBold;
	if (GetObject(hFont, sizeof(lfFontBold), &lfFontBold) != 0) {
		// Adjust the font and create a new one.
		lfFontBold.lfWeight = FW_BOLD;
		hFontBold = CreateFontIndirect(&lfFontBold);
	}
}

/**
 * Initialize the program title text.
 */
void AboutTabPrivate::initProgramTitleText(void)
{
	// Initialize the bold font.
	HFONT hFontDlg = GetWindowFont(hWndPropSheet);
	initBoldFont(hFontDlg);

	// Set the bold font for the program title.
	HWND hStaticLine1 = GetDlgItem(hWndPropSheet, IDC_ABOUT_LINE1);
	assert(hStaticLine1 != nullptr);
	if (hStaticLine1 && hFontBold) {
		SetWindowFont(hStaticLine1, hFontBold, FALSE);
	}

	// Version number.
	HWND hStaticVersion = GetDlgItem(hWndPropSheet, IDC_ABOUT_VERSION);
	if (hStaticVersion) {
		wstring s_version;
		s_version.reserve(128);
		s_version= L"Version ";
		s_version += RP2W_c(AboutTabText::prg_version);
		if (AboutTabText::git_version) {
			s_version += L"\r\n";
			s_version += RP2W_c(AboutTabText::git_version);
			if (AboutTabText::git_describe) {
				s_version += L"\r\n";
				s_version += RP2W_c(AboutTabText::git_describe);
			}
		}
		SetWindowText(hStaticVersion, s_version.c_str());
	}

	// Set the icon.
	HWND hStaticIcon = GetDlgItem(hWndPropSheet, IDC_ABOUT_ICON);
	if (hStaticIcon) {
		HICON hIcon = PropSheetIcon::get96Icon();
		if (hIcon) {
			// Get the dialog margin.
			// 7x7 DLU margin is recommended by the Windows UX guidelines.
			// Reference: http://stackoverflow.com/questions/2118603/default-dialog-padding
			RECT dlgMargin = {7, 7, 8, 8};
			MapDialogRect(hWndPropSheet, &dlgMargin);
			const int leftPos_icon = dlgMargin.left * 2;
			const int leftPos = leftPos_icon + 96 + dlgMargin.left;
			const int topPos = (dlgMargin.top * 2) + 96;

			// Set the icon and move it over a bit.
			Static_SetIcon(hStaticIcon, hIcon);
			SetWindowPos(hStaticIcon, 0,
				leftPos_icon, dlgMargin.top,
				96, 96,
				SWP_NOZORDER | SWP_NOOWNERZORDER);
			ShowWindow(hStaticIcon, SW_SHOW);

			// Window rectangle.
			RECT winRect;
			GetClientRect(hWndPropSheet, &winRect);

			// Adjust the other labels.
			for (unsigned int id = IDC_ABOUT_LINE1; id <= IDC_ABOUT_VERSION; id++) {
				HWND hLabel = GetDlgItem(hWndPropSheet, id);
				assert(hLabel != nullptr);
				if (!hLabel)
					continue;

				RECT rect_label;
				GetClientRect(hLabel, &rect_label);
				MapWindowPoints(hLabel, hWndPropSheet, (LPPOINT)&rect_label, 2);
				SetWindowPos(hLabel, 0,
					leftPos, rect_label.top,
					winRect.right - leftPos - dlgMargin.left,
					rect_label.bottom - rect_label.top,
					SWP_NOZORDER | SWP_NOOWNERZORDER);
			}

			// Adjust the tab control.
			// TODO: Text control?
			HWND hTabControl = GetDlgItem(hWndPropSheet, IDC_ABOUT_TABCONTROL);
			assert(hTabControl != nullptr);
			if (hTabControl) {
				SetWindowPos(hTabControl, 0,
					dlgMargin.left, topPos,
					winRect.right - (dlgMargin.left*2),
					winRect.bottom - topPos - dlgMargin.top,
					SWP_NOZORDER | SWP_NOOWNERZORDER);
			}
		} else {
			// No icon...
			ShowWindow(hStaticIcon, SW_HIDE);
		}
	}
}

/**
 * Initialize the dialog.
 */
void AboutTabPrivate::init(void)
{
	initProgramTitleText();
}

/** AboutTab **/

AboutTab::AboutTab(void)
	: d_ptr(new AboutTabPrivate())
{ }

AboutTab::~AboutTab()
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
HPROPSHEETPAGE AboutTab::getHPropSheetPage(void)
{
	RP_D(AboutTab);
	assert(d->hPropSheetPage == nullptr);
	if (d->hPropSheetPage) {
		// Property sheet has already been created.
		return nullptr;
	}

	extern HINSTANCE g_hInstance;

	PROPSHEETPAGE psp;
	psp.dwSize = sizeof(psp);	
	psp.dwFlags = PSP_USECALLBACK | PSP_USETITLE;
	psp.hInstance = g_hInstance;
	psp.pszTemplate = MAKEINTRESOURCE(IDD_CONFIG_ABOUT);
	psp.pszIcon = nullptr;
	psp.pszTitle = L"About";
	psp.pfnDlgProc = AboutTabPrivate::dlgProc;
	psp.lParam = reinterpret_cast<LPARAM>(d);
	psp.pcRefParent = nullptr;
	psp.pfnCallback = AboutTabPrivate::callbackProc;

	d->hPropSheetPage = CreatePropertySheetPage(&psp);
	return d->hPropSheetPage;
}

/**
 * Reset the contents of this tab.
 */
void AboutTab::reset(void)
{
	// Nothing to reset here...
}

/**
 * Load the default configuration.
 * This does NOT save, and will only emit modified()
 * if it's different from the current configuration.
 */
void AboutTab::loadDefaults(void)
{
	// Nothing to load here...
}

/**
 * Save the contents of this tab.
 */
void AboutTab::save(void)
{
	// Nothing to load here...
}
