/***************************************************************************
 * ROM Properties Page shell extension. (librptext)                        *
 * wchar.hpp: wchar_t text conversion macros                               *
 * Generally only used on Windows.                                         *
 *                                                                         *
 * Copyright (c) 2009-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

/**
 * NOTE: These are defined outside of the LibRpText
 * namespace because macros are needed for the UTF-8
 * versions.
 *
 * NOTE 2: The UTF-8 versions return c_str() from
 * temporary strings. Therefore, you *must* assign
 * the result to an std::wstring or std::string if
 * storing it, since a wchar_t* or char* will
 * result in a dangling pointer.
 */

// Make sure conversion.hpp was included.
#include "librptext/conversion.hpp"
#include "common.h"

#ifdef _WIN32
#  include "tcharx.h"
#endif /* _WIN32 */

#ifndef RP_WIS16
#  error Cannot use librptext/wchar.hpp if sizeof(wchar_t) != 2
#endif /* RP_WIS16 */

/** wchar_t (Unicode) **/

/**
 * Convert UTF-8 const char* to UTF-16 const wchar_t*.
 * @param str UTF-8 const char*
 * @return UTF-16 const wchar_t*
 */
#define U82W_c(str) \
	(reinterpret_cast<const wchar_t*>( \
		LibRpText::utf8_to_utf16(str, -1).c_str()))

/**
 * Convert UTF-8 std::string to UTF-16 const wchar_t*.
 * @param str UTF-8 std::string
 * @return UTF-16 const wchar_t*
 */
#define U82W_s(str) \
	(reinterpret_cast<const wchar_t*>( \
		LibRpText::utf8_to_utf16(str).c_str()))

/**
 * Convert UTF-16 const wchar_t* to UTF-8 std::string.
 * @param wcs UTF-16 const wchar_t*
 * @param len Length. (If -1, assuming this is a C string.)
 * @return UTF-8 std::string.
 */
static inline std::string W2U8(const wchar_t *wcs, int len = -1)
{
	return LibRpText::utf16_to_utf8(
		reinterpret_cast<const char16_t*>(wcs), len);
}

/**
 * Convert UTF-16 std::wstring to UTF-8 std::string.
 * @param wcs UTF-16 std::wstring
 * @return UTF-8 std::string
 */
static inline std::string W2U8(const std::wstring &wcs)
{
	return LibRpText::utf16_to_utf8(
		reinterpret_cast<const char16_t*>(
			wcs.data()), static_cast<int>(wcs.size()));
}

/** char (ANSI) **/

/**
 * Convert UTF-8 const char* to ANSI const char*.
 * @param str UTF-8 const char*
 * @return ANSI const char*
 */
#define U82A_c(str) \
	(LibRpText::utf8_to_ansi(str, -1).c_str())

/**
 * Convert UTF-8 std::string to ANSI const char*.
 * @param str UTF-8 std::string
 * @return ANSI const char*
 */
#define U82A_s(str) \
	(LibRpText::utf8_to_ansi(str).c_str())

/**
 * Convert ANSI const char* to UTF-8 std::string.
 * @param str ANSI const char*
 * @param len Length. (If -1, assuming this is a C string.)
 * @return UTF-8 std::string.
 */
static inline std::string A2U8(const char *str, int len = -1)
{
	return LibRpText::ansi_to_utf8(str, len);
}

/**
 * Convert ANSI std::string to UTF-8 std::string.
 * @param str ANSI std::string
 * @return UTF-8 std::string
 */
static inline std::string A2U8(const std::string &str)
{
	return LibRpText::ansi_to_utf8(
		str.data(), static_cast<int>(str.size()));
}

/** UTF-16 to ANSI and vice-versa **/

/**
 * Convert ANSI const char* to UTF-16 const wchar_t*.
 * @param str ANSI const char*
 * @return UTF-16 const wchar_t*
 */
#define A2W_c(str) \
	(reinterpret_cast<const wchar_t*>( \
		LibRpText::cpN_to_utf16(CP_ACP, str, -1).c_str()))

/**
 * Convert ANSI std::string to UTF-16 const wchar_t*.
 * @param str ANSI std::string
 * @return UTF-16 const wchar_t*
 */
#define A2W_s(str) \
	(reinterpret_cast<const wchar_t*>( \
		LibRpText::cpN_to_utf16( \
			CP_ACP, str.data(), static_cast<int>(str.size())).c_str()))

/**
 * Convert UTF-16 const wchar_t* to ANSI std::string.
 * @param wcs UTF-16 const wchar_t*
 * @param len Length. (If -1, assuming this is a C string.)
 * @return ANSI std::string.
 */
static inline std::string W2A(const wchar_t *wcs, int len = -1)
{
	return LibRpText::utf16_to_cpN(
		CP_ACP, reinterpret_cast<const char16_t*>(wcs), len);
}

/**
 * Convert UTF-16 std::wstring to ANSI std::string.
 * @param wcs UTF-16 std::wstring
 * @return ANSI std::string
 */
static inline std::string W2A(const std::wstring &wcs)
{
	return LibRpText::utf16_to_cpN(
		CP_ACP, reinterpret_cast<const char16_t*>(
			wcs.data()), static_cast<int>(wcs.size()));
}

/** TCHAR **/

#ifdef _WIN32
#ifdef UNICODE

#define U82T_c(tcs) U82W_c(tcs)
#define U82T_s(tcs) U82W_s(tcs)

/**
 * Convert const TCHAR* to UTF-8 std::string.
 * @param tcs const TCHAR*
 * @param len Length. (If -1, assuming this is a C string.)
 * @return UTF-8 std::string.
 */
static inline std::string T2U8(const TCHAR *tcs, int len = -1)
{
	return W2U8(tcs, len);
}

/**
 * Convert TCHAR std::string to UTF-8 std::string.
 * @param tcs TCHAR std::string
 * @return UTF-8 std::string
 */
static inline std::string T2U8(const std::tstring &tcs)
{
	return W2U8(tcs);
}

#else /* !UNICODE */

#define U82T_c(tcs) U82A_c(tcs)
#define U82T_s(tcs) U82A_s(tcs)

/**
 * Convert const TCHAR* to UTF-8 std::string.
 * @param str const TCHAR*
 * @param len Length. (If -1, assuming this is a C string.)
 * @return UTF-8 std::string.
 */
static inline std::string T2U8(const TCHAR *tcs, int len = -1)
{
	return A2U8(tcs, len);
}

/**
 * Convert TCHAR std::tstring to UTF-8 std::string.
 * @param str TCHAR std::string
 * @return UTF-8 std::string
 */
static inline std::string T2U8(const std::tstring &tcs)
{
	return A2U8(tcs);
}

#endif /* UNICODE */
#endif /* _WIN32 */
