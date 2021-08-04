/***************************************************************************
 * ROM Properties Page shell extension installer. (svrplus)                *
 * svrplus.c: Win32 installer for rom-properties.                          *
 *                                                                         *
 * Copyright (c) 2017-2018 by Egor.                                        *
 * Copyright (c) 2017-2020 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
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

// stdbool
#include "stdboolx.h"

// libwin32common
#include "libwin32common/RpWin32_sdk.h"

// librpsecure
#include "librpsecure/os-secure.h"

// Additional Windows headers.
#include <windowsx.h>
#include <shellapi.h>
#include <commctrl.h>

// Application resources.
#include "resource.h"

// These were introduced in MSVC 2015 or 2017.
// AppVeyor is set to MSVC 2013, so it fails there.
#ifndef _MAX_ITOSTR_BASE10_COUNT
#define _MAX_ITOSTR_BASE10_COUNT   (1 + 10 + 1)
#endif
#ifndef _MAX_ULTOSTR_BASE10_COUNT
#define _MAX_ULTOSTR_BASE10_COUNT  (10 + 1)
#endif

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
	const RECT *rect;
	int sw_status;

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
	int ret = GetSystemDirFilePath(msvcrt_path, _countof(msvcrt_path), _T("msvcp140.dll"), is64);
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
	TCHAR regsvr32_path[MAX_PATH];
	TCHAR args[14 + MAX_PATH + 4 + 3 + _countof(str_rp64path)] = _T("regsvr32.exe \"");
	DWORD szModuleFn;
	TCHAR *bs;

	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	DWORD status;
	BOOL bRet;

	// Determine the REGSVR32 path.
	int ret = GetSystemDirFilePath(regsvr32_path, _countof(regsvr32_path), _T("regsvr32.exe"), is64);
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
	_tcscat_s(args, _countof(args), is64 ? str_rp64path : str_rp32path);
	if (!fileExists(&args[14])) {
		// File not found.
		return ISR_FILE_NOT_FOUND;
	}

	// Append /s (silent) key, and optionally append /u (uninstall) key.
	_tcscat_s(args, _countof(args), isUninstall ? _T("\" /s /u") : _T("\" /s"));

	memset(&si, 0, sizeof(si));
	memset(&pi, 0, sizeof(pi));
	si.cb = sizeof(si);

	bRet = CreateProcess(
		regsvr32_path,		// lpApplicationName
		args,			// lpCommandLine
		NULL,			// lpProcessAttributes
		NULL,			// lpThreadAttributes
		FALSE,			// bInheritHandles
		CREATE_NO_WINDOW,	// dwCreationFlags
		NULL,			// lpEnvironment
		NULL,			// lpCurrentDirectory
		&si,			// lpStartupInfo
		&pi);			// lpProcessInformation

	if (!bRet) {
		if (pErrorCode) {
			*pErrorCode = GetLastError();
		}
		return ISR_CREATEPROCESS_FAILED;
	}

	// Wait for the process to exit.
	WaitForSingleObject(pi.hProcess, INFINITE);
	bRet = GetExitCodeProcess(pi.hProcess, &status);
	CloseHandle(pi.hThread);
	CloseHandle(pi.hProcess);

	if (!bRet) {
		// GetExitCodeProcess() failed.
		// Assume the process is still active.
		return ISR_PROCESS_STILL_ACTIVE;
	}

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
	TCHAR *sErrBuf, size_t cchErrBuf)
{
	const TCHAR *dll_path, *entry_point;
	TCHAR ultot_buf[_MAX_ULTOSTR_BASE10_COUNT];
	DWORD errorCode;
	InstallServerResult res = InstallServer(isUninstall, is64, &errorCode);

	if (!sErrBuf) {
		// No error message buffer specified.
		return res;
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
			_tcscpy_s(sErrBuf, cchErrBuf, dll_path);
			_tcscat_s(sErrBuf, cchErrBuf, _T(" is missing."));
			break;
		case ISR_CREATEPROCESS_FAILED:
			_ultot_s(errorCode, ultot_buf, _countof(ultot_buf), 10);
			_tcscpy_s(sErrBuf, cchErrBuf, _T("Could not start REGSVR32.exe. (Err:"));
			_tcscat_s(sErrBuf, cchErrBuf, ultot_buf);
			_tcscat_s(sErrBuf, cchErrBuf, _T(")"));
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
					_tcscpy_s(sErrBuf, cchErrBuf, _T("REGSVR32 failed: "));
					_tcscat_s(sErrBuf, cchErrBuf, dll_path);
					_tcscat_s(sErrBuf, cchErrBuf, _T(" is not a valid DLL."));
					break;
				case REGSVR32_FAIL_ENTRY:
					_tcscpy_s(sErrBuf, cchErrBuf, _T("REGSVR32 failed: "));
					_tcscat_s(sErrBuf, cchErrBuf, dll_path);
					_tcscat_s(sErrBuf, cchErrBuf, _T(" is missing "));
					_tcscat_s(sErrBuf, cchErrBuf, entry_point);
					_tcscat_s(sErrBuf, cchErrBuf, _T("()."));
					break;
				case REGSVR32_FAIL_REG:
					_tcscpy_s(sErrBuf, cchErrBuf, _T("REGSVR32 failed: "));
					_tcscat_s(sErrBuf, cchErrBuf, entry_point);
					_tcscat_s(sErrBuf, cchErrBuf, _T("() returned an error."));
					break;
				default:
					_ultot_s(errorCode, ultot_buf, _countof(ultot_buf), 10);
					_tcscpy_s(sErrBuf, cchErrBuf, _T("REGSVR32 failed: Unknown exit code: "));
					_tcscat_s(sErrBuf, cchErrBuf, ultot_buf);
					break;
			}
			break;
	}

	return res;
}

// Thread parameters.
// Statically allocated so we don't need to use malloc().
// Only one thread can run at a time. (see g_inProgress)
typedef struct _ThreadParams {
	HWND hWnd;		/**< Window that created the thread */
	bool isUninstall;	/**< true if uninstalling */
} ThreadParams;
static ThreadParams threadParams = {NULL, false};

