/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * integrity_level.c: Integrity level manipulation for process tokens.     *
 *                                                                         *
 * Copyright (c) 2029 by David Korth.                                      *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_LIBWIN32COMMON_INTEGRITY_LEVEL_H__
#define __ROMPROPERTIES_LIBWIN32COMMON_INTEGRITY_LEVEL_H__

#include "RpWin32_sdk.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Create a token with the specified integrity level.
 * This requires Windows Vista or later.
 *
 * Caller must call CloseHandle() on the token when done using it.
 *
 * @param level Integrity level. (SECURITY_MANDATORY_*_RID)
 * @return New token, or NULL on error.
 */
HANDLE CreateIntegrityLevelToken(int level);

/**
 * Get the current process's integrity level.
 * @return Integrity level (SECURITY_MANDATORY_*_RID), or -1 on error.
 */
int GetProcessIntegrityLevel(void);

/**
 * Adjust the current process's integrity level.
 *
 * References:
 * - https://github.com/chromium/chromium/blob/4e88a3c4fa53bf4d3622d07fd13f3812d835e40f/sandbox/win/src/restricted_token_utils.cc
 * - https://github.com/chromium/chromium/blob/master/sandbox/win/src/restricted_token_utils.cc
 *
 * @param level Integrity level. (SECURITY_MANDATORY_*_RID)
 * @return 0 on success; GetLastError() on error.
 */
DWORD SetProcessIntegrityLevel(int level);

#ifdef __cplusplus
}
#endif

#endif /* __ROMPROPERTIES_LIBWIN32COMMON_INTEGRITY_LEVEL_H__ */
