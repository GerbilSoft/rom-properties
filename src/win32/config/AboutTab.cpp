/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * AboutTab.cpp: About tab for rp-config.                                  *
 *                                                                         *
 * Copyright (c) 2016-2018 by David Korth.                                 *
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
#include "config.librpbase.h"

#include "AboutTab.hpp"
#include "resource.h"

// libwin32common
#include "libwin32common/WinUI.hpp"

// librpbase
#include "librpbase/TextFuncs.hpp"
#include "librpbase/config/AboutTabText.hpp"
using namespace LibRpBase;

// libi18n
#include "libi18n/i18n.h"

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
using std::u16string;

// Windows: RichEdit control.
#include <richedit.h>
// NOTE: AURL_ENABLEURL is only defined if _RICHEDIT_VER >= 0x0800
// but this seems to work on Windows XP.
// NOTE 2: AURL_ENABLEEMAILADDR might only work on Win8+.
#ifndef AURL_ENABLEURL
# define AURL_ENABLEURL 1
#endif
#ifndef AURL_ENABLEEMAILADDR
# define AURL_ENABLEEMAILADDR 2
#endif

// Other libraries.
#ifdef HAVE_ZLIB
# include <zlib.h>
#endif
#ifdef HAVE_PNG
# include "librpbase/img/APNG_dlopen.h"
# include <png.h>
#endif
#ifdef ENABLE_XML
# include "tinyxml2.h"
#endif

