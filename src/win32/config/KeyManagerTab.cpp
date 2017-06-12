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

// libwin32common
#include "libwin32common/WinUI.hpp"

// librpbase
#include "librpbase/TextFuncs.hpp"
#include "librpbase/config/Config.hpp"
using LibRpBase::Config;

// libromdata
#include "libromdata/disc/WiiPartition.hpp"
#include "libromdata/crypto/CtrKeyScrambler.hpp"
#include "libromdata/crypto/N3DSVerifyKeys.hpp"
using namespace LibRomData;

// C includes. (C++ namespace)
#include <cassert>

// C++ includes.
#include <algorithm>

// Win32: Split buttons.
// Not available unless _WIN32_WINNT >= 0x0600
#ifndef BS_SPLITBUTTON
#define BS_SPLITBUTTON	0x0000000CL
#endif
#ifndef BCSIF_GLYPH
#define BCSIF_GLYPH	0x0001
#endif
#ifndef BCSIF_IMAGE
#define BCSIF_IMAGE	0x0002
#endif
#ifndef BCSIF_STYLE
#define BCSIF_STYLE	0x0004
#endif
#ifndef BCSIF_SIZE
#define BCSIF_SIZE	0x0008
#endif
#ifndef BCSS_NOSPLIT
#define BCSS_NOSPLIT	0x0001
#endif
#ifndef BCSS_STRETCH
#define BCSS_STRETCH	0x0002
#endif
#ifndef BCSS_ALIGNLEFT
#define BCSS_ALIGNLEFT	0x0004
#endif
#ifndef BCSS_IMAGE
#define BCSS_IMAGE	0x0008
#endif
#ifndef BCM_SETSPLITINFO
#define BCM_SETSPLITINFO         (BCM_FIRST + 0x0007)
#define Button_SetSplitInfo(hwnd, pInfo) \
    (BOOL)SNDMSG((hwnd), BCM_SETSPLITINFO, 0, (LPARAM)(pInfo))
#define BCM_GETSPLITINFO         (BCM_FIRST + 0x0008)
#define Button_GetSplitInfo(hwnd, pInfo) \
    (BOOL)SNDMSG((hwnd), BCM_GETSPLITINFO, 0, (LPARAM)(pInfo))
typedef struct tagBUTTON_SPLITINFO {
	UINT        mask;
	HIMAGELIST  himlGlyph;
	UINT        uSplitStyle;
	SIZE        size;
} BUTTON_SPLITINFO, *PBUTTON_SPLITINFO;
#endif

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

	public:
		// Property sheet.
		HPROPSHEETPAGE hPropSheetPage;
		HWND hWndPropSheet;

		// Has the user changed anything?
		bool changed;

	public:
		// "Import" popup menu.
		HMENU hMenuImport;	// Must be deleted using DestroyMenu().

		// KeyStore.
		KeyStoreWin32 *keyStore;

		// Fonts.
		HFONT hFontDlg;		// Main dialog font.
		HFONT hFontMono;	// Monospaced font.

		// Monospaced font details.
		LOGFONT lfFontMono;
		bool bPrevIsClearType;	// Previous ClearType setting.

	public:
		// TODO: Share with rpcli/verifykeys.cpp.
		// TODO: Central registration of key verification functions?
		typedef int (*pfnKeyCount_t)(void);
		typedef const char* (*pfnKeyName_t)(int keyIdx);
		typedef const uint8_t* (*pfnVerifyData_t)(int keyIdx);

		struct EncKeyFns_t {
			pfnKeyCount_t pfnKeyCount;
			pfnKeyName_t pfnKeyName;
			pfnVerifyData_t pfnVerifyData;
		};

		#define ENCKEYFNS(klass) { \
			klass::encryptionKeyCount_static, \
			klass::encryptionKeyName_static, \
			klass::encryptionVerifyData_static, \
		}

		static const EncKeyFns_t encKeyFns[];
};

/** KeyManagerTabPrivate **/

const KeyManagerTabPrivate::EncKeyFns_t KeyManagerTabPrivate::encKeyFns[] = {
	ENCKEYFNS(WiiPartition),
	ENCKEYFNS(CtrKeyScrambler),
	ENCKEYFNS(N3DSVerifyKeys),

	{nullptr, nullptr, nullptr}
};

