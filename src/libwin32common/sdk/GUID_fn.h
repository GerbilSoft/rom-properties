/***************************************************************************
 * ROM Properties Page shell extension. (libwin32common)                   *
 * GUID.h: GUID function reimplementations for ANSI builds.                *
 *                                                                         *
 * Copyright (c) 2016-2022 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_LIBWIN32COMMON_GUID_FN_H__
#define __ROMPROPERTIES_LIBWIN32COMMON_GUID_FN_H__

#include "../RpWin32_sdk.h"

#ifndef UNICODE

#ifdef __cplusplus
extern "C" {
#endif

/**
 * StringFromGUID2() implementation for ANSI builds.
 * @param rclsid	[in] CLSID
 * @param lpszClsidA	[out] Buffer for the CLSID string.
 * @param cchMax	[in] Length of lpszClsidA, in characters.
 */
int WINAPI StringFromGUID2A(_In_ REFGUID rclsid, _Out_writes_(cchMax) LPSTR lpszClsidA, _In_ int cchMax);
#define StringFromGUID2(rclsid, lpszClsidA, cchMax) StringFromGUID2A((rclsid), (lpszClsidA), (cchMax))

/**
 * CLSIDFromString() implementation for ANSI builds.
 * NOTE: This implementation does NOT do ProgId lookups.
 * @param lpsz		[in] CLSID string.
 * @param pclsid	[out] CLSID
 * @return S_OK on success; E_FAIL on error.
 */
HRESULT WINAPI CLSIDFromStringA(_In_ LPCSTR lpsz, _Out_ LPCLSID pclsid);
#define CLSIDFromString(lpsz, pclsid) CLSIDFromStringA((lpsz), (pclsid))

#ifdef __cplusplus
}
#endif

#endif /* !UNICODE */

#endif /* __ROMPROPERTIES_LIBWIN32COMMON_GUID_FN_H__ */
