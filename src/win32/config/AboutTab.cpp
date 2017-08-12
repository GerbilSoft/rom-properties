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
 * You should have received a copy of the GNU General Public License       *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.   *
 ***************************************************************************/

#include "stdafx.h"
#include "AboutTab.hpp"
#include "resource.h"

// librpbase
#include "librpbase/TextFuncs.hpp"
#include "librpbase/TextFuncs_utf8.hpp"
#include "librpbase/config/AboutTabText.hpp"
using LibRpBase::AboutTabText;

// Property sheet icon.
// Extracted from imageres.dll or shell32.dll.
#include "PropSheetIcon.hpp"

// C includes. (C++ namespace)
#include <cassert>
#include <cstring>

// C++ includes.
#include <string>
using std::string;
using std::wstring;

// Windows: RichEdit control.
#include <richedit.h>

// Useful RTF strings.
#define RTF_BR "\\par\n"
#define RTF_BULLET "\\bullet "

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
		// Current RichText streaming context.
		struct RTF_CTX {
			const string *str;
			size_t pos;
		};
		RTF_CTX rtfCtx;

		/**
		 * RTF EditStream callback.
		 * @param dwCookie	[in] Pointer to RTF_CTX.
		 * @param lpBuff	[out] Output buffer.
		 * @param cb		[in] Number of bytes to write.
		 * @param pcb		[out] Number of bytes actually written.
		 * @return 0 on success; non-zero on error.
		 */
		static DWORD CALLBACK EditStreamCallback(_In_ DWORD_PTR dwCookie,
			_Out_ LPBYTE pbBuff, _In_ LONG cb, _Out_ LONG *pcb);

	protected:
		// Tab text. (RichText format)
		string sCredits;

		/**
		 * Initialize the program title text.
		 */
		void initProgramTitleText(void);

		/**
		 * Initialize the "Credits" tab.
		 */
		void initCreditsTab(void);

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
{
	memset(&rtfCtx, 0, sizeof(rtfCtx));
}

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
 * RTF EditStream callback.
 * @param dwCookie	[in] Pointer to RTF_CTX.
 * @param lpBuff	[out] Output buffer.
 * @param cb		[in] Number of bytes to write.
 * @param pcb		[out] Number of bytes actually written.
 * @return 0 on success; non-zero on error.
 */
DWORD CALLBACK AboutTabPrivate::EditStreamCallback(
	_In_ DWORD_PTR dwCookie,
	_Out_ LPBYTE pbBuff,
	_In_ LONG cb,
	_Out_ LONG *pcb)
{
	// Reference: https://blogs.msdn.microsoft.com/oldnewthing/20070110-13/?p=28463
	RTF_CTX *const rtfCtx = reinterpret_cast<RTF_CTX*>(dwCookie);
	if (!rtfCtx->str) {
		// No string.
		*pcb = 0;
		return -1;
	}

	if (rtfCtx->pos >= rtfCtx->str->size()) {
		// No string data left.
		*pcb = 0;
		return 0;
	}

	// Check how much data we have left.
	LONG remain = (LONG)(rtfCtx->str->size() - rtfCtx->pos);
	if (cb > remain) {
		cb = remain;
	}

	// Copy the string data.
	memcpy(pbBuff, &rtfCtx->str->data()[rtfCtx->pos], cb);
	rtfCtx->pos += cb;
	*pcb = cb;
	return 0;
}

/**
 * Initialize the program title text.
 */
void AboutTabPrivate::initProgramTitleText(void)
{
	// Get the controls.
	HWND hStaticIcon = GetDlgItem(hWndPropSheet, IDC_ABOUT_ICON);
	HWND hStaticLine1 = GetDlgItem(hWndPropSheet, IDC_ABOUT_LINE1);
	HWND hStaticVersion = GetDlgItem(hWndPropSheet, IDC_ABOUT_VERSION);
	HWND hTabControl = GetDlgItem(hWndPropSheet, IDC_ABOUT_TABCONTROL);
	assert(hStaticIcon != nullptr);
	assert(hStaticLine1 != nullptr);
	assert(hStaticVersion != nullptr);
	assert(hTabControl != nullptr);
	if (!hStaticIcon || !hStaticLine1 || !hStaticVersion || !hTabControl) {
		// Something went wrong...
		return;
	}

	// Initialize the bold font.
	HFONT hFontDlg = GetWindowFont(hWndPropSheet);
	initBoldFont(hFontDlg);

	// Set the bold font for the program title.
	assert(hFontBold != nullptr);
	if (hFontBold) {
		SetWindowFont(hStaticLine1, hFontBold, FALSE);
	}

	// Version number.
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

	// Set the icon.
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
		SetWindowPos(hTabControl, 0,
			dlgMargin.left, topPos,
			winRect.right - (dlgMargin.left*2),
			winRect.bottom - topPos - dlgMargin.top,
			SWP_NOZORDER | SWP_NOOWNERZORDER);
	} else {
		// No icon...
		ShowWindow(hStaticIcon, SW_HIDE);
	}
}

