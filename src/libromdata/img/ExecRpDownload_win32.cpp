/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * ExecRpDownload_win32.cpp: Execute rp-download.exe. (Win32)              *
 *                                                                         *
 * Copyright (c) 2016-2020 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "CacheManager.hpp"

// Windows includes.
#include "libwin32common/RpWin32_sdk.h"
#include "librpbase/TextFuncs_wchar.hpp"
#include <sddl.h>

// librpthreads
#include "librpthreads/pthread_once.h"

// C++ includes.
#include <string>
using std::string;
using std::wstring;

namespace LibRomData {

// pthread_once() control variable.
static pthread_once_t once_control = PTHREAD_ONCE_INIT;
// Are we running Windows Vista or later?
static bool isVista = false;

/**
 * Check if we're running Windows Vista or later.
 * Called by pthread_once().
 */
static void initIsVista(void)
{
	// TODO: Use versionhelpers.h.
	OSVERSIONINFO osvi;
	osvi.dwOSVersionInfoSize = sizeof(osvi);
	isVista = (GetVersionEx(&osvi) && osvi.dwMajorVersion >= 6);
}

/**
 * Create a low-integrity token.
 * This requires Windows Vista or later.
 *
 * Caller must call CloseHandle() on the token when done using it.
 *
 * @return Low-integrity token, or nullptr on error.
 */
static HANDLE createLowIntegrityToken(void)
{
	// Are we running Windows Vista or later?
	pthread_once(&once_control, initIsVista);
	if (!isVista) {
		// Not running Windows Vista or later.
		// Can't create a low-integrity token.
		return nullptr;
	}

	// Reference: https://docs.microsoft.com/en-us/previous-versions/dotnet/articles/bb625960(v=msdn.10)?redirectedfrom=MSDN

	// Low-integrity SID
	// NOTE: The MSDN example has an incorrect integrity value.
	// References:
	// - https://stackoverflow.com/questions/3139938/windows-7-x64-low-il-process-msdn-example-does-not-work
	// - https://stackoverflow.com/a/3842990
	TCHAR szIntegritySid[] = _T("S-1-16-4096");
	PSID pIntegritySid = nullptr;

	HANDLE hToken;
	BOOL bRet = OpenProcessToken(
		GetCurrentProcess(),	// ProcessHandle
		TOKEN_DUPLICATE |
		TOKEN_ADJUST_DEFAULT |
		TOKEN_QUERY |
		TOKEN_ASSIGN_PRIMARY,	// DesiredAccess
		&hToken);		// TokenHandle
	if (!bRet) {
		// Unable to open the process token.
		return nullptr;
	}

	// Duplicate the token.
	HANDLE hNewToken;
	bRet = DuplicateTokenEx(
		hToken,			// hExistingToken
		0,			// dwDesiredAccess
		nullptr,		// lpTokenAttributes
		SecurityImpersonation,	// ImpersonationLevel
		TokenPrimary,		// TokenType
		&hNewToken);		// phNewToken
	CloseHandle(hToken);
	if (!bRet) {
		// Unable to duplicate the token.
		return nullptr;
	}

	// Convert the string SID to a real SID.
	if (!ConvertStringSidToSid(szIntegritySid, &pIntegritySid)) {
		// Failed to convert the SID.
		CloseHandle(hNewToken);
		return nullptr;
	}

	// Token settings.
	TOKEN_MANDATORY_LABEL til;
	til.Label.Attributes = SE_GROUP_INTEGRITY;
	til.Label.Sid = pIntegritySid;

	// Set the process integrity level.
	bRet = SetTokenInformation(hNewToken, TokenIntegrityLevel, &til,
		sizeof(TOKEN_MANDATORY_LABEL) + GetLengthSid(pIntegritySid));
	LocalFree(pIntegritySid);
	if (!bRet) {
		// Failed to set the process integrity level.
		CloseHandle(hNewToken);
		return nullptr;
	}

	// Low-integrity token created.
	return hNewToken;
}

/**
 * Execute rp-download. (Win32 version)
 * @param filteredCacheKey Filtered cache key.
 * @return 0 on success; negative POSIX error code on error.
 */
int CacheManager::execRpDownload(const string &filteredCacheKey)
{
	// The executable should be located in the DLL directory.
	extern TCHAR dll_filename[];
	tstring rp_download_exe = dll_filename;
	tstring::size_type bs = rp_download_exe.rfind(_T('\\'));
	if (bs == tstring::npos) {
		// Backslash not found.
		// TODO: Better error code?
		return -EINVAL;
	}
	rp_download_exe.resize(bs+1);
	rp_download_exe += _T("rp-download.exe");

	// CreateProcessW() *can* modify the command line,
	// so we'll store in a tstring.
	// NOTE: Spaces are allowed in cache keys, so everything
	// needs to be quoted properly.
	tstring t_filteredCacheKey = U82T_s(filteredCacheKey);
	tstring t_cmd_line;
	t_cmd_line.reserve(rp_download_exe.size() + 5 + t_filteredCacheKey.size());
	t_cmd_line += _T('"');
	t_cmd_line += rp_download_exe;
	t_cmd_line += _T("\" \"");
	t_cmd_line += t_filteredCacheKey;
	t_cmd_line += _T('"');

	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	BOOL bRet;
	memset(&si, 0, sizeof(si));
	memset(&pi, 0, sizeof(pi));
	si.cb = sizeof(si);

	// Attempt to create a low-integrity token.
	HANDLE hLowToken = createLowIntegrityToken();
	if (hLowToken) {
		// Low-integrity token created. Create the process using this token.
		bRet = CreateProcessAsUser(
			hLowToken,			// hToken
			rp_download_exe.c_str(),	// lpApplicationName
			&t_cmd_line[0],			// lpCommandLine
			nullptr,			// lpProcessAttributes
			nullptr,			// lpThreadAttributes
			false,				// bInheritHandles
			CREATE_NO_WINDOW,		// dwCreationFlags
			nullptr,			// lpEnvironment
			nullptr,			// lpCurrentDirectory
			&si,				// lpStartupInfo
			&pi);				// lpProcessInformation
		CloseHandle(hLowToken);
	} else {
		// Unable to create a low-integrity token.
		// Create the process normally.
		bRet = CreateProcess(
			rp_download_exe.c_str(),	// lpApplicationName
			&t_cmd_line[0],			// lpCommandLine
			nullptr,			// lpProcessAttributes
			nullptr,			// lpThreadAttributes
			false,				// bInheritHandles
			CREATE_NO_WINDOW,		// dwCreationFlags
			nullptr,			// lpEnvironment
			nullptr,			// lpCurrentDirectory
			&si,				// lpStartupInfo
			&pi);				// lpProcessInformation
	}

	if (!bRet) {
		// Error starting rp-download.exe.
		// TODO: Try the architecture-specific subdirectory?
		// TODO: Better error code?
		return -ECHILD;
	}

	// Wait up to 10 seconds for the process to exit.
	DWORD dwRet = WaitForSingleObject(pi.hProcess, 10*1000);
	DWORD status = 0;
	bRet = GetExitCodeProcess(pi.hProcess, &status);
	if (dwRet != WAIT_OBJECT_0 || !bRet || status == STILL_ACTIVE) {
		// Process either timed out or failed.
		TerminateProcess(pi.hProcess, EXIT_FAILURE);
		CloseHandle(pi.hThread);
		CloseHandle(pi.hProcess);
		// TODO: Better error code?
		return -ECHILD;
	}

	CloseHandle(pi.hThread);
	CloseHandle(pi.hProcess);

	if (status != 0) {
		// rp-download failed for some reason.
		// TODO: Better error code?
		return -EIO;
	}

	// rp-download has successfully downloaded the file.
	return 0;
}

}
