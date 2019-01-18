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

#ifndef UNICODE

// StringFromGUID2()
#include <objbase.h>

// Our functions
#include "GUID_fn.h"

// C includes.
#include <assert.h>
#include <stdio.h>

/**
 * StringFromGUID2() wrapper function for ANSI builds.
 * @param rclsid	[in] CLSID
 * @param lpszClsidA	[out] Buffer for the CLSID string.
 * @param cchMax	[in] Length of lpszClsidA, in characters.
 */
#undef StringFromGUID2
int StringFromGUID2A(_In_ REFGUID rclsid, _Out_writes_(cchMax) LPSTR lpszClsidA, _In_ int cchMax)
{
	// NOTE: This is only correct on little-endian systems.
	// Windows only supports little-endian, so that's fine.
	snprintf(lpszClsidA, cchMax, "{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}",
		rclsid->Data1, rclsid->Data2, rclsid->Data3,
		rclsid->Data4[0], rclsid->Data4[1], rclsid->Data4[2], rclsid->Data4[3],
		rclsid->Data4[4], rclsid->Data4[5], rclsid->Data4[6], rclsid->Data4[7]);
	return ERROR_SUCCESS;
}

#endif /* !UNICODE */
