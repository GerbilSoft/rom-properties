/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * CacheTab.cpp: Thumbnail Cache tab for rp-config.                        *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "CacheTab.hpp"
#include "res/resource.h"

// Other rom-properties libraries
#include "librpfile/RecursiveScan.hpp"
using namespace LibRpBase;
using namespace LibRpFile;
using namespace LibRpText;

// libwin32ui
#include "libwin32ui/LoadResource_i18n.hpp"
using LibWin32UI::LoadDialog_i18n;
using LibWin32UI::RegKey;
using LibWin32UI::WTSSessionNotification;

// Win32 dark mode
#include "libwin32darkmode/DarkMode.hpp"
#include "libwin32darkmode/DarkModeCtrl.hpp"

// IEmptyVolumeCacheCallBack implementation.
#include "RP_EmptyVolumeCacheCallback.hpp"
// Windows: WM_DEVICECHANGE structs.
#include <dbt.h>
#include <devguid.h>

// COM smart pointer typedefs.
_COM_SMARTPTR_TYPEDEF(IImageList, IID_IImageList);
_COM_SMARTPTR_TYPEDEF(IEmptyVolumeCache, IID_IEmptyVolumeCache);

#ifdef __CRT_UUID_DECL
// FIXME: MSYS2/MinGW-w64 (gcc-9.2.0-2, MinGW-w64 7.0.0.5524.2346384e-1)
// doesn't declare the UUID for IEmptyVolumeCache for __uuidof() emulation.
__CRT_UUID_DECL(IEmptyVolumeCache, __MSABI_LONG(0x8fce5227), 0x04da, 0x11d1, 0xa0,0x04, 0x00, 0x80, 0x5f, 0x8a, 0xbe, 0x06)
#endif

// C++ STL classes.
using std::forward_list;
using std::pair;
using std::string;
using std::wstring;

// Timer ID for the XP drive update procedure.
#define TMRID_XP_DRIVE_UPDATE 0xD103

class CacheTabPrivate
{
public:
	CacheTabPrivate();
	~CacheTabPrivate();

private:
	RP_DISABLE_COPY(CacheTabPrivate)

public:
	/**
	 * Initialize the dialog.
	 */
	void initDialog(void);

	/**
	 * Enumerate all drives. (XP version)
	 */
	void enumDrivesXP(void);

	/**
	 * Update drives in the drive list.
	 * @param unitmask Drive mask. (may have multiple bits set)
	 */
	void updateDrivesXP(DWORD unitmask);

	/**
	 * Clear the Thumbnail Cache. (Windows Vista and later.)
	 * @return 0 on success; non-zero on error.
	 */
	int clearThumbnailCacheVista(void);

	/**
	 * Clear the rom-properties cache.
	 * @return 0 on success; non-zero on error.
	 */
	int clearRomPropertiesCache(void);

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
	// Property sheet
	HPROPSHEETPAGE hPropSheetPage;
	HWND hWndPropSheet;

	// Function pointer for SHGetImageList.
	// This function is not exported by name prior to Windows XP.
	typedef HRESULT (STDAPICALLTYPE *PFNSHGETIMAGELIST)(
		_In_ int iImageList, _In_ REFIID riid, _Outptr_result_nullonfailure_ void **ppvObj);

	// Image list for the XP drive list.
	IImageListPtr pImageList;

	// wtsapi32.dll for Remote Desktop status. (WinXP and later)
	WTSSessionNotification wts;

	DWORD dwUnitmaskXP;	// XP drive update mask
	bool isVista;		// Is this Windows Vista or later?

public:
	// Dark Mode background brush
	HBRUSH hbrBkgnd;
	bool lastDarkModeEnabled;
};

/** CacheTabPrivate **/

