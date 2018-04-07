/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * TextFuncs_libc.c: Reimplementations of libc functions that aren't       *
 * present on this system.                                                 *
 *                                                                         *
 * Copyright (c) 2009-2018 by David Korth.                                 *
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
 * You should have received a copy of the GNU General Public License       *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.   *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_LIBRPBASE_TEXTFUNCS_LIBC_H__
#define __ROMPROPERTIES_LIBRPBASE_TEXTFUNCS_LIBC_H__

#include "config.librpbase.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef HAVE_STRNLEN
/**
 * String length with limit. (8-bit strings)
 * @param str The string itself
 * @param maxlen Maximum length of the string
 * @returns equivivalent to min(strlen(str), maxlen) without buffer overruns
 */
size_t strnlen(const char *str, size_t maxlen);
#endif /* HAVE_STRNLEN */

#ifndef HAVE_MEMMEM
/**
 * Find a string within a block of memory.
 * @param haystack Block of memory.
 * @param haystacklen Length of haystack.
 * @param needle String to search for.
 * @param needlelen Length of needle.
 * @return Location of needle in haystack, or NULL if not found.
 */
void *memmem(const void *haystack, size_t haystacklen,
	     const void *needle, size_t needlelen);
#endif /* HAVE_MEMMEM */

#ifdef __cplusplus
}
#endif

#endif /* __ROMPROPERTIES_LIBRPBASE_TEXTFUNCS_LIBC_H__ */
