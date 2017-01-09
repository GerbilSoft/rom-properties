/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * RP_ShellPropSheetExt.hpp: IShellPropSheetExt implementation.            *
 *                                                                         *
 * Copyright (c) 2016 by David Korth.                                      *
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

// References:
// - http://www.codeproject.com/Articles/338268/COM-in-C
// - https://code.msdn.microsoft.com/windowsapps/CppShellExtPropSheetHandler-d93b49b7
// - https://msdn.microsoft.com/en-us/library/ms677109(v=vs.85).aspx

#include "stdafx.h"
#include "RP_ShellPropSheetExt.hpp"
#include "RegKey.hpp"
#include "RpImageWin32.hpp"
#include "resource.h"

// libromdata
#include "libromdata/common.h"
#include "libromdata/RomDataFactory.hpp"
#include "libromdata/RomData.hpp"
#include "libromdata/RomFields.hpp"
#include "libromdata/file/RpFile.hpp"
#include "libromdata/img/rp_image.hpp"
#include "libromdata/RpWin32.hpp"
using namespace LibRomData;

// C includes. (C++ namespace)
#include <cassert>
#include <cstring>

// C++ includes.
#include <memory>
#include <string>
#include <unordered_set>
#include <vector>
using std::unique_ptr;
using std::unordered_set;
using std::wstring;
using std::vector;

// Gdiplus for image drawing.
// NOTE: Gdiplus requires min/max.
#include <algorithm>
namespace Gdiplus {
	using std::min;
	using std::max;
}
#include <gdiplus.h>
#include "libromdata/img/GdiplusHelper.hpp"
#include "libromdata/img/RpGdiplusBackend.hpp"
#include "libromdata/img/IconAnimData.hpp"
#include "libromdata/img/IconAnimHelper.hpp"

// CLSID
const CLSID CLSID_RP_ShellPropSheetExt =
	{0x2443C158, 0xDF7C, 0x4352, {0xB4, 0x35, 0xBC, 0x9F, 0x88, 0x5F, 0xFD, 0x52}};

// IDC_STATIC might not be defined.
#ifndef IDC_STATIC
#define IDC_STATIC (-1)
#endif

// Control base IDs.
#define IDC_STATIC_BANNER		0x100
#define IDC_STATIC_ICON			0x101
#define IDC_STATIC_DESC(idx)		(0x1000 + (idx))
#define IDC_RFT_STRING(idx)		(0x1400 + (idx))
#define IDC_RFT_LISTDATA(idx)		(0x1800 + (idx))
// Date/Time acts like a string widget internally.
#define IDC_RFT_DATETIME(idx)		IDC_RFT_STRING(idx)

// Bitfield is last due to multiple controls per field.
#define IDC_RFT_BITFIELD(idx, bit)	(0x7000 + ((idx) * 32) + (bit))

// Property for "external pointer".
// This links the property sheet to the COM object.
#define EXT_POINTER_PROP L"RP_ShellPropSheetExt"

/** RP_ShellPropSheetExt_Private **/
// Workaround for RP_D() expecting the no-underscore naming convention.
#define RP_ShellPropSheetExtPrivate RP_ShellPropSheetExt_Private

class RP_ShellPropSheetExt_Private
{
	public:
		RP_ShellPropSheetExt_Private(RP_ShellPropSheetExt *q);
		~RP_ShellPropSheetExt_Private();

	private:
		RP_ShellPropSheetExt_Private(const RP_ShellPropSheetExt_Private &other);
		RP_ShellPropSheetExt_Private &operator=(const RP_ShellPropSheetExt_Private &other);
	private:
		RP_ShellPropSheetExt *const q_ptr;

	public:
		// ROM data.
		LibRomData::RomData *romData;

		// Useful window handles.
		HWND hDlgProps;		// Property dialog. (parent)
		HWND hDlgSheet;		// Property sheet.

		// Fonts.
		HFONT hFontBold;	// Bold font.
		HFONT hFontMono;	// Monospaced font.

		// Monospaced font details.
		LOGFONT lfFontMono;
		unordered_set<wstring> monospaced_fonts;
		vector<HWND> hwndMonoControls;			// Controls using the monospaced font.
		bool bPrevIsClearType;	// Previous ClearType setting.

		// Controls that need to be drawn using red text.
		unordered_set<HWND> hwndWarningControls;	// Controls using the "Warning" font.
		// SysLink controls.
		unordered_set<HWND> hwndSysLinkControls;

		// GDI+ token.
		ScopedGdiplus gdipScope;

		// Header row widgets.
		HWND lblSysInfo;

		// Window background color.
		COLORREF colorWinBg;
		// XP theming.
		typedef BOOL (STDAPICALLTYPE* PFNISTHEMEACTIVE)(void);
		HMODULE hUxTheme_dll;
		PFNISTHEMEACTIVE pfnIsThemeActive;

		// Banner.
		HBITMAP hbmpBanner;
		POINT ptBanner;
		SIZE szBanner;

		// Animated icon data.
		const IconAnimData *iconAnimData;
		HBITMAP hbmpIconFrames[IconAnimData::MAX_FRAMES];
		RECT rectIcon;
		SIZE szIcon;
		IconAnimHelper iconAnimHelper;
		UINT_PTR animTimerID;		// Animation timer ID. (non-zero == running)
		int last_frame_number;		// Last frame number.

		/**
		 * Start the animation timer.
		 */
		void startAnimTimer(void);

		/**
		 * Stop the animation timer.
		 */
		void stopAnimTimer(void);

	private:
		/**
		 * Convert UNIX line endings to DOS line endings.
		 * TODO: Move to RpWin32?
		 * @param wstr_unix	[in] wstring with UNIX line endings.
		 * @param lf_count	[out,opt] Number of LF characters found.
		 * @return wstring with DOS line endings.
		 */
		inline wstring unix2dos(const wstring &wstr_unix, int *lf_count);

		/**
		 * Measure text size using GDI.
		 * @param hWnd		[in] hWnd.
		 * @param hFont		[in] Font.
		 * @param str		[in] String.
		 * @param lpSize	[out] Size.
		 * @return 0 on success; non-zero on error.
		 */
		int measureTextSize(HWND hWnd, HFONT hFont, const wstring &wstr, LPSIZE lpSize);

		/**
		 * Measure text size using GDI.
		 * This version removes HTML-style tags before
		 * calling the regular measureTextSize() function.
		 * @param hWnd		[in] hWnd.
		 * @param hFont		[in] Font.
		 * @param str		[in] String.
		 * @param lpSize	[out] Size.
		 * @return 0 on success; non-zero on error.
		 */
		int measureTextSizeLink(HWND hWnd, HFONT hFont, const wstring &wstr, LPSIZE lpSize);

	public:
		/**
		 * Load the banner and icon as HBITMAPs.
		 *
		 * This function should bee called on startup and if
		 * the window's background color changes.
		 *
		 * @param hDlg	[in] Dialog window.
		 */
		void loadImages(HWND hDlg);

	private:
		/**
		 * Increase an image's size to the minimum size, if necesary.
		 * @param sz Image size.
		 */
		void incSizeToMinimum(SIZE &sz);

		/**
		 * Create the header row.
		 * @param hDlg		[in] Dialog window.
		 * @param pt_start	[in] Starting position, in pixels.
		 * @param size		[in] Width and height for a full-width single line label.
		 * @return Row height, in pixels.
		 */
		int createHeaderRow(HWND hDlg, const POINT &pt_start, const SIZE &size);

		/**
		 * Initialize a string field. (Also used for Date/Time.)
		 * @param hDlg		[in] Dialog window.
		 * @param pt_start	[in] Starting position, in pixels.
		 * @param idx		[in] Field index.
		 * @param size		[in] Width and height for a single line label.
		 * @param wcs		[in,opt] String data. (If nullptr, field data is used.)
		 * @return Field height, in pixels.
		 */
		int initString(HWND hDlg, const POINT &pt_start, int idx, const SIZE &size, LPCWSTR wcs);

		/**
		 * Initialize a bitfield layout.
		 * @param hDlg Dialog window.
		 * @param pt_start Starting position, in pixels.
		 * @param idx Field index.
		 * @return Field height, in pixels.
		 */
		int initBitfield(HWND hDlg, const POINT &pt_start, int idx);

		/**
		 * Initialize a ListView control.
		 * @param hWnd HWND of the ListView control.
		 * @param desc RomFields description.
		 * @param data RomFields data.
		 */
		void initListView(HWND hWnd, const LibRomData::RomFields::Desc *desc, const LibRomData::RomFields::Data *data);

		/**
		 * Initialize a Date/Time field.
		 * This function internally calls initString().
		 * @param hDlg		[in] Dialog window.
		 * @param pt_start	[in] Starting position, in pixels.
		 * @param idx		[in] Field index.
		 * @param size		[in] Width and height for a single line label.
		 * @return Field height, in pixels.
		 */
		int initDateTime(HWND hDlg, const POINT &pt_start, int idx, const SIZE &size);

		/**
		 * Initialize the bold font.
		 * @param hFont Base font.
		 */
		void initBoldFont(HFONT hFont);

		/**
		 * Monospaced font enumeration procedure.
		 * @param lpelfe Enumerated font information.
		 * @param lpntme Font metrics.
		 * @param FontType Font type.
		 * @param lParam Pointer to RP_ShellPropSheetExt_Private.
		 */
		static int CALLBACK MonospacedFontEnumProc(
			const LOGFONT *lpelfe, const TEXTMETRIC *lpntme,
			DWORD FontType, LPARAM lParam);

	public:
		/**
		 * Initialize the monospaced font.
		 * @param hFont Base font.
		 */
		void initMonospacedFont(HFONT hFont);

		/**
		 * Initialize the dialog.
		 * Called by WM_INITDIALOG.
		 * @param hDlg Dialog window.
		 */
		void initDialog(HWND hDlg);
};

/** RP_ShellPropSheetExt_Private **/

