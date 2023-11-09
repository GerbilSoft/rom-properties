/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * AchievementsTab.cpp: Achievements tab for rp-config.                    *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "AchievementsTab.hpp"
#include "RomDataFormat.hpp"
#include "RpImageWin32.hpp"
#include "res/resource.h"
#include "../AchSpriteSheet.hpp"

// Other rom-properties libraries
#include "librpbase/Achievements.hpp"
#include "librpbase/RomFields.hpp"
#include "librpbase/img/RpPng.hpp"
using namespace LibRpBase;

// libwin32ui
#include "libwin32ui/AutoGetDC.hpp"
#include "libwin32ui/LoadResource_i18n.hpp"
using LibWin32UI::AutoGetDC_font;
using LibWin32UI::LoadDialog_i18n;

// Win32 dark mode
#include "libwin32darkmode/DarkMode.hpp"
#include "libwin32darkmode/DarkModeCtrl.hpp"
#include "libwin32darkmode/ListViewUtil.hpp"

// C++ STL classes
using std::tstring;
using std::unique_ptr;

class AchievementsTabPrivate
{
public:
	AchievementsTabPrivate();
	~AchievementsTabPrivate();

private:
	RP_DISABLE_COPY(AchievementsTabPrivate)

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

	// Image list for achievement icons.
	HIMAGELIST himglAch;

	// Alternate row color.
	COLORREF colorAltRow;

public:
	/**
	 * Update the ListView style.
	 */
	void updateListViewStyle(void);

	/**
	 * Update the ListView ImageList.
	 */
	void updateImageList(void);

	/**
	 * ListView CustomDraw function.
	 * @param plvcd	[in/out] NMLVCUSTOMDRAW
	 * @return Return value.
	 */
	int ListView_CustomDraw(NMLVCUSTOMDRAW *plvcd) const;

	/**
	 * Reset the configuration.
	 */
	void reset(void);

public:
	// Dark Mode background brush
	HBRUSH hbrBkgnd;
	bool lastDarkModeEnabled;
};

/** AchievementsTabPrivate **/

AchievementsTabPrivate::AchievementsTabPrivate()
	: hPropSheetPage(nullptr)
	, hWndPropSheet(nullptr)
	, himglAch(nullptr)
	, colorAltRow(0)
	, hbrBkgnd(nullptr)
	, lastDarkModeEnabled(false)
{}

