/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * KeyManagerTab.hpp: Key Manager tab for rp-config.                       *
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
#include "KeyManagerTab.hpp"
#include "resource.h"

// KeyStore
#include "KeyStoreWin32.hpp"

// IListView and other undocumented stuff.
#include "libwin32common/sdk/IListView.hpp"
#include "KeyStore_OwnerDataCallback.hpp"

// libwin32common
#include "libwin32common/WinUI.hpp"

// librpbase
#include "librpbase/TextFuncs.hpp"
#include "librpbase/config/Config.hpp"
#include "librpbase/crypto/KeyManager.hpp"
using namespace LibRpBase;

// libi18n
#include "libi18n/i18n.h"

// libromdata
#include "libromdata/disc/WiiPartition.hpp"
#include "libromdata/crypto/CtrKeyScrambler.hpp"
#include "libromdata/crypto/N3DSVerifyKeys.hpp"
using namespace LibRomData;

// C includes. (C++ namespace)
#include <cassert>

// C++ includes.
#include <algorithm>
#include <string>
using std::wstring;

class KeyManagerTabPrivate
{
	public:
		KeyManagerTabPrivate();
		~KeyManagerTabPrivate();

	private:
		RP_DISABLE_COPY(KeyManagerTabPrivate)

	public:
		// Property for "D pointer".
		// This points to the KeyManagerTabPrivate object.
		static const wchar_t D_PTR_PROP[];

	public:
		/**
		 * Initialize the UI.
		 */
		void initUI(void);

		/**
		 * Initialize the monospaced font.
		 * TODO: Combine with RP_ShellPropSheetExt's monospaced font code.
		 * @param hFont Base font.
		 */
		void initMonospacedFont(HFONT hFont);

	public:
		/**
		 * Reset the configuration.
		 */
		void reset(void);

		/**
		 * Save the configuration.
		 */
		void save(void);

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

		/**
		 * ListView subclass procedure.
		 * @param hWnd		Control handle.
		 * @param uMsg		Message.
		 * @param wParam	WPARAM
		 * @param lParam	LPARAM
		 * @param uIdSubclass	Subclass ID. (usually the control ID)
		 * @param dwRefData	KeyManagerTabPrivate*
		 * @return
		 */
		static LRESULT CALLBACK ListViewSubclassProc(
			HWND hWnd, UINT uMsg,
			WPARAM wParam, LPARAM lParam,
			UINT_PTR uIdSubclass, DWORD_PTR dwRefData);

		/**
		 * ListView EDIT control subclass procedure.
		 * @param hWnd		Control handle.
		 * @param uMsg		Message.
		 * @param wParam	WPARAM
		 * @param lParam	LPARAM
		 * @param uIdSubclass	Subclass ID. (usually the control ID)
		 * @param dwRefData	KeyManagerTabPrivate*
		 * @return
		 */
		static LRESULT CALLBACK ListViewEditSubclassProc(
			HWND hWnd, UINT uMsg,
			WPARAM wParam, LPARAM lParam,
			UINT_PTR uIdSubclass, DWORD_PTR dwRefData);

	public:
		// Property sheet.
		HPROPSHEETPAGE hPropSheetPage;
		HWND hWndPropSheet;

	public:
		// "Import" popup menu.
		HMENU hMenuImport;	// Must be deleted using DestroyMenu().

		// KeyStore.
		KeyStoreWin32 *keyStore;
		KeyStore_OwnerDataCallback *keyStore_ownerDataCallback;

		// Fonts.
		HFONT hFontDlg;		// Main dialog font.
		HFONT hFontMono;	// Monospaced font.

		// Monospaced font details.
		LOGFONT lfFontMono;
		bool bPrevIsClearType;	// Previous ClearType setting.

		// EDIT box for ListView.
		HWND hEditBox;
		int iEditItem;		// Item being edited. (-1 for none)
		bool bCancelEdit;	// True if the edit is being cancelled.
		bool bAllowKanji;	// Allow kanji in the editor.

		// Is this COMCTL32.dll 6.10 or later?
		bool isComCtl32_610;

		// Icons for the "Valid?" column.
		// NOTE: "?" and "X" are copies from User32.
		// Checkmark is a PNG image loaded from a resource.
		// FIXME: Assuming 16x16 icons. May need larger for HiDPI.
		static const SIZE szIcon;
		HICON hIconUnknown;	// "?" (USER32.dll,-102)
		HICON hIconInvalid;	// "X" (USER32.dll,-103)
		HICON hIconGood;	// Checkmark

		// Alternate row color.
		COLORREF colorAltRow;
		HBRUSH hbrAltRow;

		/**
		 * ListView CustomDraw function.
		 * @param hListView	[in] ListView control.
		 * @param plvcd		[in/out] NMLVCUSTOMDRAW
		 * @return Return value.
		 */
		inline int ListView_CustomDraw(HWND hListView, NMLVCUSTOMDRAW *plvcd);

		/**
		 * Load images.
		 */
		void loadImages(void);

	public:
		/** "Import" menu actions. **/

		/**
		 * Convert pipes (L'|') to NULLs (L'\0').
		 *
		 * This is needed because Win32 file filters use embedded
		 * NULL characters, but gettext doesn't support that because
		 * it uses C strings.
		 *
		 * @param wstr std::wstring to modify.
		 */
		static inline void convertPipesToNulls(wstring &wstr)
		{
			for (auto iter = wstr.begin(); iter != wstr.end(); ++iter) {
				if (*iter == L'|') {
					*iter = L'\0';
				}
			}
		}

		/**
		 * Import keys from Wii keys.bin. (BootMii format)
		 */
		void importWiiKeysBin(void);

		/**
		 * Import keys from Wii U otp.bin.
		 */
		void importWiiUOtpBin(void);

		/**
		 * Import keys from 3DS boot9.bin.
		 */
		void import3DSboot9bin(void);

		/**
		 * Import keys from 3DS aeskeydb.bin.
		 */
		void import3DSaeskeydb(void);
};

/** KeyManagerTabPrivate **/

// FIXME: Assuming 16x16 icons. May need larger for HiDPI.
const SIZE KeyManagerTabPrivate::szIcon = {16, 16};

