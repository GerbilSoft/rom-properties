/***************************************************************************
 * ROM Properties Page shell extension installer. (svrplus)                *
 * svrplus.c: Win32 installer for rom-properties.                          *
 *                                                                         *
 * Copyright (c) 2017-2018 by Egor.                                        *
 * Copyright (c) 2017-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

// C includes
#include <assert.h>
#include <errno.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>
// MSVCRT-specific
#include <process.h>

// Common includes
#include "common.h"
#include "stdboolx.h"

// libwin32common
#include "libwin32common/RpWin32_sdk.h"
#include "libwin32common/rp_versionhelpers.h"

// librpsecure
#include "librpsecure/os-secure.h"
#include "librpsecure/restrict-dll.h"

// Additional Windows headers
#include <commctrl.h>
#include <shellapi.h>
#include <windowsx.h>

// Older versions of the Windows SDK might be missing some ARM systems.
#ifndef IMAGE_FILE_MACHINE_ARM
#  define IMAGE_FILE_MACHINE_ARM 0x01C0
#endif
#ifndef IMAGE_FILE_MACHINE_THUMB
#  define IMAGE_FILE_MACHINE_THUMB 0x01C2
#endif
#ifndef IMAGE_FILE_MACHINE_ARMNT
#  define IMAGE_FILE_MACHINE_ARMNT 0x01C4
#endif
#ifndef IMAGE_FILE_MACHINE_ARM64
#  define IMAGE_FILE_MACHINE_ARM64 0xAA64
#endif

// Application resources.
#include "resource.h"

// Single-instance mutex
static HANDLE g_hSingleInstanceMutex = NULL;

// DLL filename
// Location should be prepended with an arch-specific directory name.
static const TCHAR dll_filename[] = _T("rom-properties.dll");

// Bullet symbol
#ifdef _UNICODE
#  define BULLET _T("\x2022")
#else /* !_UNICODE */
#  define BULLET _T("*")
#endif

// Globals

/**
 * NOTE: svrplus.c assumes it's compiled for 32-bit i386.
 * Windows also supports i386 on the following platforms:
 * - ia64 (emulated) [Windows 2000 and XP]
 * - amd64 (native) [Windows XP 64-bit, Windows Server 2003, and later]
 * - arm (native, 32-bit only) [Windows RT]
 * - arm64 (native) [Windows 10]
 *
 * Non-i386 versions of Windows NT 4.0 and earlier are not supported.
 * (Then again, Windows NT 4.0 on i386 isn't supported, either.)
 */
#if !defined(_M_IX86) && !defined(__i386__)
#  error svrplus should be compiled for 32-bit i386 only
#endif

// Runtime architecture
// NOTE: Named CPU_* to match CMakeLists.txt.
typedef enum {
	CPU_unknown	= 0,

	CPU_i386	= 1,
	CPU_amd64	= 2,
	CPU_ia64	= 3,
	CPU_arm		= 4,
	CPU_arm64	= 5,

	CPU_MAX
} SysArch;

typedef struct _s_arch_tbl_t {
	const TCHAR *name;	// "User-friendly" name, e.g. "amd64"
	const TCHAR *runtime;	// Runtime name, e.g. "x64"
} s_arch_tbl_t;

static const s_arch_tbl_t s_arch_tbl[] = {
	{_T("Unknown"),	NULL},		// CPU_unknown
	{_T("i386"),	_T("x86")},	// CPU_i386
	{_T("amd64"),	_T("x64")},	// CPU_amd64
	{_T("ia64"),	NULL},		// CPU_ia64
	{_T("arm"),	NULL},		// CPU_arm
	{_T("arm64"),	_T("arm64")},	// CPU_arm64
};