AchievementsTabPrivate::~AchievementsTabPrivate()
{
	if (himglAch) {
		ImageList_Destroy(himglAch);
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
INT_PTR CALLBACK AchievementsTabPrivate::dlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	RP_UNUSED(wParam);

	switch (uMsg) {
		case WM_INITDIALOG: {
			// Get the pointer to the property sheet page object. This is 
			// contained in the LPARAM of the PROPSHEETPAGE structure.
			LPPROPSHEETPAGE pPage = reinterpret_cast<LPPROPSHEETPAGE>(lParam);
			if (!pPage)
				return TRUE;

			// Get the pointer to the AchievementsTabPrivate object.
			AchievementsTabPrivate *const d = reinterpret_cast<AchievementsTabPrivate*>(pPage->lParam);
			if (!d)
				return TRUE;

			assert(d->hWndPropSheet == nullptr);
			d->hWndPropSheet = hDlg;

			// Store the D object pointer with this particular page dialog.
			SetWindowLongPtr(hDlg, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(d));

			//  NOTE: This should be in WM_CREATE, but we don't receive WM_CREATE here.
			DarkMode_InitDialog(hDlg);
			d->lastDarkModeEnabled = g_darkModeEnabled;

			// Set window themes for Win10's dark mode.
			if (g_darkModeSupported) {
				// Initialize Dark Mode in the ListView.
				HWND hListView = GetDlgItem(hDlg, IDC_ACHIEVEMENTS_LIST);
				assert(hListView != nullptr);
				DarkMode_InitListView(hListView);
			}

			// Reset the configuration.
			d->reset();
			return TRUE;
		}

		case WM_NOTIFY: {
			auto *const d = reinterpret_cast<AchievementsTabPrivate*>(GetWindowLongPtr(hDlg, GWLP_USERDATA));
			if (!d) {
				// No AchievementsTabPrivate. Can't do anything...
				return false;
			}

			NMHDR *const pHdr = reinterpret_cast<NMHDR*>(lParam);
			if (pHdr->code == NM_CUSTOMDRAW && pHdr->idFrom == IDC_ACHIEVEMENTS_LIST) {
				// NOTE: Since this is a DlgProc, we can't simply return
				// the CDRF code. It has to be set as DWLP_MSGRESULT.
				// References:
				// - https://stackoverflow.com/questions/40549962/c-winapi-listview-nm-customdraw-not-getting-cdds-itemprepaint
				// - https://stackoverflow.com/a/40552426
				const int result = d->ListView_CustomDraw(reinterpret_cast<NMLVCUSTOMDRAW*>(pHdr));
				SetWindowLongPtr(hDlg, DWLP_MSGRESULT, result);
				return TRUE;
			}

			break;
		}

		case WM_SYSCOLORCHANGE: {
			auto *const d = reinterpret_cast<AchievementsTabPrivate*>(GetWindowLongPtr(hDlg, GWLP_USERDATA));
			if (!d) {
				// No AchievementsTabPrivate. Can't do anything...
				return false;
			}

			// Update the ListView style.
			d->updateListViewStyle();
			break;
		}

		/** Dark Mode **/

		case WM_CTLCOLORDLG:
		case WM_CTLCOLORSTATIC:
			if (g_darkModeSupported && g_darkModeEnabled) {
				auto *const d = reinterpret_cast<AchievementsTabPrivate*>(GetWindowLongPtr(hDlg, GWLP_USERDATA));
				if (!d) {
					// No AchievementsTabPrivate. Can't do anything...
					return FALSE;
				}

				HDC hdc = reinterpret_cast<HDC>(wParam);
				SetTextColor(hdc, g_darkTextColor);
				SetBkColor(hdc, g_darkBkColor);
				if (!d->hbrBkgnd) {
					d->hbrBkgnd = CreateSolidBrush(g_darkBkColor);
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
			auto *const d = reinterpret_cast<AchievementsTabPrivate*>(GetWindowLongPtr(hDlg, GWLP_USERDATA));
			if (!d) {
				// No AchievementsTabPrivate. Can't do anything...
				return FALSE;
			}

			if (g_darkModeSupported) {
				UpdateDarkModeEnabled();
				if (d->lastDarkModeEnabled != g_darkModeEnabled) {
					d->lastDarkModeEnabled = g_darkModeEnabled;
					InvalidateRect(hDlg, NULL, true);

					// Propagate WM_THEMECHANGED to window controls that don't
					// automatically handle Dark Mode changes, e.g. ComboBox and Button.
					SendMessage(GetDlgItem(hDlg, IDC_ACHIEVEMENTS_LIST), WM_THEMECHANGED, 0, 0);
				}
			}

			// Update the ListView style.
			d->updateListViewStyle();
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
UINT CALLBACK AchievementsTabPrivate::callbackProc(HWND hWnd, UINT uMsg, LPPROPSHEETPAGE ppsp)
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
 * Update the ListView style.
 */
void AchievementsTabPrivate::updateListViewStyle(void)
{
	HWND hListView = GetDlgItem(hWndPropSheet, IDC_ACHIEVEMENTS_LIST);
	assert(hListView != nullptr);
	if (!hListView)
		return;

	// Set extended ListView styles.
	// Double-buffering is enabled if using RDP or RemoteFX.
	const DWORD lvsExStyle = likely(!GetSystemMetrics(SM_REMOTESESSION))
		? LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER
		: LVS_EX_FULLROWSELECT;
	ListView_SetExtendedListViewStyle(hListView, lvsExStyle);

	// If the alt row color changed, redo the ImageList.
	const COLORREF colorAltRow = LibWin32UI::ListView_GetBkColor_AltRow(hListView);
	if (colorAltRow != this->colorAltRow) {
		this->colorAltRow = colorAltRow;
		updateImageList();
	}
}

/**
 * Update the ListView ImageList.
 */
void AchievementsTabPrivate::updateImageList(void)
{
	// Remove the current ImageList from the ListView.
	HWND hListView = GetDlgItem(hWndPropSheet, IDC_ACHIEVEMENTS_LIST);
	assert(hListView != nullptr);
	if (!hListView)
		return;
	ListView_SetImageList(hListView, nullptr, LVSIL_SMALL);

	if (!himglAch) {
		// Delete the existing ImageList.
		ImageList_Destroy(himglAch);
		himglAch = nullptr;
	}

	// Get the icon size for the current DPI.
	// Reference: https://docs.microsoft.com/en-us/windows/win32/hidpi/high-dpi-desktop-application-development-on-windows
	// TODO: Handle WM_DPICHANGED.
	const UINT dpi = rp_GetDpiForWindow(hWndPropSheet);
	unsigned int iconSize;
	if (dpi < 144) {
		// [96,144) dpi: Use 32x32.
		iconSize = 32;
	} else {
		// >144dpi: Use 64x64.
		iconSize = 64;
	}

	// Create the image list.
	himglAch = ImageList_Create(iconSize, iconSize, ILC_COLOR32,
		(int)Achievements::ID::Max, (int)Achievements::ID::Max);
	assert(himglAch != nullptr);
	if (!himglAch)
		return;

	// Load the achievements sprite sheet.
	AchSpriteSheet achSpriteSheet(iconSize);

	// Add icons.
	const Achievements *const pAch = Achievements::instance();
	for (int i = 0; i < (int)Achievements::ID::Max; i++) {
		const Achievements::ID id = (Achievements::ID)i;
		const time_t timestamp = pAch->isUnlocked(id);
		const bool unlocked = (timestamp != -1);

		// Get the achievement icon.
		HBITMAP hbmIcon = achSpriteSheet.getIcon(id, !unlocked, dpi);
		assert(hbmIcon != nullptr);

		// FIXME: Handle highlighting of alpha-transparent areas correctly.
		bool bIconWasAdded = false;
		if (hbmIcon) {
			HICON hIcon = RpImageWin32::toHICON(hbmIcon);
			assert(hIcon != nullptr);
			if (hIcon) {
				ImageList_AddIcon(himglAch, hIcon);
				DestroyIcon(hIcon);
				bIconWasAdded = true;
			}
			DeleteBitmap(hbmIcon);
		}
		if (!bIconWasAdded) {
			// Add an empty icon.
			const size_t icon_byte_count = iconSize * iconSize * sizeof(uint32_t);
			unique_ptr<uint8_t[]> iconData(new uint8_t[icon_byte_count * 2]);
			memset(&iconData[0], 0xFF, icon_byte_count);
			memset(&iconData[icon_byte_count], 0xFF, icon_byte_count);
			HICON hIcon = CreateIcon(HINST_THISCOMPONENT, iconSize, iconSize, 1, 1,
				&iconData[0], &iconData[icon_byte_count]);
			assert(hIcon != nullptr);
			ImageList_AddIcon(himglAch, hIcon);
			DestroyIcon(hIcon);
		}
	}

	// NOTE: ListView uses LVSIL_SMALL for LVS_REPORT.
	// TODO: The row highlight doesn't surround the empty area
	// of the icon. LVS_OWNERDRAW is probably needed for that.
	ListView_SetImageList(hListView, himglAch, LVSIL_SMALL);
}

/**
 * ListView CustomDraw function.
 * @param plvcd	[in/out] NMLVCUSTOMDRAW
 * @return Return value.
 */
int AchievementsTabPrivate::ListView_CustomDraw(NMLVCUSTOMDRAW *plvcd) const
{
	int result = CDRF_DODEFAULT;
	switch (plvcd->nmcd.dwDrawStage) {
		case CDDS_PREPAINT:
			// Request notifications for individual ListView items.
			result = CDRF_NOTIFYITEMDRAW;
			break;

		case CDDS_ITEMPREPAINT:
			// Set the background color for alternating row colors.
			if (plvcd->nmcd.dwItemSpec % 2 != 0) {
				// NOTE: plvcd->clrTextBk is set to 0xFF000000 here,
				// not the actual default background color.
				// FIXME: On Windows 7:
				// - Standard row colors are 19px high.
				// - Alternate row colors are 17px high. (top and bottom lines ignored?)
				plvcd->clrTextBk = colorAltRow;
				result = CDRF_NEWFONT;
			}
			break;

		default:
			break;
	}
	return result;
}

/**
 * Reset the configuration.
 */
void AchievementsTabPrivate::reset(void)
{
	// Load achievements.
	HWND hListView = GetDlgItem(hWndPropSheet, IDC_ACHIEVEMENTS_LIST);
	assert(hListView != nullptr);
	if (!hListView)
		return;

	// Clear the ListView.
	ListView_DeleteAllItems(hListView);
	if (himglAch) {
		ListView_SetImageList(hListView, nullptr, LVSIL_SMALL);
	}

	// Check if we need to set up columns.
	int colCount = 0;
	HWND hHeader = ListView_GetHeader(hListView);
	assert(hHeader != nullptr);
	if (hHeader) {
		colCount = Header_GetItemCount(hHeader);
	}

	if (colCount == 0) {
		// Add the columns.
		// TODO: Add an ID column with the icon?
		LVCOLUMN lvColumn;
		lvColumn.mask = LVCF_TEXT | LVCF_FMT;

		// Column 0: Achievement
		tstring tstr = U82T_c(C_("AchievementsTab", "Achievement"));
		lvColumn.pszText = const_cast<LPTSTR>(tstr.c_str());
		lvColumn.fmt = LVCFMT_LEFT;
		ListView_InsertColumn(hListView, 0, &lvColumn);

		// Column 1: Unlock Time
		tstr = U82T_c(C_("AchievementsTab", "Unlock Time"));
		lvColumn.pszText = const_cast<LPTSTR>(tstr.c_str());
		lvColumn.fmt = LVCFMT_LEFT;
		ListView_InsertColumn(hListView, 1, &lvColumn);
	}

	// Maximum width for column 1.
	// FIXME: Get auto-sizing working.
	// FIXME: Newlines don't work in ListView on WinXP and wine-staging-5.18.
	AutoGetDC_font hDC(hWndPropSheet, GetWindowFont(hWndPropSheet));
	int col1Width = 0;

	// Add the achievements.
	// TODO: Copy over CustomDraw from RP_ShellPropSheetExt for newline handling?
	const Achievements *const pAch = Achievements::instance();
	for (int i = 0; i < (int)Achievements::ID::Max; i++) {
		const Achievements::ID id = (Achievements::ID)i;
		const time_t timestamp = pAch->isUnlocked(id);

		// Get the name and description.
		tstring ts_ach = U82T_c(pAch->getName(id));
		ts_ach += _T('\n');
		// TODO: Locked description?
		ts_ach += U82T_c(pAch->getDescUnlocked(id));

		// Measure the text width.
		const int col1Width_cur = LibWin32UI::measureStringForListView(hDC, ts_ach);
		if (col1Width_cur > col1Width) {
			col1Width = col1Width_cur;
		}

		// Column 0: Achievement
		LVITEM item;
		item.mask = LVIF_TEXT | LVIF_IMAGE;
		item.iItem = i;
		item.iSubItem = 0;
		item.pszText = const_cast<LPTSTR>(ts_ach.c_str());
		item.iImage = i;
		ListView_InsertItem(hListView, &item);

		// Column 1: Unlock time
		tstring ts_timestamp;
		if (timestamp != -1) {
			ts_timestamp = formatDateTime(timestamp,
				RomFields::RFT_DATETIME_HAS_DATE | RomFields::RFT_DATETIME_HAS_TIME);
		}

		item.mask = LVIF_TEXT;
		item.iSubItem = 1;
		item.pszText = const_cast<LPTSTR>(ts_timestamp.c_str());
		ListView_SetItem(hListView, &item);
	}

	// Get the icon size for the current DPI.
	// Reference: https://docs.microsoft.com/en-us/windows/win32/hidpi/high-dpi-desktop-application-development-on-windows
	// TODO: Handle WM_DPICHANGED.
	// TODO: Consolidate with updateImageList().
	const UINT dpi = rp_GetDpiForWindow(hWndPropSheet);
	unsigned int iconSize;
	if (dpi < 144) {
		// [96,144) dpi: Use 32x32.
		iconSize = 32;
	} else {
		// >144dpi: Use 64x64.
		iconSize = 64;
	}

	// Auto-size the columns.
	ListView_SetColumnWidth(hListView, 0, iconSize + 4 + col1Width);
	ListView_SetColumnWidth(hListView, 1, LVSCW_AUTOSIZE_USEHEADER);

	// Update the ListView style.
	// This will also update the icons.
	updateListViewStyle();
}

/** AchievementsTab **/

AchievementsTab::AchievementsTab(void)
	: d_ptr(new AchievementsTabPrivate())
{}

AchievementsTab::~AchievementsTab()
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
HPROPSHEETPAGE AchievementsTab::getHPropSheetPage(void)
{
	RP_D(AchievementsTab);
	assert(d->hPropSheetPage == nullptr);
	if (d->hPropSheetPage) {
		// Property sheet has already been created.
		return nullptr;
	}

	// tr: Tab title.
	const tstring tsTabTitle = U82T_c(C_("AchievementsTab", "Achievements"));

	PROPSHEETPAGE psp;
	psp.dwSize = sizeof(psp);
	psp.dwFlags = PSP_USECALLBACK | PSP_USETITLE | PSP_DLGINDIRECT;
	psp.hInstance = HINST_THISCOMPONENT;
	psp.pResource = LoadDialog_i18n(HINST_THISCOMPONENT, IDD_CONFIG_ACHIEVEMENTS);
	psp.pszIcon = nullptr;
	psp.pszTitle = tsTabTitle.c_str();
	psp.pfnDlgProc = AchievementsTabPrivate::dlgProc;
	psp.lParam = reinterpret_cast<LPARAM>(d);
	psp.pcRefParent = nullptr;
	psp.pfnCallback = AchievementsTabPrivate::callbackProc;

	d->hPropSheetPage = CreatePropertySheetPage(&psp);
	return d->hPropSheetPage;
}

/**
 * Reset the contents of this tab.
 */
void AchievementsTab::reset(void)
{
	RP_D(AchievementsTab);
	d->reset();
}