KeyManagerTabPrivate::KeyManagerTabPrivate()
	: hPropSheetPage(nullptr)
	, hWndPropSheet(nullptr)
	, hMenuImport(nullptr)
	, keyStore(new KeyStoreWin32(nullptr))
	, keyStore_ownerDataCallback(nullptr)
	, hFontDlg(nullptr)
	, hFontMono(nullptr)
	, bPrevIsClearType(nullptr)
	, hEditBox(nullptr)
	, iEditItem(-1)
	, bCancelEdit(false)
	, bAllowKanji(false)
	, isComCtl32_610(false)
	, hIconUnknown(nullptr)
	, hIconInvalid(nullptr)
	, hIconGood(nullptr)
	, colorAltRow(0)
	, hbrAltRow(nullptr)
{
	memset(&lfFontMono, 0, sizeof(lfFontMono));

	// Load images.
	loadImages();

	// Initialize the alternate row color.
	colorAltRow = LibWin32Common::getAltRowColor();
	hbrAltRow = CreateSolidBrush(colorAltRow);

	// Check the COMCTL32.DLL version.
	HMODULE hComCtl32 = GetModuleHandle(L"COMCTL32");
	assert(hComCtl32 != nullptr);
	typedef HRESULT (CALLBACK *PFNDLLGETVERSION)(DLLVERSIONINFO *pdvi);
	PFNDLLGETVERSION pfnDllGetVersion = nullptr;
	if (hComCtl32) {
		pfnDllGetVersion = (PFNDLLGETVERSION)GetProcAddress(hComCtl32, "DllGetVersion");
	}
	if (pfnDllGetVersion) {
		DLLVERSIONINFO dvi;
		dvi.cbSize = sizeof(dvi);
		HRESULT hr = pfnDllGetVersion(&dvi);
		if (SUCCEEDED(hr)) {
			isComCtl32_610 = dvi.dwMajorVersion > 6 ||
				(dvi.dwMajorVersion == 6 && dvi.dwMinorVersion >= 10);
		}
	}
}

KeyManagerTabPrivate::~KeyManagerTabPrivate()
{
	if (hMenuImport) {
		DestroyMenu(hMenuImport);
	}
	if (hFontMono) {
		DeleteFont(hFontMono);
	}

	delete keyStore;
	if (keyStore_ownerDataCallback) {
		keyStore_ownerDataCallback->Release();
	}

	// Icons.
	if (hIconUnknown) {
		DestroyIcon(hIconUnknown);
	}
	if (hIconInvalid) {
		DestroyIcon(hIconInvalid);
	}
	if (hIconGood) {
		DestroyIcon(hIconGood);
	}

	// Alternate row color.
	if (hbrAltRow) {
		DeleteBrush(hbrAltRow);
	}
}

// Property for "D pointer".
// This points to the KeyManagerTabPrivate object.
const wchar_t KeyManagerTabPrivate::D_PTR_PROP[] = L"KeyManagerTabPrivate";

/**
 * Initialize the UI.
 */
void KeyManagerTabPrivate::initUI(void)
{
	assert(hWndPropSheet != nullptr);
	if (!hWndPropSheet)
		return;

	// Initialize the fonts.
	hFontDlg = GetWindowFont(hWndPropSheet);
	initMonospacedFont(hFontDlg);

	// Get the required controls.
	HWND hBtnImport = GetDlgItem(hWndPropSheet, IDC_KEYMANAGER_IMPORT);
	HWND hListView = GetDlgItem(hWndPropSheet, IDC_KEYMANAGER_LIST);
	assert(hBtnImport != nullptr);
	assert(hListView != nullptr);
	if (!hBtnImport || !hListView)
		return;

	if (isComCtl32_610) {
		// COMCTL32 is v6.10 or later. Use BS_SPLITBUTTON.
		// (Windows Vista or later)
		LONG lStyle = GetWindowLong(hBtnImport, GWL_STYLE);
		lStyle |= BS_SPLITBUTTON;
		SetWindowLong(hBtnImport, GWL_STYLE, lStyle);
		BUTTON_SPLITINFO bsi;
		bsi.mask = BCSIF_STYLE;
		bsi.uSplitStyle = BCSS_NOSPLIT;
		Button_SetSplitInfo(hBtnImport, &bsi);
	} else {
		// COMCTL32 is older than v6.10. Use a regular button.
		// NOTE: The Unicode down arrow doesn't show on on Windows XP.
		// Maybe we *should* use ownerdraw...
		SetWindowText(hBtnImport, U82W_c(C_("KeyManagerTab", "Import...")));
	}

	// Initialize the ListView.
	// Set full row selection.
	DWORD dwExStyle;
	if (!GetSystemMetrics(SM_REMOTESESSION)) {
		// Not RDP (or is RemoteFX): Enable double buffering.
		dwExStyle = LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER;
	} else {
		// RDP: Disable double buffering to reduce bandwidth usage.
		dwExStyle = LVS_EX_FULLROWSELECT;
	}
	ListView_SetExtendedListViewStyle(hListView, dwExStyle);

	// Set the virtual list item count.
	ListView_SetItemCountEx(hListView, keyStore->totalKeyCount(),
		LVSICF_NOINVALIDATEALL | LVSICF_NOSCROLL);

	// Column title.
	wstring wsColTitle;

	// tr: Column 0: Key Name.
	wsColTitle = U82W_c(C_("KeyManagerTab", "Key Name"));
	LVCOLUMN lvCol;
	memset(&lvCol, 0, sizeof(lvCol));
	lvCol.mask = LVCF_FMT | LVCF_TEXT | LVCF_SUBITEM;
	lvCol.fmt = LVCFMT_LEFT;
	lvCol.pszText = const_cast<LPWSTR>(wsColTitle.c_str());
	ListView_InsertColumn(hListView, 0, &lvCol);

	// tr: Column 1: Value.
	wsColTitle = U82W_c(C_("KeyManagerTab", "Value"));
	lvCol.pszText = const_cast<LPWSTR>(wsColTitle.c_str());
	ListView_InsertColumn(hListView, 1, &lvCol);

	// tr: Column 2: Verification status.
	wsColTitle = U82W_c(C_("KeyManagerTab", "Valid?"));
	lvCol.pszText = const_cast<LPWSTR>(wsColTitle.c_str());
	ListView_InsertColumn(hListView, 2, &lvCol);

	if (isComCtl32_610) {
		// Set the IOwnerDataCallback.
		bool hasIListView = false;

		// Check for Windows 7 IListView first.
		{
			IListView_Win7 *pListView = nullptr;
			ListView_QueryInterface(hListView, IID_IListView_Win7, &pListView);
			if (pListView) {
				// IListView obtained.
				keyStore_ownerDataCallback = new KeyStore_OwnerDataCallback(keyStore);
				pListView->SetOwnerDataCallback(keyStore_ownerDataCallback);
				pListView->Release();
				hasIListView = true;
			}
		}

		// If that failed, check for Windows Vista IListView.
		if (!hasIListView) {
			IListView_WinVista *pListView = nullptr;
			ListView_QueryInterface(hListView, IID_IListView_WinVista, &pListView);
			if (pListView) {
				// IListView obtained.
				keyStore_ownerDataCallback = new KeyStore_OwnerDataCallback(keyStore);
				pListView->SetOwnerDataCallback(keyStore_ownerDataCallback);
				pListView->Release();
				hasIListView = true;
			}
		}

		if (hasIListView) {
			// Create groups for each section.
			// NOTE: We have to use the Vista+ LVGROUP definition.
			LVGROUP_Vista lvGroup;
			lvGroup.cbSize = sizeof(lvGroup);
			lvGroup.mask = LVGF_ALIGN | LVGF_GROUPID | LVGF_HEADER | LVGF_ITEMS;
			lvGroup.uAlign = LVGA_HEADER_LEFT;
			for (int sectIdx = 0; sectIdx < keyStore->sectCount(); sectIdx++) {
				const wstring sectName = U82W_c(keyStore->sectName(sectIdx));
				lvGroup.iGroupId = sectIdx;
				lvGroup.pszHeader = const_cast<LPWSTR>(sectName.c_str());
				lvGroup.cItems = keyStore->keyCount(sectIdx);
				ListView_InsertGroup(hListView, sectIdx, &lvGroup);
			}
			ListView_EnableGroupView(hListView, true);
		}
	}

	// Determine the maximum width of columns 0 and 1.
	// This is needed because LVSCW_AUTOSIZE_USEHEADER doesn't
	// work with LVS_OWNERDATA.
	// Reference: https://stackoverflow.com/questions/9255540/how-auto-size-the-columns-width-of-a-list-view-in-virtual-mode
	// TODO: Determine the correct padding.
	// 8,12 seems to be right on both XP and 7...
	// TODO: If the user double-clicks the column splitter, it will
	// resize based on the displayed rows, not all rows.
	static const int column_padding[2] = {8, 12};
	int column_width[2] = {0, 0};

	// Make sure the "Value" column is at least 32 characters wide.
	// NOTE: ListView_GetStringWidth() doesn't adjust for the monospaced font.
	SIZE szValue;
	int ret = LibWin32Common::measureTextSize(hListView, hFontMono, L"0123456789ABCDEF0123456789ABCDEF", &szValue);
	assert(ret == 0);
	if (ret == 0) {
		column_width[1] = szValue.cx + column_padding[1];
	}
	//column_width[1] = ListView_GetStringWidth(hListView, L"0123456789ABCDEF0123456789ABCDEF") + column_padding[1];

	for (int i = keyStore->totalKeyCount()-1; i >= 0; i--) {
		const KeyStoreWin32::Key *key = keyStore->getKey(i);
		assert(key != nullptr);
		if (!key)
			continue;

		int tmp_width[2];
		tmp_width[0] = ListView_GetStringWidth(hListView, U82W_s(key->name)) + column_padding[0];
		//tmp_width[1] = ListView_GetStringWidth(hListView, U82W_s(key->value)) + column_padding[1];

		column_width[0] = std::max(column_width[0], tmp_width[0]);
		//column_width[1] = std::max(column_width[1], tmp_width[1]);

		int ret = LibWin32Common::measureTextSize(hListView, hFontMono, U82W_s(key->value), &szValue);
		assert(ret == 0);
		if (ret == 0) {
			column_width[1] = std::max(column_width[1], (int)szValue.cx + column_padding[1]);
		}
	}
	ListView_SetColumnWidth(hListView, 0, column_width[0]);
	ListView_SetColumnWidth(hListView, 1, column_width[1]);

	// Auto-size the "Valid?" column.
	ListView_SetColumnWidth(hListView, 2, LVSCW_AUTOSIZE_USEHEADER);

	// Subclass the ListView.
	// TODO: Error handling?
	SetWindowSubclass(hListView, ListViewSubclassProc,
		IDC_KEYMANAGER_LIST, reinterpret_cast<DWORD_PTR>(this));

	// Create the EDIT box.
	hEditBox = CreateWindowEx(WS_EX_LEFT,
		WC_EDIT, nullptr,
		WS_CHILDWINDOW | WS_BORDER | ES_LEFT | ES_AUTOHSCROLL | ES_UPPERCASE | ES_WANTRETURN,
		0, 0, 0, 0,
		hListView, (HMENU)IDC_KEYMANAGER_EDIT, nullptr, nullptr);
	SetWindowFont(hEditBox, hFontMono ? hFontMono : hFontDlg, FALSE);
	SetWindowSubclass(hEditBox, ListViewEditSubclassProc,
		IDC_KEYMANAGER_EDIT, reinterpret_cast<DWORD_PTR>(this));

	// Set the KeyStore's window.
	keyStore->setHWnd(hWndPropSheet);

	// Register for WTS session notifications. (Remote Desktop)
	WTSRegisterSessionNotification(hWndPropSheet, NOTIFY_FOR_THIS_SESSION);
}

