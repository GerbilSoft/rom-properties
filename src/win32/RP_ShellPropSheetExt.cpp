/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * RP_ShellPropSheetExt.cpp: IShellPropSheetExt implementation.            *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

// References:
// - http://www.codeproject.com/Articles/338268/COM-in-C
// - https://code.msdn.microsoft.com/windowsapps/CppShellExtPropSheetHandler-d93b49b7
// - https://docs.microsoft.com/en-us/windows/win32/ad/implementing-the-property-page-com-object

#include "stdafx.h"

#include "RP_ShellPropSheetExt.hpp"
#include "RP_ShellPropSheetExt_p.hpp"
#include "RomDataFormat.hpp"
#include "RpImageWin32.hpp"
#include "res/resource.h"

// Custom controls (some are pseudo-controls)
#include "DragImageLabel.hpp"
#include "LanguageComboBox.hpp"
#include "MessageWidget.hpp"
#include "OptionsMenuButton.hpp"

// libwin32ui
#include "libwin32ui/AutoGetDC.hpp"
using LibWin32UI::AutoGetDC_font;
using LibWin32UI::WTSSessionNotification;

// NOTE: Using "RomDataView" for the libi18n context, since that
// matches what's used for the KDE and GTK+ frontends.

// Other rom-properties libraries
#include "librpbase/RomFields.hpp"
#include "librpbase/TextOut.hpp"
using namespace LibRpBase;
using namespace LibRpFile;
using namespace LibRpText;
using namespace LibRpTexture;
using namespace LibRomData;

// C++ STL classes
using std::array;
using std::set;
using std::string;
using std::unique_ptr;
using std::vector;
using std::wstring;	// for tstring

// Win32 dark mode
#include "libwin32darkmode/DarkMode.hpp"

// CLSID
const CLSID CLSID_RP_ShellPropSheetExt =
	{0x2443C158, 0xDF7C, 0x4352, {0xB4, 0x35, 0xBC, 0x9F, 0x88, 0x5F, 0xFD, 0x52}};

/** RP_ShellPropSheetExt_Private **/

// Property for "tab pointer".
// This points to the RP_ShellPropSheetExt_Private::tab object.
const TCHAR RP_ShellPropSheetExt_Private::TAB_PTR_PROP[] = _T("RP_ShellPropSheetExt_Private::tab");

/**
 * RP_ShellPropSheetExt_Private constructor
 * @param q
 * @param tfilename
 */
RP_ShellPropSheetExt_Private::RP_ShellPropSheetExt_Private(RP_ShellPropSheetExt *q, const TCHAR *tfilename)
	: q_ptr(q)
	, hDlgSheet(nullptr)
	, tfilename(tfilename)
	, fontHandler(nullptr)
	, lblSysInfo(nullptr)
	, tabWidget(nullptr)
	, lblDescHeight(0)
	, hBtnOptions(nullptr)
	, hMessageWidget(nullptr)
	, iTabHeightOrig(0)
	, def_lc(0)
	, cboLanguage(nullptr)
	, dwExStyleRTL(LibWin32UI::isSystemRTL())
	, isDarkModeEnabled(false)
	, isFullyInit(false)
{
	// Initialize structs.
	dlgSize.cx = 0;
	dlgSize.cy = 0;
}

/**
 * Load the banner and icon as HBITMAPs.
 *
 * This function should be called on startup and if
 * the window's background color changes.
 *
 * NOTE: The HWND isn't needed here, since this function
 * doesn't touch the dialog at all.
 */
void RP_ShellPropSheetExt_Private::loadImages(void)
{
	// Supported image types
	const uint32_t imgbf = romData->supportedImageTypes();
	// FIXME: Store the standard image height somewhere else.
	// FIXME: Adjust image sizes if the DPI changes.
	const int imgStdHeight = rp_AdjustSizeForDpi(32, rp_GetDpiForWindow(hDlgSheet));

	// Banner
	bool ok = false;
	if (imgbf & RomData::IMGBF_INT_BANNER) {
		// Get the banner.
		const rp_image_const_ptr banner = romData->image(RomData::IMG_INT_BANNER);
		if (banner && banner->isValid()) {
			if (!lblBanner) {
				lblBanner.reset(new DragImageLabel(hDlgSheet));
				// TODO: Required size? For now, disabling scaling.
				lblBanner->setRequiredSize(0, 0);
			}

			ok = lblBanner->setRpImage(banner);
			if (ok) {
				// Adjust the banner size.
				const SIZE bannerSize = {banner->width(), banner->height()};
				if (bannerSize.cy != imgStdHeight) {
					// Need to scale the banner label to match the aspect ratio.
					const SIZE bannerScaledSize = {
						static_cast<LONG>(rintf(
							static_cast<float>(imgStdHeight) * (static_cast<float>(bannerSize.cx) /
							static_cast<float>(bannerSize.cy)))),
						imgStdHeight
					};
					lblBanner->setRequiredSize(bannerScaledSize);
				} else {
					// Use the original banner size.
					lblBanner->setRequiredSize(bannerSize);
				}
			}
		}
	}
	if (!ok) {
		// No banner, or unable to load the banner.
		// Delete the DragImageLabel if it was created previously.
		lblBanner.reset();
	}

	// Icon
	ok = false;
	if (imgbf & RomData::IMGBF_INT_ICON) {
		// Get the icon.
		const rp_image_const_ptr icon = romData->image(RomData::IMG_INT_ICON);
		if (icon && icon->isValid()) {
			if (!lblIcon) {
				lblIcon.reset(new DragImageLabel(hDlgSheet));
			}

			// Is this an animated icon?
			ok = lblIcon->setIconAnimData(romData->iconAnimData());
			if (!ok) {
				// Not an animated icon, or invalid icon data.
				// Set the static icon.
				ok = lblIcon->setRpImage(icon);
			}

			if (ok) {
				// Adjust the icon size.
				const SIZE iconSize = {icon->width(), icon->height()};
				if (iconSize.cy != imgStdHeight) {
					// Need to scale the icon label to match the aspect ratio.
					const SIZE iconScaledSize = {
						static_cast<LONG>(rintf(
							static_cast<float>(imgStdHeight) * (static_cast<float>(iconSize.cx) /
							static_cast<float>(iconSize.cy)))),
						imgStdHeight
					};
					lblIcon->setRequiredSize(iconScaledSize);
				} else {
					// Use the original icon size.
					lblIcon->setRequiredSize(iconSize);
				}
			}
		}
	}
	if (!ok) {
		// No icon, or unable to load the icon.
		// Delete the DragImageLabel if it was created previously.
		lblIcon.reset();
	}
}

/**
 * Create the header row.
 * @param hDlg		[in] Dialog window
 * @param pt_start	[in] Starting position, in pixels
 * @param size		[in] Width and height for a full-width single line label
 * @return Row height, in pixels.
 */
int RP_ShellPropSheetExt_Private::createHeaderRow(_In_ POINT pt_start, _In_ SIZE size)
{
	if (!hDlgSheet || !romData)
		return 0;

	// Total widget width.
	int total_widget_width = 0;

	// Label size.
	SIZE size_lblSysInfo = {0, 0};

	// Font to use.
	// TODO: Handle these assertions in release builds.
	HFONT hFont = fontHandler.boldFont();
	if (!hFont) {
		hFont = GetWindowFont(hDlgSheet);
	}

	// System name and file type.
	// TODO: System logo and/or game title?
	const char *systemName = romData->systemName(
		RomData::SYSNAME_TYPE_LONG | RomData::SYSNAME_REGION_ROM_LOCAL);
	const char *fileType = romData->fileType_string();
	assert(systemName != nullptr);
	assert(fileType != nullptr);
	if (!systemName) {
		systemName = C_("RomDataView", "(unknown system)");
	}
	if (!fileType) {
		fileType = C_("RomDataView", "(unknown filetype)");
	}

	const tstring ts_sysInfo =
		LibWin32UI::unix2dos(U82T_s(fmt::format(
			// tr: {0:s} == system name, {1:s} == file type
			FRUN(C_("RomDataView", "{0:s}\n{1:s}")), systemName, fileType)));

	if (!ts_sysInfo.empty()) {
		// Determine the appropriate label size.
		if (!LibWin32UI::measureTextSize(hDlgSheet, hFont, ts_sysInfo, &size_lblSysInfo)) {
			// Start the total_widget_width.
			total_widget_width = size_lblSysInfo.cx;
		} else {
			// Error determining the label size.
			// Don't draw the label.
			size_lblSysInfo.cx = 0;
			size_lblSysInfo.cy = 0;
		}
	}

	// Add the banner and icon widths.

	// Banner
	// TODO: Spacing between banner and text?
	// Doesn't seem to be needed with Dreamcast saves...
	const int banner_width = (lblBanner ? lblBanner->actualSize().cx : 0);
	total_widget_width += banner_width;

	// Icon
	const int icon_width = (lblIcon ? lblIcon->actualSize().cx : 0);
	if (icon_width > 0) {
		if (total_widget_width > 0) {
			total_widget_width += pt_start.x;
		}
		total_widget_width += icon_width;
	}

	// Starting point
	POINT curPt = {
		((size.cx - total_widget_width) / 2) + pt_start.x,
		pt_start.y
	};

	// lblSysInfo
	if (size_lblSysInfo.cx > 0 && size_lblSysInfo.cy > 0) {
		ptSysInfo.x = curPt.x;
		ptSysInfo.y = curPt.y;

		lblSysInfo = CreateWindowEx(WS_EX_NOPARENTNOTIFY | WS_EX_TRANSPARENT,
			WC_STATIC, ts_sysInfo.c_str(),
			WS_CHILD | WS_VISIBLE | SS_CENTER,
			ptSysInfo.x, ptSysInfo.y,
			size_lblSysInfo.cx, size_lblSysInfo.cy,
			hDlgSheet, (HMENU)IDC_STATIC, nullptr, nullptr);
		SetWindowFont(lblSysInfo, hFont, false);
		curPt.x += size_lblSysInfo.cx + pt_start.x;
	}

	// Banner
	if (banner_width > 0) {
		lblBanner->setPosition(curPt);
		curPt.x += banner_width + pt_start.x;
	}

	// Icon
	if (icon_width > 0) {
		lblIcon->setPosition(curPt);
		curPt.x += icon_width + pt_start.x;
	}

	if (lblIcon) {
		const bool ecksBawks = (romData->fileType() == RomData::FileType::DiscImage &&
		                        systemName && strstr(systemName, "Xbox") != nullptr);
		lblIcon->setEcksBawks(ecksBawks);
	}

	// Return the label height and some extra padding.
	// TODO: Icon/banner height?
	return size_lblSysInfo.cy + (pt_start.y * 5 / 8);
}

/**
 * Initialize a string field. (Also used for Date/Time.)
 * @param hWndTab	[in] Tab window (for the actual control)
 * @param pt_start	[in] Starting position, in pixels
 * @param size		[in] Width and height for a single line label
 * @param field		[in] RomFields::Field
 * @param fieldIdx	[in] Field index
 * @param str		[in,opt] String data (If nullptr, field data is used.)
 * @param pOutHWND	[out,opt] Retrieves the control's HWND
 * @return Field height, in pixels.
 */