RP_ShellPropSheetExt_Private::RP_ShellPropSheetExt_Private(RP_ShellPropSheetExt *q)
	: q_ptr(q)
	, romData(nullptr)
	, hDlgProps(nullptr)
	, hDlgSheet(nullptr)
	, hFontBold(nullptr)
	, hFontMono(nullptr)
	, bPrevIsClearType(nullptr)
	, lblSysInfo(nullptr)
	, colorWinBg(0)
	, hUxTheme_dll(nullptr)
	, pfnIsThemeActive(nullptr)
	, hbmpBanner(nullptr)
	, iconAnimData(nullptr)
	, animTimerID(0)
	, last_frame_number(0)
{
	memset(hbmpIconFrames, 0, sizeof(hbmpIconFrames));

	// Attempt to get IsThemeActive() from uxtheme.dll.
	hUxTheme_dll = LoadLibrary(L"uxtheme.dll");
	if (hUxTheme_dll) {
		pfnIsThemeActive = (PFNISTHEMEACTIVE)GetProcAddress(hUxTheme_dll, "IsThemeActive");
	}
}

RP_ShellPropSheetExt_Private::~RP_ShellPropSheetExt_Private()
{
	stopAnimTimer();
	iconAnimHelper.setIconAnimData(nullptr);
	if (romData) {
		romData->unref();
	}

	// Delete the banner and icon frames.
	if (hbmpBanner) {
		DeleteObject(hbmpBanner);
	}
	for (int i = ARRAY_SIZE(hbmpIconFrames)-1; i >= 0; i--) {
		if (hbmpIconFrames[i]) {
			DeleteObject(hbmpIconFrames[i]);
		}
	}

	// Delete the fonts.
	if (hFontBold) {
		DeleteFont(hFontBold);
	}
	if (hFontMono) {
		DeleteFont(hFontMono);
	}

	// Close uxtheme.dll.
	if (hUxTheme_dll) {
		FreeLibrary(hUxTheme_dll);
	}
}

/**
 * Start the animation timer.
 */
void RP_ShellPropSheetExt_Private::startAnimTimer(void)
{
	if (!iconAnimData) {
		// Not an animated icon.
		return;
	}

	// Get the current frame information.
	last_frame_number = iconAnimHelper.frameNumber();
	const int delay = iconAnimHelper.frameDelay();
	if (delay <= 0) {
		// Invalid delay value.
		return;
	}

	// Set a timer for the current frame.
	// We're using the 'd' pointer as nIDEvent.
	animTimerID = SetTimer(hDlgSheet,
		reinterpret_cast<UINT_PTR>(this),
		delay,
		RP_ShellPropSheetExt::AnimTimerProc);
}

/**
 * Stop the animation timer.
 */
void RP_ShellPropSheetExt_Private::stopAnimTimer(void)
{
	if (animTimerID) {
		KillTimer(hDlgSheet, animTimerID);
		animTimerID = 0;
	}
}

/**
 * Convert UNIX line endings to DOS line endings.
 * TODO: Move to RpWin32?
 * @param wstr_unix	[in] wstring with UNIX line endings.
 * @param lf_count	[out,opt] Number of LF characters found.
 * @return wstring with DOS line endings.
 */
inline wstring RP_ShellPropSheetExt_Private::unix2dos(const wstring &wstr_unix, int *lf_count)
{
	// TODO: Optimize this!
	wstring wstr_dos;
	wstr_dos.reserve(wstr_unix.size());
	int lf = 0;
	for (size_t i = 0; i < wstr_unix.size(); i++) {
		if (wstr_unix[i] == L'\n') {
			wstr_dos += L"\r\n";
			lf++;
		} else {
			wstr_dos += wstr_unix[i];
		}
	}
	if (lf_count) {
		*lf_count = lf;
	}
	return wstr_dos;
}

/**
 * Measure text size using GDI.
 * @param hWnd		[in] hWnd.
 * @param hFont		[in] Font.
 * @param wstr		[in] String.
 * @param lpSize	[out] Size.
 * @return 0 on success; non-zero on errro.
 */
int RP_ShellPropSheetExt_Private::measureTextSize(HWND hWnd, HFONT hFont, const wstring &wstr, LPSIZE lpSize)
{
	SIZE size_total = {0, 0};

	HDC hDC = GetDC(hWnd);
	HFONT hFontOrig = SelectFont(hDC, hFont);

	// Handle newlines.
	int lines = 0;
	const wchar_t *data = wstr.data();
	int nl_pos_prev = -1;
	size_t nl_pos = 0;	// Assuming no NL at the start.
	do {
		nl_pos = wstr.find(L'\n', nl_pos + 1);
		const int start = nl_pos_prev + 1;
		int len;
		if (nl_pos != wstring::npos) {
			len = (int)(nl_pos - start);
		} else {
			len = (int)(wstr.size() - start);
		}

		// Check if a '\r' is present before the '\n'.
		if (data[nl_pos - 1] == L'\r') {
			// Ignore the '\r'.
			len--;
		}

		SIZE size_cur;
		BOOL bRet = GetTextExtentPoint32(hDC, &data[start], len, &size_cur);
		if (!bRet) {
			// Something failed...
			SelectFont(hDC, hFontOrig);
			ReleaseDC(hWnd, hDC);
			return -1;
		}

		if (size_cur.cx > size_total.cx) {
			size_total.cx = size_cur.cx;
		}
		size_total.cy += size_cur.cy;

		// Next newline.
		nl_pos_prev = (int)nl_pos;
	} while (nl_pos != wstring::npos);

	SelectFont(hDC, hFontOrig);
	ReleaseDC(hWnd, hDC);

	*lpSize = size_total;
	return 0;
}

/**
 * Measure text size using GDI.
 * This version removes HTML-style tags before
 * calling the regular measureTextSize() function.
 * @param hWnd		[in] hWnd.
 * @param hFont		[in] Font.
 * @param wstr		[in] String.
 * @param lpSize	[out] Size.
 * @return 0 on success; non-zero on error.
 */
int RP_ShellPropSheetExt_Private::measureTextSizeLink(HWND hWnd, HFONT hFont, const wstring &wstr, LPSIZE lpSize)
{
	// Remove HTML-style tags.
	// NOTE: This is a very simplistic version.
	wstring nwstr;
	nwstr.reserve(wstr.size());

	int lbrackets = 0;
	for (int i = 0; i < (int)wstr.size(); i++) {
		if (wstr[i] == '<') {
			// Starting bracket.
			lbrackets++;
		} else if (wstr[i] == '>') {
			// Ending bracket.
			assert(lbrackets > 0);
			lbrackets--;
		}

		if (lbrackets == 0) {
			// Not currently in a tag.
			nwstr += wstr[i];
		}
	}

	return measureTextSize(hWnd, hFont, nwstr, lpSize);
}

/**
 * Load the banner and icon as HBITMAPs.
 *
 * This function should be called on startup and if
 * the window's background color changes.
 *
 * @param hDlg	[in] Dialog window.
 */
void RP_ShellPropSheetExt_Private::loadImages(HWND hDlg)
{
	// Window background color.
	// Static controls don't support alpha transparency (?? test),
	// so we have to fake it.
	// NOTE: GetSysColor() has swapped R and B channels
	// compared to GDI+.
	// TODO: Get the actual background color of the window.
	// TODO: Use DrawThemeBackground:
	// - http://www.codeproject.com/Articles/5978/Correctly-drawn-themed-dialogs-in-WinXP
	// - https://blogs.msdn.microsoft.com/dsui_team/2013/06/26/using-theme-apis-to-draw-the-border-of-a-control/
	// - https://blogs.msdn.microsoft.com/pareshj/2011/11/03/draw-the-background-of-static-control-with-gradient-fill-when-theme-is-enabled/
	if (pfnIsThemeActive && pfnIsThemeActive()) {
		// Theme is active.
		colorWinBg = GetSysColor(COLOR_WINDOW);
	} else {
		// Theme is not active.
		colorWinBg = GetSysColor(COLOR_3DFACE);
	}
	Gdiplus::ARGB gdipBgColor =
		   (colorWinBg & 0x00FF00) | 0xFF000000 |
		  ((colorWinBg & 0xFF) << 16) |
		  ((colorWinBg >> 16) & 0xFF);

	// Supported image types.
	const uint32_t imgbf = romData->supportedImageTypes();

	// Banner.
	if (imgbf & RomData::IMGBF_INT_BANNER) {
		// Delete the old banner.
		if (hbmpBanner != nullptr) {
			DeleteObject(hbmpBanner);
			hbmpBanner = nullptr;
		}

		// Get the banner.
		const rp_image *banner = romData->image(RomData::IMG_INT_BANNER);
		if (banner && banner->isValid()) {
			// Convert to HBITMAP using the window background color.
			// TODO: Redo if the window background color changes.
			hbmpBanner = RpImageWin32::toHBITMAP(banner, gdipBgColor, szBanner, true);
		}
	}

	// Icon.
	if (imgbf & RomData::IMGBF_INT_ICON) {
		// Delete the old icons.
		for (int i = ARRAY_SIZE(hbmpIconFrames)-1; i >= 0; i--) {
			if (hbmpIconFrames[i]) {
				DeleteObject(hbmpIconFrames[i]);
				hbmpIconFrames[i] = nullptr;
			}
		}

		// Get the icon.
		const rp_image *icon = romData->image(RomData::IMG_INT_ICON);
		if (icon && icon->isValid()) {
			// Get the animated icon data.
			if (iconAnimData) {
				// Convert the icons to GDI+ bitmaps.
				// TODO: Refactor this a bit...
				for (int i = 0; i < iconAnimData->count; i++) {
					if (iconAnimData->frames[i] && iconAnimData->frames[i]->isValid()) {
						// Convert to HBITMAP using the window background color.
						// TODO: Redo if the window background color changes.
						hbmpIconFrames[i] = RpImageWin32::toHBITMAP(iconAnimData->frames[i], gdipBgColor, szIcon, true);
					}
				}

				// Set up the IconAnimHelper.
				iconAnimHelper.setIconAnimData(iconAnimData);
				last_frame_number = iconAnimHelper.frameNumber();

				// Icon animation timer is set in startAnimTimer().
			} else {
				// Not an animated icon.
				last_frame_number = 0;

				// Convert to HBITMAP using the window background color.
				// TODO: Redo if the window background color changes.
				hbmpIconFrames[0] = RpImageWin32::toHBITMAP(icon, gdipBgColor, szIcon, true);
			}
		}
	}
}

