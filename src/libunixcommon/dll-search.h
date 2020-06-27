/***************************************************************************
 * ROM Properties Page shell extension. (libunixcommon)                    *
 * dll-search.h: Function to search for a usable rom-properties library.   *
 *                                                                         *
 * Copyright (c) 2016-2017 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_RP_STUB_DLL_SEARCH_H__
#define __ROMPROPERTIES_RP_STUB_DLL_SEARCH_H__

#ifdef __cplusplus
extern "C" {
#endif

// Common definitions, including function attributes.
#include "common.h"

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

#endif /* __ROMPROPERTIES_RP_STUB_DLL_SEARCH_H__ */
