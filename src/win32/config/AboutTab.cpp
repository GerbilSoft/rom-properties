/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * AboutTab.cpp: About tab for rp-config.                                  *
 *                                                                         *
 * Copyright (c) 2016-2022 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "config.librpbase.h"

#include "AboutTab.hpp"
#include "res/resource.h"

// librpbase
#include "librpbase/config/AboutTabText.hpp"
using namespace LibRpBase;

// libwin32ui
#include "libwin32ui/SubclassWindow.h"

// Property sheet icon.
// Extracted from imageres.dll or shell32.dll.
#include "PropSheetIcon.hpp"

// C++ STL classes.
using std::string;
using std::wstring;
using std::u8string;
using std::u16string;

// Maximum number of tabs.
// NOTE: Must be adjusted if more tabs are added!
#define MAX_TABS 3

// Windows: RichEdit control.
#include <richedit.h>
// NOTE: AURL_ENABLEURL is only defined if _RICHEDIT_VER >= 0x0800
// but this seems to work on Windows XP.
// NOTE 2: AURL_ENABLEEMAILADDR might only work on Win8+.
#ifndef AURL_ENABLEURL
#  define AURL_ENABLEURL 1
#endif
#ifndef AURL_ENABLEEMAILADDR
#  define AURL_ENABLEEMAILADDR 2
#endif
// Uncomment to enable use of RichEdit 4.1 if available.
// FIXME: Friendly links aren't underlined or blue on WinXP or Win7...
// Reference: https://docs.microsoft.com/en-us/archive/blogs/murrays/richedit-colors
//#define MSFTEDIT_USE_41 1

// Other libraries.
#ifdef HAVE_ZLIB
#  include <zlib.h>
#endif
#ifdef HAVE_PNG
#  include "librpbase/img/APNG_dlopen.h"
#  include <png.h>
#endif
#ifdef ENABLE_XML
#  include "tinyxml2.h"
#endif

// Useful RTF strings.
#define RTF_START U8("{\\rtf1\\ansi\\ansicpg1252\\deff0\\deflang1033\n")
#define RTF_BR U8("\\par\n")
#define RTF_TAB U8("\\tab ")
#define RTF_BULLET U8("\\bullet ")

class AboutTabPrivate
{
	public:
		AboutTabPrivate();
		~AboutTabPrivate();

	private:
		RP_DISABLE_COPY(AboutTabPrivate)

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

		// RichEdit DLLs.
		HMODULE hRichEd20_dll;
#ifdef MSFTEDIT_USE_41
		HMODULE hMsftEdit_dll;
#endif /* MSFTEDIT_USE_41 */
		bool bUseFriendlyLinks;

	protected:
		// Current RichText streaming context.
		struct RTF_CTX {
			const u8string *str;
			size_t pos;

			RTF_CTX() : str(nullptr), pos(0) { }
		};
		RTF_CTX rtfCtx;

		/**
		 * RTF EditStream callback
		 * @param dwCookie	[in] Pointer to RTF_CTX
		 * @param lpBuff	[out] Output buffer
		 * @param cb		[in] Number of bytes to write
		 * @param pcb		[out] Number of bytes actually written
		 * @return 0 on success; non-zero on error.
		 */
		static DWORD CALLBACK EditStreamCallback(_In_ DWORD_PTR dwCookie,
			_Out_ LPBYTE pbBuff, _In_ LONG cb, _Out_ LONG *pcb);

		/**
		 * Convert a UTF-8 string to RTF-escaped text.
		 * @param str UTF-8 string.
		 * @return RTF-escaped text.
		 */
		static u8string rtfEscape(const char8_t *str);

		/**
		 * Create an RTF "friendly link" if supported.
		 * If not supported, returns the escaped link title.
		 * @param link	[in] Link address.
		 * @param title	[in] Link title.
		 * @return RTF "friendly link", or title only.
		 */
		u8string rtfFriendlyLink(const char8_t *link, const char8_t *title);

	protected:
		// Tab text. (RichText format)
		u8string sCredits;
		u8string sLibraries;
		u8string sSupport;

