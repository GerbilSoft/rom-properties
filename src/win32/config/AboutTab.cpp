/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * AboutTab.cpp: About tab for rp-config.                                  *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "config.librpbase.h"
#include "config.win32.h"

#include "AboutTab.hpp"
#include "res/resource.h"

#ifdef ENABLE_NETWORKING
#  include "UpdateChecker.hpp"
#endif /* ENABLE_NETWORKING */

// Other rom-properties libraries
#include "librpbase/config/AboutTabText.hpp"
using namespace LibRpBase;
using namespace LibRpText;

// Property sheet icon.
// Extracted from imageres.dll or shell32.dll.
#include "PropSheetIcon.hpp"

// libwin32ui
#include "libwin32ui/LoadResource_i18n.hpp"
using LibWin32UI::LoadDialog_i18n;

// Win32 dark mode
#include "libwin32darkmode/DarkMode.hpp"
#include "libwin32darkmode/DarkModeCtrl.hpp"

// C++ STL classes
using std::array;
using std::string;
using std::wstring;
using std::u16string;
using std::unique_ptr;

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

/** Libraries **/

// zlib and libpng
// TODO: Make ZLIBNG_VERSION and ZLIB_VERSION accessible via RpPng.
// TODO: Make PNG_LIBPNG_VER_STRING accessible via RpPng.
#include <zlib.h>
#include <png.h>
#include "librpbase/img/RpPng.hpp"
// TODO: JPEG
#ifdef ENABLE_XML
#  include <pugixml.hpp>
#endif

// Useful RTF strings.
#define RTF_START "{\\rtf1\\ansi\\ansicpg1252\\deff0\\deflang1033\n"
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
	// Property sheet
	HPROPSHEETPAGE hPropSheetPage;
	HWND hWndPropSheet;

	// Bold font
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

#ifdef ENABLE_NETWORKING
public:
	/**
	 * Check for updates.
	 */
	void checkForUpdates(void);

	bool bCheckedForUpdates;	// Checked for updates yet?
	unique_ptr<UpdateChecker> updChecker;

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
#endif /* ENABLE_NETWORKING */

protected:
	// Current RichText streaming context
	struct RTF_CTX {
		const string *str;
		size_t pos;
	};
	RTF_CTX rtfCtx_main;
	RTF_CTX rtfCtx_upd;

	/**
	 * RTF EditStream callback
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
	string sCredits;
	string sLibraries;
	string sSupport;

#ifdef ENABLE_NETWORKING
	HWND hUpdateCheck;	// UpdateCheck label
	string sVersionLabel;	// Version string
#endif /* ENABLE_NETWORKING */

	// Main RichEdit control
	HWND hRichEdit;

	/**
	 * Initialize the RTF color table.
	 * @return RTF color table
	 */
	string initRtfColorTable(void);

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

	/**
	 * Update RTF color tables in existing RTF strings for the system theme.
	 */
	void updateRtfColorTablesInRtfStrings(void);

public:
	/**
	 * Initialize the dialog.
	 */
	void initDialog(void);

public:
	// Dark Mode background brush
	HBRUSH hbrBkgnd;
	bool lastDarkModeEnabled;
};

/** AboutTabPrivate **/

AboutTabPrivate::AboutTabPrivate()
	: hPropSheetPage(nullptr)
	, hWndPropSheet(nullptr)
	, hFontBold(nullptr)
	, bUseFriendlyLinks(false)
#ifdef ENABLE_NETWORKING
	, bCheckedForUpdates(false)
	, hUpdateCheck(nullptr)
#endif /* ENABLE_NETWORKING */
	, hRichEdit(nullptr)
	, hbrBkgnd(nullptr)
	, lastDarkModeEnabled(false)
{
	memset(&rtfCtx_main, 0, sizeof(rtfCtx_main));
	memset(&rtfCtx_upd, 0, sizeof(rtfCtx_upd));

	// Load the RichEdit DLLs.
	// TODO: What if this fails?
	hRichEd20_dll = LoadLibraryEx(_T("riched20.dll"), nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
#ifdef MSFTEDIT_USE_41
	hMsftEdit_dll = LoadLibraryEx(_T("msftedit.dll"), nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
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

	// Dark mode background brush
	if (hbrBkgnd) {
		DeleteBrush(hbrBkgnd);
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

			// NOTE: This should be in WM_CREATE, but we don't receive WM_CREATE here.
			DarkMode_InitDialog(hDlg);
			d->lastDarkModeEnabled = g_darkModeEnabled;

			// Initialize the dialog.
			d->initDialog();
			return TRUE;
		}

#ifdef ENABLE_NETWORKING
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
#endif /* ENABLE_NETWORKING */

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
					break;
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
						const int index = TabCtrl_GetCurSel(hTabControl);
						d->setTabContents(index);
					}
					break;
				}

				default:
					break;
			}
			break;
		}

