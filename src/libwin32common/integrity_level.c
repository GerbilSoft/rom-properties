/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * integrity_level.c: Integrity level manipulation for process tokens.     *
 *                                                                         *
 * Copyright (c) 2020 by David Korth.                                      *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

// NOTE: Using LocalAlloc()/LocalFree() here to prevent issues
// mixing and matching static and dynamic CRT versions.

#include "integrity_level.h"

// C includes.
#include <assert.h>
#include <errno.h>
#include <malloc.h>
#include <stdlib.h>

// libwin32common
#include "libwin32common/RpWin32_sdk.h"
// Windows includes.
#include <sddl.h>

// librpthreads
#include "librpthreads/pthread_once.h"

// stdboolx
#include "stdboolx.h"

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
#ifdef _MSC_VER
# pragma warning(push)
# pragma warning(disable: 4996)
#endif /* _MSC_VER */
	// TODO: Use versionhelpers.h.
	OSVERSIONINFO osvi;
	osvi.dwOSVersionInfoSize = sizeof(osvi);
	isVista = (GetVersionEx(&osvi) && osvi.dwMajorVersion >= 6);
#ifdef _MSC_VER
# pragma warning(pop)
#endif /* _MSC_VER */
}

/**
 * Create a low-integrity token.
 * This requires Windows Vista or later.
 *
 * Caller must call CloseHandle() on the token when done using it.
 *
 * @return Low-integrity token, or NULL on error.
 */
HANDLE CreateLowIntegrityToken(void)
{
	static const TCHAR szIntegritySid[] = _T("S-1-16-4096");
	PSID pIntegritySid = NULL;
	HANDLE hToken = NULL;
	HANDLE hNewToken = NULL;
	TOKEN_MANDATORY_LABEL tml;
	BOOL bRet;

	// Are we running Windows Vista or later?
	pthread_once(&once_control, initIsVista);
	if (!isVista) {
		// Not running Windows Vista or later.
		// Can't create a low-integrity token.
		return NULL;
	}

	// Reference: https://docs.microsoft.com/en-us/previous-versions/dotnet/articles/bb625960(v=msdn.10)?redirectedfrom=MSDN

	// Low-integrity SID
	// NOTE: The MSDN example has an incorrect integrity value.
	// References:
	// - https://stackoverflow.com/questions/3139938/windows-7-x64-low-il-process-msdn-example-does-not-work
	// - https://stackoverflow.com/a/3842990
	bRet = OpenProcessToken(
		GetCurrentProcess(),	// ProcessHandle
		TOKEN_DUPLICATE |
		TOKEN_ADJUST_DEFAULT |
		TOKEN_QUERY |
		TOKEN_ASSIGN_PRIMARY,	// DesiredAccess
		&hToken);		// TokenHandle
	if (!bRet) {
		// Unable to open the process token.
		goto out;
	}

	// Duplicate the token.
	bRet = DuplicateTokenEx(
		hToken,			// hExistingToken
		0,			// dwDesiredAccess
		NULL,			// lpTokenAttributes
		SecurityImpersonation,	// ImpersonationLevel
		TokenPrimary,		// TokenType
		&hNewToken);		// phNewToken
	if (!bRet) {
		// Unable to duplicate the token.
		goto out;
	}

	// Convert the string SID to a real SID.
	if (!ConvertStringSidToSid(szIntegritySid, &pIntegritySid)) {
		// Failed to convert the SID.
		CloseHandle(hNewToken);
		hNewToken = NULL;
		goto out;
	}

	// Token settings.
	tml.Label.Attributes = SE_GROUP_INTEGRITY;
	tml.Label.Sid = pIntegritySid;

	// Set the process integrity level.
	bRet = SetTokenInformation(
		hNewToken,			// TokenHandle
		TokenIntegrityLevel,		// TokenInformationClass
		&tml,				// TokenInformation
		sizeof(TOKEN_MANDATORY_LABEL) +
		GetLengthSid(pIntegritySid));	// TokenInformationLength
	if (!bRet) {
		// Failed to set the process integrity level.
		CloseHandle(hNewToken);
		hNewToken = NULL;
		goto out;
	}

out:
	LocalFree(pIntegritySid);
	if (hToken) {
		CloseHandle(hToken);
	}

	return hNewToken;
}

/**
 * Get the current process's integrity level.
 * @return IntegrityLevel.
 */
