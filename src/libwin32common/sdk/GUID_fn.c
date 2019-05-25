/***************************************************************************
 * ROM Properties Page shell extension. (libwin32common)                   *
 * GUID.h: GUID function reimplementations for ANSI builds.                *
 *                                                                         *
 * Copyright (c) 2016-2019 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
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
 * StringFromGUID2() implementation for ANSI builds.
 * @param rclsid	[in] CLSID
 * @param lpszClsidA	[out] Buffer for the CLSID string.
 * @param cchMax	[in] Length of lpszClsidA, in characters.
 */
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

/**
 * CLSIDFromString() implementation for ANSI builds.
 * NOTE: This implementation does NOT do ProgId lookups.
 * @param lpsz		[in] CLSID string.
 * @param pclsid	[out] CLSID
 * @return S_OK on success; E_FAIL on error.
 */
HRESULT CLSIDFromStringA(_In_ LPCSTR lpsz, _Out_ LPCLSID pclsid)
{
	// NOTE: This is only correct on little-endian systems.
	// Windows only supports little-endian, so that's fine.
	int ret = sscanf(lpsz, "{%08X-%04hX-%04hX-%02hhX%02hhX-%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX}",
		&pclsid->Data1, &pclsid->Data2, &pclsid->Data3,
		&pclsid->Data4[0], &pclsid->Data4[1], &pclsid->Data4[2], &pclsid->Data4[3],
		&pclsid->Data4[4], &pclsid->Data4[5], &pclsid->Data4[6], &pclsid->Data4[7]);
	return (ret == 11 ? S_OK : E_FAIL);
}

#endif /* !UNICODE */
