/***************************************************************************
 * ROM Properties Page shell extension installer. (svrplus)                *
 * svrplus.c: Win32 installer for rom-properties.                          *
 *                                                                         *
 * Copyright (c) 2017-2018 by Egor.                                        *
 * Copyright (c) 2017-2018 by David Korth.                                 *
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
 * You should have received a copy of the GNU General Public License       *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.   *
 ***************************************************************************/

// C includes.
#include <assert.h>
#include <errno.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>
// MSVCRT-specific
#include <process.h>

// libwin32common
#include "libwin32common/RpWin32_sdk.h"
#include "libwin32common/secoptions.h"

// Additional Windows headers.
#include <windowsx.h>
#include <shellapi.h>
#include <commctrl.h>

// Application resources.
#include "resource.h"

/**
 * Number of elements in an array.
 * (from librpbase/common.h)
 *
 * Includes a static check for pointers to make sure
 * a dynamically-allocated array wasn't specified.
 * Reference: http://stackoverflow.com/questions/8018843/macro-definition-array-size
 */
#define ARRAY_SIZE(x) \
	((int)(((sizeof(x) / sizeof(x[0]))) / \
		(size_t)(!(sizeof(x) % sizeof(x[0])))))

// TODO: Move to our own stdboolx.h file.
#ifndef __cplusplus
# if defined(_MSC_VER) && _MSC_VER >= 1800
#  include <stdbool.h>
# else
typedef unsigned char bool;
#  define true 1
#  define false 0
# endif
#endif /* __cplusplus */

// File paths
static const wchar_t str_rp32path[] = L"i386\\rom-properties.dll";
static const wchar_t str_rp64path[] = L"amd64\\rom-properties.dll";

// Globals

#ifdef _WIN64
static const bool g_is64bit = true;	/**< true if running on 64-bit system */
#else /* !_WIN64 */
bool g_is64bit = false;			/**< true if running on 64-bit system */
#endif
bool g_inProgress = false;		/**< true if currently (un)installing the DLLs */

// Custom messages
#define WM_APP_ENDTASK WM_APP

// Icons. (NOTE: These MUST be deleted after use!)
static HICON hIconDialog = NULL;
static HICON hIconDialogSmall = NULL;

// System 16x16 icons. (Do NOT delete these!)
static HICON hIconExclaim = NULL;	// USER32.dll,-101
//static HICON hIconQuestion = NULL;	// USER32.dll,-102
static HICON hIconCritical = NULL;	// USER32.dll,-103
static HICON hIconInfo = NULL;		// USER32.dll,-104

// IDC_STATIC_STATUS1 rectangles with and without the exclamation point icon.
static RECT rectStatus1_noIcon;
static RECT rectStatus1_icon;

/**
 * Show a status message.
 * @param hDlg Dialog.
 * @param line1 Line 1. (If NULL, status message will be hidden.)
 * @param line2 Line 2. (May contain up to 3 lines and have links.)
 * @param uType Icon type. (Use MessageBox constants.)
 */
static void ShowStatusMessage(HWND hDlg, const wchar_t *line1, const wchar_t *line2, UINT uType)
{
	HICON hIcon;
	int sw_status;
	const RECT *rect;

	HWND const hStaticIcon = GetDlgItem(hDlg, IDC_STATIC_ICON);
	HWND const hStatus1 = GetDlgItem(hDlg, IDC_STATIC_STATUS1);
	HWND const hStatus2 = GetDlgItem(hDlg, IDC_STATIC_STATUS2);

	if (!line1) {
		// No status message.
		ShowWindow(hStatus1, SW_HIDE);
		ShowWindow(hStatus2, SW_HIDE);
		return;
	}

	// Determine the icon to use.
	switch (uType & 0x70) {
		case MB_ICONSTOP:
			hIcon = hIconCritical;
			break;
		case MB_ICONQUESTION:	// TODO?
		case MB_ICONINFORMATION:
			hIcon = hIconInfo;
			break;
		case MB_ICONEXCLAMATION:
			hIcon = hIconExclaim;
			break;
		default:
			hIcon = NULL;
			break;
	}

	if (hIcon != NULL) {
		// Show the icon.
		sw_status = SW_SHOW;
		rect = &rectStatus1_icon;
		Static_SetIcon(hStaticIcon, hIcon);
	} else {
		// Hide the icon.
		sw_status = SW_HIDE;
		rect = &rectStatus1_noIcon;
	}

	// Adjust the icon and Status1 rectangle.
	ShowWindow(hStaticIcon, sw_status);
	SetWindowPos(hStatus1, NULL, rect->left, rect->top,
		rect->right - rect->left, rect->bottom - rect->top,
		SWP_NOACTIVATE | SWP_NOREDRAW | SWP_NOZORDER);

	SetWindowText(hStatus1, line1);
	SetWindowText(hStatus2, line2);
	ShowWindow(hStatus1, SW_SHOW);
	ShowWindow(hStatus2, SW_SHOW);
}