int RP_ShellPropSheetExt_Private::initString(_In_ HWND hWndTab,
	_In_ POINT pt_start, _In_ SIZE size,
	_In_ const RomFields::Field &field, _In_ int fieldIdx,
	_In_ LPCTSTR str, _Outptr_opt_ HWND *pOutHWND)
{
	if (pOutHWND) {
		// Clear the output HWND initially.
		*pOutHWND = nullptr;
	}

	// NOTE: libromdata uses Unix-style newlines.
	// For proper display on Windows, we have to
	// add carriage returns.

	// If string data wasn't specified, get the RFT_STRING data
	// from the RomFields::Field object.
	int lf_count = 0;
	tstring str_nl;
	if (!str) {
		switch (field.type) {
			default:
				// Not supported.
				assert(!"Unsupported field type!");
				return 0;

			case RomFields::RFT_STRING:
				// NULL string == empty string
				if (field.data.str) {
					str_nl = LibWin32UI::unix2dos(U82T_s(field.data.str), &lf_count);
				}
				break;

			case RomFields::RFT_STRING_MULTI:
				// Need to count newlines for *all* strings in this field.
				const auto *const pStr_multi = field.data.str_multi;
				for (const auto &p : *pStr_multi) {
					// Count the number of newlines.
					int tmp_lf_count = std::accumulate(p.second.cbegin(), p.second.cend(), 0,
						[](int sum, char chr) -> int {
							if (chr == '\n')
								sum++;
							return sum;
						});
					if (tmp_lf_count > lf_count) {
						lf_count = tmp_lf_count;
					}
				}

				// NOTE: Not setting str_nl here, since the user will be
				// able to change the displayed language.
				break;
		}
	} else {
		// Use the specified string.
		str_nl = LibWin32UI::unix2dos(str, &lf_count);
	}

	// Field height.
	int field_cy = size.cy;
	if (lf_count > 0) {
		// Multiple lines.
		// NOTE: Only add 5/8 of field_cy per line.
		// FIXME: 5/8 needs adjustment...
		field_cy += (field_cy * lf_count) * 5 / 8;
	}

	// Get the default font.
	HFONT hFont = GetWindowFont(hDlgSheet);

	// Check for any formatting options.
	bool isWarning = false, isMonospace = false;
	if (field.type == RomFields::RFT_STRING) {
		// FIXME: STRF_MONOSPACE | STRF_WARNING is not supported.
		// Preferring STRF_WARNING.
		assert((field.flags &
			(RomFields::STRF_MONOSPACE | RomFields::STRF_WARNING)) !=
			(RomFields::STRF_MONOSPACE | RomFields::STRF_WARNING));

		if (field.flags & RomFields::STRF_WARNING) {
			// "Warning" font.
			isWarning = true;
			HFONT hFontBold = fontHandler.boldFont();
			if (hFontBold) {
				hFont = hFontBold;
			}
			// Set the font of the description control.
			HWND hStatic = GetDlgItem(hWndTab, IDC_STATIC_DESC(fieldIdx));
			if (hStatic) {
				SetWindowFont(hStatic, hFont, false);
				SetWindowLongPtr(hStatic, GWLP_USERDATA, static_cast<LONG_PTR>(RGB(255, 0, 0)));
			}
		} else if (field.flags & RomFields::STRF_MONOSPACE) {
			// Monospaced font.
			isMonospace = true;
		}
	}

	// Dialog item.
	const uint16_t cId = IDC_RFT_STRING(fieldIdx);
	HWND hDlgItem;

	if (field.type == RomFields::RFT_STRING &&
	    (field.flags & RomFields::STRF_CREDITS))
	{
		// Align to the bottom of the dialog and center-align the text.
		// 7x7 DLU margin is recommended by the Windows UX guidelines.
		// Reference: http://stackoverflow.com/questions/2118603/default-dialog-padding
		RECT tmpRect = {7, 7, 8, 8};
		MapDialogRect(hWndTab, &tmpRect);
		RECT winRect;
		GetClientRect(hWndTab, &winRect);
		// NOTE: We need to move left by 1px.
		OffsetRect(&winRect, -1, 0);

		// There should be a maximum of one STRF_CREDITS per tab.
		auto &tab = tabs[field.tabIdx];
		assert(tab.lblCredits == nullptr);
		if (tab.lblCredits != nullptr) {
			// Duplicate credits label.
			return 0;
		}

		// Create a SysLink widget.
		// SysLink allows the user to click a link and
		// open a webpage. It does NOT allow highlighting.
		// TODO: SysLink + EDIT?
		// FIXME: Centered text alignment?
		// TODO: With subtabs:
		// - Verify behavior of LWS_TRANSPARENT.
		// - Show below subtabs.
		hDlgItem = CreateWindowEx(WS_EX_NOPARENTNOTIFY | WS_EX_TRANSPARENT | dwExStyleRTL,
			WC_LINK, str_nl.c_str(),
			WS_CHILD | WS_TABSTOP | WS_VISIBLE,
			0, 0, 0, 0,	// will be adjusted afterwards
			hWndTab, (HMENU)(INT_PTR)cId, nullptr, nullptr);
		if (!hDlgItem) {
			// Unable to create a SysLink control
			// This might happen if this is an ANSI build
			// or if we're running on Windows 2000.

			// FIXME: Remove links from the text before creating
			// a plain-old WC_EDIT control.

			// Create a read-only EDIT control.
			// The STATIC control doesn't allow the user
			// to highlight and copy data.
			DWORD dwStyle = WS_CHILD | WS_TABSTOP | WS_VISIBLE | WS_CLIPSIBLINGS |
			                ES_READONLY | ES_AUTOHSCROLL;
			if (lf_count > 0) {
				// Multiple lines.
				dwStyle |= ES_MULTILINE;
			}

			hDlgItem = CreateWindowEx(WS_EX_NOPARENTNOTIFY | WS_EX_TRANSPARENT | dwExStyleRTL,
				WC_EDIT, str_nl.c_str(), dwStyle,
				0, 0, 0, 0,	// will be adjusted afterwards
				hWndTab, (HMENU)(INT_PTR)cId, nullptr, nullptr);

			// Subclass multi-line EDIT controls to work around Enter/Escape issues.
			// We're also subclassing single-line EDIT controls to disable the
			// initial selection. (DLGC_HASSETSEL)
			// Reference: http://blogs.msdn.com/b/oldnewthing/archive/2007/08/20/4470527.aspx
			// TODO: Error handling?
			SUBCLASSPROC proc = (dwStyle & ES_MULTILINE)
				? LibWin32UI::MultiLineEditProc
				: LibWin32UI::SingleLineEditProc;
			SetWindowSubclass(hDlgItem, proc,
				static_cast<UINT_PTR>(cId),
				reinterpret_cast<DWORD_PTR>(GetParent(hDlgSheet)));
		}

		tab.lblCredits = hDlgItem;
		SetWindowFont(hDlgItem, hFont, false);

		// NOTE: We can't use LibWin32UI::measureTextSize() because
		// that includes the HTML markup, and LM_GETIDEALSIZE is Vista+ only.
		// Use a wrapper measureTextSizeLink() that removes HTML-like
		// tags and then calls measureTextSize().
		SIZE szText;
		LibWin32UI::measureTextSizeLink(hWndTab, hFont, str_nl, &szText);

		// Determine the position.
		const POINT pos = {
			(((winRect.right - winRect.left) - szText.cx) / 2) + winRect.left,
			winRect.bottom - tmpRect.top - szText.cy
		};
		// Set the position and size.
		SetWindowPos(hDlgItem, nullptr, pos.x, pos.y, szText.cx, szText.cy,
			SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOZORDER);

		// Clear field_cy so the description widget won't show up
		// and the "normal" area will be empty.
		field_cy = 0;
	} else {
		// Create a read-only EDIT control.
		// The STATIC control doesn't allow the user
		// to highlight and copy data.
		DWORD dwStyle;
		if (lf_count > 0) {
			// Multiple lines.
			dwStyle = WS_CHILD | WS_TABSTOP | WS_VISIBLE | WS_CLIPSIBLINGS | ES_READONLY | ES_AUTOHSCROLL | ES_MULTILINE;
		} else {
			// Single line.
			dwStyle = WS_CHILD | WS_TABSTOP | WS_VISIBLE | WS_CLIPSIBLINGS | ES_READONLY | ES_AUTOHSCROLL;
		}
		hDlgItem = CreateWindowEx(WS_EX_NOPARENTNOTIFY | WS_EX_TRANSPARENT | dwExStyleRTL,
			WC_EDIT, str_nl.c_str(), dwStyle,
			pt_start.x, pt_start.y,
			size.cx, field_cy,
			hWndTab, (HMENU)(INT_PTR)cId, nullptr, nullptr);
		SetWindowFont(hDlgItem, hFont, false);

		// Get the EDIT control margins.
		const DWORD dwMargins = (DWORD)SendMessage(hDlgItem, EM_GETMARGINS, 0, 0);

		// Adjust the window size to compensate for the margins not being clickable.
		// NOTE: Not adjusting the right margins.
		SetWindowPos(hDlgItem, nullptr, pt_start.x - LOWORD(dwMargins), pt_start.y,
			size.cx + LOWORD(dwMargins), field_cy,
			SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOZORDER);

		// Subclass multi-line EDIT controls to work around Enter/Escape issues.
		// We're also subclassing single-line EDIT controls to disable the
		// initial selection. (DLGC_HASSETSEL)
		// Reference:  http://blogs.msdn.com/b/oldnewthing/archive/2007/08/20/4470527.aspx
		// TODO: Error handling?
		SUBCLASSPROC proc = (dwStyle & ES_MULTILINE)
			? LibWin32UI::MultiLineEditProc
			: LibWin32UI::SingleLineEditProc;
		SetWindowSubclass(hDlgItem, proc,
			static_cast<UINT_PTR>(cId),
			reinterpret_cast<DWORD_PTR>(GetParent(hDlgSheet)));
	}

	// Save the control in the appropriate container, if necessary.
	if (isWarning) {
		SetWindowLongPtr(hDlgItem, GWLP_USERDATA, static_cast<LONG_PTR>(RGB(255, 0, 0)));
	}
	if (isMonospace) {
		fontHandler.addMonoControl(hDlgItem);
	}

	// Return the HWND if requested.
	if (pOutHWND) {
		*pOutHWND = hDlgItem;
	}

	return field_cy;
}

/**
 * Initialize a bitfield layout.
 * @param hWndTab	[in] Tab window (for the actual control)
 * @param pt_start	[in] Starting position, in pixels
 * @param field		[in] RomFields::Field
 * @param fieldIdx	[in] Field index
 * @return Field height, in pixels.
 */