#ifdef ENABLE_NETWORKING
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
#endif /* ENABLE_NETWORKING */

		/** Dark Mode **/

		case WM_CTLCOLORDLG:
		case WM_CTLCOLORSTATIC:
			if (g_darkModeEnabled) {
				auto *const d = reinterpret_cast<AboutTabPrivate*>(GetWindowLongPtr(hDlg, GWLP_USERDATA));
				if (!d) {
					// No AboutTabPrivate. Can't do anything...
					return FALSE;
				}

				HDC hdc = reinterpret_cast<HDC>(wParam);
				SetTextColor(hdc, g_darkTextColor);
				SetBkColor(hdc, g_darkSubDlgBkColor);
				if (!d->hbrBkgnd) {
					d->hbrBkgnd = CreateSolidBrush(g_darkSubDlgBkColor);
				}
				return reinterpret_cast<INT_PTR>(d->hbrBkgnd);
			}
			break;

		case WM_SETTINGCHANGE:
			if (g_darkModeSupported && IsColorSchemeChangeMessage(lParam)) {
				SendMessage(hDlg, WM_THEMECHANGED, 0, 0);
			}
			break;

		case WM_THEMECHANGED: {
			if (g_darkModeSupported) {
				auto *const d = reinterpret_cast<AboutTabPrivate*>(GetWindowLongPtr(hDlg, GWLP_USERDATA));
				if (!d) {
					// No ImageTypesTabPrivate. Can't do anything...
					return FALSE;
				}

				UpdateDarkModeEnabled();
				if (d->lastDarkModeEnabled != g_darkModeEnabled) {
					d->lastDarkModeEnabled = g_darkModeEnabled;

					// Tab control isn't getting WM_THEMECHANGED, even though it should...
					HWND hTabControl = GetDlgItem(hDlg, IDC_ABOUT_TABCONTROL);
					SendMessage(hTabControl, WM_THEMECHANGED, 0, 0);

					// RichEdit doesn't support dark mode per se, but we can
					// adjust its background and text colors.

					// Set the RichEdit colors.
#ifdef ENABLE_NETWORKING
					DarkMode_InitRichEdit(d->hUpdateCheck);
#endif /* ENABLE_NETWORKING */
					DarkMode_InitRichEdit(d->hRichEdit);

					// Update the RTF color tables.
					// This fixes text colors on Win10 21H2. (On 1809, it worked without this.)
					d->updateRtfColorTablesInRtfStrings();
				}
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

#ifdef ENABLE_NETWORKING
/**
 * Check for updates.
 */
void AboutTabPrivate::checkForUpdates(void)
{
	sVersionLabel.clear();
	sVersionLabel.reserve(64);
	sVersionLabel = RTF_START;
	sVersionLabel += initRtfColorTable();

	sVersionLabel += RTF_ALIGN_RIGHT;
	sVersionLabel += rtfEscape(C_("AboutTab", "Checking for updates..."));

	// NOTE: EM_SETTEXTEX doesn't seem to work.
	// We'll need to stream in the text instead.
	// Reference: https://devblogs.microsoft.com/oldnewthing/20070110-13/?p=28463
	rtfCtx_upd.str = &sVersionLabel;
	rtfCtx_upd.pos = 0;
	EDITSTREAM es = { (DWORD_PTR)&rtfCtx_upd, 0, EditStreamCallback };
	SendMessage(hUpdateCheck, EM_STREAMIN, SF_RTF, (LPARAM)&es);

	if (!updChecker) {
		updChecker.reset(new UpdateChecker());
	}
	if (!updChecker->run(hWndPropSheet)) {
		// Failed to run the Update Checker.
		sVersionLabel.clear();
		sVersionLabel.reserve(64);
		sVersionLabel = RTF_START;
		sVersionLabel += initRtfColorTable();

		sVersionLabel += RTF_ALIGN_RIGHT;
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
	sVersionLabel = RTF_START;
	sVersionLabel += initRtfColorTable();

	sVersionLabel += RTF_ALIGN_RIGHT RTF_BOLD_ON;
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
	string sUpdVersion;
	const array<unsigned int, 3> upd = {{
		RP_PROGRAM_VERSION_MAJOR(updateVersion),
		RP_PROGRAM_VERSION_MINOR(updateVersion),
		RP_PROGRAM_VERSION_REVISION(updateVersion)
	}};

	if (upd[2] == 0) {
		sUpdVersion = fmt::format(FSTR("{:d}.{:d}"), upd[0], upd[1]);
	} else {
		sUpdVersion = fmt::format(FSTR("{:d}.{:d}.{:d}"), upd[0], upd[1], upd[2]);
	}

	sVersionLabel.clear();
	sVersionLabel.reserve(384);
	sVersionLabel = RTF_START;
	sVersionLabel += initRtfColorTable();

	sVersionLabel += RTF_ALIGN_RIGHT;

	const string s_latest_version = fmt::format(FRUN(C_("AboutTab", "Latest version: {:s}")), sUpdVersion);
	sVersionLabel += rtfEscape(s_latest_version.c_str());
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
#endif /* ENABLE_NETWORKING */

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
		return {};
	}

	// Convert the string to UTF-16 first.
	const u16string u16str = utf8_to_utf16(str, -1);

	// RTF return string.
	string s_rtf;
	s_rtf.reserve(u16str.size() * 2);

	// Reference: http://www.zopatista.com/python/2012/06/06/rtf-and-unicode/
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
			s_rtf += fmt::format(FSTR("\\u{:d}?"), (int16_t)*p);
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
		// TODO: Don't include cf2/cf1 on Win8+?
		return fmt::format(FSTR("{{\\field{{\\*\\fldinst{{HYPERLINK \"{:s}\"}}}}{{\\fldrslt{{\\cf2\\ul {:s}\\ul0\\cf1 }}}}}}"),
			rtfEscape(link), rtfEscape(title));
	} else {
		// No friendly links.
		return rtfEscape(title);
	}
}

/**
 * Initialize the RTF color table.
 * @return RTF color table
 */
string AboutTabPrivate::initRtfColorTable(void)
{
	union COLORREF_t {
		struct {
			uint8_t r;
			uint8_t g;
			uint8_t b;
			uint8_t a;
		};
		COLORREF color;
	};

	COLORREF_t ctext, clink;
	if (g_darkModeEnabled) {
		ctext.color = g_darkTextColor;
		clink.color = 0x00CC6600;	// TODO: Dark mode version.
	} else {
		ctext.color = GetSysColor(COLOR_WINDOWTEXT);
		clink.color = 0x00CC6600;
	}

	// Color table:
	// - cf1: Main text color (COLOR_WINDOWTEXT in light mode, g_darkTextColor in dark mode)
	// - cf2: Link color (Technically not needed on Win8+...)
	// The color table may be automatically updated on theme change.
	return fmt::format(FSTR("{{\\colortbl ;\\red{:0>3d}\\green{:0>3d}\\blue{:0>3d};\\red{:0>3d}\\green{:0>3d}\\blue{:0>3d};}}\n\\cf1"),
		ctext.r, ctext.g, ctext.b,
		clink.r, clink.g, clink.b);
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
	// NOTE: ru/uk have the program title as the second line.
	// The resource IDs are reversed to compensate for this.
	assert(hFontBold != nullptr);
	if (unlikely(hFontBold)) {
		SetWindowFont(hStaticLine1, hFontBold, FALSE);
	}

	// Version number.
	const char *const programVersion =
		AboutTabText::getProgramInfoString(AboutTabText::ProgramInfoStringID::ProgramVersion);
	const char *const gitVersion =
		AboutTabText::getProgramInfoString(AboutTabText::ProgramInfoStringID::GitVersion);

	string s_version = fmt::format(FRUN(C_("AboutTab", "Version {:s}")), programVersion);
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
		SetWindowPos(hStaticVersion, nullptr, 0, 0,
			rectStaticVersion.right - rectStaticVersion.left, sz_hStaticVersion.cy,
			SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_NOMOVE);

		// Also adjust the update check static control.
		HWND hStaticUpdateCheck = GetDlgItem(hWndPropSheet, IDC_ABOUT_UPDATE_CHECK);
		assert(hStaticUpdateCheck != nullptr);
		if (hStaticUpdateCheck) {
			RECT rectStaticUpdateCheck;
			GetWindowRect(hStaticUpdateCheck, &rectStaticUpdateCheck);
			SetWindowPos(hStaticUpdateCheck, nullptr, 0, 0,
				rectStaticUpdateCheck.right - rectStaticUpdateCheck.left, sz_hStaticVersion.cy,
				SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_NOMOVE);
		}
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
		SetWindowPos(hStaticIcon, nullptr,
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
			SetWindowPos(hLabel, nullptr,
				leftPos, rect_label.top,
				winRect.right - leftPos - rightPos - dlgMargin.left,
				rect_label.bottom - rect_label.top,
				SWP_NOZORDER | SWP_NOOWNERZORDER);
		}

		// Adjust the tab control.
		SetWindowPos(hTabControl, nullptr,
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
	sCredits = RTF_START;
	sCredits += initRtfColorTable();

	// FIXME: Figure out how to get links to work without
	// resorting to manually adding CFE_LINK data...
	// NOTE: Copyright is NOT localized.
	sCredits += AboutTabText::getProgramInfoString(AboutTabText::ProgramInfoStringID::Copyright);
	sCredits += RTF_BR RTF_BR;
	sCredits += fmt::format(
		// tr: {:s} is the name of the license.
		FRUN(rtfEscape(C_("AboutTab|Credits", "This program is licensed under the {:s} or later."))),
			rtfFriendlyLink(
				"https://www.gnu.org/licenses/gpl-2.0.html",
				C_("AboutTabl|Credits", "GNU GPL v2")));
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
			// New credit type
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
			// tr: Sub-credit
			sCredits += fmt::format(FRUN(rtfEscape(C_("AboutTab|Credits", " ({:s})"))),
				rtfEscape(creditsData->sub));
		}

		lastCreditType = creditsData->type;
	}

	sCredits += '}';

	// Add the "Credits" tab.
	const tstring tsTabTitle = TC_("AboutTab", "Credits");
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
	sLibraries.clear();
	sLibraries.reserve(8192);
	sLibraries = RTF_START;
	sLibraries += initRtfColorTable();

	// NOTE: These strings can NOT be static.
	// Otherwise, they won't be retranslated if the UI language
	// is changed at runtime.

	// tr: Using an internal copy of a library.
	const string sIntCopyOf = rtfEscape(C_("AboutTab|Libraries", "Internal copy of {:s}."));
	// tr: Compiled with a specific version of an external library.
	const string sCompiledWith = rtfEscape(C_("AboutTab|Libraries", "Compiled with {:s}."));
	// tr: Using an external library, e.g. libpcre.so
	const string sUsingDll = rtfEscape(C_("AboutTab|Libraries", "Using {:s}."));
	// tr: License: (libraries with only a single license)
	const string sLicense = rtfEscape(C_("AboutTab|Libraries", "License: {:s}"));
	// tr: Licenses: (libraries with multiple licenses)
	const string sLicenses = rtfEscape(C_("AboutTab|Libraries", "Licenses: {:s}"));

	// NOTE: We're only showing the "compiled with" version here,
	// since the DLLs are delay-loaded and might not be available.

	/** zlib **/
#ifdef HAVE_ZLIB
	const bool zlib_is_ng = RpPng::zlib_is_ng();
	string sZlibVersion = (zlib_is_ng ? "zlib-ng " : "zlib ");
	sZlibVersion += RpPng::zlib_version_string();

#if defined(USE_INTERNAL_ZLIB) && !defined(USE_INTERNAL_ZLIB_DLL)
	sLibraries += fmt::format(FRUN(sIntCopyOf), sZlibVersion);
#else
#  ifdef ZLIBNG_VERSION
	sLibraries += fmt::format(FRUN(sCompiledWith), "zlib-ng " ZLIBNG_VERSION);
#  else /* !ZLIBNG_VERSION */
	sLibraries += fmt::format(FRUN(sCompiledWith), "zlib " ZLIB_VERSION);
#  endif /* ZLIBNG_VERSION */
	sLibraries += RTF_BR;
	sLibraries += fmt::format(FRUN(sUsingDll), sZlibVersion);
#endif
	sLibraries += RTF_BR
		"Copyright (C) 1995-2022 Jean-loup Gailly and Mark Adler." RTF_BR
		"https://zlib.net/" RTF_BR;
#  ifdef ZLIBNG_VERSION
	// TODO: Also if zlibVersion() contains "zlib-ng"?
	sLibraries += "https://github.com/zlib-ng/zlib-ng" RTF_BR;
#  endif /* ZLIBNG_VERSION */
	sLibraries += fmt::format(FRUN(sLicense), "zlib license");
#endif /* HAVE_ZLIB */

	/** libpng **/
	// FIXME: Use png_get_copyright().
	// FIXME: Check for APNG.
#ifdef HAVE_PNG
	const bool APNG_is_supported = RpPng::libpng_has_APNG();
	const uint32_t png_version_number = RpPng::libpng_version_number();
	const string pngVersion = fmt::format(FSTR("libpng {:d}.{:d}.{:d}{:s}"),
		png_version_number / 10000,
		(png_version_number / 100) % 100,
		png_version_number % 100,
		(APNG_is_supported ? " + APNG" : " (No APNG support)"));

	sLibraries += RTF_BR RTF_BR;
#if defined(USE_INTERNAL_PNG) && !defined(USE_INTERNAL_ZLIB_DLL)
	sLibraries += fmt::format(FRUN(sIntCopyOf), pngVersion);
#else
	// NOTE: Gentoo's libpng has "+apng" at the end of
	// PNG_LIBPNG_VER_STRING if APNG is enabled.
	// We have our own "+ APNG", so remove Gentoo's.
	string pngVersionCompiled = "libpng " PNG_LIBPNG_VER_STRING;
	for (size_t i = pngVersionCompiled.size()-1; i > 6; i--) {
		const char chr = pngVersionCompiled[i];
		if (ISDIGIT(chr))
			break;
		pngVersionCompiled.resize(i);
	}

	string fullPngVersionCompiled;
	if (APNG_is_supported) {
		// PNG version, with APNG support.
		fullPngVersionCompiled = fmt::format(FSTR("{:s} + APNG"), pngVersionCompiled);
	} else {
		// PNG version, without APNG support.
		fullPngVersionCompiled = fmt::format(FSTR("{:s} (No APNG support)"), pngVersionCompiled);
	}

	sLibraries += fmt::format(FRUN(sCompiledWith), fullPngVersionCompiled);
	sLibraries += RTF_BR;
	sLibraries += fmt::format(FRUN(sUsingDll), pngVersion);
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
	sLibraries += fmt::format(FRUN(sLicense), "libpng license");
#endif /* HAVE_PNG */

	/** PugiXML **/
#ifdef ENABLE_XML
	// PugiXML 1.10 and later uses this format: 1140 == 1.14.0
	// PugiXML 1.9 and earlier uses this format: 190 == 1.9.0
	unsigned int pugixml_major, pugixml_minor, pugixml_patch;
	if (PUGIXML_VERSION >= 1000) {
		pugixml_major = PUGIXML_VERSION / 1000;
		pugixml_minor = (PUGIXML_VERSION % 1000) / 10;
		pugixml_patch = PUGIXML_VERSION  % 10;
	} else {
		pugixml_major = PUGIXML_VERSION / 100;
		pugixml_minor = (PUGIXML_VERSION % 100) / 10;
		pugixml_patch = PUGIXML_VERSION % 10;
	}

	string pugiXmlVersion = fmt::format(FSTR("PugiXML {:d}.{:d}"),
		pugixml_major, pugixml_minor);
	if (pugixml_patch > 0) {
		pugiXmlVersion += fmt::format(FSTR(".{:d}"), pugixml_patch);
	}

	// FIXME: Runtime version?
	sLibraries += RTF_BR RTF_BR;
	sLibraries += fmt::format(FRUN(sCompiledWith), pugiXmlVersion);
	sLibraries += RTF_BR
		"Copyright (C) 2006-2025, by Arseny Kapoulkine (arseny.kapoulkine@gmail.com)" RTF_BR
		"https://pugixml.org/" RTF_BR;
	sLibraries += fmt::format(FRUN(sLicense), "MIT license");
#endif /* ENABLE_XML */

	/** GNU gettext **/
	// NOTE: glibc's libintl.h doesn't have the version information,
	// so we're only printing this if we're using GNU gettext's version.
#if defined(HAVE_GETTEXT) && defined(LIBINTL_VERSION)
	string gettextVersion;
	if (LIBINTL_VERSION & 0xFF) {
		gettextVersion = fmt::format(FSTR("GNU gettext {:d}.{:d}.{:d}"),
			static_cast<unsigned int>( LIBINTL_VERSION >> 16),
			static_cast<unsigned int>((LIBINTL_VERSION >>  8) & 0xFF),
			static_cast<unsigned int>( LIBINTL_VERSION        & 0xFF));
	} else {
		gettextVersion = fmt::format(FSTR("GNU gettext {:d}.{:d}"),
			static_cast<unsigned int>( LIBINTL_VERSION >> 16),
			static_cast<unsigned int>((LIBINTL_VERSION >>  8) & 0xFF));
	}

	sLibraries += RTF_BR RTF_BR;
	// NOTE: Not actually an "internal copy"...
	sLibraries += fmt::format(FRUN(sIntCopyOf), gettextVersion);
	sLibraries += RTF_BR
		"Copyright (C) 1995-1997, 2000-2016, 2018-2020 Free Software Foundation, Inc." RTF_BR
		"https://www.gnu.org/software/gettext/" RTF_BR;
	sLibraries += fmt::format(FRUN(sLicense), "GNU LGPL v2.1+");
#endif /* HAVE_GETTEXT && LIBINTL_VERSION */

	sLibraries += '}';

	// Add the "Libraries" tab.
	const tstring tsTabTitle = TC_("AboutTab", "Libraries");
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
	sSupport = RTF_START;
	sSupport += initRtfColorTable();

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
	const tstring tsTabTitle = TC_("AboutTab", "Support");
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
 * Update RTF color tables for the system theme.
 */
void AboutTabPrivate::updateRtfColorTablesInRtfStrings(void)
{
	// Color table
	const string sRtfColorTable = initRtfColorTable();

	// Update the strings, assuming the color table is present.
	const size_t pos = sizeof(RTF_START) - 1;
	const size_t len = sRtfColorTable.size();

#ifdef ENABLE_NETWORKING
	if (!sVersionLabel.empty()) {
		sVersionLabel.replace(pos, len, sRtfColorTable);
	}
#endif /* ENABLE_NETWORKING */

	bool areTabsEmpty = true;
	if (!sCredits.empty()) {
		sCredits.replace(pos, len, sRtfColorTable);
		areTabsEmpty = false;
	}
	if (!sLibraries.empty()) {
		sLibraries.replace(pos, len, sRtfColorTable);
		areTabsEmpty = false;
	}
	if (!sSupport.empty()) {
		sSupport.replace(pos, len, sRtfColorTable);
		areTabsEmpty = false;
	}

	// Update the current tab contents.
	if (!areTabsEmpty) {
		HWND hTabControl = GetDlgItem(hWndPropSheet, IDC_ABOUT_TABCONTROL);
		assert(hTabControl != nullptr);
		if (likely(hTabControl)) {
			// Set the tab contents.
			const int index = TabCtrl_GetCurSel(hTabControl);
			setTabContents(index);
		}
	}

#ifdef ENABLE_NETWORKING
	// Update the version label.
	// FIXME: Only if we're not currently checking for updates?
	if (!sVersionLabel.empty()) {
		// NOTE: EM_SETTEXTEX doesn't seem to work.
		// We'll need to stream in the text instead.
		// Reference: https://devblogs.microsoft.com/oldnewthing/20070110-13/?p=28463
		rtfCtx_upd.str = &sVersionLabel;
		rtfCtx_upd.pos = 0;
		EDITSTREAM es = { (DWORD_PTR)&rtfCtx_upd, 0, EditStreamCallback };
		SendMessage(hUpdateCheck, EM_STREAMIN, SF_RTF, (LPARAM)&es);
	}
#endif /* ENABLE_NETWORKING */
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
	tcItem.pszText = const_cast<LPTSTR>(_T("DUMMY"));
	TabCtrl_InsertItem(hTabControl, MAX_TABS, &tcItem);

	// Adjust the RichEdit position.
	assert(hWndPropSheet != nullptr);
	if (unlikely(!hWndPropSheet)) {
		// Something went wrong...
		return;
	}

#ifdef ENABLE_NETWORKING
	hUpdateCheck = GetDlgItem(hWndPropSheet, IDC_ABOUT_UPDATE_CHECK);
	assert(hUpdateCheck != nullptr);
	if (unlikely(!hUpdateCheck)) {
		// Something went wrong...
		return;
	}
#endif /* ENABLE_NETWORKING */

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
			WS_EX_LEFT | WS_EX_NOPARENTNOTIFY | WS_EX_CLIENTEDGE | WS_EX_TRANSPARENT,
			MSFTEDIT_CLASS, _T(""),
			WS_TABSTOP | WS_VISIBLE | WS_CHILD | WS_VSCROLL |
				ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL,
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

#ifdef ENABLE_NETWORKING
		// IDC_ABOUT_UPDATE_CHECK
		RECT rectUpdateCheck;
		GetClientRect(hUpdateCheck, &rectUpdateCheck);
		MapWindowPoints(hUpdateCheck, hWndPropSheet, (LPPOINT)&rectUpdateCheck, 2);

		HWND hUpdateCheck41 = CreateWindowEx(
			WS_EX_RIGHT | WS_EX_NOPARENTNOTIFY | WS_EX_TRANSPARENT,
			MSFTEDIT_CLASS, _T(""),
			WS_VISIBLE | WS_CHILD | ES_MULTILINE | ES_READONLY,
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
#endif /* ENABLE_NETWORKING */
	}
#endif /* MSFTEDIT_USE_41 */

	// Set the RichEdit's position.
	SetWindowPos(hRichEdit, nullptr,
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

#ifdef ENABLE_NETWORKING
	// RichEdit adjustments for IDC_ABOUT_UPDATE_CHECK.
	// Enable links.
	eventMask = SendMessage(hUpdateCheck, EM_GETEVENTMASK, 0, 0);
	SendMessage(hUpdateCheck, EM_SETEVENTMASK, 0, (LPARAM)(eventMask | ENM_LINK));
	SendMessage(hUpdateCheck, EM_AUTOURLDETECT, AURL_ENABLEURL, 0);
#endif /* ENABLE_NETWORKING */

	// Initialize the tab text.
	initCreditsTab();
	initLibrariesTab();
	initSupportTab();

	// Subclass the RichEdit controls.
	// TODO: Error handling?
#ifdef ENABLE_NETWORKING
	SetWindowSubclass(hUpdateCheck,
		LibWin32UI::MultiLineEditProc,
		IDC_ABOUT_UPDATE_CHECK,
		reinterpret_cast<DWORD_PTR>(GetParent(hWndPropSheet)));
#endif /* ENABLE_NETWORKING */
	SetWindowSubclass(hRichEdit,
			  LibWin32UI::MultiLineEditProc,
		   IDC_ABOUT_RICHEDIT,
		   reinterpret_cast<DWORD_PTR>(GetParent(hWndPropSheet)));

	// Remove the dummy tab.
	TabCtrl_DeleteItem(hTabControl, MAX_TABS);
	TabCtrl_SetCurSel(hTabControl, 0);

	// Set window themes for Win10's dark mode.
	if (g_darkModeSupported) {
		// RichEdit doesn't support dark mode per se, but we can
		// adjust its background and text colors.

		// NOTE: These functions must be called again on theme change!
#ifdef ENABLE_NETWORKING
		DarkMode_InitRichEdit(hUpdateCheck);
#endif /* ENABLE_NETWORKING */
		DarkMode_InitRichEdit(hRichEdit);
		// ...but not this function.
		DarkMode_InitTabControl(hTabControl);
	}

	// Set tab contents to Credits.
	setTabContents(0);
}

/** AboutTab **/

AboutTab::AboutTab(void)
	: d_ptr(new AboutTabPrivate())
{}

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
	const tstring tsTabTitle = TC_("AboutTab", "About");

	PROPSHEETPAGE psp;
	psp.dwSize = sizeof(psp);
	psp.dwFlags = PSP_USECALLBACK | PSP_USETITLE | PSP_DLGINDIRECT;
	psp.hInstance = HINST_THISCOMPONENT;
	psp.pResource = LoadDialog_i18n(HINST_THISCOMPONENT, IDD_CONFIG_ABOUT);
	psp.pszIcon = nullptr;
	psp.pszTitle = tsTabTitle.c_str();
	psp.pfnDlgProc = AboutTabPrivate::dlgProc;
	psp.lParam = reinterpret_cast<LPARAM>(d);
	psp.pcRefParent = nullptr;
	psp.pfnCallback = AboutTabPrivate::callbackProc;

	d->hPropSheetPage = CreatePropertySheetPage(&psp);
	return d->hPropSheetPage;
}