/**
 * Enable/disable the Install/Uninstall buttons.
 * @param hDlg Dialog.
 * @param enable True to enable; false to disable.
 */
static inline void EnableButtons(HWND hDlg, bool enable)
{
	EnableWindow(GetDlgItem(hDlg, IDC_BUTTON_INSTALL), enable);
	EnableWindow(GetDlgItem(hDlg, IDC_BUTTON_UNINSTALL), enable);
}

/**
 * Get a pathname in the specified System directory.
 * @param pszPath	[out] Path buffer.
 * @param cchPath	[in] Size of path buffer, in characters.
 * @param filename	[in] Filename to append. (Should be at least MAX_PATH.)
 * @param is64		[in] If true, use the 64-bit System directory.
 * @return Length of path in characters, excluding NULL terminator, on success; negative POSIX error code on error.
 */
static int GetSystemDirFilePath(wchar_t *pszPath, size_t cchPath, const wchar_t *filename, bool is64)
{
	unsigned int len, cchFilename;

	// Get the Windows directory first.
	len = GetWindowsDirectory(pszPath, (UINT)cchPath);
	assert(len != 0);
	if (len == 0) {
		// Unable to get the Windows directory.
		return -EIO;
	} else if (len >= (cchPath-2)) {
		// Not enough space for the Windows directory.
		return -ENOMEM;
	}

	// Make sure the path ends with a backslash.
	if (pszPath[len-1] != '\\') {
		pszPath[len] = '\\';
		pszPath[len+1] = 0;
		len++;
	}

	// Make sure we have enough space for the System directory and filename.
	cchFilename = (unsigned int)wcslen(filename);
	if (len + cchFilename + 11 >= cchPath) {
		// Not enough space.
		return -ENOMEM;
	}

	// Append the System directory name.
#ifdef _WIN64
	wcscpy_s(&pszPath[len], cchPath-len, (is64 ? L"System32\\" : L"SysWOW64\\"));
	len += 9;
#else /* !_WIN64 */
	wcscpy_s(&pszPath[len], cchPath-len, (is64 ? L"Sysnative\\" : L"System32\\"));
	len += (is64 ? 10 : 9);
#endif /* _WIN64 */

	// Append the filename.
	wcscpy_s(&pszPath[len], cchPath-len, filename);
	return len + cchFilename;
}

/**
 * Check if a file exists.
 * @param filename Filename.
 * @return True if the file exists; false if not.
 */
static inline bool fileExists(const wchar_t *filename)
{
	return (GetFileAttributes(filename) != INVALID_FILE_ATTRIBUTES);
}

/**
 * Checks whether or not the Visual C++ runtime is installed.
 *
 * @param is64 when true, checks for 64-bit version
 * @return true if installed
 */
static bool CheckMsvc(bool is64)
{
	// Determine the MSVCRT DLL name.
	wchar_t msvcrt_path[MAX_PATH];
	int ret = GetSystemDirFilePath(msvcrt_path, ARRAY_SIZE(msvcrt_path), L"msvcp140.dll", is64);
	if (ret <= 0) {
		// Unable to get the path.
		// Assume the file exists.
		return true;
	}

	// Check if the file exists.
	return fileExists(msvcrt_path);
}