// Array of architectures to check
// This always includes i386, and then one or two more
// depending on the detected host architecture.
#define MAX_ARCHS 4
static uint8_t g_archs[MAX_ARCHS] = {CPU_i386, CPU_unknown, CPU_unknown, CPU_unknown};
static uint8_t g_arch_count = 1;

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
	const RECT *lpRect;
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
		lpRect = &rectStatus1_icon;
		Static_SetIcon(hStaticIcon, hIcon);
	} else {
		// Hide the icon.
		sw_status = SW_HIDE;
		lpRect = &rectStatus1_noIcon;
	}

	// Adjust the icon and Status1 rectangle.
	ShowWindow(hStaticIcon, sw_status);
	SetWindowPos(hStatus1, NULL, lpRect->left, lpRect->top,
		lpRect->right - lpRect->left, lpRect->bottom - lpRect->top,
		SWP_NOACTIVATE | SWP_NOREDRAW | SWP_NOZORDER);

	SetWindowText(hStatus1, line1);
	SetWindowText(hStatus2, line2);
	ShowWindow(hStatus1, SW_SHOW);
	ShowWindow(hStatus2, SW_SHOW);

	// Invalidate the full line 1 rectangle to prevent garbage from
	// being drawn between the icon and the label.
	InvalidateRect(hDlg, &rectStatus1_noIcon, TRUE);
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
 * @param arch		[in] System architecture to use.
 * @return Length of path in characters, excluding NULL terminator, on success; negative POSIX error code on error.
 */
