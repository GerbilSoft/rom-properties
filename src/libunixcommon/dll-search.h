/***************************************************************************
 * ROM Properties Page shell extension. (libunixcommon)                    *
 * dll-search.h: Function to search for a usable rom-properties library.   *
 *                                                                         *
 * Copyright (c) 2016-2017 by David Korth.                                 *
 *                                                                         *
 * This program is free software; you can redistribute it and/or modify it *
 * under the terms of the GNU General Public License as published by the   *
 * Free Software Foundation; either version 2 of the License, or (at your  *
 * option) any later version.                                              *
 *                                                                         *
 * This program is distributed in the hope that it will be useful, but     *
 * WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 * GNU General Public License for more details.                            *
 *                                                                         *
 * You should have received a copy of the GNU General Public License along *
 * with this program; if not, write to the Free Software Foundation, Inc., *
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.           *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_RP_STUB_DLL_SEARCH_H__
#define __ROMPROPERTIES_RP_STUB_DLL_SEARCH_H__

#ifdef __cplusplus
extern "C" {
#endif

#ifndef ATTR_PRINTF
# ifdef __GNUC__
#  define ATTR_PRINTF(arg_format, arg_varargs) __attribute__ ((format (printf, arg_format, arg_varargs)))
# else
#  define ATTR_PRINTF(arg_format, arg_varargs)
# endif
#endif

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