typedef enum {
	ISR_FATAL_ERROR = -1,		// Error that should never happen.
	ISR_OK = 0,
	ISR_FILE_NOT_FOUND,
	ISR_CREATEPROCESS_FAILED,	// errorCode is GetLastError()
	ISR_PROCESS_STILL_ACTIVE,
	ISR_REGSVR32_EXIT_CODE,		// errorCode is REGSVR32's exit code.
} InstallServerResult;

// References:
// - http://stackoverflow.com/questions/22094309/regsvr32-exit-codes-documentation
// - http://stackoverflow.com/a/22095500
typedef enum {
	REGSVR32_OK		= 0,
	REGSVR32_FAIL_ARGS	= 1, // Invalid argument.
	REGSVR32_FAIL_OLE	= 2, // OleInitialize() failed.
	REGSVR32_FAIL_LOAD	= 3, // LoadLibrary() failed.
	REGSVR32_FAIL_ENTRY	= 4, // GetProcAddress() failed.
	REGSVR32_FAIL_REG	= 5, // DllRegisterServer() or DllUnregisterServer() failed.
} RegSvr32ExitCode;

/**
 * (Un)installs the COM Server DLL.
 *
 * @param isUninstall	[in] When true, uninstalls the DLL, instead of installing it.
 * @param is64		[in] When true, installs 64-bit version.
 * @param pErrorCode	[out,opt] Additional error code.
 * @return InstallServerResult
 */
static InstallServerResult InstallServer(bool isUninstall, bool is64, DWORD *pErrorCode)
{
	wchar_t regsvr32_path[MAX_PATH];
	wchar_t args[14 + MAX_PATH + 4 + 3 + ARRAY_SIZE(str_rp64path)] = L"regsvr32.exe \"";
	DWORD szModuleFn;
	wchar_t *bs;

	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	DWORD status;

	// Determine the REGSVR32 path.
	int ret = GetSystemDirFilePath(regsvr32_path, ARRAY_SIZE(regsvr32_path), L"regsvr32.exe", is64);
	if (ret <= 0) {
		// Unable to get the path.
		return ISR_FATAL_ERROR;
	}

	// Construct arguments
	// Construct path to rom-properties.dll inside the arguments
	szModuleFn = GetModuleFileName(HINST_THISCOMPONENT, &args[14], MAX_PATH);
	assert(szModuleFn != 0);
	assert(szModuleFn < MAX_PATH);
	if (szModuleFn == 0 || szModuleFn >= MAX_PATH) {
		// TODO: add an error message for the MAX_PATH case?
		return ISR_FATAL_ERROR;
	}

	// Find the last backslash in the module filename.
	bs = wcsrchr(args, L'\\');
	if (!bs) {
		// No backslashes...
		return ISR_FATAL_ERROR;
	}

	// Remove the EXE filename, then append the DLL relative path.
	bs[1] = 0;
	wcscat_s(args, ARRAY_SIZE(args), is64 ? str_rp64path : str_rp32path);
	if (!fileExists(&args[14])) {
		// File not found.
		return ISR_FILE_NOT_FOUND;
	}

	// Append /s (silent) key, and optionally append /u (uninstall) key.
	wcscat_s(args, ARRAY_SIZE(args), isUninstall ? L"\" /s /u" : L"\" /s");

	memset(&si, 0, sizeof(si));
	memset(&pi, 0, sizeof(pi));
	si.cb = sizeof(si);

	if (!CreateProcess(regsvr32_path, args, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
		if (pErrorCode) {
			*pErrorCode = GetLastError();
		}
		return ISR_CREATEPROCESS_FAILED;
	}

	// Wait for the process to exit.
	WaitForSingleObject(pi.hProcess, INFINITE);
	GetExitCodeProcess(pi.hProcess, &status);
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);

	switch (status) {
		case STILL_ACTIVE:
			// Process is still active...
			return ISR_PROCESS_STILL_ACTIVE;

		case 0:
			// Process completed.
			break;

		default:
			// Process returned a non-zero exit code.
			if (pErrorCode) {
				*pErrorCode = status;
			}
			return ISR_REGSVR32_EXIT_CODE;
	}

	// Process completed.
	return ISR_OK;
}

