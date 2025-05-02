/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * RP_XAttrView.cpp: Extended attribute viewer property page.              *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

// References:
// - http://www.codeproject.com/Articles/338268/COM-in-C
// - https://code.msdn.microsoft.com/windowsapps/CppShellExtPropSheetHandler-d93b49b7
// - https://docs.microsoft.com/en-us/windows/win32/ad/implementing-the-property-page-com-object

#include "stdafx.h"

#include "RP_XAttrView.hpp"
#include "RP_XAttrView_p.hpp"
#include "res/resource.h"

// librpbase, librpfile
#include "librpbase/config/Config.hpp"
#include "librpfile/xattr/XAttrReader.hpp"
using LibRpBase::Config;
using LibRpFile::XAttrReader;

// libwin32ui
#include "libwin32ui/LoadResource_i18n.hpp"
using LibWin32UI::LoadDialog_i18n;

// MS-DOS and Windows attributes
// NOTE: Does not depend on the Windows SDK.
#include "librpfile/xattr/dos_attrs.h"

// C++ STL classes
using std::array;
using std::tstring;

// Win32 dark mode
#include "libwin32darkmode/DarkMode.hpp"

// CLSID
const CLSID CLSID_RP_XAttrView =
	{0xB0503F2E, 0xC4AE, 0x48DF, {0xA8,0x80, 0xE2, 0xB1, 0x22, 0xB5, 0x85, 0x71}};

/** RP_XAttrView_Private **/

// Property for "tab pointer".
// This points to the RP_XAttrView_Private::tab object.
const TCHAR RP_XAttrView_Private::TAB_PTR_PROP[] = _T("RP_XAttrView_Private::tab");

/**
 * RP_XAttrView_Private constructor
 * @param q
 * @param filename Filename (RP_XAttrView_Private takes ownership)
 */
RP_XAttrView_Private::RP_XAttrView_Private(RP_XAttrView *q, LPTSTR tfilename)
	: q_ptr(q)
	, hDlgSheet(nullptr)
	, tfilename(tfilename)
	, dwExStyleRTL(LibWin32UI::isSystemRTL())
	, colorAltRow(0)	// initialized later
	, isDarkModeEnabled(false)
	, isFullyInit(false)
{}

RP_XAttrView_Private::~RP_XAttrView_Private()
{
	free(tfilename);
}

/**
 * Load MS-DOS attributes, if available.
 * @return 0 on success; negative POSIX error code on error.
 */
int RP_XAttrView_Private::loadDosAttrs(void)
{
	const bool hasDosAttributes = xattrReader->hasDosAttributes();
	unsigned int attrs, validAttrs;
	if (likely(hasDosAttributes)) {
		attrs = xattrReader->dosAttributes();
		validAttrs = xattrReader->validDosAttributes();
	} else {
		attrs = 0;
		validAttrs = 0;
	}

	// TODO: Use a "starting resource ID" instead of specifying each one?
	struct res_map_t {
		uint16_t id;
		uint16_t attr;
	};
	static const array<res_map_t, 6> res_map = {{
		{IDC_XATTRVIEW_DOS_READONLY, FILE_ATTRIBUTE_READONLY},
		{IDC_XATTRVIEW_DOS_HIDDEN, FILE_ATTRIBUTE_HIDDEN},
		{IDC_XATTRVIEW_DOS_ARCHIVE, FILE_ATTRIBUTE_ARCHIVE},
		{IDC_XATTRVIEW_DOS_SYSTEM, FILE_ATTRIBUTE_SYSTEM},
		{IDC_XATTRVIEW_NTFS_COMPRESSED, FILE_ATTRIBUTE_COMPRESSED},
		{IDC_XATTRVIEW_NTFS_ENCRYPTED, FILE_ATTRIBUTE_ENCRYPTED},
	}};

	for (const auto &p : res_map) {
		HWND hCheckbox = GetDlgItem(hDlgSheet, p.id);
		Button_SetCheck(hCheckbox, (attrs & p.attr) ? BST_CHECKED : BST_UNCHECKED);
		EnableWindow(hCheckbox, !!(validAttrs & p.attr));
	}

	return (likely(hasDosAttributes)) ? 0 : -ENOENT;
}

