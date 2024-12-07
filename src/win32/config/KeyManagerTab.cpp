/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * KeyManagerTab.hpp: Key Manager tab for rp-config.                       *
 *                                                                         *
 * Copyright (c) 2016-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "KeyManagerTab.hpp"
#include "res/resource.h"
#include "../FontHandler.hpp"
#include "../MessageWidget.hpp"

// KeyStore
#include "KeyStoreWin32.hpp"

// IListView and other undocumented stuff.
#include "libwin32common/sdk/IListView.hpp"
#include "KeyStore_OwnerDataCallback.hpp"

// libwin32common
#include "libwin32common/rp_versionhelpers.h"

// libwin32ui
#include "libwin32ui/LoadResource_i18n.hpp"
using LibWin32UI::LoadDialog_i18n;
using LibWin32UI::WTSSessionNotification;

// Win32 dark mode
#include "libwin32darkmode/DarkMode.hpp"
#include "libwin32darkmode/DarkModeCtrl.hpp"
#include "libwin32darkmode/ListViewUtil.hpp"

// Other rom-properties libraries
#include "librpbase/crypto/KeyManager.hpp"
using namespace LibRpBase;
using namespace LibRpFile;
using namespace LibRpText;

// libromdata
using namespace LibRomData;

// C++ STL classes
using std::array;
using std::locale;
using std::ostringstream;
using std::string;
using std::wostringstream;
using std::wstring;

// KeyStoreUI::ImportFileID
static const array<LPCTSTR, 4> import_menu_actions = {{
	_T("Wii keys.bin"),
	_T("Wii U otp.bin"),
	_T("3DS boot9.bin"),
	_T("3DS aeskeydb.bin"),
}};

static constexpr array<uint16_t, 4> import_menu_actions_ids = {{
	IDM_KEYMANAGER_IMPORT_WII_KEYS_BIN,
	IDM_KEYMANAGER_IMPORT_WIIU_OTP_BIN,
	IDM_KEYMANAGER_IMPORT_3DS_BOOT9_BIN,
	IDM_KEYMANAGER_IMPORT_3DS_AESKEYDB,
}};

class KeyManagerTabPrivate
{
public:
	KeyManagerTabPrivate();
	~KeyManagerTabPrivate();

private:
	RP_DISABLE_COPY(KeyManagerTabPrivate)

public:
	/**
	 * Initialize the dialog.
	 */
	void initDialog(void);

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
	// "Import" popup menu
	HMENU hMenuImport;	// Must be deleted using DestroyMenu().

	// KeyStore
	KeyStoreWin32 *keyStore;
	KeyStore_OwnerDataCallback *keyStore_ownerDataCallback;

	// Font Handler
	FontHandler fontHandler;

	// wtsapi32.dll for Remote Desktop status. (WinXP and later)
	WTSSessionNotification wts;

	// MessageWidget for ROM operation notifications.
	HWND hMessageWidget;
	POINT ptListView;	// Original ListView position
	SIZE szListView;	// Original ListView size

	// EDIT box for ListView.
	HWND hEditBox;
	int iEditItem;		// Item being edited. (-1 for none)
	bool bCancelEdit;	// True if the edit is being cancelled.
	bool bAllowKanji;	// Allow kanji in the editor.

	bool isComCtl32_v610;	// Is this COMCTL32.dll v6.10 or later?

	// Icons for the "Valid?" column.
	// NOTE: "?" and "X" are copies from User32.
	// Checkmark is a PNG image loaded from a resource.
	int iconSize;		// NOTE: Needs to be SIGNED to prevent issues with negative coordinates.
	HICON hIconUnknown;	// "?" (USER32.dll,-102)
	HICON hIconInvalid;	// "X" (USER32.dll,-103)
	HICON hIconGood;	// Checkmark

	// Alternate row color
	COLORREF colorAltRow;
	HBRUSH hbrAltRow;

public:
	/**
	 * Update the ListView style.
	 */
	void updateListViewStyle(void);

	/**
	 * ListView GetDispInfo function.
	 * @param plvdi	[in/out] NMLVDISPINFO
	 * @return True if handled; false if not.
	 */
	inline BOOL ListView_GetDispInfo(NMLVDISPINFO *plvdi);

	/**
	 * ListView CustomDraw function.
	 * @param plvcd	[in/out] NMLVCUSTOMDRAW
	 * @return Return value.
	 */
	inline int ListView_CustomDraw(NMLVCUSTOMDRAW *plvcd);

	/**
	 * Load images.
	 */
	void loadImages(void);

public:
	/** "Import" menu actions **/

	// Starting directory for importing keys.
	// TODO: Save this in the configuration file?
	tstring ts_keyFileDir;

	/**
	 * Update ts_keyFileDir.
	 * @param tfilename New filename.
	 */
	inline void updateKeyFileDir(const tstring &tfilename)
	{
		// Remove everything after the first backslash.
		// NOTE: If this is the root directory, the backslash is left intact.
		// Otherwise, the backslash is removed.
		ts_keyFileDir = tfilename;
		const size_t bspos = ts_keyFileDir.rfind(_T('\\'));
		if (bspos != string::npos) {
			if (bspos > 2) {
				ts_keyFileDir.resize(bspos);
			} else if (bspos == 2) {
				ts_keyFileDir.resize(3);
			}
		}
	}

	/**
	 * Show key import return status.
	 * @param filename Filename
	 * @param keyType Key type
	 * @param iret ImportReturn
	 */
	void showKeyImportReturnStatus(const tstring &filename,
		const TCHAR *keyType, const KeyStoreUI::ImportReturn &iret);

	/**
	 * Import keys from a binary file.
	 * @param fileID Type of file
	 */
	void importKeysFromBin(KeyStoreUI::ImportFileID id);

public:
	// Dark Mode background brush
	HBRUSH hbrBkgnd;
	bool lastDarkModeEnabled;
};

/** KeyManagerTabPrivate **/

