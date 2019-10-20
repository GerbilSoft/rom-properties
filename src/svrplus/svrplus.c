/***************************************************************************
 * ROM Properties Page shell extension installer. (svrplus)                *
 * svrplus.c: Win32 installer for rom-properties.                          *
 *                                                                         *
 * Copyright (c) 2017-2019 by Egor.                                        *
 * Copyright (c) 2017-2019 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

// C includes.
#include <assert.h>
#include <errno.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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
static const TCHAR str_rp32path[] = _T("i386\\rom-properties.dll");
static const TCHAR str_rp64path[] = _T("amd64\\rom-properties.dll");

// Bullet symbol
#ifdef _UNICODE
# define BULLET _T("\x2022")
#else /* !_UNICODE */
# define BULLET _T("*")
#endif

// Globals

#ifdef _WIN64
static const bool g_is64bit = true;	/**< true if running on 64-bit system */
#else /* !_WIN64 */
bool g_is64bit = false;			/**< true if running on 64-bit system */
#endif
bool g_inProgress = false;		/**< true if currently (un)installing the DLLs */

// Custom messages
#define WM_APP_ENDTASK WM_APP
#define WM_APP_SIGNAL (WM_APP+1)
#define WM_APP_WAIT (WM_APP+2)

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
static void ShowStatusMessage(HWND hDlg, const TCHAR *line1, const TCHAR *line2, UINT uType)
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
static int GetSystemDirFilePath(TCHAR *pszPath, size_t cchPath, const TCHAR *filename, bool is64)
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
	cchFilename = (unsigned int)_tcslen(filename);
	if (len + cchFilename + 11 >= cchPath) {
		// Not enough space.
		return -ENOMEM;
	}

	// Append the System directory name.
#ifdef _WIN64
	_tcscpy_s(&pszPath[len], cchPath-len, (is64 ? _T("System32\\") : _T("SysWOW64\\")));
	len += 9;
#else /* !_WIN64 */
	_tcscpy_s(&pszPath[len], cchPath-len, (is64 ? _T("Sysnative\\") : _T("System32\\")));
	len += (is64 ? 10 : 9);
#endif /* _WIN64 */

	// Append the filename.
	_tcscpy_s(&pszPath[len], cchPath-len, filename);
	return len + cchFilename;
}

/**
 * Check if a file exists.
 * @param filename Filename.
 * @return True if the file exists; false if not.
 */
static inline bool fileExists(const TCHAR *filename)
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
	TCHAR msvcrt_path[MAX_PATH];
	int ret = GetSystemDirFilePath(msvcrt_path, ARRAY_SIZE(msvcrt_path), _T("msvcp140.dll"), is64);
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
 * @param isUninstall [in] When true, uninstalls the DLL, instead of installing it.
 * @param is64        [in] When true, installs 64-bit version.
 * @param pHandle     [out,opt] Process handle.
 * @param pErrorCode  [out,opt] Additional error code.
 * @return InstallServerResult
 */
