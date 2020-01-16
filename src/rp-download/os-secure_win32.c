/***************************************************************************
 * ROM Properties Page shell extension. (rp-download)                      *
 * os-secure_win32.c: OS security functions. (Win32)                       *
 *                                                                         *
 * Copyright (c) 2016-2020 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "os-secure.h"

// C includes.
#include <assert.h>
#include <errno.h>

// libwin32common
#include "libwin32common/RpWin32_sdk.h"
#include "libwin32common/secoptions.h"

// Windows includes.
#include <sddl.h>

typedef enum {
	INTEGRITY_NOT_SUPPORTED	= -1,
	INTEGRITY_LOW		= 0,
	INTEGRITY_MEDIUM	= 1,
	INTEGRITY_HIGH		= 2,
} IntegrityLevel;

/**
 * Get the process's current integrity level.
 * @return IntegrityLevel.
 */
static IntegrityLevel get_integrity_level(void)
{
	// Reference: https://kb.digital-detective.net/display/BF/Understanding+and+Working+in+Protected+Mode+Internet+Explorer
	IntegrityLevel ret = INTEGRITY_NOT_SUPPORTED;
	BOOL bRet;

	HANDLE hToken = NULL;
	PTOKEN_MANDATORY_LABEL pTIL = NULL;
	PUCHAR pucSidSubAuthorityCount;
	PDWORD pdwIntegrityLevel;
	DWORD dwLengthNeeded;

	// Process integrity levels are supported starting with Windows Vista.
	OSVERSIONINFO osvi;
	osvi.dwOSVersionInfoSize = sizeof(osvi);
	if (!GetVersionEx(&osvi) || osvi.dwMajorVersion < 6) {
		// Either we failed to get the OS version information,
		// or the OS is earlier than Windows Vista.
		// Assuming integrity levels are not supported.
		return ret;
	}

	if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY | TOKEN_QUERY_SOURCE, &hToken)) {
		// Failed to open the process token.
		// Assume low integrity is not supported.
		// TODO: Check GetLastError()?
		goto out;
	}

	// Get the integrity level.
	bRet = GetTokenInformation(hToken, TokenIntegrityLevel, NULL, 0, &dwLengthNeeded);
	assert(bRet == 0);	// should fail here
	assert(GetLastError() == ERROR_INSUFFICIENT_BUFFER);
	if (bRet || GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
		// We didn't fail, or the error was incorrect.
		goto out;
	}

	pTIL = malloc(dwLengthNeeded);
	if (!pTIL) {
		// malloc() failed.
		goto out;
	}

	if (!GetTokenInformation(hToken, TokenIntegrityLevel, pTIL, dwLengthNeeded, &dwLengthNeeded)) {
		// GetTokenInformation() failed.
		goto out;
	}

	// Get the SID sub-authority value.
	// This is equivalent to the integrity level.
	pucSidSubAuthorityCount = GetSidSubAuthorityCount(pTIL->Label.Sid);
	if (!pucSidSubAuthorityCount) {
		// SID is invalid.
		goto out;
	}
	pdwIntegrityLevel = GetSidSubAuthority(pTIL->Label.Sid, *pucSidSubAuthorityCount - 1);
	if (!pdwIntegrityLevel) {
		// SID is invalid.
		goto out;
	}

	// Check the level.
	if (*pdwIntegrityLevel < SECURITY_MANDATORY_MEDIUM_RID) {
		// Low integrity.
		ret = INTEGRITY_LOW;
	} else if (*pdwIntegrityLevel < SECURITY_MANDATORY_HIGH_RID) {
		// Medium integrity.
		ret = INTEGRITY_MEDIUM;
	} else /*if (*pdwIntegrityLevel >= SECURITY_MANDATORY_HIGH_RID)*/ {
		// High integrity.
		ret = INTEGRITY_HIGH;
	}

out:
	free(pTIL);
	if (hToken) {
		CloseHandle(hToken);
	}
	return ret;
}

/**
 * Enable OS-specific security functionality.
 * @return 0 on success; negative POSIX error code on error.
 */
int rp_download_os_secure(void)
{
	IntegrityLevel level;

	// Set Win32 security options.
	secoptions_init();

	// Check the process integrity level.
	// TODO: If it's higher than low, relaunch the program with low integrity if supported.
	level = get_integrity_level();
	if (level > INTEGRITY_LOW) {
		_ftprintf(stderr, _T("WARNING: Not running as low integrity!!!\n"));
	}

	return 0;
}
