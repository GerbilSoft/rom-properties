/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * ExecRpDownload_win32.cpp: Execute rp-download.exe. (Win32)              *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "CacheManager.hpp"

// Windows includes.
#include "libwin32common/RpWin32_sdk.h"
#include "librptext/wchar.hpp"

// librpsecure
#include "librpsecure/win32/integrity_level.h"

// C++ includes
#include <string>
using std::string;
using std::wstring;

namespace LibRomData {

/**
 * Execute rp-download. (Win32 version)
 * @param filteredCacheKey Filtered cache key.
 * @return 0 on success; negative POSIX error code on error.
 */
int CacheManager::execRpDownload(const string &filteredCacheKey)
{
	// The executable should be located in the DLL directory.
	TCHAR dll_filename[MAX_PATH];
	SetLastError(ERROR_SUCCESS);	// required for XP
	DWORD dwResult = GetModuleFileName(HINST_THISCOMPONENT,
		dll_filename, _countof(dll_filename));
	if (dwResult == 0 || dwResult >= _countof(dll_filename) || GetLastError() != ERROR_SUCCESS) {
		// Cannot get the DLL filename.
		// TODO: Windows XP doesn't SetLastError() if the
		// filename is too big for the buffer.
		return -EINVAL;
	}

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

#ifdef ENABLE_EXTRA_SECURITY
	// Attempt to create a low-integrity token.
	HANDLE hLowToken = CreateIntegrityLevelToken(SECURITY_MANDATORY_LOW_RID);
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
	} else
#endif /* ENABLE_EXTRA_SECURITY */
	{
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