static int GetSystemDirFilePath(TCHAR *pszPath, size_t cchPath, const TCHAR *filename, SysArch arch)
{
	unsigned int len, cchFilename;
	const TCHAR *system32_dir;

	if (arch < CPU_unknown || arch >= CPU_MAX)
		return -EINVAL;

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

	// Determine which system directory to use.
	// FIXME: i386 on ARM?
#ifdef _WIN64
#  define HOST_ARCH_DIR(i386, amd64) (amd64)
#else /* !_WIN64 */
#  define HOST_ARCH_DIR(i386, amd64) (i386)
#endif
	static const TCHAR *const system32_dir_tbl[] = {
		NULL,						// CPU_unknown
		HOST_ARCH_DIR(_T("System32"), _T("SysWOW64")),	// CPU_i386
		HOST_ARCH_DIR(_T("Sysnative"), _T("System32")),	// CPU_amd64
		HOST_ARCH_DIR(_T("Sysnative"), _T("System32")),	// CPU_ia64
		HOST_ARCH_DIR(_T("System32"), _T("SysWOW64")),	// CPU_arm [TODO: Verify]
		HOST_ARCH_DIR(_T("Sysnative"), _T("System32")),	// CPU_arm64 [TODO: Verify]
	};
	static_assert(ARRAY_SIZE(system32_dir_tbl) == CPU_MAX, "system32_dir_tbl[] is out of sync with g_archs[]!");
	system32_dir = system32_dir_tbl[arch];
	if (!system32_dir)
		return -EINVAL;

	// Append the System32 directory name and the filename.
	len += _sntprintf(&pszPath[len], cchPath-len, _T("%s\\%s"),
		system32_dir, filename);
	return len;
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
 * @param arch System architecture to check.
 * @return true if installed
 */
static bool CheckMsvc(SysArch arch)
{
	// Determine the MSVCRT DLL name.
	TCHAR msvcrt_path[MAX_PATH];
	int ret = GetSystemDirFilePath(msvcrt_path, _countof(msvcrt_path), _T("msvcp140.dll"), arch);
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
	ISR_INVALID_ARCH,		// invalid system architecture
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
 * @param arch		[in] System architecture to install.
 * @param pErrorCode	[out,opt] Additional error code.
 * @return InstallServerResult
 */
static InstallServerResult InstallServer(bool isUninstall, SysArch arch, DWORD *pErrorCode)
{
	const s_arch_tbl_t *s_arch;
	TCHAR regsvr32_path[MAX_PATH];
	TCHAR args[14 + MAX_PATH + 4 + 3 + _countof(dll_filename) + 32] = _T("regsvr32.exe \"");
	DWORD szModuleFn;
	TCHAR *bs;

	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	DWORD status;
	BOOL bRet;

	static_assert(ARRAY_SIZE(s_arch_tbl) == CPU_MAX, "s_arch_tbl[] size is out of sync with g_archs[]!");
	if (arch <= CPU_unknown || arch >= CPU_MAX) {
		// Invalid system architecture.
		return ISR_INVALID_ARCH;
	}

	// Determine the REGSVR32 path.
	int ret = GetSystemDirFilePath(regsvr32_path, _countof(regsvr32_path), _T("regsvr32.exe"), arch);
	if (ret <= 0) {
		// Unable to get the path.
		return ISR_FATAL_ERROR;
	}

	// Construct arguments
	// Construct path to rom-properties.dll inside the arguments
	SetLastError(ERROR_SUCCESS);	// required for XP
	szModuleFn = GetModuleFileName(HINST_THISCOMPONENT, &args[14], MAX_PATH);
	assert(szModuleFn != 0);
	assert(szModuleFn < MAX_PATH);
	if (szModuleFn == 0 || szModuleFn >= MAX_PATH || GetLastError() != ERROR_SUCCESS) {
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
	s_arch = &s_arch_tbl[arch];
	_tcscat_s(args, _countof(args), s_arch->name);
	_tcscat_s(args, _countof(args), _T("\\"));
	_tcscat_s(args, _countof(args), dll_filename);
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
 * @param arch		[in] system architecture to register.
 * @param sErrBuf	[out,opt] error message buffer
 * @param cchErrBuf	[in,opt] size of sErrBuf, in characters
 * @return InstallServerResult
 */
static InstallServerResult TryInstallServer(HWND hWnd,
	bool isUninstall, SysArch arch,
	TCHAR *sErrBuf, size_t cchErrBuf)
{
	const TCHAR *entry_point;
	const s_arch_tbl_t *s_arch;

	DWORD errorCode;
	InstallServerResult res = InstallServer(isUninstall, arch, &errorCode);

	if (!sErrBuf) {
		// No error message buffer specified.
		return res;
	}

	entry_point = (isUninstall
		? _T("DllUnregisterServer")
		: _T("DllRegisterServer"));
	s_arch = &s_arch_tbl[arch];

	// NOTE: Using _tcscpy_s() instead of _tcsncpy() because
	// _tcsncpy() zeroes the rest of the buffer.
#define LOAD_STRING(id) do { \
		LPCTSTR s_strtbl; \
		int ls_ret = LoadString(NULL, (id), (LPTSTR)&s_strtbl, 0); \
		assert(ls_ret > 0); \
		assert(s_strtbl != NULL); \
		_tcscpy_s(sErrBuf, cchErrBuf, (ls_ret > 0) ? s_strtbl : _T("RES ERR")); \
	} while (0)
#define LOAD_STRING_PRINTF(id, ...) do { \
		LPCTSTR s_strtbl; \
		int ls_ret = LoadString(NULL, (id), (LPTSTR)&s_strtbl, 0); \
		assert(ls_ret > 0); \
		assert(s_strtbl != NULL); \
		if (ls_ret > 0) { \
			_sntprintf(sErrBuf, cchErrBuf, s_strtbl, __VA_ARGS__); \
		} else { \
			_tcscpy_s(sErrBuf, cchErrBuf, _T("RES ERR")); \
		} \
	} while (0)

	switch (res) {
		case ISR_OK:
			// No error.
			if (cchErrBuf > 0) {
				sErrBuf[0] = _T('\0');
			}
			break;
		case ISR_FATAL_ERROR:
		default:
			LOAD_STRING(IDS_ISR_FATAL_ERROR);
			break;
		case ISR_INVALID_ARCH:
			LOAD_STRING_PRINTF(IDSF_ISR_INVALID_ARCH, arch);
			break;
		case ISR_FILE_NOT_FOUND:
			LOAD_STRING_PRINTF(IDSF_ISR_FILE_NOT_FOUND, s_arch->name, dll_filename);
			break;
		case ISR_CREATEPROCESS_FAILED:
			LOAD_STRING_PRINTF(IDSF_ISR_CREATEPROCESS_FAILED, errorCode);
			break;
		case ISR_PROCESS_STILL_ACTIVE:
			LOAD_STRING(IDS_ISR_PROCESS_STILL_ACTIVE);
			break;
		case ISR_REGSVR32_EXIT_CODE:
			switch (errorCode) {
				case REGSVR32_FAIL_ARGS:
					LOAD_STRING(IDS_ISR_REGSVR32_FAIL_ARGS);
					break;
				case REGSVR32_FAIL_OLE:
					LOAD_STRING(IDS_ISR_REGSVR32_FAIL_OLE);
					break;
				case REGSVR32_FAIL_LOAD:
					LOAD_STRING_PRINTF(IDSF_ISR_REGSVR32_FAIL_LOAD, s_arch->name, dll_filename);
					break;
				case REGSVR32_FAIL_ENTRY:
					LOAD_STRING_PRINTF(IDSF_ISR_REGSVR32_FAIL_ENTRY, s_arch->name, dll_filename, entry_point);
					break;
				case REGSVR32_FAIL_REG:
					LOAD_STRING_PRINTF(IDSF_ISR_REGSVR32_FAIL_REG, entry_point);
					break;
				default:
					LOAD_STRING_PRINTF(IDSF_ISR_REGSVR32_UNKNOWN_EXIT_CODE, errorCode);
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
	const ThreadParams *const params = (ThreadParams*)lpParameter;

	unsigned int i;
	InstallServerResult res[MAX_ARCHS] = {ISR_OK, ISR_OK, ISR_OK, ISR_OK};

	int status_icon;
	int ls_ret;
	uint16_t msg1_id;
	LPCTSTR msg1;

	// line2 message for error display.
	// If all DLLs registered successfully, this will stay blank.
	TCHAR msg2[1024];
	msg2[0] = _T('\0');

	// Try to (un)install the various architecture versions.
	for (i = 0; i < g_arch_count; i++) {
		TCHAR msg_ret[256];
		res[i] = TryInstallServer(params->hWnd, params->isUninstall, g_archs[i], msg_ret, _countof(msg_ret));
		if (res[i] != ISR_OK) {
			// Append the error message.
			// TODO: Use _sntprintf() to a temporary buffer?
			const s_arch_tbl_t *const s_arch = &s_arch_tbl[g_archs[i]];
			if (msg2[0] != _T('\0')) {
				_tcscat_s(msg2, _countof(msg2), _T("\n"));
			}
			_tcscat_s(msg2, _countof(msg2), BULLET _T(" "));
			_tcscat_s(msg2, _countof(msg2), s_arch->name);
			_tcscat_s(msg2, _countof(msg2), _T(": "));
			_tcscat_s(msg2, _countof(msg2), msg_ret);
		}
	}

	// FIXME: Plural localization for languages with more than two plural forms,
	// or where plural forms don't match English.
	if (msg2[0] == _T('\0')) {
		// DLL(s) registered successfully.
		if (g_arch_count > 1) {
			msg1_id = (params->isUninstall) ? IDS_DLLS_UNREG_OK : IDS_DLLS_REG_OK;
		} else {
			msg1_id = (params->isUninstall) ? IDS_DLL_UNREG_OK : IDS_DLL_REG_OK;
		}
		status_icon = MB_ICONINFORMATION;
	} else {
		// At least one of the DLLs failed to register.
		if (g_arch_count > 1) {
			msg1_id = (params->isUninstall) ? IDS_DLLS_UNREG_ERROR : IDS_DLLS_REG_ERROR;
		} else {
			msg1_id = (params->isUninstall) ? IDS_DLL_UNREG_ERROR : IDS_DLL_REG_ERROR;
		}
		status_icon = MB_ICONSTOP;;
	}

	ls_ret = LoadString(NULL, msg1_id, (LPTSTR)&msg1, 0); \
	assert(ls_ret > 0);
	assert(msg1 != NULL);
	if (ls_ret <= 0) {
		msg1 = _T("RES ERR");
	}

	ShowStatusMessage(params->hWnd, msg1, msg2, status_icon);
	MessageBeep(status_icon);
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

	HWND hStatus1, hExclaim;
	HMODULE hUser32;
	bool bErr;
	TCHAR line1[80], line2[1024];
	TCHAR s_missing_arch_names[128];
	unsigned int i;
	unsigned int missing_arch_count = 0;
	bool bHasMsvcForArch[MAX_ARCHS] = {false, false, false, false};

	// String table access
	LPCTSTR s_strtbl;
	int ls_ret;

	// OS version check
	// MSVC 2022 runtime requires Windows Vista or later.
	unsigned int vcyear, vcver;

	static_assert(ARRAY_SIZE(g_archs) == ARRAY_SIZE(bHasMsvcForArch), "bHasMsvcForArch[] is out of sync with g_archs[]!");

	// Clear the lines.
	line1[0] = _T('\0');
	line2[0] = _T('\0');
	s_missing_arch_names[0] = _T('\0');

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
	// TODO: Use SHGetStockIconInfo if available?
	hUser32 = GetModuleHandle(_T("user32.dll"));
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
	ls_ret = LoadString(NULL, IDS_MAIN_DESCRIPTION, (LPTSTR)&s_strtbl, 0);
	assert(ls_ret > 0);
	assert(s_strtbl != NULL);
	SetWindowText(GetDlgItem(hDlg, IDC_STATIC_DESC), (ls_ret > 0) ? s_strtbl : _T("RES ERR"));

	// MSVC 2022 runtime requires Windows Vista or later.
	if (IsWindowsVistaOrGreater()) {
		// Windows Vista or later. Use MSVC 2022.
		vcyear = 2022;
		vcver = 17;
	} else {
		// Windows XP/2003 or earlier. Use MSVC 2017.
		vcyear = 2017;
		vcver = 15;
	}

	for (i = 0; i < g_arch_count; i++) {
		bHasMsvcForArch[i] = CheckMsvc(g_archs[i]);
		if (!bHasMsvcForArch[i]) {
			if (missing_arch_count == 0) {
				ls_ret = LoadString(NULL, IDSF_MSVCRT_DOWNLOAD_AT, (LPTSTR)&s_strtbl, 0);
				assert(ls_ret > 0);
				assert(s_strtbl != NULL);
				if (ls_ret > 0) {
					_sntprintf(line2, _countof(line2), s_strtbl, vcyear);
				} else {
					_tcscpy_s(line2, _countof(line2), _T("RES ERR"));
				}
			} else if (missing_arch_count > 0) {
				// TODO: Localize; improve by adding "and" where necessary.
				// May require removing the comma for only two archs.
				_tcscat_s(s_missing_arch_names, _countof(s_missing_arch_names), _T(", "));
			}

			const s_arch_tbl_t *const s_arch = &s_arch_tbl[g_archs[i]];
			_tcscat_s(s_missing_arch_names, _countof(s_missing_arch_names), s_arch->name);
			missing_arch_count++;
		}
	}

	if (missing_arch_count > 0) {
		// One or more MSVC runtime versions are missing.
		// FIXME: Plural localization for languages with more than two plural forms,
		// or where plural forms don't match English.
		const uint16_t line1_id = (missing_arch_count == 1) ? IDSF_MSVCRT_MISSING_ONE : IDSF_MSVCRT_MISSING_MULTIPLE;
		ls_ret = LoadString(NULL, line1_id, (LPTSTR)&s_strtbl, 0);
		assert(ls_ret > 0);
		assert(s_strtbl != NULL);
		if (ls_ret > 0) {
			_sntprintf(line1, _countof(line1), s_strtbl, s_missing_arch_names, vcyear);
		} else {
			_tcscpy_s(line1, _countof(line1), _T("RES ERR"));
		}

		for (i = 0; i < g_arch_count; i++) {
			TCHAR s_runtime_line[160];
			const s_arch_tbl_t *s_arch;
			if (bHasMsvcForArch[i])
				continue;

			s_arch = &s_arch_tbl[g_archs[i]];
			if (!s_arch->runtime) {
				// No runtime name specified...
				continue;
			}
			_sntprintf(s_runtime_line, _countof(s_runtime_line),
				_T("\n") BULLET _T(" %s: <a href=\"https://aka.ms/vs/%u/release/VC_redist.%s.exe\">")
					_T("https://aka.ms/vs/%u/release/VC_redist.%s.exe</a>"),
				s_arch->name, vcver, s_arch->runtime, vcver, s_arch->runtime);
			_tcscat_s(line2, _countof(line2), s_runtime_line);
		}
	}

	// Show the status message.
	// If line1 is set, an error occurred, so we should
	// show the exclamation icon and disable the buttons.
	bErr = (line1[0] != _T('\0'));
	ShowStatusMessage(hDlg, line1, line2, (bErr ? MB_ICONEXCLAMATION : 0));
	EnableButtons(hDlg, !bErr);

	// FIXME: INITIAL COMMIT: Set focus to the "Install" button.
	// NOTE: Not working...???
	SetFocus(GetDlgItem(hDlg, IDC_BUTTON_INSTALL));
}

/**
 * IDC_BUTTON_INSTALL / IDC_BUTTON_UNINSTALL handler.
 * @param hDlg Dialog handle.
 * @param isUninstall True for uninstall; false for install.
 */
static void HandleInstallUninstall(HWND hDlg, bool isUninstall)
{
	HANDLE hThread;
	LPCTSTR msg;
	int ls_ret;
	uint16_t msg_id;

	if (g_inProgress) {
		// Already (un)installing...
		return;
	}
	g_inProgress = true;

	if (g_arch_count > 1) {
		msg_id = (isUninstall) ? IDS_DLLS_UNREGISTERING : IDS_DLLS_REGISTERING;
	} else {
		msg_id = (isUninstall) ? IDS_DLL_UNREGISTERING : IDS_DLL_REGISTERING;
	}
	ls_ret = LoadString(NULL, msg_id, (LPTSTR)&msg, 0);
	assert(ls_ret > 0);
	assert(msg != NULL);
	ShowStatusMessage(hDlg, (ls_ret > 0) ? msg : _T("RES ERR"), _T(""), 0);

	EnableButtons(hDlg, false);
	DlgUpdateCursor();

	// The installation is done on a separate thread so that we don't lock the message loop
	threadParams.hWnd = hDlg;
	threadParams.isUninstall = isUninstall;
	hThread = (HANDLE)_beginthreadex(NULL, 0, ThreadProc, &threadParams, 0, NULL);
	if (!hThread) {
		// Couldn't start the worker thread.
		TCHAR threadErr[128];
		const DWORD lastError = GetLastError();
		// TODO: Localize this.
		_sntprintf(threadErr, _countof(threadErr), BULLET _T(" Win32 error code: %u"), lastError);

		ls_ret = LoadString(NULL, IDS_ERROR_STARTING_WORKER_THREAD, (LPTSTR)&msg, 0);
		assert(ls_ret > 0);
		assert(msg != NULL);
		ShowStatusMessage(hDlg, (ls_ret > 0) ? msg : _T("RES ERR"), threadErr, MB_ICONSTOP);
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
					// - https://docs.microsoft.com/en-us/windows/win32/api/shellapi/nf-shellapi-shellexecutew
					// - https://devblogs.microsoft.com/oldnewthing/20061108-05/?p=29083
					pNMLink = (const NMLINK*)pHdr;
					ret = (INT_PTR)ShellExecute(NULL, _T("open"), pNMLink->item.szUrl, NULL, NULL, SW_SHOW);
					if (ret <= 32) {
						// ShellExecute() failed.
						LPCTSTR msg;
						int ls_ret;

						TCHAR err[128];
						ls_ret = LoadString(NULL, IDSF_ERROR_COULD_NOT_OPEN_URL, (LPTSTR)&msg, 0);
						assert(ls_ret > 0);
						assert(msg != NULL);
						if (ls_ret > 0) {
							_sntprintf(err, _countof(err), msg, (int)ret);
						} else {
							_tcscpy_s(err, _countof(err), _T("RES ERR"));
						}

						ls_ret = LoadString(NULL, IDS_ERROR_COULD_NOT_OPEN_URL_TITLE, (LPTSTR)&msg, 0);
						assert(ls_ret > 0);
						assert(msg != NULL);
						MessageBox(hDlg, err, (ls_ret > 0) ? msg : _T("RES ERR"), MB_ICONERROR);
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
 * Convert an IMAGE_FILE_MACHINE_* value to SysArch.
 * @param ifm IMAGE_FILE_MACHINE*
 * @return SysArch, or CPU_unknown if not supported.
 */
static SysArch ifm_to_SysArch(USHORT ifm)
{
	typedef struct _ifm_to_cpu_tbl_t {
		uint16_t ifm;
		uint8_t cpu;
	} ifm_to_cpu_tbl_t;

	static const ifm_to_cpu_tbl_t ifm_to_cpu_tbl[] = {
		{IMAGE_FILE_MACHINE_I386,	CPU_i386},
		{IMAGE_FILE_MACHINE_AMD64,	CPU_amd64},
		{IMAGE_FILE_MACHINE_IA64,	CPU_ia64},
		{IMAGE_FILE_MACHINE_ARM,	CPU_arm},
		{IMAGE_FILE_MACHINE_THUMB,	CPU_arm},	// TODO: Verify
		{IMAGE_FILE_MACHINE_ARMNT,	CPU_arm},	// TODO: Verify
		{IMAGE_FILE_MACHINE_ARM64,	CPU_arm64},
	};
	static const ifm_to_cpu_tbl_t *const p_end = &ifm_to_cpu_tbl[ARRAY_SIZE(ifm_to_cpu_tbl)];
	for (const ifm_to_cpu_tbl_t *p = ifm_to_cpu_tbl; p < p_end; p++) {
		if (p->ifm == ifm) {
			return p->cpu;
		}
	}

	// Not found and/or supported.
	return CPU_unknown;
}

/**
 * Check the system architecture(s).
 * @return 0 on success; non-zero on error.
 */
static int check_system_architectures(void)
{
	HMODULE hKernel32;

	typedef BOOL (WINAPI *PFNISWOW64PROCESS2)(HANDLE hProcess, USHORT *pProcessMachine, USHORT *pNativeMachine);
	typedef BOOL (WINAPI *PFNISWOW64PROCESS)(HANDLE hProcess, PBOOL Wow64Process);
	union {
		PFNISWOW64PROCESS2 pfnIsWow64Process2;
		PFNISWOW64PROCESS pfnIsWow64Process;
	} pfn;

	// WoW64 functions are located in kernel32.dll.
	// NOTE: Using GetProcAddress() because regular linking would
	// prevent svrplus from running on earlier versions of Windows.
	hKernel32 = GetModuleHandle(_T("kernel32.dll"));
	assert(hKernel32 != NULL);
	if (!hKernel32) {
		// Something is seriously wrong if kernel32 isn't loaded...
		CloseHandle(g_hSingleInstanceMutex);
		DebugBreak();
		return -1;
	}

	// Check for IsWow64Process2().
	pfn.pfnIsWow64Process2 = (PFNISWOW64PROCESS2)GetProcAddress(hKernel32, "IsWow64Process2");
	if (pfn.pfnIsWow64Process2) {
		// IsWow64Process2() is available.
		// Check the machine architecture(s).
		USHORT processMachine, nativeMachine;
		BOOL bRet = pfn.pfnIsWow64Process2(GetCurrentProcess(), &processMachine, &nativeMachine);
		if (!bRet) {
			// Failed?! This shouldn't happen...
			CloseHandle(g_hSingleInstanceMutex);
			DebugBreak();
			return -2;
		}

		// processMachine contains the emulated architecture if
		// running under WOW64, or IMAGE_FILE_MACHINE_UNKNOWN if not.
		// We won't bother checking processMachine. Instead, use
		// a hard-coded set for each host architecture.
		switch (ifm_to_SysArch(nativeMachine)) {
			case CPU_unknown:
				// Not supported. Show an error message.
				// TODO
				CloseHandle(g_hSingleInstanceMutex);
				DebugBreak();
				return -3;
			case CPU_i386:
				// g_archs[] starts with CPU_i386, so we're done.
				break;
			case CPU_amd64:
				g_archs[g_arch_count++] = CPU_amd64;
				break;
			case CPU_ia64:
				g_archs[g_arch_count++] = CPU_ia64;
				break;
			case CPU_arm:
				g_archs[g_arch_count++] = CPU_arm;
				break;
			case CPU_arm64:
				// NOTE: amd64 was added starting with Windows 10 Build 21277.
				// https://blogs.windows.com/windows-insider/2020/12/10/introducing-x64-emulation-in-preview-for-windows-10-on-arm-pcs-to-the-windows-insider-program/
				// TODO: Check for it!
				g_archs[g_arch_count++] = CPU_amd64;
				g_archs[g_arch_count++] = CPU_arm64;
				break;
		}
		return 0;
	}

	// Check for IsWow64Process().
	pfn.pfnIsWow64Process = (PFNISWOW64PROCESS)GetProcAddress(hKernel32, "IsWow64Process");
	if (pfn.pfnIsWow64Process) {
		// IsWow64Process() is available.
		BOOL bWow;
		if (!pfn.pfnIsWow64Process(GetCurrentProcess(), &bWow)) {
			// Failed?! This shouldn't happen...
			CloseHandle(g_hSingleInstanceMutex);
			DebugBreak();
			return -4;
		}

		if (bWow) {
			// This system is either i386/amd64 or i386/ia64.
			// Assuming i386/amd64 for now.
			// FIXME: IsWow64Process() doesn't distinguish between i386/amd64 and i386/ia64.
			// TODO: Use GetNativeSystemInfo() maybe? (Or, just forget about Itanium...)
			g_archs[g_arch_count++] = CPU_amd64;
		}
		return 0;
	}

	// No functions are available.
	// System is only i386.
	return 0;
}

/**
 * Entry point
 */
int CALLBACK _tWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPTSTR lpCmdLine, _In_ int nCmdShow)
{
	// Restrict DLL lookups.
	rp_secure_restrict_dll_lookups();
	// Set Win32 security options.
	rp_secure_param_t param;
	param.bHighSec = FALSE;
	rp_secure_enable(param);

	// Unused parameters (Win16 baggage)
	RP_UNUSED(hPrevInstance);
	RP_UNUSED(lpCmdLine);

	// Check if another instance of svrplus is already running.
	// References:
	// - https://stackoverflow.com/questions/4191465/how-to-run-only-one-instance-of-application
	// - https://stackoverflow.com/a/33531179
	g_hSingleInstanceMutex = CreateMutex(NULL, TRUE, _T("Local\\com.gerbilsoft.rom-properties.svrplus"));
	if (!g_hSingleInstanceMutex || GetLastError() == ERROR_ALREADY_EXISTS) {
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
#if 0
	setlocale(LC_ALL, "");
	// NOTE: Revert LC_CTYPE to "C" to fix UTF-8 output.
	// (Needed for MSVC 2022; does nothing for MinGW-w64 11.0.0)
	setlocale(LC_CTYPE, "C");
#endif

	// Check the system architecture(s).
	if (check_system_architectures() != 0) {
		// Failed...
		// NOTE: check_system_architectures() releases
		// g_hSingleInstanceMutex on failure.
		return EXIT_FAILURE;
	}

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

	CloseHandle(g_hSingleInstanceMutex);
	return EXIT_SUCCESS;
}