static InstallServerResult InstallServer(bool isUninstall, bool is64, HANDLE *pHandle, DWORD *pErrorCode)
{
	TCHAR regsvr32_path[MAX_PATH];
	TCHAR args[14 + MAX_PATH + 4 + 3 + ARRAY_SIZE(str_rp64path)] = _T("regsvr32.exe \"");
	DWORD szModuleFn;
	TCHAR *bs;

	STARTUPINFO si;
	PROCESS_INFORMATION pi;

	// Determine the REGSVR32 path.
	int ret = GetSystemDirFilePath(regsvr32_path, ARRAY_SIZE(regsvr32_path), _T("regsvr32.exe"), is64);
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
	bs = _tcsrchr(args, _T('\\'));
	if (!bs) {
		// No backslashes...
		return ISR_FATAL_ERROR;
	}

	// Remove the EXE filename, then append the DLL relative path.
	bs[1] = 0;
	_tcscat_s(args, ARRAY_SIZE(args), is64 ? str_rp64path : str_rp32path);
	if (!fileExists(&args[14])) {
		// File not found.
		return ISR_FILE_NOT_FOUND;
	}

	// Append /s (silent) key, and optionally append /u (uninstall) key.
	_tcscat_s(args, ARRAY_SIZE(args), isUninstall ? _T("\" /s /u") : _T("\" /s"));

	memset(&si, 0, sizeof(si));
	memset(&pi, 0, sizeof(pi));
	si.cb = sizeof(si);

	if (!CreateProcess(regsvr32_path, args, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
		if (pErrorCode) {
			*pErrorCode = GetLastError();
		}
		return ISR_CREATEPROCESS_FAILED;
	}

	CloseHandle(pi.hThread);

	*pHandle = pi.hProcess;

	// The calles should wait for the process to exit.
	return ISR_OK;
}

/**
 * (Un)installs the COM Server DLL.
 *
 * @param handle     [in] Process handle from InstallServer
 * @param pErrorCode [out,opt] Additional error code.
 * @return InstallServerResult
 */
static InstallServerResult InstallServerEnd(HANDLE handle, DWORD *pErrorCode)
{
	DWORD status;

	if (!handle) {
		return ISR_FATAL_ERROR;
	}

	GetExitCodeProcess(handle, &status);
	CloseHandle(handle);

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
 * Converts error codes from InstallServer into a string.
 *
 * @param res         [in] InstallServerResult as returned from InstallServer
 * @param errorCode   [in] additional error code
 * @param isUninstall [in] same as given to InstallServer
 * @param is64        [in] same as given to InstallServer
 * @param sErrBuf     [out,opt] error message buffer
 * @param cchErrBuf   [in,opt] size of sErrBuf, in characters
 * @return InstallServerResult
 */
static void InstallServerErrorMsg(
	InstallServerResult res, DWORD errorCode,
	bool isUninstall, bool is64,
	TCHAR *sErrBuf, size_t cchErrBuf)
{
	const TCHAR *dll_path, *entry_point;

	if (!sErrBuf) {
		// No error message buffer specified.
		return;
	}

	dll_path = (is64 ? str_rp64path : str_rp32path);
	entry_point = (isUninstall
		? _T("DllUnregisterServer")
		: _T("DllRegisterServer"));

	// NOTE: Using _tcscpy_s() instead of _tcsncpy() because
	// _tcsncpy() zeroes the rest of the buffer.
	switch (res) {
		case ISR_OK:
			// No error.
			if (cchErrBuf > 0) {
				sErrBuf[0] = _T('\0');
			}
			break;
		case ISR_FATAL_ERROR:
		default:
			_tcscpy_s(sErrBuf, cchErrBuf, _T("An unknown fatal error occurred."));
			break;
		case ISR_FILE_NOT_FOUND:
			_sntprintf(sErrBuf, cchErrBuf, _T("%s is missing."), dll_path);
			break;
		case ISR_CREATEPROCESS_FAILED:
			_sntprintf(sErrBuf, cchErrBuf, _T("Could not start REGSVR32.exe. (Err:%u)"), errorCode);
			break;
		case ISR_PROCESS_STILL_ACTIVE:
			_tcscpy_s(sErrBuf, cchErrBuf, _T("The REGSVR32 process never completed."));
			break;
		case ISR_REGSVR32_EXIT_CODE:
			switch (errorCode) {
				case REGSVR32_FAIL_ARGS:
					_tcscpy_s(sErrBuf, cchErrBuf, _T("REGSVR32 failed: Invalid argument."));
					break;
				case REGSVR32_FAIL_OLE:
					_tcscpy_s(sErrBuf, cchErrBuf, _T("REGSVR32 failed: OleInitialize() failed."));
					break;
				case REGSVR32_FAIL_LOAD:
					_sntprintf(sErrBuf, cchErrBuf, _T("REGSVR32 failed: %s is not a valid DLL."), dll_path);
					break;
				case REGSVR32_FAIL_ENTRY:
					_sntprintf(sErrBuf, cchErrBuf, _T("REGSVR32 failed: %s is missing %s()."),
						dll_path, entry_point);
					break;
				case REGSVR32_FAIL_REG:
					_sntprintf(sErrBuf, cchErrBuf, _T("REGSVR32 failed: %s() returned an error."),
						entry_point);
					break;
				default:
					_sntprintf(sErrBuf, cchErrBuf, _T("REGSVR32 failed: Unknown exit code %u."), errorCode);
					break;
			}
			break;
	}

	return;
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
	TCHAR msg32[256], msg64[256];
	InstallServerResult res32 = ISR_OK, res64 = ISR_OK;
	DWORD errorCode = 0;
	HANDLE hProcess;

	ThreadParams *const params = (ThreadParams*)lpParameter;

	// Try to (un)install the 64-bit version.
	if (g_is64bit) {
		res64 = InstallServer(params->isUninstall, true, &hProcess, &errorCode);
		if (res64 == ISR_OK) {
			WaitForSingleObject(hProcess, INFINITE);
			res64 = InstallServerEnd(hProcess, &errorCode);
		}
		InstallServerErrorMsg(res64, errorCode, params->isUninstall, true, msg64, ARRAY_SIZE(msg64));
	}

	// Try to (un)install the 32-bit version.
	res32 = InstallServer(params->isUninstall, false, &hProcess, &errorCode);
	if (res32 == ISR_OK) {
		WaitForSingleObject(hProcess, INFINITE);
		res32 = InstallServerEnd(hProcess, &errorCode);
	}
	InstallServerErrorMsg(res32, errorCode, params->isUninstall, false, msg32, ARRAY_SIZE(msg32));

	if (res32 == ISR_OK && res64 == ISR_OK) {
		// DLL(s) registered successfully.
		const TCHAR *msg;
		if (g_is64bit) {
			msg = (params->isUninstall
				? _T("DLLs unregistered successfully.")
				: _T("DLLs registered successfully."));
		} else {
			msg = (params->isUninstall
				? _T("DLL unregistered successfully.")
				: _T("DLL registered successfully."));
		}
		ShowStatusMessage(params->hWnd, msg, _T(""), MB_ICONINFORMATION);
		MessageBeep(MB_ICONINFORMATION);
	} else {
		// At least one of the DLLs failed to register.
		const TCHAR *msg1;
		TCHAR msg2[540];
		msg2[0] = _T('\0');

		if (g_is64bit) {
			msg1 = (params->isUninstall
				? _T("An error occurred while unregistering the DLLs:")
				: _T("An error occurred while registering the DLLs:"));
		} else {
			msg1 = (params->isUninstall
				? _T("An error occurred while unregistering the DLL:")
				: _T("An error occurred while registering the DLL:"));
		}

		if (res32 != ISR_OK) {
			if (g_is64bit) {
				_tcscpy_s(msg2, ARRAY_SIZE(msg2), BULLET _T(" 32-bit: "));
			}
			_tcscat_s(msg2, ARRAY_SIZE(msg2), msg32);
		}
		if (res64 != ISR_OK) {
			if (msg2[0] != _T('\0')) {
				_tcscat_s(msg2, ARRAY_SIZE(msg2), _T("\n"));
			}
			_tcscat_s(msg2, ARRAY_SIZE(msg2), BULLET _T(" 64-bit: "));
			_tcscat_s(msg2, ARRAY_SIZE(msg2), msg64);
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
	static const TCHAR strdlg_desc[] =
		_T("This installer will register the ROM Properties Page DLL with the system, ")
		_T("which will provide extra functionality for supported files in Windows Explorer.\n\n")
		_T("Note that the DLL locations are hard-coded in the registry. If you move the DLLs, ")
		_T("you will have to rerun this installer. In addition, the DLLs will usually be locked ")
		_T("by Explorer, so you will need to use this program to uninstall the DLLs first and ")
		_T("then restart Explorer in order to move the DLLs.\n\n")
		_T("Uninstalling will unregister the ROM Properties DLL, which will disable the extra ")
		_T("functionality provided by the DLL for supported ROM files.");

	HWND hStatus1, hExclaim;
	HMODULE hUser32;
	bool bHasMsvc32, bErr;
	const TCHAR *line1 = NULL, *line2 = NULL;

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
	hUser32 = GetModuleHandle(_T("user32"));
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
			line1 = _T("The 32-bit MSVC 2015-2019 runtime is not installed.");
			line2 = _T("You can download the 32-bit MSVC 2015-2019 runtime at:\n")
				BULLET _T(" <a href=\"https://aka.ms/vs/16/release/vc_redist.x86.exe\">")
					_T("https://aka.ms/vs/16/release/vc_redist.x86.exe</a>");
		}
	} else
#endif /* !_WIN64 */
	{
		// 64-bit system.
		const bool bHasMsvc64 = CheckMsvc(true);
		if (!bHasMsvc32 && !bHasMsvc64) {
			// Both 32-bit and 64-bit MSVCRT are missing.
			line1 = _T("The 32-bit and 64-bit MSVC 2015-2019 runtimes are not installed.");
			line2 = _T("You can download the MSVC 2015-2019 runtime at:\n")
				BULLET _T(" 32-bit: <a href=\"https://aka.ms/vs/16/release/vc_redist.x86.exe\">")
					_T("https://aka.ms/vs/16/release/vc_redist.x86.exe</a>\n")
				BULLET _T(" 64-bit: <a href=\"https://aka.ms/vs/16/release/vc_redist.x64.exe\">")
					_T("https://aka.ms/vs/16/release/vc_redist.x64.exe</a>");
		} else if (!bHasMsvc32 && bHasMsvc64) {
			// 32-bit MSVCRT is missing.
			line1 = _T("The 32-bit MSVC 2015-2019 runtime is not installed.");
			line2 = _T("You can download the 32-bit MSVC 2015-2019 runtime at:\n")
				BULLET _T(" <a href=\"https://aka.ms/vs/16/release/vc_redist.x86.exe\">")
					_T("https://aka.ms/vs/16/release/vc_redist.x86.exe</a>");
		} else if (bHasMsvc32 && !bHasMsvc64) {
			// 64-bit MSVCRT is missing.
			line1 = _T("The 64-bit MSVC 2015-2019 runtime is not installed.");
			line2 = _T("You can download the 64-bit MSVC 2015-2019 runtime at:\n")
				BULLET _T(" <a href=\"https://aka.ms/vs/16/release/vc_redist.x64.exe\">")
					_T("https://aka.ms/vs/16/release/vc_redist.x64.exe</a>");
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
					const TCHAR *msg;
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
							? _T("\n\nUnregistering DLLs...")
							: _T("\n\nRegistering DLLs..."));
					} else {
						msg = (isUninstall
							? _T("\n\nUnregistering DLL...")
							: _T("\n\nRegistering DLL..."));
					}
					ShowStatusMessage(hDlg, msg, _T(""), 0);

					EnableButtons(hDlg, false);
					g_inProgress = true;
					DlgUpdateCursor();

					// The installation is done on a separate thread so that we don't lock the message loop
					params->hWnd = hDlg;
					params->isUninstall = isUninstall;
					hThread = (HANDLE)_beginthreadex(NULL, 0, ThreadProc, params, 0, NULL);
					if (!hThread) {
						// Couldn't start the worker thread.
						TCHAR threadErr[128];
						_sntprintf(threadErr, ARRAY_SIZE(threadErr),
							BULLET _T(" Win32 error code: %u"), GetLastError());
						ShowStatusMessage(hDlg, _T("An error occurred while starting the worker thread."), threadErr, MB_ICONSTOP);
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
						DestroyWindow(hDlg);
						PostQuitMessage(0);
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
#ifdef UNICODE
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
					ret = (int)(INT_PTR)ShellExecute(NULL, _T("open"), pNMLink->item.szUrl, NULL, NULL, SW_SHOW);
					if (ret <= 32) {
						// ShellExecute() failed.
						TCHAR err[128];
						_sntprintf(err, ARRAY_SIZE(err),
							_T("Could not open the URL.\n\n")
							_T("Win32 error code: %d"), ret);
						MessageBox(hDlg, err, _T("Could not open URL"), MB_ICONERROR);
					}
					return TRUE;
				}
#endif

				default:
					break;
			}
			break;
		}

		case WM_CLOSE:
			if (!g_inProgress) {
				DestroyWindow(hDlg);
				PostQuitMessage(0);
			}
			return TRUE;

		default:
			break;
	}

	return FALSE;
}