KeyManagerTabPrivate::KeyManagerTabPrivate()
	: hPropSheetPage(nullptr)
	, hWndPropSheet(nullptr)
	, hMenuImport(nullptr)
	, keyStore(new KeyStoreWin32(nullptr))
	, keyStore_ownerDataCallback(nullptr)
	, fontHandler(nullptr)
	, hMessageWidget(nullptr)
	, hEditBox(nullptr)
	, iEditItem(-1)
	, bCancelEdit(false)
	, bAllowKanji(false)
	, iconSize(0)
	, hIconUnknown(nullptr)
	, hIconInvalid(nullptr)
	, hIconGood(nullptr)
	, colorAltRow(0)
	, hbrAltRow(nullptr)
	, hbrBkgnd(nullptr)
	, lastDarkModeEnabled(false)
{
	// Check the COMCTL32.DLL version.
	isComCtl32_v610 = LibWin32UI::isComCtl32_v610();
}

KeyManagerTabPrivate::~KeyManagerTabPrivate()
{
	if (hMenuImport) {
		DestroyMenu(hMenuImport);
	}

	delete keyStore;
	if (keyStore_ownerDataCallback) {
		keyStore_ownerDataCallback->Release();
	}

	// Icons
	if (hIconUnknown) {
		DestroyIcon(hIconUnknown);
	}
	if (hIconInvalid) {
		DestroyIcon(hIconInvalid);
	}
	if (hIconGood) {
		DestroyIcon(hIconGood);
	}

	// Alternate row color
	if (hbrAltRow) {
		DeleteBrush(hbrAltRow);
	}

	// Dark mode background brush
	if (hbrBkgnd) {
		DeleteBrush(hbrBkgnd);
	}
}

/**
 * Initialize the dialog.
 */