/**
 * Tries to (un)install the COM Server DLL, displays errors in message boxes.
 *
 * @param hWnd		[in] owner window of message boxes
 * @param isUninstall	[in] when true, uninstalls the DLL, instead of installing it.
 * @param is64		[in] when true, installs 64-bit version
 * @param sErrBuf	[out,opt] error message buffer
 * @param cchErrBuf	[in,opt] size of sErrBuf, in characters
 * @return InstallServerResult
 */
static InstallServerResult TryInstallServer(HWND hWnd,
	bool isUninstall, bool is64,
	wchar_t *sErrBuf, size_t cchErrBuf)
{
	const wchar_t *dll_path, *entry_point;
	DWORD errorCode;
	InstallServerResult res = InstallServer(isUninstall, is64, &errorCode);

	if (!sErrBuf) {
		// No error message buffer specified.
		return res;
	}

	dll_path = (is64 ? str_rp64path : str_rp32path);
	entry_point = (isUninstall
		? L"DllUnregisterServer"
		: L"DllRegisterServer");

	// NOTE: Using wcscpy_s() instead of wcsncpy() because
	// wcsncpy() zeroes the rest of the buffer.
	switch (res) {
		case ISR_OK:
			// No error.
			if (cchErrBuf > 0) {
				sErrBuf[0] = L'\0';
			}
			break;
		case ISR_FATAL_ERROR:
		default:
			wcscpy_s(sErrBuf, cchErrBuf, L"An unknown fatal error occurred.");
			break;
		case ISR_FILE_NOT_FOUND:
			swprintf(sErrBuf, cchErrBuf, L"%s is missing.", dll_path);
			break;
		case ISR_CREATEPROCESS_FAILED:
			swprintf(sErrBuf, cchErrBuf, L"Could not start REGSVR32.exe. (Err:%u)", errorCode);
			break;
		case ISR_PROCESS_STILL_ACTIVE:
			wcscpy_s(sErrBuf, cchErrBuf, L"The REGSVR32 process never completed.");
			break;
		case ISR_REGSVR32_EXIT_CODE:
			switch (errorCode) {
				case REGSVR32_FAIL_ARGS:
					wcscpy_s(sErrBuf, cchErrBuf, L"REGSVR32 failed: Invalid argument.");
					break;
				case REGSVR32_FAIL_OLE:
					wcscpy_s(sErrBuf, cchErrBuf, L"REGSVR32 failed: OleInitialize() failed.");
					break;
				case REGSVR32_FAIL_LOAD:
					swprintf(sErrBuf, cchErrBuf, L"REGSVR32 failed: %s is not a valid DLL.", dll_path);
					break;
				case REGSVR32_FAIL_ENTRY:
					swprintf(sErrBuf, cchErrBuf, L"REGSVR32 failed: %s is missing %s().",
						dll_path, entry_point);
					break;
				case REGSVR32_FAIL_REG:
					swprintf(sErrBuf, cchErrBuf, L"REGSVR32 failed: %s() returned an error.",
						entry_point);
					break;
				default:
					swprintf(sErrBuf, cchErrBuf, L"REGSVR32 failed: Unknown exit code %u.", errorCode);
					break;
			}
			break;
	}

	return res;
}

typedef struct _ThreadParams {
	HWND hWnd;		/**< Window that created the thread */
	bool isUninstall;	/**< true if uninstalling */
} ThreadParams;

/**
 * Worker thread procedure.
 *
 * @param lpParameter ptr to parameters of type ThreadPrams. Owned by this thread.
 */