struct MsgObjState {
	HWND hWnd; /**< Window that should receive SIGNAL messages */
	DWORD count; /**< Count of objects to wait on */
	WPARAM params[MAXIMUM_WAIT_OBJECTS-1]; /**< wParams corresponding to objects */
	HANDLE handles[MAXIMUM_WAIT_OBJECTS-1]; /**< Objects to wait on */
};

/**
 * Get messages and wait for objects
 *
 * To wait for an object, post WM_APP_WAIT to the thread message queue with
 * lParam=handle. When an object is signaled, the main window (state->hWnd)
 * receives WM_APP_SIGNAL, with the same parameters as the WAIT message (thus
 * WPARAM can be used to store additional state info).
 * Both WAIT and SIGNAL messages imply transfer of handle ownership.
 *
 * @param msg same as GetMessage
 * @param state state that should persist between calls
 * @return same as GetMessage
 */
BOOL GetMessageObjects(MSG *msg, struct MsgObjState *state)
{
	BOOL bRet;
	for (;;) {
		DWORD ev = MsgWaitForMultipleObjects(state->count, state->handles, FALSE, INFINITE, QS_ALLINPUT);
		if (ev == WAIT_OBJECT_0 + state->count) { // Message
			if ((bRet = GetMessage(msg, NULL, 0, 0)) > 0 && msg->message == WM_APP_WAIT) {
				if (state->count < MAXIMUM_WAIT_OBJECTS-1) {
					// Add event
					state->params[state->count] = msg->wParam;
					state->handles[state->count] = (HANDLE)msg->lParam;
					state->count++;
				}
			} else {
				if (bRet == 0) { // Quitting
					for (DWORD i = 0; i < state->count; i++) {
						CloseHandle(state->handles[i]);
					}
					state->count = 0;
				}
				return bRet;
			}
		} else if (ev >= WAIT_OBJECT_0 && ev < WAIT_OBJECT_0 + state->count) { // Object
			ev -= WAIT_OBJECT_0;
			PostMessage(state->hWnd, WM_APP_SIGNAL, state->params[ev], (LPARAM)state->handles[ev]);
			// Remove event
			memmove(&state->params[ev], &state->params[ev+1], (state->count-(ev+1))*sizeof(WPARAM));
			memmove(&state->handles[ev], &state->handles[ev+1], (state->count-(ev+1))*sizeof(HANDLE));
			state->count--;
		}
		// FIXME: handle WAIT_FAILED and others?
	}
}
/**
 * Dialog message loop
 */