/**
 * Load the compression algorithm, if available.
 * @return 0 on success; negative POSIX error code on error.
 */
int RP_XAttrView_Private::loadCompressionAlgorithm(void)
{
	HWND hCboZAlg = GetDlgItem(hDlgSheet, IDC_XATTRVIEW_NTFS_COMPRESSION_ALG);

	if (!xattrReader->hasCompressionAlgorithm()) {
		// No compression algorithm...
		// NOTE: If FILE_ATTRIBUTE_COMPRESSED is set, assume LZNT1.
		XAttrReader::ZAlgorithm zalg = XAttrReader::ZAlgorithm::None;
		if (xattrReader->hasDosAttributes()) {
			if ((xattrReader->validDosAttributes() & FILE_ATTRIBUTE_COMPRESSED) &&
			    (xattrReader->dosAttributes() & FILE_ATTRIBUTE_COMPRESSED))
			{
				// File is compressed. Assume LZNT1.
				zalg = XAttrReader::ZAlgorithm::LZNT1;
			}
		}
		ComboBox_SetCurSel(hCboZAlg, static_cast<int>(zalg));
		return -ENOENT;
	}

	ComboBox_SetCurSel(hCboZAlg, static_cast<int>(xattrReader->compressionAlgorithm()));
	return 0;
}

/**
 * Load alternate data streams, if available.
 * @return 0 on success; negative POSIX error code on error.
 */
int RP_XAttrView_Private::loadADS(void)
{
	// Hide by default.
	// If we do have attributes, we'll show the widgets there.
	HWND grpADS = GetDlgItem(hDlgSheet, IDC_XATTRVIEW_GRPADS);
	HWND hListViewADS = GetDlgItem(hDlgSheet, IDC_XATTRVIEW_LISTVIEW_ADS);
	assert(grpADS != nullptr);
	assert(hListViewADS != nullptr);
	ShowWindow(grpADS, SW_HIDE);
	ShowWindow(hListViewADS, SW_HIDE);

	ListView_DeleteAllItems(hListViewADS);
	if (!xattrReader->hasGenericXAttrs()) {
		// No generic attributes.
		return -ENOENT;
	}
	const XAttrReader::XAttrList &xattrList = xattrReader->genericXAttrs();

	LVITEM lvItem;
	lvItem.mask = LVIF_TEXT;
	lvItem.iItem = 0;
	for (const auto &xattr : xattrList) {
		tstring tstr = U82T_c(xattr.first.c_str());
		lvItem.iSubItem = 0;
		lvItem.pszText = const_cast<LPTSTR>(tstr.c_str());
		ListView_InsertItem(hListViewADS, &lvItem);

		// Trim spaces for the value.
		// TODO: Split this into a separate function.
		tstr = U82T_c(xattr.second.c_str());
		// Trim at the end.
		size_t pos = tstr.size();
		while (pos > 0 && ISSPACE(tstr[pos-1])) {
			pos--;
		}
		tstr.resize(pos);
		// Trim at the start.
		pos = 0;
		while (pos < tstr.size() && ISSPACE(tstr[pos])) {
			pos++;
		}
		if (pos == tstr.size()) {
			tstr.clear();
		} else if (pos > 0) {
			tstr = tstr.substr(pos);
		}

		// TODO: Handle newlines.
		lvItem.iSubItem = 1;
		lvItem.pszText = const_cast<LPTSTR>(tstr.c_str());
		ListView_SetItem(hListViewADS, &lvItem);

		// Next item
		lvItem.iItem++;
	}

	// Set extended ListView styles.
	DWORD lvsExStyle = LVS_EX_FULLROWSELECT;
	if (!GetSystemMetrics(SM_REMOTESESSION)) {
		// Not RDP (or is RemoteFX): Enable double buffering.
		lvsExStyle |= LVS_EX_DOUBLEBUFFER;
	}
	ListView_SetExtendedListViewStyle(hListViewADS, lvsExStyle);

	// Auto-size columns
	ListView_SetColumnWidth(hListViewADS, 0, LVSCW_AUTOSIZE_USEHEADER);
	ListView_SetColumnWidth(hListViewADS, 1, LVSCW_AUTOSIZE_USEHEADER);

	// Extended attributes retrieved.
	ShowWindow(grpADS, SW_SHOW);
	ShowWindow(hListViewADS, SW_SHOW);
	return 0;
}