static unsigned int WINAPI ThreadProc(LPVOID lpParameter)
{
	wchar_t msg32[256], msg64[256];
	InstallServerResult res32 = ISR_OK, res64 = ISR_OK;

	ThreadParams *const params = (ThreadParams*)lpParameter;

	// Try to (un)install the 64-bit version.
	if (g_is64bit) {
		res64 = TryInstallServer(params->hWnd, params->isUninstall, true, msg64, ARRAY_SIZE(msg64));
	}

	// Try to (un)install the 32-bit version.
	res32 = TryInstallServer(params->hWnd, params->isUninstall, false, msg32, ARRAY_SIZE(msg32));

	if (res32 == ISR_OK && res64 == ISR_OK) {
		// DLL(s) registered successfully.
		const wchar_t *msg;
		if (g_is64bit) {
			msg = (params->isUninstall
				? L"DLLs unregistered successfully."
				: L"DLLs registered successfully.");
		} else {
			msg = (params->isUninstall
				? L"DLL unregistered successfully."
				: L"DLL registered successfully.");
		}
		ShowStatusMessage(params->hWnd, msg, L"", MB_ICONINFORMATION);
		MessageBeep(MB_ICONINFORMATION);
	} else {
		// At least one of the DLLs failed to register.
		const wchar_t *msg1;
		wchar_t msg2[540];
		msg2[0] = L'\0';

		if (g_is64bit) {
			msg1 = (params->isUninstall
				? L"An error occurred while unregistering the DLLs:"
				: L"An error occurred while registering the DLLs:");
		} else {
			msg1 = (params->isUninstall
				? L"An error occurred while unregistering the DLL:"
				: L"An error occurred while registering the DLL:");
		}

		// TODO: Optimize by not using wcscat_s()?
		if (res32 != ISR_OK) {
			if (g_is64bit) {
				wcscpy_s(msg2, ARRAY_SIZE(msg2), L"\x2022 32-bit: ");
			}
			wcscat_s(msg2, ARRAY_SIZE(msg2), msg32);
		}
		if (res64 != ISR_OK) {
			if (msg2[0] != L'\0') {
				wcscat_s(msg2, ARRAY_SIZE(msg2), L"\n");
			}
			wcscat_s(msg2, ARRAY_SIZE(msg2), L"\x2022 64-bit: ");
			wcscat_s(msg2, ARRAY_SIZE(msg2), msg64);
		}

		ShowStatusMessage(params->hWnd, msg1, msg2, MB_ICONSTOP);
		MessageBeep(MB_ICONSTOP);
	}

	SendMessage(params->hWnd, WM_APP_ENDTASK, 0, 0);
	free(params);
	return 0;
}

/**
 * Changes the cursor depending on wether installation is in progress.
 */
static inline void DlgUpdateCursor(void)
{
	SetCursor(LoadCursor(NULL, g_inProgress ? IDC_WAIT : IDC_ARROW));
}

/**
 * Initialize the dialog.
 * @param hDlg Dialog handle.
 */
