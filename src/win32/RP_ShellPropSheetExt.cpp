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
#include "res/resource.h"

// RomDataView control
#include "RomDataView.hpp"

// Other rom-properties libraries
using namespace LibRpBase;
using namespace LibRomData;

// NOTE: Using "RomDataView" for the libi18n context, since that
// matches what's used for the KDE and GTK+ frontends.

// C++ STL classes
using std::string;
using std::wstring;	// for tstring

// Win32 dark mode
#include "libwin32darkmode/DarkMode.hpp"

// CLSID
const CLSID CLSID_RP_ShellPropSheetExt =
	{0x2443C158, 0xDF7C, 0x4352, {0xB4, 0x35, 0xBC, 0x9F, 0x88, 0x5F, 0xFD, 0x52}};

/** RP_ShellPropSheetExt_Private **/

/** RP_ShellPropSheetExt_Private **/
// Workaround for RP_D() expecting the no-underscore naming convention.
#define RP_ShellPropSheetExtPrivate RP_ShellPropSheetExt_Private

class RP_ShellPropSheetExt_Private
{
public:
	/**
	 * RP_ShellPropSheetExt_Private constructor
	 * @param tfilename ROM filename (NOTE: This class takes ownership!)
	 */
	RP_ShellPropSheetExt_Private(LPTSTR tfilename)
		: hDlgSheet(nullptr)
		, hRomDataView(nullptr)
		, tfilename(tfilename)
		, isDarkModeEnabled(false)
	{ }

	~RP_ShellPropSheetExt_Private()
	{
		free(tfilename);
	}

public:
	// Internal functions used by the callback functions
	INT_PTR DlgProc_WM_NOTIFY(HWND hDlg, NMHDR *pHdr);

public:
	// Property sheet callback functions
	static INT_PTR CALLBACK DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	static UINT CALLBACK CallbackProc(HWND hWnd, UINT uMsg, LPPROPSHEETPAGE ppsp);

public:
	HWND hDlgSheet;		// Property sheet dialog
	HWND hRomDataView;	// RomDataView control
	LPTSTR tfilename;	// ROM filename
	bool isDarkModeEnabled;	// Is the dialog in Dark Mode? (requires something like StartAllBack)
};

/** RP_ShellPropSheetExt **/

RP_ShellPropSheetExt::RP_ShellPropSheetExt()
	: d_ptr(nullptr)
{
	// NOTE: hRomDataView is not initialized until we receive a valid
	// ROM file. This reduces overhead in cases where there are lots
	// of files with ROM-like file extensions but aren't actually
	// supported by rom-properties.
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
	assert(!d_ptr);
	if (!d_ptr) {
		d_ptr = new RP_ShellPropSheetExt_Private(tfilename);
	} else {
		free(d_ptr->tfilename);
		d_ptr->tfilename = tfilename;
	}
	// RP_ShellPropSheetExt_Private takes ownership of tfilename.
	tfilename = nullptr;

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

/** Callback functions **/

/**
 * WM_NOTIFY handler for the property sheet.
 * @param hDlg Dialog window
 * @param pHdr NMHDR
 * @return Return value.
 */
INT_PTR RP_ShellPropSheetExt_Private::DlgProc_WM_NOTIFY(HWND hDlg, NMHDR *pHdr)
{
	INT_PTR ret = FALSE;

	switch (pHdr->code) {
		case PSN_SETACTIVE:
			RomDataView_AnimationCtrl(hRomDataView, true);
			break;

		case PSN_KILLACTIVE:
			RomDataView_AnimationCtrl(hRomDataView, false);
			break;

		default:
			break;
	}

	return ret;
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
			if (!pPage) {
				return true;
			}

			// Access the property sheet extension from property page.
			RP_ShellPropSheetExt *const pExt = reinterpret_cast<RP_ShellPropSheetExt*>(pPage->lParam);
			if (!pExt) {
				return true;
			}
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

			if (d->hRomDataView) {
				// Dialog is already initialized.
				break;
			}

			// Create a new RomDataView control.
			RECT rect_hDlg;
			GetWindowRect(hDlg, &rect_hDlg);
			MapWindowPoints(HWND_DESKTOP, hDlg, (LPPOINT)&rect_hDlg, 2);

			RomDataViewRegister();
			POINT ptRomDataView = {rect_hDlg.left, rect_hDlg.top};
			const SIZE szRomDataView = {
				rect_hDlg.right - rect_hDlg.left,
				rect_hDlg.bottom - rect_hDlg.top
			};

			DWORD dwExStyleRTL = LibWin32UI::isSystemRTL();
			d->hRomDataView = CreateWindowEx(dwExStyleRTL,
				WC_ROMDATAVIEW, nullptr,
				WS_CHILD | WS_VISIBLE | WS_TABSTOP,
				ptRomDataView.x, ptRomDataView.y,
				szRomDataView.cx, szRomDataView.cy,
				hDlg, nullptr, nullptr, nullptr);
			SetWindowFont(d->hRomDataView, GetWindowFont(hDlg), FALSE);
			RomDataView_SetFileName(d->hRomDataView, d->tfilename);

			// Continue normal processing.
			break;
		}

		case WM_NOTIFY: {
			auto *const d = reinterpret_cast<RP_ShellPropSheetExt_Private*>(GetWindowLongPtr(hDlg, GWLP_USERDATA));
			if (!d) {
				// No RP_ShellPropSheetExt_Private. Can't do anything...
				return false;
			}

			return d->DlgProc_WM_NOTIFY(hDlg, reinterpret_cast<NMHDR*>(lParam));
		}

		case WM_SYSCOLORCHANGE:
		case WM_THEMECHANGED: {
			auto* const d = reinterpret_cast<RP_ShellPropSheetExt_Private*>(GetWindowLongPtr(hDlg, GWLP_USERDATA));
			if (!d) {
				// No RP_ShellPropSheetExt_Private. Can't do anything...
				break;
			}

			UpdateDarkModeEnabled();
			d->isDarkModeEnabled = VerifyDialogDarkMode(GetParent(hDlg));
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