INT_PTR DialogLoop(HINSTANCE hInstance, LPCTSTR lpTemplateName, HWND hWndParent, DLGPROC lpDialogFunc, LPARAM dwInitParam)
{
	MSG msg;
	BOOL bRet;
	HWND hDlg;

	if (!(hDlg = CreateDialogParam(hInstance, lpTemplateName, hWndParent, lpDialogFunc, dwInitParam))) {
		return -1;
	}

	struct MsgObjState state = { hDlg };

	while ((bRet = GetMessageObjects(&msg, &state)) != 0) {
		if (bRet == -1) {
			break;
		} else if (!IsWindow(hDlg) || !IsDialogMessage(hDlg, &msg)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	if (bRet == 0) {
		return msg.wParam;
	}
	return -1;
}

/**
 * Entry point
 */
int CALLBACK wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
	HANDLE hSingleInstanceMutex;
#ifndef _WIN64
	HMODULE hKernel32;
	typedef BOOL (WINAPI *PFNISWOW64PROCESS)(HANDLE hProcess, PBOOL Wow64Process);
	PFNISWOW64PROCESS pfnIsWow64Process;
#endif /* !_WIN64 */
	((void)hPrevInstance);

	// Set Win32 security options.
	secoptions_init();

	// Check if another instance of svrplus is already running.
	// References:
	// - https://stackoverflow.com/questions/4191465/how-to-run-only-one-instance-of-application
	// - https://stackoverflow.com/a/33531179
	hSingleInstanceMutex = CreateMutex(NULL, TRUE, _T("Local\\com.gerbilsoft.rom-properties.svrplus"));
	if (!hSingleInstanceMutex || GetLastError() == ERROR_ALREADY_EXISTS) {
		// Mutex already exists.
		// Set focus to the existing instance.
		HWND hWnd = FindWindow(_T("#32770"), _T("ROM Properties Page Installer"));
		if (hWnd) {
			SetForegroundWindow(hWnd);
		}
		return EXIT_SUCCESS;
	}

	// Set the C locale.
	setlocale(LC_ALL, "");

#ifndef _WIN64
	// Check if this is a 64-bit system. (Wow64)
	hKernel32 = GetModuleHandle(_T("kernel32"));
	assert(hKernel32 != NULL);
	if (!hKernel32) {
		CloseHandle(hSingleInstanceMutex);
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

	// Run the dialog.
	// FIXME: SysLink controls won't work in ANSI builds.
	DialogLoop(hInstance, MAKEINTRESOURCE(IDD_SVRPLUS), NULL, DialogProc, 0L);

	// Delete the icons.
	if (hIconDialog) {
		DestroyIcon(hIconDialog);
	}
	if (hIconDialogSmall) {
		DestroyIcon(hIconDialogSmall);
	}

	CloseHandle(hSingleInstanceMutex);
	return EXIT_SUCCESS;
}
