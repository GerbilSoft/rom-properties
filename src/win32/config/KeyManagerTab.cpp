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
{ }

KeyManagerTabPrivate::~KeyManagerTabPrivate()
{
	if (hMenuImport) {
		DestroyMenu(hMenuImport);
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

	// Column 0: Key Name.
	LVCOLUMN lvCol;
	memset(&lvCol, 0, sizeof(lvCol));
	lvCol.mask = LVCF_FMT | LVCF_TEXT | LVCF_SUBITEM;
	lvCol.fmt = LVCFMT_LEFT;
	lvCol.pszText = L"Key Name";
	ListView_InsertColumn(hListView, 0, &lvCol);

	// Column 1: Value.
	// TODO: Set width to 32 monospace chars.
	// TODO: Set font.
	lvCol.pszText = L"Value";
	ListView_InsertColumn(hListView, 1, &lvCol);

	// Column 2: Verification status.
	// TODO: Draw an icon.
	lvCol.pszText = L"Valid?";
	ListView_InsertColumn(hListView, 2, &lvCol);

	// Auto-size the columns.
	ListView_SetColumnWidth(hListView, 0, LVSCW_AUTOSIZE_USEHEADER);
	ListView_SetColumnWidth(hListView, 1, LVSCW_AUTOSIZE_USEHEADER);
	ListView_SetColumnWidth(hListView, 2, LVSCW_AUTOSIZE_USEHEADER);
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

			LPPSHNOTIFY lppsn = reinterpret_cast<LPPSHNOTIFY>(lParam);
			switch (lppsn->hdr.code) {
				case PSN_APPLY:
					// Save settings.
					if (d->changed) {
						d->save();
					}
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