		// RichEdit control.
		HWND hRichEdit;

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
		void initDialog(void);
};

/** AboutTabPrivate **/

AboutTabPrivate::AboutTabPrivate()
	: hPropSheetPage(nullptr)
	, hWndPropSheet(nullptr)
	, hFontBold(nullptr)
	, bUseFriendlyLinks(false)
	, hRichEdit(nullptr)
{
	// Load the RichEdit DLLs.
	// TODO: What if this fails?
	hRichEd20_dll = LoadLibrary(_T("RICHED20.DLL"));
#ifdef MSFTEDIT_USE_41
	hMsftEdit_dll = LoadLibrary(_T("MSFTEDIT.DLL"));
#endif /* MSFTEDIT_USE_41 */
}

AboutTabPrivate::~AboutTabPrivate()
{
	if (hFontBold) {
		DeleteFont(hFontBold);
	}
#ifdef MSFTEDIT_USE_41
	if (hMsftEdit_dll) {
		FreeLibrary(hMsftEdit_dll);
	}
#endif /* MSFTEDIT_USE_41 */
	if (hRichEd20_dll) {
		FreeLibrary(hRichEd20_dll);
	}
}

/**
 * Dialog procedure.
 * @param hDlg
 * @param uMsg
 * @param wParam
 * @param lParam
 */