/**
 * Initialize the monospaced font.
 * TODO: Combine with RP_ShellPropSheetExt's monospaced font code.
 * @param hFont Base font.
 */
void KeyManagerTabPrivate::initMonospacedFont(HFONT hFont)
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

		// Find a monospaced font.
		int ret = LibWin32Common::findMonospacedFont(&lfFontMono);
		if (ret != 0) {
			// Monospaced font not found.
			return;
		}
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

	// TODO: Update the ListView fonts?

	// Delete the old font and save the new one.
	HFONT hFontMonoOld = hFontMono;
	hFontMono = hFontMonoNew;
	if (hFontMonoOld) {
		DeleteFont(hFontMonoOld);
	}
	bPrevIsClearType = bIsClearType;
}

/**
 * Reset the configuration.
 */
void KeyManagerTabPrivate::reset(void)
{
	assert(hWndPropSheet != nullptr);
	if (!hWndPropSheet)
		return;

	// Reset the keys.
	keyStore->reset();
}

/**
 * Save the configuration.
 */
void KeyManagerTabPrivate::save(void)
{
	assert(hWndPropSheet != nullptr);
	if (!hWndPropSheet)
		return;

	if (!keyStore->hasChanged())
		return;

	// NOTE: This may re-check the configuration timestamp.
	const KeyManager *const keyManager = KeyManager::instance();
	const char *const filename = keyManager->filename();
	assert(filename != nullptr);
	if (!filename) {
		// No configuration filename...
		return;
	}

	// Save the keys.
	const int totalKeyCount = keyStore->totalKeyCount();
	for (int i = 0; i < totalKeyCount; i++) {
		const KeyStoreWin32::Key *const pKey = keyStore->getKey(i);
		assert(pKey != nullptr);
		if (!pKey || !pKey->modified)
			continue;

		// Save this key.
		WritePrivateProfileString(L"Keys", U82W_s(pKey->name), U82W_s(pKey->value), U82W_c(filename));
	}

	// Clear the modified status.
	keyStore->allKeysSaved();
}

/**
 * Dialog procedure.
 * @param hDlg
 * @param uMsg
 * @param wParam
 * @param lParam
 */