/**
 * Load the attributes from the specified file.
 * The attributes will be loaded into the display widgets.
 * @return 0 on success; negative POSIX error code on error.
 */
int RP_XAttrView_Private::loadAttributes(void)
{
	if (!tfilename) {
		// No filename.
		xattrReader.reset();
		return -EIO;
	}

	// Open an XAttrReader.
	xattrReader.reset(new XAttrReader(tfilename));
	int err = xattrReader->lastError();
	if (err != 0) {
		// Error reading attributes.
		// TODO: Cancel tab loading?
		xattrReader.reset();
		return err;
	}

	// Load the attributes.
	bool hasAnyAttrs = false;
	// TODO: Load Linux attributes? (WSL, etc)
	int ret = loadDosAttrs();
	if (ret == 0) {
		hasAnyAttrs = true;
	}
	ret = loadCompressionAlgorithm();
	if (ret == 0) {
		hasAnyAttrs = true;
	}
	ret = loadADS();
	if (ret == 0) {
		hasAnyAttrs = true;
	}

	// If we have attributes, great!
	// If not, clear the display widgets.
	if (!hasAnyAttrs) {
		// TODO: Cancel tab loading?
		clearDisplayWidgets();
	}
	return 0;
}

/**
 * Clear the display widgets.
 */
void RP_XAttrView_Private::clearDisplayWidgets()
{
	// NOTE: Assuming contiguous resource IDs.
	for (uint16_t id = IDC_XATTRVIEW_DOS_READONLY; id <= IDC_XATTRVIEW_NTFS_ENCRYPTED; id++) {
		Button_SetCheck(GetDlgItem(hDlgSheet, id), BST_UNCHECKED);
	}

	ListView_DeleteAllItems(GetDlgItem(hDlgSheet, IDC_XATTRVIEW_LISTVIEW_ADS));
}

/**
 * Initialize the dialog. (hDlgSheet)
 * Called by WM_INITDIALOG.
 */