/**
 * Increase an image's size to the minimum size, if necesary.
 * @param sz Image size.
 */
void RP_ShellPropSheetExt_Private::incSizeToMinimum(SIZE &sz)
{
	// Minimum image size.
	// If images are smaller, they will be resized.
	// TODO: Adjust minimum size for DPI.
	static const SIZE min_img_size = {32, 32};

	if (sz.cx >= min_img_size.cx && sz.cx >= min_img_size.cy) {
		// No resize necessary.
		return;
	}

	// Resize the image.
	SIZE orig_sz = sz;
	do {
		// Increase by integer multiples until
		// the icon is at least 32x32.
		// TODO: Constrain to 32x32?
		sz.cx += orig_sz.cx;
		sz.cy += orig_sz.cy;
	} while (sz.cx < min_img_size.cx && sz.cy < min_img_size.cy);
}

/**
 * Create the header row.
 * @param hDlg		[in] Dialog window.
 * @param pt_start	[in] Starting position, in pixels.
 * @param size		[in] Width and height for a full-width single line label.
 * @return Row height, in pixels.
 */
int RP_ShellPropSheetExt_Private::createHeaderRow(HWND hDlg, const POINT &pt_start, const SIZE &size)
{
	if (!hDlg)
		return 0;

	// Total widget width.
	int total_widget_width = 0;

	// System name.
	// TODO: Logo, game icon, and game title?
	const rp_char *systemName = romData->systemName(
		RomData::SYSNAME_TYPE_LONG | RomData::SYSNAME_REGION_ROM_LOCAL);

	// File type.
	const rp_char *const fileType = romData->fileType_string();

	wstring sysInfo;
	if (systemName) {
		sysInfo = RP2W_c(systemName);
	}
	if (fileType) {
		if (!sysInfo.empty()) {
			sysInfo += L"\r\n";
		}
		sysInfo += RP2W_c(fileType);
	}

	// Get the default font.
	HFONT hFont = GetWindowFont(hDlg);
 
	// Label size.
	SIZE sz_lblSysInfo = {0, 0};

	if (!sysInfo.empty()) {
		// Determine the appropriate label size.
		int ret = measureTextSize(hDlg,
			(hFontBold ? hFontBold : hFont),
			sysInfo, &sz_lblSysInfo);
		if (ret != 0) {
			// Error determining the label size.
			// Don't draw the label.
			sz_lblSysInfo.cx = 0;
			sz_lblSysInfo.cy = 0;
		} else {
			// Start the total_widget_width.
			total_widget_width = sz_lblSysInfo.cx;
		}
	}

	// Supported image types.
	const uint32_t imgbf = romData->supportedImageTypes();

	// Banner.
	const rp_image *banner = nullptr;
	if (imgbf & RomData::IMGBF_INT_BANNER) {
		// Get the banner.
		banner = romData->image(RomData::IMG_INT_BANNER);
		if (banner && banner->isValid()) {
			szBanner.cx = banner->width();
			szBanner.cy = banner->height();
			incSizeToMinimum(szBanner);

			// Add the banner width.
			// The banner will be assigned to a WC_STATIC control.
			if (total_widget_width > 0) {
				total_widget_width += pt_start.x;
			}
			total_widget_width += szBanner.cx;
		} else {
			// No banner.
			banner = nullptr;
		}
	}

	// Icon.
	const rp_image *icon = nullptr;
	if (imgbf & RomData::IMGBF_INT_ICON) {
		// Get the icon.
		icon = romData->image(RomData::IMG_INT_ICON);
		if (icon && icon->isValid()) {
			szIcon.cx = icon->width();
			szIcon.cy = icon->height();
			incSizeToMinimum(szIcon);

			// Add the icon width.
			// The icon will be assigned to a WC_STATIC control.
			if (total_widget_width > 0) {
				total_widget_width += pt_start.x;
			}
			total_widget_width += szIcon.cx;

			// Get the animated icon data.
			iconAnimData = romData->iconAnimData();
		} else {
			// No icon.
			icon = nullptr;
		}
	}

	// Starting point.
	POINT curPt = {
		((size.cx - total_widget_width) / 2) + pt_start.x,
		pt_start.y
	};

	// lblSysInfo
	if (sz_lblSysInfo.cx > 0 && sz_lblSysInfo.cy > 0) {
		lblSysInfo = CreateWindow(WC_STATIC, sysInfo.c_str(),
			WS_CHILD | WS_VISIBLE | SS_CENTER,
			curPt.x, curPt.y,
			sz_lblSysInfo.cx, sz_lblSysInfo.cy,
			hDlg, (HMENU)IDC_STATIC, nullptr, nullptr);
		SetWindowFont(lblSysInfo, (hFontBold ? hFontBold : hFont), FALSE);
		curPt.x += sz_lblSysInfo.cx + pt_start.x;
	}

	// lblBanner
	if (banner) {
		ptBanner = curPt;
		curPt.x += szBanner.cx + pt_start.x;
	}

	// lblIcon
	if (icon) {
		SetRect(&rectIcon, curPt.x, curPt.y,
			curPt.x + szIcon.cx, curPt.y + szIcon.cy);
		curPt.x += szIcon.cx + pt_start.x;
	}

	// Load the images.
	loadImages(hDlg);

	// Return the label height and some extra padding.
	// TODO: Icon/banner height?
	return sz_lblSysInfo.cy + (pt_start.y * 5 / 8);
}

/**
 * Initialize a string field. (Also used for Date/Time.)
 * @param hDlg		[in] Dialog window.
 * @param pt_start	[in] Starting position, in pixels.
 * @param idx		[in] Field index.
 * @param size		[in] Width and height for a single line label.
 * @param wcs		[in,opt] String data. (If nullptr, field data is used.)
 * @return Field height, in pixels.
 */
int RP_ShellPropSheetExt_Private::initString(HWND hDlg,
	const POINT &pt_start, int idx, const SIZE &size, LPCWSTR wcs)
{
	if (!hDlg)
		return 0;

	const RomFields *fields = romData->fields();
	if (!fields)
		return 0;

	const RomFields::Desc *desc = fields->desc(idx);
	if (!desc)
		return 0;
	if (!desc->name || desc->name[0] == '\0')
		return 0;

	// NOTE: libromdata uses Unix-style newlines.
	// For proper display on Windows, we have to
	// add carriage returns.

	// If string data wasn't specified, get the RFT_STRING data
	// from the RomFields::Data object.
	int lf_count = 0;
	wstring wstr;
	if (!wcs) {
		const RomFields::Data *data = fields->data(idx);
		if (!data)
			return 0;
		if (desc->type != RomFields::RFT_STRING ||
		    data->type != RomFields::RFT_STRING)
			return 0;

		// TODO: NULL string == empty string?
		if (data->str) {
			wstr = unix2dos(RP2W_s(data->str), &lf_count);
		}
	} else {
		// Use the specified string.
		wstr = unix2dos(wstring(wcs), &lf_count);
	}

	// Field height.
	int field_cy = size.cy;
	if (lf_count > 0) {
		// Multiple lines.
		// NOTE: Only add 5/8 of field_cy per line.
		// FIXME: 5/8 needs adjustment...
		field_cy += (field_cy * lf_count) * 5 / 8;
	}

	// Dialog item.
	const HMENU cId = (HMENU)(INT_PTR)(IDC_RFT_STRING(idx));
	HWND hDlgItem;

	// Get the default font.
	HFONT hFont = GetWindowFont(hDlg);

	// Check for any formatting options.
	bool isWarning = false, isMonospace = false;
	if (desc->type == RomFields::RFT_STRING && desc->str_desc) {
		// FIXME: STRF_MONOSPACE | STRF_WARNING is not supported.
		// Preferring STRF_WARNING.
		assert((desc->str_desc->formatting &
			(RomFields::StringDesc::STRF_MONOSPACE | RomFields::StringDesc::STRF_WARNING)) !=
			(RomFields::StringDesc::STRF_MONOSPACE | RomFields::StringDesc::STRF_WARNING));

		if (desc->str_desc->formatting & RomFields::StringDesc::STRF_WARNING) {
			// "Warning" font.
			if (hFontBold) {
				hFont = hFontBold;
				isWarning = true;
				// Set the font of the description control.
				HWND hStatic = GetDlgItem(hDlg, IDC_STATIC_DESC(idx));
				if (hStatic) {
					SetWindowFont(hStatic, hFont, FALSE);
					hwndWarningControls.insert(hStatic);
				}
			}
		} else if (desc->str_desc->formatting & RomFields::StringDesc::STRF_MONOSPACE) {
			// Monospaced font.
			if (hFontMono) {
				hFont = hFontMono;
				isMonospace = true;
			}
		}
	}

	if (desc->type == RomFields::RFT_STRING && desc->str_desc &&
	    (desc->str_desc->formatting & RomFields::StringDesc::STRF_CREDITS))
	{
		// Align to the bottom of the dialog and center-align the text.
		// 7x7 DLU margin is recommended by the Windows UX guidelines.
		// Reference: http://stackoverflow.com/questions/2118603/default-dialog-padding
		RECT tmpRect = {7, 7, 8, 8};
		MapDialogRect(hDlg, &tmpRect);
		RECT winRect;
		GetClientRect(hDlg, &winRect);

		// Create a SysLink widget.
		// SysLink allows the user to click a link and
		// open a webpage. It does NOT allow highlighting.
		// TODO: SysLink + EDIT?
		// FIXME: Centered text alignment?
		hDlgItem = CreateWindow(WC_LINK, wstr.c_str(),
			WS_CHILD | WS_VISIBLE,
			0, 0, 0, 0,	// will be adjusted afterwards
			hDlg, cId, nullptr, nullptr);
		// There should be a maximum of one STRF_CREDITS per RomData subclass.
		assert(hwndSysLinkControls.empty());
		hwndSysLinkControls.insert(hDlgItem);
		SetWindowFont(hDlgItem, hFont, FALSE);

		// NOTE: We can't use measureTextSize() because that includes
		// the HTML markup, and LM_GETIDEALSIZE is Vista+ only.
		// Use a wrapper measureTextSizeLink() that removes HTML-like
		// tags and then calls measureTextSize().
		SIZE szText;
		measureTextSizeLink(hDlg, hFont, wstr, &szText);

		// Determine the position.
		const int x = (((winRect.right - (tmpRect.left * 2)) - szText.cx) / 2) + tmpRect.left;
		const int y = winRect.bottom - tmpRect.top - szText.cy;
		// Set the position and size.
		SetWindowPos(hDlgItem, 0, x, y, szText.cx, szText.cy,
			SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOZORDER);

		// Clear field_cy so the description widget won't show up
		// and the "normal" area will be empty.
		field_cy = 0;
	} else {
		// Create a read-only EDIT widget.
		// The STATIC control doesn't allow the user
		// to highlight and copy data.
		DWORD dwStyle = WS_CHILD | WS_VISIBLE | ES_READONLY;
		if (lf_count > 0) {
			// Multiple lines.
			dwStyle |= ES_MULTILINE;
		}
		hDlgItem = CreateWindow(WC_EDIT, wstr.c_str(), dwStyle,
			pt_start.x, pt_start.y,
			size.cx, field_cy,
			hDlg, cId, nullptr, nullptr);
		SetWindowFont(hDlgItem, hFont, FALSE);

		// Subclass multi-line EDIT controls to work around Enter/Escape issues.
		// Reference:  http://blogs.msdn.com/b/oldnewthing/archive/2007/08/20/4470527.aspx
		if (dwStyle & ES_MULTILINE) {
			// Store the object pointer so we can reference it later.
			SetProp(hDlgItem, EXT_POINTER_PROP, static_cast<HANDLE>(q_ptr));

			// Subclass the control.
			// TODO: Error handling?
			SetWindowSubclass(hDlgItem,
				RP_ShellPropSheetExt::MultilineEditProc,
				(UINT_PTR)cId, (DWORD_PTR)q_ptr);
		}
	}

	// Save the control in the appropriate set, if necessary.
	if (isWarning) {
		hwndWarningControls.insert(hDlgItem);
	}
	if (isMonospace) {
		hwndMonoControls.push_back(hDlgItem);
	}

	return field_cy;
}

