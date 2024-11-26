/***************************************************************************
 * ROM Properties Page shell extension. (libunixcommon)                    *
 * dll-search.h: Function to search for a usable rom-properties library.   *
 *                                                                         *
 * Copyright (c) 2016-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include <sys/types.h>

// Common definitions, including function attributes.
#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Get a process's executable name and, optionally, its parent process ID.
 * @param pid	[	in] Process ID.
 * @param pidname	[out] String buffer.
 * @param len		[in] Size of buf.
 * @param ppid	[out,opt] Parent process ID.
 * @return 0 on success; non-zero on error.
 */
ATTR_ACCESS_SIZE(write_only, 2, 3)
ATTR_ACCESS(write_only, 4)
int rp_get_process_name(pid_t pid, char *pidname, size_t len, pid_t *ppid);

#define LEVEL_DEBUG 0
#define LEVEL_ERROR 1
typedef int (*PFN_RP_DLL_DEBUG)(int level, const char *format, ...) ATTR_PRINTF(2, 3);

/**
 * Search for a rom-properties library.
 * @param symname	[in] Symbol name to look up.
 * @param ppDll		[out] Handle to opened library.
 * @param ppfn		[out] Pointer to function.
 * @param pfnDebug	[in,opt] Pointer to debug logging function. (printf-style) (may be NULL)
 * @return 0 on success; negative POSIX error code on error.
 */
int rp_dll_search(const char *symname, void **ppDll, void **ppfn, PFN_RP_DLL_DEBUG pfnDebug);

#ifdef __cplusplus
}
#endif
