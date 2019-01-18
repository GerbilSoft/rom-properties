/***************************************************************************
 * ROM Properties Page shell extension. (libwin32common)                   *
 * GUID.h: GUID function reimplementations for ANSI builds.                *
 *                                                                         *
 * Copyright (c) 2016-2019 by David Korth.                                 *
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

#ifndef __ROMPROPERTIES_LIBWIN32COMMON_GUID_FN_H__
#define __ROMPROPERTIES_LIBWIN32COMMON_GUID_FN_H__

#include "../RpWin32_sdk.h"

#ifndef UNICODE

#ifdef __cplusplus
extern "C" {
#endif

/**
 * StringFromGUID2() wrapper function for ANSI builds.
 * @param rclsid	[in] CLSID
 * @param lpszClsidA	[out] Buffer for the CLSID string.
 * @param cchMax	[in] Length of lpszClsidA, in characters.
 */
int StringFromGUID2A(_In_ REFGUID rclsid, _Out_writes_(cchMax) LPSTR lpszClsidA, _In_ int cchMax);
#define StringFromGUID2(rclsid, lpszClsidA, cchMax) StringFromGUID2A((rclsid), (lpszClsidA), (cchMax))

#ifdef __cplusplus
}
#endif

#endif /* !UNICODE */

#endif /* __ROMPROPERTIES_LIBWIN32COMMON_GUID_FN_H__ */