static void InitDialog(HWND hDlg)
{
	// FIXME: Assuming 16x16 icons. May need larger for HiDPI.
	static const SIZE szIcon = {16, 16};

	// Main dialog description.
	static const wchar_t strdlg_desc[] =
		L"This installer will register the ROM Properties Page DLL with the system, "
		L"which will provide extra functionality for supported files in Windows Explorer.\n\n"
		L"Note that the DLL locations are hard-coded in the registry. If you move the DLLs, "
		L"you will have to rerun this installer. In addition, the DLLs will usually be locked "
		L"by Explorer, so you will need to use this program to uninstall the DLLs first and "
		L"then restart Explorer in order to move the DLLs.\n\n"
		L"Uninstalling will unregister the ROM Properties DLL, which will disable the extra "
		L"functionality provided by the DLL for supported ROM files.";

	HWND hStatus1, hExclaim;
	HMODULE hUser32;
	bool bHasMsvc32, bErr;
	const wchar_t *line1 = NULL, *line2 = NULL;

	if (hIconDialog) {
		SendMessage(hDlg, WM_SETICON, ICON_BIG, (LPARAM)hIconDialog);
	}
	if (hIconDialogSmall) {
		SendMessage(hDlg, WM_SETICON, ICON_SMALL, (LPARAM)hIconDialogSmall);
	}

	// Get Status1's dimensions.
	hStatus1 = GetDlgItem(hDlg, IDC_STATIC_STATUS1);
	GetWindowRect(hStatus1, &rectStatus1_noIcon);
	MapWindowPoints(HWND_DESKTOP, hDlg, (LPPOINT)&rectStatus1_noIcon, 2);

	// Adjust the left boundary for the icon.
	// FIXME: Assuming 16x16 icons. May need larger for HiDPI.
	rectStatus1_icon = rectStatus1_noIcon;
	rectStatus1_icon.left += szIcon.cx + (szIcon.cx / 5);

	// Load the icons.
	// NOTE: Using IDI_EXCLAMATION will only return the 32x32 icon.
	// Need to get the icon from USER32 directly.
	hUser32 = GetModuleHandle(L"user32");
	assert(hUser32 != NULL);
	if (hUser32) {
		hIconExclaim = (HICON)LoadImage(hUser32,
			MAKEINTRESOURCE(101), IMAGE_ICON,
			szIcon.cx, szIcon.cy, LR_SHARED);
		/*hIconQuestion = (HICON)LoadImage(hUser32,
			MAKEINTRESOURCE(102), IMAGE_ICON,
			szIcon.cx, szIcon.cy, LR_SHARED);*/
		hIconCritical = (HICON)LoadImage(hUser32,
			MAKEINTRESOURCE(103), IMAGE_ICON,
			szIcon.cx, szIcon.cy, LR_SHARED);
		hIconInfo = (HICON)LoadImage(hUser32,
			MAKEINTRESOURCE(104), IMAGE_ICON,
			szIcon.cx, szIcon.cy, LR_SHARED);
	}

	// Initialize the icon control.
	hExclaim = CreateWindowEx(WS_EX_NOPARENTNOTIFY | WS_EX_TRANSPARENT,
		WC_STATIC, NULL,
		WS_CHILD | WS_CLIPSIBLINGS | SS_ICON,
		rectStatus1_noIcon.left, rectStatus1_noIcon.top-1,
		szIcon.cx, szIcon.cy,
		hDlg, (HMENU)IDC_STATIC_ICON, NULL, NULL);

	// FIXME: Figure out the SysLink styles. (0x50010000?)
	ShowWindow(GetDlgItem(hDlg, IDC_STATIC_STATUS2), SW_HIDE);

	// Set the dialog strings.
	SetWindowText(GetDlgItem(hDlg, IDC_STATIC_DESC), strdlg_desc);

	// Check if MSVCRT is installed. If it isn't, show a warning
	// message and disable the buttons.
	bHasMsvc32 = CheckMsvc(false);

	// Go through the various permutations.
#ifndef _WIN64
	if (!g_is64bit) {
		// 32-bit system.
		if (!bHasMsvc32) {
			// 32-bit MSVCRT is missing.
			line1 = L"The 32-bit MSVC 2017 runtime is not installed.";
			line2 = L"You can download the 32-bit MSVC 2017 runtime at:\n"
				L"\x2022 <a href=\"https://aka.ms/vs/15/release/vc_redist.x86.exe\">"
					L"https://aka.ms/vs/15/release/vc_redist.x86.exe</a>";
		}
	} else
#endif /* !_WIN64 */
	{
		// 64-bit system.
		const bool bHasMsvc64 = CheckMsvc(true);
		if (!bHasMsvc32 && !bHasMsvc64) {
			// Both 32-bit and 64-bit MSVCRT are missing.
			line1 = L"The 32-bit and 64-bit MSVC 2017 runtimes are not installed.";
			line2 = L"You can download the MSVC 2017 runtime at:\n"
				L"\x2022 32-bit: <a href=\"https://aka.ms/vs/15/release/vc_redist.x86.exe\">"
					L"https://aka.ms/vs/15/release/vc_redist.x86.exe</a>\n"
				L"\x2022 64-bit: <a href=\"https://aka.ms/vs/15/release/vc_redist.x64.exe\">"
					L"https://aka.ms/vs/15/release/vc_redist.x64.exe</a>";
		} else if (!bHasMsvc32 && bHasMsvc64) {
			// 32-bit MSVCRT is missing.
			line1 = L"The 32-bit MSVC 2017 runtime is not installed.";
			line2 = L"You can download the 32-bit MSVC 2017 runtime at:\n"
				L"\x2022 <a href=\"https://aka.ms/vs/15/release/vc_redist.x86.exe\">"
					L"https://aka.ms/vs/15/release/vc_redist.x86.exe</a>";
		} else if (bHasMsvc32 && !bHasMsvc64) {
			// 64-bit MSVCRT is missing.
			line1 = L"The 64-bit MSVC 2017 runtime is not installed.";
			line2 = L"You can download the 64-bit MSVC 2017 runtime at:\n"
				L"\x2022 <a href=\"https://aka.ms/vs/15/release/vc_redist.x64.exe\">"
					L"https://aka.ms/vs/15/release/vc_redist.x64.exe</a>";
		}
	}

	// Show the status message.
	// If line1 is set, an error occurred, so we should
	// show the exclamation icon and disable the buttons.
	bErr = (line1 != NULL);
	ShowStatusMessage(hDlg, line1, line2, (bErr ? MB_ICONEXCLAMATION : 0));
	EnableButtons(hDlg, !bErr);
}