CacheTabPrivate::CacheTabPrivate()
	: hPropSheetPage(nullptr)
	, hWndPropSheet(nullptr)
	, dwUnitmaskXP(0)
	, isVista(false)
	, hbrBkgnd(nullptr)
	, lastDarkModeEnabled(false)
{
	// Determine which dialog we should use.
	RegKey hKey(HKEY_LOCAL_MACHINE, _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\VolumeCaches\\Thumbnail Cache"), KEY_READ, false);
	if (hKey.isOpen()) {
		// Windows Vista Thumbnail Cache cleaner is available.
		isVista = true;
	} else {
		// Not available. Use manual cache cleaning.
		isVista = false;

		// Handle "critical" errors ourselves.
		// This fixes an issue where Windows shows a
		// "There is no disk in the drive." message when
		// a CD-ROM is removed and we call SHGetFileInfo().
		SetErrorMode(SEM_FAILCRITICALERRORS);
	}
}

CacheTabPrivate::~CacheTabPrivate()
{
	// Dark mode background brush
	if (hbrBkgnd) {
		DeleteBrush(hbrBkgnd);
	}
}

/**
 * Initialize the dialog.
 */
void CacheTabPrivate::initDialog(void)
{
	// Initialize strings.
	SetWindowText(GetDlgItem(hWndPropSheet, IDC_CACHE_DESCRIPTION), U82T_c(isVista
		// tr: Windows Vista and later. Has a centralized thumbnails cache.
		? C_("CacheTab", "If any image type settings were changed, you will need to clear the system thumbnail cache.")
		// tr: Windows XP or earlier. Has Thumbs.db scattered throughout the system.
		: C_("CacheTab", "If any image type settings were changed, you will need to clear the thumbnail cache files.\nThis version of Windows does not have a centralized thumbnail database, so it may take a while for all Thumbs.db files to be located and deleted.")));

	// Set window themes for Win10's dark mode.
	if (g_darkModeSupported) {
		// NOTE: If Dark Mode is supported, then we're definitely
		// running on Windows 10 or later, so this will have the
		// Windows Vista layout.
		// TODO: Progress bar?
		DarkMode_InitButton_Dlg(hWndPropSheet, IDC_CACHE_CLEAR_SYS_THUMBS);
		DarkMode_InitButton_Dlg(hWndPropSheet, IDC_CACHE_CLEAR_RP_DL);
	}

	if (isVista) {
		// System is Vista or later.
		// XP initialization is not needed.
		return;
	}

	// The XP version requires some control initialization.
	Button_SetCheck(GetDlgItem(hWndPropSheet, IDC_CACHE_XP_FIND_DRIVES), BST_CHECKED);
	ShowWindow(GetDlgItem(hWndPropSheet, IDC_CACHE_XP_PATH), SW_HIDE);
	ShowWindow(GetDlgItem(hWndPropSheet, IDC_CACHE_XP_BROWSE), SW_HIDE);

	// FIXME: If a drive's label is short, but later changes to long,
	// the column doesn't automatically expand.
	HWND hListView = GetDlgItem(hWndPropSheet, IDC_CACHE_XP_DRIVES);
	if (!hListView)
		return;

	// NOTE: CacheTab, DllMain, and others call SHELL32 functions
	// directly, so we can assume SHELL32.DLL is loaded.
	HMODULE hShell32_dll = GetModuleHandle(_T("shell32.dll"));
	assert(hShell32_dll != nullptr);
	if (hShell32_dll) {
		// Get SHGetImageList() by ordinal.
		PFNSHGETIMAGELIST pfnSHGetImageList = (PFNSHGETIMAGELIST)GetProcAddress(hShell32_dll, MAKEINTRESOURCEA(727));
		if (pfnSHGetImageList) {
			// Initialize the ListView image list.
			// NOTE: HIMAGELIST and IImageList are compatible.
			// Since this is a system image list, we should *not*
			// release/destroy it when we're done using it.
			HRESULT hr = pfnSHGetImageList(SHIL_SMALL, IID_PPV_ARGS(&pImageList));
			if (SUCCEEDED(hr)) {
				ListView_SetImageList(hListView, reinterpret_cast<HIMAGELIST>(pImageList.GetInterfacePtr()), LVSIL_SMALL);
			}
		}
	}

	// Enable double-buffering if not using RDP.
	if (!GetSystemMetrics(SM_REMOTESESSION)) {
		ListView_SetExtendedListViewStyle(hListView, LVS_EX_DOUBLEBUFFER);
	}

	// Register for WTS session notifications. (Remote Desktop)
	wts.registerSessionNotification(hWndPropSheet, NOTIFY_FOR_THIS_SESSION);

	// Enumerate the drives.
	enumDrivesXP();
}

/**
 * Enumerate all drives. (XP version)
 */
void CacheTabPrivate::enumDrivesXP(void)
{
	HWND hListView = GetDlgItem(hWndPropSheet, IDC_CACHE_XP_DRIVES);
	assert(hListView != nullptr);
	if (!hListView) {
		// Should not be called on Vista+...
		return;
	}

	// Clear the ListView.
	ListView_DeleteAllItems(hListView);

	// Get available drives.
	DWORD dwDrives = GetLogicalDrives();

	// Check each drive.
	TCHAR path[] = _T("X:\\");
	SHFILEINFO sfi;
	LVITEM lvi;
	memset(&lvi, 0, sizeof(lvi));
	lvi.mask = LVIF_IMAGE | LVIF_PARAM | LVIF_TEXT;
	for (unsigned int i = 0; i < 26 && dwDrives != 0; i++, dwDrives >>= 1) {
		// Ignore missing drives and network drives.
		if (!(dwDrives & 1))
			continue;
		path[0] = L'A' + i;
		const UINT uiDriveType = GetDriveType(path);
		if (uiDriveType <= DRIVE_NO_ROOT_DIR || uiDriveType == DRIVE_REMOTE)
			continue;

		DWORD_PTR ret = SHGetFileInfo(path, 0, &sfi, sizeof(sfi),
			SHGFI_DISPLAYNAME | SHGFI_SYSICONINDEX);
		if (ret == 0)
			continue;

		lvi.iItem = i;
		lvi.lParam = i;
		lvi.iImage = sfi.iIcon;
		lvi.pszText = sfi.szDisplayName;
		ListView_InsertItem(hListView, &lvi);
	}
}

/**
 * Update drives in the drive list.
 * @param unitmask Drive mask. (may have multiple bits set)
 */
void CacheTabPrivate::updateDrivesXP(DWORD unitmask)
{
	HWND hListView = GetDlgItem(hWndPropSheet, IDC_CACHE_XP_DRIVES);
	assert(hListView != nullptr);
	if (!hListView) {
		// Should not be called on Vista+...
		return;
	}

	TCHAR path[] = _T("X:\\");
	SHFILEINFO sfi;
	LVITEM lviNew, lviCur;
	memset(&lviNew, 0, sizeof(lviNew));
	memset(&lviCur, 0, sizeof(lviCur));
	lviNew.mask = LVIF_IMAGE | LVIF_PARAM | LVIF_TEXT;
	lviCur.mask = LVIF_PARAM;

	// TODO: Optimize by storing a map of drive letters
	// to ListView indexes somewhere.
	unsigned int mask = 1;
	for (unsigned int i = 0; i < 26; i++, mask <<= 1) {
		// Check if this drive is specified.
		if (!(unitmask & mask))
			continue;

		bool toDelete = true;
		bool isPresent = false;

		// Check the drive status.
		path[0] = L'A' + i;
		const UINT uiDriveType = GetDriveType(path);
		if (uiDriveType > DRIVE_NO_ROOT_DIR && uiDriveType != DRIVE_REMOTE) {
			// Get drive information.
			DWORD_PTR ret = SHGetFileInfo(path, 0, &sfi, sizeof(sfi),
				SHGFI_DISPLAYNAME | SHGFI_SYSICONINDEX);
			if (ret != 0) {
				lviNew.lParam = i;
				lviNew.iImage = sfi.iIcon;
				lviNew.pszText = sfi.szDisplayName;
				toDelete = false;
			}
		}

		// Check if this drive is already in the ListView.
		const int lvItemCount = ListView_GetItemCount(hListView);
		for (int j = 0; j < lvItemCount; j++) {
			lviCur.iItem = j;
			ListView_GetItem(hListView, &lviCur);
			if (lviCur.lParam == i) {
				// Found a match!
				isPresent = true;
				if (toDelete) {
					// Delete the item.
					ListView_DeleteItem(hListView, j);
				} else {
					// Update the item.
					lviNew.iItem = j;
					ListView_SetItem(hListView, &lviNew);
				}
				break;
			}
		}

		if (!toDelete && !isPresent) {
			// Item not found. Add it to the end of the list.
			// TODO: Add in drive letter order?
			lviNew.iItem = lvItemCount;
			ListView_InsertItem(hListView, &lviNew);
		}
	}
}

/**
 * Clear the Thumbnail Cache. (Windows Vista and later.)
 * @return 0 on success; non-zero on error.
 */
int CacheTabPrivate::clearThumbnailCacheVista(void)
{
	HWND hStatusLabel = GetDlgItem(hWndPropSheet, IDC_CACHE_STATUS);
	HWND hProgressBar = GetDlgItem(hWndPropSheet, IDC_CACHE_PROGRESS);
	ShowWindow(hStatusLabel, SW_SHOW);
	ShowWindow(hProgressBar, SW_SHOW);

	// Reset the progress bar.
	SendMessage(hProgressBar, PBM_SETSTATE, PBST_NORMAL, 0);
	SendMessage(hProgressBar, PBM_SETRANGE, 0, MAKELONG(0, 100));
	SendMessage(hProgressBar, PBM_SETPOS, 0, 0);

	// Get all available drive letters.
	char errbuf[128];
	DWORD driveLetters = GetLogicalDrives();
	driveLetters &= 0x3FFFFFF;	// Mask out non-drive letter bits.
	if (driveLetters == 0) {
		// Error retrieving drive letters...
		// NOTE: PBM_SETSTATE is Vista+, which is fine here because
		// this is only run on Vista+.
		DWORD dwErr = GetLastError();
		snprintf(errbuf, sizeof(errbuf), C_("CacheTab|Win32",
			"ERROR: GetLogicalDrives() failed. (GetLastError() == 0x%08X)"),
			static_cast<unsigned int>(dwErr));
		SetWindowText(hStatusLabel, U82T_c(errbuf));
		SendMessage(hProgressBar, PBM_SETSTATE, PBST_ERROR, 0);
		return 1;
	}

	// Ignore all drives that aren't fixed HDDs.
	TCHAR szDrivePath[4] = _T("X:\\");
	unsigned int driveCount = 0;
	for (unsigned int bit = 0; bit < 26; bit++) {
		const uint32_t mask = (1U << bit);
		if (!(driveLetters & mask))
			continue;
		szDrivePath[0] = _T('A') + bit;
		if (GetDriveType(szDrivePath) != DRIVE_FIXED) {
			// Not a fixed HDD.
			driveLetters &= ~mask;
		} else {
			// This is a fixed HDD.
			driveCount++;
		}
	}
	if (driveLetters == 0) {
		// No fixed hard drives detected...
		SetWindowText(hStatusLabel, U82T_c(C_("CacheTab|Win32",
			"ERROR: No fixed HDDs or SSDs detected.")));
		SendMessage(hProgressBar, PBM_SETSTATE, PBST_ERROR, 0);
		return 2;
	}

	// Open the registry key for the thumbnail cache cleaner.
	RegKey hKey(HKEY_LOCAL_MACHINE, _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\VolumeCaches\\Thumbnail Cache"), KEY_READ, false);
	if (!hKey.isOpen()) {
		// Failed to open the registry key.
		snprintf(errbuf, sizeof(errbuf), C_("CacheTab|Win32",
			"ERROR: Thumbnail Cache cleaner not found. (res == %ld)"),
			hKey.lOpenRes());
		SetWindowText(hStatusLabel, U82T_c(errbuf));
		SendMessage(hProgressBar, PBM_SETSTATE, PBST_ERROR, 0);
		return 3;
	}

	// Get the CLSID.
	const tstring s_clsidThumbnailCacheCleaner = hKey.read(nullptr);
	if (s_clsidThumbnailCacheCleaner.size() != 38) {
		// Not a CLSID.
		SetWindowText(hStatusLabel, U82T_c(C_("CacheTab|Win32",
			"ERROR: Thumbnail Cache cleaner CLSID is invalid.")));
		SendMessage(hProgressBar, PBM_SETSTATE, PBST_ERROR, 0);
		return 4;
	}
	CLSID clsidThumbnailCacheCleaner;
	HRESULT hr = CLSIDFromString(s_clsidThumbnailCacheCleaner.c_str(), &clsidThumbnailCacheCleaner);
	if (FAILED(hr)) {
		// Failed to convert the CLSID from string.
		SetWindowText(hStatusLabel, U82T_c(C_("CacheTab|Win32",
			"ERROR: Thumbnail Cache cleaner CLSID is invalid.")));
		SendMessage(hProgressBar, PBM_SETSTATE, PBST_ERROR, 0);
		return 5;
	}

	// Attempt to clear the cache on all non-removable hard drives.
	// TODO: Check mount points?
	// Reference: http://stackoverflow.com/questions/23677175/clean-windows-thumbnail-cache-programmatically
	IEmptyVolumeCachePtr pCleaner;
	hr = CoCreateInstance(clsidThumbnailCacheCleaner, nullptr,
		CLSCTX_INPROC_SERVER | CLSCTX_LOCAL_SERVER, IID_PPV_ARGS(&pCleaner));
	if (FAILED(hr)) {
		// Failed...
		snprintf(errbuf, sizeof(errbuf), C_("CacheTab|Win32",
			"ERROR: CoCreateInstance() failed. (hr == 0x%08X)"),
			static_cast<unsigned int>(hr));
		SetWindowText(hStatusLabel, U82T_c(errbuf));
		SendMessage(hProgressBar, PBM_SETSTATE, PBST_ERROR, 0);
		return 6;
	}

	// Disable the buttons until we're done.
	// TODO: Disable the main tab control too?
	HWND hClearSysThumbs = GetDlgItem(hWndPropSheet, IDC_CACHE_CLEAR_SYS_THUMBS);
	HWND hClearRpDl = GetDlgItem(hWndPropSheet, IDC_CACHE_CLEAR_RP_DL);
	EnableWindow(hClearSysThumbs, FALSE);
	EnableWindow(hClearRpDl, FALSE);
	SetCursor(LoadCursor(nullptr, IDC_WAIT));

	// Initialize the progress bar.
	SendMessage(hProgressBar, PBM_SETSTATE, PBST_NORMAL, 0);
	SendMessage(hProgressBar, PBM_SETRANGE, 0, MAKELONG(0, driveCount * 100));
	SendMessage(hProgressBar, PBM_SETPOS, 0, 0);

	// Thumbnail cache callback.
	RP_EmptyVolumeCacheCallback *const pCallback = new RP_EmptyVolumeCacheCallback(hWndPropSheet);

	// NOTE: IEmptyVolumeCache only supports Unicode strings.
	pCallback->m_baseProgress = 0;
	unsigned int clearCount = 0;	// Number of drives actually cleared. (S_OK)
	for (unsigned int bit = 0; bit < 26; bit++) {
		const uint32_t mask = (1U << bit);
		if (!(driveLetters & mask))
			continue;

		// TODO: Add operator HKEY() to RegKey?
		LPWSTR pwszDisplayName = nullptr;
		LPWSTR pwszDescription = nullptr;
		DWORD dwFlags = 0;
		szDrivePath[0] = L'A' + bit;
		hr = pCleaner->Initialize(hKey.handle(), szDrivePath,
			&pwszDisplayName, &pwszDescription, &dwFlags);

		// Free the display name and description,
		// since we don't need them.
		CoTaskMemFree(pwszDisplayName);
		CoTaskMemFree(pwszDescription);

		// Did initialization succeed?
		if (hr == S_FALSE) {
			// Nothing to delete.
			pCallback->m_baseProgress += 100;
			SendMessage(hProgressBar, PBM_SETPOS, pCallback->m_baseProgress, 0);
			continue;
		} else if (hr != S_OK) {
			// Some error occurred.
			// TODO: Continue with other drives?
			snprintf(errbuf, sizeof(errbuf), C_("CacheTab|Win32",
				"ERROR: IEmptyVolumeCache::Initialize() failed on drive %c. (hr == 0x%08X)"),
				(char)szDrivePath[0], static_cast<unsigned int>(hr));
			SetWindowText(hStatusLabel, U82T_c(errbuf));
			SendMessage(hProgressBar, PBM_SETSTATE, PBST_ERROR, 0);
			pCallback->Release();
			EnableWindow(hClearSysThumbs, TRUE);
			EnableWindow(hClearRpDl, TRUE);
			SetCursor(LoadCursor(nullptr, IDC_ARROW));
			MessageBeep(MB_ICONERROR);
			return 7;
		}

		// Clear the thumbnails.
		hr = pCleaner->Purge(-1LL, pCallback);
		if (FAILED(hr)) {
			// Cleanup failed. (TODO: Figure out why!)
			snprintf(errbuf, sizeof(errbuf), C_("CacheTab|Win32",
				"ERROR: IEmptyVolumeCache::Purge() failed on drive %c. (hr == 0x%08X)"),
				(char)szDrivePath[0], static_cast<unsigned int>(hr));
			SetWindowText(hStatusLabel, U82T_c(errbuf));
			SendMessage(hProgressBar, PBM_SETSTATE, PBST_ERROR, 0);
			pCallback->Release();
			EnableWindow(hClearSysThumbs, TRUE);
			EnableWindow(hClearRpDl, TRUE);
			SetCursor(LoadCursor(nullptr, IDC_ARROW));
			MessageBeep(MB_ICONERROR);
			return 8;
		}

		// Next drive.
		clearCount++;
		pCallback->m_baseProgress += 100;
		SendMessage(hProgressBar, PBM_SETPOS, pCallback->m_baseProgress, 0);
	}

	// TODO: SPI_SETICONS to clear the icon cache?
	const char *success_message;
	if (clearCount > 0) {
		success_message = C_("CacheTab", "System thumbnail cache cleared successfully.");
	} else {
		success_message = C_("CacheTab", "System thumbnail cache is already empty. Nothing to do here.");
	}
	SetWindowText(hStatusLabel, U82T_c(success_message));
	pCallback->Release();
	EnableWindow(hClearSysThumbs, TRUE);
	EnableWindow(hClearRpDl, TRUE);
	SetCursor(LoadCursor(nullptr, IDC_ARROW));
	MessageBeep(MB_ICONINFORMATION);
	return 0;
}

/**
* Clear the rom-properties cache.
* @return 0 on success; non-zero on error.
*/
int CacheTabPrivate::clearRomPropertiesCache(void)
{
	// TODO: Use a separate thread with callbacks?
	HWND hStatusLabel = GetDlgItem(hWndPropSheet, IDC_CACHE_STATUS);
	HWND hProgressBar = GetDlgItem(hWndPropSheet, IDC_CACHE_PROGRESS);
	ShowWindow(hStatusLabel, SW_SHOW);
	ShowWindow(hProgressBar, SW_SHOW);

	// Reset the progress bar.
	SendMessage(hProgressBar, PBM_SETSTATE, PBST_NORMAL, 0);
	SendMessage(hProgressBar, PBM_SETRANGE, 0, MAKELONG(0, 100));
	SendMessage(hProgressBar, PBM_SETPOS, 0, 0);

	// Cache directory.
	// Sanity check: Must be at least 8 characters.
	const string &cacheDir = FileSystem::getCacheDirectory();
	const size_t bscount = std::count_if(cacheDir.cbegin(), cacheDir.cend(),
		[](TCHAR chr) noexcept -> bool { return (chr == L'\\'); });

	if (cacheDir.size() < 8 || bscount < 6) {
		const string s_err = rp_sprintf(C_("CacheTab", "ERROR: %s"),
			C_("CacheCleaner", "Unable to get the rom-properties cache directory."));
		SetWindowText(hStatusLabel, U82T_s(s_err));
		SendMessage(hProgressBar, PBM_SETRANGE, 0, MAKELONG(0, 1));
		SendMessage(hProgressBar, PBM_SETPOS, 1, 0);
		// FIXME: Not working...
		SendMessage(hProgressBar, PBM_SETSTATE, PBST_ERROR, 0);
		MessageBeep(MB_ICONERROR);
		return 0;
	}

	// NOTE: FileSystem::getCacheDirectory() doesn't have "\\\\?\\"
	// prepended, since we don't want to display this to the user.
	// RpFile_Win32 normally prepends it automatically, but we're not
	// using that here.
	tstring cacheDirT = _T("\\\\?\\");
	cacheDirT += U82T_s(cacheDir);

	// Disable the buttons until we're done.
	// TODO: Disable the main tab control too?
	HWND hClearSysThumbs = GetDlgItem(hWndPropSheet, IDC_CACHE_CLEAR_SYS_THUMBS);
	HWND hClearRpDl = GetDlgItem(hWndPropSheet, IDC_CACHE_CLEAR_RP_DL);
	EnableWindow(hClearSysThumbs, FALSE);
	EnableWindow(hClearRpDl, FALSE);
	SetCursor(LoadCursor(nullptr, IDC_WAIT));

	SetWindowText(hStatusLabel, U82T_c(C_("CacheTab", "Clearing the rom-properties cache...")));

	// Initialize the progress bar.
	// TODO: Before or after scanning?
	SendMessage(hProgressBar, PBM_SETSTATE, PBST_NORMAL, 0);
	SendMessage(hProgressBar, PBM_SETRANGE, 0, 1);
	SendMessage(hProgressBar, PBM_SETPOS, 0, 0);

	// Does the cache directory exist?
	// If it doesn't, we'll act like it's empty.
	if (FileSystem::taccess(cacheDirT.c_str(), R_OK) != 0) {
		// Unable to read the directory. Assume it's missing.
		SetWindowText(hStatusLabel, U82T_c(C_("CacheTab", "rom-properties cache is empty. Nothing to do.")));
		SendMessage(hProgressBar, PBM_SETRANGE, 0, MAKELONG(0, 1));
		SendMessage(hProgressBar, PBM_SETPOS, 1, 0);
		EnableWindow(hClearSysThumbs, TRUE);
		EnableWindow(hClearRpDl, TRUE);
		SetCursor(LoadCursor(nullptr, IDC_ARROW));
		MessageBeep(MB_ICONINFORMATION);
		return 0;
	}

	// Recursively scan the cache directory.
	// TODO: Do we really want to store everything in a list? (Wastes memory.)
	// Maybe do a simple counting scan first, then delete.
	forward_list<pair<tstring, uint8_t> > rlist;
	int ret = recursiveScan(cacheDirT.c_str(), rlist);
	if (ret != 0) {
		// Non-image file found.
		const string s_err = rp_sprintf(C_("CacheTab", "ERROR: %s"),
			C_("CacheCleaner", "rom-properties cache has unexpected files. Not clearing it."));
		SetWindowText(hStatusLabel, U82T_s(s_err));
		SendMessage(hProgressBar, PBM_SETRANGE, 0, MAKELONG(0, 1));
		SendMessage(hProgressBar, PBM_SETPOS, 1, 0);
		EnableWindow(hClearSysThumbs, TRUE);
		EnableWindow(hClearRpDl, TRUE);
		SetCursor(LoadCursor(nullptr, IDC_ARROW));
		// FIXME: Not working...
		SendMessage(hProgressBar, PBM_SETSTATE, PBST_ERROR, 0);
		MessageBeep(MB_ICONERROR);
		return 0;
	} else if (rlist.empty()) {
		// Nothing to do!
		SetWindowText(hStatusLabel, U82T_c(C_("CacheTab", "rom-properties cache is empty. Nothing to do.")));
		SendMessage(hProgressBar, PBM_SETRANGE, 0, MAKELONG(0, 1));
		SendMessage(hProgressBar, PBM_SETPOS, 1, 0);
		EnableWindow(hClearSysThumbs, TRUE);
		EnableWindow(hClearRpDl, TRUE);
		SetCursor(LoadCursor(nullptr, IDC_ARROW));
		MessageBeep(MB_ICONINFORMATION);
		return 0;
	}

	// NOTE: std::forward_list doesn't have size().
	const size_t rlist_size = std::distance(rlist.cbegin(), rlist.cend());

	// Delete all of the files and subdirectories.
	SendMessage(hProgressBar, PBM_SETRANGE32, 0, rlist_size);
	SendMessage(hProgressBar, PBM_SETPOS, 2, 0);
	unsigned int count = 0;
	unsigned int dirErrs = 0, fileErrs = 0;
	for (const auto &p : rlist) {
		switch (p.second) {
			case DT_DIR: {
				// Remove the directory.
				BOOL bRet = RemoveDirectory(p.first.c_str());
				if (!bRet) {
					dirErrs++;
					SendMessage(hProgressBar, PBM_SETSTATE, PBST_ERROR, 0);
				}
				break;
			}

			case DT_REG: {
				// Delete the file.
				BOOL bRet = TRUE;
				if (p.second & FILE_ATTRIBUTE_READONLY) {
					// Need to remove the read-only attribute.
					bRet = SetFileAttributes(p.first.c_str(), (p.second & ~FILE_ATTRIBUTE_READONLY));
				}
				if (!bRet) {
					// Error removing the read-only attribute.
					fileErrs++;
					SendMessage(hProgressBar, PBM_SETSTATE, PBST_ERROR, 0);
				} else {
					bRet = DeleteFile(p.first.c_str());
					if (!bRet) {
						// Error deleting the file.
						fileErrs++;
						SendMessage(hProgressBar, PBM_SETSTATE, PBST_ERROR, 0);
					}
				}
				break;
			}

			default:
				// FIXME: Other type of file?
				fileErrs++;
				SendMessage(hProgressBar, PBM_SETSTATE, PBST_ERROR, 0);
				break;
		}

		// TODO: Restrict update frequency to X number of files/directories?
		count++;
		SendMessage(hProgressBar, PBM_SETPOS, count, 0);
	}

	if (dirErrs > 0 || fileErrs > 0) {
		char buf[256];
		snprintf_p(buf, sizeof(buf), C_("CacheTab", "Unable to delete %1$u file(s) and/or %2$u dir(s)."),
			fileErrs, dirErrs);
		const string s_err = rp_sprintf(C_("CacheTab", "ERROR: %s"), buf);
		SetWindowText(hStatusLabel, U82T_s(s_err));
		MessageBeep(MB_ICONWARNING);
	} else {
		SetWindowText(hStatusLabel, U82T_c(C_("CacheTab", "rom-properties cache cleared successfully.")));
		MessageBeep(MB_ICONINFORMATION);
	}

	EnableWindow(hClearSysThumbs, TRUE);
	EnableWindow(hClearRpDl, TRUE);
	SetCursor(LoadCursor(nullptr, IDC_ARROW));
	return 0;
}

/**
 * Dialog procedure.
 * @param hDlg
 * @param uMsg
 * @param wParam
 * @param lParam
 */
INT_PTR CALLBACK CacheTabPrivate::dlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
		case WM_INITDIALOG: {
			// Get the pointer to the property sheet page object. This is 
			// contained in the LPARAM of the PROPSHEETPAGE structure.
			LPPROPSHEETPAGE pPage = reinterpret_cast<LPPROPSHEETPAGE>(lParam);
			if (!pPage)
				return TRUE;

			// Get the pointer to the CacheTabPrivate object.
			CacheTabPrivate *const d = reinterpret_cast<CacheTabPrivate*>(pPage->lParam);
			if (!d)
				return TRUE;

			assert(d->hWndPropSheet == nullptr);
			d->hWndPropSheet = hDlg;

			// Store the D object pointer with this particular page dialog.
			SetWindowLongPtr(hDlg, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(d));

			// NOTE: This should be in WM_CREATE, but we don't receive WM_CREATE here.
			DarkMode_InitDialog(hDlg);
			d->lastDarkModeEnabled = g_darkModeEnabled;

			// Initialize the dialog.
			d->initDialog();
			return TRUE;
		}

		case WM_NOTIFY: {
			auto *const d = reinterpret_cast<CacheTabPrivate*>(GetWindowLongPtr(hDlg, GWLP_USERDATA));
			if (!d) {
				// No CacheTabPrivate. Can't do anything...
				return FALSE;
			}

			NMHDR *pHdr = reinterpret_cast<NMHDR*>(lParam);
			switch (pHdr->code) {
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
			auto *const d = reinterpret_cast<CacheTabPrivate*>(GetWindowLongPtr(hDlg, GWLP_USERDATA));
			if (!d) {
				// No CacheTabPrivate. Can't do anything...
				return FALSE;
			}

			if (HIWORD(wParam) != BN_CLICKED)
				break;

			switch (LOWORD(wParam)) {
				case IDC_CACHE_CLEAR_SYS_THUMBS:
					// Clear the system thumbnail cache. (Vista+)
					d->clearThumbnailCacheVista();
					return TRUE;
				case IDC_CACHE_CLEAR_RP_DL:
					// Clear the rom-properties cache.
					d->clearRomPropertiesCache();
					return TRUE;
				case IDC_CACHE_XP_CLEAR_SYS_THUMBS:
					// Clear the system thumbnail cache. (XP)
					// TODO
					break;
				case IDC_CACHE_XP_FIND_DRIVES:
					ShowWindow(GetDlgItem(d->hWndPropSheet, IDC_CACHE_XP_DRIVES), SW_SHOW);
					ShowWindow(GetDlgItem(d->hWndPropSheet, IDC_CACHE_XP_PATH), SW_HIDE);
					ShowWindow(GetDlgItem(d->hWndPropSheet, IDC_CACHE_XP_BROWSE), SW_HIDE);
					return TRUE;
				case IDC_CACHE_XP_FIND_PATH:
					ShowWindow(GetDlgItem(d->hWndPropSheet, IDC_CACHE_XP_DRIVES), SW_HIDE);
					ShowWindow(GetDlgItem(d->hWndPropSheet, IDC_CACHE_XP_PATH), SW_SHOW);
					ShowWindow(GetDlgItem(d->hWndPropSheet, IDC_CACHE_XP_BROWSE), SW_SHOW);
					return TRUE;
				default:
					break;
			}
			break;
		}

		case WM_DEVICECHANGE: {
			auto *const d = reinterpret_cast<CacheTabPrivate*>(GetWindowLongPtr(hDlg, GWLP_USERDATA));
			if (!d || d->isVista) {
				// No CacheTabPrivate, or using Vista+.
				// Nothing to do here.
				break;
			}

			if (wParam != DBT_DEVICEARRIVAL && wParam != DBT_DEVICEREMOVECOMPLETE)
				break;

			// Device is being added or removed.
			// Update the device in the drive list.
			const PDEV_BROADCAST_HDR lpdb = reinterpret_cast<PDEV_BROADCAST_HDR>(lParam);
			if (lpdb->dbch_devicetype == DBT_DEVTYP_VOLUME) {
				const PDEV_BROADCAST_VOLUME lpdbv = reinterpret_cast<PDEV_BROADCAST_VOLUME>(lParam);
				// Schedule an update after a second to allow the drive to stabilize.
				// TODO: Instead of waiting 1 second, just keep retrying
				// SHGetFileInfo() until it succeeds? (media change only)
				d->dwUnitmaskXP |= lpdbv->dbcv_unitmask;
				SetTimer(hDlg, TMRID_XP_DRIVE_UPDATE, 1000, nullptr);
			}
			return TRUE;
		}

		case WM_TIMER: {
			if (wParam != TMRID_XP_DRIVE_UPDATE)
				break;
			auto *const d = reinterpret_cast<CacheTabPrivate*>(GetWindowLongPtr(hDlg, GWLP_USERDATA));
			if (!d || d->isVista) {
				// No CacheTabPrivate, or using Vista+.
				// Nothing to do here.
				return FALSE;
			}

			// Update the drives.
			DWORD unitmask = 0;
			std::swap(unitmask, d->dwUnitmaskXP);
			d->updateDrivesXP(unitmask);
			return TRUE;
		}

		case WM_WTSSESSION_CHANGE: {
			auto *const d = reinterpret_cast<CacheTabPrivate*>(GetWindowLongPtr(hDlg, GWLP_USERDATA));
			if (!d || d->isVista) {
				// No CacheTabPrivate, or using Vista+.
				// Nothing to do here.
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

		/** Dark Mode **/

		case WM_CTLCOLORDLG:
		case WM_CTLCOLORSTATIC:
			if (g_darkModeSupported && g_darkModeEnabled) {
				auto *const d = reinterpret_cast<CacheTabPrivate*>(GetWindowLongPtr(hDlg, GWLP_USERDATA));
				if (!d) {
					// No CacheTabPrivate. Can't do anything...
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

		case WM_THEMECHANGED:
			if (g_darkModeSupported) {
				auto *const d = reinterpret_cast<CacheTabPrivate*>(GetWindowLongPtr(hDlg, GWLP_USERDATA));
				if (!d) {
					// No ImageTypesTabPrivate. Can't do anything...
					return FALSE;
				}

				UpdateDarkModeEnabled();
				if (d->lastDarkModeEnabled != g_darkModeEnabled) {
					d->lastDarkModeEnabled = g_darkModeEnabled;
					InvalidateRect(hDlg, NULL, true);

					// Propagate WM_THEMECHANGED to window controls that don't
					// automatically handle Dark Mode changes, e.g. ComboBox and Button.
					// NOTE: If Dark Mode is supported, then we're definitely
					// running on Windows 10 or later, so this will have the
					// Windows Vista layout.
					SendMessage(GetDlgItem(hDlg, IDC_CACHE_CLEAR_SYS_THUMBS), WM_THEMECHANGED, 0, 0);
					SendMessage(GetDlgItem(hDlg, IDC_CACHE_CLEAR_RP_DL), WM_THEMECHANGED, 0, 0);
					// TODO: Progress bar?
				}
			}
			break;

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
UINT CALLBACK CacheTabPrivate::callbackProc(HWND hWnd, UINT uMsg, LPPROPSHEETPAGE ppsp)
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

/** CacheTab **/

CacheTab::CacheTab(void)
	: d_ptr(new CacheTabPrivate())
{}

CacheTab::~CacheTab()
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
HPROPSHEETPAGE CacheTab::getHPropSheetPage(void)
{
	RP_D(CacheTab);
	assert(d->hPropSheetPage == nullptr);
	if (d->hPropSheetPage) {
		// Property sheet has already been created.
		return nullptr;
	}

	// tr: Tab title.
	const tstring tsTabTitle = U82T_c(C_("CacheTab", "Thumbnail Cache"));

	PROPSHEETPAGE psp;
	psp.dwSize = sizeof(psp);
	psp.dwFlags = PSP_USECALLBACK | PSP_USETITLE | PSP_DLGINDIRECT;
	psp.hInstance = HINST_THISCOMPONENT;
	psp.pszIcon = nullptr;
	psp.pszTitle = tsTabTitle.c_str();
	psp.pfnDlgProc = CacheTabPrivate::dlgProc;
	psp.lParam = reinterpret_cast<LPARAM>(d);
	psp.pcRefParent = nullptr;
	psp.pfnCallback = CacheTabPrivate::callbackProc;

	const unsigned int resId = likely(d->isVista) ? IDD_CONFIG_CACHE : IDD_CONFIG_CACHE_XP;
	psp.pResource = LoadDialog_i18n(HINST_THISCOMPONENT, resId);

	d->hPropSheetPage = CreatePropertySheetPage(&psp);
	return d->hPropSheetPage;
}