INT_PTR CALLBACK KeyManagerTabPrivate::dlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
		case WM_INITDIALOG: {
			// Get the pointer to the property sheet page object. This is 
			// contained in the LPARAM of the PROPSHEETPAGE structure.
			LPPROPSHEETPAGE pPage = reinterpret_cast<LPPROPSHEETPAGE>(lParam);
			if (!pPage)
				return TRUE;

			// Get the pointer to the KeyManagerTabPrivate object.
			KeyManagerTabPrivate *const d = reinterpret_cast<KeyManagerTabPrivate*>(pPage->lParam);
			if (!d)
				return TRUE;

			assert(d->hWndPropSheet == nullptr);
			d->hWndPropSheet = hDlg;

			// Store the D object pointer with this particular page dialog.
			SetProp(hDlg, D_PTR_PROP, reinterpret_cast<HANDLE>(d));

			// Initialize the UI.
			d->initUI();

			// Reset the configuration.
			d->reset();
			return TRUE;
		}

		case WM_DESTROY: {
			// Remove the D_PTR_PROP property from the page. 
			// The D_PTR_PROP property stored the pointer to the 
			// KeyManagerTabPrivate object.
			RemoveProp(hDlg, D_PTR_PROP);
			return TRUE;
		}

		case WM_NOTIFY: {
			KeyManagerTabPrivate *const d = static_cast<KeyManagerTabPrivate*>(
				GetProp(hDlg, D_PTR_PROP));
			if (!d) {
				// No KeyManagerTabPrivate. Can't do anything...
				return FALSE;
			}

			NMHDR *pHdr = reinterpret_cast<NMHDR*>(lParam);
			switch (pHdr->code) {
				case PSN_APPLY:
					// Save settings.
					d->save();
					break;

				case LVN_GETDISPINFO: {
					if (!d->keyStore || pHdr->idFrom != IDC_KEYMANAGER_LIST)
						break;

					LVITEM *plvItem = &reinterpret_cast<NMLVDISPINFO*>(lParam)->item;
					if (plvItem->iItem < 0 || plvItem->iItem >= d->keyStore->totalKeyCount()) {
						// Index is out of range.
						break;
					}

					const KeyStoreWin32::Key *key = d->keyStore->getKey(plvItem->iItem);
					if (!key)
						break;

					if (plvItem->mask & LVIF_TEXT) {
						// Fill in text.
						// NOTE: We need to store the text in a
						// temporary wstring buffer.
						switch (plvItem->iSubItem) {
							case 0:
								// Key name.
								wcscpy_s(plvItem->pszText, plvItem->cchTextMax, U82W_s(key->name));
								return TRUE;
							case 1:
								// Value.
								wcscpy_s(plvItem->pszText, plvItem->cchTextMax, U82W_s(key->value));
								return TRUE;
							default:
								// No text for "Valid?".
								plvItem->pszText[0] = 0;
								return TRUE;
						}
					}
					break;
				}

				case NM_CUSTOMDRAW: {
					// Custom drawing notification.
					if (pHdr->idFrom != IDC_KEYMANAGER_LIST)
						break;

					// NOTE: Since this is a DlgProc, we can't simply return
					// the CDRF code. It has to be set as DWLP_MSGRESULT.
					// References:
					// - https://stackoverflow.com/questions/40549962/c-winapi-listview-nm-customdraw-not-getting-cdds-itemprepaint
					// - https://stackoverflow.com/a/40552426
					const int result = d->ListView_CustomDraw(pHdr->hwndFrom, reinterpret_cast<NMLVCUSTOMDRAW*>(lParam));
					SetWindowLongPtr(hDlg, DWLP_MSGRESULT, result);
					return TRUE;
				}

				case PSN_SETACTIVE:
					// Disable the "Defaults" button.
					RpPropSheet_EnableDefaults(GetParent(hDlg), false);
					break;

				default:
					break;
			}
			break;
		}

		case WM_COMMAND: {
			KeyManagerTabPrivate *const d = static_cast<KeyManagerTabPrivate*>(
				GetProp(hDlg, D_PTR_PROP));
			if (!d) {
				// No KeyManagerTabPrivate. Can't do anything...
				return FALSE;
			}

			switch (LOWORD(wParam)) {
				case IDC_KEYMANAGER_IMPORT: {
					// Show the "Import" popup menu.
					if (!d->hMenuImport) {
						d->hMenuImport = LoadMenu(HINST_THISCOMPONENT, MAKEINTRESOURCE(IDR_KEYMANAGER_IMPORT));
					}

					if (!d->hMenuImport) {
						// Unable to create the "Import" popup menu.
						return TRUE;
					}

					HMENU hSubMenu = GetSubMenu(d->hMenuImport, 0);
					if (hSubMenu) {
						RECT btnRect;
						GetWindowRect(GetDlgItem(hDlg, IDC_KEYMANAGER_IMPORT), &btnRect);
						TrackPopupMenu(hSubMenu, TPM_LEFTALIGN|TPM_TOPALIGN,
							btnRect.left, btnRect.bottom, 0, hDlg, nullptr);
					}
					return TRUE;
				}

				case IDM_KEYMANAGER_IMPORT_WII_KEYS_BIN:
					d->importWiiKeysBin();
					return TRUE;
				case IDM_KEYMANAGER_IMPORT_WIIU_OTP_BIN:
					d->importWiiUOtpBin();
					return TRUE;
				case IDM_KEYMANAGER_IMPORT_3DS_BOOT9_BIN:
					d->import3DSboot9bin();
					break;
				case IDM_KEYMANAGER_IMPORT_3DS_AESKEYDB:
					d->import3DSaeskeydb();
					break;

				default:
					break;
			}
			break;
		}

		case WM_RP_PROP_SHEET_RESET: {
			KeyManagerTabPrivate *const d = static_cast<KeyManagerTabPrivate*>(
				GetProp(hDlg, D_PTR_PROP));
			if (!d) {
				// No KeyManagerTabPrivate. Can't do anything...
				return FALSE;
			}

			// Reset the tab.
			d->reset();
			break;
		}

		case WM_SYSCOLORCHANGE:
		case WM_THEMECHANGED: {
			// Reinitialize the alternate row color.
			KeyManagerTabPrivate *const d = static_cast<KeyManagerTabPrivate*>(
				GetProp(hDlg, D_PTR_PROP));
			if (d) {
				// Reinitialize the alternate row color.
				d->colorAltRow = LibWin32Common::getAltRowColor();
				if (d->hbrAltRow) {
					DeleteBrush(d->hbrAltRow);
				}
				d->hbrAltRow = CreateSolidBrush(d->colorAltRow);
			}
			break;
		}

		case WM_NCPAINT: {
			// Update the monospaced font.
			KeyManagerTabPrivate *const d = static_cast<KeyManagerTabPrivate*>(
				GetProp(hDlg, D_PTR_PROP));
			if (d) {
				d->initMonospacedFont(d->hFontDlg);
			}
			break;
		}

		case WM_KEYSTORE_KEYCHANGED_IDX: {
			KeyManagerTabPrivate *const d = static_cast<KeyManagerTabPrivate*>(
				GetProp(hDlg, D_PTR_PROP));
			if (!d) {
				// No KeyManagerTabPrivate. Can't do anything...
				return FALSE;
			}

			// Update the row.
			HWND hListView = GetDlgItem(d->hWndPropSheet, IDC_KEYMANAGER_LIST);
			assert(hListView != nullptr);
			if (hListView) {
				ListView_RedrawItems(hListView, (int)lParam, (int)lParam);
			}
			return TRUE;
		}

		case WM_KEYSTORE_ALLKEYSCHANGED: {
			KeyManagerTabPrivate *const d = static_cast<KeyManagerTabPrivate*>(
				GetProp(hDlg, D_PTR_PROP));
			if (!d) {
				// No KeyManagerTabPrivate. Can't do anything...
				return FALSE;
			}

			// Update all rows.
			HWND hListView = GetDlgItem(d->hWndPropSheet, IDC_KEYMANAGER_LIST);
			assert(hListView != nullptr);
			if (hListView) {
				ListView_RedrawItems(hListView, 0, d->keyStore->totalKeyCount()-1);
			}
			return TRUE;
		}

		case WM_KEYSTORE_MODIFIED: {
			KeyManagerTabPrivate *const d = static_cast<KeyManagerTabPrivate*>(
				GetProp(hDlg, D_PTR_PROP));
			if (!d) {
				// No KeyManagerTabPrivate. Can't do anything...
				return FALSE;
			}

			// Key was modified.
			PropSheet_Changed(GetParent(hDlg), hDlg);
			return TRUE;
		}

		case WM_WTSSESSION_CHANGE: {
			KeyManagerTabPrivate *const d = static_cast<KeyManagerTabPrivate*>(
				GetProp(hDlg, D_PTR_PROP));
			if (!d) {
				// No KeyManagerTabPrivate. Can't do anything...
				return FALSE;
			}
			HWND hListView = GetDlgItem(d->hWndPropSheet, IDC_KEYMANAGER_LIST);
			assert(hListView != nullptr);
			if (!hListView)
				break;
			DWORD dwExStyle = ListView_GetExtendedListViewStyle(hListView);

			// If RDP was connected, disable ListView double-buffering.
			// If console (or RemoteFX) was connected, enable ListView double-buffering.
			switch (wParam) {
				case WTS_CONSOLE_CONNECT:
					dwExStyle |= LVS_EX_DOUBLEBUFFER;
					ListView_SetExtendedListViewStyle(hListView, dwExStyle);
					break;
				case WTS_REMOTE_CONNECT:
					dwExStyle &= ~LVS_EX_DOUBLEBUFFER;
					ListView_SetExtendedListViewStyle(hListView, dwExStyle);
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
UINT CALLBACK KeyManagerTabPrivate::callbackProc(HWND hWnd, UINT uMsg, LPPROPSHEETPAGE ppsp)
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
 * ListView subclass procedure.
 * @param hWnd		Control handle.
 * @param uMsg		Message.
 * @param wParam	WPARAM
 * @param lParam	LPARAM
 * @param uIdSubclass	Subclass ID. (usually the control ID)
 * @param dwRefData	KeyManagerTabPrivate*
 * @return
 */
LRESULT CALLBACK KeyManagerTabPrivate::ListViewSubclassProc(
	HWND hWnd, UINT uMsg,
	WPARAM wParam, LPARAM lParam,
	UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	if (!dwRefData) {
		// No RP_ShellPropSheetExt. Can't do anything...
		return DefSubclassProc(hWnd, uMsg, wParam, lParam);
	}

	switch (uMsg) {
		case WM_LBUTTONDBLCLK: {
			// Reference: http://www.cplusplus.com/forum/windows/107679/
			KeyManagerTabPrivate *const d =
				reinterpret_cast<KeyManagerTabPrivate*>(dwRefData);
			assert(d->hWndPropSheet != nullptr);
			if (!d->hWndPropSheet)
				return FALSE;

			// Check for a double-click in the ListView.
			// ListView only directly supports editing of the
			// first column, so we have to handle it manually
			// for the second column (Value).
			LVHITTESTINFO lvhti;
			lvhti.pt.x = GET_X_LPARAM(lParam);
			lvhti.pt.y = GET_Y_LPARAM(lParam);

			// Check if this point maps to a valid "Value" subitem.
			int iItem = ListView_SubItemHitTest(hWnd, &lvhti);
			if (iItem < 0 || lvhti.iSubItem != 1) {
				// Not a "Value" subitem.
				break;
			}

			// Get the key.
			const KeyStoreWin32::Key *key = d->keyStore->getKey(iItem);
			assert(key != nullptr);
			if (!key)
				break;

			// Make the edit box visible at the subitem's location.
			// TODO: Subclass the edit box.
			//HWND hEditBox = GetDlgItem(d->hWndPropSheet, IDC_KEYMANAGER_EDIT);
			assert(d->hEditBox != nullptr);
			if (!d->hEditBox)
				break;

			// Copy the text from the ListView to the EDIT control.
			wchar_t szItemText[128];
			ListView_GetItemText(hWnd, iItem, lvhti.iSubItem, szItemText, ARRAY_SIZE(szItemText));
			SetWindowText(d->hEditBox, szItemText);
			// FIXME: ES_AUTOHSCROLL causes some initial scrolling weirdness here,
			// but disabling it prevents entering more text than fits onscreen...
			Edit_SetSel(d->hEditBox, 0, -1);	// Select All

			d->iEditItem = iItem;
			d->bCancelEdit = false;
			d->bAllowKanji = key->allowKanji;

			// Set the EDIT control's position.
			RECT rectSubItem;
			ListView_GetSubItemRect(hWnd, iItem, lvhti.iSubItem, LVIR_BOUNDS, &rectSubItem);
			SetWindowPos(d->hEditBox, HWND_TOPMOST, rectSubItem.left, rectSubItem.top,
				rectSubItem.right - rectSubItem.left,
				rectSubItem.bottom - rectSubItem.top,
				SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOZORDER | SWP_SHOWWINDOW);
			SetFocus(d->hEditBox);
			return TRUE;
		}

		case WM_NCDESTROY:
			// Remove the window subclass.
			// Reference: https://blogs.msdn.microsoft.com/oldnewthing/20031111-00/?p=41883
			RemoveWindowSubclass(hWnd, ListViewSubclassProc, uIdSubclass);
			break;

		default:
			break;
	}

	return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

/**
 * ListView EDIT control subclass procedure.
 * @param hWnd		Control handle.
 * @param uMsg		Message.
 * @param wParam	WPARAM
 * @param lParam	LPARAM
 * @param uIdSubclass	Subclass ID. (usually the control ID)
 * @param dwRefData	KeyManagerTabPrivate*
 * @return
 */
LRESULT CALLBACK KeyManagerTabPrivate::ListViewEditSubclassProc(
	HWND hWnd, UINT uMsg,
	WPARAM wParam, LPARAM lParam,
	UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	if (!dwRefData) {
		// No RP_ShellPropSheetExt. Can't do anything...
		return DefSubclassProc(hWnd, uMsg, wParam, lParam);
	}

	KeyManagerTabPrivate *const d =
		reinterpret_cast<KeyManagerTabPrivate*>(dwRefData);
	assert(d->hWndPropSheet != nullptr);
	if (!d->hWndPropSheet)
		return FALSE;

	switch (uMsg) {
		case WM_KILLFOCUS: {
			ShowWindow(hWnd, SW_HIDE);
			if (d->bCancelEdit)
				break;

			// NOTE: ListView_SetItem() doesn't work with LVS_OWNERDATA.
			// We'll have to edit the KeyStore directly.
			if (!d->keyStore)
				break;
			else if (d->iEditItem < 0 || d->iEditItem >= d->keyStore->totalKeyCount())
				break;

			// Save the key.
			wchar_t buf[128];
			buf[0] = 0;
			GetWindowText(hWnd, buf, ARRAY_SIZE(buf));
			d->keyStore->setKey(d->iEditItem, W2U8(buf));

			// Item is no longer being edited.
			d->iEditItem = -1;
			break;
		}

		case WM_GETDLGCODE:
			return (DLGC_WANTALLKEYS | DefSubclassProc(hWnd, uMsg, wParam, lParam));

		case WM_CHAR: {
			// Reference: https://support.microsoft.com/en-us/help/102589/how-to-use-the-enter-key-from-edit-controls-in-a-dialog-box
			switch (wParam) {
				case VK_RETURN:
					// Finished editing.
					d->bCancelEdit = false;
					ShowWindow(hWnd, SW_HIDE);
					return TRUE;
				case VK_ESCAPE:
					// Cancel editing.
					d->bCancelEdit = true;
					ShowWindow(hWnd, SW_HIDE);
					return TRUE;
				default:
					break;
			}

			// Filter out invalid characters.

			// Always allow control characters and hexadecimal digits.
			if (iswcntrl((wint_t)wParam) || iswxdigit((wint_t)wParam)) {
				// This is a valid control character or hexadecimal digit.
				break;
			}

			// Check for kanji.
			if (d->bAllowKanji) {
				// Reference: http://www.localizingjapan.com/blog/2012/01/20/regular-expressions-for-japanese-text/
				if ((wParam >= 0x3400 && wParam <= 0x4DB5) ||
				    (wParam >= 0x4E00 && wParam <= 0x9FCB) ||
				    (wParam >= 0xF900 && wParam <= 0xFA6A))
				{
					// Valid kanji character.
					break;
				}
			}

			// Character is not allowed.
			return TRUE;
		}

		case WM_KEYDOWN:
		case WM_KEYUP: {
			// Reference: https://support.microsoft.com/en-us/help/102589/how-to-use-the-enter-key-from-edit-controls-in-a-dialog-box
			switch (wParam) {
				case VK_RETURN:
					// Finished editing.
					d->bCancelEdit = false;
					ShowWindow(hWnd, SW_HIDE);
					return TRUE;
				case VK_ESCAPE:
					// Cancel editing.
					d->bCancelEdit = true;
					ShowWindow(hWnd, SW_HIDE);
					return TRUE;
				default:
					break;
			}
			break;
		}

		case WM_PASTE: {
			// Filter out text pasted in from the clipboard.
			// Reference: https://stackoverflow.com/questions/22263612/properly-handle-wm-paste-in-subclass-procedure
			if (!OpenClipboard(hWnd))
				return TRUE;

			HANDLE hClipboardData = GetClipboardData(CF_UNICODETEXT);
			if (!hClipboardData) {
				CloseClipboard();
				return TRUE;
			}

			const wchar_t *const pchData = reinterpret_cast<const wchar_t*>(GlobalLock(hClipboardData));
			if (!pchData) {
				// No data.
				CloseClipboard();
				return TRUE;
			} else if (pchData[0] == 0) {
				// Empty string.
				// TODO: Paste anyway?
				GlobalUnlock(hClipboardData);
				CloseClipboard();
				return TRUE;
			}

			// Filter out invalid characters.
			wstring wstr;
			wstr.reserve(wcslen(pchData));
			for (const wchar_t *p = pchData; *p != 0; p++) {
				// Allow hexadecimal digits.
				if (iswxdigit(*p)) {
					wstr += *p;
				} else if (d->bAllowKanji) {
					// Allow kanji.
					// Reference: http://www.localizingjapan.com/blog/2012/01/20/regular-expressions-for-japanese-text/
					if ((*p >= 0x3400 && *p <= 0x4DB5) ||
					    (*p >= 0x4E00 && *p <= 0x9FCB) ||
					    (*p >= 0xF900 && *p <= 0xFA6A))
					{
						wstr += *p;
					} else {
						// Invalid character.
						// Prevent the paste.
						GlobalUnlock(hClipboardData);
						CloseClipboard();
						return TRUE;
					}
				} else {
					// Invalid character.
					// Prevent the paste.
					GlobalUnlock(hClipboardData);
					CloseClipboard();
					return TRUE;
				}
			}

			GlobalUnlock(hClipboardData);
			CloseClipboard();

			if (!wstr.empty()) {
				// Insert the text.
				// TODO: Paste even if empty?
				Edit_ReplaceSel(hWnd, wstr.c_str());
			}
			return TRUE;
		}

		case WM_NCDESTROY:
			// Remove the window subclass.
			// Reference: https://blogs.msdn.microsoft.com/oldnewthing/20031111-00/?p=41883
			RemoveWindowSubclass(hWnd, ListViewSubclassProc, uIdSubclass);
			break;

		default:
			break;
	}

	return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

/**
 * ListView CustomDraw function.
 * @param hListView	[in] ListView control.
 * @param plvcd		[in] NMLVCUSTOMDRAW
 * @return Return value.
 */
inline int KeyManagerTabPrivate::ListView_CustomDraw(HWND hListView, NMLVCUSTOMDRAW *plvcd)
{
	// Check if this is an "odd" row.
	bool isOdd;
	if (isComCtl32_610) {
		// COMCTL32.dll v6.10: We're using groups, so
		// check the key index within the section.
		int sectIdx = -1, keyIdx = -1;
		int ret = keyStore->idxToSectKey((int)plvcd->nmcd.dwItemSpec, &sectIdx, &keyIdx);
		if (ret == 0) {
			isOdd = !!(keyIdx % 2);
		} else {
			// Unable to get sect/key.
			// Fall back to the flat index.
			isOdd = !!(plvcd->nmcd.dwItemSpec % 2);
		}
	} else {
		// COMCTL32.dll v6.00 or earlier: No groups.
		// Use the flat key index.
		isOdd = !!(plvcd->nmcd.dwItemSpec % 2);
	}

	// Make sure the "Value" column is drawn with a monospaced font.
	// Reference: https://www.codeproject.com/Articles/2890/Using-ListView-control-under-Win-API
	int result = CDRF_DODEFAULT;
	switch (plvcd->nmcd.dwDrawStage) {
		case CDDS_PREPAINT:
			// Request notifications for individual ListView items.
			result = CDRF_NOTIFYITEMDRAW;
			break;

		case CDDS_ITEMPREPAINT: {
			// Set the background color for alternating row colors.
			if (isOdd) {
				// NOTE: plvcd->clrTextBk is set to 0xFF000000 here,
				// not the actual default background color.
				// FIXME: On Windows 7:
				// - Standard row colors are 19px high.
				// - Alternate row colors are 17px high. (top and bottom lines ignored?)
				plvcd->clrTextBk = colorAltRow;
				result = CDRF_NOTIFYSUBITEMDRAW | CDRF_NEWFONT;
			} else {
				result = CDRF_NOTIFYSUBITEMDRAW;
			}
			break;
		}

		case CDDS_SUBITEM | CDDS_ITEMPREPAINT:
			switch (plvcd->iSubItem) {
				case 1:
					// "Value" column.
					// Use the monospaced font.
					if (hFontMono) {
						SelectObject(plvcd->nmcd.hdc, hFontMono);
						result = CDRF_NEWFONT;
					}
					break;

				case 2: {
					// "Valid?" column.
					// Draw the icon manually.
					const KeyStoreWin32::Key *key = keyStore->getKey((int)plvcd->nmcd.dwItemSpec);
					assert(key != nullptr);
					if (!key)
						break;

					HICON hDrawIcon = nullptr;
					switch (key->status) {
						case KeyStoreWin32::Key::Status_Unknown:
							// Unknown...
							hDrawIcon = hIconUnknown;
							break;
						case KeyStoreWin32::Key::Status_NotAKey:
							// The key data is not in the correct format.
							hDrawIcon = hIconInvalid;
							break;
						case KeyStoreWin32::Key::Status_Empty:
							// Empty key.
							break;
						case KeyStoreWin32::Key::Status_Incorrect:
							// Key is incorrect.
							hDrawIcon = hIconInvalid;
							break;
						case KeyStoreWin32::Key::Status_OK:
							// Key is correct.
							hDrawIcon = hIconGood;
							break;
					}

					if (!hDrawIcon)
						break;

					const RECT *pRcSubItem = &plvcd->nmcd.rc;
					RECT rectTmp;
					if (pRcSubItem->right == 0 || pRcSubItem->bottom == 0) {
						// Windows XP: plvcd->nmcd.rc isn't initialized.
						// Get the subitem RECT manually.
						// TODO: Increase row height, or decrease icon size?
						// The icon is slightly too big for the default row
						// height on XP.
						BOOL bRet = ListView_GetSubItemRect(hListView, (int)plvcd->nmcd.dwItemSpec, plvcd->iSubItem, LVIR_BOUNDS, &rectTmp);
						if (!bRet)
							break;
						pRcSubItem = &rectTmp;
					}

					// Custom drawing this subitem.
					result = CDRF_SKIPDEFAULT;

					// Set the row background color.
					// TODO: "Disabled" state?
					// NOTE: plvcd->clrTextBk is set to 0xFF000000 here,
					// not the actual default background color.
					HBRUSH hbr;
					if (plvcd->nmcd.uItemState & CDIS_SELECTED) {
						// Row is selected.
						hbr = (HBRUSH)(COLOR_HIGHLIGHT+1);
					} else if (isOdd) {
						// FIXME: On Windows 7:
						// - Standard row colors are 19px high.
						// - Alternate row colors are 17px high. (top and bottom lines ignored?)
						hbr = hbrAltRow;
					} else {
						// Standard row color. Draw it anyway in case
						// the theme was changed, since ListView only
						// partially recognizes theme changes.
						hbr = (HBRUSH)(COLOR_WINDOW+1);
					}
					FillRect(plvcd->nmcd.hdc, pRcSubItem, hbr);

					const int x = pRcSubItem->left + (((pRcSubItem->right - pRcSubItem->left) - szIcon.cx) / 2);
					const int y = pRcSubItem->top + (((pRcSubItem->bottom - pRcSubItem->top) - szIcon.cy) / 2);

					DrawIconEx(plvcd->nmcd.hdc, x, y, hDrawIcon,
						szIcon.cx, szIcon.cy, 0, nullptr, DI_NORMAL);
					break;
				}

				default:
					break;
			}
			break;

		default:
			break;
	}

	return result;
}

/**
 * Load images.
 */
void KeyManagerTabPrivate::loadImages(void)
{
	if (hIconInvalid) {
		// Images are already loaded.
		return;
	}

	// Load the icons.
	// NOTE: Using IDI_* will only return the 32x32 icon.
	// Need to get the icon from USER32 directly.
	HMODULE hUser32 = GetModuleHandle(L"user32");
	assert(hUser32 != nullptr);
	if (hUser32) {
		hIconUnknown = (HICON)LoadImage(hUser32,
			MAKEINTRESOURCE(102), IMAGE_ICON,
			szIcon.cx, szIcon.cy, 0);
		hIconInvalid = (HICON)LoadImage(hUser32,
			MAKEINTRESOURCE(103), IMAGE_ICON,
			szIcon.cx, szIcon.cy, 0);
	}

	// Load hIconGood from our own resource section.
	// Based on KDE Oxygen 5.35.0's base/16x16/actions/dialog-ok-apply.png
	hIconGood = (HICON)LoadImage(HINST_THISCOMPONENT,
		MAKEINTRESOURCE(IDI_KEY_VALID), IMAGE_ICON,
		szIcon.cx, szIcon.cy, 0);
}

/** "Import" menu actions. **/

/**
 * Import keys from Wii keys.bin. (BootMii format)
 */
void KeyManagerTabPrivate::importWiiKeysBin(void)
{
	assert(hWndPropSheet != nullptr);
	if (!hWndPropSheet)
		return;

	// TODO: Does the "Common Item Dialog" support >MAX_PATH?
	wchar_t filename[MAX_PATH];
	filename[0] = 0;

	// tr: Wii keys.bin dialog title.
	const wstring wsDlgTitle = U82W_c(C_("KeyManagerTab", "Select Wii keys.bin File"));
	// tr: Wii keys.bin file filter. (Win32) [Use '|' instead of '\0'! gettext() doesn't support embedded nulls.]
	wstring wsFileFilter = U82W_c(C_("KeyManagerTab", "keys.bin|keys.bin|Binary Files (*.bin)|*.bin|All Files (*.*)|*.*||"));
	convertPipesToNulls(wsFileFilter);

	OPENFILENAME ofn;
	memset(&ofn, 0, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = hWndPropSheet;
	ofn.lpstrFilter = wsFileFilter.c_str();
	ofn.lpstrCustomFilter = nullptr;
	ofn.lpstrFile = filename;
	ofn.nMaxFile = ARRAY_SIZE(filename);
	ofn.lpstrTitle = wsDlgTitle.c_str();
	ofn.Flags = OFN_DONTADDTORECENT | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;

	BOOL bRet = GetOpenFileName(&ofn);
	if (!bRet || filename[0] == 0)
		return;

	KeyStoreWin32::ImportReturn iret = keyStore->importWiiKeysBin(W2U8(filename).c_str());
	// TODO: Port showKeyImportReturnStatus from the KDE version.
	//d->showKeyImportReturnStatus(filename, L"Wii keys.bin", iret);
}

/**
 * Import keys from Wii U otp.bin.
 */
void KeyManagerTabPrivate::importWiiUOtpBin(void)
{
	assert(hWndPropSheet != nullptr);
	if (!hWndPropSheet)
		return;

	// TODO: Does the "Common Item Dialog" support >MAX_PATH?
	wchar_t filename[MAX_PATH];
	filename[0] = 0;

	// tr: Wii U otp.bin dialog title.
	const wstring wsDlgTitle = U82W_c(C_("KeyManagerTab", "Select Wii U otp.bin File"));
	// tr: Wii U otp.bin file filter. (Win32) [Use '|' instead of '\0'! gettext() doesn't support embedded nulls.]
	wstring wsFileFilter = U82W_c(C_("KeyManagerTab", "otp.bin|otp.bin|Binary Files (*.bin)|*.bin|All Files (*.*)|*.*||"));
	convertPipesToNulls(wsFileFilter);

	OPENFILENAME ofn;
	memset(&ofn, 0, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = hWndPropSheet;
	ofn.lpstrFilter = wsFileFilter.c_str();
	ofn.lpstrCustomFilter = nullptr;
	ofn.lpstrFile = filename;
	ofn.nMaxFile = ARRAY_SIZE(filename);
	ofn.lpstrTitle = wsDlgTitle.c_str();
	ofn.Flags = OFN_DONTADDTORECENT | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;

	BOOL bRet = GetOpenFileName(&ofn);
	if (!bRet || filename[0] == 0)
		return;

	KeyStoreWin32::ImportReturn iret = keyStore->importWiiUOtpBin(W2U8(filename).c_str());
	// TODO: Port showKeyImportReturnStatus from the KDE version.
	//d->showKeyImportReturnStatus(filename, L"Wii U otp.bin", iret);
}

/**
 * Import keys from 3DS boot9.bin.
 */
void KeyManagerTabPrivate::import3DSboot9bin(void)
{
	assert(hWndPropSheet != nullptr);
	if (!hWndPropSheet)
		return;

	// TODO: Does the "Common Item Dialog" support >MAX_PATH?
	wchar_t filename[MAX_PATH];
	filename[0] = 0;

	// tr: 3DS boot9.bin dialog title.
	const wstring wsDlgTitle = U82W_c(C_("KeyManagerTab", "Select 3DS boot9.bin File"));
	// tr: 3DS boot9.bin file filter. (Win32) [Use '|' instead of '\0'! gettext() doesn't support embedded nulls.]
	wstring wsFileFilter = U82W_c(C_("KeyManagerTab", "boot9.bin|boot9.bin|Binary Files (*.bin)|*.bin|All Files (*.*)|*.*||"));
	convertPipesToNulls(wsFileFilter);

	OPENFILENAME ofn;
	memset(&ofn, 0, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = hWndPropSheet;
	ofn.lpstrFilter = wsFileFilter.c_str();
	ofn.lpstrCustomFilter = nullptr;
	ofn.lpstrFile = filename;
	ofn.nMaxFile = ARRAY_SIZE(filename);
	ofn.lpstrTitle = wsDlgTitle.c_str();
	ofn.Flags = OFN_DONTADDTORECENT | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;

	BOOL bRet = GetOpenFileName(&ofn);
	if (!bRet || filename[0] == 0)
		return;

	KeyStoreWin32::ImportReturn iret = keyStore->import3DSboot9bin(W2U8(filename).c_str());
	// TODO: Port showKeyImportReturnStatus from the KDE version.
	//d->showKeyImportReturnStatus(filename, L"3DS boot9.bin", iret);
}

/**
 * Import keys from 3DS aeskeydb.bin.
 */
void KeyManagerTabPrivate::import3DSaeskeydb(void)
{
	assert(hWndPropSheet != nullptr);
	if (!hWndPropSheet)
		return;

	// TODO: Does the "Common Item Dialog" support >MAX_PATH?
	wchar_t filename[MAX_PATH];
	filename[0] = 0;

	// tr: aeskeydb.bin dialog title.
	const wstring wsDlgTitle = U82W_c(C_("KeyManagerTab", "Select 3DS aeskeydb.bin File"));
	// tr: aeskeydb.bin file filter. (Win32) [Use '|' instead of '\0'! gettext() doesn't support embedded nulls.]
	wstring wsFileFilter = U82W_c(C_("KeyManagerTab", "aeskeydb.bin|aeskeydb.bin|Binary Files (*.bin)|*.bin|All Files (*.*)|*.*||"));
	convertPipesToNulls(wsFileFilter);

	OPENFILENAME ofn;
	memset(&ofn, 0, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = hWndPropSheet;
	ofn.lpstrFilter = wsFileFilter.c_str();
	ofn.lpstrCustomFilter = nullptr;
	ofn.lpstrFile = filename;
	ofn.nMaxFile = ARRAY_SIZE(filename);
	ofn.lpstrTitle = wsDlgTitle.c_str();
	ofn.Flags = OFN_DONTADDTORECENT | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;

	BOOL bRet = GetOpenFileName(&ofn);
	if (!bRet || filename[0] == 0)
		return;

	KeyStoreWin32::ImportReturn iret = keyStore->import3DSaeskeydb(W2U8(filename).c_str());
	// TODO: Port showKeyImportReturnStatus from the KDE version.
	//d->showKeyImportReturnStatus(filename, L"3DS aeskeydb.bin", iret);
}

/** KeyManagerTab **/

KeyManagerTab::KeyManagerTab(void)
	: d_ptr(new KeyManagerTabPrivate())
{ }

KeyManagerTab::~KeyManagerTab()
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
HPROPSHEETPAGE KeyManagerTab::getHPropSheetPage(void)
{
	RP_D(KeyManagerTab);
	assert(d->hPropSheetPage == nullptr);
	if (d->hPropSheetPage) {
		// Property sheet has already been created.
		return nullptr;
	}

	// tr: Tab title.
	const wstring wsTabTitle = U82W_c(C_("KeyManagerTab", "Key Manager"));

	PROPSHEETPAGE psp;
	psp.dwSize = sizeof(psp);
	psp.dwFlags = PSP_USECALLBACK | PSP_USETITLE;
	psp.hInstance = HINST_THISCOMPONENT;
	psp.pszTemplate = MAKEINTRESOURCE(IDD_CONFIG_KEYMANAGER);
	psp.pszIcon = nullptr;
	psp.pszTitle = wsTabTitle.c_str();
	psp.pfnDlgProc = KeyManagerTabPrivate::dlgProc;
	psp.lParam = reinterpret_cast<LPARAM>(d);
	psp.pcRefParent = nullptr;
	psp.pfnCallback = KeyManagerTabPrivate::callbackProc;

	d->hPropSheetPage = CreatePropertySheetPage(&psp);
	return d->hPropSheetPage;
}

/**
 * Reset the contents of this tab.
 */
void KeyManagerTab::reset(void)
{
	RP_D(KeyManagerTab);
	d->reset();
}

/**
 * Load the default configuration.
 * This does NOT save, and will only emit modified()
 * if it's different from the current configuration.
 */
void KeyManagerTab::loadDefaults(void)
{
	// Not implemented for this tab.
}

/**
 * Save the contents of this tab.
 */
void KeyManagerTab::save(void)
{
	RP_D(KeyManagerTab);
	d->save();
}