/**
 * Worker thread procedure.
 *
 * @param lpParameter ptr to parameters of type ThreadPrams. Owned by this thread.
 */
static unsigned int WINAPI ThreadProc(LPVOID lpParameter)
{
	TCHAR msg32[256], msg64[256];
	InstallServerResult res32 = ISR_OK, res64 = ISR_OK;

	ThreadParams *const params = (ThreadParams*)lpParameter;

	// Try to (un)install the 64-bit version.
	if (g_is64bit) {
		res64 = TryInstallServer(params->hWnd, params->isUninstall, true, msg64, _countof(msg64));
	}

	// Try to (un)install the 32-bit version.
	res32 = TryInstallServer(params->hWnd, params->isUninstall, false, msg32, _countof(msg32));

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
				_tcscpy_s(msg2, _countof(msg2), BULLET _T(" 32-bit: "));
			}
			_tcscat_s(msg2, _countof(msg2), msg32);
		}
		if (res64 != ISR_OK) {
			if (msg2[0] != _T('\0')) {
				_tcscat_s(msg2, _countof(msg2), _T("\n"));
			}
			_tcscat_s(msg2, _countof(msg2), BULLET _T(" 64-bit: "));
			_tcscat_s(msg2, _countof(msg2), msg64);
		}

		ShowStatusMessage(params->hWnd, msg1, msg2, MB_ICONSTOP);
		MessageBeep(MB_ICONSTOP);
	}

	SendMessage(params->hWnd, WM_APP_ENDTASK, 0, 0);
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
	TCHAR line1[80], line2[512];
	line1[0] = _T('\0');
	line2[0] = _T('\0');

	// OS version check.
	OSVERSIONINFO osvi;
	unsigned int vcyear, vcver;

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

	// MSVC 2019 runtime requires Windows Vista or later.
	osvi.dwOSVersionInfoSize = sizeof(osvi);
	if (GetVersionEx(&osvi) != 0 && osvi.dwMajorVersion >= 6) {
		// Windows Vista or later. Use MSVC 2019.
		vcyear = 2019;
		vcver = 16;
	} else {
		// Windows XP/2003 or earlier, or GetVersionEx() failed.
		// Use MSVC 2017.
		vcyear = 2017;
		vcver = 15;
	}

	// Go through the various permutations.