int RP_ShellPropSheetExt_Private::initBitfield(_In_ HWND hWndTab,
	_In_ POINT pt_start, _In_ const RomFields::Field &field, _In_ int fieldIdx)
{
	// Checkbox size.
	// Reference: http://stackoverflow.com/questions/1164868/how-to-get-size-of-check-and-gap-in-check-box
	RECT rect_chkbox = {0, 0, 12+4, 11};
	MapDialogRect(hDlgSheet, &rect_chkbox);

	// Dialog font and device context.
	// NOTE: Using the parent dialog's font.
	HFONT hFontDlg = GetWindowFont(hDlgSheet);
	AutoGetDC_font hDC(hWndTab, hFontDlg);

	// Create a grid of checkboxes.
	const auto &bitfieldDesc = field.desc.bitfield;
	int count = (int)bitfieldDesc.names->size();
	assert(count <= 32);
	if (count > 32) {
		count = 32;
	}

	// Determine the available width for checkboxes.
	RECT rectDlg;
	GetClientRect(hWndTab, &rectDlg);
	// NOTE: We need to move left by 1px.
	OffsetRect(&rectDlg, -1, 0);
	const int max_width = rectDlg.right - pt_start.x;

	// Convert the bitfield description names to the
	// native Windows encoding once.
	vector<tstring> tnames;
	tnames.reserve(count);
	for (const string &name : *(bitfieldDesc.names)) {
		if (!name.empty()) {
			tnames.push_back(U82T_s(name));
		} else {
			// Skip U82T_s() for empty strings.
			tnames.emplace_back();
		}
	}

	// Column widths for multi-row layouts.
	// (Includes the checkbox size.)
	std::vector<int> col_widths;
	int row = 0, col = 0;
	int elemsPerRow = bitfieldDesc.elemsPerRow;
	if (elemsPerRow == 1) {
		// Optimization: Use the entire width of the dialog.
		// TODO: Testing; right margin.
		col_widths.push_back(max_width);
	} else if (elemsPerRow > 1) {
		// Determine the widest entry in each column.
		// If the columns are wider than the available area,
		// reduce the number of columns until it fits.
		for (; elemsPerRow > 1; elemsPerRow--) {
			col_widths.resize(elemsPerRow);
			row = 0; col = 0;
			auto iter = tnames.cbegin();
			for (int j = 0; j < count; ++iter, j++) {
				const tstring &tname = *iter;
				if (tname.empty())
					continue;

				// Get the width of this specific entry.
				// TODO: Use LibWin32UI::measureTextSize()?
				SIZE textSize;
				GetTextExtentPoint32(hDC, tname.data(), (int)tname.size(), &textSize);
				const int chk_w = rect_chkbox.right + textSize.cx;
				if (chk_w > col_widths[col]) {
					col_widths[col] = chk_w;
				}

				// Next column.
				col++;
				if (col == elemsPerRow) {
					// Next row.
					row++;
					col = 0;
				}
			}

			// Add up the widths.
			const int total_width = std::accumulate(col_widths.cbegin(), col_widths.cend(), 0);

			// TODO: "DLL" on Windows executables is forced to the next line.
			// Add 7x7 DLU margins?
			if (total_width <= max_width) {
				// Everything fits.
				break;
			}

			// Too wide; try removing a column.
			// Reset the column widths first.
			// TODO: Better way to clear a vector?
			// TODO: Skip the last element?
			memset(col_widths.data(), 0, col_widths.size() * sizeof(int));
		}

		if (elemsPerRow == 1) {
			// We're left with 1 column.
			col_widths.resize(1);
			col_widths[0] = max_width;
		}
	}

	// Initial position on the dialog, in pixels.
	POINT pt = pt_start;
	// Subtract 0.5 DLU from the starting row.
	RECT rect_subtract = {0, 0, 1, 1};
	MapDialogRect(hDlgSheet, &rect_subtract);
	if (rect_subtract.bottom > 1) {
		rect_subtract.bottom /= 2;
	}
	pt.y -= rect_subtract.bottom;

	row = 0; col = 0;
	auto iter = tnames.cbegin();
	uint32_t bitfield = field.data.bitfield;
	for (int bit = 0; bit < count; ++iter, bit++, bitfield >>= 1) {
		const tstring &tname = *iter;
		if (tname.empty())
			continue;

		// Get the text size.
		int chk_w;
		if (elemsPerRow == 0) {
			// Get the width of this specific entry.
			// TODO: Use LibWin32UI::measureTextSize()?
			SIZE textSize;
			GetTextExtentPoint32(hDC, tname.data(), (int)tname.size(), &textSize);
			chk_w = rect_chkbox.right + textSize.cx;
		} else {
			if (col == elemsPerRow) {
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
		const uint16_t cId = IDC_RFT_BITFIELD(fieldIdx, bit);
		HWND hCheckBox = CreateWindowEx(WS_EX_NOPARENTNOTIFY | WS_EX_TRANSPARENT | dwExStyleRTL,
			WC_BUTTON, tname.c_str(),
			WS_CHILD | WS_TABSTOP | WS_VISIBLE | BS_CHECKBOX,
			pt.x, pt.y, chk_w, rect_chkbox.bottom,
			hWndTab, (HMENU)(INT_PTR)cId,
			nullptr, nullptr);
		SetWindowFont(hCheckBox, hFontDlg, false);

		// Set the checkbox state.
		Button_SetCheck(hCheckBox, (bitfield & 1) ? BST_CHECKED : BST_UNCHECKED);

		// Next column.
		pt.x += chk_w;
		col++;
	}

	// Return the total number of rows times the checkbox height,
	// plus another 0.25 of a checkbox.
	int ret = ((row + 1) * rect_chkbox.bottom);
	ret += (rect_chkbox.bottom / 4);
	return ret;
}

/**
 * Initialize a ListData field.
 * @param hWndTab	[in] Tab window (for the actual control)
 * @param pt_start	[in] Starting position, in pixels
 * @param size		[in] Width and height for a default ListView
 * @param doResize	[in] If true, resize the ListView to accomodate rows_visible.
 * @param field		[in] RomFields::Field
 * @param fieldIdx	[in] Field index
 * @return Field height, in pixels.
 */
int RP_ShellPropSheetExt_Private::initListData(_In_ HWND hWndTab,
	_In_ POINT pt_start, _In_ SIZE size, _In_ bool doResize,
	_In_ const RomFields::Field &field, _In_ int fieldIdx)
{
	const auto &listDataDesc = field.desc.list_data;
	// NOTE: listDataDesc.names can be nullptr,
	// which means we don't have any column headers.

	// Single language ListData_t.
	// For RFT_LISTDATA_MULTI, this is only used for row and column count.
	const RomFields::ListData_t *list_data;
	const bool isMulti = !!(field.flags & RomFields::RFT_LISTDATA_MULTI);
	if (isMulti) {
		// Multiple languages
		const auto *const multi = field.data.list_data.data.multi;
		assert(multi != nullptr);
		assert(!multi->empty());
		if (!multi || multi->empty()) {
			// No data...
			return 0;
		}

		list_data = &multi->cbegin()->second;
	} else {
		// Single language
		list_data = field.data.list_data.data.single;
	}

	assert(list_data != nullptr);
	if (!list_data) {
		// No list data...
		return 0;
	}

	// Validate flags.
	// Cannot have both checkboxes and icons.
	const bool hasCheckboxes = !!(field.flags & RomFields::RFT_LISTDATA_CHECKBOXES);
	const bool hasIcons = !!(field.flags & RomFields::RFT_LISTDATA_ICONS);
	assert(!(hasCheckboxes && hasIcons));
	if (hasCheckboxes && hasIcons) {
		// Both are set. This shouldn't happen...
		return 0;
	}

	if (hasIcons) {
		assert(field.data.list_data.mxd.icons != nullptr);
		if (!field.data.list_data.mxd.icons) {
			// No icons vector...
			return 0;
		}
	}

	// Create a ListView widget.
	// NOTE: Separate row option is handled by the caller.
	DWORD lvsStyle = WS_CHILD | WS_VISIBLE | WS_TABSTOP | LVS_ALIGNLEFT |
	                 LVS_REPORT | LVS_SINGLESEL | LVS_OWNERDATA;
	if (!listDataDesc.names) {
		lvsStyle |= LVS_NOCOLUMNHEADER;
	}
	const uint16_t cId = IDC_RFT_LISTDATA(fieldIdx);
	HFONT hFontDlg = GetWindowFont(hDlgSheet);
	HWND hListView = CreateWindowEx(WS_EX_NOPARENTNOTIFY | WS_EX_CLIENTEDGE | dwExStyleRTL,
		WC_LISTVIEW, nullptr, lvsStyle,
		pt_start.x, pt_start.y, size.cx, size.cy,
		hWndTab, (HMENU)(INT_PTR)cId, nullptr, nullptr);
	SetWindowFont(hListView, hFontDlg, false);
	hwndListViewControls.push_back(hListView);

	// Set extended ListView styles.
	DWORD lvsExStyle = LVS_EX_FULLROWSELECT;
	if (!GetSystemMetrics(SM_REMOTESESSION)) {
		// Not RDP (or is RemoteFX): Enable double buffering.
		lvsExStyle |= LVS_EX_DOUBLEBUFFER;
	}
	if (hasCheckboxes) {
		lvsExStyle |= LVS_EX_CHECKBOXES;
	}
	ListView_SetExtendedListViewStyle(hListView, lvsExStyle);

	// Insert columns.
	int colCount = 1;
	if (listDataDesc.names) {
		colCount = (int)listDataDesc.names->size();
	} else {
		// No column headers.
		// Use the first row.
		colCount = (int)list_data->at(0).size();
	}
	assert(colCount > 0);
	if (colCount <= 0) {
		// No columns...
		return 0;
	}

	// NOTE: We're converting the strings for use with
	// LVS_OWNERDATA.
	vector<vector<tstring> > lvStringData;
	lvStringData.reserve(list_data->size());
	LvData lvData(hListView);
	lvData.vvStr.reserve(list_data->size());
	lvData.hasCheckboxes = hasCheckboxes;
	lvData.col_widths.resize(colCount);
	if (hasCheckboxes) {
		// TODO: Better calculation?
		lvData.col0sizeadj = GetSystemMetrics(SM_CXMENUCHECK) + GetSystemMetrics(SM_CXEDGE);
	} else if (hasIcons) {
		lvData.vImageList.reserve(list_data->size());
	}

	// Dialog font and device context.
	// NOTE: Using the parent dialog's font.
	AutoGetDC_font hDC(hListView, hFontDlg);

	// Format table.
	// All values are known to fit in uint8_t.
	static constexpr array<uint8_t, 4> align_tbl = {{
		// Order: TXA_D, TXA_L, TXA_C, TXA_R
		LVCFMT_LEFT, LVCFMT_LEFT, LVCFMT_CENTER, LVCFMT_RIGHT
	}};

	// NOTE: ListView header alignment matches data alignment.
	// We'll prefer the data alignment value.
	unsigned int align = listDataDesc.col_attrs.align_data;

	LVCOLUMN lvColumn;
	if (listDataDesc.names) {
		auto iter = listDataDesc.names->cbegin();
		for (int col = 0; col < colCount; ++iter, col++, align >>= RomFields::TXA_BITS) {
			lvColumn.fmt = align_tbl[align & RomFields::TXA_MASK];

			const string &str = *iter;
			if (likely(!str.empty())) {
				// NOTE: pszText is LPTSTR, not LPCTSTR...
				// NOTE 2: ListView is limited to 260 characters. (259+1)
				// TODO: Handle surrogate pairs?
				tstring tstr = U82T_s(str);
				if (tstr.size() >= 260) {
					// Reduce to 256 and add "..."
					tstr.resize(256);
					tstr += _T("...");
				}
				lvData.col_widths[col] = LibWin32UI::measureStringForListView(hDC, tstr);
				lvColumn.mask = LVCF_TEXT | LVCF_FMT;
				lvColumn.pszText = const_cast<LPTSTR>(tstr.c_str());
				ListView_InsertColumn(hListView, col, &lvColumn);
			} else {
				// Don't show this column.
				// FIXME: Zero-width column is a bad hack...
				lvColumn.pszText = const_cast<LPTSTR>(_T(""));
				lvColumn.mask = LVCF_TEXT | LVCF_FMT | LVCF_WIDTH;
				lvColumn.cx = 0;
				ListView_InsertColumn(hListView, col, &lvColumn);
			}
		}
	} else {
		lvColumn.mask = LVCF_FMT;
		for (int i = 0; i < colCount; i++, align >>= RomFields::TXA_BITS) {
			lvColumn.fmt = align_tbl[align & RomFields::TXA_MASK];
			ListView_InsertColumn(hListView, i, &lvColumn);
		}
	}

	// Add the row data.
	uint32_t checkboxes = 0;
	if (hasCheckboxes) {
		checkboxes = field.data.list_data.mxd.checkboxes;
	}

	int lv_row_num = 0;
	int nl_max = 0;	// Highest number of newlines in any string.
	for (const auto &data_row : *list_data) {
		// FIXME: Skip even if we don't have checkboxes?
		// (also check other UI frontends)
		if (hasCheckboxes) {
			if (data_row.empty()) {
				// Skip this row.
				checkboxes >>= 1;
				continue;
			} else {
				// Store the checkbox value for this row.
				if (checkboxes & 1) {
					lvData.checkboxes |= (1U << lv_row_num);
				}
				checkboxes >>= 1;
			}
		}

		// Destination row
		lvData.vvStr.resize(lvData.vvStr.size()+1);
		auto &lv_row_data = lvData.vvStr.at(lvData.vvStr.size()-1);
		lv_row_data.reserve(data_row.size());

		// String data
		if (isMulti) {
			// RFT_LISTDATA_MULTI. Allocate space for the strings,
			// but don't initialize them here.
			lv_row_data.resize(data_row.size());

			// Check newline counts in all strings to find nl_max.
			for (const auto &ldm : *(field.data.list_data.data.multi)) {
				const RomFields::ListData_t &ld = ldm.second;
				for (const auto &m_data_row : ld) {
					unsigned int is_timestamp = listDataDesc.col_attrs.is_timestamp;
					for (const auto &m_data_col : m_data_row) {
						if (unlikely((is_timestamp & 1) && m_data_col.size() == sizeof(int64_t))) {
							// Timestamp column. NL count is 0.
							is_timestamp >>= 1;
							continue;
						}

						size_t prev_nl_pos = 0;
						size_t cur_nl_pos;
						int nl = 0;
						while ((cur_nl_pos = m_data_col.find('\n', prev_nl_pos)) != tstring::npos) {
							nl++;
							prev_nl_pos = cur_nl_pos + 1;
						}
						nl_max = std::max(nl_max, nl);
						is_timestamp >>= 1;
					}
				}
			}
		} else {
			// Single language
			int col = 0;
			unsigned int is_timestamp = listDataDesc.col_attrs.is_timestamp;
			for (const auto &data_str : data_row) {
				// NOTE: ListView is limited to 260 characters. (259+1)
				// TODO: Handle surrogate pairs?
				tstring tstr;
				if (unlikely((is_timestamp & 1) && data_str.size() == sizeof(int64_t))) {
					// Timestamp column. Format the timestamp.
					RomFields::TimeString_t time_string;
					memcpy(time_string.str, data_str.data(), 8);

					tstr = formatDateTime(time_string.time,
						listDataDesc.col_attrs.dtflags);
					if (unlikely(tstr.empty())) {
						tstr = TC_("RomData", "Unknown");
					}
				} else {
					tstr = U82T_s(data_str);
					if (tstr.size() >= 260) {
						// Reduce to 256 and add "..."
						tstr.resize(256);
						tstr += _T("...");
					}
				}

				int nl_count;
				const int width = LibWin32UI::measureStringForListView(hDC, tstr, &nl_count);
				if (col < colCount) {
					lvData.col_widths[col] = std::max(lvData.col_widths[col], width);
				}
				nl_max = std::max(nl_max, nl_count);

				// TODO: Store the icon index if necessary.
				lv_row_data.push_back(std::move(tstr));

				// Next column.
				is_timestamp >>= 1;
				col++;
			}
		}

		// Next row.
		lv_row_num++;
	}

	// Icons
	if (hasIcons) {
		// Icon size is 32x32, adjusted for DPI. (TODO: WM_DPICHANGED)
		// ImageList will resize the original icons to 32x32.

		// NOTE: LVS_REPORT doesn't allow variable row sizes,
		// at least not without using ownerdraw. Hence, we'll
		// use a hackish workaround: If any entry has more than
		// two newlines, increase the Imagelist icon size by
		// 16 pixels.
		// TODO: Handle this better.
		// FIXME: This only works if the RFT_LISTDATA has icons.
		const int px = rp_AdjustSizeForDpi(32, rp_GetDpiForWindow(hDlgSheet));
		lvData.col0sizeadj = px;

		const SIZE sizeListIconOrig = {px, px};
		SIZE sizeListIconPhys = {px, px};
		float factor = 1.0f;
		bool resizeNeeded = false;
		if (nl_max >= 2) {
			// Two or more newlines.
			// Add half of the icon size per newline over 1.
			sizeListIconPhys.cy += ((px/2) * (nl_max - 1));
			factor = (float)sizeListIconPhys.cy / (float)px;
			resizeNeeded = true;
		}

		HIMAGELIST himl = ImageList_Create(sizeListIconPhys.cx, sizeListIconPhys.cy,
			ILC_COLOR32, static_cast<int>(list_data->size()), 0);
		assert(himl != nullptr);
		if (himl) {
			// NOTE: ListView uses LVSIL_SMALL for LVS_REPORT.
			ListView_SetImageList(hListView, himl, LVSIL_SMALL);
			const uint32_t lvBgColor[2] = {
				ListView_GetBkColor(hListView) | 0xFF000000,
				LibWin32UI::ListView_GetBkColor_AltRow(hListView) | 0xFF000000
			};

			// Add icons.
			uint8_t rowColorIdx = 0;
			const auto &icons = field.data.list_data.mxd.icons;
			const auto icons_cend = icons->cend();
			for (auto iter = icons->cbegin(); iter != icons_cend;
			     ++iter, rowColorIdx = !rowColorIdx)
			{
				rp_image_const_ptr icon = *iter;
				if (!icon) {
					// No icon for this row.
					lvData.vImageList.push_back(-1);
					continue;
				}

				if (dwExStyleRTL != 0) {
					// WS_EX_LAYOUTRTL will flip bitmaps in the ListView.
					// ILC_MIRROR mirrors the bitmaps if the process is mirrored,
					// but we can't rely on that being the case, and this option
					// was first introduced in Windows XP.
					// We'll flip the image here to counteract it.
					rp_image_const_ptr flipimg = icon->flip(rp_image::FLIP_H);
					assert((bool)flipimg);
					if (flipimg) {
						icon = std::move(flipimg);
					}
				}

				// Convert the icon to ARGB32 if it's isn't already.
				// If the original icon is CI8, it needs to be
				// converted to ARGB32 first. Otherwise, when
				// resizing, the "empty" background area will be black.
				if (icon->format() != rp_image::Format::ARGB32) {
					rp_image_const_ptr icon32 = icon->dup_ARGB32();
					assert((bool)icon32);
					if (icon32) {
						icon = std::move(icon32);
					}
				}

				// Resize the icon, if necessary.
				if (resizeNeeded) {
					SIZE szResize = {icon->width(), (LONG)(icon->height() * factor)};

					// NOTE: We still need to specify a background color,
					// since the ListView highlight won't show up on
					// alpha-transparent pixels.
					// TODO: Handle this in rp_image::resized()?
					// TODO: Handle theme changes?
					// TODO: Error handling.

					// Resize the icon.
					rp_image_const_ptr icon_resized = icon->resized(
						szResize.cx, szResize.cy,
						rp_image::AlignVCenter, lvBgColor[rowColorIdx]);
					assert((bool)icon_resized);
					if (icon_resized) {
						icon = std::move(icon_resized);
					}
				}

				int iImage = -1;
				HBITMAP hbmIcon;
				// FIXME: If not rescaled, C64 monochrome icons show up as completely white.
				const int icon_w = icon->width();
				const int icon_h = icon->height();
				if (icon_w == sizeListIconPhys.cx || icon_h == sizeListIconPhys.cy) {
					// No rescaling is necessary.
					hbmIcon = RpImageWin32::toHBITMAP_alpha(icon);
				} else {
					// Icon needs to be rescaled.
					// NOTE: Using nearest-neighbor scaling if it's an integer multiple for scaling up.
					bool nearest = false;
					if (sizeListIconPhys.cx > icon_w && sizeListIconPhys.cy > icon_h) {
						// NOTE: Assuming square icons.
						if (icon_w != 0) {
							nearest = ((sizeListIconPhys.cx % icon_w) == 0);
						}
					}
					// TODO: Use nearest-neighbor scaling if it's an integer multiple?
					hbmIcon = RpImageWin32::toHBITMAP_alpha(icon, sizeListIconPhys, nearest);
				}
				assert(hbmIcon != nullptr);
				if (hbmIcon) {
					const int idx = ImageList_Add(himl, hbmIcon, nullptr);
					if (idx >= 0) {
						// Icon added.
						iImage = idx;
					}
					// ImageList makes a copy of the icon.
					DeleteBitmap(hbmIcon);
				}

				// Save the ImageList index for later.
				lvData.vImageList.push_back(iImage);
			}
		}
	}

	if (isMulti) {
		lvData.pField = &field;
	}

	// Sorting methods
	// NOTE: lvData only contains the active language for RFT_LISTDATA_MULTI.
	// TODO: Make it more like the KDE version?
	lvData.sortingMethods = listDataDesc.col_attrs.sorting;
	if (listDataDesc.col_attrs.sort_col >= 0) {
		lvData.setInitialSort(listDataDesc.col_attrs.sort_col, listDataDesc.col_attrs.sort_dir);
	} else {
		// Reset the sort map to unsorted defaults.
		lvData.resetSortMap();
	}

	// Adjust column 0 width.
	lvData.col_widths[0] += lvData.col0sizeadj;

	// Set the last column to LVSCW_AUTOSIZE_USEHEADER.
	// FIXME: This doesn't account for the vertical scrollbar...
	//lvData.col_widths[lvData.col_widths.size()-1] = LVSCW_AUTOSIZE_USEHEADER

	if (!isMulti) {
		// Resize all of the columns.
		// TODO: Do this on system theme change?
		// TODO: Add a flag for 'main data column' and adjust it to
		// not exceed the viewport.
		// NOTE: Must count up; otherwise, XDBF Gamerscore ends up being too wide.
		for (int i = 0; i < colCount; i++) {
			ListView_SetColumnWidth(hListView, i, lvData.col_widths[i]);
		}
	}

	// Save the LvData.
	// TODO: Verify that std::move() works here.
	map_lvData.emplace(cId, std::move(lvData));

	// Set the virtual list item count.
	ListView_SetItemCountEx(hListView, lv_row_num,
		LVSICF_NOINVALIDATEALL | LVSICF_NOSCROLL);

	// Get the dialog margin.
	// 7x7 DLU margin is recommended by the Windows UX guidelines.
	// Reference: http://stackoverflow.com/questions/2118603/default-dialog-padding
	RECT dlgMargin = {7, 7, 8, 8};
	MapDialogRect(hDlgSheet, &dlgMargin);

	// Increase the ListView height.
	// Default: 5 rows, plus the header.
	// FIXME: Might not work for LVS_OWNERDATA.
	int cy = 0;
	if (doResize && ListView_GetItemCount(hListView) > 0) {
		if (listDataDesc.names) {
			// Get the header rect.
			HWND hHeader = ListView_GetHeader(hListView);
			assert(hHeader != nullptr);
			if (hHeader) {
				RECT rectListViewHeader;
				GetClientRect(hHeader, &rectListViewHeader);
				cy = rectListViewHeader.bottom;
			}
		}

		// Get an item rect.
		RECT rectItem;
		ListView_GetItemRect(hListView, 0, &rectItem, LVIR_BOUNDS);
		const int item_cy = (rectItem.bottom - rectItem.top);
		if (item_cy > 0) {
			// Multiply by the requested number of visible rows.
			int rows_visible = field.desc.list_data.rows_visible;
			if (rows_visible == 0) {
				rows_visible = 5;
			}
			cy += (item_cy * rows_visible);
			// Add half of the dialog margin.
			// TODO Get the ListView border size?
			cy += (dlgMargin.top/2);
		} else {
			// TODO: Can't handle this case...
			cy = size.cy;
		}
	} else {
		// TODO: Can't handle this if no items are present.
		cy = size.cy;
	}

	// Subclass the parent dialog so we can intercept HDN_DIVIDERDBLCLICK.
	SetWindowSubclass(hListView, LibWin32UI::ListViewNoDividerDblClickSubclassProc,
		static_cast<UINT_PTR>(cId),
		reinterpret_cast<DWORD_PTR>(this));

	// TODO: Skip this if cy == size.cy?
	SetWindowPos(hListView, nullptr, 0, 0, size.cx, cy,
		SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOZORDER | SWP_NOMOVE);
	return cy;
}

/**
 * Initialize a Date/Time field.
 * This function internally calls initString().
 * @param hWndTab	[in] Tab window. (for the actual control)
 * @param pt_start	[in] Starting position, in pixels.
 * @param size		[in] Width and height for a single line label.
 * @param field		[in] RomFields::Field
 * @param fieldIdx	[in] Field index
 * @return Field height, in pixels.
 */
int RP_ShellPropSheetExt_Private::initDateTime(_In_ HWND hWndTab,
	_In_ POINT pt_start, _In_ SIZE size,
	_In_ const RomFields::Field &field, _In_ int fieldIdx)
{
	if (field.data.date_time == -1) {
		// Invalid date/time.
		return initString(hWndTab, pt_start, size, field, fieldIdx,
			TC_("RomDataView", "Unknown"));
	}

	// Format the date/time.
	const tstring dateTimeStr = formatDateTime(field.data.date_time, field.flags);

	// Initialize the string.
	return initString(hWndTab, pt_start, size, field, fieldIdx,
		(likely(!dateTimeStr.empty()) ? dateTimeStr.c_str() : TC_("RomDataView", "Unknown")));
}

/**
 * Initialize an Age Ratings field.
 * This function internally calls initString().
 * @param hWndTab	[in] Tab window (for the actual control)
 * @param pt_start	[in] Starting position, in pixels
 * @param size		[in] Width and height for a single line label
 * @param field		[in] RomFields::Field
 * @param fieldIdx	[in] Field index
 * @return Field height, in pixels.
 */
int RP_ShellPropSheetExt_Private::initAgeRatings(_In_ HWND hWndTab,
	_In_ POINT pt_start, _In_ SIZE size,
	_In_ const RomFields::Field &field, _In_ int fieldIdx)
{
	const RomFields::age_ratings_t *const age_ratings = field.data.age_ratings;
	assert(age_ratings != nullptr);
	if (!age_ratings) {
		// No age ratings data.
		return initString(hWndTab, pt_start, size, field, fieldIdx,
			TC_("RomDataView", "ERROR"));
	}

	// Convert the age ratings field to a string and initialize the string field.
	return initString(hWndTab, pt_start, size, field, fieldIdx,
		U82T_s(RomFields::ageRatingsDecode(age_ratings)));
}

/**
 * Initialize a Dimensions field.
 * This function internally calls initString().
 * @param hWndTab	[in] Tab window (for the actual control)
 * @param pt_start	[in] Starting position, in pixels
 * @param size		[in] Width and height for a single line label
 * @param field		[in] RomFields::Field
 * @param fieldIdx	[in] Field index
 * @return Field height, in pixels.
 */
int RP_ShellPropSheetExt_Private::initDimensions(_In_ HWND hWndTab,
	_In_ POINT pt_start, _In_ SIZE size,
	_In_ const RomFields::Field &field, _In_ int fieldIdx)
{
	// TODO: 'x' or '×'? Using 'x' for now.
	const int *const dimensions = field.data.dimensions;
	return initString(hWndTab, pt_start, size, field, fieldIdx,
		formatDimensions(dimensions));
}

/**
 * Initialize a multi-language string field.
 * @param hWndTab	[in] Tab window (for the actual control)
 * @param pt_start	[in] Starting position, in pixels
 * @param size		[in] Width and height for a single line label
 * @param field		[in] RomFields::Field
 * @param fieldIdx	[in] Field index
 * @return Field height, in pixels.
 */
int RP_ShellPropSheetExt_Private::initStringMulti(_In_ HWND hWndTab,
	_In_ POINT pt_start, _In_ SIZE size,
	_In_ const RomFields::Field &field, _In_ int fieldIdx)
{
	// Mutli-language string.
	// NOTE: The string contents won't be initialized here.
	// They will be initialized separately, since the user will
	// be able to change the displayed language.
	HWND lblStringMulti = nullptr;
	const int field_cy = initString(hWndTab, pt_start, size, field, fieldIdx,
		nullptr, &lblStringMulti);
	if (lblStringMulti) {
		vecStringMulti.emplace_back(lblStringMulti, &field);
	}
	return field_cy;
}

/**
 * Update all multi-language fields.
 * @param user_lc User-specified language code.
 */
void RP_ShellPropSheetExt_Private::updateMulti(uint32_t user_lc)
{
	set<uint32_t> set_lc;

	// RFT_STRING_MULTI
	for (const auto &vsm : vecStringMulti) {
		const HWND lblString = vsm.first;
		const RomFields::Field *const pField = vsm.second;
		const auto *const pStr_multi = pField->data.str_multi;
		assert(pStr_multi != nullptr);
		assert(!pStr_multi->empty());
		if (!pStr_multi || pStr_multi->empty()) {
			// Invalid multi-string...
			continue;
		}

		if (!cboLanguage) {
			// Need to add all supported languages.
			// TODO: Do we need to do this for all of them, or just one?
			for (const auto &psm : *pStr_multi) {
				set_lc.insert(psm.first);
			}
		}

		// Get the string and update the text.
		const string *const pStr = RomFields::getFromStringMulti(pStr_multi, def_lc, user_lc);
		assert(pStr != nullptr);
		if (pStr != nullptr) {
			SetWindowText(lblString, LibWin32UI::unix2dos(U82T_s(*pStr)).c_str());
		} else {
			SetWindowText(lblString, _T(""));
		}
	}

	HFONT hFontDlg = GetWindowFont(hDlgSheet);

	// RFT_LISTDATA_MULTI
	for (auto &&mlvd : map_lvData) {
		LvData &lvData = mlvd.second;
		if (!lvData.pField) {
			// Not an RFT_LISTDATA_MULTI.
			continue;
		}

		const HWND hListView = lvData.hListView;
		const RomFields::Field *const pField = lvData.pField;
		const auto *const pListData_multi = pField->data.list_data.data.multi;
		assert(pListData_multi != nullptr);
		assert(!pListData_multi->empty());
		if (!pListData_multi || pListData_multi->empty()) {
			// Invalid RFT_LISTDATA_MULTI...
			continue;
		}

		if (!cboLanguage) {
			// Need to add all supported languages.
			// TODO: Do we need to do this for all of them, or just one?
			for (const auto &pldm : *pListData_multi) {
				set_lc.insert(pldm.first);
			}
		}

		// Get the ListData_t.
		const auto *const pListData = RomFields::getFromListDataMulti(pListData_multi, def_lc, user_lc);
		assert(pListData != nullptr);
		if (pListData != nullptr) {
			// Column count.
			const auto &listDataDesc = pField->desc.list_data;
			int colCount = 1;
			if (listDataDesc.names) {
				colCount = (int)listDataDesc.names->size();
			} else {
				// No column headers.
				// Use the first row.
				assert(!pListData->empty());
				if (!pListData->empty()) {
					colCount = (int)pListData->at(0).size();
				}
			}

			// Column widths.
			lvData.col_widths.clear();
			lvData.col_widths.resize(colCount);

			// Dialog font and device context.
			// NOTE: Using the parent dialog's font.
			AutoGetDC_font hDC(hListView, hFontDlg);

			if (listDataDesc.names) {
				// Measure header text widths.
				auto iter = listDataDesc.names->cbegin();
				for (int col = 0; col < colCount; ++iter, col++) {
					const string &str = *iter;
					if (str.empty())
						continue;

					// NOTE: ListView is limited to 260 characters. (259+1)
					// TODO: Handle surrogate pairs?
					tstring tstr = U82T_s(str);
					if (tstr.size() >= 260) {
						// Reduce to 256 and add "..."
						tstr.resize(256);
						tstr += _T("...");
					}
					lvData.col_widths[col] = LibWin32UI::measureStringForListView(hDC, tstr);
				}
			}

			// Get the ListView data vector for LVS_OWNERDATA.
			vector<vector<tstring> > &vvStr = lvData.vvStr;

			auto iter_ld_row = pListData->cbegin();
			auto iter_vvStr_row = vvStr.begin();
			const auto pListData_cend = pListData->cend();
			const auto vvStr_end = vvStr.end();
			for (; iter_ld_row != pListData_cend && iter_vvStr_row != vvStr_end;
			     ++iter_ld_row, ++iter_vvStr_row)
			{
				const vector<string> &src_data_row = *iter_ld_row;
				vector<tstring> &dest_data_row = *iter_vvStr_row;

				int col = 0;
				unsigned int is_timestamp = listDataDesc.col_attrs.is_timestamp;
				auto iter_sdr = src_data_row.cbegin();
				auto iter_ddr = dest_data_row.begin();
				const auto src_data_row_cend = src_data_row.cend();
				const auto dest_data_row_end = dest_data_row.end();
				for (; iter_sdr != src_data_row_cend && iter_ddr != dest_data_row_end;
				     ++iter_sdr, ++iter_ddr, col++, is_timestamp >>= 1)
				{
					// NOTE: ListView is limited to 260 characters. (259+1)
					// TODO: Handle surrogate pairs?
					tstring tstr;
					if (unlikely((is_timestamp & 1) && iter_sdr->size() == sizeof(int64_t))) {
						// Timestamp column. Format the timestamp.
						RomFields::TimeString_t time_string;
						memcpy(time_string.str, iter_sdr->data(), 8);

						tstr = formatDateTime(time_string.time,
							listDataDesc.col_attrs.dtflags);
						if (unlikely(tstr.empty())) {
							tstr = TC_("RomData", "Unknown");
						}
					} else {
						tstr = U82T_s(*iter_sdr);
						if (tstr.size() >= 260) {
							// Reduce to 256 and add "..."
							tstr.resize(256);
							tstr += _T("...");
						}
					}

					const int width = LibWin32UI::measureStringForListView(hDC, tstr);
					if (col < colCount) {
						lvData.col_widths[col] = std::max(lvData.col_widths[col], width);
					}
					*iter_ddr = std::move(tstr);
				}
			}

			// Add the column 0 size adjustment.
			lvData.col_widths[0] += lvData.col0sizeadj;

			// Set the last column to LVSCW_AUTOSIZE_USEHEADER.
			// FIXME: This doesn't account for the vertical scrollbar...
			//lvData.col_widths[lvData.col_widths.size()-1] = LVSCW_AUTOSIZE_USEHEADER;

			// Resize the columns to fit the contents.
			// NOTE: Only done on first load. (TODO: Maybe we should do it every time?)
			// TODO: Need to measure text...
			if (!cboLanguage) {
				// TODO: Do this on system theme change?
				// TODO: Add a flag for 'main data column' and adjust it to
				// not exceed the viewport.
				// NOTE: Must count up; otherwise, XDBF Gamerscore ends up being too wide.
				for (int i = 0; i < colCount; i++) {
					ListView_SetColumnWidth(hListView, i, lvData.col_widths[i]);
				}
			}

			// Redraw all items.
			ListView_RedrawItems(hListView, 0, static_cast<int>(vvStr.size()));
		}
	}

	if (!cboLanguage && set_lc.size() > 1) {
		// Create the language combobox.
		// FIXME: May need to create this after the header row
		// in order to preserve tab order. Need to check the
		// KDE and GTK+ versions, too.
		// NOTE: We won't have the correct position or size until LCs are set.

		// NOTE: We need to initialize combobox height to iconSize * 8 in order
		// to allow up to 8 entries to be displayed at once.
		const UINT dpi = rp_GetDpiForWindow(hDlgSheet);
		int iconSize;
		if (dpi < 120) {
			// [96,120) dpi: Use 16x16.
			iconSize = 16;
		} else if (dpi <= 144) {
			// [120,144] dpi: Use 24x24.
			// TODO: Maybe needs to be slightly higher?
			iconSize = 24;
		} else {
			// >144dpi: Use 32x32.
			iconSize = 32;
		}

		// Calculate text height.
		SIZE textSize;
		if (LibWin32UI::measureTextSize(hDlgSheet, hFontDlg, _T("Ay"), &textSize) != 0) {
			// Error getting text height.
			textSize.cy = 0;
		}

		LanguageComboBoxRegister();
		cboLanguage = CreateWindowEx(WS_EX_NOPARENTNOTIFY,
			WC_LANGUAGECOMBOBOX, nullptr,
			CBS_DROPDOWNLIST | WS_CHILD | WS_TABSTOP | WS_VISIBLE,
			0, 0, 0, (iconSize * 8) + textSize.cy - (textSize.cy / 8),
			hDlgSheet, (HMENU)(INT_PTR)IDC_CBO_LANGUAGE, nullptr, nullptr);
		SetWindowFont(cboLanguage, hFontDlg, false);

		// Set the languages.
		// NOTE: LanguageComboBox uses a 0-terminated array, so we'll
		// need to convert the std::set<> to an std::vector<>.
		vector<uint32_t> vec_lc;
		vec_lc.reserve(set_lc.size() + 1);
		vec_lc.assign(set_lc.cbegin(), set_lc.cend());
		vec_lc.push_back(0);
		LanguageComboBox_SetForcePAL(cboLanguage, romData->isPAL());
		LanguageComboBox_SetLCs(cboLanguage, vec_lc.data());

		// Get the minimum size for the combobox.
		const LPARAM minSize = LanguageComboBox_GetMinSize(cboLanguage);
		SetWindowPos(cboLanguage, nullptr,
			rectHeader.right - GET_X_LPARAM(minSize), rectHeader.top,
			GET_X_LPARAM(minSize), GET_Y_LPARAM(minSize),
			SWP_NOOWNERZORDER | SWP_NOZORDER);

		// Select the default language.
		uint32_t lc_to_set = 0;
		const auto set_lc_end = set_lc.end();
		if (set_lc.find(def_lc) != set_lc_end) {
			// def_lc was found.
			lc_to_set = def_lc;
		} else if (set_lc.find('en') != set_lc_end) {
			// 'en' was found.
			lc_to_set = 'en';
		} else {
			// Unknown. Select the first language.
			if (!set_lc.empty()) {
				lc_to_set = *(set_lc.cbegin());
			}
		}
		LanguageComboBox_SetSelectedLC(cboLanguage, lc_to_set);

		// Get the dialog margin.
		// 7x7 DLU margin is recommended by the Windows UX guidelines.
		// Reference: http://stackoverflow.com/questions/2118603/default-dialog-padding
		RECT dlgMargin = { 7, 7, 8, 8 };
		MapDialogRect(hDlgSheet, &dlgMargin);

		// Adjust the header row.
		const int adj = (GET_X_LPARAM(minSize) + dlgMargin.left) / 2;
		if (lblSysInfo) {
			ptSysInfo.x -= adj;
			SetWindowPos(lblSysInfo, nullptr, ptSysInfo.x, ptSysInfo.y, 0, 0,
				SWP_NOSIZE | SWP_NOOWNERZORDER | SWP_NOZORDER);
		}
		if (lblBanner) {
			POINT pos = lblBanner->position();
			pos.x -= adj;
			lblBanner->setPosition(pos);
		}
		if (lblIcon) {
			POINT pos = lblIcon->position();
			pos.x -= adj;
			lblIcon->setPosition(pos);
		}
	}
}

/**
 * Initialize the dialog. (hDlgSheet)
 * Called by WM_INITDIALOG.
 */
void RP_ShellPropSheetExt_Private::initDialog(void)
{
	assert(hDlgSheet != nullptr);
	assert(romData != nullptr);
	if (!hDlgSheet || !romData) {
		// No dialog, or no ROM data loaded.
		return;
	}

	// Set the dialog to allow automatic right-to-left adjustment
	// if the system is using an RTL language.
	if (dwExStyleRTL != 0) {
		LONG_PTR lpExStyle = GetWindowLongPtr(hDlgSheet, GWL_EXSTYLE);
		lpExStyle |= WS_EX_LAYOUTRTL;
		SetWindowLongPtr(hDlgSheet, GWL_EXSTYLE, lpExStyle);
	}

	// Determine if Dark Mode is enabled.
	isDarkModeEnabled = VerifyDialogDarkMode(GetParent(hDlgSheet));

	// Get the fields.
	const RomFields *const pFields = romData->fields();
	assert(pFields != nullptr);
	if (!pFields) {
		// No fields.
		// TODO: Show an error?
		return;
	}
	const int count = pFields->count();

	// Make sure we have all required window classes available.
	// Reference: https://docs.microsoft.com/en-us/windows/win32/api/commctrl/ns-commctrl-initcommoncontrolsex
	INITCOMMONCONTROLSEX initCommCtrl;
	initCommCtrl.dwSize = sizeof(initCommCtrl);
	initCommCtrl.dwICC =
		ICC_LISTVIEW_CLASSES |
		ICC_LINK_CLASS |
		ICC_TAB_CLASSES |
		ICC_USEREX_CLASSES;
	// TODO: Also ICC_STANDARD_CLASSES on XP+?
	InitCommonControlsEx(&initCommCtrl);

	// Initialize the font handler.
	fontHandler.setWindow(hDlgSheet);

	// Device context for text measurement
	AutoGetDC_font hDC(hDlgSheet, GetWindowFont(hDlgSheet));

	// Convert the bitfield description names to the
	// native Windows encoding once.
	vector<tstring> t_desc_text;
	t_desc_text.reserve(count);

	// Tab count.
	int tabCount = pFields->tabCount();
	if (tabCount < 1) {
		tabCount = 1;
	}

	// Determine the maximum length of all field names.
	// TODO: Line breaks?
	unique_ptr<int[]> a_max_text_width(new int[tabCount]());

	// tr: Field description label.
	const char *const desc_label_fmt = C_("RomDataView", "{:s}:");
	for (const RomFields::Field &field : *pFields) {
		assert(field.isValid());
		if (!field.isValid())
			continue;

		tstring desc_text = U82T_s(fmt::format(FRUN(desc_label_fmt), field.name));

		// Get the width of this specific entry.
		// TODO: Use measureTextSize()?
		SIZE textSize;
		if (field.type == RomFields::RFT_STRING &&
		    field.flags & RomFields::STRF_WARNING)
		{
			// Label is bold.
			HFONT hFontOrig = nullptr;
			HFONT hFontBold = fontHandler.boldFont();
			if (hFontBold) {
				hFontOrig = SelectFont(hDC, hFontBold);
			}
			GetTextExtentPoint32(hDC, desc_text.data(),
				static_cast<int>(desc_text.size()), &textSize);
			if (hFontBold) {
				SelectFont(hDC, hFontOrig);
			}
		}
		else
		{
			// Regular font.
			GetTextExtentPoint32(hDC, desc_text.data(),
				static_cast<int>(desc_text.size()), &textSize);
		}

		assert(field.tabIdx >= 0);
		assert(field.tabIdx < tabCount);
		if (field.tabIdx >= 0 && field.tabIdx < tabCount) {
			if (textSize.cx > a_max_text_width[field.tabIdx]) {
				a_max_text_width[field.tabIdx] = textSize.cx;
			}
		}

		// Save for later.
		t_desc_text.push_back(std::move(desc_text));
	}

	// Add additional spacing between the ':' and the field.
	// TODO: Use measureTextSize()?
	// TODO: Reduce to 1 space?
	SIZE textSize;
	GetTextExtentPoint32(hDC, _T("  "), 2, &textSize);
	for (int i = tabCount - 1; i >= 0; i--) {
		a_max_text_width[i] += textSize.cx;
	}

	// Create the ROM field widgets.
	// Each static control is max_text_width pixels wide
	// and 8 DLUs tall, plus 4 vertical DLUs for spacing.
	RECT tmpRect = {0, 0, 0, 8+4};
	MapDialogRect(hDlgSheet, &tmpRect);
	SIZE descSize = {0, tmpRect.bottom};
	this->lblDescHeight = descSize.cy;

	// Get the dialog margin.
	// 7x7 DLU margin is recommended by the Windows UX guidelines.
	// Reference: http://stackoverflow.com/questions/2118603/default-dialog-padding
	RECT dlgMargin = {7, 7, 8, 8};
	MapDialogRect(hDlgSheet, &dlgMargin);

	// Get the dialog size.
	// - fullDlgRect: Full dialog size
	// - dlgRect: Adjusted dialog size.
	// FIXME: Vertical height is off by 3px on Win7...
	// Verified with WinSpy++: expected 341x408, got 341x405.
	RECT fullDlgRect, dlgRect;
	GetClientRect(hDlgSheet, &fullDlgRect);
	dlgRect = fullDlgRect;
	// Adjust the rectangle for margins.
	InflateRect(&dlgRect, -dlgMargin.left, -dlgMargin.top);
	// Calculate the size for convenience purposes.
	dlgSize.cx = dlgRect.right - dlgRect.left;
	dlgSize.cy = dlgRect.bottom - dlgRect.top;

	// Increase the total widget width by half of the margin.
	InflateRect(&dlgRect, dlgMargin.left/2, 0);
	dlgSize.cx += dlgMargin.left - 1;

	// Current position.
	POINT headerPt = {dlgRect.left, dlgRect.top};
	int dlg_value_width_base = dlgSize.cx - 1;

	// Create the header row.
	const SIZE header_size = {dlgSize.cx, descSize.cy};
	const int headerH = createHeaderRow(headerPt, header_size);
	// Save the header rect for later.
	rectHeader.left = headerPt.x;
	rectHeader.top = headerPt.y;
	rectHeader.right = headerPt.x + dlgSize.cx;
	rectHeader.bottom = headerPt.y + headerH;

	// Adjust values for the tabs.
	// TODO: Ratio instead of absolute 2px?
	dlgRect.top += (headerH + 2);
	dlgSize.cy -= (headerH + 2);
	headerPt.y += headerH;

	HFONT hFontDlg = GetWindowFont(hDlgSheet);

	// Do we need to create a tab widget?
	if (tabCount > 1) {
		// TODO: Do this regardless of tabs?
		// NOTE: Margin with this change on Win7 is now 9px left, 12px bottom.
		dlgRect.bottom = fullDlgRect.bottom - dlgRect.left;
		dlgSize.cy = dlgRect.bottom - dlgRect.top;

		// Create the tab widget.
		tabs.resize(tabCount);
		tabWidget = CreateWindowEx(WS_EX_NOPARENTNOTIFY | WS_EX_TRANSPARENT | dwExStyleRTL,
			WC_TABCONTROL, nullptr,
			WS_CHILD | WS_TABSTOP | WS_VISIBLE,
			dlgRect.left, dlgRect.top, dlgSize.cx, dlgSize.cy,
			hDlgSheet, (HMENU)(INT_PTR)IDC_TAB_WIDGET, nullptr, nullptr);
		SetWindowFont(tabWidget, hFontDlg, false);

		// Add tabs.
		// NOTE: Tabs must be added *before* calling TabCtrl_AdjustRect();
		// otherwise, the tab bar height won't be taken into account.
		for (int i = 0; i < tabCount; i++) {
			// Create a tab.
			const char *name = pFields->tabName(i);
			if (!name) {
				// Skip this tab.
				continue;
			}
			const tstring tstr = U82T_c(name);

			TCITEM tcItem;
			tcItem.mask = TCIF_TEXT;
			tcItem.pszText = const_cast<LPTSTR>(tstr.c_str());
			// FIXME: Does the index work correctly if a tab is skipped?
			TabCtrl_InsertItem(tabWidget, i, &tcItem);
		}

		// Adjust the dialog size for subtabs.
		TabCtrl_AdjustRect(tabWidget, false, &dlgRect);
		// Update dlgSize.
		dlgSize.cx = dlgRect.right - dlgRect.left;
		dlgSize.cy = dlgRect.bottom - dlgRect.top;
		iTabHeightOrig = dlgSize.cy;	// for MessageWidget
		// Update dlg_value_width.
		// FIXME: Results in 9px left, 8px right margins for RFT_LISTDATA.
		dlg_value_width_base = dlgSize.cx - dlgMargin.left - 1;

		// Create windows for each tab.
		DWORD swpFlags = SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOZORDER | SWP_SHOWWINDOW;
		for (int i = 0; i < tabCount; i++) {
			if (!pFields->tabName(i)) {
				// Skip this tab.
				continue;
			}

			auto &tab = tabs[i];

			// Create a child dialog for the tab.
			tab.hDlg = CreateDialog(HINST_THISCOMPONENT,
				MAKEINTRESOURCE(IDD_SUBTAB_CHILD_DIALOG),
				hDlgSheet, SubtabDlgProc);
			SetWindowPos(tab.hDlg, nullptr,
				dlgRect.left, dlgRect.top,
				dlgSize.cx, dlgSize.cy,
				swpFlags);
			// Hide subsequent tabs.
			swpFlags = SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOZORDER | SWP_HIDEWINDOW;

			// Store the D object pointer with this particular tab dialog.
			SetWindowLongPtr(tab.hDlg, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
			// Store the D object pointer with this particular tab dialog.
			SetProp(tab.hDlg, TAB_PTR_PROP, static_cast<HANDLE>(&tab));

			// Current point should be equal to the margins.
			// FIXME: On both WinXP and Win7, ths ends up with an
			// 8px left margin, and 6px top/right margins.
			// (Bottom margin is 6px on WinXP, 7px on Win7.)
			tab.curPt.x = dlgMargin.left/2;
			tab.curPt.y = dlgMargin.top/2;
		}
	} else {
		// No tabs.
		// Don't create a WC_TABCONTROL, but do create a
		// child dialog in order to handle scrolling.
		tabs.resize(1);
		auto &tab = tabs[0];

		// for MessageWidget
		iTabHeightOrig = dlgSize.cy;

		// Create a child dialog.
		tab.hDlg = CreateDialog(HINST_THISCOMPONENT,
			MAKEINTRESOURCE(IDD_SUBTAB_CHILD_DIALOG),
			hDlgSheet, SubtabDlgProc);
		SetWindowPos(tab.hDlg, nullptr,
			dlgRect.left, dlgRect.top,
			dlgSize.cx, dlgSize.cy,
			SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOZORDER | SWP_SHOWWINDOW);

		// Store the D object pointer with this particular tab dialog.
		SetWindowLongPtr(tab.hDlg, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
		// Store the D object pointer with this particular tab dialog.
		SetProp(tab.hDlg, TAB_PTR_PROP, static_cast<HANDLE>(&tab));

		// We already have margins on the dialog position,
		// so we shouldn't have margins here.
		tab.curPt.x = 0;
		tab.curPt.y = 0;
	}

	int fieldIdx = 0;	// needed for control IDs
	auto iter_desc = t_desc_text.cbegin();
	const auto pFields_cend = pFields->cend();
	for (auto iter = pFields->cbegin(); iter != pFields_cend; ++iter, ++iter_desc, fieldIdx++) {
		assert(iter_desc != t_desc_text.cend());
		const RomFields::Field &field = *iter;
		assert(field.isValid());
		if (!field.isValid()) {
			continue;
		}

		// Verify the tab index.
		const int tabIdx = field.tabIdx;
		assert(tabIdx >= 0 && tabIdx < (int)tabs.size());
		if (tabIdx < 0 || tabIdx >= (int)tabs.size()) {
			// Tab index is out of bounds.
			continue;
		} else if (!tabs[tabIdx].hDlg) {
			// Tab name is empty. Tab is hidden.
			continue;
		}

		// Current tab.
		auto &tab = tabs[tabIdx];

		// Description width is based on the tab index.
		descSize.cx = a_max_text_width[tabIdx];

		// Create the static text widget. (FIXME: Disable mnemonics?)
		HWND hStatic = CreateWindowEx(WS_EX_NOPARENTNOTIFY | WS_EX_TRANSPARENT,
			WC_STATIC, iter_desc->c_str(),
			WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | SS_LEFT,
			tab.curPt.x, tab.curPt.y, descSize.cx, descSize.cy,
			tab.hDlg, (HMENU)(INT_PTR)IDC_STATIC_DESC(fieldIdx), nullptr, nullptr);
		SetWindowFont(hStatic, hFontDlg, false);

		// Create the value widget.
		int field_cy = descSize.cy;	// Default row size.
		const POINT pt_start = {tab.curPt.x + descSize.cx, tab.curPt.y};
		SIZE size = {dlg_value_width_base - descSize.cx, field_cy};
		switch (field.type) {
			case RomFields::RFT_INVALID:
				// Should not happen due to the above check...
				assert(!"Field type is RFT_INVALID");
				field_cy = 0;
				break;
			default:
				// Unsupported data type.
				assert(!"Unsupported RomFields::RomFieldsType.");
				field_cy = 0;
				break;

			case RomFields::RFT_STRING:
				// String data.
				field_cy = initString(tab.hDlg, pt_start, size, field, fieldIdx, nullptr);
				break;
			case RomFields::RFT_BITFIELD:
				// Create checkboxes starting at the current point.
				field_cy = initBitfield(tab.hDlg, pt_start, field, fieldIdx);
				break;

			case RomFields::RFT_LISTDATA: {
				// Create a ListView control.
				size.cy *= 6;	// TODO: Is this needed?
				POINT pt_ListData = pt_start;

				// Should the RFT_LISTDATA be placed on its own row?
				bool doVBox = false;
				if (field.flags & RomFields::RFT_LISTDATA_SEPARATE_ROW) {
					// Separate row.
					size.cx = dlgSize.cx - 1;
					// NOTE: This varies depending on if we have subtabs.
					if (tabCount > 1) {
						// Subtract the dialog margin.
						size.cx -= dlgMargin.left;
					} else {
						// Subtract another pixel.
						size.cx--;
					}
					pt_ListData.x = tab.curPt.x;
					pt_ListData.y += (descSize.cy - (dlgMargin.top/3));

					// If this is the last RFT_LISTDATA in the tab,
					// extend it vertically.
					if (tabIdx + 1 == tabCount && fieldIdx == count-1) {
						// Last tab, and last field.
						doVBox = true;
					} else {
						// Check if the next field is on the next tab.
						RomFields::const_iterator nextIter = iter;
						++nextIter;
						if (nextIter != pFields_cend && nextIter->tabIdx != tabIdx) {
							// Next field is on the next tab.
							doVBox = true;
						}
					}

					if (doVBox) {
						// Extend it vertically.
						size.cy = dlgSize.cy - pt_ListData.y;
						if (tabCount > 1) {
							// FIXME: This seems a bit wonky...
							size.cy -= ((dlgMargin.top / 2) + 1);
						} else {
							// This also seems wonky...
							size.cy += dlgRect.top - 1;
						}
					}
				}

				field_cy = initListData(tab.hDlg, pt_ListData, size, !doVBox, field, fieldIdx);
				if (field_cy > 0) {
					// Add the extra row if necessary.
					if (field.flags & RomFields::RFT_LISTDATA_SEPARATE_ROW) {
						const int szAdj = descSize.cy - (dlgMargin.top/3);
						field_cy += szAdj;
						// Reduce the hStatic size slightly.
						SetWindowPos(hStatic, nullptr, 0, 0, descSize.cx, szAdj,
							SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOZORDER | SWP_NOMOVE);
					}
				}
				break;
			}

			case RomFields::RFT_DATETIME:
				// Date/Time in Unix format.
				field_cy = initDateTime(tab.hDlg, pt_start, size, field, fieldIdx);
				break;
			case RomFields::RFT_AGE_RATINGS:
				// Age Ratings field.
				field_cy = initAgeRatings(tab.hDlg, pt_start, size, field, fieldIdx);
				break;
			case RomFields::RFT_DIMENSIONS:
				// Dimensions field.
				field_cy = initDimensions(tab.hDlg, pt_start, size, field, fieldIdx);
				break;
			case RomFields::RFT_STRING_MULTI:
				// Multi-language string field.
				field_cy = initStringMulti(tab.hDlg, pt_start, size, field, fieldIdx);
				break;
		}

		if (field_cy > 0) {
			// Next row.
			tab.curPt.y += field_cy;
		} else /* if (field_cy == 0) */ {
			// Failed to initialize the widget.
			// Remove the description label.
			DestroyWindow(hStatic);
		}
	}

	// Update scrollbar settings.
	// TODO: If a VScroll bar is added, adjust widths of RFT_LISTDATA.
	// TODO: HScroll bar?
	for (const tab &tab : tabs) {
		// Set scroll info.
		// FIXME: Separate child dialog for no tabs.

		// VScroll bar
		SCROLLINFO si;
		si.cbSize = sizeof(SCROLLINFO);
		si.fMask = SIF_ALL;
		si.nMin = 0;
		si.nMax = tab.curPt.y - 2;	// max is exclusive
		si.nPage = dlgSize.cy;
		si.nPos = 0;
		SetScrollInfo(tab.hDlg, SB_VERT, &si, TRUE);

		// HScroll bar
		// NOTE: ReactOS 0.4.13 is showing an HScroll bar, even though
		// the child dialog doesn't have WS_HSCROLL set.
		si.nMin = 0;
		si.nMax = 1;
		si.nPage = 2;
		si.nPos = 0;
		SetScrollInfo(tab.hDlg, SB_HORZ, &si, TRUE);
	}

	// Initial update of RFT_MULTI_STRING fields.
	if (!vecStringMulti.empty()) {
		def_lc = pFields->defaultLanguageCode();
		updateMulti(0);
	}

	// Check for "viewed" achievements.
	romData->checkViewedAchievements();

	// Register for WTS session notifications. (Remote Desktop)
	wts.registerSessionNotification(hDlgSheet, NOTIFY_FOR_THIS_SESSION);

	// Window is fully initialized.
	isFullyInit = true;
}

/** RP_ShellPropSheetExt **/

RP_ShellPropSheetExt::RP_ShellPropSheetExt()
	: d_ptr(nullptr)
{
	// NOTE: d_ptr is not initialized until we receive a valid
	// ROM file. This reduces overhead in cases where there are
	// lots of files with ROM-like file extensions but aren't
	// actually supported by rom-properties.
}

RP_ShellPropSheetExt::~RP_ShellPropSheetExt()
{
	delete d_ptr;
}

/** IUnknown **/
// Reference: https://docs.microsoft.com/en-us/office/client-developer/outlook/mapi/implementing-iunknown-in-c-plus-plus

IFACEMETHODIMP RP_ShellPropSheetExt::QueryInterface(_In_ REFIID riid, _Outptr_ LPVOID *ppvObj)
{
#ifdef _MSC_VER
#  pragma warning(push)
#  pragma warning(disable: 4365 4838)
#endif /* _MSC_VER */
	static const QITAB rgqit[] = {
		QITABENT(RP_ShellPropSheetExt, IShellExtInit),
		QITABENT(RP_ShellPropSheetExt, IShellPropSheetExt),
		{ 0, 0 }
	};
#ifdef _MSC_VER
#  pragma warning(pop)
#endif /* _MSC_VER */
	return LibWin32Common::rp_QISearch(this, rgqit, riid, ppvObj);
}

/** IShellExtInit **/
// Reference: https://docs.microsoft.com/en-us/windows/win32/api/shobjidl_core/nf-shobjidl_core-ishellextinit-initialize [Initialize()]

IFACEMETHODIMP RP_ShellPropSheetExt::Initialize(
	_In_ LPCITEMIDLIST pidlFolder, _In_ LPDATAOBJECT pDataObj, _In_ HKEY hKeyProgID)
{
	((void)pidlFolder);
	((void)hKeyProgID);

	// Based on CppShellExtPropSheetHandler.
	// https://code.msdn.microsoft.com/windowsapps/CppShellExtPropSheetHandler-d93b49b7
	if (!pDataObj) {
		return E_INVALIDARG;
	}

	// TODO: Handle CFSTR_MOUNTEDVOLUME for volumes mounted on an NTFS mount point.
	FORMATETC fe = { CF_HDROP, nullptr, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
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

	HRESULT hr = E_FAIL;
	UINT nFiles, cchFilename;
	TCHAR *tfilename = nullptr;

	const Config *config;

	// Determine how many files are involved in this operation. This
	// code sample displays the custom context menu item when only
	// one file is selected.
	nFiles = DragQueryFile(hDrop, 0xFFFFFFFF, nullptr, 0);
	if (nFiles != 1) {
		// Wrong file count.
		goto cleanup;
	}

	// Get the path of the file.
	cchFilename = DragQueryFile(hDrop, 0, nullptr, 0);
	if (cchFilename == 0) {
		// No filename.
		goto cleanup;
	}

	tfilename = static_cast<TCHAR*>(malloc((cchFilename+1) * sizeof(TCHAR)));
	if (!tfilename) {
		hr = E_OUTOFMEMORY;
		goto cleanup;
	}
	cchFilename = DragQueryFile(hDrop, 0, tfilename, cchFilename+1);
	if (cchFilename == 0) {
		// No filename.
		goto cleanup;
	}

	// Check for "bad" file systems.
	config = Config::instance();
	if (FileSystem::isOnBadFS(tfilename, config->getBoolConfigOption(Config::BoolConfig::Options_EnableThumbnailOnNetworkFS))) {
		// This file is on a "bad" file system.
		goto cleanup;
	}

	// Attempt to open the file using RomDataFactory.
	{
		const RomDataPtr romData_tmp = RomDataFactory::create(tfilename);
		if (!romData_tmp) {
			// Could not open the RomData object.
			goto cleanup;
		}

		// NOTE: We're not saving the RomData object at this point.
		// We only want to open the RomData if the "ROM Properties"
		// tab is clicked, because otherwise the file will be held
		// open and may block the user from changing attributes.
	}

	// Save the filename in the private class for later.
	if (!d_ptr) {
		d_ptr = new RP_ShellPropSheetExt_Private(this, tfilename);
	}

	// Make sure the Dark Mode function pointers are initialized.
	InitDarkModePFNs();

	// Everything's good to go.
	hr = S_OK;

cleanup:
	GlobalUnlock(stm.hGlobal);
	ReleaseStgMedium(&stm);
	free(tfilename);

	// If any value other than S_OK is returned from the method, the property 
	// sheet is not displayed.
	return hr;
}

/** IShellPropSheetExt **/
// Reference: https://docs.microsoft.com/en-us/windows/win32/api/shobjidl_core/nn-shobjidl_core-ishellpropsheetext

IFACEMETHODIMP RP_ShellPropSheetExt::AddPages(_In_ LPFNADDPROPSHEETPAGE pfnAddPage, LPARAM lParam)
{
	// Based on CppShellExtPropSheetHandler.
	// https://code.msdn.microsoft.com/windowsapps/CppShellExtPropSheetHandler-d93b49b7
	if (!d_ptr) {
		// Not initialized.
		return E_FAIL;
	}

	// tr: Tab title.
	const tstring tsTabTitle = TC_("RomDataView", "ROM Properties");

	// Create a property sheet page.
	PROPSHEETPAGE psp;
	psp.dwSize = sizeof(psp);
	psp.dwFlags = PSP_USECALLBACK | PSP_USETITLE;
	psp.hInstance = HINST_THISCOMPONENT;
	psp.pszTemplate = MAKEINTRESOURCE(IDD_PROPERTY_SHEET);
	psp.pszIcon = nullptr;
	psp.pszTitle = tsTabTitle.c_str();
	psp.pfnDlgProc = RP_ShellPropSheetExt_Private::DlgProc;
	psp.pcRefParent = nullptr;
	psp.pfnCallback = RP_ShellPropSheetExt_Private::CallbackProc;
	psp.lParam = reinterpret_cast<LPARAM>(this);

	HPROPSHEETPAGE hPage = CreatePropertySheetPage(&psp);
	if (hPage == nullptr) {
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

IFACEMETHODIMP RP_ShellPropSheetExt::ReplacePage(UINT uPageID,
	_In_ LPFNADDPROPSHEETPAGE pfnReplaceWith, LPARAM lParam)
{
	// Not used.
	RP_UNUSED(uPageID);
	RP_UNUSED(pfnReplaceWith);
	RP_UNUSED(lParam);
	return E_NOTIMPL;
}

/** Property sheet callback functions. **/

/**
 * ListView GetDispInfo function
 * @param plvdi	[in/out] NMLVDISPINFO
 * @return TRUE if handled; FALSE if not.
 */
inline BOOL RP_ShellPropSheetExtPrivate::ListView_GetDispInfo(NMLVDISPINFO *plvdi)
{
	// TODO: Assertions for row/column indexes.
	LVITEM *const plvItem = &plvdi->item;
	const unsigned int idFrom = static_cast<unsigned int>(plvdi->hdr.idFrom);

	auto iter_lvData = map_lvData.find(idFrom);
	if (iter_lvData == map_lvData.end()) {
		// ListView data not found...
		return FALSE;
	}
	const LvData &lvData = iter_lvData->second;

	assert(lvData.vvStr.size() == lvData.vSortMap.size());
	if (lvData.vvStr.size() != lvData.vSortMap.size()) {
		// Size mismatch!
		return FALSE;
	}

	// Get the real item number using the sort map.
	int iItem = plvItem->iItem;
	if (iItem < 0 || iItem >= static_cast<int>(lvData.vSortMap.size())) {
		// Out of range...
		return FALSE;
	}
	iItem = lvData.vSortMap[iItem];

	BOOL bRet = FALSE;

	if (plvItem->mask & LVIF_TEXT) {
		// Fill in text.
		const auto &row_data = lvData.vvStr[iItem];

		// Is the column in range?
		if (plvItem->iSubItem >= 0 && plvItem->iSubItem < static_cast<int>(row_data.size())) {
			// Return the string data.
			// NOTE: ListView is limited to 260 characters. (259+1)
			// This should be handled when the ListView data is initialized.
			// If it isn't, we'll truncate the string here (but without the "...").
			const tstring &tstr = row_data[plvItem->iSubItem];
			_tcsncpy_s(plvItem->pszText, plvItem->cchTextMax, tstr.c_str(), _TRUNCATE);
			bRet = TRUE;
		}
	}

	if (plvItem->mask & LVIF_IMAGE) {
		// Fill in the ImageList index.
		// NOTE: Only valid for the base item.
		if (plvItem->iSubItem == 0) {
			// LVS_OWNERDATA prevents LVS_EX_CHECKBOXES from working correctly,
			// so we have to implement it here.
			// Reference: https://www.codeproject.com/Articles/29064/CGridListCtrlEx-Grid-Control-Based-on-CListCtrl
			if (lvData.hasCheckboxes) {
				// We have checkboxes.
				// Enable state handling.
				plvItem->mask |= LVIF_STATE;
				plvItem->stateMask = LVIS_STATEIMAGEMASK;
				plvItem->state = INDEXTOSTATEIMAGEMASK(
					((lvData.checkboxes & (1U << iItem)) ? 2 : 1));
				bRet = TRUE;
			} else if (!lvData.vImageList.empty()) {
				// We have an ImageList.
				assert(lvData.vImageList.size() == lvData.vSortMap.size());
				if (lvData.vImageList.size() != lvData.vSortMap.size()) {
					// Size mismatch!
					return FALSE;
				}

				const int iImage = lvData.vImageList[iItem];
				if (iImage >= 0) {
					// Set the ImageList index.
					plvItem->iImage = iImage;
					bRet = TRUE;
				}
			}
		}
	}

	return bRet;
}

/**
 * ListView ColumnClick function
 * @param plv	[in] NMLISTVIEW (only iSubItem is significant)
 * @return TRUE if handled; FALSE if not.
 */
inline BOOL RP_ShellPropSheetExt_Private::ListView_ColumnClick(const NMLISTVIEW *plv)
{
	// NOTE: GTK+ and Qt use "down" to indicate "ascending".
	// Windows uses "up" to indicate "ascending".
	// NOTE 2: According to MSDN, HDF_SORTUP/HDF_SORTDOWN are
	// supported starting with COMMCTRL 6.00 (WinXP).
	const unsigned int idFrom = static_cast<unsigned int>(plv->hdr.idFrom);

	// Get the ListView data.
	auto iter_lvData = map_lvData.find(idFrom);
	if (iter_lvData == map_lvData.end()) {
		// ListView data not found...
		return FALSE;
	}
	LvData &lvData = iter_lvData->second;
	return lvData.toggleSortColumn(plv->iSubItem);
}

/**
 * Header DividerDblClick function
 * @param phd	[in] NMHEADER
 * @return TRUE if handled; FALSE if not.
 */
inline BOOL RP_ShellPropSheetExt_Private::Header_DividerDblClick(const NMHEADER *phd)
{
	// Button should always be 0. (left button)
	if (phd->iButton != 0) {
		// Not the left button.
		return FALSE;
	}

	// NOTE: We need to get the dialog ID of the ListView, not the Header.
	HWND hHeader = phd->hdr.hwndFrom;
	HWND hListView = GetParent(hHeader);
	assert(hListView != nullptr);
	if (!hListView)
		return FALSE;
	const unsigned int idFrom = static_cast<unsigned int>(GetDlgCtrlID(hListView));

	// Get the ListView data.
	auto iter_lvData = map_lvData.find(idFrom);
	if (iter_lvData == map_lvData.end()) {
		// ListView data not found...
		return FALSE;
	}
	const LvData &lvData = iter_lvData->second;

	// Adjust the specified column.
	assert(phd->iItem < static_cast<int>(lvData.col_widths.size()));
	if (phd->iItem < static_cast<int>(lvData.col_widths.size())) {
		ListView_SetColumnWidth(hListView, phd->iItem, lvData.col_widths[phd->iItem]);
		return TRUE;
	}

	// Not handled...
	return FALSE;
}

/**
 * ListView CustomDraw function
 * @param plvcd	[in/out] NMLVCUSTOMDRAW
 * @return Return value.
 */
inline int RP_ShellPropSheetExt_Private::ListView_CustomDraw(NMLVCUSTOMDRAW *plvcd) const
{
	int result = CDRF_DODEFAULT;
	switch (plvcd->nmcd.dwDrawStage) {
		case CDDS_PREPAINT:
			// Request notifications for individual ListView items.
			result = CDRF_NOTIFYITEMDRAW;
			break;

		case CDDS_ITEMPREPAINT: {
			// Set the background color for alternating row colors.
			if (plvcd->nmcd.dwItemSpec % 2) {
				// NOTE: plvcd->clrTextBk is set to 0xFF000000 here,
				// not the actual default background color.
				// FIXME: On Windows 7:
				// - Standard row colors are 19px high.
				// - Alternate row colors are 17px high. (top and bottom lines ignored?)
				// TODO: Cache the alternate row color in each ListView.
				plvcd->clrTextBk = LibWin32UI::ListView_GetBkColor_AltRow(plvcd->nmcd.hdr.hwndFrom);
				result = CDRF_NEWFONT;
			}
			break;
		}

		default:
			break;
	}
	return result;
}

/**
 * WM_NOTIFY handler for the property sheet.
 * @param hDlg Dialog window.
 * @param pHdr NMHDR
 * @return Return value.
 */
INT_PTR RP_ShellPropSheetExt_Private::DlgProc_WM_NOTIFY(HWND hDlg, NMHDR *pHdr)
{
	INT_PTR ret = FALSE;

	switch (pHdr->code) {
		case PSN_SETACTIVE:
			if (lblIcon) {
				lblIcon->startAnimTimer();
			}
			if (hBtnOptions) {
				ShowWindow(hBtnOptions, SW_SHOW);
			}
			break;

		case PSN_KILLACTIVE:
			if (lblIcon) {
				lblIcon->stopAnimTimer();
			}
			if (hBtnOptions) {
				ShowWindow(hBtnOptions, SW_HIDE);
			}
			break;

		case NM_CLICK:
		case NM_RETURN: {
			// Check if this is a SysLink control.
			// NOTE: SysLink control only supports Unicode.
			// NOTE: Linear search...
			const HWND hwndFrom = pHdr->hwndFrom;
			const bool isSysLink = std::any_of(tabs.cbegin(), tabs.cend(),
				[hwndFrom](const tab& tab) noexcept -> bool {
					return (tab.lblCredits == hwndFrom);
				});
			if (isSysLink) {
				// It's a SysLink control.
				// Open the URL.
				PNMLINK pNMLink = reinterpret_cast<PNMLINK>(pHdr);
				ShellExecute(nullptr, _T("open"), pNMLink->item.szUrl, nullptr, nullptr, SW_SHOW);
			}
			break;
		}

		case TCN_SELCHANGING: {
			if (!tabWidget || tabWidget != pHdr->hwndFrom) {
				break;
			}

			// Selected tab is changing. Hide the currently-selected tab.
			const int tabIndex = TabCtrl_GetCurSel(tabWidget);
			assert(tabIndex >= 0);
			assert(tabIndex < static_cast<int>(tabs.size()));
			if (tabIndex >= 0 && tabIndex < static_cast<int>(tabs.size())) {
				ShowWindow(tabs[tabIndex].hDlg, SW_HIDE);
			}
			break;
		}

		case TCN_SELCHANGE: {
			if (!tabWidget || tabWidget != pHdr->hwndFrom) {
				break;
			}

			// Selected tab has changed. Show the newly-selected tab.
			const int tabIndex = TabCtrl_GetCurSel(tabWidget);
			assert(tabIndex >= 0);
			assert(tabIndex < static_cast<int>(tabs.size()));
			if (tabIndex >= 0 && tabIndex < static_cast<int>(tabs.size())) {
				ShowWindow(tabs[tabIndex].hDlg, SW_SHOW);
			}
			break;
		}

		case LVN_GETDISPINFO: {
			// Get data for an LVS_OWNERDATA ListView.
			if ((pHdr->idFrom & 0xFC00) != IDC_RFT_LISTDATA(0))
				break;

			ret = ListView_GetDispInfo(reinterpret_cast<NMLVDISPINFO*>(pHdr));
			break;
		}

		case LVN_COLUMNCLICK: {
			// Column header was clicked.
			if ((pHdr->idFrom & 0xFC00) != IDC_RFT_LISTDATA(0))
				break;

			ret = ListView_ColumnClick(reinterpret_cast<const NMLISTVIEW*>(pHdr));
			break;
		}

		case HDN_DIVIDERDBLCLICK: {
			// Header divider was double-clicked.
			// NOTE: We can't check idFrom here because this is the
			// Header control sending the notification, not the ListView.
			ret = Header_DividerDblClick(reinterpret_cast<const NMHEADER*>(pHdr));
			break;
		}

		case NM_CUSTOMDRAW: {
			// Custom drawing notification.
			if ((pHdr->idFrom & 0xFC00) != IDC_RFT_LISTDATA(0))
				break;

			// NOTE: Since this is a DlgProc, we can't simply return
			// the CDRF code. It has to be set as DWLP_MSGRESULT.
			// References:
			// - https://stackoverflow.com/questions/40549962/c-winapi-listview-nm-customdraw-not-getting-cdds-itemprepaint
			// - https://stackoverflow.com/a/40552426
			const int result = ListView_CustomDraw(reinterpret_cast<NMLVCUSTOMDRAW*>(pHdr));
			SetWindowLongPtr(hDlg, DWLP_MSGRESULT, result);
			ret = true;
			break;
		}

		case LVN_ITEMCHANGING: {
			// If the window is fully initialized,
			// disable modification of checkboxes.
			// Reference: https://groups.google.com/forum/embed/#!topic/microsoft.public.vc.mfc/e9cbkSsiImA
			if (!isFullyInit)
				break;
			if ((pHdr->idFrom & 0xFC00) != IDC_RFT_LISTDATA(0))
				break;

			const NMLISTVIEW *const pnmlv = reinterpret_cast<const NMLISTVIEW*>(pHdr);
			const unsigned int state = (pnmlv->uOldState ^ pnmlv->uNewState) & LVIS_STATEIMAGEMASK;
			// Set result to true if the state difference is non-zero (i.e. it's changed).
			SetWindowLongPtr(hDlg, DWLP_MSGRESULT, (state != 0));
			ret = true;
			break;
		}

		case MSGWN_CLOSED: {
			// MessageWidget Close button was clicked.
			adjustTabsForMessageWidgetVisibility(false);
			ret = true;
			break;
		}

		default:
			break;
	}

	return ret;
}

/**
 * WM_COMMAND handler for the property sheet.
 * @param hDlg Dialog window.
 * @param wParam
 * @param lParam
 * @return Return value.
 */
INT_PTR RP_ShellPropSheetExt_Private::DlgProc_WM_COMMAND(HWND hDlg, WPARAM wParam, LPARAM lParam)
{
	((void)hDlg);
	((void)lParam);
	INT_PTR ret = false;

	switch (HIWORD(wParam)) {
		case CBN_SELCHANGE: {
			// The user may be changing the selected language
			// for RFT_STRING_MULTI.
			if (LOWORD(wParam) != IDC_CBO_LANGUAGE)
				break;

			// Get the LC.
			// TODO: Custom WM_NOTIFY message with the LC?
			const uint32_t lc = LanguageComboBox_GetSelectedLC(cboLanguage);
			if (lc != 0) {
				updateMulti(lc);
			}
			break;
		}

		default:
			break;
	}

	return ret;
}

/**
 * WM_PAINT handler for the property sheet.
 * @param hDlg Dialog window.
 * @return Return value.
 */
INT_PTR RP_ShellPropSheetExt_Private::DlgProc_WM_PAINT(HWND hDlg)
{
	if (!lblBanner && !lblIcon) {
		// Nothing to draw...
		return false;
	}

	PAINTSTRUCT ps;
	HDC hdc = BeginPaint(hDlg, &ps);

	// TODO: Share the memory DC between lblBanner and lblIcon?

	// Draw the banner.
	if (lblBanner && lblBanner->intersects(&ps.rcPaint)) {
		lblBanner->draw(hdc);
	}

	// Draw the icon.
	if (lblIcon && lblIcon->intersects(&ps.rcPaint)) {
		lblIcon->draw(hdc);
	}

	EndPaint(hDlg, &ps);
	return true;
}

//
//   FUNCTION: FilePropPageDlgProc
//
//   PURPOSE: Processes messages for the property page.
//
INT_PTR CALLBACK RP_ShellPropSheetExt_Private::DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	// Based on CppShellExtPropSheetHandler.
	// https://code.msdn.microsoft.com/windowsapps/CppShellExtPropSheetHandler-d93b49b7
	switch (uMsg) {
		case WM_INITDIALOG: {
			// Get the pointer to the property sheet page object. This is 
			// contained in the LPARAM of the PROPSHEETPAGE structure.
			LPPROPSHEETPAGE pPage = reinterpret_cast<LPPROPSHEETPAGE>(lParam);
			if (!pPage)
				return true;

			// Access the property sheet extension from property page.
			RP_ShellPropSheetExt *const pExt = reinterpret_cast<RP_ShellPropSheetExt*>(pPage->lParam);
			if (!pExt)
				return true;
			RP_ShellPropSheetExt_Private *const d = pExt->d_ptr;

			// Store the D object pointer with this particular page dialog.
			SetWindowLongPtr(hDlg, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(d));
			// Save handles for later.
			d->hDlgSheet = hDlg;

			// Dialog initialization is postponed to WM_SHOWWINDOW,
			// since some other extension (e.g. HashTab) may be
			// resizing the dialog.

			// NOTE: We're using WM_SHOWWINDOW instead of WM_SIZE
			// because WM_SIZE isn't sent for block devices,
			// e.g. CD-ROM drives.
			return true;
		}

		// FIXME: FBI's age rating is cut off on Windows
		// if we don't adjust for WM_SHOWWINDOW.
		case WM_SHOWWINDOW: {
			auto *const d = reinterpret_cast<RP_ShellPropSheetExt_Private*>(GetWindowLongPtr(hDlg, GWLP_USERDATA));
			if (!d) {
				// No RP_ShellPropSheetExt_Private. Can't do anything...
				return false;
			}

			if (d->isFullyInit) {
				// Dialog is already initialized.
				break;
			}

			// Get the appropriate RomData class for this ROM.
			d->romData = RomDataFactory::create(d->tfilename);
			if (!d->romData) {
				// Unable to get a RomData object.
				break;
			}

			// Load the images.
			d->loadImages();
			// Initialize the dialog.
			d->initDialog();
			// We can close the RomData's underlying IRpFile now.
			d->romData->close();

			// Create the "Options" button in the parent window.
			d->createOptionsButton();

			// Start the icon animation timer.
			if (d->lblIcon) {
				d->lblIcon->startAnimTimer();
			}

			// Continue normal processing.
			break;
		}

		case WM_DESTROY: {
			auto *const d = reinterpret_cast<RP_ShellPropSheetExt_Private*>(GetWindowLongPtr(hDlg, GWLP_USERDATA));
			if (d && d->lblIcon) {
				// Stop the animation timer.
				d->lblIcon->stopAnimTimer();
			}
			return true;
		}

		case WM_NOTIFY: {
			auto *const d = reinterpret_cast<RP_ShellPropSheetExt_Private*>(GetWindowLongPtr(hDlg, GWLP_USERDATA));
			if (!d) {
				// No RP_ShellPropSheetExt_Private. Can't do anything...
				return false;
			}

			return d->DlgProc_WM_NOTIFY(hDlg, reinterpret_cast<NMHDR*>(lParam));
		}

		case WM_COMMAND: {
			auto *const d = reinterpret_cast<RP_ShellPropSheetExt_Private*>(GetWindowLongPtr(hDlg, GWLP_USERDATA));
			if (!d) {
				// No RP_ShellPropSheetExt_Private. Can't do anything...
				return false;
			}

			return d->DlgProc_WM_COMMAND(hDlg, wParam, lParam);
		}

		case WM_PAINT: {
			auto *const d = reinterpret_cast<RP_ShellPropSheetExt_Private*>(GetWindowLongPtr(hDlg, GWLP_USERDATA));
			if (!d) {
				// No RP_ShellPropSheetExt_Private. Can't do anything...
				return false;
			}
			return d->DlgProc_WM_PAINT(hDlg);
		}

		case WM_SYSCOLORCHANGE:
		case WM_THEMECHANGED: {
			auto* const d = reinterpret_cast<RP_ShellPropSheetExt_Private*>(GetWindowLongPtr(hDlg, GWLP_USERDATA));
			if (!d) {
				// No RP_ShellPropSheetExt_Private. Can't do anything...
				return false;
			}

			UpdateDarkModeEnabled();
			d->isDarkModeEnabled = VerifyDialogDarkMode(GetParent(hDlg));
			// TODO: Force a window update?

			// Reload the images.
			// Assuming the main background color may have changed.

			// Reload images with the new background color.
			d->loadImages();

			// Invalidate the banner and icon rectangles.
			if (d->lblBanner) {
				d->lblBanner->invalidateRect();
			}
			if (d->lblIcon) {
				d->lblIcon->invalidateRect();
			}

			// TODO: Check for RFT_LISTDATA with icons and reinitialize
			// the icons if the background color changed.
			// Alternatively, maybe store them as ARGB32 bitmaps?
			// That method works for ComboBoxEx...

			// Update the fonts.
			d->fontHandler.updateFonts(true);
			break;
		}

		case WM_NCPAINT: {
			// Update the monospaced font.
			// NOTE: This should be WM_SETTINGCHANGE with
			// SPI_GETFONTSMOOTHING or SPI_GETFONTSMOOTHINGTYPE,
			// but that message isn't sent when previewing changes
			// for ClearType. (It's sent when applying the changes.)
			auto *const d = reinterpret_cast<RP_ShellPropSheetExt_Private*>(GetWindowLongPtr(hDlg, GWLP_USERDATA));
			if (d) {
				// Update the fonts.
				d->fontHandler.updateFonts();
			}
			break;
		}

		case WM_WTSSESSION_CHANGE: {
			auto *const d = reinterpret_cast<RP_ShellPropSheetExt_Private*>(GetWindowLongPtr(hDlg, GWLP_USERDATA));
			if (!d) {
				// No RP_ShellPropSheetExt_Private. Can't do anything...
				return false;
			}

			// If RDP was connected, disable ListView double-buffering.
			// If console (or RemoteFX) was connected, enable ListView double-buffering.
			switch (wParam) {
				case WTS_CONSOLE_CONNECT:
					for (HWND hWnd : d->hwndListViewControls) {
						DWORD dwExStyle = ListView_GetExtendedListViewStyle(hWnd);
						dwExStyle |= LVS_EX_DOUBLEBUFFER;
						ListView_SetExtendedListViewStyle(hWnd, dwExStyle);
					}
					break;
				case WTS_REMOTE_CONNECT:
					for (HWND hWnd : d->hwndListViewControls) {
						DWORD dwExStyle = ListView_GetExtendedListViewStyle(hWnd);
						dwExStyle &= ~LVS_EX_DOUBLEBUFFER;
						ListView_SetExtendedListViewStyle(hWnd, dwExStyle);
					}
					break;
				default:
					break;
			}
			break;
		}

		case WM_MOUSEWHEEL: {
			auto *const d = reinterpret_cast<RP_ShellPropSheetExt_Private*>(GetWindowLongPtr(hDlg, GWLP_USERDATA));
			if (!d) {
				// No RP_ShellPropSheetExt_Private. Can't do anything...
				return FALSE;
			}

			// Forward the message to the active child dialog.
			const int tabIndex = TabCtrl_GetCurSel(d->tabWidget);
			assert(tabIndex >= 0);
			assert(tabIndex < static_cast<int>(d->tabs.size()));
			if (tabIndex >= 0 && tabIndex < static_cast<int>(d->tabs.size())) {
				SendMessage(d->tabs[tabIndex].hDlg, uMsg, wParam, lParam);
			}
			return TRUE;
		}

		case WM_RBUTTONUP: {
			auto *const d = reinterpret_cast<RP_ShellPropSheetExt_Private*>(GetWindowLongPtr(hDlg, GWLP_USERDATA));
			if (!d) {
				// No RP_ShellPropSheetExt_Private. Can't do anything...
				return FALSE;
			}

			// Allow lblIcon to process this in case it's in lblIcon's rectangle.
			// TODO: Make DragImageLabel a real control?
			if (d->lblIcon) {
				d->lblIcon->tryPopupEcksBawks(lParam);
				return TRUE;
			}
			break;
		}

		case WM_CTLCOLORMSGBOX:
		case WM_CTLCOLOREDIT:
		case WM_CTLCOLORLISTBOX:
		case WM_CTLCOLORBTN:
		case WM_CTLCOLORDLG:
		case WM_CTLCOLORSCROLLBAR:
		case WM_CTLCOLORSTATIC: {
			// If using Dark Mode, forward WM_CTLCOLOR* to the parent window.
			// This fixes issues when using StartAllBack on Windows 11
			// to enforce Dark Mode schemes in Windows Explorer.
			// TODO: Handle color scheme changes?
			auto* const d = reinterpret_cast<RP_ShellPropSheetExt_Private*>(GetWindowLongPtr(hDlg, GWLP_USERDATA));
			if (d && d->isDarkModeEnabled) {
				return SendMessage(GetParent(hDlg), uMsg, wParam, lParam);
			}
			break;
		}

		case WM_SETTINGCHANGE:
			if (g_darkModeSupported && IsColorSchemeChangeMessage(lParam)) {
				SendMessage(hDlg, WM_THEMECHANGED, 0, 0);
			}
			break;

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
UINT CALLBACK RP_ShellPropSheetExt_Private::CallbackProc(HWND hWnd, UINT uMsg, LPPROPSHEETPAGE ppsp)
{
	((void)hWnd);	// TODO: Validate this?

	switch (uMsg) {
		case PSPCB_CREATE: {
			// Must return true to enable the page to be created.
			return true;
		}

		case PSPCB_RELEASE: {
			// When the callback function receives the PSPCB_RELEASE notification, 
			// the ppsp parameter of the PropSheetPageProc contains a pointer to 
			// the PROPSHEETPAGE structure. The lParam member of the PROPSHEETPAGE 
			// structure contains the extension pointer which can be used to 
			// release the object.

			// Release the property sheet extension object. This is called even 
			// if the property page was never actually displayed.
			auto *const pExt = reinterpret_cast<RP_ShellPropSheetExt*>(ppsp->lParam);
			if (pExt) {
				pExt->Release();
			}
			break;
		}

		default:
			break;
	}

	return false;
}

/**
 * Dialog procedure for subtabs.
 * @param hDlg
 * @param uMsg
 * @param wParam
 * @param lParam
 */
INT_PTR CALLBACK RP_ShellPropSheetExt_Private::SubtabDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
		case WM_DESTROY: {
			// Remove the TAB_PTR_PROP property from the page.
			// The TAB_PTR_PROP property stored the pointer to the
			// RP_ShellPropSheetExt_Private::tab object.
			RemoveProp(hDlg, RP_ShellPropSheetExtPrivate::TAB_PTR_PROP);
			break;
		}

		case WM_NOTIFY: {
			// Propagate NM_CUSTOMDRAW to the parent dialog.
			const NMHDR *const pHdr = reinterpret_cast<const NMHDR*>(lParam);
			switch (pHdr->code) {
				case LVN_GETDISPINFO:
				case LVN_COLUMNCLICK:
				case HDN_DIVIDERDBLCLICK:
				case NM_CUSTOMDRAW:
				case LVN_ITEMCHANGING: {
					// NOTE: Since this is a DlgProc, we can't simply return
					// the CDRF code. It has to be set as DWLP_MSGRESULT.
					// References:
					// - https://stackoverflow.com/questions/40549962/c-winapi-listview-nm-customdraw-not-getting-cdds-itemprepaint
					// - https://stackoverflow.com/a/40552426
					INT_PTR result = SendMessage(GetParent(hDlg), uMsg, wParam, lParam);
					SetWindowLongPtr(hDlg, DWLP_MSGRESULT, result);
					return TRUE;
				}

				default:
					break;
			}
			break;
		}

		case WM_CTLCOLORMSGBOX:
		case WM_CTLCOLOREDIT:
		case WM_CTLCOLORLISTBOX:
		case WM_CTLCOLORBTN:
		case WM_CTLCOLORDLG:
		case WM_CTLCOLORSCROLLBAR: {
			// If using Dark Mode, forward WM_CTLCOLOR* to the parent window.
			// This fixes issues when using StartAllBack on Windows 11
			// to enforce Dark Mode schemes in Windows Explorer.
			// TODO: Handle color scheme changes?
			auto* const d = reinterpret_cast<RP_ShellPropSheetExt_Private*>(GetWindowLongPtr(hDlg, GWLP_USERDATA));
			if (d && d->isDarkModeEnabled) {
				return SendMessage(GetParent(hDlg), uMsg, wParam, lParam);
			}
			break;
		}

		case WM_CTLCOLORSTATIC: {
			const COLORREF color = static_cast<COLORREF>(GetWindowLongPtr(
				reinterpret_cast<HWND>(lParam), GWLP_USERDATA));
			if (color != 0) {
				// Set the specified color.
				HDC hdc = reinterpret_cast<HDC>(wParam);
				SetTextColor(hdc, color);
			} else {
				// No custom color. Forward the message to the parent window.
				// This fixes issues when using StartAllBack on Windows 11
				// to enforce Dark Mode schemes in Windows Explorer.
				// TODO: Handle color scheme changes?
				auto* const d = reinterpret_cast<RP_ShellPropSheetExt_Private*>(GetWindowLongPtr(hDlg, GWLP_USERDATA));
				if (d && d->isDarkModeEnabled) {
					return SendMessage(GetParent(hDlg), uMsg, wParam, lParam);
				}
			}
			break;
		}

		case WM_VSCROLL: {
			auto *const d = reinterpret_cast<RP_ShellPropSheetExt_Private*>(GetWindowLongPtr(hDlg, GWLP_USERDATA));
			auto *const tab = static_cast<RP_ShellPropSheetExt_Private::tab*>(GetProp(hDlg, TAB_PTR_PROP));
			if (!d || !tab) {
				// No RP_ShellPropSheetExt_Private or tab. Can't do anything...
				break;
			}

			// Check the operation and scroll there.
			int deltaY = 0;
			switch (LOWORD(wParam)) {
				case SB_TOP:
					deltaY = -tab->scrollPos;
					break;
				case SB_BOTTOM:
					deltaY = tab->curPt.y - tab->scrollPos;
					break;

				case SB_LINEUP:
					deltaY = -d->lblDescHeight;
					break;
				case SB_LINEDOWN:
					deltaY = d->lblDescHeight;
					break;

				case SB_PAGEUP:
					deltaY = -d->dlgSize.cy;
					break;
				case SB_PAGEDOWN:
					deltaY = d->dlgSize.cy;
					break;

				case SB_THUMBTRACK:
				case SB_THUMBPOSITION:
					deltaY = HIWORD(wParam) - tab->scrollPos;
					break;
			}

			// Make sure this doesn't go out of range.
			const int scrollPos = tab->scrollPos + deltaY;
			if (scrollPos < 0) {
				deltaY -= scrollPos;
			} else if (scrollPos + d->dlgSize.cy > tab->curPt.y) {
				deltaY -= (scrollPos + 1) - (tab->curPt.y - d->dlgSize.cy);
			}

			tab->scrollPos += deltaY;
			SetScrollPos(hDlg, SB_VERT, tab->scrollPos, TRUE);
			ScrollWindow(hDlg, 0, -deltaY, nullptr, nullptr);
			return TRUE;
		}

		case WM_MOUSEWHEEL: {
			// NOTE: All code paths return TRUE to prevent a stack overflow.
			if (LOWORD(wParam) != 0) {
				// Some hotkey or mouse button is held down.
				// Don't scroll the scrollbar.
				return TRUE;
			}

			const int16_t wheel = HIWORD(wParam);
			if (wheel == 0) {
				// No movement...
				return TRUE;
			}

			auto *const d = reinterpret_cast<RP_ShellPropSheetExt_Private*>(GetWindowLongPtr(hDlg, GWLP_USERDATA));
			auto *const tab = static_cast<RP_ShellPropSheetExt_Private::tab*>(GetProp(hDlg, TAB_PTR_PROP));
			if (!d || !tab) {
				// No RP_ShellPropSheetExt_Private or tab. Can't do anything...
				return TRUE;
			}

			if (tab->curPt.y <= d->dlgSize.cy) {
				// Tab height is less than the dialog size.
				// No scrolling.
				return TRUE;
			}

			// TODO: Handle multiples of WHEEL_DELTA?
			int deltaY = d->lblDescHeight;
			if (wheel > 0) {
				deltaY = -deltaY;
			}

			// Make sure this doesn't go out of range.
			const int scrollPos = tab->scrollPos + deltaY;
			if (scrollPos < 0) {
				deltaY -= scrollPos;
			} else if (scrollPos + d->dlgSize.cy > tab->curPt.y) {
				deltaY -= (scrollPos + 1) - (tab->curPt.y - d->dlgSize.cy);
			}

			tab->scrollPos += deltaY;
			SetScrollPos(hDlg, SB_VERT, tab->scrollPos, TRUE);
			ScrollWindow(hDlg, 0, -deltaY, nullptr, nullptr);
			return TRUE;
		}
	}

	// Nothing to do here...
	return FALSE;
}