/**
 * Main dialog message handler
 */
static INT_PTR CALLBACK DialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {
		case WM_INITDIALOG:
			InitDialog(hDlg);
			break;

		case WM_SETCURSOR:
			DlgUpdateCursor();
			return TRUE;

		case WM_APP_ENDTASK:
			g_inProgress = false;
			EnableButtons(hDlg, true);
			DlgUpdateCursor();
			return TRUE;

		case WM_COMMAND:
			switch (LOWORD(wParam)) {
				case IDC_BUTTON_INSTALL:
				case IDC_BUTTON_UNINSTALL: {
					ThreadParams *params;
					HANDLE hThread;
					const wchar_t *msg;
					bool isUninstall;

					if (g_inProgress) {
						// Already (un)installing...
						return TRUE;
					}

					isUninstall = (LOWORD(wParam) == IDC_BUTTON_UNINSTALL);
					params = malloc(sizeof(*params));
					if (!params) {
						// Could not allocate memory.
						return TRUE;
					}

					if (g_is64bit) {
						msg = (isUninstall
							? L"\n\nUnregistering DLLs..."
							: L"\n\nRegistering DLLs...");
					} else {
						msg = (isUninstall
							? L"\n\nUnregistering DLL..."
							: L"\n\nRegistering DLL...");
					}
					ShowStatusMessage(hDlg, msg, L"", 0);

					EnableButtons(hDlg, false);
					g_inProgress = true;
					DlgUpdateCursor();

					// The installation is done on a separate thread so that we don't lock the message loop
					params->hWnd = hDlg;
					params->isUninstall = isUninstall;
					hThread = (HANDLE)_beginthreadex(NULL, 0, ThreadProc, params, 0, NULL);
					if (!hThread) {
						// Couldn't start the worker thread.
						wchar_t threadErr[128];
						swprintf(threadErr, ARRAY_SIZE(threadErr),
							L"\x2022 Win32 error code: %u", GetLastError());
						ShowStatusMessage(hDlg, L"An error occurred while starting the worker thread.", threadErr, MB_ICONSTOP);
						MessageBeep(MB_ICONSTOP);
						EnableButtons(hDlg, true);
						g_inProgress = false;
						DlgUpdateCursor();
						free(params);
						return TRUE;
					} else {
						// We don't need to keep the thread handle open.
						CloseHandle(hThread);
					}
					return TRUE;
				}

				case IDOK:
					// There's no "OK" button here...
					// Silently ignore it.
					return TRUE;

				case IDCANCEL:
					// User pressed Escape.
					if (!g_inProgress) {
						EndDialog(hDlg, 0);
					}
					return TRUE;

				default:
					break;
			}
			return FALSE;

		case WM_NOTIFY: {
			const NMHDR *const pHdr = (const NMHDR*)lParam;
			assert(pHdr != NULL);
			switch (pHdr->code) {
				case NM_CLICK:
				case NM_RETURN: {
					const NMLINK *pNMLink;
					int ret;

					if (pHdr->idFrom != IDC_STATIC_STATUS2)
						break;

					// This is a SysLink control.
					// Open the URL.
					// ShellExecute return value references:
					// - https://msdn.microsoft.com/en-us/library/windows/desktop/bb762153(v=vs.85).aspx
					// - https://blogs.msdn.microsoft.com/oldnewthing/20061108-05/?p=29083
					pNMLink = (const NMLINK*)pHdr;
					ret = (int)(INT_PTR)ShellExecute(NULL, L"open", pNMLink->item.szUrl, NULL, NULL, SW_SHOW);
					if (ret <= 32) {
						// ShellExecute() failed.
						wchar_t err[128];
						swprintf(err, ARRAY_SIZE(err),
							L"Could not open the URL.\n\n"
							L"Win32 error code: %d", ret);
						MessageBox(hDlg, err, L"Could not open URL", MB_ICONERROR);
					}
					return TRUE;
				}

				default:
					break;
			}
			break;
		}

		case WM_CLOSE:
			if (!g_inProgress) {
				EndDialog(hDlg, 0);
			}
			return TRUE;

		default:
			break;
	}

	return FALSE;
}