// Useful RTF strings.
#define RTF_START "{\\rtf1\\ansi\\ansicpg1252\\deff0\\deflang1033\n"
#define RTF_BR "\\par\n"
#define RTF_TAB "\\tab "
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

		/**
		 * Convert a UTF-8 string to RTF-escaped text.
		 * @param str UTF-8 string.
		 * @return RTF-escaped text.
		 */
		static string rtfEscape(const char *str);

	protected:
		// Tab text. (RichText format)
		string sCredits;
		string sLibraries;
		string sSupport;

		/**
		 * Initialize the program title text.
		 */
		void initProgramTitleText(void);

		/**
		 * Initialize the "Credits" tab.
		 */
		void initCreditsTab(void);

		/**
		 * Initialize the "Libraries" tab.
		 */
		void initLibrariesTab(void);

		/**
		 * Initialize the "Support" tab.
		 */
		void initSupportTab(void);

		/**
		 * Set tab contents.
		 * @param index Tab index.
		 */
		void setTabContents(int index);

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
				case EN_LINK: {
					// RichEdit link notification.
					if (pHdr->idFrom != IDC_ABOUT_RICHEDIT)
						break;

					ENLINK *pENLink = reinterpret_cast<ENLINK*>(pHdr);
					if (pENLink->msg == WM_LBUTTONUP) {
						wchar_t urlbuf[256];
						if ((pENLink->chrg.cpMax - pENLink->chrg.cpMin) >= ARRAY_SIZE(urlbuf)) {
							// URL is too big.
							break;
						}
						TEXTRANGE range;
						range.chrg = pENLink->chrg;
						range.lpstrText = urlbuf;
						LRESULT lResult = SendMessage(pHdr->hwndFrom, EM_GETTEXTRANGE, 0, (LPARAM)&range);
						if (lResult > 0 && lResult < ARRAY_SIZE(urlbuf)) {
							ShellExecute(nullptr, L"open", urlbuf, nullptr, nullptr, SW_SHOW);
						}
					}
				}

				case NM_CLICK:
				case NM_RETURN:
					// SysLink control notification.
					// TODO
					break;

				case PSN_SETACTIVE:
					// Disable the "Defaults" button.
					RpPropSheet_EnableDefaults(GetParent(hDlg), false);
					break;

				case TCN_SELCHANGE: {
					// Tab change. Make sure this is the correct WC_TABCONTROL.
					HWND hTabControl = GetDlgItem(hDlg, IDC_ABOUT_TABCONTROL);
					assert(hTabControl != nullptr);
					if (likely(hTabControl)) {
						// Set the tab contents.
						int index = TabCtrl_GetCurSel(hTabControl);
						d->setTabContents(index);
					}
					break;
				}

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
	if (unlikely(!hFont) || hFontBold) {
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
 * Convert a UTF-8 string to RTF-escaped text.
 * @param str UTF-8 string.
 * @return RTF-escaped text.
 */
string AboutTabPrivate::rtfEscape(const char *str)
{
	assert(str != nullptr);
	if (unlikely(!str)) {
		return string();
	}

	// Convert the string to UTF-16 first.
	const u16string u16str = utf8_to_utf16(str, -1);
	const char16_t *wcs = u16str.c_str();

	// RTF return string.
	string ret;

	// Reference: http://www.zopatista.com/python/2012/06/06/rtf-and-unicode/
	char buf[12];	// Conversion buffer.
	for (; *wcs != 0; wcs++) {
		if (*wcs <= 0x00FF) {
			// cp1252 is a superset of ISO-8859-1.
			ret += (char)*wcs;
		} else {
			// Convert to a signed 16-bit integer.
			// Surrogate pairs are encoded as two separate characters.
			snprintf(buf, sizeof(buf), "\\u%d?", (int16_t)*wcs);
			ret += buf;
		}
	}

	return ret;
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
	if (unlikely(!hStaticIcon || !hStaticLine1 || !hStaticVersion || !hTabControl)) {
		// Something went wrong...
		return;
	}

	// Initialize the bold font.
	HFONT hFontDlg = GetWindowFont(hWndPropSheet);
	initBoldFont(hFontDlg);

	// Set the bold font for the program title.
	assert(hFontBold != nullptr);
	if (unlikely(hFontBold)) {
		SetWindowFont(hStaticLine1, hFontBold, FALSE);
	}

	// Version number.
	string s_version = rp_sprintf(C_("AboutTab", "Version %s"),
		AboutTabText::prg_version);
	s_version.reserve(1024);
	if (AboutTabText::git_version[0] != 0) {
		s_version += "\r\n";
		s_version += AboutTabText::git_version;
		if (AboutTabText::git_describe[0] != 0) {
			s_version += "\r\n";
			s_version += AboutTabText::git_describe;
		}
	}
	SetWindowText(hStaticVersion, U82W_s(s_version));

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
			if (unlikely(!hLabel))
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
	sCredits = RTF_START;
	// FIXME: Figure out how to get links to work without
	// resorting to manually adding CFE_LINK data...
	// NOTE: Copyright is NOT localized.
	sCredits += "Copyright (c) 2016-2018 by David Korth." RTF_BR;
	sCredits += RTF_BR;
	sCredits += C_("AboutTab|Credits", "This program is licensed under the GNU GPL v2 or later.");
	sCredits += RTF_BR;
	sCredits += C_("AboutTab|Credits", "https://www.gnu.org/licenses/old-licenses/gpl-2.0.en.html");

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
					sCredits += C_("AboutTab|Credits", "Developers:");
					break;
				case AboutTabText::CT_CONTRIBUTOR:
					sCredits += C_("AboutTab|Credits", "Contributors:");
					break;
				case AboutTabText::CT_TRANSLATOR:
					sCredits += C_("AboutTab|Credits", "Translators:");
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
		sCredits += RTF_BR RTF_TAB RTF_BULLET " ";
		sCredits += rtfEscape(creditsData->name);
		if (creditsData->url) {
			// FIXME: Figure out how to get hyperlinks working.
			sCredits += " <";
			sCredits += rtfEscape(creditsData->linkText);
			sCredits += '>';
		}
		if (creditsData->sub) {
			// Sub-credit.
			sCredits += rp_sprintf(C_("AboutTab|Credits", " (%s)"),
				rtfEscape(creditsData->sub));
		}
	}

	sCredits += '}';

	// Add the "Credits" tab.
	const wstring wsTabTitle = U82W_c(C_("AboutTab", "Credits"));
	TCITEM tcItem;
	tcItem.mask = TCIF_TEXT;
	tcItem.pszText = const_cast<LPWSTR>(wsTabTitle.c_str());
	TabCtrl_InsertItem(GetDlgItem(hWndPropSheet, IDC_ABOUT_TABCONTROL), 0, &tcItem);
}