#ifndef _WIN64
	if (!g_is64bit) {
		// 32-bit system.
		if (!bHasMsvc32) {
			// 32-bit MSVCRT is missing.
			_sntprintf(line1, _countof(line1),
				_T("The 32-bit MSVC 2015-%u runtime is not installed."), vcyear);
			_sntprintf(line2, _countof(line2),
				_T("You can download the 32-bit MSVC 2015-%u runtime at:\n")
				BULLET _T(" <a href=\"https://aka.ms/vs/%u/release/vc_redist.x86.exe\">")
					_T("https://aka.ms/vs/%u/release/vc_redist.x86.exe</a>"),
				vcyear, vcver, vcver);
		}
	} else
#endif /* !_WIN64 */
	{
		// 64-bit system.
		const bool bHasMsvc64 = CheckMsvc(true);
		if (!bHasMsvc32 && !bHasMsvc64) {
			// Both 32-bit and 64-bit MSVCRT are missing.
			_sntprintf(line1, _countof(line1),
				_T("The 32-bit and 64-bit MSVC 2015-%u runtimes are not installed."), vcyear);
			_sntprintf(line2, _countof(line2),
				_T("You can download the MSVC 2015-%u runtime at:\n")
				BULLET _T(" 32-bit: <a href=\"https://aka.ms/vs/%u/release/vc_redist.x86.exe\">")
					_T("https://aka.ms/vs/%u/release/vc_redist.x86.exe</a>\n")
				BULLET _T(" 64-bit: <a href=\"https://aka.ms/vs/%u/release/vc_redist.x64.exe\">")
					_T("https://aka.ms/vs/%u/release/vc_redist.x64.exe</a>"),
				vcyear, vcver, vcver, vcver, vcver);
		} else if (!bHasMsvc32 && bHasMsvc64) {
			// 32-bit MSVCRT is missing.
			_sntprintf(line1, _countof(line1),
				_T("The 32-bit MSVC 2015-%u runtime is not installed."), vcyear);
			_sntprintf(line2, _countof(line2),
				_T("You can download the 32-bit MSVC 2015-%u runtime at:\n")
				BULLET _T(" <a href=\"https://aka.ms/vs/%u/release/vc_redist.x86.exe\">")
					_T("https://aka.ms/vs/%u/release/vc_redist.x86.exe</a>"),
				vcyear, vcver, vcver);
		} else if (bHasMsvc32 && !bHasMsvc64) {
			// 64-bit MSVCRT is missing.
			_sntprintf(line1, _countof(line1),
				_T("The 64-bit MSVC 2015-%u runtime is not installed."), vcyear);
			_sntprintf(line2, _countof(line2),
				_T("You can download the 64-bit MSVC 2015-%u runtime at:\n")
				BULLET _T(" <a href=\"https://aka.ms/vs/%u/release/vc_redist.x64.exe\">")
					_T("https://aka.ms/vs/%u/release/vc_redist.x64.exe</a>"),
				vcyear, vcver, vcver);
		}
	}

	// Show the status message.
	// If line1 is set, an error occurred, so we should
	// show the exclamation icon and disable the buttons.
	bErr = (line1[0] != _T('\0'));
	ShowStatusMessage(hDlg, line1, line2, (bErr ? MB_ICONEXCLAMATION : 0));
	EnableButtons(hDlg, !bErr);
}

/**
 * IDC_BUTTON_INSTALL / IDC_BUTTON_UNINSTALL handler.
 * @param hDlg Dialog handle.
 * @param isUninstall True for uninstall; false for install.
 */