INT_PTR CALLBACK AboutTabPrivate::dlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	RP_UNUSED(wParam);

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
			SetWindowLongPtr(hDlg, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(d));

			// Initialize the dialog.
			d->initDialog();
			return TRUE;
		}

		case WM_NOTIFY: {
			auto *const d = reinterpret_cast<AboutTabPrivate*>(GetWindowLongPtr(hDlg, GWLP_USERDATA));
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
						TCHAR urlbuf[256];
						if ((pENLink->chrg.cpMax - pENLink->chrg.cpMin) >= _countof(urlbuf)) {
							// URL is too big.
							break;
						}
						TEXTRANGE range;
						range.chrg = pENLink->chrg;
						range.lpstrText = urlbuf;
						LRESULT lResult = SendMessage(pHdr->hwndFrom, EM_GETTEXTRANGE, 0, (LPARAM)&range);
						if (lResult > 0 && lResult < _countof(urlbuf)) {
							ShellExecute(nullptr, _T("open"), urlbuf, nullptr, nullptr, SW_SHOW);
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
 * RTF EditStream callback
 * @param dwCookie	[in] Pointer to RTF_CTX
 * @param lpBuff	[out] Output buffer
 * @param cb		[in] Number of bytes to write
 * @param pcb		[out] Number of bytes actually written
 * @return 0 on success; non-zero on error.
 */
DWORD CALLBACK AboutTabPrivate::EditStreamCallback(
	_In_ DWORD_PTR dwCookie,
	_Out_ LPBYTE pbBuff,
	_In_ LONG cb,
	_Out_ LONG *pcb)
{
	// Reference: https://devblogs.microsoft.com/oldnewthing/20070110-13/?p=28463
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
u8string AboutTabPrivate::rtfEscape(const char8_t *str)
{
	u8string s_ret;

	assert(str != nullptr);
	if (unlikely(!str)) {
		return s_ret;
	}

	// Convert the string to UTF-16 first.
	const u16string u16str = utf8_to_utf16(str, -1);
	const char16_t *wcs = u16str.c_str();

	// Reference: http://www.zopatista.com/python/2012/06/06/rtf-and-unicode/
	char buf[12];	// Conversion buffer.
	for (; *wcs != 0; wcs++) {
		if (*wcs <= 0x00FF) {
			// cp1252 is a superset of ISO-8859-1.
			s_ret += (char8_t)*wcs;
		} else {
			// Convert to a signed 16-bit integer.
			// Surrogate pairs are encoded as two separate characters.
			// NOTE: snprintf() doesn't support char8_t.
			snprintf(buf, sizeof(buf), "\\u%d?", (int16_t)*wcs);
			s_ret += reinterpret_cast<const char8_t*>(buf);
		}
	}

	return s_ret;
}

/**
 * Create an RTF "friendly link" if supported.
 * If not supported, returns the escaped link title.
 * @param link	[in] Link address
 * @param title	[in] Link title
 * @return RTF "friendly link", or title only.
 */
u8string AboutTabPrivate::rtfFriendlyLink(const char8_t *link, const char8_t *title)
{
	assert(link != nullptr);
	assert(title != nullptr);

	if (bUseFriendlyLinks) {
		// Friendly links are available.
		// Reference: https://docs.microsoft.com/en-us/archive/blogs/murrays/richedit-friendly-name-hyperlinks
		// FIXME: U8STRFIX for rp_sprintf().
		/*return rp_sprintf("{\\field{\\*\\fldinst{HYPERLINK \"%s\"}}{\\fldrslt{%s}}}",
			reinterpret_cast<const char*>(rtfEscape(link).c_str()),
			reinterpret_cast<const char*>(rtfEscape(title).c_str()));*/
		char buf[1024];
		snprintf(buf, sizeof(buf), "{\\field{\\*\\fldinst{HYPERLINK \"%s\"}}{\\fldrslt{%s}}}",
			reinterpret_cast<const char*>(rtfEscape(link).c_str()),
			reinterpret_cast<const char*>(rtfEscape(title).c_str()));
		return u8string(reinterpret_cast<const char8_t*>(buf));
	} else {
		// No friendly links.
		return rtfEscape(title);
	}
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
	const char *const programVersion =
		AboutTabText::getProgramInfoString(AboutTabText::ProgramInfoStringID::ProgramVersion);
	const char *const gitVersion =
		AboutTabText::getProgramInfoString(AboutTabText::ProgramInfoStringID::GitVersion);

	string s_version = rp_sprintf(C_("AboutTab", "Version %s"), programVersion);
	s_version.reserve(1024);
	if (gitVersion) {
		s_version += "\r\n";
		s_version += gitVersion;
		const char *const gitDescription =
			AboutTabText::getProgramInfoString(AboutTabText::ProgramInfoStringID::GitDescription);
		if (gitDescription) {
			s_version += "\r\n";
			s_version += gitDescription;
		}
	}
	const tstring st_version = U82T_s(s_version);
	SetWindowText(hStaticVersion, st_version.c_str());

	// Reduce the vertical size of hStaticVersion to fit the text.
	// High DPI (e.g. 150% on 1920x1080) can cause the label to
	// overlap the tab control.
	// FIXME: If we have too many lines of text, this might still cause problems.
	SIZE sz_hStaticVersion;
	int ret = LibWin32UI::measureTextSize(hWndPropSheet, hFontDlg, st_version.c_str(), &sz_hStaticVersion);
	if (ret == 0) {
		RECT rectStaticVersion;
		GetWindowRect(hStaticVersion, &rectStaticVersion);
		SetWindowPos(hStaticVersion, 0, 0, 0,
			rectStaticVersion.right - rectStaticVersion.left, sz_hStaticVersion.cy,
			SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_NOMOVE);
	}

	// Set the icon.
	const PropSheetIcon *const psi = PropSheetIcon::instance();
	HICON hIcon = psi->get96Icon();
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
	// FIXME: U8STRFIX
	sCredits += reinterpret_cast<const char8_t*>(
		AboutTabText::getProgramInfoString(AboutTabText::ProgramInfoStringID::Copyright));
	sCredits += RTF_BR RTF_BR;
	sCredits += reinterpret_cast<const char8_t*>(rp_sprintf(
		// tr: %s is the name of the license.
		reinterpret_cast<const char*>(rtfEscape(
			(const char8_t*)C_("AboutTab|Credits", "This program is licensed under the %s or later.")).c_str()),
		reinterpret_cast<const char*>(rtfFriendlyLink(
			U8("https://www.gnu.org/licenses/gpl-2.0.html"),
			(const char8_t*)C_("AboutTabl|Credits", "GNU GPL v2")).c_str())).c_str());
	if (!bUseFriendlyLinks) {
		sCredits += RTF_BR;
		sCredits += U8("https://www.gnu.org/licenses/gpl-2.0.html");
	}

	AboutTabText::CreditType lastCreditType = AboutTabText::CreditType::Continue;
	for (const AboutTabText::CreditsData_t *creditsData = AboutTabText::getCreditsData();
	     creditsData->type < AboutTabText::CreditType::Max; creditsData++)
	{
		if (creditsData->type != AboutTabText::CreditType::Continue &&
		    creditsData->type != lastCreditType)
		{
			// New credit type.
			sCredits += RTF_BR RTF_BR;
			sCredits += U8("\\b ");

			// FIXME: U8STRFIX
			switch (creditsData->type) {
				case AboutTabText::CreditType::Developer:
					sCredits += rtfEscape((const char8_t*)C_("AboutTab|Credits", "Developers:"));;
					break;
				case AboutTabText::CreditType::Contributor:
					sCredits += rtfEscape((const char8_t*)C_("AboutTab|Credits", "Contributors:"));
					break;
				case AboutTabText::CreditType::Translator:
					sCredits += rtfEscape((const char8_t*)C_("AboutTab|Credits", "Translators:"));
					break;

				case AboutTabText::CreditType::Continue:
				case AboutTabText::CreditType::Max:
				default:
					assert(!"Invalid credit type.");
					break;
			}

			sCredits += U8("\\b0 ");
		}

		// Append the contributor's name.
		// FIXME: U8STRFIX
		sCredits += RTF_BR RTF_TAB RTF_BULLET U8(" ");
		sCredits += rtfEscape((const char8_t*)creditsData->name).c_str();
		if (creditsData->url) {
			// FIXME: Figure out how to get hyperlinks working.
			sCredits += U8(" <");
			if (creditsData->linkText) {
				sCredits += rtfFriendlyLink((const char8_t*)creditsData->url, (const char8_t*)creditsData->linkText);
			} else {
				sCredits += rtfFriendlyLink((const char8_t*)creditsData->url, (const char8_t*)creditsData->url);
			}
			sCredits += U8(">");
		}
		if (creditsData->sub) {
			// Sub-credit.
			sCredits += reinterpret_cast<const char8_t*>(
				rp_sprintf(reinterpret_cast<const char*>(
					rtfEscape((const char8_t*)C_("AboutTab|Credits", " (%s)")).c_str()),
					reinterpret_cast<const char*>(
						rtfEscape((const char8_t*)creditsData->sub).c_str())).c_str());
		}

		lastCreditType = creditsData->type;
	}

	sCredits += '}';

	// Add the "Credits" tab.
	const tstring tsTabTitle = U82T_c((const char8_t*)C_("AboutTab", "Credits"));
	TCITEM tcItem;
	tcItem.mask = TCIF_TEXT;
	tcItem.pszText = const_cast<LPTSTR>(tsTabTitle.c_str());
	TabCtrl_InsertItem(GetDlgItem(hWndPropSheet, IDC_ABOUT_TABCONTROL), 0, &tcItem);
}

/**
 * Initialize the "Libraries" tab.
 */
void AboutTabPrivate::initLibrariesTab(void)
{
	char sVerBuf[64];

	sLibraries.clear();
	sLibraries.reserve(8192);

	// RTF starting sequence.
	sLibraries = RTF_START;

	// NOTE: These strings can NOT be static.
	// Otherwise, they won't be retranslated if the UI language
	// is changed at runtime.

	// tr: Using an internal copy of a library.
	const char *const sIntCopyOf = C_("AboutTab|Libraries", "Internal copy of %s.");
	// tr: Compiled with a specific version of an external library.
	const char *const sCompiledWith = C_("AboutTab|Libraries", "Compiled with %s.");
	// tr: Using an external library, e.g. libpcre.so
	const char *const sUsingDll = C_("AboutTab|Libraries", "Using %s.");
	// tr: License: (libraries with only a single license)
	const char *const sLicense = C_("AboutTab|Libraries", "License: %s");
	// tr: Licenses: (libraries with multiple licenses)
	const char *const sLicenses = C_("AboutTab|Libraries", "Licenses: %s");

	// NOTE: We're only showing the "compiled with" version here,
	// since the DLLs are delay-loaded and might not be available.

	/** zlib **/
#ifdef HAVE_ZLIB
#  ifdef ZLIBNG_VERSION
	sLibraries += reinterpret_cast<const char8_t*>(
		rp_sprintf(sCompiledWith, "zlib-ng " ZLIBNG_VERSION).c_str());
	sLibraries += RTF_BR;
#  else /* !ZLIBNG_VERSION */
	sLibraries += reinterpret_cast<const char8_t*>(
		rp_sprintf(sCompiledWith, "zlib " ZLIB_VERSION).c_str());
	sLibraries += RTF_BR;
#  endif /* ZLIBNG_VERSION */
	sLibraries += U8("Copyright (C) 1995-2022 Jean-loup Gailly and Mark Adler.") RTF_BR
		U8("https://zlib.net/") RTF_BR;
#  ifdef ZLIBNG_VERSION
	// TODO: Also if zlibVersion() contains "zlib-ng"?
	sLibraries += U8("https://github.com/zlib-ng/zlib-ng") RTF_BR;
#  endif /* ZLIBNG_VERSION */
	sLibraries += reinterpret_cast<const char8_t*>(
		rp_sprintf(sLicense, "zlib license").c_str());
#endif /* HAVE_ZLIB */

	/** libpng **/
	// FIXME: Use png_get_copyright().
	// FIXME: Check for APNG.
#ifdef HAVE_PNG
	sLibraries += RTF_BR RTF_BR;
	sLibraries += reinterpret_cast<const char8_t*>(
		rp_sprintf(sCompiledWith, "libpng " PNG_LIBPNG_VER_STRING).c_str());
	sLibraries += RTF_BR
			U8("libpng version 1.6.37 - April 14, 2019") RTF_BR
			U8("Copyright (c) 2018-2019 Cosmin Truta") RTF_BR
			U8("Copyright (c) 1998-2002,2004,2006-2018 Glenn Randers-Pehrson") RTF_BR
			U8("Copyright (c) 1996-1997 Andreas Dilger") RTF_BR
			U8("Copyright (c) 1995-1996 Guy Eric Schalnat, Group 42, Inc.") RTF_BR
			U8("http://www.libpng.org/pub/png/libpng.html") RTF_BR;
	sLibraries += reinterpret_cast<const char8_t*>(
		rp_sprintf(sLicense, "libpng license").c_str());
#endif /* HAVE_PNG */

	/** TinyXML2 **/
#ifdef ENABLE_XML
	snprintf(sVerBuf, sizeof(sVerBuf), "TinyXML2 %u.%u.%u",
		static_cast<unsigned int>(TIXML2_MAJOR_VERSION),
		static_cast<unsigned int>(TIXML2_MINOR_VERSION),
		static_cast<unsigned int>(TIXML2_PATCH_VERSION));

	// FIXME: Runtime version?
	sLibraries += RTF_BR RTF_BR;
	sLibraries += reinterpret_cast<const char8_t*>(
		rp_sprintf(sCompiledWith, sVerBuf).c_str());
	sLibraries += RTF_BR
		U8("Copyright (C) 2000-2021 Lee Thomason") RTF_BR
		U8("http://www.grinninglizard.com/") RTF_BR;
	sLibraries += reinterpret_cast<const char8_t*>(
		rp_sprintf(sLicense, "zlib license").c_str());
#endif /* ENABLE_XML */

	/** GNU gettext **/
	// NOTE: glibc's libintl.h doesn't have the version information,
	// so we're only printing this if we're using GNU gettext's version.
#if defined(HAVE_GETTEXT) && defined(LIBINTL_VERSION)
	if (LIBINTL_VERSION & 0xFF) {
		snprintf(sVerBuf, sizeof(sVerBuf), "GNU gettext %u.%u.%u",
			static_cast<unsigned int>( LIBINTL_VERSION >> 16),
			static_cast<unsigned int>((LIBINTL_VERSION >>  8) & 0xFF),
			static_cast<unsigned int>( LIBINTL_VERSION        & 0xFF));
	} else {
		snprintf(sVerBuf, sizeof(sVerBuf), "GNU gettext %u.%u",
			static_cast<unsigned int>( LIBINTL_VERSION >> 16),
			static_cast<unsigned int>((LIBINTL_VERSION >>  8) & 0xFF));
	}

	sLibraries += RTF_BR RTF_BR;
#  ifdef _WIN32
	sLibraries += reinterpret_cast<const char8_t*>(
		rp_sprintf(sIntCopyOf, sVerBuf).c_str());
#  else /* _WIN32 */
	// FIXME: Runtime version?
	sLibraries += reinterpret_cast<const char8_t*>(
		rp_sprintf(sCompiledWith, sVerBuf).c_str());
#  endif /* _WIN32 */
	sLibraries += RTF_BR
		U8("Copyright (C) 1995-1997, 2000-2016, 2018-2020 Free Software Foundation, Inc.") RTF_BR
		U8("https://www.gnu.org/software/gettext/") RTF_BR;
	sLibraries += reinterpret_cast<const char8_t*>(
		rp_sprintf(sLicense, "GNU LGPL v2.1+").c_str());
#endif /* HAVE_GETTEXT && LIBINTL_VERSION */

	sLibraries += U8("}");

	// Add the "Libraries" tab.
	const tstring tsTabTitle = U82T_c(
		reinterpret_cast<const char8_t*>(C_("AboutTab", "Libraries")));
	TCITEM tcItem;
	tcItem.mask = TCIF_TEXT;
	tcItem.pszText = const_cast<LPTSTR>(tsTabTitle.c_str());
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

	// FIXME: U8STRFIX
	sSupport += rtfEscape(reinterpret_cast<const char8_t*>(C_("AboutTab|Support",
		"For technical support, you can visit the following websites:")));
	sSupport += RTF_BR;

	for (const AboutTabText::SupportSite_t *supportSite = AboutTabText::getSupportSites();
	     supportSite->name != nullptr; supportSite++)
	{
		sSupport += RTF_TAB RTF_BULLET U8(" ");
		sSupport += rtfEscape(reinterpret_cast<const char8_t*>(supportSite->name));
		sSupport += U8(" <");
		sSupport += rtfEscape(reinterpret_cast<const char8_t*>(supportSite->url));
		sSupport += U8(">");
		sSupport += RTF_BR;
	}

	// Email the author.
	sSupport += RTF_BR;
	sSupport += rtfEscape(reinterpret_cast<const char8_t*>(C_("AboutTab|Support",
		"You can also email the developer directly:")));
	sSupport += RTF_BR RTF_TAB RTF_BULLET U8(" David Korth <");
	sSupport += rtfFriendlyLink(U8("mailto:gerbilsoft@gerbilsoft.com"), U8("gerbilsoft@gerbilsoft.com"));
	sSupport += U8(">}");

	// Add the "Support" tab.
	const tstring tsTabTitle = U82T_c(reinterpret_cast<const char*>(C_("AboutTab", "Support")));
	TCITEM tcItem;
	tcItem.mask = TCIF_TEXT;
	tcItem.pszText = const_cast<LPTSTR>(tsTabTitle.c_str());
	TabCtrl_InsertItem(GetDlgItem(hWndPropSheet, IDC_ABOUT_TABCONTROL), 2, &tcItem);
}

/**
 * Set tab contents.
 * @param index Tab index.
 */
void AboutTabPrivate::setTabContents(int index)
{
	assert(index >= 0);
	assert(index < MAX_TABS);
	if (unlikely(index < 0 || index >= MAX_TABS))
		return;

	assert(hRichEdit != nullptr);
	if (unlikely(!hRichEdit)) {
		// Something went wrong...
		return;
	}

	// FIXME: Figure out how to get links to work without
	// resorting to manually adding CFE_LINK data...

	// NOTE: EM_SETTEXTEX doesn't seem to work.
	// We'll need to stream in the text instead.
	// Reference: https://devblogs.microsoft.com/oldnewthing/20070110-13/?p=28463
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
void AboutTabPrivate::initDialog(void)
{
	// Initialize the program title text.
	initProgramTitleText();

	// Insert a dummy tab for proper sizing for now.
	HWND hTabControl = GetDlgItem(hWndPropSheet, IDC_ABOUT_TABCONTROL);
	assert(hTabControl != nullptr);
	if (unlikely(!hTabControl)) {
		// Something went wrong...
		return;
	}
	TCITEM tcItem;
	tcItem.mask = TCIF_TEXT;
	tcItem.pszText = _T("DUMMY");
	TabCtrl_InsertItem(hTabControl, MAX_TABS, &tcItem);

	// Adjust the RichEdit position.
	assert(hWndPropSheet != nullptr);
	if (unlikely(!hWndPropSheet)) {
		// Something went wrong...
		return;
	}

	// NOTE: We can't seem to set the dialog ID correctly
	// when using CreateWindowEx(), so we'll save hRichEdit here.
	hRichEdit = GetDlgItem(hWndPropSheet, IDC_ABOUT_RICHEDIT);
	assert(hRichEdit != nullptr);
	if (unlikely(!hRichEdit)) {
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

	// Attempt to switch to RichEdit 4.1 if it's available.
#ifdef MSFTEDIT_USE_41
	if (hMsftEdit_dll) {
		HWND hRichEdit41 = CreateWindowEx(
			WS_EX_NOPARENTNOTIFY | WS_EX_TRANSPARENT | WS_EX_LEFT | WS_EX_CLIENTEDGE,
			MSFTEDIT_CLASS, _T(""),
			WS_TABSTOP | ES_MULTILINE | ES_READONLY | WS_VSCROLL | ES_AUTOVSCROLL | WS_VISIBLE | WS_CHILD,
			0, 0, 0, 0,
			hWndPropSheet, nullptr, nullptr, nullptr);
		if (hRichEdit41) {
			DestroyWindow(hRichEdit);
			SetWindowFont(hRichEdit41, GetWindowFont(hWndPropSheet), FALSE);
			hRichEdit = hRichEdit41;
			// FIXME: Not working...
			SetWindowLong(hRichEdit, GWL_ID, IDC_ABOUT_RICHEDIT);
			bUseFriendlyLinks = true;
		}
	}
#endif /* MSFTEDIT_USE_41 */

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

	// Initialize the tab text.
	initCreditsTab();
	initLibrariesTab();
	initSupportTab();

	// Subclass the control.
	// TODO: Error handling?
	SetWindowSubclass(hRichEdit,
		LibWin32UI::MultiLineEditProc,
		IDC_ABOUT_RICHEDIT,
		reinterpret_cast<DWORD_PTR>(GetParent(hWndPropSheet)));

	// Remove the dummy tab.
	TabCtrl_DeleteItem(hTabControl, MAX_TABS);
	TabCtrl_SetCurSel(hTabControl, 0);
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
	const tstring tsTabTitle = U82T_c(C_("AboutTab", "About"));

	PROPSHEETPAGE psp;
	psp.dwSize = sizeof(psp);	
	psp.dwFlags = PSP_USECALLBACK | PSP_USETITLE | PSP_DLGINDIRECT;
	psp.hInstance = HINST_THISCOMPONENT;
	psp.pResource = LoadDialog_i18n(IDD_CONFIG_ABOUT);
	psp.pszIcon = nullptr;
	psp.pszTitle = tsTabTitle.c_str();
	psp.pfnDlgProc = AboutTabPrivate::dlgProc;
	psp.lParam = reinterpret_cast<LPARAM>(d);
	psp.pcRefParent = nullptr;
	psp.pfnCallback = AboutTabPrivate::callbackProc;

	d->hPropSheetPage = CreatePropertySheetPage(&psp);
	return d->hPropSheetPage;
}