/**
 * Initialize a bitfield layout.
 * @param hDlg Dialog window.
 * @param pt_start Starting position, in pixels.
 * @param idx Field index.
 * @return Field height, in pixels.
 */
int RP_ShellPropSheetExt_Private::initBitfield(HWND hDlg,
	const POINT &pt_start, int idx)
{
	if (!hDlg)
		return 0;

	const RomFields *fields = romData->fields();
	if (!fields)
		return 0;

	const RomFields::Desc *desc = fields->desc(idx);
	const RomFields::Data *data = fields->data(idx);
	if (!desc || !data)
		return 0;
	if (desc->type != RomFields::RFT_BITFIELD ||
	    data->type != RomFields::RFT_BITFIELD)
		return 0;
	if (!desc->name || desc->name[0] == '\0')
		return 0;

	// Checkbox size.
	// Reference: http://stackoverflow.com/questions/1164868/how-to-get-size-of-check-and-gap-in-check-box
	RECT rect_chkbox = {0, 0, 12+4, 11};
	MapDialogRect(hDlg, &rect_chkbox);

	// Dialog font and device context.
	HFONT hFont = GetWindowFont(hDlg);
	HDC hDC = GetDC(hDlg);
	HFONT hFontOrig = SelectFont(hDC, hFont);

	// Create a grid of checkboxes.
	const RomFields::BitfieldDesc *bitfieldDesc = desc->bitfield;
	assert(bitfieldDesc != nullptr);

	// Column widths for multi-row layouts.
	// (Includes the checkbox size.)
	std::vector<int> col_widths;
	int row = 0, col = 0;
	if (bitfieldDesc->elemsPerRow == 1) {
		// Optimization: Use the entire width of the dialog.
		// TODO: Testing; right margin.
		RECT rectDlg;
		GetClientRect(hDlg, &rectDlg);
		col_widths.push_back(rectDlg.right - pt_start.x);
	} else if (bitfieldDesc->elemsPerRow > 1) {
		// Determine the widest entry in each column.
		col_widths.resize(bitfieldDesc->elemsPerRow);
		for (int j = 0; j < bitfieldDesc->elements; j++) {
			const rp_char *name = bitfieldDesc->names[j];
			if (!name)
				continue;

			// Make sure this is a UTF-16 string.
			wstring s_name = RP2W_c(name);

			// Get the width of this specific entry.
			// TODO: Use measureTextSize()?
			SIZE textSize;
			GetTextExtentPoint32(hDC, s_name.data(), (int)s_name.size(), &textSize);
			int chk_w = rect_chkbox.right + textSize.cx;
			if (chk_w > col_widths[col]) {
				col_widths[col] = chk_w;
			}

			// Next column.
			col++;
			if (col == bitfieldDesc->elemsPerRow) {
				// Next row.
				row++;
				col = 0;
			}
		}
	}

	// Initial position on the dialog, in pixels.
	POINT pt = pt_start;
	// Subtract 0.5 DLU from the starting row.
	RECT rect_subtract = {0, 0, 1, 1};
	MapDialogRect(hDlg, &rect_subtract);
	if (rect_subtract.bottom > 1) {
		rect_subtract.bottom /= 2;
	}
	pt.y -= rect_subtract.bottom;

	row = 0; col = 0;
	for (int j = 0; j < bitfieldDesc->elements; j++) {
		const rp_char *name = bitfieldDesc->names[j];
		if (!name)
			continue;

		// Get the text size.
		int chk_w;
		if (bitfieldDesc->elemsPerRow == 0) {
			// Make sure this is a UTF-16 string.
			wstring s_name = RP2W_c(name);

			// Get the width of this specific entry.
			// TODO: Use measureTextSize()?
			SIZE textSize;
			GetTextExtentPoint32(hDC, s_name.data(), (int)s_name.size(), &textSize);
			chk_w = rect_chkbox.right + textSize.cx;
		} else {
			if (col == bitfieldDesc->elemsPerRow) {
				// Next row.
				row++;
				col = 0;
				pt.x = pt_start.x;
				pt.y += rect_chkbox.bottom;
			}

			// Use the largest width in the column.
			chk_w = col_widths[col];
		}

		// FIXME: Tab ordering?
		HWND hCheckBox = CreateWindow(WC_BUTTON, RP2W_c(name),
			WS_CHILD | WS_VISIBLE | BS_CHECKBOX,
			pt.x, pt.y, chk_w, rect_chkbox.bottom,
			hDlg, (HMENU)(INT_PTR)(IDC_RFT_BITFIELD(idx, j)),
			nullptr, nullptr);
		SetWindowFont(hCheckBox, hFont, FALSE);

		// Set the checkbox state.
		Button_SetCheck(hCheckBox, (data->bitfield & (1 << j)) ? BST_CHECKED : BST_UNCHECKED);

		// Next column.
		pt.x += chk_w;
		col++;
	}

	SelectFont(hDC, hFontOrig);
	ReleaseDC(hDlg, hDC);

	// Return the total number of rows times the checkbox height,
	// plus another 0.25 of a checkbox.
	int ret = ((row + 1) * rect_chkbox.bottom);
	ret += (rect_chkbox.bottom / 4);
	return ret;
}

/**
 * Initialize a ListView control.
 * @param hWnd HWND of the ListView control.
 * @param desc RomFields description.
 * @param data RomFields data.
 */
void RP_ShellPropSheetExt_Private::initListView(HWND hWnd,
	const RomFields::Desc *desc, const RomFields::Data *data)
{
	if (!hWnd || !desc || !data)
		return;
	if (desc->type != RomFields::RFT_LISTDATA ||
	    data->type != RomFields::RFT_LISTDATA)
		return;
	if (!desc->name || desc->name[0] == '\0')
		return;

	// Set extended ListView styles.
	ListView_SetExtendedListViewStyle(hWnd, LVS_EX_FULLROWSELECT);

	// Insert columns.
	// TODO: Make sure there aren't any columns to start with?
	const RomFields::ListDataDesc *listDataDesc = desc->list_data;
	assert(listDataDesc != nullptr);
	const int count = listDataDesc->count;
	for (int i = 0; i < count; i++) {
		LVCOLUMN lvColumn;
		lvColumn.mask = LVCF_FMT | LVCF_TEXT;
		lvColumn.fmt = LVCFMT_LEFT;
		if (listDataDesc->names[i]) {
			// TODO: Support for RP_UTF8?
			// NOTE: pszText is LPWSTR, not LPCWSTR...
			lvColumn.pszText = (LPWSTR)listDataDesc->names[i];
		} else {
			// Don't show this column.
			// FIXME: Zero-width column is a bad hack...
			lvColumn.pszText = L"";
			lvColumn.mask |= LVCF_WIDTH;
			lvColumn.cx = 0;
		}

		ListView_InsertColumn(hWnd, i, &lvColumn);
	}

	// Add the row data.
	const RomFields::ListData *listData = data->list_data;
	for (int i = 0; i < (int)listData->data.size(); i++) {
		LVITEM lvItem;
		lvItem.mask = LVIF_TEXT;
		lvItem.iItem = i;

		const vector<rp_string> &data_row = listData->data.at(i);
		int field = 0;
		for (auto iter = data_row.cbegin(); iter != data_row.cend(); ++iter, ++field) {
			lvItem.iSubItem = field;
			// TODO: Support for RP_UTF8?
			// NOTE: pszText is LPWSTR, not LPCWSTR...
			lvItem.pszText = (LPWSTR)(*iter).c_str();
			if (field == 0) {
				// Field 0: Insert the item.
				ListView_InsertItem(hWnd, &lvItem);
			} else {
				// Fields 1 and higher: Set the subitem.
				ListView_SetItem(hWnd, &lvItem);
			}
		}
	}

	// Resize all of the columns.
	// TODO: Do this on system theme change?
	for (int i = 0; i < count; i++) {
		ListView_SetColumnWidth(hWnd, i, LVSCW_AUTOSIZE_USEHEADER);
	}
}

/**
 * Initialize a Date/Time field.
 * This function internally calls initString().
 * @param hDlg		[in] Dialog window.
 * @param pt_start	[in] Starting position, in pixels.
 * @param idx		[in] Field index.
 * @param size		[in] Width and height for a single line label.
 * @return Field height, in pixels.
 */