KeyManagerTabPrivate::KeyManagerTabPrivate()
	: hPropSheetPage(nullptr)
	, hWndPropSheet(nullptr)
	, changed(false)
	, hMenuImport(nullptr)
	, keyStore(new KeyStoreWin32())
	, hFontDlg(nullptr)
	, hFontMono(nullptr)
	, bPrevIsClearType(nullptr)
{
	memset(&lfFontMono, 0, sizeof(lfFontMono));
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

	// Check the COMCTL32.DLL version.
	HMODULE hComCtl32 = GetModuleHandle(L"COMCTL32");
	assert(hComCtl32 != nullptr);
	typedef HRESULT (CALLBACK *PFNDLLGETVERSION)(DLLVERSIONINFO *pdvi);
	PFNDLLGETVERSION pfnDllGetVersion = nullptr;
	bool isComCtl32_610 = false;
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
		SetWindowText(hBtnImport, L"Import...");
	}

	// Initialize the ListView.
	// Set full row selection.
	ListView_SetExtendedListViewStyle(hListView, LVS_EX_FULLROWSELECT);
	// Set the virtual list item count.
	ListView_SetItemCountEx(hListView, keyStore->totalKeyCount(),
		LVSICF_NOINVALIDATEALL | LVSICF_NOSCROLL);

	// Column 0: Key Name.
	LVCOLUMN lvCol;
	memset(&lvCol, 0, sizeof(lvCol));
	lvCol.mask = LVCF_FMT | LVCF_TEXT | LVCF_SUBITEM;
	lvCol.fmt = LVCFMT_LEFT;
	lvCol.pszText = L"Key Name";
	ListView_InsertColumn(hListView, 0, &lvCol);

	// Column 1: Value.
	lvCol.pszText = L"Value";
	ListView_InsertColumn(hListView, 1, &lvCol);

	// Column 2: Verification status.
	// TODO: Draw an icon.
	lvCol.pszText = L"Valid?";
	ListView_InsertColumn(hListView, 2, &lvCol);

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
		tmp_width[0] = ListView_GetStringWidth(hListView, RP2W_s(key->name)) + column_padding[0];
		//tmp_width[1] = ListView_GetStringWidth(hListView, RP2W_s(key->value)) + column_padding[1];

		column_width[0] = std::max(column_width[0], tmp_width[0]);
		//column_width[1] = std::max(column_width[1], tmp_width[1]);

		int ret = LibWin32Common::measureTextSize(hListView, hFontMono, RP2W_s(key->value), &szValue);
		assert(ret == 0);
		if (ret == 0) {
			column_width[1] = std::max(column_width[1], (int)szValue.cx + column_padding[1]);
		}
	}
	ListView_SetColumnWidth(hListView, 0, column_width[0]);
	ListView_SetColumnWidth(hListView, 1, column_width[1]);

	// Auto-size the "Valid?" column.
	ListView_SetColumnWidth(hListView, 2, LVSCW_AUTOSIZE_USEHEADER);
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

	/* TODO: Load keys from KeyManager. */

	// No longer changed.
	changed = false;
}

/**
 * Save the configuration.
 */
void KeyManagerTabPrivate::save(void)
{
	assert(hWndPropSheet != nullptr);
	if (!hWndPropSheet)
		return;

	/* TODO: Save keys. */

	// No longer changed.
	changed = false;
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
					if (d->changed) {
						d->save();
					}
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
								wcscpy_s(plvItem->pszText, plvItem->cchTextMax, RP2W_s(key->name));
								break;
							case 1:
								// Value.
								wcscpy_s(plvItem->pszText, plvItem->cchTextMax, RP2W_s(key->value));
								break;
							default:
								// No text for "Valid?".
								plvItem->pszText[0] = 0;
								break;
						}
					}
					break;
				}

				case NM_CUSTOMDRAW: {
					// Custom drawing notification.
					if (pHdr->idFrom != IDC_KEYMANAGER_LIST)
						break;

					// Make sure the "Value" column is drawn with a monospaced font.
					// Reference: https://www.codeproject.com/Articles/2890/Using-ListView-control-under-Win-API
					NMLVCUSTOMDRAW *plvcd = reinterpret_cast<LPNMLVCUSTOMDRAW>(lParam);

					// NOTE: Since this is a DlgProc, we can't simply return
					// the CDRF code. It has to be set as DWLP_MSGRESULT.
					// References:
					// - https://stackoverflow.com/questions/40549962/c-winapi-listview-nm-customdraw-not-getting-cdds-itemprepaint
					// - https://stackoverflow.com/a/40552426
					int result = CDRF_DODEFAULT;
					switch (plvcd->nmcd.dwDrawStage) {
						case CDDS_PREPAINT:
							// Request notifications for individual ListView items.
							result = CDRF_NOTIFYITEMDRAW;
							break;

						case CDDS_ITEMPREPAINT:
							result = CDRF_NOTIFYSUBITEMDRAW;
							break;

						case CDDS_SUBITEM | CDDS_ITEMPREPAINT:
							if (plvcd->iSubItem == 1) {
								// Use the monospaced font.
								if (d->hFontMono) {
									SelectObject(plvcd->nmcd.hdc, d->hFontMono);
									result = CDRF_NEWFONT;
									break;
								}
							}
							break;

						default:
							break;
					}
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

			if (HIWORD(wParam) == BN_CLICKED && LOWORD(wParam) == IDC_KEYMANAGER_IMPORT) {
				// Show the "Import" popup menu.
				if (!d->hMenuImport) {
					extern HINSTANCE g_hInstance;
					d->hMenuImport = LoadMenu(g_hInstance, MAKEINTRESOURCE(IDR_KEYMANAGER_IMPORT));
				}

				if (d->hMenuImport) {
					HMENU hSubMenu = GetSubMenu(d->hMenuImport, 0);
					if (hSubMenu) {
						RECT btnRect;
						GetWindowRect(GetDlgItem(hDlg, IDC_KEYMANAGER_IMPORT), &btnRect);
						TrackPopupMenu(hSubMenu, TPM_LEFTALIGN|TPM_TOPALIGN,
							btnRect.left, btnRect.bottom, 0, hDlg, nullptr);
					}
				}
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

	extern HINSTANCE g_hInstance;

	PROPSHEETPAGE psp;
	psp.dwSize = sizeof(psp);
	psp.dwFlags = PSP_USECALLBACK | PSP_USETITLE;
	psp.hInstance = g_hInstance;
	psp.pszTemplate = MAKEINTRESOURCE(IDD_CONFIG_KEYMANAGER);
	psp.pszIcon = nullptr;
	psp.pszTitle = L"Key Manager";
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