void RP_XAttrView_Private::initDialog(void)
{
	assert(hDlgSheet != nullptr);
	if (!hDlgSheet) {
		// No dialog.
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

	// Set up strings for NTFS compression.
	// NOTE: Not localized!
	HWND hCboZAlg = GetDlgItem(hDlgSheet, IDC_XATTRVIEW_NTFS_COMPRESSION_ALG);
	ComboBox_AddString(hCboZAlg, _T("None"));	// TODO: Localize this?
	ComboBox_AddString(hCboZAlg, _T("LZNT1"));
	ComboBox_AddString(hCboZAlg, _T("XPRESS4K"));
	ComboBox_AddString(hCboZAlg, _T("LZX"));
	ComboBox_AddString(hCboZAlg, _T("XPRESS8K"));
	ComboBox_AddString(hCboZAlg, _T("XPRESS16K"));

	// Initialize ADS ListView columns.
	HWND hListViewADS = GetDlgItem(hDlgSheet, IDC_XATTRVIEW_LISTVIEW_ADS);
	assert(hListViewADS != nullptr);
	LVCOLUMN lvColumn;
	lvColumn.mask = LVCF_TEXT | LVCF_FMT;
	lvColumn.fmt = LVCFMT_LEFT;
	lvColumn.pszText = const_cast<LPTSTR>(_T("Name"));
	ListView_InsertColumn(hListViewADS, 0, &lvColumn);
	lvColumn.pszText = const_cast<LPTSTR>(_T("Value"));
	ListView_InsertColumn(hListViewADS, 1, &lvColumn);

	// Auto-size columns
	ListView_SetColumnWidth(hListViewADS, 0, LVSCW_AUTOSIZE_USEHEADER);
	ListView_SetColumnWidth(hListViewADS, 1, LVSCW_AUTOSIZE_USEHEADER);

	// Initialize the alternate row color.
	colorAltRow = LibWin32UI::ListView_GetBkColor_AltRow(hListViewADS);

	// Load attributes.
	// TODO: Cancel tab loading if it fails?
	loadAttributes();

	// Window is fully initialized.
	isFullyInit = true;
}

/** RP_XAttrView **/

RP_XAttrView::RP_XAttrView()
	: d_ptr(nullptr)
{
	// NOTE: d_ptr is not initialized until we receive a valid
	// filename. This reduces overhead in cases where there are
	// lots of files with ROM-like file extensions but aren't
	// actually supported by rom-properties.
}

RP_XAttrView::~RP_XAttrView()
{
	delete d_ptr;
}

/** IUnknown **/
// Reference: https://docs.microsoft.com/en-us/office/client-developer/outlook/mapi/implementing-iunknown-in-c-plus-plus

IFACEMETHODIMP RP_XAttrView::QueryInterface(_In_ REFIID riid, _Outptr_ LPVOID *ppvObj)
{
#ifdef _MSC_VER
#  pragma warning(push)
#  pragma warning(disable: 4365 4838)
#endif /* _MSC_VER */
	static const QITAB rgqit[] = {
		QITABENT(RP_XAttrView, IShellExtInit),
		QITABENT(RP_XAttrView, IShellPropSheetExt),
		{ 0, 0 }
	};
#ifdef _MSC_VER
#  pragma warning(pop)
#endif /* _MSC_VER */
	return LibWin32Common::rp_QISearch(this, rgqit, riid, ppvObj);
}

/** IShellExtInit **/
// Reference: https://docs.microsoft.com/en-us/windows/win32/api/shobjidl_core/nf-shobjidl_core-ishellextinit-initialize [Initialize()]

IFACEMETHODIMP RP_XAttrView::Initialize(
	_In_ LPCITEMIDLIST pidlFolder, _In_ LPDATAOBJECT pDataObj, _In_ HKEY hKeyProgID)
{
	((void)pidlFolder);
	((void)hKeyProgID);

	// Based on CppShellExtPropSheetHandler.
	// https://code.msdn.microsoft.com/windowsapps/CppShellExtPropSheetHandler-d93b49b7
	if (!pDataObj) {
		return E_INVALIDARG;
	}

	// Check if XAttrView is enabled.
	const Config *const config = Config::instance();
	if (!config->getBoolConfigOption(Config::BoolConfig::Options_ShowXAttrView)) {
		// XAttrView is disabled.
		return E_FAIL;
	}

	// TODO: Handle CFSTR_MOUNTEDVOLUME for volumes mounted on an NTFS mount point.
	FORMATETC fe = { CF_HDROP, nullptr, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
	STGMEDIUM stm;

	HRESULT hr = E_FAIL;
	HDROP hDrop;
	UINT nFiles, cchFilename;
	TCHAR *tfilename = nullptr;

	// The pDataObj pointer contains the objects being acted upon. In this 
	// example, we get an HDROP handle for enumerating the selected files and 
	// folders.
	if (FAILED(pDataObj->GetData(&fe, &stm)))
		return E_FAIL;

	// Get an HDROP handle.
	hDrop = static_cast<HDROP>(GlobalLock(stm.hGlobal));
	if (!hDrop) {
		ReleaseStgMedium(&stm);
		return E_FAIL;
	}

	// From this point forward, goto cleanup; on error.

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

	// TODO: Check for "bad" file systems before checking ADS?
#if 0
	config = Config::instance();
	if (FileSystem::isOnBadFS(tfilename, config->getBoolConfigOption(Config::BoolConfig::Options_EnableThumbnailOnNetworkFS))) {
		// This file is on a "bad" file system.
		goto cleanup;
	}
#endif

	// Save the filename in the private class for later.
	if (!d_ptr) {
		d_ptr = new RP_XAttrView_Private(this, tfilename);
		tfilename = nullptr;
	}

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

IFACEMETHODIMP RP_XAttrView::AddPages(_In_ LPFNADDPROPSHEETPAGE pfnAddPage, LPARAM lParam)
{
	// Based on CppShellExtPropSheetHandler.
	// https://code.msdn.microsoft.com/windowsapps/CppShellExtPropSheetHandler-d93b49b7
	if (!d_ptr) {
		// Not initialized.
		return E_FAIL;
	}

	// tr: Tab title.
	static const TCHAR tsTabTitle[] = _T("xattrs");

	// Create an XAttrView page.
	PROPSHEETPAGE psp;
	psp.dwSize = sizeof(psp);
	psp.dwFlags = PSP_USECALLBACK | PSP_USETITLE | PSP_DLGINDIRECT;
	psp.hInstance = HINST_THISCOMPONENT;
	psp.pResource = LoadDialog_i18n(HINST_THISCOMPONENT, IDD_XATTRVIEW);
	psp.pszIcon = nullptr;
	psp.pszTitle = tsTabTitle;
	psp.pfnDlgProc = RP_XAttrView_Private::DlgProc;
	psp.pcRefParent = nullptr;
	psp.pfnCallback = RP_XAttrView_Private::CallbackProc;
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

IFACEMETHODIMP RP_XAttrView::ReplacePage(UINT uPageID,
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
 * ListView CustomDraw function.
 * @param plvcd	[in/out] NMLVCUSTOMDRAW
 * @return Return value.
 */
inline int RP_XAttrView_Private::ListView_CustomDraw(NMLVCUSTOMDRAW *plvcd) const
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
				plvcd->clrTextBk = colorAltRow;
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
INT_PTR RP_XAttrView_Private::DlgProc_WM_NOTIFY(HWND hDlg, NMHDR *pHdr)
{
	INT_PTR ret = FALSE;

	switch (pHdr->code) {
		case NM_CUSTOMDRAW: {
			// Custom drawing notification.
			if (pHdr->idFrom != IDC_XATTRVIEW_LISTVIEW_ADS)
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

		default:
			break;
	}

	return ret;
}

/**
 * WM_COMMAND handler for the property sheet.
 * @param hDlg Dialog window
 * @param wParam
 * @param lParam
 * @return Return value.
 */
INT_PTR RP_XAttrView_Private::DlgProc_WM_COMMAND(HWND hDlg, WPARAM wParam, LPARAM lParam)
{
	RP_UNUSED(lParam);

	if (wParam == MAKEWPARAM(IDC_XATTRVIEW_NTFS_COMPRESSION_ALG, CBN_SELCHANGE)) {
		// Don't allow the user to change the compression algorithm.
		// TODO: Maybe make it possible to do that later?
		loadCompressionAlgorithm();
	}

	// Nothing to do here...
	return FALSE;
}

//
//   FUNCTION: FilePropPageDlgProc
//
//   PURPOSE: Processes messages for the property page.
//
INT_PTR CALLBACK RP_XAttrView_Private::DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
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
			RP_XAttrView *const pExt = reinterpret_cast<RP_XAttrView*>(pPage->lParam);
			if (!pExt)
				return true;
			RP_XAttrView_Private *const d = pExt->d_ptr;

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
			auto *const d = reinterpret_cast<RP_XAttrView_Private*>(GetWindowLongPtr(hDlg, GWLP_USERDATA));
			if (!d) {
				// No RP_XAttrView_Private. Can't do anything...
				return false;
			}

			if (d->isFullyInit) {
				// Dialog is already initialized.
				break;
			}

			// Initialize the dialog.
			d->initDialog();

			// Continue normal processing.
			break;
		}

		case WM_NOTIFY: {
			auto *const d = reinterpret_cast<RP_XAttrView_Private*>(GetWindowLongPtr(hDlg, GWLP_USERDATA));
			if (!d) {
				// No RP_XAttrView_Private. Can't do anything...
				return false;
			}

			return d->DlgProc_WM_NOTIFY(hDlg, reinterpret_cast<NMHDR*>(lParam));
		}

		case WM_COMMAND: {
			auto *const d = reinterpret_cast<RP_XAttrView_Private*>(GetWindowLongPtr(hDlg, GWLP_USERDATA));
			if (!d) {
				// No RP_XAttrView_Private. Can't do anything...
				return false;
			}

			return d->DlgProc_WM_COMMAND(hDlg, wParam, lParam);
		}

		case WM_SYSCOLORCHANGE:
		case WM_THEMECHANGED: {
			auto* const d = reinterpret_cast<RP_XAttrView_Private*>(GetWindowLongPtr(hDlg, GWLP_USERDATA));
			if (!d) {
				// No RP_XAttrView_Private. Can't do anything...
				return false;
			}

			UpdateDarkModeEnabled();
			d->isDarkModeEnabled = VerifyDialogDarkMode(GetParent(hDlg));
			// TODO: Force a window update?

			// Update the alternate row color.
			HWND hListViewADS = GetDlgItem(hDlg, IDC_XATTRVIEW_LISTVIEW_ADS);
			assert(hListViewADS != nullptr);
			d->colorAltRow = LibWin32UI::ListView_GetBkColor_AltRow(hListViewADS);
			break;
		}

		case WM_WTSSESSION_CHANGE: {
			auto *const d = reinterpret_cast<RP_XAttrView_Private*>(GetWindowLongPtr(hDlg, GWLP_USERDATA));
			if (!d) {
				// No RP_XAttrView_Private. Can't do anything...
				return false;
			}

			// If RDP was connected, disable ListView double-buffering.
			// If console (or RemoteFX) was connected, enable ListView double-buffering.
			switch (wParam) {
				case WTS_CONSOLE_CONNECT: {
					HWND hListViewADS = GetDlgItem(hDlg, IDC_XATTRVIEW_LISTVIEW_ADS);
					assert(hListViewADS != nullptr);
					if (hListViewADS) {
						DWORD dwExStyle = ListView_GetExtendedListViewStyle(hListViewADS);
						dwExStyle |= LVS_EX_DOUBLEBUFFER;
						ListView_SetExtendedListViewStyle(hListViewADS, dwExStyle);
					}
					break;
				}
				case WTS_REMOTE_CONNECT: {
					HWND hListViewADS = GetDlgItem(hDlg, IDC_XATTRVIEW_LISTVIEW_ADS);
					if (hListViewADS) {
						DWORD dwExStyle = ListView_GetExtendedListViewStyle(hListViewADS);
						dwExStyle &= ~LVS_EX_DOUBLEBUFFER;
						ListView_SetExtendedListViewStyle(hListViewADS, dwExStyle);
					}
					break;
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
		case WM_CTLCOLORSCROLLBAR:
		case WM_CTLCOLORSTATIC: {
			// If using Dark Mode, forward WM_CTLCOLOR* to the parent window.
			// This fixes issues when using StartAllBack on Windows 11
			// to enforce Dark Mode schemes in Windows Explorer.
			// TODO: Handle color scheme changes?
			auto* const d = reinterpret_cast<RP_XAttrView_Private*>(GetWindowLongPtr(hDlg, GWLP_USERDATA));
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
UINT CALLBACK RP_XAttrView_Private::CallbackProc(HWND hWnd, UINT uMsg, LPPROPSHEETPAGE ppsp)
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
			auto *const pExt = reinterpret_cast<RP_XAttrView*>(ppsp->lParam);
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