int RP_ShellPropSheetExt_Private::initDateTime(HWND hDlg,
	const POINT &pt_start, int idx, const SIZE &size)
{
	if (!hDlg)
		return 0;

	const RomFields *fields = romData->fields();
	if (!fields)
		return 0;

	const RomFields::Desc *desc = fields->desc(idx);
	const RomFields::Data *data = fields->data(idx);
	if (!desc || !data)
		return 0;
	if (desc->type != RomFields::RFT_DATETIME ||
	    data->type != RomFields::RFT_DATETIME)
		return 0;
	if (!desc->name || desc->name[0] == '\0')
		return 0;

	if (data->date_time == -1) {
		// Invalid date/time.
		return initString(hDlg, pt_start, idx, size, L"Unknown");
	}

	// Format the date/time using the system locale.
	const RomFields::DateTimeDesc *const dateTimeDesc = desc->date_time;
	assert(dateTimeDesc != nullptr);
	wchar_t dateTimeStr[256];
	int start_pos = 0;
	int cchBuf = ARRAY_SIZE(dateTimeStr);

	// Convert from Unix time to Win32 SYSTEMTIME.
	SYSTEMTIME st;
	UnixTimeToSystemTime(data->date_time, &st);

	// At least one of Date and/or Time must be set.
	assert((dateTimeDesc->flags &
		(RomFields::RFT_DATETIME_HAS_DATE | RomFields::RFT_DATETIME_HAS_TIME)) != 0);

	if (!(dateTimeDesc->flags & RomFields::RFT_DATETIME_IS_UTC)) {
		// Convert to the current timezone.
		SYSTEMTIME st_utc = st;
		BOOL ret = SystemTimeToTzSpecificLocalTime(nullptr, &st_utc, &st);
		if (!ret) {
			// Conversion failed.
			return 0;
		}
	}

	if (dateTimeDesc->flags & RomFields::RFT_DATETIME_HAS_DATE) {
		// Format the date.
		int ret = GetDateFormat(
			MAKELCID(LOCALE_USER_DEFAULT, SORT_DEFAULT),
			DATE_SHORTDATE,
			&st, nullptr, &dateTimeStr[start_pos], cchBuf);
		if (ret <= 0) {
			// Error!
			return 0;
		}

		// Adjust the buffer position.
		// NOTE: ret includes the NULL terminator.
		start_pos += ret-1;
		cchBuf -= ret-1;
	}

	if (dateTimeDesc->flags & RomFields::RFT_DATETIME_HAS_TIME) {
		// Format the time.
		if (start_pos > 0 && cchBuf >= 1) {
			// Add a space.
			dateTimeStr[start_pos] = L' ';
			dateTimeStr[start_pos+1] = 0;
			start_pos++;
			cchBuf--;
		}

		int ret = GetTimeFormat(
			MAKELCID(LOCALE_USER_DEFAULT, SORT_DEFAULT),
			0, &st, nullptr, &dateTimeStr[start_pos], cchBuf);
		if (ret <= 0) {
			// Error!
			return 0;
		}

		// Adjust the buffer position.
		// NOTE: ret includes the NULL terminator.
		start_pos += ret-1;
		cchBuf -= ret-1;
	}

	if (start_pos == 0) {
		// Empty string.
		// Something failed...
		return 0;
	}

	// Initialize the string.
	return initString(hDlg, pt_start, idx, size, dateTimeStr);
}

/**
 * Monospaced font enumeration procedure.
 * @param lpelfe Enumerated font information.
 * @param lpntme Font metrics.
 * @param FontType Font type.
 * @param lParam Pointer to RP_ShellPropSheetExt_Private.
 */
int CALLBACK RP_ShellPropSheetExt_Private::MonospacedFontEnumProc(
	const LOGFONT *lpelfe, const TEXTMETRIC *lpntme,
	DWORD FontType, LPARAM lParam)
{
	RP_ShellPropSheetExt_Private *d =
		reinterpret_cast<RP_ShellPropSheetExt_Private*>(lParam);

	// Check the font attributes:
	// - Must be monospaced.
	// - Must be horizontally-oriented.
	if ((lpelfe->lfPitchAndFamily & FIXED_PITCH) &&
	     lpelfe->lfFaceName[0] != '@')
	{
		d->monospaced_fonts.insert(lpelfe->lfFaceName);
	}

	// Continue enumeration.
	return 1;
}

/**
 * Initialize the bold font.
 * @param hFont Base font.
 */