static void HandleInstallUninstall(HWND hDlg, bool isUninstall)
{
	HANDLE hThread;
	const TCHAR *msg;

	if (g_inProgress) {
		// Already (un)installing...
		return;
	}
	g_inProgress = true;

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
	DlgUpdateCursor();

	// The installation is done on a separate thread so that we don't lock the message loop
	threadParams.hWnd = hDlg;
	threadParams.isUninstall = isUninstall;
	hThread = (HANDLE)_beginthreadex(NULL, 0, ThreadProc, &threadParams, 0, NULL);
	if (!hThread) {
		// Couldn't start the worker thread.
		TCHAR threadErr[128];
		TCHAR ultot_buf[_MAX_ULTOSTR_BASE10_COUNT];

		const DWORD lastError = GetLastError();
		_ultot_s(lastError, ultot_buf, _countof(ultot_buf), 10);
		_tcscpy_s(threadErr, _countof(threadErr), BULLET _T(" Win32 error code: "));
		_tcscat_s(threadErr, _countof(threadErr), ultot_buf);

		ShowStatusMessage(hDlg, _T("An error occurred while starting the worker thread."), threadErr, MB_ICONSTOP);
		MessageBeep(MB_ICONSTOP);
		EnableButtons(hDlg, true);
		DlgUpdateCursor();

		threadParams.hWnd = NULL;
		threadParams.isUninstall = false;
		g_inProgress = false;
	} else {
		// Install/uninstall thread is running.
		// We don't need to keep the thread handle open.
		CloseHandle(hThread);
	}
	return;
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
			// Install/uninstall thread has completed.
			EnableButtons(hDlg, true);
			DlgUpdateCursor();

			g_inProgress = false;
			threadParams.hWnd = NULL;
			threadParams.isUninstall = false;
			return TRUE;

		case WM_COMMAND:
			switch (LOWORD(wParam)) {
				case IDC_BUTTON_INSTALL:
					HandleInstallUninstall(hDlg, false);
					break;
				case IDC_BUTTON_UNINSTALL:
					HandleInstallUninstall(hDlg, true);
					break;

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
#ifdef UNICODE
				case NM_CLICK:
				case NM_RETURN: {
					const NMLINK *pNMLink;
					INT_PTR ret;

					if (pHdr->idFrom != IDC_STATIC_STATUS2)
						break;

					// This is a SysLink control.
					// Open the URL.
					// ShellExecute return value references:
					// - https://msdn.microsoft.com/en-us/library/windows/desktop/bb762153(v=vs.85).aspx
					// - https://blogs.msdn.microsoft.com/oldnewthing/20061108-05/?p=29083
					pNMLink = (const NMLINK*)pHdr;
					ret = (INT_PTR)ShellExecute(NULL, _T("open"), pNMLink->item.szUrl, NULL, NULL, SW_SHOW);
					if (ret <= 32) {
						// ShellExecute() failed.
						TCHAR err[128];
						TCHAR itot_buf[_MAX_ITOSTR_BASE10_COUNT];
						_itot_s((int)ret, itot_buf, _countof(itot_buf), 10);
						_tcscpy_s(err, _countof(err),
							_T("Could not open the URL.\n\n")
							_T("Win32 error code: "));
						_tcscat_s(err, _countof(err), itot_buf);
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
int CALLBACK wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
	HANDLE hSingleInstanceMutex;
#ifndef _WIN64
	HMODULE hKernel32;
	typedef BOOL (WINAPI *PFNISWOW64PROCESS)(HANDLE hProcess, PBOOL Wow64Process);
	PFNISWOW64PROCESS pfnIsWow64Process;
#endif /* !_WIN64 */

	// Set Win32 security options.
	rp_secure_param_t param;
	param.bHighSec = FALSE;
	rp_secure_enable(param);

	// Unused parameters. (Win16 baggage)
	((void)hPrevInstance);

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
	// NOTE: svrplus doesn't use localization. Using setlocale()
	// adds 28,672 bytes to the statically-linked executable
	//setlocale(LC_ALL, "");

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
			CloseHandle(hSingleInstanceMutex);
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
	DialogBox(hInstance, MAKEINTRESOURCE(IDD_SVRPLUS), NULL, DialogProc);

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
