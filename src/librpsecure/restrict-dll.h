/***************************************************************************
 * ROM Properties Page shell extension. (librpsecure)                      *
 * restrict-dll.c: Restrict DLL lookups.                                   *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "librpsecure/config.librpsecure.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Restrict DLL lookups to system directories.
 *
 * After calling this function, any DLLs located in the application
 * directory will need to be loaded using LoadLibrary() with an
 * absolute path.
 *
 * @return 0 on success; non-zero on error.
 */
#if defined(_WIN32) && defined(ENABLE_EXTRA_SECURITY)
int rp_secure_restrict_dll_lookups(void);
#else /* !(_WIN32 && ENABLE_EXTRA_SECURITY) */
static inline int rp_secure_restrict_dll_lookups(void)
{
	/* do nothing */
	return 0;
}
#endif /* _WIN32 && ENABLE_EXTRA_SECURITY */

#ifdef __cplusplus
}
#endif
