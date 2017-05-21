/***************************************************************************
 * ROM Properties Page shell extension. (libwin32common)                   *
 * w32err.c: Error code mapping. (Windows to POSIX)                        *
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

#ifndef __ROMPROPERTIES_LIBWIN32COMMON_W32ERR_H__
#define __ROMPROPERTIES_LIBWIN32COMMON_W32ERR_H__

#include "RpWin32_sdk.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Convert a Win32 error code to a POSIX error code.
 * @param w32err Win32 error code.
 * @return Positive POSIX error code. (If no equivalent is found, default is EINVAL.)
 */
int w32err_to_posix(DWORD w32err);

#ifdef __cplusplus
}
#endif

#endif /* __ROMPROPERTIES_LIBWIN32COMMON_W32ERR_H__ */