void RP_ShellPropSheetExt_Private::initBoldFont(HFONT hFont)
{
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
 * Initialize the monospaced font.
 * @param hFont Base font.
 */
void RP_ShellPropSheetExt_Private::initMonospacedFont(HFONT hFont)
{
	if (!hFont) {
		// No base font...
		return;
	}

	// Get the current ClearType setting.
	bool bIsClearType = false;
	BOOL bFontSmoothing;
	BOOL bRet = SystemParametersInfo(SPI_GETFONTSMOOTHING, 0, &bFontSmoothing, 0);
	if (bRet) {
		UINT uiFontSmoothingType;
		bRet = SystemParametersInfo(SPI_GETFONTSMOOTHINGTYPE, 0, &uiFontSmoothingType, 0);
		if (bRet) {
			bIsClearType = (bFontSmoothing && (uiFontSmoothingType == FE_FONTSMOOTHINGCLEARTYPE));
		}
	}

	if (hFontMono) {
		// Font exists. Only re-create it if the ClearType setting has changed.
		if (bIsClearType == bPrevIsClearType) {
			// ClearType setting has not changed.
			return;
		}
	} else {
		// Font hasn't been created yet.
		if (GetObject(hFont, sizeof(lfFontMono), &lfFontMono) == 0) {
			// Unable to obtain the LOGFONT.
			return;
		}

		// Enumerate all monospaced fonts.
		// Reference: http://www.catch22.net/tuts/fixed-width-font-enumeration
		monospaced_fonts.clear();
#if !defined(_MSC_VER) || _MSC_VER >= 1700
		monospaced_fonts.reserve(64);
#endif
		LOGFONT lfEnumFonts;
		memset(&lfEnumFonts, 0, sizeof(lfEnumFonts));
		lfEnumFonts.lfCharSet = DEFAULT_CHARSET;
		lfEnumFonts.lfPitchAndFamily = FIXED_PITCH | FF_DONTCARE;
		HDC hdc = GetDC(nullptr);
		EnumFontFamiliesEx(hdc, &lfEnumFonts, MonospacedFontEnumProc,
			reinterpret_cast<LPARAM>(this), 0);
		ReleaseDC(nullptr, hdc);

		// Fonts to try.
		static const wchar_t *const fonts[] = {
			L"DejaVu Sans Mono",
			L"Consolas",
			L"Lucida Console",
			L"Fixedsys Excelsior 3.01",
			L"Fixedsys Excelsior 3.00",
			L"Fixedsys Excelsior 3.0",
			L"Fixedsys Excelsior 2.00",
			L"Fixedsys Excelsior 2.0",
			L"Fixedsys Excelsior 1.00",
			L"Fixedsys Excelsior 1.0",
			L"Fixedsys",
			L"Courier New",
		};

		const wchar_t *font = nullptr;

		for (int i = 0; i < ARRAY_SIZE(fonts); i++) {
			if (monospaced_fonts.find(fonts[i]) != monospaced_fonts.end()) {
				// Found a font.
				font = fonts[i];
				break;
			}
		}

		// We don't need the enumerated fonts anymore.
		monospaced_fonts.clear();

		if (!font) {
			// Monospaced font not found.
			return;
		}

		// Adjust the font and create a new one.
		wcscpy(lfFontMono.lfFaceName, font);
	}

	// Create the monospaced font.
	// If ClearType is enabled, use DEFAULT_QUALITY;
	// otherwise, use NONANTIALIASED_QUALITY.
	lfFontMono.lfQuality = (bIsClearType ? DEFAULT_QUALITY : NONANTIALIASED_QUALITY);
	HFONT hFontMonoNew = CreateFontIndirect(&lfFontMono);
	if (!hFontMonoNew) {
		// Unable to create new font.
		return;
	}

	// Update all existing monospaced controls.
	for (auto iter = hwndMonoControls.cbegin(); iter != hwndMonoControls.cend(); ++iter) {
		SetWindowFont(*iter, hFontMonoNew, FALSE);
	}

	// Delete the old font and save the new one.
	HFONT hFontMonoOld = hFontMono;
	hFontMono = hFontMonoNew;
	if (hFontMonoOld) {
		DeleteFont(hFontMonoOld);
	}
	bPrevIsClearType = bIsClearType;
}

/**
 * Initialize the dialog.
 * Called by WM_INITDIALOG.
 * @param hDlg Dialog window.
 */
void RP_ShellPropSheetExt_Private::initDialog(HWND hDlg)
{
	if (!romData) {
		// No ROM data loaded.
		return;
	}

	// Get the fields.
	const RomFields *fields = romData->fields();
	if (!fields) {
		// No fields.
		// TODO: Show an error?
		return;
	}
	const int count = fields->count();

	// Make sure we have all required window classes available.
	// Reference: https://msdn.microsoft.com/en-us/library/windows/desktop/bb775507(v=vs.85).aspx
	INITCOMMONCONTROLSEX initCommCtrl;
	initCommCtrl.dwSize = sizeof(initCommCtrl);
	initCommCtrl.dwICC = ICC_LISTVIEW_CLASSES | ICC_LINK_CLASS;
	// TODO: Also ICC_STANDARD_CLASSES on XP+?
	InitCommonControlsEx(&initCommCtrl);

	// Dialog font and device context.
	HFONT hFont = GetWindowFont(hDlg);
	HDC hDC = GetDC(hDlg);
	HFONT hFontOrig = SelectFont(hDC, hFont);

	// Initialize the bold and monospaced fonts.
	initBoldFont(hFont);
	initMonospacedFont(hFont);

	// Determine the maximum length of all field names.
	// TODO: Line breaks?
	int max_text_width = 0;
	SIZE textSize;
	for (int i = 0; i < count; i++) {
		const RomFields::Desc *desc = fields->desc(i);
		const RomFields::Data *data = fields->data(i);
		if (!desc || !data)
			continue;
		if (desc->type != data->type)
			continue;
		if (!desc->name || desc->name[0] == '\0')
			continue;

		// Make sure this is a UTF-16 string.
		wstring s_name = RP2W_c(desc->name);

		// TODO: Handle STRF_WARNING?

		// Get the width of this specific entry.
		// TODO: Use measureTextSize()?
		GetTextExtentPoint32(hDC, s_name.data(), (int)s_name.size(), &textSize);
		if (textSize.cx > max_text_width) {
			max_text_width = textSize.cx;
		}
	}

	// Add additional spacing for the ':'.
	// TODO: Use measureTextSize()?
	GetTextExtentPoint32(hDC, L":  ", 3, &textSize);
	max_text_width += textSize.cx;

	// Release the DC.
	SelectFont(hDC, hFontOrig);
	ReleaseDC(hDlg, hDC);

	// Create the ROM field widgets.
	// Each static control is max_text_width pixels wide
	// and 8 DLUs tall, plus 4 vertical DLUs for spacing.
	RECT tmpRect = {0, 0, 0, 8+4};
	MapDialogRect(hDlg, &tmpRect);
	const SIZE descSize = {max_text_width, tmpRect.bottom};

	// Current position.
	// 7x7 DLU margin is recommended by the Windows UX guidelines.
	// Reference: http://stackoverflow.com/questions/2118603/default-dialog-padding
	tmpRect.left = 7; tmpRect.top = 7;
	tmpRect.right = 8; tmpRect.bottom = 8;
	MapDialogRect(hDlg, &tmpRect);
	POINT curPt = {tmpRect.left, tmpRect.top};

	// Width available for the value widget(s).
	RECT dlgRect;
	GetClientRect(hDlg, &dlgRect);
	LONG x = GetDialogBaseUnits();
	const int dlg_value_width = dlgRect.right - (curPt.x * 2) - descSize.cx;

	// Create the header row.
	const SIZE full_width_size = {dlg_value_width + descSize.cx, descSize.cy};
	curPt.y += createHeaderRow(hDlg, curPt, full_width_size);

	for (int idx = 0; idx < count; idx++) {
		const RomFields::Desc *desc = fields->desc(idx);
		const RomFields::Data *data = fields->data(idx);
		if (!desc || !data)
			continue;
		if (desc->type != data->type)
			continue;
		if (!desc->name || desc->name[0] == '\0')
			continue;

		// Append a ":" to the description.
		// TODO: Localization.
		wstring desc_text = RP2W_c(desc->name);
		desc_text += L':';

		// Create the static text widget. (FIXME: Disable mnemonics?)
		HWND hStatic = CreateWindow(WC_STATIC, desc_text.c_str(),
			WS_CHILD | WS_VISIBLE | SS_LEFT,
			curPt.x, curPt.y, descSize.cx, descSize.cy,
			hDlg, (HMENU)(INT_PTR)(IDC_STATIC_DESC(idx)),
			nullptr, nullptr);
		SetWindowFont(hStatic, hFont, FALSE);

		// Create the value widget.
		int field_cy = descSize.cy;	// Default row size.
		const POINT pt_start = {curPt.x + descSize.cx, curPt.y};
		HWND hDlgItem;
		switch (desc->type) {
			case RomFields::RFT_INVALID:
				// No data here.
				DestroyWindow(hStatic);
				field_cy = 0;
				break;

			case RomFields::RFT_STRING: {
				// String data.
				SIZE size = {dlg_value_width, field_cy};
				field_cy = initString(hDlg, pt_start, idx, size, nullptr);
				if (field_cy == 0) {
					// initString() failed.
					// Remove the description label.
					DestroyWindow(hStatic);
				}
				break;
			}

			case RomFields::RFT_BITFIELD:
				// Create checkboxes starting at the current point.
				field_cy = initBitfield(hDlg, pt_start, idx);
				if (field_cy == 0) {
					// initBitfield() failed.
					// Remove the description label.
					DestroyWindow(hStatic);
				}
				break;

			case RomFields::RFT_LISTDATA:
				// Increase row height to 72 DLU.
				// descSize is 8+4 DLU (12); 72/12 == 6
				// FIXME: The last row seems to be cut off with the
				// Windows XP Luna theme, but not Windows Classic...
				field_cy *= 6;

				// Create a ListView widget.
				hDlgItem = CreateWindowEx(WS_EX_LEFT | WS_EX_CLIENTEDGE,
					WC_LISTVIEW, RP2W_c(data->str),
					WS_CHILD | WS_VISIBLE | WS_TABSTOP | LVS_ALIGNLEFT | LVS_REPORT,
					pt_start.x, pt_start.y,
					dlg_value_width, field_cy,
					hDlg, (HMENU)(INT_PTR)(IDC_RFT_LISTDATA(idx)),
					nullptr, nullptr);
				SetWindowFont(hDlgItem, hFont, FALSE);

				// Initialize the ListView data.
				initListView(hDlgItem, desc, fields->data(idx));
				break;

			case RomFields::RFT_DATETIME: {
				// Date/Time in Unix format.
				SIZE size = {dlg_value_width, field_cy};
				field_cy = initDateTime(hDlg, pt_start, idx, size);
				if (field_cy == 0) {
					// initDateTime() failed.
					// Remove the description label.
					DestroyWindow(hStatic);
				}
				break;
			}

			default:
				// Unsupported data type.
				assert(!"Unsupported RomFields::RomFieldsType.");
				DestroyWindow(hStatic);
				field_cy = 0;
				break;
		}

		// Next row.
		curPt.y += field_cy;
	}
}

/** RP_ShellPropSheetExt **/

RP_ShellPropSheetExt::RP_ShellPropSheetExt()
	: d_ptr(new RP_ShellPropSheetExt_Private(this))
{ }

RP_ShellPropSheetExt::~RP_ShellPropSheetExt()
{
	delete d_ptr;
}

/** IUnknown **/
// Reference: https://msdn.microsoft.com/en-us/library/office/cc839627.aspx

IFACEMETHODIMP RP_ShellPropSheetExt::QueryInterface(REFIID riid, LPVOID *ppvObj)
{
	// Always set out parameter to NULL, validating it first.
	if (!ppvObj)
		return E_INVALIDARG;

	// Check if this interface is supported.
	// NOTE: static_cast<> is required due to vtable shenanigans.
	// Also, IID_IUnknown must always return the same pointer.
	// References:
	// - http://stackoverflow.com/questions/1742848/why-exactly-do-i-need-an-explicit-upcast-when-implementing-queryinterface-in-a
	// - http://stackoverflow.com/a/2812938
	if (riid == IID_IUnknown || riid == IID_IShellExtInit) {
		*ppvObj = static_cast<IShellExtInit*>(this);
	} else if (riid == IID_IShellPropSheetExt) {
		*ppvObj = static_cast<IShellPropSheetExt*>(this);
	} else {
		// Interface is not supported.
		*ppvObj = nullptr;
		return E_NOINTERFACE;
	}

	// Make sure we count this reference.
	AddRef();
	return NOERROR;
}

/**
 * Register the COM object.
 * @return ERROR_SUCCESS on success; Win32 error code on error.
 */
LONG RP_ShellPropSheetExt::RegisterCLSID(void)
{
	static const wchar_t description[] = L"ROM Properties Page - Property Sheet";
	extern const wchar_t RP_ProgID[];

	// Convert the CLSID to a string.
	wchar_t clsid_str[48];	// maybe only 40 is needed?
	LONG lResult = StringFromGUID2(__uuidof(RP_ShellPropSheetExt), clsid_str, sizeof(clsid_str)/sizeof(clsid_str[0]));
	if (lResult <= 0) {
		return ERROR_INVALID_PARAMETER;
	}

	// Register the COM object.
	lResult = RegKey::RegisterComObject(__uuidof(RP_ShellPropSheetExt), RP_ProgID, description);
	if (lResult != ERROR_SUCCESS) {
		return lResult;
	}

	// Register as an "approved" shell extension.
	lResult = RegKey::RegisterApprovedExtension(__uuidof(RP_ShellPropSheetExt), description);
	if (lResult != ERROR_SUCCESS) {
		return lResult;
	}

	// COM object registered.
	return ERROR_SUCCESS;
}

/**
 * Register the file type handler.
 * @param hkey_Assoc File association key to register under.
 * @return ERROR_SUCCESS on success; Win32 error code on error.
 */
LONG RP_ShellPropSheetExt::RegisterFileType(RegKey &hkey_Assoc)
{
	extern const wchar_t RP_ProgID[];

	// Convert the CLSID to a string.
	wchar_t clsid_str[48];	// maybe only 40 is needed?
	LONG lResult = StringFromGUID2(__uuidof(RP_ShellPropSheetExt), clsid_str, sizeof(clsid_str)/sizeof(clsid_str[0]));
	if (lResult <= 0) {
		return ERROR_INVALID_PARAMETER;
	}

	// Register as a property sheet handler for this file association.

	// Create/open the "ShellEx" key.
	RegKey hkcr_ShellEx(hkey_Assoc, L"ShellEx", KEY_WRITE, true);
	if (!hkcr_ShellEx.isOpen()) {
		return hkcr_ShellEx.lOpenRes();
	}
	// Create/open the PropertySheetHandlers key.
	RegKey hkcr_PropertySheetHandlers(hkcr_ShellEx, L"PropertySheetHandlers", KEY_WRITE, true);
	if (!hkcr_PropertySheetHandlers.isOpen()) {
		return hkcr_PropertySheetHandlers.lOpenRes();
	}

	// Create/open the "rom-properties" property sheet handler key.
	// NOTE: This always uses RP_ProgID[], not the specified progID.
	RegKey hkcr_PropSheet_RomProperties(hkcr_PropertySheetHandlers, RP_ProgID, KEY_WRITE, true);
	if (!hkcr_PropSheet_RomProperties.isOpen()) {
		return hkcr_PropSheet_RomProperties.lOpenRes();
	}
	// Set the default value to this CLSID.
	lResult = hkcr_PropSheet_RomProperties.write(nullptr, clsid_str);
	if (lResult != ERROR_SUCCESS) {
		return lResult;
	}

	// File type handler registered.
	return ERROR_SUCCESS;
}

/**
 * Unregister the COM object.
 * @return ERROR_SUCCESS on success; Win32 error code on error.
 */
LONG RP_ShellPropSheetExt::UnregisterCLSID(void)
{
	extern const wchar_t RP_ProgID[];

	// Unegister the COM object.
	LONG lResult = RegKey::UnregisterComObject(__uuidof(RP_ShellPropSheetExt), RP_ProgID);
	if (lResult != ERROR_SUCCESS) {
		return lResult;
	}

	// TODO
	return ERROR_SUCCESS;
}

/**
 * Unregister the file type handler.
 * @param hkey_Assoc File association key to register under.
 * @return ERROR_SUCCESS on success; Win32 error code on error.
 */
LONG RP_ShellPropSheetExt::UnregisterFileType(RegKey &hkey_Assoc)
{
	extern const wchar_t RP_ProgID[];

	// Convert the CLSID to a string.
	wchar_t clsid_str[48];	// maybe only 40 is needed?
	LONG lResult = StringFromGUID2(__uuidof(RP_ShellPropSheetExt), clsid_str, sizeof(clsid_str)/sizeof(clsid_str[0]));
	if (lResult <= 0) {
		return ERROR_INVALID_PARAMETER;
	}

	// Unregister as a property sheet handler for this file association.

	// Open the "ShellEx" key.
	RegKey hkcr_ShellEx(hkey_Assoc, L"ShellEx", KEY_READ, false);
	if (!hkcr_ShellEx.isOpen()) {
		// ERROR_FILE_NOT_FOUND is acceptable here.
		if (hkcr_ShellEx.lOpenRes() == ERROR_FILE_NOT_FOUND) {
			return ERROR_SUCCESS;
		}
		return hkcr_ShellEx.lOpenRes();
	}
	// Open the PropertySheetHandlers key.
	RegKey hkcr_PropertySheetHandlers(hkcr_ShellEx, L"PropertySheetHandlers", KEY_READ, false);
	if (!hkcr_PropertySheetHandlers.isOpen()) {
		// ERROR_FILE_NOT_FOUND is acceptable here.
		if (hkcr_PropertySheetHandlers.lOpenRes() == ERROR_FILE_NOT_FOUND) {
			return ERROR_SUCCESS;
		}
		return hkcr_PropertySheetHandlers.lOpenRes();
	}

	// Open the "rom-properties" property sheet handler key.
	// NOTE: This always uses RP_ProgID[], not the specified progID.
	RegKey hkcr_PropSheet_RomProperties(hkcr_PropertySheetHandlers, RP_ProgID, KEY_READ, false);
	if (!hkcr_PropSheet_RomProperties.isOpen()) {
		// ERROR_FILE_NOT_FOUND is acceptable here.
		if (hkcr_PropSheet_RomProperties.lOpenRes() == ERROR_FILE_NOT_FOUND) {
			return ERROR_SUCCESS;
		}
		return hkcr_PropSheet_RomProperties.lOpenRes();
	}
	// Check if the default value matches the CLSID.
	wstring str_IShellPropSheetExt = hkcr_PropSheet_RomProperties.read(nullptr);
	if (str_IShellPropSheetExt == clsid_str) {
		// Default value matches.
		// Remove the subkey.
		hkcr_PropSheet_RomProperties.close();
		lResult = hkcr_PropertySheetHandlers.deleteSubKey(RP_ProgID);
		if (lResult != ERROR_SUCCESS) {
			return lResult;
		}
	} else {
		// Default value does not match.
		// We're done here.
		return hkcr_PropSheet_RomProperties.lOpenRes();
	}

	// Check if PropertySheetHandlers is empty.
	// TODO: Error handling.
	if (hkcr_PropertySheetHandlers.isKeyEmpty()) {
		// No subkeys. Delete this key.
		hkcr_PropertySheetHandlers.close();
		hkcr_ShellEx.deleteSubKey(L"PropertySheetHandlers");
	}

	// File type handler unregistered.
	return ERROR_SUCCESS;
}

/** IShellExtInit **/

/** IShellPropSheetExt **/
// References:
// - https://msdn.microsoft.com/en-us/library/windows/desktop/bb775094(v=vs.85).aspx
IFACEMETHODIMP RP_ShellPropSheetExt::Initialize(
	LPCITEMIDLIST pidlFolder, LPDATAOBJECT pDataObj, HKEY hKeyProgID)
{
	// Based on CppShellExtPropSheetHandler.
	// https://code.msdn.microsoft.com/windowsapps/CppShellExtPropSheetHandler-d93b49b7
	if (!pDataObj) {
		return E_INVALIDARG;
	}

	HRESULT hr = E_FAIL;
	FORMATETC fe = { CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
	STGMEDIUM stm;

	// The pDataObj pointer contains the objects being acted upon. In this 
	// example, we get an HDROP handle for enumerating the selected files and 
	// folders.
	if (FAILED(pDataObj->GetData(&fe, &stm)))
		return E_FAIL;

	// Get an HDROP handle.
	HDROP hDrop = static_cast<HDROP>(GlobalLock(stm.hGlobal));
	if (!hDrop) {
		ReleaseStgMedium(&stm);
		return E_FAIL;
	}

	// Determine how many files are involved in this operation. This
	// code sample displays the custom context menu item when only
	// one file is selected.
	UINT nFiles = DragQueryFile(hDrop, 0xFFFFFFFF, NULL, 0);
	if (nFiles == 1) {
		// Get the path of the file.
		UINT cchFilename = DragQueryFile(hDrop, 0, nullptr, 0);
		if (cchFilename > 0) {
			cchFilename++;	// Add one for the NULL terminator.
			wchar_t *filename = (wchar_t*)malloc(cchFilename * sizeof(wchar_t));
			cchFilename = DragQueryFile(hDrop, 0, filename, cchFilename);
			if (cchFilename > 0) {
				// Open the file.
				unique_ptr<IRpFile> file(new RpFile(
					W2RP_cl(filename, cchFilename),
					RpFile::FM_OPEN_READ));
				if (file && file->isOpen()) {
					// Get the appropriate RomData class for this ROM.
					// file is dup()'d by RomData.
					RP_D(RP_ShellPropSheetExt);
					RomData *romData = RomDataFactory::getInstance(file.get());
					if (romData) {
						// Make sure the existing RomData is unreferenced.
						// TODO: If the filename matches, don't reopen?
						if (d->romData) {
							d->romData->unref();
						}
						d->romData = romData;
						hr = S_OK;
					} else {
						// Could not open the RomData object.
						hr = E_FAIL;
					}
				}
			}
			free(filename);
		}
	}

	GlobalUnlock(stm.hGlobal);
	ReleaseStgMedium(&stm);

	// If any value other than S_OK is returned from the method, the property 
	// sheet is not displayed.
	return hr;
}

/** IShellPropSheetExt **/

IFACEMETHODIMP RP_ShellPropSheetExt::AddPages(LPFNADDPROPSHEETPAGE pfnAddPage, LPARAM lParam)
{
	// Based on CppShellExtPropSheetHandler.
	// https://code.msdn.microsoft.com/windowsapps/CppShellExtPropSheetHandler-d93b49b7

	// Create a property sheet page.
	extern HINSTANCE g_hInstance;
	PROPSHEETPAGE psp;
	psp.dwSize = sizeof(psp);
	psp.dwFlags = PSP_USECALLBACK | PSP_USETITLE;
	psp.hInstance = g_hInstance;
	psp.pszTemplate = MAKEINTRESOURCE(IDD_PROPERTY_SHEET);
	psp.pszIcon = nullptr;
	psp.pszTitle = L"ROM Properties";
	psp.pfnDlgProc = DlgProc;
	psp.pcRefParent = nullptr;
	psp.pfnCallback = CallbackProc;
	psp.lParam = reinterpret_cast<LPARAM>(this);

	HPROPSHEETPAGE hPage = CreatePropertySheetPage(&psp);
	if (hPage == NULL) {
		return E_OUTOFMEMORY;
	}

	// The property sheet page is then added to the property sheet by calling 
	// the callback function (LPFNADDPROPSHEETPAGE pfnAddPage) passed to 
	// IShellPropSheetExt::AddPages.
	if (pfnAddPage(hPage, lParam)) {
		// By default, after AddPages returns, the shell releases its 
		// IShellPropSheetExt interface and the property page cannot access the
		// extension object. However, it is sometimes desirable to be able to use 
		// the extension object, or some other object, from the property page. So 
		// we increase the reference count and maintain this object until the 
		// page is released in PropPageCallbackProc where we call Release upon 
		// the extension.
		this->AddRef();
	} else {
		DestroyPropertySheetPage(hPage);
		return E_FAIL;
	}

	// If any value other than S_OK is returned from the method, the property 
	// sheet is not displayed.
	return S_OK;
}

IFACEMETHODIMP RP_ShellPropSheetExt::ReplacePage(UINT uPageID, LPFNADDPROPSHEETPAGE pfnReplaceWith, LPARAM lParam)
{
	// Not used.
	((void)uPageID);
	((void)pfnReplaceWith);
	((void)lParam);
	return E_NOTIMPL;
}

/** Property sheet callback functions. **/

//
//   FUNCTION: FilePropPageDlgProc
//
//   PURPOSE: Processes messages for the property page.
//
INT_PTR CALLBACK RP_ShellPropSheetExt::DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	// Based on CppShellExtPropSheetHandler.
	// https://code.msdn.microsoft.com/windowsapps/CppShellExtPropSheetHandler-d93b49b7
	switch (uMsg) {
		case WM_INITDIALOG: {
			// Get the pointer to the property sheet page object. This is 
			// contained in the LPARAM of the PROPSHEETPAGE structure.
			LPPROPSHEETPAGE pPage = reinterpret_cast<LPPROPSHEETPAGE>(lParam);
			if (!pPage)
				return TRUE;

			// Access the property sheet extension from property page.
			RP_ShellPropSheetExt *pExt = reinterpret_cast<RP_ShellPropSheetExt*>(pPage->lParam);
			if (!pExt)
				return TRUE;
			RP_ShellPropSheetExt_Private *const d = pExt->d_ptr;

			// Store the object pointer with this particular page dialog.
			SetProp(hDlg, EXT_POINTER_PROP, static_cast<HANDLE>(pExt));
			// Save handles for later.
			d->hDlgProps = GetParent(hDlg);
			d->hDlgSheet = hDlg;
			// Initialize the dialog.
			d->initDialog(hDlg);

			// Make sure the underlying file handle is closed,
			// since we don't need it once the RomData has been
			// loaded by RomDataView.
			d->romData->close();
			return TRUE;
		}

		case WM_DESTROY: {
			RP_ShellPropSheetExt *pExt = static_cast<RP_ShellPropSheetExt*>(
				GetProp(hDlg, EXT_POINTER_PROP));
			if (pExt) {
				// Stop the animation timer.
				RP_ShellPropSheetExt_Private *const d = pExt->d_ptr;
				d->stopAnimTimer();
			}

			// Remove the EXT_POINTER_PROP property from the page. 
			// The EXT_POINTER_PROP property stored the pointer to the 
			// FilePropSheetExt object.
			RemoveProp(hDlg, EXT_POINTER_PROP);
			return TRUE;
		}

		case WM_NOTIFY: {
			RP_ShellPropSheetExt *pExt = static_cast<RP_ShellPropSheetExt*>(
				GetProp(hDlg, EXT_POINTER_PROP));
			if (!pExt) {
				// No RP_ShellPropSheetExt. Can't do anything...
				return FALSE;
			}

			RP_ShellPropSheetExt_Private *const d = pExt->d_ptr;
			LPPSHNOTIFY lppsn = reinterpret_cast<LPPSHNOTIFY>(lParam);
			switch (lppsn->hdr.code) {
				case PSN_SETACTIVE:
					d->startAnimTimer();
					break;

				case PSN_KILLACTIVE:
					d->stopAnimTimer();
					break;

				case NM_CLICK:
				case NM_RETURN: {
					// Check if this is a SysLink control.
					RP_ShellPropSheetExt_Private *const d = pExt->d_ptr;
					if (d->hwndSysLinkControls.find(lppsn->hdr.hwndFrom) !=
					    d->hwndSysLinkControls.end())
					{
						// It's a SysLink control.
						// Open the URL.
						PNMLINK pNMLink = reinterpret_cast<PNMLINK>(lParam);
						ShellExecute(nullptr, L"open", pNMLink->item.szUrl, nullptr, nullptr, SW_SHOW);
					}
					break;
				}

				default:
					break;
			}

			// Continue normal processing.
			break;
		}

		case WM_PAINT: {
			RP_ShellPropSheetExt *pExt = static_cast<RP_ShellPropSheetExt*>(
				GetProp(hDlg, EXT_POINTER_PROP));
			if (!pExt) {
				// No RP_ShellPropSheetExt. Can't do anything...
				return FALSE;
			}

			RP_ShellPropSheetExt_Private *const d = pExt->d_ptr;
			if (!d->hbmpBanner && !d->hbmpIconFrames[0]) {
				// Nothing to draw...
				break;
			}

			PAINTSTRUCT ps;
			HDC hdc = BeginPaint(hDlg, &ps);

			// Memory DC for BitBlt.
			HDC hdcMem = CreateCompatibleDC(hdc);

			// Draw the banner.
			if (d->hbmpBanner) {
				HBITMAP hbmOld = SelectBitmap(hdcMem, d->hbmpBanner);
				BitBlt(hdc, d->ptBanner.x, d->ptBanner.y,
					d->szBanner.cx, d->szBanner.cy,
					hdcMem, 0, 0, SRCCOPY);
				SelectBitmap(hdcMem, hbmOld);
			}

			// Draw the icon.
			if (d->hbmpIconFrames[d->last_frame_number]) {
				HBITMAP hbmOld = SelectBitmap(hdcMem, d->hbmpIconFrames[d->last_frame_number]);
				BitBlt(hdc, d->rectIcon.left, d->rectIcon.top,
					d->szIcon.cx, d->szIcon.cy,
					hdcMem, 0, 0, SRCCOPY);
				SelectBitmap(hdcMem, hbmOld);
			}

			DeleteDC(hdcMem);
			EndPaint(hDlg, &ps);

			return TRUE;
		}

		case WM_SYSCOLORCHANGE:
		case WM_THEMECHANGED: {
			// Reload the images.
			RP_ShellPropSheetExt *pExt = static_cast<RP_ShellPropSheetExt*>(
				GetProp(hDlg, EXT_POINTER_PROP));
			if (pExt) {
				RP_ShellPropSheetExt_Private *const d = pExt->d_ptr;
				d->loadImages(hDlg);
			}
			break;
		}

		case WM_NCPAINT: {
			// Update the monospaced font.
			RP_ShellPropSheetExt *pExt = static_cast<RP_ShellPropSheetExt*>(
				GetProp(hDlg, EXT_POINTER_PROP));
			if (pExt) {
				RP_ShellPropSheetExt_Private *const d = pExt->d_ptr;
				HFONT hFont = GetWindowFont(hDlg);
				d->initMonospacedFont(hFont);
			}
			break;
		}

		case WM_CTLCOLORSTATIC: {
			RP_ShellPropSheetExt *pExt = static_cast<RP_ShellPropSheetExt*>(
				GetProp(hDlg, EXT_POINTER_PROP));
			if (pExt) {
				RP_ShellPropSheetExt_Private *const d = pExt->d_ptr;
				if (d->hwndWarningControls.find(reinterpret_cast<HWND>(lParam)) !=
				    d->hwndWarningControls.end())
				{
					// Set the "Warning" color.
					HDC hdc = reinterpret_cast<HDC>(wParam);
					SetTextColor(hdc, RGB(255, 0, 0));
				}
			}
			break;
		}

		default:
			break;
	}

	return FALSE; // Let system deal with other messages
}


//
//   FUNCTION: FilePropPageCallbackProc
//
//   PURPOSE: Specifies an application-defined callback function that a property
//            sheet calls when a page is created and when it is about to be 
//            destroyed. An application can use this function to perform 
//            initialization and cleanup operations for the page.
//
UINT CALLBACK RP_ShellPropSheetExt::CallbackProc(HWND hWnd, UINT uMsg, LPPROPSHEETPAGE ppsp)
{
	switch (uMsg) {
		case PSPCB_CREATE: {
			// Must return TRUE to enable the page to be created.
			return TRUE;
		}

		case PSPCB_RELEASE: {
			// When the callback function receives the PSPCB_RELEASE notification, 
			// the ppsp parameter of the PropSheetPageProc contains a pointer to 
			// the PROPSHEETPAGE structure. The lParam member of the PROPSHEETPAGE 
			// structure contains the extension pointer which can be used to 
			// release the object.

			// Release the property sheet extension object. This is called even 
			// if the property page was never actually displayed.
			RP_ShellPropSheetExt *pExt = reinterpret_cast<RP_ShellPropSheetExt*>(ppsp->lParam);
			if (pExt) {
				pExt->Release();
			}
			break;
		}

		default:
			break;
	}

	return FALSE;
}

/**
 * Subclass procedure for ES_MULTILINE EDIT controls.
 * @param hWnd		Control handle.
 * @param uMsg		Message.
 * @param wParam	WPARAM
 * @param lParam	LPARAM
 * @param uIdSubclass	Subclass ID. (usually the control ID)
 * @param dwRefData	RP_ShellProSheetExt*
 */
LRESULT CALLBACK RP_ShellPropSheetExt::MultilineEditProc(
	HWND hWnd, UINT uMsg,
	WPARAM wParam, LPARAM lParam,
	UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	if (!dwRefData) {
		// No RP_ShellPropSheetExt. Can't do anything...
		return DefSubclassProc(hWnd, uMsg, wParam, lParam);
	}
	RP_ShellPropSheetExt *const pExt =
		reinterpret_cast<RP_ShellPropSheetExt*>(dwRefData);

	switch (uMsg) {
		case WM_KEYDOWN: {
			// Work around Enter/Escape issues.
			// Reference: http://blogs.msdn.com/b/oldnewthing/archive/2007/08/20/4470527.aspx
			RP_ShellPropSheetExt_Private *const d = pExt->d_ptr;
			switch (wParam) {
				case VK_RETURN:
					SendMessage(d->hDlgProps, WM_COMMAND, IDOK, 0);
					return TRUE;

				case VK_ESCAPE:
					SendMessage(d->hDlgProps, WM_COMMAND, IDCANCEL, 0);
					return TRUE;

				default:
					break;
			}
		}

		case WM_NCDESTROY:
			// Remove the window subclass.
			// Reference: https://blogs.msdn.microsoft.com/oldnewthing/20031111-00/?p=41883
			RemoveWindowSubclass(hWnd, MultilineEditProc, uIdSubclass);
			break;

		default:
			break;
	}

	return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

/**
 * Animated icon timer.
 * @param hWnd
 * @param uMsg
 * @param idEvent
 * @param dwTime
 */
void CALLBACK RP_ShellPropSheetExt::AnimTimerProc(HWND hWnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)
{
	if (hWnd == nullptr || idEvent == 0) {
		// Not a valid timer procedure call...
		// - hWnd should not be nullptr.
		// - idEvent should be the 'd' pointer.
		return;
	}

	RP_ShellPropSheetExt_Private *d =
		reinterpret_cast<RP_ShellPropSheetExt_Private*>(idEvent);

	// Next frame.
	int delay = 0;
	int frame = d->iconAnimHelper.nextFrame(&delay);
	if (delay <= 0 || frame < 0) {
		// Invalid frame...
		KillTimer(hWnd, idEvent);
		d->animTimerID = 0;
		return;
	}

	if (frame != d->last_frame_number) {
		// New frame number.
		// Update the icon.
		d->last_frame_number = frame;
		InvalidateRect(d->hDlgSheet, &d->rectIcon, FALSE);
	}

	// Update the timer.
	// TODO: Verify that this affects the next callback.
	SetTimer(hWnd, idEvent, delay, RP_ShellPropSheetExt::AnimTimerProc);
}
