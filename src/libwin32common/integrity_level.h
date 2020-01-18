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

// Simplified mapping of integrity levels.
typedef enum {
	INTEGRITY_NOT_SUPPORTED	= -1,
	INTEGRITY_LOW		= 0,
	INTEGRITY_MEDIUM	= 1,
	INTEGRITY_HIGH		= 2,
} IntegrityLevel;

/**
 * Create a low-integrity token.
 * This requires Windows Vista or later.
 *
 * Caller must call CloseHandle() on the token when done using it.
 *
 * @return Low-integrity token, or nullptr on error.
 */
HANDLE CreateLowIntegrityToken(void);

/**
 * Get the current process's integrity level.
 * @return IntegrityLevel.
 */
IntegrityLevel GetIntegrityLevel(void);

#ifdef __cplusplus
}
#endif

#endif /* __ROMPROPERTIES_LIBWIN32COMMON_INTEGRITY_LEVEL_H__ */