/**
 * Entry point
 */
int CALLBACK wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
#ifndef _WIN64
	HMODULE hKernel32;
	typedef BOOL (WINAPI *PFNISWOW64PROCESS)(HANDLE hProcess, PBOOL Wow64Process);
	PFNISWOW64PROCESS pfnIsWow64Process;
#endif /* !_WIN64 */
	((void)hPrevInstance);

	// Set Win32 security options.
	secoptions_init();

	// Set the C locale.
	setlocale(LC_ALL, "");

#ifndef _WIN64
	// Check if this is a 64-bit system. (Wow64)
	hKernel32 = GetModuleHandle(L"kernel32");
	if (!hKernel32) {
		DebugBreak();
		return EXIT_FAILURE;
	}
	pfnIsWow64Process = (PFNISWOW64PROCESS)GetProcAddress(hKernel32, "IsWow64Process");

	if (!pfnIsWow64Process) {
		// IsWow64Process() isn't available.
		// This must be a 32-bit system.
		g_is64bit = false;
	} else {
		BOOL bWow;
		if (!pfnIsWow64Process(GetCurrentProcess(), &bWow)) {
			DebugBreak();
			return EXIT_FAILURE;
		}
		g_is64bit = !!bWow;
	}
#endif /* !_WIN64 */

	// Load the icon.
	hIconDialog = (HICON)LoadImage(
		hInstance, MAKEINTRESOURCE(IDI_SVRPLUS), IMAGE_ICON,
		GetSystemMetrics(SM_CXICON),
		GetSystemMetrics(SM_CYICON), 0);
	hIconDialogSmall = (HICON)LoadImage(
		hInstance, MAKEINTRESOURCE(IDI_SVRPLUS), IMAGE_ICON,
		GetSystemMetrics(SM_CXSMICON),
		GetSystemMetrics(SM_CYSMICON), 0);

	// Run the dialog
	DialogBox(hInstance, MAKEINTRESOURCE(IDD_SVRPLUS), NULL, DialogProc);

	// Delete the icons.
	if (hIconDialog) {
		DestroyIcon(hIconDialog);
	}
	if (hIconDialogSmall) {
		DestroyIcon(hIconDialogSmall);
	}

	return EXIT_SUCCESS;
}