/**
 * Initialize the "Libraries" tab.
 */
void AboutTabPrivate::initLibrariesTab(void)
{
	sLibraries.clear();
	sLibraries.reserve(4096);

	// RTF starting sequence.
	sLibraries = RTF_START;

	// NOTE: We're only showing the "compiled with" version here,
	// since the DLLs are delay-loaded and might not be available.

	/** zlib **/
#ifdef HAVE_ZLIB
	sLibraries += "Compiled with zlib " ZLIB_VERSION "." RTF_BR
		"Copyright (C) 1995-2017 Jean-loup Gailly and Mark Adler." RTF_BR
		"https://zlib.net/" RTF_BR
		"License: zlib license";
#endif /* HAVE_ZLIB */

	/** libpng **/
	// FIXME: Use png_get_copyright().
#ifdef HAVE_PNG
	sLibraries += RTF_BR RTF_BR
		"Compiled with libpng " PNG_LIBPNG_VER_STRING "." RTF_BR
		"libpng version 1.6.34 - September 29, 2017" RTF_BR
		"Copyright (c) 1998-2002,2004,2006-2017 Glenn Randers-Pehrson" RTF_BR
		"Copyright (c) 1996-1997 Andreas Dilger" RTF_BR
		"Copyright (c) 1995-1996 Guy Eric Schalnat, Group 42, Inc." RTF_BR
		"http://www.libpng.org/pub/png/libpng.html" RTF_BR
		"License: libpng license";
#endif /* HAVE_PNG */

	/** TinyXML2 **/
#ifdef ENABLE_XML
	char sXmlVersion[24];
	snprintf(sXmlVersion, sizeof(sXmlVersion), "TinyXML2 %u.%u.%u",
		TIXML2_MAJOR_VERSION,
		TIXML2_MINOR_VERSION,
		TIXML2_PATCH_VERSION);

	// FIXME: Runtime version?
	sLibraries += RTF_BR RTF_BR "Compiled with ";
	sLibraries += sXmlVersion;
	sLibraries += '.';
	sLibraries += RTF_BR
		"Copyright (C) 2000-2017 Lee Thomason" RTF_BR
		"http://www.grinninglizard.com/" RTF_BR
		"License: zlib license";
#endif /* ENABLE_XML */

	sLibraries += "}";

	// Add the "Libraries" tab.
	const wstring wsTabTitle = U82W_c(C_("AboutTab", "Libraries"));
	TCITEM tcItem;
	tcItem.mask = TCIF_TEXT;
	tcItem.pszText = const_cast<LPWSTR>(wsTabTitle.c_str());
	TabCtrl_InsertItem(GetDlgItem(hWndPropSheet, IDC_ABOUT_TABCONTROL), 1, &tcItem);
}

/**
 * Initialize the "Support" tab.
 */
void AboutTabPrivate::initSupportTab(void)
{
	sSupport.clear();
	sSupport.reserve(4096);

	// RTF starting sequence.
	sSupport = RTF_START;

	sSupport += C_("AboutTab|Support",
		"For technical support, you can visit the following websites:");
	sSupport += RTF_BR;

	for (const AboutTabText::SupportSite_t *supportSite = &AboutTabText::SupportSites[0];
	     supportSite->name != nullptr; supportSite++)
	{
		sSupport += RTF_TAB RTF_BULLET " ";
		sSupport += rtfEscape(supportSite->name);
		sSupport += " <";
		sSupport += rtfEscape(supportSite->url);
		sSupport += '>';
		sSupport += RTF_BR;
	}

	// Email the author.
	sSupport += RTF_BR;
	sSupport += C_("AboutTab|Support",
		"You can also email the developer directly:");
	sSupport += RTF_BR RTF_TAB RTF_BULLET " David Korth <gerbilsoft@gerbilsoft.com>";
	sSupport += '}';

	// Add the "Support" tab.
	const wstring wsTabTitle = U82W_c(C_("AboutTab", "Support"));
	TCITEM tcItem;
	tcItem.mask = TCIF_TEXT;
	tcItem.pszText = const_cast<LPWSTR>(wsTabTitle.c_str());
	TabCtrl_InsertItem(GetDlgItem(hWndPropSheet, IDC_ABOUT_TABCONTROL), 2, &tcItem);
}

