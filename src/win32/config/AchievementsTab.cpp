/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * AchievementsTab.cpp: Achievements tab for rp-config.                    *
 *                                                                         *
 * Copyright (c) 2016-2020 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "AchievementsTab.hpp"
#include "res/resource.h"

// librpbase
#include "librpbase/Achievements.hpp"
using LibRpBase::Achievements;

// C++ STL classes.
using std::tstring;

class AchievementsTabPrivate
{
	public:
		AchievementsTabPrivate();

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

	public:
		/**
		 * Update the ListView style.
		 */
		void updateListViewStyle(void);

		/**
		 * Reset the configuration.
		 */
		void reset(void);
};

/** AchievementsTabPrivate **/

AchievementsTabPrivate::AchievementsTabPrivate()
	: hPropSheetPage(nullptr)
	, hWndPropSheet(nullptr)
{ }

/**
 * Dialog procedure.
 * @param hDlg
 * @param uMsg
 * @param wParam
 * @param lParam
 */
INT_PTR CALLBACK AchievementsTabPrivate::dlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
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

			// Reset the configuration.
			d->reset();
			return TRUE;
		}

		case WM_SYSCOLORCHANGE:
		case WM_THEMECHANGED: {
			auto *const d = reinterpret_cast<AchievementsTabPrivate*>(GetWindowLongPtr(hDlg, GWLP_USERDATA));
			if (!d) {
				// No AchievementsTabPrivate. Can't do anything...
				return false;
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
	const DWORD lvsExStyle = likely(!GetSystemMetrics(SM_REMOTESESSION))
		? LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER
		: LVS_EX_DOUBLEBUFFER;
	ListView_SetExtendedListViewStyle(hListView, lvsExStyle);
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

	// Update the ListView style.
	updateListViewStyle();

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

	// Add the achievements.
	// TODO: Copy over CustomDraw from RP_ShellPropSheetExt for newline handling?
	// TODO: Icon/imagelist.
	LVITEM item;
	item.mask = LVIF_TEXT;
	const Achievements *const pAch = Achievements::instance();
	for (int i = 0; i < (int)Achievements::ID::Max; i++) {
		const Achievements::ID id = (Achievements::ID)i;
		const time_t timestamp = pAch->isUnlocked(id);
		const bool unlocked = (timestamp != -1);

		// Get the name and description.
		tstring ts_ach = U82T_c(pAch->getName(id));
		ts_ach += _T('\n');
		// TODO: Locked description?
		ts_ach += U82T_c(pAch->getDescUnlocked(id));

		// Column 0: Achievement
		item.iItem = i;
		item.iSubItem = 0;
		item.pszText = const_cast<LPTSTR>(ts_ach.c_str());
		ListView_InsertItem(hListView, &item);

		// Column 1: Unlock time
		TCHAR dateTimeStr[256];
		dateTimeStr[0] = _T('\0');
		if (timestamp != -1) {
			int cchBuf = _countof(dateTimeStr);

			SYSTEMTIME st;
			UnixTimeToSystemTime(timestamp, &st);

			// Date portion
			int ret = GetDateFormat(
				MAKELCID(LOCALE_USER_DEFAULT, SORT_DEFAULT),
				DATE_SHORTDATE,
				&st, nullptr, dateTimeStr, cchBuf);
			if (ret > 0) {
				int start_pos = ret-1;
				cchBuf -= (ret-1);

				// Add a space.
				dateTimeStr[start_pos] = _T(' ');
				start_pos++;
				cchBuf--;

				// Time portion
				int ret = GetTimeFormat(
					MAKELCID(LOCALE_USER_DEFAULT, SORT_DEFAULT),
					0, &st, nullptr, &dateTimeStr[start_pos], cchBuf);
				if (ret <= 0) {
					dateTimeStr[0] = _T('\0');
				}
			}
		}

		item.iSubItem = 1;
		item.pszText = dateTimeStr;
		ListView_SetItem(hListView, &item);
	}

	// Auto-size the columns.
	// TODO: Does it work with newlines when not using ownerdata?
	for (int i = 0; i < 2; i++) {
		ListView_SetColumnWidth(hListView, i, LVSCW_AUTOSIZE_USEHEADER);
	}
}

/** AchievementsTab **/

AchievementsTab::AchievementsTab(void)
	: d_ptr(new AchievementsTabPrivate())
{ }

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
	psp.pResource = LoadDialog_i18n(IDD_CONFIG_ACHIEVEMENTS);
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

/**
 * Load the default configuration.
 * This does NOT save, and will only emit modified()
 * if it's different from the current configuration.
 */
void AchievementsTab::loadDefaults(void)
{
	// Nothing to load here...
}

/**
 * Save the contents of this tab.
 */
void AchievementsTab::save(void)
{
	// Nothing to save here...
}