IntegrityLevel GetIntegrityLevel(void)
{
	// Reference: https://kb.digital-detective.net/display/BF/Understanding+and+Working+in+Protected+Mode+Internet+Explorer
	IntegrityLevel ret = INTEGRITY_NOT_SUPPORTED;
	BOOL bRet;

	HANDLE hToken = NULL;
	PTOKEN_MANDATORY_LABEL pTML = NULL;
	PUCHAR pucSidSubAuthorityCount;
	PDWORD pdwIntegrityLevel;
	DWORD dwLengthNeeded;

	// Are we running Windows Vista or later?
	pthread_once(&once_control, initIsVista);
	if (!isVista) {
		// Not running Windows Vista or later.
		// Can't get the integrity level.
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

	pTML = LocalAlloc(LMEM_FIXED, dwLengthNeeded);
	assert(pTML != NULL);
	if (!pTML) {
		// malloc() failed.
		goto out;
	}

	if (!GetTokenInformation(hToken, TokenIntegrityLevel, pTML, dwLengthNeeded, &dwLengthNeeded)) {
		// GetTokenInformation() failed.
		goto out;
	}

	// Get the SID sub-authority value.
	// This is equivalent to the integrity level.
	pucSidSubAuthorityCount = GetSidSubAuthorityCount(pTML->Label.Sid);
	if (!pucSidSubAuthorityCount) {
		// SID is invalid.
		goto out;
	}
	pdwIntegrityLevel = GetSidSubAuthority(pTML->Label.Sid, *pucSidSubAuthorityCount - 1);
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
	LocalFree(pTML);
	if (hToken) {
		CloseHandle(hToken);
	}
	return ret;
}

/**
 * Adjust the current process's integrity level.
 *
 * References:
 * - https://github.com/chromium/chromium/blob/4e88a3c4fa53bf4d3622d07fd13f3812d835e40f/sandbox/win/src/restricted_token_utils.cc
 * - https://github.com/chromium/chromium/blob/master/sandbox/win/src/restricted_token_utils.cc
 *
 * @param level IntegrityLevel.
 * @return 0 on success; GetLastError() on error.
 */
DWORD SetIntegrityLevel(IntegrityLevel level)
{
	HANDLE hToken;
	LPCTSTR lpszIntegritySid;
	PSID pIntegritySid = NULL;
	TOKEN_MANDATORY_LABEL tml;
	DWORD dwLastError;

	// Are we running Windows Vista or later?
	pthread_once(&once_control, initIsVista);
	if (!isVista) {
		// Not running Windows Vista or later.
		// Can't set the process integrity level.
		return INTEGRITY_NOT_SUPPORTED;
	}

	// TODO: Add Untrusted, Below Low, Medium Low, and System?
	switch (level) {
		case INTEGRITY_LOW:
			lpszIntegritySid = _T("S-1-16-4096");
			break;
		case INTEGRITY_MEDIUM:
			lpszIntegritySid = _T("S-1-16-8192");
			break;
		case INTEGRITY_HIGH:
			lpszIntegritySid = _T("S-1-16-12288");
			break;
		default:
			return ERROR_INVALID_PARAMETER;
	}

	if (!OpenProcessToken(
		GetCurrentProcess(),	// ProcessHandle
		TOKEN_ADJUST_DEFAULT,	// DesiredAccess
		&hToken))		// TokenHandle
	{
		// OpenProcessToken() failed.
		return GetLastError();
	}

	// Based on Chromium's SetTokenIntegrityLevel().
	if (!ConvertStringSidToSid(lpszIntegritySid, &pIntegritySid)) {
		// Failed to convert the SID.
		goto out;
	}

	tml.Label.Attributes = SE_GROUP_INTEGRITY;
	tml.Label.Sid = pIntegritySid;

	// Set the process integrity level.
	SetLastError(ERROR_INVALID_PARAMETER);
	if (!SetTokenInformation(
		hToken,				// TokenHandle
		TokenIntegrityLevel,		// TokenInformationClass
		&tml,				// TokenInformation
		sizeof(TOKEN_MANDATORY_LABEL) +
		GetLengthSid(pIntegritySid)));	// TokenInformationLength
	{
		// Failed to set the process integrity level.
		// NOTE: It may have succeeded anyway, in which case,
		// GetLastError() will return 0.
		dwLastError = GetLastError();
		goto out;
	}

	// Success!
	dwLastError = 0;

out:
	LocalFree(pIntegritySid);
	CloseHandle(hToken);
	return dwLastError;
}