void KeyManagerTabPrivate::initDialog(void)
{
	assert(hWndPropSheet != nullptr);
	if (!hWndPropSheet)
		return;

	// Initialize the fonts.
	fontHandler.setWindow(hWndPropSheet);

	// Get the required controls.
	HWND hBtnImport = GetDlgItem(hWndPropSheet, IDC_KEYMANAGER_IMPORT);
	HWND hListView = GetDlgItem(hWndPropSheet, IDC_KEYMANAGER_LIST);
	assert(hBtnImport != nullptr);
	assert(hListView != nullptr);
	if (!hBtnImport || !hListView)
		return;

	if (isComCtl32_v610) {
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
		SetWindowText(hBtnImport, TC_("KeyManagerTab", "I&mport..."));
	}

	// Ensure the images are loaded before initializing the ListView.
	// NOTE: The ListView control is created at this point, which is
	// required by loadImages() in order to determine the DPI.
	loadImages();

	// Initialize the ListView.

	// Set the virtual list item count.
	ListView_SetItemCountEx(hListView, keyStore->totalKeyCount(),
		LVSICF_NOINVALIDATEALL | LVSICF_NOSCROLL);

	// Column title
	tstring tsColTitle;

	// tr: Column 0: Key Name
	tsColTitle = TC_("KeyManagerTab", "Key Name");
	LVCOLUMN lvColumn;
	lvColumn.mask = LVCF_FMT | LVCF_TEXT | LVCF_SUBITEM;
	lvColumn.fmt = LVCFMT_LEFT;
	lvColumn.pszText = const_cast<LPTSTR>(tsColTitle.c_str());
	lvColumn.iItem = 0;
	lvColumn.iSubItem = 0;
	ListView_InsertColumn(hListView, 0, &lvColumn);

	// tr: Column 1: Value
	tsColTitle = TC_("KeyManagerTab", "Value");
	lvColumn.pszText = const_cast<LPTSTR>(tsColTitle.c_str());
	lvColumn.iSubItem = 1;
	ListView_InsertColumn(hListView, 1, &lvColumn);

	// tr: Column 2: Verification status
	tsColTitle = TC_("KeyManagerTab", "Valid?");
	lvColumn.pszText = const_cast<LPTSTR>(tsColTitle.c_str());
	lvColumn.iSubItem = 2;
	ListView_InsertColumn(hListView, 2, &lvColumn);

	if (isComCtl32_v610) {
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
			// NOTE: LVGROUP always uses Unicode strings.
#if _WIN32_WINNT < 0x0600
# error Windows Vista SDK or later is required.
#endif
			LVGROUP lvGroup;
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
	static constexpr array<int, 2> column_padding = {{8, 12}};
	array<int, 2> column_width = {{0, 0}};

	// Make sure the "Value" column is at least 32 characters wide.
	// NOTE: ListView_GetStringWidth() doesn't adjust for the monospaced font.
	SIZE szValue;
	HFONT hFontMono = fontHandler.monospacedFont();
	int ret = LibWin32UI::measureTextSize(hListView, hFontMono, _T("0123456789ABCDEF0123456789ABCDEF"), &szValue);
	assert(ret == 0);
	if (ret == 0) {
		column_width[1] = szValue.cx + column_padding[1];
	}
	//column_width[1] = ListView_GetStringWidth(hListView, _T("0123456789ABCDEF0123456789ABCDEF")) + column_padding[1];

	for (int i = keyStore->totalKeyCount()-1; i >= 0; i--) {
		const KeyStoreWin32::Key *key = keyStore->getKey(i);
		assert(key != nullptr);
		if (!key)
			continue;

		int tmp_width[2];
		tmp_width[0] = ListView_GetStringWidth(hListView, U82T_s(key->name)) + column_padding[0];
		//tmp_width[1] = ListView_GetStringWidth(hListView, U82T_s(key->value)) + column_padding[1];

		column_width[0] = std::max(column_width[0], tmp_width[0]);
		//column_width[1] = std::max(column_width[1], tmp_width[1]);

		ret = LibWin32UI::measureTextSize(hListView, hFontMono, U82T_s(key->value), &szValue);
		assert(ret == 0);
		if (ret == 0) {
			column_width[1] = std::max(column_width[1], (int)szValue.cx + column_padding[1]);
		}
	}
	ListView_SetColumnWidth(hListView, 0, column_width[0]);
	ListView_SetColumnWidth(hListView, 1, column_width[1]);

	// Auto-size the "Valid?" column.
	ListView_SetColumnWidth(hListView, 2, LVSCW_AUTOSIZE_USEHEADER);

	// Get the ListView's initial position and size.
	// This will be needed to adjust the ListView when
	// displaying the MessageWidget.
	RECT rectListView;
	GetWindowRect(hListView, &rectListView);
	MapWindowPoints(HWND_DESKTOP, hWndPropSheet, (LPPOINT)&rectListView, 2);
	ptListView.x = rectListView.left;
	ptListView.y = rectListView.top;
	szListView.cx = rectListView.right - rectListView.left;
	szListView.cy = rectListView.bottom - rectListView.top;

	// Create the EDIT box.
	hEditBox = CreateWindowEx(WS_EX_LEFT,
		WC_EDIT, nullptr,
		WS_CHILDWINDOW | WS_BORDER | ES_LEFT | ES_AUTOHSCROLL | ES_UPPERCASE | ES_WANTRETURN,
		0, 0, 0, 0,
		hListView, (HMENU)IDC_KEYMANAGER_EDIT, nullptr, nullptr);
	fontHandler.addMonoControl(hEditBox);
	SetWindowSubclass(hEditBox, ListViewEditSubclassProc,
		IDC_KEYMANAGER_EDIT, reinterpret_cast<DWORD_PTR>(this));

	// Set the KeyStore's window.
	keyStore->setHWnd(hWndPropSheet);

	// Set window themes for Win10's dark mode.
	// NOTE: This must be done before subclassing the ListView
	// because this initializes the alternate row color and brush.
	if (g_darkModeSupported) {
		DarkMode_InitButton(hBtnImport);
		DarkMode_InitEdit(hEditBox);

		// Initialize Dark Mode in the ListView.
		DarkMode_InitListView(hListView);
	}

	// Update the ListView style.
	updateListViewStyle();

	// Subclass the ListView.
	// TODO: Error handling?
	SetWindowSubclass(hListView, ListViewSubclassProc,
		IDC_KEYMANAGER_LIST, reinterpret_cast<DWORD_PTR>(this));

	// Register for WTS session notifications. (Remote Desktop)
	wts.registerSessionNotification(hWndPropSheet, NOTIFY_FOR_THIS_SESSION);

	// Reset the configuration.
	reset();
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

	// Make sure the configuration directory exists.
	// NOTE: The filename portion MUST be kept in config_path,
	// since the last component is ignored by rmkdir().
	int ret = FileSystem::rmkdir(filename);
	if (ret != 0) {
		// rmkdir() failed.
		return;
	}

	// Save the keys.
	const tstring tfilename = U82T_c(filename);
	const int totalKeyCount = keyStore->totalKeyCount();
	for (int i = 0; i < totalKeyCount; i++) {
		const KeyStoreWin32::Key *const pKey = keyStore->getKey(i);
		assert(pKey != nullptr);
		if (!pKey || !pKey->modified)
			continue;

		// Save this key.
		WritePrivateProfileString(_T("Keys"), U82T_s(pKey->name), U82T_s(pKey->value), tfilename.c_str());
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
				return true;

			// Get the pointer to the KeyManagerTabPrivate object.
			KeyManagerTabPrivate *const d = reinterpret_cast<KeyManagerTabPrivate*>(pPage->lParam);
			if (!d)
				return true;

			assert(d->hWndPropSheet == nullptr);
			d->hWndPropSheet = hDlg;

			// Store the D object pointer with this particular page dialog.
			SetWindowLongPtr(hDlg, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(d));

			// NOTE: This should be in WM_CREATE, but we don't receive WM_CREATE here.
			DarkMode_InitDialog(hDlg);
			d->lastDarkModeEnabled = g_darkModeEnabled;

			// Initialize the dialog.
			d->initDialog();
			return true;
		}

		case WM_NOTIFY: {
			auto *const d = reinterpret_cast<KeyManagerTabPrivate*>(GetWindowLongPtr(hDlg, GWLP_USERDATA));
			if (!d) {
				// No KeyManagerTabPrivate. Can't do anything...
				return false;
			}

			NMHDR *const pHdr = reinterpret_cast<NMHDR*>(lParam);
			switch (pHdr->code) {
				case PSN_APPLY:
					// Save settings.
					d->save();
					break;

				case PSN_SETACTIVE:
					// Disable the "Defaults" button.
					RpPropSheet_EnableDefaults(GetParent(hDlg), false);
					break;

				case LVN_GETDISPINFO: {
					// Get data for an LVS_OWNERDRAW ListView.
					if (!d->keyStore || pHdr->idFrom != IDC_KEYMANAGER_LIST)
						break;

					return d->ListView_GetDispInfo(reinterpret_cast<NMLVDISPINFO*>(lParam));
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
					const int result = d->ListView_CustomDraw(reinterpret_cast<NMLVCUSTOMDRAW*>(pHdr));
					SetWindowLongPtr(hDlg, DWLP_MSGRESULT, result);
					return true;
				}

				case MSGWN_CLOSED: {
					// MessageWidget's Close button was pressed.
					HWND hListView = GetDlgItem(hDlg, IDC_KEYMANAGER_LIST);
					assert(hListView != nullptr);
					SetWindowPos(hListView, nullptr,
						d->ptListView.x, d->ptListView.y,
						d->szListView.cx, d->szListView.cy,
						SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOZORDER);
					break;
				}

				default:
					break;
			}
			break;
		}

		case WM_COMMAND: {
			auto *const d = reinterpret_cast<KeyManagerTabPrivate*>(GetWindowLongPtr(hDlg, GWLP_USERDATA));
			if (!d) {
				// No KeyManagerTabPrivate. Can't do anything...
				return false;
			}

			switch (LOWORD(wParam)) {
				case IDC_KEYMANAGER_IMPORT: {
					// Show the "Import" popup menu.
					if (!d->hMenuImport) {
						d->hMenuImport = CreatePopupMenu();
						if (!d->hMenuImport) {
							// Unable to create the "Import" popup menu.
							return true;
						}
						for (unsigned int i = 0; i < (unsigned int)ARRAY_SIZE(import_menu_actions); i++) {
							AppendMenu(d->hMenuImport, MF_STRING, import_menu_actions_ids[i], import_menu_actions[i]);
						}
					}

					RECT btnRect;
					GetWindowRect(GetDlgItem(hDlg, IDC_KEYMANAGER_IMPORT), &btnRect);
					TrackPopupMenu(d->hMenuImport, TPM_LEFTALIGN|TPM_BOTTOMALIGN,
						btnRect.left, btnRect.top, 0, hDlg, nullptr);
					return true;
				}

				case IDM_KEYMANAGER_IMPORT_WII_KEYS_BIN:
					d->importKeysFromBin(KeyStoreUI::ImportFileID::WiiKeysBin);
					return true;
				case IDM_KEYMANAGER_IMPORT_WIIU_OTP_BIN:
					d->importKeysFromBin(KeyStoreUI::ImportFileID::WiiUOtpBin);
					return true;
				case IDM_KEYMANAGER_IMPORT_3DS_BOOT9_BIN:
					d->importKeysFromBin(KeyStoreUI::ImportFileID::N3DSboot9bin);
					break;
				case IDM_KEYMANAGER_IMPORT_3DS_AESKEYDB:
					d->importKeysFromBin(KeyStoreUI::ImportFileID::N3DSaeskeydb);
					break;

				default:
					break;
			}
			break;
		}

		case WM_RP_PROP_SHEET_RESET: {
			auto *const d = reinterpret_cast<KeyManagerTabPrivate*>(GetWindowLongPtr(hDlg, GWLP_USERDATA));
			if (!d) {
				// No KeyManagerTabPrivate. Can't do anything...
				return false;
			}

			// Reset the tab.
			d->reset();
			break;
		}

		case WM_NCPAINT: {
			// Update the monospaced font.
			// NOTE: This should be WM_SETTINGCHANGE with
			// SPI_GETFONTSMOOTHING or SPI_GETFONTSMOOTHINGTYPE,
			// but that message isn't sent when previewing changes
			// for ClearType. (It's sent when applying the changes.)
			auto *const d = reinterpret_cast<KeyManagerTabPrivate*>(GetWindowLongPtr(hDlg, GWLP_USERDATA));
			if (d) {
				// Update the fonts.
				d->fontHandler.updateFonts();
			}
			break;
		}

		case WM_KEYSTORE_KEYCHANGED_IDX: {
			auto *const d = reinterpret_cast<KeyManagerTabPrivate*>(GetWindowLongPtr(hDlg, GWLP_USERDATA));
			if (!d) {
				// No KeyManagerTabPrivate. Can't do anything...
				return false;
			}

			// Update the row.
			HWND hListView = GetDlgItem(d->hWndPropSheet, IDC_KEYMANAGER_LIST);
			assert(hListView != nullptr);
			if (hListView) {
				ListView_RedrawItems(hListView, (int)lParam, (int)lParam);
			}
			return true;
		}

		case WM_KEYSTORE_ALLKEYSCHANGED: {
			auto *const d = reinterpret_cast<KeyManagerTabPrivate*>(GetWindowLongPtr(hDlg, GWLP_USERDATA));
			if (!d) {
				// No KeyManagerTabPrivate. Can't do anything...
				return false;
			}

			// Update all rows.
			HWND hListView = GetDlgItem(d->hWndPropSheet, IDC_KEYMANAGER_LIST);
			assert(hListView != nullptr);
			if (hListView) {
				ListView_RedrawItems(hListView, 0, d->keyStore->totalKeyCount()-1);
			}
			return true;
		}

		case WM_KEYSTORE_MODIFIED: {
			auto *const d = reinterpret_cast<KeyManagerTabPrivate*>(GetWindowLongPtr(hDlg, GWLP_USERDATA));
			if (!d) {
				// No KeyManagerTabPrivate. Can't do anything...
				return false;
			}

			// Key was modified.
			PropSheet_Changed(GetParent(hDlg), hDlg);
			return true;
		}

		case WM_WTSSESSION_CHANGE: {
			auto *const d = reinterpret_cast<KeyManagerTabPrivate*>(GetWindowLongPtr(hDlg, GWLP_USERDATA));
			if (!d) {
				// No KeyManagerTabPrivate. Can't do anything...
				return false;
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

		case WM_DPICHANGED: {
			auto *const d = reinterpret_cast<KeyManagerTabPrivate*>(GetWindowLongPtr(hDlg, GWLP_USERDATA));
			if (!d) {
				// No KeyManagerTabPrivate. Can't do anything...
				return false;
			}

			// TODO: Verify that this works. (Might be top-level only?)
			d->loadImages();
			break;
		}

		case WM_SYSCOLORCHANGE: {
			auto *const d = reinterpret_cast<KeyManagerTabPrivate*>(GetWindowLongPtr(hDlg, GWLP_USERDATA));
			if (!d) {
				// No KeyManagerTabPrivate. Can't do anything...
				return false;
			}

			// Update the fonts. (TODO: Might not be needed here?)
			if (d->fontHandler.window()) {
				d->fontHandler.updateFonts(true);
			}
			// Update the ListView style.
			d->updateListViewStyle();
			break;
		}

		/** Dark Mode **/

		case WM_CTLCOLORDLG:
		case WM_CTLCOLORSTATIC:
			if (g_darkModeEnabled) {
				auto *const d = reinterpret_cast<KeyManagerTabPrivate*>(GetWindowLongPtr(hDlg, GWLP_USERDATA));
				if (!d) {
					// No KeyManagerTabPrivate. Can't do anything...
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
			auto *const d = reinterpret_cast<KeyManagerTabPrivate*>(GetWindowLongPtr(hDlg, GWLP_USERDATA));
			if (!d) {
				// No KeyManagerTabPrivate. Can't do anything...
				return FALSE;
			}

			if (g_darkModeSupported) {
				UpdateDarkModeEnabled();
				if (d->lastDarkModeEnabled != g_darkModeEnabled) {
					d->lastDarkModeEnabled = g_darkModeEnabled;
					InvalidateRect(hDlg, NULL, true);

					// Propagate WM_THEMECHANGED to window controls that don't
					// automatically handle Dark Mode changes, e.g. ComboBox and Button.
					SendMessage(GetDlgItem(hDlg, IDC_KEYMANAGER_LIST), WM_THEMECHANGED, 0, 0);
					SendMessage(GetDlgItem(hDlg, IDC_KEYMANAGER_IMPORT), WM_THEMECHANGED, 0, 0);
				}
			}

			// Update the fonts.
			if (d->fontHandler.window()) {
				d->fontHandler.updateFonts(true);
			}
			// Update the ListView style.
			d->updateListViewStyle();
			break;
		}

		default:
			break;
	}

	return false; // Let system deal with other messages
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
	RP_UNUSED(hWnd);
	RP_UNUSED(ppsp);

	switch (uMsg) {
		case PSPCB_CREATE: {
			// Must return true to enable the page to be created.
			return true;
		}

		case PSPCB_RELEASE: {
			// TODO: Do something here?
			break;
		}

		default:
			break;
	}

	return false;
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
				return false;

			// Check for a double-click in the ListView.
			// ListView only directly supports editing of the
			// first column, so we have to handle it manually
			// for the second column (Value).
			LVHITTESTINFO lvhti;
			lvhti.pt.x = GET_X_LPARAM(lParam);
			lvhti.pt.y = GET_Y_LPARAM(lParam);

			// Check if this point maps to a valid "Value" subitem.
			const int iItem = ListView_SubItemHitTest(hWnd, &lvhti);
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
			TCHAR szItemText[128];
			ListView_GetItemText(hWnd, iItem, lvhti.iSubItem, szItemText, _countof(szItemText));
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
			return true;
		}

		case WM_NCDESTROY:
			// Remove the window subclass.
			// Reference: https://devblogs.microsoft.com/oldnewthing/20031111-00/?p=41883
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
		return false;

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
			TCHAR tbuf[128];
			tbuf[0] = 0;
			GetWindowText(hWnd, tbuf, _countof(tbuf));
			d->keyStore->setKey(d->iEditItem, T2U8(tbuf).c_str());

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
					return true;
				case VK_ESCAPE:
					// Cancel editing.
					d->bCancelEdit = true;
					ShowWindow(hWnd, SW_HIDE);
					return true;
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
			return true;
		}

		case WM_KEYDOWN:
		case WM_KEYUP: {
			// Reference: https://support.microsoft.com/en-us/help/102589/how-to-use-the-enter-key-from-edit-controls-in-a-dialog-box
			switch (wParam) {
				case VK_RETURN:
					// Finished editing.
					d->bCancelEdit = false;
					ShowWindow(hWnd, SW_HIDE);
					return true;
				case VK_ESCAPE:
					// Cancel editing.
					d->bCancelEdit = true;
					ShowWindow(hWnd, SW_HIDE);
					return true;
				default:
					break;
			}
			break;
		}

		case WM_PASTE: {
			// Filter out text pasted in from the clipboard.
			// Reference: https://stackoverflow.com/questions/22263612/properly-handle-wm-paste-in-subclass-procedure
			if (!OpenClipboard(hWnd))
				return true;

			HANDLE hClipboardData = GetClipboardData(CF_UNICODETEXT);
			if (!hClipboardData) {
				CloseClipboard();
				return true;
			}

			const TCHAR *const pchData = static_cast<const TCHAR*>(GlobalLock(hClipboardData));
			if (!pchData) {
				// No data.
				CloseClipboard();
				return true;
			} else if (pchData[0] == 0) {
				// Empty string.
				// TODO: Paste anyway?
				GlobalUnlock(hClipboardData);
				CloseClipboard();
				return true;
			}

			// Filter out invalid characters.
			tstring tstr;
			tstr.reserve(_tcslen(pchData));
			for (const TCHAR *p = pchData; *p != 0; p++) {
				// Allow hexadecimal digits.
				if (_istxdigit(*p)) {
					tstr += *p;
				} else if (d->bAllowKanji) {
					// Allow kanji.
					// Reference: http://www.localizingjapan.com/blog/2012/01/20/regular-expressions-for-japanese-text/
					if ((*p >= 0x3400 && *p <= 0x4DB5) ||
					    (*p >= 0x4E00 && *p <= 0x9FCB) ||
					    (*p >= 0xF900 && *p <= 0xFA6A))
					{
						tstr += *p;
					} else {
						// Invalid character.
						// Prevent the paste.
						GlobalUnlock(hClipboardData);
						CloseClipboard();
						return true;
					}
				} else {
					// Invalid character.
					// Prevent the paste.
					GlobalUnlock(hClipboardData);
					CloseClipboard();
					return true;
				}
			}

			GlobalUnlock(hClipboardData);
			CloseClipboard();

			if (!tstr.empty()) {
				// Insert the text.
				// TODO: Paste even if empty?
				Edit_ReplaceSel(hWnd, tstr.c_str());
			}
			return true;
		}

		case WM_NCDESTROY:
			// Remove the window subclass.
			// Reference: https://devblogs.microsoft.com/oldnewthing/20031111-00/?p=41883
			RemoveWindowSubclass(hWnd, ListViewSubclassProc, uIdSubclass);
			break;

		default:
			break;
	}

	return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

/**
 * Update the ListView style.
 */
void KeyManagerTabPrivate::updateListViewStyle(void)
{
	HWND hListView = GetDlgItem(hWndPropSheet, IDC_KEYMANAGER_LIST);
	assert(hListView != nullptr);
	if (!hListView)
		return;

	// Set extended ListView styles.
	// Double-buffering is enabled if using RDP or RemoteFX.
	const DWORD lvsExStyle = likely(!GetSystemMetrics(SM_REMOTESESSION))
		? LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER
		: LVS_EX_FULLROWSELECT;
	ListView_SetExtendedListViewStyle(hListView, lvsExStyle);

	// Update the alternate row color.
	colorAltRow = LibWin32UI::ListView_GetBkColor_AltRow(hListView);
	if (hbrAltRow) {
		DeleteBrush(hbrAltRow);
	}
	hbrAltRow = CreateSolidBrush(colorAltRow);
}

/**
 * ListView GetDispInfo function.
 * @param plvdi	[in/out] NMLVDISPINFO
 * @return True if handled; false if not.
 */
inline BOOL KeyManagerTabPrivate::ListView_GetDispInfo(NMLVDISPINFO *plvdi)
{
	LVITEM *const plvItem = &plvdi->item;
	if (plvItem->iItem < 0 || plvItem->iItem >= keyStore->totalKeyCount()) {
		// Index is out of range.
		return false;
	}

	const KeyStoreWin32::Key *const key = keyStore->getKey(plvItem->iItem);
	if (!key) {
		// No key...
		return false;
	}

	if (plvItem->mask & LVIF_TEXT) {
		// Fill in text.
		switch (plvItem->iSubItem) {
			case 0:
				// Key name.
				_tcscpy_s(plvItem->pszText, plvItem->cchTextMax, U82T_s(key->name));
				return true;
			case 1:
				// Value.
				_tcscpy_s(plvItem->pszText, plvItem->cchTextMax, U82T_s(key->value));
				return true;
			default:
				// No text for "Valid?".
				plvItem->pszText[0] = 0;
				return true;
		}
	}

	// Nothing to do here...
	return false;
}

/**
 * ListView CustomDraw function.
 * @param plvcd	[in/out] NMLVCUSTOMDRAW
 * @return Return value.
 */
inline int KeyManagerTabPrivate::ListView_CustomDraw(NMLVCUSTOMDRAW *plvcd)
{
	// Check if this is an "odd" row.
	bool isOdd;
	if (isComCtl32_v610) {
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
				case 1: {
					// "Value" column.
					// Use the monospaced font.
					HFONT hFontMono = fontHandler.monospacedFont();
					if (hFontMono) {
						SelectObject(plvcd->nmcd.hdc, hFontMono);
						result = CDRF_NEWFONT;
					}
					break;
				}

				case 2: {
					// "Valid?" column.
					// Draw the icon manually.
					const KeyStoreWin32::Key *const key = keyStore->getKey((int)plvcd->nmcd.dwItemSpec);
					assert(key != nullptr);
					if (!key)
						break;

					HICON hDrawIcon = nullptr;
					switch (key->status) {
						case KeyStoreWin32::Key::Status::Unknown:
							// Unknown...
							hDrawIcon = hIconUnknown;
							break;
						case KeyStoreWin32::Key::Status::NotAKey:
							// The key data is not in the correct format.
							hDrawIcon = hIconInvalid;
							break;
						case KeyStoreWin32::Key::Status::Empty:
							// Empty key.
							break;
						case KeyStoreWin32::Key::Status::Incorrect:
							// Key is incorrect.
							hDrawIcon = hIconInvalid;
							break;
						case KeyStoreWin32::Key::Status::OK:
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
						BOOL bRet = ListView_GetSubItemRect(plvcd->nmcd.hdr.hwndFrom,
							(int)plvcd->nmcd.dwItemSpec, plvcd->iSubItem, LVIR_BOUNDS, &rectTmp);
						if (!bRet)
							break;
						pRcSubItem = &rectTmp;
					}

					// Custom drawing this subitem.
					result = CDRF_SKIPDEFAULT;

					if (g_darkModeEnabled) {
						// Windows 10 Dark Mode method. (Tested on 1809 and 21H2.)
						// TODO: Check Windows 8?

						// Alternate row color, if necessary.
						// NOTE: Only if not highlighted or selected.
						// NOTE 2: Need to check highlighted row ID because uItemState
						// will be 0 if the user mouses over another column on the same row.
						if (isOdd && plvcd->nmcd.uItemState == 0 &&
						    ListView_GetHotItem(plvcd->nmcd.hdr.hwndFrom) != plvcd->nmcd.dwItemSpec)
						{
							FillRect(plvcd->nmcd.hdc, pRcSubItem, hbrAltRow);
						}
					} else {
						// Windows XP/7 method. (Also Windows 10 Light Mode.)
						// FIXME: May have been changed to the Dark Mode method
						// in 21H2, or sometime after 1809.

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
					}

					const int x = pRcSubItem->left + (((pRcSubItem->right - pRcSubItem->left) - iconSize) / 2);
					const int y = pRcSubItem->top + (((pRcSubItem->bottom - pRcSubItem->top) - iconSize) / 2);

					DrawIconEx(plvcd->nmcd.hdc, x, y, hDrawIcon,
						iconSize, iconSize, 0, nullptr, DI_NORMAL);
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
	// Get the current DPI.
	HWND hListView = GetDlgItem(hWndPropSheet, IDC_KEYMANAGER_LIST);
	const UINT dpi = rp_GetDpiForWindow(hListView);
	assert(dpi != 0);

	int iconSize_new;
	if (dpi <= 144) {
		// [0,144] dpi: Use 16x16.
		iconSize_new = 16;
	} else if (dpi <= 192) {
		// (144,192] dpi: Use 24x24.
		iconSize_new = 24;
	} else {
		// >192dpi: Use 32x32.
		iconSize_new = 32;
	}

	if (iconSize == iconSize_new) {
		// Icons are already loaded.
		return;
	}

	// Save the new icon size.
	iconSize = iconSize_new;

	// Free the icons if they were already loaded.
	if (hIconUnknown) {
		DestroyIcon(hIconUnknown);
		hIconUnknown = nullptr;
	}
	if (hIconInvalid) {
		DestroyIcon(hIconInvalid);
		hIconInvalid = nullptr;
	}
	if (hIconGood) {
		DestroyIcon(hIconGood);
		hIconGood = nullptr;
	}

	// Load the icons.
	// NOTE: Using IDI_* will only return the 32x32 icon.
	// Need to get the icon from USER32 directly.
	HMODULE hUser32 = GetModuleHandle(_T("user32.dll"));
	assert(hUser32 != nullptr);
	if (hUser32) {
		hIconUnknown = (HICON)LoadImage(hUser32,
			MAKEINTRESOURCE(102), IMAGE_ICON,
			iconSize_new, iconSize_new, 0);
		hIconInvalid = (HICON)LoadImage(hUser32,
			MAKEINTRESOURCE(103), IMAGE_ICON,
			iconSize_new, iconSize_new, 0);
	}

	// Load hIconGood from our own resource section.
	// Based on KDE Oxygen 5.35.0's base/16x16/actions/dialog-ok-apply.png
	hIconGood = (HICON)LoadImage(HINST_THISCOMPONENT,
		MAKEINTRESOURCE(IDI_KEY_VALID), IMAGE_ICON,
		iconSize_new, iconSize_new, 0);
}

/**
 * Show key import return status.
 * @param filename Filename
 * @param keyType Key type
 * @param iret ImportReturn
 */
void KeyManagerTabPrivate::showKeyImportReturnStatus(
	const tstring &filename,
	const TCHAR *keyType,
	const KeyStoreUI::ImportReturn &iret)
{
	unsigned int type = MB_ICONINFORMATION;
	bool showKeyStats = false;
	tstring msg;
	msg.reserve(1024);

	// Filename, minus directory.
	tstring fileNoPath;
	const size_t bs_pos = filename.rfind(_T('\\'));
	if (bs_pos != tstring::npos && bs_pos != fileNoPath.size()-1) {
		fileNoPath = filename.substr(bs_pos + 1);
	} else {
		fileNoPath = filename;
	}


	// Using ostringstream for numeric formatting.
	// (MSVCRT doesn't support the apostrophe printf specifier.)
	tostringstream toss;
	toss.imbue(std::locale(""));

	// TODO: Localize POSIX error messages?
	// TODO: Thread-safe _wcserror()?

	switch (iret.status) {
		case KeyStoreUI::ImportStatus::InvalidParams:
		default:
			msg = LibWin32UI::unix2dos(TC_("KeyManagerTab",
				"An invalid parameter was passed to the key importer.\n"
				"THIS IS A BUG; please report this to the developers!"));
			type = MB_ICONSTOP;
			break;

		case KeyStoreUI::ImportStatus::UnknownKeyID:
			msg = LibWin32UI::unix2dos(TC_("KeyManagerTab",
				"An unknown key ID was passed to the key importer.\n"
				"THIS IS A BUG; please report this to the developers!"));
			type = MB_ICONSTOP;
			break;

		case KeyStoreUI::ImportStatus::OpenError:
			if (iret.error_code != 0) {
				// tr: %1$s == filename, %2$s == error message
				msg = rp_stprintf_p(TC_("KeyManagerTab",
					"An error occurred while opening '%1$s': %2$s"),
					fileNoPath.c_str(), _wcserror(iret.error_code));
			} else {
				// tr: %s == filename
				msg = rp_stprintf(TC_("KeyManagerTab",
					"An error occurred while opening '%s'."),
					fileNoPath.c_str());
			}
			type = MB_ICONSTOP;
			break;

		case KeyStoreUI::ImportStatus::ReadError:
			// TODO: Error code for short reads.
			if (iret.error_code != 0) {
				// tr: %1$s == filename, %2$s == error message
				msg = rp_stprintf_p(TC_("KeyManagerTab",
					"An error occurred while reading '%1$s': %2$s"),
					fileNoPath, _wcserror(iret.error_code));
			} else {
				// tr: %s == filename
				msg = rp_stprintf(TC_("KeyManagerTab",
					"An error occurred while reading '%s'."),
					fileNoPath.c_str());
			}
			type = MB_ICONSTOP;
			break;

		case KeyStoreUI::ImportStatus::InvalidFile:
			// tr: %1$s == filename, %2$s == type of file
			msg = rp_stprintf_p(TC_("KeyManagerTab",
				"The file '%1$s' is not a valid %2$s file."),
				fileNoPath.c_str(), keyType);
			type = MB_ICONWARNING;
			break;

		case KeyStoreUI::ImportStatus::NoKeysImported:
			// tr: %s == filename
			msg = rp_stprintf(TC_("KeyManagerTab",
				"No keys were imported from '%s'."),
				fileNoPath.c_str());
			type = MB_ICONINFORMATION;
			showKeyStats = true;
			break;

		case KeyStoreUI::ImportStatus::KeysImported: {
			const unsigned int keyCount = iret.keysImportedVerify + iret.keysImportedNoVerify;
			toss << keyCount;
			// tr: %1$s == number of keys (formatted), %2$u == filename
			msg = rp_stprintf_p(TNC_("KeyManagerTab",
				"%1$s key was imported from '%2$s'.",
				"%1$s keys were imported from '%2$s'.",
				keyCount),
				toss.str().c_str(), fileNoPath.c_str());
			type = MB_ICONINFORMATION;
			showKeyStats = true;
			break;
		}
	}

	// U+2022 (BULLET) == \xE2\x80\xA2
#ifdef _UNICODE
	static constexpr TCHAR nl_bullet[] = _T("\r\n\x2022 ");
#else /* !_UNICODE */
	static constexpr TCHAR nl_bullet[] = _T("\r\n* ");
#endif /* _UNICODE */

	// TODO: Numeric formatting.
	if (showKeyStats) {
		if (iret.keysExist > 0) {
			toss.str(_T(""));
			toss.clear();
			toss << iret.keysExist;
			msg += nl_bullet;
			// tr: %s == number of keys (formatted)
			msg += rp_stprintf(TNC_("KeyManagerTab",
				"%s key already exists in the Key Manager.",
				"%s keys already exist in the Key Manager.",
				iret.keysExist),
				toss.str().c_str());
		}
		if (iret.keysInvalid > 0) {
			toss.str(_T(""));
			toss.clear();
			toss << iret.keysInvalid;
			msg += nl_bullet;
			// tr: %s == number of keys (formatted)
			msg += rp_stprintf(TNC_("KeyManagerTab",
				"%s key was not imported because it is incorrect.",
				"%s keys were not imported because they are incorrect.",
				iret.keysInvalid),
				toss.str().c_str());
		}
		if (iret.keysNotUsed > 0) {
			toss.str(_T(""));
			toss.clear();
			toss << iret.keysNotUsed;
			msg += nl_bullet;
			// tr: %s == number of keys (formatted)
			msg += rp_stprintf(TNC_("KeyManagerTab",
				"%s key was not imported because it isn't used by rom-properties.",
				"%s keys were not imported because they aren't used by rom-properties.",
				iret.keysNotUsed),
				toss.str().c_str());
		}
		if (iret.keysCantDecrypt > 0) {
			toss.str(_T(""));
			toss.clear();
			toss << iret.keysCantDecrypt;
			msg += nl_bullet;
			// tr: %s == number of keys (formatted)
			msg += rp_stprintf(TNC_("KeyManagerTab",
				"%s key was not imported because it is encrypted and the master key isn't available.",
				"%s keys were not imported because they are encrypted and the master key isn't available.",
				iret.keysCantDecrypt),
				toss.str().c_str());
		}
		if (iret.keysImportedVerify > 0) {
			toss.str(_T(""));
			toss.clear();
			toss << iret.keysImportedVerify;
			msg += nl_bullet;
			// tr: %s == number of keys (formatted)
			msg += rp_stprintf(TNC_("KeyManagerTab",
				"%s key has been imported and verified as correct.",
				"%s keys have been imported and verified as correct.",
				iret.keysImportedVerify),
				toss.str().c_str());
		}
		if (iret.keysImportedNoVerify > 0) {
			toss.str(_T(""));
			toss.clear();
			toss << iret.keysImportedVerify;
			msg += nl_bullet;
			msg += rp_stprintf(TNC_("KeyManagerTab",
				"%s key has been imported without verification.",
				"%s keys have been imported without verification.",
				iret.keysImportedNoVerify),
				toss.str().c_str());
		}
	}

	RECT winRect;
	GetClientRect(hWndPropSheet, &winRect);
	// NOTE: We need to move left by 1px.
	OffsetRect(&winRect, -1, 0);

	// Count the number of newlines and increase the MessageWidget height.
	const int nl_count = static_cast<int>(std::count_if(msg.cbegin(), msg.cend(),
		[](TCHAR chr) noexcept -> bool { return (chr == _T('\n')); }
	));

	// Determine the size.
	// TODO: Update on DPI change.
	const int cySmIcon = GetSystemMetrics(SM_CYSMICON);
	const SIZE szMsgw = {
		winRect.right - winRect.left,
		(cySmIcon * (nl_count + 1)) + 8
	};

	if (!hMessageWidget) {
		// Create the MessageWidget.
		MessageWidgetRegister();

		// Determine the position.
		const POINT ptMsgw = {winRect.left, winRect.top};

		const DWORD dwExStyleRTL = LibWin32UI::isSystemRTL();
		hMessageWidget = CreateWindowEx(
			WS_EX_NOPARENTNOTIFY | WS_EX_TRANSPARENT | dwExStyleRTL,
			WC_MESSAGEWIDGET, nullptr,
			WS_CHILD | WS_TABSTOP | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
			ptMsgw.x, ptMsgw.y, szMsgw.cx, szMsgw.cy,
			hWndPropSheet, nullptr,
			HINST_THISCOMPONENT, nullptr);
		SetWindowFont(hMessageWidget, GetWindowFont(hWndPropSheet), false);
	} else {
		// Adjust the MessageWidget height.
		SetWindowPos(hMessageWidget, nullptr, 0, 0, szMsgw.cx, szMsgw.cy,
			SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOZORDER | SWP_NOMOVE);
	}

	// Adjust the ListView positioning and size.
	HWND hListView = GetDlgItem(hWndPropSheet, IDC_KEYMANAGER_LIST);
	assert(hListView != nullptr);
	SetWindowPos(hListView, nullptr,
		ptListView.x, ptListView.y + szMsgw.cy,
		szListView.cx, szListView.cy - szMsgw.cy,
		SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOZORDER);

	// Show the message widget.
	// TODO: Adjust the ListView positioning and size?
	MessageBeep(type);
	MessageWidget_SetMessageType(hMessageWidget, type);
	SetWindowText(hMessageWidget, msg.c_str());
	ShowWindow(hMessageWidget, SW_SHOW);
}

/**
 * Import keys from a binary file.
 * @param id Type of file
 */
void KeyManagerTabPrivate::importKeysFromBin(KeyStoreUI::ImportFileID id)
{
	assert(id >= KeyStoreUI::ImportFileID::WiiKeysBin);
	assert(id <= KeyStoreUI::ImportFileID::N3DSaeskeydb);
	if (id < KeyStoreUI::ImportFileID::WiiKeysBin ||
	    id > KeyStoreUI::ImportFileID::N3DSaeskeydb)
		return;

	assert(hWndPropSheet != nullptr);
	if (!hWndPropSheet)
		return;

	static constexpr char dialog_titles_tbl[][32] = {
		// tr: Wii keys.bin dialog title
		NOP_C_("KeyManagerTab", "Select Wii keys.bin File"),
		// tr: Wii U otp.bin dialog title
		NOP_C_("KeyManagerTab", "Select Wii U otp.bin File"),
		// tr: Nintendo 3DS boot9.bin dialog title
		NOP_C_("KeyManagerTab", "Select 3DS boot9.bin File"),
		// tr: Nintendo 3DS aeskeydb.bin dialog title
		NOP_C_("KeyManagerTab", "Select 3DS aeskeydb.bin File"),
	};

	static constexpr char file_filters_tbl[][88] = {
		// tr: Wii keys.bin file filter (RP format)
		NOP_C_("KeyManagerTab", "keys.bin|keys.bin|-|Binary Files|*.bin|-|All Files|*|-"),
		// tr: Wii U otp.bin file filter (RP format)
		NOP_C_("KeyManagerTab", "otp.bin|otp.bin|-|Binary Files|*.bin|-|All Files|*|-"),
		// tr: Nintendo 3DS boot9.bin file filter (RP format)
		NOP_C_("KeyManagerTab", "boot9.bin|boot9.bin|-|Binary Files|*.bin|-|All Files|*|-"),
		// tr: Nintendo 3DS aeskeydb.bin file filter (RP format)
		NOP_C_("KeyManagerTab", "aeskeydb.bin|aeskeydb.bin|-|Binary Files|*.bin|-|All Files|*|-"),
	};

	const tstring tfilename = LibWin32UI::getOpenFileName(hWndPropSheet,
		tpgettext_expr("KeyManagerTab", dialog_titles_tbl[(int)id]),
		tpgettext_expr("KeyManagerTab", file_filters_tbl[(int)id]),
		ts_keyFileDir.c_str());
	if (tfilename.empty())
		return;

	// Update ts_keyFileDir using the returned filename.
	updateKeyFileDir(tfilename);

	const KeyStoreWin32::ImportReturn iret = keyStore->importKeysFromBin(id, tfilename.c_str());
	showKeyImportReturnStatus(tfilename, import_menu_actions[(int)id], iret);
}

/** KeyManagerTab **/

KeyManagerTab::KeyManagerTab(void)
	: d_ptr(new KeyManagerTabPrivate())
{}

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
	const tstring tsTabTitle = TC_("KeyManagerTab", "Key Manager");

	PROPSHEETPAGE psp;
	psp.dwSize = sizeof(psp);
	psp.dwFlags = PSP_USECALLBACK | PSP_USETITLE | PSP_DLGINDIRECT;
	psp.hInstance = HINST_THISCOMPONENT;
	psp.pResource = LoadDialog_i18n(HINST_THISCOMPONENT, IDD_CONFIG_KEYMANAGER);
	psp.pszIcon = nullptr;
	psp.pszTitle = tsTabTitle.c_str();
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
 * Save the contents of this tab.
 */
void KeyManagerTab::save(void)
{
	RP_D(KeyManagerTab);
	d->save();
}