/**
 * Initialize the "Credits" tab.
 */
void AboutTabPrivate::initCreditsTab(void)
{
	sCredits.clear();
	sCredits.reserve(4096);

	// RTF starting sequence.
	sCredits = "{\\rtf1\\ansi\\ansicpg1252\\deff0\\deflang1033\n";
	// FIXME: Figure out how to get links to work without
	// resorting to manually adding CFE_LINK data...
	sCredits += "Copyright (c) 2016-2017 by David Korth." RTF_BR
		"This program is licensed under the GNU GPL v2 or later." RTF_BR
		"https://www.gnu.org/licenses/old-licenses/gpl-2.0.en.html";

	AboutTabText::CreditType_t lastCreditType = AboutTabText::CT_CONTINUE;
	for (const AboutTabText::CreditsData_t *creditsData = &AboutTabText::CreditsData[0];
	     creditsData->type < AboutTabText::CT_MAX; creditsData++)
	{
		if (creditsData->type != AboutTabText::CT_CONTINUE &&
		    creditsData->type != lastCreditType)
		{
			// New credit type.
			sCredits += RTF_BR RTF_BR;
			sCredits += "\\b ";

			switch (creditsData->type) {
				case AboutTabText::CT_DEVELOPER:
					sCredits += "Developers:";
					break;

				case AboutTabText::CT_CONTRIBUTOR:
					sCredits += "Contributors:";
					break;

				case AboutTabText::CT_TRANSLATOR:
					sCredits += "Translators:";
					break;

				case AboutTabText::CT_CONTINUE:
				case AboutTabText::CT_MAX:
				default:
					assert(!"Invalid credit type.");
					break;
			}

			sCredits += "\\b0 ";
		}

		// Append the contributor's name.
		sCredits += RTF_BR "\\tab " RTF_BULLET " ";
		// FIXME: Escape UTF-8 for RTF.
		sCredits += RP2U8_c(creditsData->name);
		if (creditsData->url) {
			// FIXME: Figure out how to get hyperlinks working.
			sCredits += " <";
			sCredits += RP2U8_c(creditsData->linkText);
			sCredits += '>';
		}
		if (creditsData->sub) {
			// FIXME: Escape UTF-8 for RTF.
			sCredits += " (";
			sCredits += RP2U8_c(creditsData->sub);
			sCredits += ')';
		}
	}

	sCredits += "}";

	// Add the "Credits" tab.
	TCITEM tcItem;
	tcItem.mask = TCIF_TEXT;
	tcItem.pszText = L"Credits";
	TabCtrl_InsertItem(GetDlgItem(hWndPropSheet, IDC_ABOUT_TABCONTROL), 0, &tcItem);
}

/**
 * Initialize the dialog.
 */
void AboutTabPrivate::init(void)
{
	initProgramTitleText();
	initCreditsTab();

	// Adjust the RichEdit position.
	assert(hWndPropSheet != nullptr);
	if (!hWndPropSheet) {
		// Something went wrong...
		return;
	}

	HWND hTabControl = GetDlgItem(hWndPropSheet, IDC_ABOUT_TABCONTROL);
	HWND hRichEdit = GetDlgItem(hWndPropSheet, IDC_ABOUT_RICHEDIT);
	assert(hTabControl != nullptr);
	assert(hRichEdit != nullptr);
	if (!hTabControl || !hRichEdit) {
		// Something went wrong...
		return;
	}

	RECT winRect, tabRect;
	GetClientRect(hWndPropSheet, &winRect);
	GetClientRect(hTabControl, &tabRect);
	MapWindowPoints(hTabControl, hWndPropSheet, (LPPOINT)&tabRect, 2);
	TabCtrl_AdjustRect(hTabControl, FALSE, &tabRect);

	// Dialog margins.
	RECT dlgMargin = {7, 7, 8, 8};
	MapDialogRect(hWndPropSheet, &dlgMargin);

	// Set the RichEdit's position.
	SetWindowPos(hRichEdit, 0,
		tabRect.left + dlgMargin.left,
		tabRect.top + dlgMargin.top,
		tabRect.right - tabRect.left - (dlgMargin.left*2),
		tabRect.bottom - tabRect.top - (dlgMargin.top*2),
		SWP_NOZORDER | SWP_NOOWNERZORDER);

	// FIXME: Figure out how to get links to work without
	// resorting to manually adding CFE_LINK data...

	// Set credits text by default.
	// NOTE: EM_SETTEXTEX doesn't seem to work.
	// We'll need to stream in the text instead.
	// Reference: https://blogs.msdn.microsoft.com/oldnewthing/20070110-13/?p=28463
	rtfCtx.str = &sCredits;
	rtfCtx.pos = 0;
	EDITSTREAM es = { (DWORD_PTR)&rtfCtx, 0, EditStreamCallback };
	SendMessage(hRichEdit, EM_STREAMIN, SF_RTF, (LPARAM)&es);
	// FIXME: Unselect the text. (This isn't working...)
	Edit_SetSel(hRichEdit, 0, -1);
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