/**
 * Set tab contents.
 * @param index Tab index.
 */
void AboutTabPrivate::setTabContents(int index)
{
	assert(index >= 0);
	assert(index <= 2);
	if (unlikely(index < 0 || index > 2))
		return;

	HWND hRichEdit = GetDlgItem(hWndPropSheet, IDC_ABOUT_RICHEDIT);
	assert(hRichEdit != nullptr);
	if (unlikely(!hRichEdit)) {
		// Something went wrong...
		return;
	}

	// FIXME: Figure out how to get links to work without
	// resorting to manually adding CFE_LINK data...

	// NOTE: EM_SETTEXTEX doesn't seem to work.
	// We'll need to stream in the text instead.
	// Reference: https://blogs.msdn.microsoft.com/oldnewthing/20070110-13/?p=28463
	switch (index) {
		case 0:
			rtfCtx.str = &sCredits;
			break;
		case 1:
			rtfCtx.str = &sLibraries;
			break;
		case 2:
			rtfCtx.str = &sSupport;
			break;
		default:
			// Should not get here...
			assert(!"Invalid tab index.");
			return;
	}
	rtfCtx.pos = 0;
	EDITSTREAM es = { (DWORD_PTR)&rtfCtx, 0, EditStreamCallback };
	SendMessage(hRichEdit, EM_STREAMIN, SF_RTF, (LPARAM)&es);
}

/**
 * Initialize the dialog.
 */
void AboutTabPrivate::init(void)
{
	initProgramTitleText();
	initCreditsTab();
	initLibrariesTab();
	initSupportTab();

	// Adjust the RichEdit position.
	assert(hWndPropSheet != nullptr);
	if (unlikely(!hWndPropSheet)) {
		// Something went wrong...
		return;
	}

	HWND hTabControl = GetDlgItem(hWndPropSheet, IDC_ABOUT_TABCONTROL);
	HWND hRichEdit = GetDlgItem(hWndPropSheet, IDC_ABOUT_RICHEDIT);
	assert(hTabControl != nullptr);
	assert(hRichEdit != nullptr);
	if (unlikely(!hTabControl || !hRichEdit)) {
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

	// Enable links.
	LRESULT eventMask = SendMessage(hRichEdit, EM_GETEVENTMASK, 0, 0);
	SendMessage(hRichEdit, EM_SETEVENTMASK, 0, (LPARAM)(eventMask | ENM_LINK));
	SendMessage(hRichEdit, EM_AUTOURLDETECT, AURL_ENABLEURL, 0);
	// NOTE: Might only work on Win8+.
	SendMessage(hRichEdit, EM_AUTOURLDETECT, AURL_ENABLEEMAILADDR, 0);

	// Subclass the control.
	// TODO: Error handling?
	SetWindowSubclass(hRichEdit,
		LibWin32Common::MultiLineEditProc,
		IDC_ABOUT_RICHEDIT,
		reinterpret_cast<DWORD_PTR>(GetParent(hWndPropSheet)));

	// Set tab contents to Credits.
	setTabContents(0);
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
	if (unlikely(d->hPropSheetPage)) {
		// Property sheet has already been created.
		return nullptr;
	}

	// tr: Tab title.
	const wstring wsTabTitle = U82W_c(C_("AboutTab", "About"));

	PROPSHEETPAGE psp;
	psp.dwSize = sizeof(psp);	
	psp.dwFlags = PSP_USECALLBACK | PSP_USETITLE;
	psp.hInstance = HINST_THISCOMPONENT;
	psp.pszTemplate = MAKEINTRESOURCE(IDD_CONFIG_ABOUT);
	psp.pszIcon = nullptr;
	psp.pszTitle = wsTabTitle.c_str();
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
