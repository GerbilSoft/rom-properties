/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * AboutTab.cpp: About tab for rp-config.                                  *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "config.librpbase.h"

#include "AboutTab.hpp"
#include "UpdateChecker.hpp"
#include "res/resource.h"

// Other rom-properties libraries
#include "librpbase/config/AboutTabText.hpp"
using namespace LibRpBase;
using namespace LibRpText;

// libwin32ui
#include "libwin32ui/SubclassWindow.h"

// Property sheet icon.
// Extracted from imageres.dll or shell32.dll.
#include "PropSheetIcon.hpp"

// C++ STL classes
using std::string;
using std::wstring;
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
// Reference: https://docs.microsoft.com/en-us/archive/blogs/murrays/richedit-colors
#define MSFTEDIT_USE_41 1

// Other libraries.
#ifdef HAVE_ZLIB
#  include <zlib.h>
#endif
#ifdef HAVE_PNG
#  include <png.h>	// PNG_LIBPNG_VER_STRING
#  include "librpbase/img/RpPng.hpp"
#endif
#ifdef ENABLE_XML
#  include "tinyxml2.h"
#endif

// Useful RTF strings.
// TODO: Custom color table for links is only needed on Win7 and earlier.
#define RTF_START "{\\rtf1\\ansi\\ansicpg1252\\deff0\\deflang1033\n"
#define RTF_COLOR_TABLE "{\\colortbl ;\\red0\\green102\\blue204;}\n"
#define RTF_BR "\\par\n"
#define RTF_TAB "\\tab "
#define RTF_BULLET "\\bullet "
#define RTF_ALIGN_LEFT "\\ql "
#define RTF_ALIGN_RIGHT "\\qr "
#define RTF_ALIGN_JUSTIFY "\\qj "
#define RTF_ALIGN_CENTER "\\qc "
#define RTF_BOLD_ON "\\b "
#define RTF_BOLD_OFF "\\b0 "

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

	public:
		/**
		 * Check for updates.
		 */
		void checkForUpdates(void);

		bool bCheckedForUpdates;	// Checked for updates yet?
		UpdateChecker *updChecker;

		/**
		 * An error occurred while trying to retrieve the update version.
		 * Called by the WM_UPD_ERROR handler.
		 * TODO: Error code?
		 */
		void updChecker_error();

		/**
		 * Update version retrieved.
		 * Called by the WM_UPD_RETRIEVED handler.
		 */
		void updChecker_retrieved(void);

	protected:
		// Current RichText streaming context.
		struct RTF_CTX {
			const string *str;
			size_t pos;
		};
		RTF_CTX rtfCtx_main;
		RTF_CTX rtfCtx_upd;

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

		/**
		 * Create an RTF "friendly link" if supported.
		 * If not supported, returns the escaped link title.
		 * @param link	[in] Link address.
		 * @param title	[in] Link title.
		 * @return RTF "friendly link", or title only.
		 */
		string rtfFriendlyLink(const char *link, const char *title);

	protected:
		// Tab text (RichText format)
		string sVersionLabel;	// UpdateCheck
		string sCredits;
		string sLibraries;
		string sSupport;

		// RichEdit controls
		HWND hRichEdit;		// Main RichEdit control
		HWND hUpdateCheck;	// UpdateCheck label

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
	, bCheckedForUpdates(false)
	, updChecker(nullptr)
	, hRichEdit(nullptr)
	, hUpdateCheck(nullptr)
{
	memset(&rtfCtx_main, 0, sizeof(rtfCtx_main));
	memset(&rtfCtx_upd, 0, sizeof(rtfCtx_upd));

	// Load the RichEdit DLLs.
	// TODO: What if this fails?
	hRichEd20_dll = LoadLibraryEx(_T("RICHED20.DLL"), NULL, LOAD_LIBRARY_SEARCH_SYSTEM32);
#ifdef MSFTEDIT_USE_41
	hMsftEdit_dll = LoadLibraryEx(_T("MSFTEDIT.DLL"), NULL, LOAD_LIBRARY_SEARCH_SYSTEM32);
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

	if (updChecker) {
		delete updChecker;
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

		case WM_SHOWWINDOW: {
			auto *const d = reinterpret_cast<AboutTabPrivate*>(GetWindowLongPtr(hDlg, GWLP_USERDATA));
			if (!d) {
				// No AboutTabPrivate. Can't do anything...
				return FALSE;
			}

			if (!d->bCheckedForUpdates) {
				// Check for updates.
				d->bCheckedForUpdates = true;
				d->checkForUpdates();
			}

			break;
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
					if (pHdr->idFrom == IDC_ABOUT_RICHEDIT ||
					    pHdr->idFrom == IDC_ABOUT_UPDATE_CHECK)
					{
						// Allow this one.
					} else {
						// Incorrect source control ID.
						break;
					}

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

		case WM_UPD_ERROR: {
			auto *const d = reinterpret_cast<AboutTabPrivate*>(GetWindowLongPtr(hDlg, GWLP_USERDATA));
			if (!d) {
				// No AboutTabPrivate. Can't do anything...
				return FALSE;
			}

			d->updChecker_error();
			return TRUE;
		}

		case WM_UPD_RETRIEVED: {
			auto *const d = reinterpret_cast<AboutTabPrivate*>(GetWindowLongPtr(hDlg, GWLP_USERDATA));
			if (!d) {
				// No AboutTabPrivate. Can't do anything...
				return FALSE;
			}

			d->updChecker_retrieved();
			return TRUE;
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
 * Check for updates.
 */
void AboutTabPrivate::checkForUpdates(void)
{
	sVersionLabel.clear();
	sVersionLabel.reserve(64);

	sVersionLabel = RTF_START RTF_ALIGN_RIGHT;
	sVersionLabel += rtfEscape(C_("AboutTab", "Checking for updates..."));

	// NOTE: EM_SETTEXTEX doesn't seem to work.
	// We'll need to stream in the text instead.
	// Reference: https://devblogs.microsoft.com/oldnewthing/20070110-13/?p=28463
	rtfCtx_upd.str = &sVersionLabel;
	rtfCtx_upd.pos = 0;
	EDITSTREAM es = { (DWORD_PTR)&rtfCtx_upd, 0, EditStreamCallback };
	SendMessage(hUpdateCheck, EM_STREAMIN, SF_RTF, (LPARAM)&es);

	if (!updChecker) {
		updChecker = new UpdateChecker();
	}
	if (!updChecker->run(hWndPropSheet)) {
		// Failed to run the Update Checker.
		sVersionLabel.clear();
		sVersionLabel.reserve(64);

		sVersionLabel = RTF_START RTF_ALIGN_RIGHT;
		sVersionLabel += rtfEscape(C_("AboutTab", "Update check failed!"));

		// NOTE: EM_SETTEXTEX doesn't seem to work.
		// We'll need to stream in the text instead.
		// Reference: https://devblogs.microsoft.com/oldnewthing/20070110-13/?p=28463
		rtfCtx_upd.str = &sVersionLabel;
		rtfCtx_upd.pos = 0;
		EDITSTREAM es = { (DWORD_PTR)&rtfCtx_upd, 0, EditStreamCallback };
		SendMessage(hUpdateCheck, EM_STREAMIN, SF_RTF, (LPARAM)&es);
	}
}

/**
 * An error occurred while trying to retrieve the update version.
 * Called by the WM_UPD_ERROR handler.
 * TODO: Error code?
 */
void AboutTabPrivate::updChecker_error(void)
{
	if (!updChecker) {
		// No Update Checker...
		return;
	}

	const char *const errorMessage = updChecker->errorMessage();

	sVersionLabel.clear();
	sVersionLabel.reserve(128);

	sVersionLabel = RTF_START RTF_ALIGN_RIGHT RTF_BOLD_ON;
	// tr: Error message template. (Windows version, without formatting)
	sVersionLabel += rtfEscape(C_("AboutTab", "ERROR:"));
	sVersionLabel += RTF_BOLD_OFF " ";
	sVersionLabel += rtfEscape(errorMessage);

	// NOTE: EM_SETTEXTEX doesn't seem to work.
	// We'll need to stream in the text instead.
	// Reference: https://devblogs.microsoft.com/oldnewthing/20070110-13/?p=28463
	rtfCtx_upd.str = &sVersionLabel;
	rtfCtx_upd.pos = 0;
	EDITSTREAM es = { (DWORD_PTR)&rtfCtx_upd, 0, EditStreamCallback };
	SendMessage(hUpdateCheck, EM_STREAMIN, SF_RTF, (LPARAM)&es);
}

/**
 * Update version retrieved.
 * Called by the WM_UPD_RETRIEVED handler.
 */
void AboutTabPrivate::updChecker_retrieved(void)
{
	if (!updChecker) {
		// No Update Checker...
		return;
	}
	const uint64_t updateVersion = updChecker->updateVersion();

	// Our version. (ignoring the development flag)
	const uint64_t ourVersion = RP_PROGRAM_VERSION_NO_DEVEL(AboutTabText::getProgramVersion());

	// Format the latest version string.
	char sUpdVersion[32];
	const unsigned int upd[3] = {
		RP_PROGRAM_VERSION_MAJOR(updateVersion),
		RP_PROGRAM_VERSION_MINOR(updateVersion),
		RP_PROGRAM_VERSION_REVISION(updateVersion)
	};

	if (upd[2] == 0) {
		snprintf(sUpdVersion, sizeof(sUpdVersion), "%u.%u", upd[0], upd[1]);
	} else {
		snprintf(sUpdVersion, sizeof(sUpdVersion), "%u.%u.%u", upd[0], upd[1], upd[2]);
	}

	sVersionLabel.clear();
	sVersionLabel.reserve(384);

	// RTF starting sequence
	sVersionLabel = RTF_START;
	sVersionLabel += RTF_COLOR_TABLE;	// TODO: Skip on Win8+?
	sVersionLabel += RTF_ALIGN_RIGHT;

	char s_latest_version[128];
	snprintf(s_latest_version, sizeof(s_latest_version), C_("AboutTab", "Latest version: %s"), sUpdVersion);
	sVersionLabel += rtfEscape(s_latest_version);
	if (updateVersion > ourVersion) {
		sVersionLabel += RTF_BR RTF_BR RTF_BOLD_ON;
		sVersionLabel += rtfEscape(C_("AboutTab", "New version available!"));
		sVersionLabel += RTF_BOLD_OFF RTF_BR;
		sVersionLabel += rtfFriendlyLink("https://github.com/GerbilSoft/rom-properties/releases", C_("AboutTab", "Download at GitHub"));
	}

	// NOTE: EM_SETTEXTEX doesn't seem to work.
	// We'll need to stream in the text instead.
	// Reference: https://devblogs.microsoft.com/oldnewthing/20070110-13/?p=28463
	rtfCtx_upd.str = &sVersionLabel;
	rtfCtx_upd.pos = 0;
	EDITSTREAM es = { (DWORD_PTR)&rtfCtx_upd, 0, EditStreamCallback };
	SendMessage(hUpdateCheck, EM_STREAMIN, SF_RTF, (LPARAM)&es);
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
string AboutTabPrivate::rtfEscape(const char *str)
{
	assert(str != nullptr);
	if (unlikely(!str)) {
		return string();
	}

	// Convert the string to UTF-16 first.
	const u16string u16str = utf8_to_utf16(str, -1);

	// RTF return string.
	string s_rtf;
	s_rtf.reserve(u16str.size() * 2);

	// Reference: http://www.zopatista.com/python/2012/06/06/rtf-and-unicode/
	char buf[12];	// Conversion buffer.
	for (const char16_t *p = u16str.c_str(); *p != 0; p++) {
		if (*p == '\n') {
			// Newline
			s_rtf += RTF_BR;
		} else if (*p <= 0x00FF) {
			// cp1252 is a superset of ISO-8859-1.
			s_rtf += (char)*p;
		} else {
			// Convert to a signed 16-bit integer.
			// Surrogate pairs are encoded as two separate characters.
			snprintf(buf, sizeof(buf), "\\u%d?", (int16_t)*p);
			s_rtf += buf;
		}
	}

	return s_rtf;
}

/**
 * Create an RTF "friendly link" if supported.
 * If not supported, returns the escaped link title.
 * @param link	[in] Link address.
 * @param title	[in] Link title.
 * @return RTF "friendly link", or title only.
 */
string AboutTabPrivate::rtfFriendlyLink(const char *link, const char *title)
{
	assert(link != nullptr);
	assert(title != nullptr);

	// Color table reference:
	// - https://stackoverflow.com/questions/42251000/inno-setup-how-to-change-the-color-of-the-hyperlink-in-rtf-text
	// - https://stackoverflow.com/a/42273986

	if (bUseFriendlyLinks) {
		// Friendly links are available.
		// Reference: https://docs.microsoft.com/en-us/archive/blogs/murrays/richedit-friendly-name-hyperlinks
		// TODO: Get the "proper" link color.
		// TODO: Don't include cf1/cf0 on Win8+?
		return rp_sprintf("{\\field{\\*\\fldinst{HYPERLINK \"%s\"}}{\\fldrslt{\\cf1\\ul %s\\ul0\\cf0 }}}",
			rtfEscape(link).c_str(), rtfEscape(title).c_str());
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
	// FIXME: ru/uk have the program title as the second line.
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

		// Reserve some space for the update check label.
		const int rightPos = 96;

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
				winRect.right - leftPos - rightPos - dlgMargin.left,
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

	// RTF starting sequence
	sCredits = RTF_START;
	sCredits += RTF_COLOR_TABLE;	// TODO: Skip on Win8+?

	// FIXME: Figure out how to get links to work without
	// resorting to manually adding CFE_LINK data...
	// NOTE: Copyright is NOT localized.
	sCredits += AboutTabText::getProgramInfoString(AboutTabText::ProgramInfoStringID::Copyright);
	sCredits += RTF_BR RTF_BR;
	sCredits += rp_sprintf(
		// tr: %s is the name of the license.
		rtfEscape(C_("AboutTab|Credits", "This program is licensed under the %s or later.")).c_str(),
			rtfFriendlyLink(
				"https://www.gnu.org/licenses/gpl-2.0.html",
				C_("AboutTabl|Credits", "GNU GPL v2")).c_str());
	if (!bUseFriendlyLinks) {
		sCredits += RTF_BR
			"https://www.gnu.org/licenses/gpl-2.0.html";
	}

	AboutTabText::CreditType lastCreditType = AboutTabText::CreditType::Continue;
	for (const AboutTabText::CreditsData_t *creditsData = AboutTabText::getCreditsData();
	     creditsData->type < AboutTabText::CreditType::Max; creditsData++)
	{
		if (creditsData->type != AboutTabText::CreditType::Continue &&
		    creditsData->type != lastCreditType)
		{
			// New credit type.
			sCredits += RTF_BR RTF_BR RTF_BOLD_ON;

			switch (creditsData->type) {
				case AboutTabText::CreditType::Developer:
					sCredits += rtfEscape(C_("AboutTab|Credits", "Developers:"));
					break;
				case AboutTabText::CreditType::Contributor:
					sCredits += rtfEscape(C_("AboutTab|Credits", "Contributors:"));
					break;
				case AboutTabText::CreditType::Translator:
					sCredits += rtfEscape(C_("AboutTab|Credits", "Translators:"));
					break;

				case AboutTabText::CreditType::Continue:
				case AboutTabText::CreditType::Max:
				default:
					assert(!"Invalid credit type.");
					break;
			}

			sCredits += RTF_BOLD_OFF;
		}

		// Append the contributor's name.
		sCredits += RTF_BR RTF_TAB RTF_BULLET " ";
		sCredits += rtfEscape(creditsData->name);
		if (creditsData->url) {
			// FIXME: Figure out how to get hyperlinks working.
			sCredits += " <";
			if (creditsData->linkText) {
				sCredits += rtfFriendlyLink(creditsData->url, creditsData->linkText);
			} else {
				sCredits += rtfFriendlyLink(creditsData->url, creditsData->url);
			}
			sCredits += '>';
		}
		if (creditsData->sub) {
			// Sub-credit.
			sCredits += rp_sprintf(rtfEscape(C_("AboutTab|Credits", " (%s)")).c_str(),
				rtfEscape(creditsData->sub).c_str());
		}

		lastCreditType = creditsData->type;
	}

	sCredits += '}';

	// Add the "Credits" tab.
	const tstring tsTabTitle = U82T_c(C_("AboutTab", "Credits"));
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

	// RTF starting sequence
	sLibraries = RTF_START;

	// NOTE: These strings can NOT be static.
	// Otherwise, they won't be retranslated if the UI language
	// is changed at runtime.

	// tr: Using an internal copy of a library.
	const string sIntCopyOf = rtfEscape(C_("AboutTab|Libraries", "Internal copy of %s."));
	// tr: Compiled with a specific version of an external library.
	const string sCompiledWith = rtfEscape(C_("AboutTab|Libraries", "Compiled with %s."));
	// tr: Using an external library, e.g. libpcre.so
	const string sUsingDll = rtfEscape(C_("AboutTab|Libraries", "Using %s."));
	// tr: License: (libraries with only a single license)
	const string sLicense = rtfEscape(C_("AboutTab|Libraries", "License: %s"));
	// tr: Licenses: (libraries with multiple licenses)
	const string sLicenses = rtfEscape(C_("AboutTab|Libraries", "Licenses: %s"));

	// NOTE: We're only showing the "compiled with" version here,
	// since the DLLs are delay-loaded and might not be available.

	/** zlib **/
#ifdef HAVE_ZLIB
	const bool zlib_is_ng = RpPng::zlib_is_ng();
	string sZlibVersion = (zlib_is_ng ? "zlib-ng " : "zlib ");
	sZlibVersion += RpPng::zlib_version_string();

#if defined(USE_INTERNAL_ZLIB) && !defined(USE_INTERNAL_ZLIB_DLL)
	sLibraries += rp_sprintf(sIntCopyOf.c_str(), sZlibVersion.c_str());
#else
#  ifdef ZLIBNG_VERSION
	sLibraries += rp_sprintf(sCompiledWith.c_str(), "zlib-ng " ZLIBNG_VERSION);
#  else /* !ZLIBNG_VERSION */
	sLibraries += rp_sprintf(sCompiledWith.c_str(), "zlib " ZLIB_VERSION);
#  endif /* ZLIBNG_VERSION */
	sLibraries += RTF_BR;
	sLibraries += rp_sprintf(sUsingDll.c_str(), sZlibVersion.c_str());
#endif
	sLibraries += RTF_BR
		"Copyright (C) 1995-2022 Jean-loup Gailly and Mark Adler." RTF_BR
		"https://zlib.net/" RTF_BR;
#  ifdef ZLIBNG_VERSION
	// TODO: Also if zlibVersion() contains "zlib-ng"?
	sLibraries += "https://github.com/zlib-ng/zlib-ng" RTF_BR;
#  endif /* ZLIBNG_VERSION */
	sLibraries += rp_sprintf(sLicense.c_str(), "zlib license");
#endif /* HAVE_ZLIB */

	/** libpng **/
	// FIXME: Use png_get_copyright().
	// FIXME: Check for APNG.
#ifdef HAVE_PNG
	const bool APNG_is_supported = RpPng::libpng_has_APNG();
	const uint32_t png_version_number = RpPng::libpng_version_number();
	char pngVersion[48];
	snprintf(pngVersion, sizeof(pngVersion), "libpng %u.%u.%u%s",
		png_version_number / 10000,
		(png_version_number / 100) % 100,
		png_version_number % 100,
		(APNG_is_supported ? " + APNG" : " (No APNG support)"));

	sLibraries += RTF_BR RTF_BR;
#if defined(USE_INTERNAL_PNG) && !defined(USE_INTERNAL_ZLIB_DLL)
	sLibraries += rp_sprintf(sIntCopyOf.c_str(), pngVersion);
#else
	// NOTE: Gentoo's libpng has "+apng" at the end of
	// PNG_LIBPNG_VER_STRING if APNG is enabled.
	// We have our own "+ APNG", so remove Gentoo's.
	string pngVersionCompiled = "libpng " PNG_LIBPNG_VER_STRING;
	for (size_t i = pngVersionCompiled.size()-1; i > 6; i--) {
		char chr = pngVersionCompiled[i];
		if (ISDIGIT(chr))
			break;
		pngVersionCompiled.resize(i);
	}

	string fullPngVersionCompiled;
	if (APNG_is_supported) {
		// PNG version, with APNG support.
		fullPngVersionCompiled = rp_sprintf("%s + APNG", pngVersionCompiled.c_str());
	} else {
		// PNG version, without APNG support.
		fullPngVersionCompiled = rp_sprintf("%s (No APNG support)", pngVersionCompiled.c_str());
	}

	sLibraries += rp_sprintf(sCompiledWith.c_str(), fullPngVersionCompiled.c_str());
	sLibraries += RTF_BR;
	sLibraries += rp_sprintf(sUsingDll.c_str(), pngVersion);
#endif

	/**
	 * NOTE: MSVC does not define __STDC__ by default.
	 * If __STDC__ is not defined, the libpng copyright
	 * will not have a leading newline, and all newlines
	 * will be replaced with groups of 6 spaces.
	 *
	 * NOTE 2: This was changed in libpng-1.6.36, so it
	 * always returns the __STDC__ copyright notice.
	 */
	// On Windows, assume we're using our own libpng,
	// which always uses the __STDC__ copyright notice.
	sLibraries += rtfEscape(RpPng::libpng_copyright_string());
	sLibraries +=
		"http://www.libpng.org/pub/png/libpng.html" RTF_BR
		"https://github.com/glennrp/libpng" RTF_BR;
	if (APNG_is_supported) {
		sLibraries += rtfEscape(C_("AboutTab|Libraries", "APNG patch:"));
		sLibraries += " https://sourceforge.net/projects/libpng-apng/" RTF_BR;
	}
	sLibraries += rp_sprintf(sLicense.c_str(), "libpng license");
#endif /* HAVE_PNG */

	/** TinyXML2 **/
#ifdef ENABLE_XML
	snprintf(sVerBuf, sizeof(sVerBuf), "TinyXML2 %u.%u.%u",
		static_cast<unsigned int>(TIXML2_MAJOR_VERSION),
		static_cast<unsigned int>(TIXML2_MINOR_VERSION),
		static_cast<unsigned int>(TIXML2_PATCH_VERSION));

	// FIXME: Runtime version?
	sLibraries += RTF_BR RTF_BR;
	sLibraries += rp_sprintf(sCompiledWith.c_str(), sVerBuf);
	sLibraries += RTF_BR
		"Copyright (C) 2000-2021 Lee Thomason" RTF_BR
		"http://www.grinninglizard.com/" RTF_BR;
	sLibraries += rp_sprintf(sLicense.c_str(), "zlib license");
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
	sLibraries += rp_sprintf(sIntCopyOf.c_str(), sVerBuf);
#  else /* _WIN32 */
	// FIXME: Runtime version?
	sLibraries += rp_sprintf(sCompiledWith.c_str(), sVerBuf);
#  endif /* _WIN32 */
	sLibraries += RTF_BR
		"Copyright (C) 1995-1997, 2000-2016, 2018-2020 Free Software Foundation, Inc." RTF_BR
		"https://www.gnu.org/software/gettext/" RTF_BR;
	sLibraries += rp_sprintf(sLicense.c_str(), "GNU LGPL v2.1+");
#endif /* HAVE_GETTEXT && LIBINTL_VERSION */

	sLibraries += '}';

	// Add the "Libraries" tab.
	const tstring tsTabTitle = U82T_c(C_("AboutTab", "Libraries"));
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

	// RTF starting sequence
	sSupport = RTF_START;
	sSupport += RTF_COLOR_TABLE;	// TODO: Skip on Win8+?

	sSupport += rtfEscape(C_("AboutTab|Support",
		"For technical support, you can visit the following websites:"));
	sSupport += RTF_BR;

	for (const AboutTabText::SupportSite_t *supportSite = AboutTabText::getSupportSites();
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
	sSupport += rtfEscape(C_("AboutTab|Support", "You can also email the developer directly:"));
	sSupport += RTF_BR RTF_TAB RTF_BULLET " David Korth <";
	sSupport += rtfFriendlyLink("mailto:gerbilsoft@gerbilsoft.com", "gerbilsoft@gerbilsoft.com");
	sSupport += ">}";

	// Add the "Support" tab.
	const tstring tsTabTitle = U82T_c(C_("AboutTab", "Support"));
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
			rtfCtx_main.str = &sCredits;
			break;
		case 1:
			rtfCtx_main.str = &sLibraries;
			break;
		case 2:
			rtfCtx_main.str = &sSupport;
			break;
		default:
			// Should not get here...
			assert(!"Invalid tab index.");
			return;
	}
	rtfCtx_main.pos = 0;
	EDITSTREAM es = { (DWORD_PTR)&rtfCtx_main, 0, EditStreamCallback };
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
	hUpdateCheck = GetDlgItem(hWndPropSheet, IDC_ABOUT_UPDATE_CHECK);
	assert(hRichEdit != nullptr);
	assert(hUpdateCheck != nullptr);
	if (unlikely(!hRichEdit || !hUpdateCheck)) {
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

		// IDC_ABOUT_UPDATE_CHECK
		RECT rectUpdateCheck;
		GetClientRect(hUpdateCheck, &rectUpdateCheck);
		MapWindowPoints(hUpdateCheck, hWndPropSheet, (LPPOINT)&rectUpdateCheck, 2);

		HWND hUpdateCheck41 = CreateWindowEx(
			WS_EX_NOPARENTNOTIFY | WS_EX_TRANSPARENT | WS_EX_RIGHT,
			MSFTEDIT_CLASS, _T(""),
			ES_MULTILINE | ES_READONLY | WS_VISIBLE | WS_CHILD,
			rectUpdateCheck.left, rectUpdateCheck.top,
			rectUpdateCheck.right - rectUpdateCheck.left,
			rectUpdateCheck.bottom - rectUpdateCheck.top,
			hWndPropSheet, nullptr, nullptr, nullptr);
		if (hUpdateCheck41) {
			DestroyWindow(hUpdateCheck);
			SetWindowFont(hUpdateCheck41, GetWindowFont(hWndPropSheet), FALSE);
			hUpdateCheck = hUpdateCheck41;
			// FIXME: Not working...
			SetWindowLong(hUpdateCheck41, GWL_ID, IDC_ABOUT_UPDATE_CHECK);
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

	// RichEdit adjustments for IDC_ABOUT_UPDATE_CHECK.
	// Enable links.
	eventMask = SendMessage(hUpdateCheck, EM_GETEVENTMASK, 0, 0);
	SendMessage(hUpdateCheck, EM_SETEVENTMASK, 0, (LPARAM)(eventMask | ENM_LINK));
	SendMessage(hUpdateCheck, EM_AUTOURLDETECT, AURL_ENABLEURL, 0);

	// Initialize the tab text.
	initCreditsTab();
	initLibrariesTab();
	initSupportTab();

	// Subclass the RichEdit controls.
	// TODO: Error handling?
	SetWindowSubclass(hRichEdit,
		LibWin32UI::MultiLineEditProc,
		IDC_ABOUT_RICHEDIT,
		reinterpret_cast<DWORD_PTR>(GetParent(hWndPropSheet)));
	SetWindowSubclass(hUpdateCheck,
		LibWin32UI::MultiLineEditProc,
		IDC_ABOUT_UPDATE_CHECK,
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
