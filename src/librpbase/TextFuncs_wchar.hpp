/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * TextFuncs_wchar.hpp: wchar_t text conversion macros.                    *
 * Generally only used on Windows.                                         *
 *                                                                         *
 * Copyright (c) 2009-2022 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_LIBRPBASE_TEXTFUNCS_WCHAR_HPP__
#define __ROMPROPERTIES_LIBRPBASE_TEXTFUNCS_WCHAR_HPP__

/**
 * NOTE: These are defined outside of the LibRpBase
 * namespace because macros are needed for the UTF-8
 * versions.
 *
 * NOTE 2: The UTF-8 versions return c_str() from
 * temporary strings. Therefore, you *must* assign
 * the result to an std::wstring or std::string if
 * storing it, since a wchar_t* or char* will
 * result in a dangling pointer.
 */

// Make sure TextFuncs.hpp was included.
#include "librpbase/TextFuncs.hpp"
#include "common.h"

#ifdef _WIN32
#  include <tchar.h>
#  if defined(__cplusplus) && !defined(tstring)
// NOTE: Can't use typedef due to std:: namespace.
#    ifdef _UNICODE
#      define tstring wstring
#    else /* !_UNICODE */
#      define tstring string
#    endif /* _UNICODE */
#  endif /* defined(__cplusplus) && !defined(tstring) */
#endif /* _WIN32 */

#ifndef RP_WIS16
# error Cannot use TextFuncs_wchar.hpp if sizeof(wchar_t) != 2
#endif /* RP_WIS16 */

/** wchar_t (Unicode) **/

// FIXME: U8STRFIX

/**
 * Convert UTF-8 const char* to UTF-16 const wchar_t*.
 * @param str UTF-8 const char*
 * @return UTF-16 const wchar_t*
 */
#define U82W_c(str) \
	(reinterpret_cast<const wchar_t*>( \
		LibRpBase::utf8_to_utf16(reinterpret_cast<const char8_t*>(str), -1).c_str()))

/**
 * Convert std::u8string to UTF-16 const wchar_t*.
 * @param str std::u8string
 * @return UTF-16 const wchar_t*
 */
#define U82W_s(str) \
	(reinterpret_cast<const wchar_t*>( \
		LibRpBase::utf8_to_utf16(reinterpret_cast<const char8_t*>(str.c_str()), (int)str.size()).c_str()))

/**
 * Convert UTF-16 const wchar_t* to std::u8string.
 * @param wcs UTF-16 const wchar_t*
 * @param len Length (If -1, assuming this is a C string.)
 * @return std::u8string
 */
static inline std::u8string W2U8(const wchar_t *wcs, int len = -1)
{
	return LibRpBase::utf16_to_utf8(
		reinterpret_cast<const char16_t*>(wcs), len);
}

/**
 * Convert UTF-16 std::wstring to std::u8string.
 * @param wcs UTF-16 std::wstring
 * @return std::u8string
 */
static inline std::u8string W2U8(const std::wstring &wcs)
{
	return LibRpBase::utf16_to_utf8(
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
	(LibRpBase::utf8_to_ansi(reinterpret_cast<const char8_t*>(str), -1).c_str())

/**
 * Convert std::u8string to ANSI const char*.
 * @param str std::u8string
 * @return ANSI const char*
 */
#define U82A_s(str) \
	(LibRpBase::utf8_to_ansi(reinterpret_cast<const char8_t*>(str.c_str()), (int)str.size()).c_str())

/**
 * Convert ANSI const char* to std::u8string.
 * @param str ANSI const char*
 * @param len Length (If -1, assuming this is a C string.)
 * @return std::u8string
 */
static inline std::u8string A2U8(const char *str, int len = -1)
{
	return LibRpBase::ansi_to_utf8(str, len);
}

/**
 * Convert ANSI std::string to std::u8string.
 * @param str ANSI std::string
 * @return std::u8string
 */
static inline std::u8string A2U8(const std::string &str)
{
	return LibRpBase::ansi_to_utf8(
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
		LibRpBase::cpN_to_utf16(CP_ACP, str, -1).c_str()))

/**
 * Convert ANSI std::string to UTF-16 const wchar_t*.
 * @param str ANSI std::string
 * @return UTF-16 const wchar_t*
 */
#define A2W_s(str) \
	(reinterpret_cast<const wchar_t*>( \
		LibRpBase::cpN_to_utf16( \
			CP_ACP, str.data(), static_cast<int>(str.size())).c_str()))

/**
 * Convert UTF-16 const wchar_t* to ANSI std::string.
 * @param wcs UTF-16 const wchar_t*
 * @param len Length (If -1, assuming this is a C string.)
 * @return ANSI std::string
 */
static inline std::string W2A(const wchar_t *wcs, int len = -1)
{
	return LibRpBase::utf16_to_cpN(
		CP_ACP, reinterpret_cast<const char16_t*>(wcs), len);
}

/**
 * Convert UTF-16 std::wstring to ANSI std::string.
 * @param wcs UTF-16 std::wstring
 * @return ANSI std::string
 */
static inline std::string W2A(const std::wstring &wcs)
{
	return LibRpBase::utf16_to_cpN(
		CP_ACP, reinterpret_cast<const char16_t*>(
			wcs.data()), static_cast<int>(wcs.size()));
}

/** TCHAR **/

#ifdef _WIN32
#ifdef UNICODE

#define U82T_c(tcs) U82W_c(tcs)
#define U82T_s(tcs) U82W_s(tcs)

/**
 * Convert const TCHAR* to std::u8string.
 * @param tcs const TCHAR*
 * @param len Length (If -1, assuming this is a C string.)
 * @return std::u8string
 */
static inline std::u8string T2U8(const TCHAR *tcs, int len = -1)
{
	return W2U8(tcs, len);
}

/**
 * Convert std::tstring to std::u8string.
 * @param tcs TCHAR std::string
 * @return std::u8string
 */
static inline std::u8string T2U8(const std::tstring &tcs)
{
	return W2U8(tcs);
}

#else /* !UNICODE */

#define U82T_c(tcs) U82A_c(tcs)
#define U82T_s(tcs) U82A_s(tcs)

/**
 * Convert const TCHAR* to std::u8string.
 * @param str const TCHAR*
 * @param len Length (If -1, assuming this is a C string.)
 * @return std::u8string
 */
static inline std::u8string T2U8(const TCHAR *tcs, int len = -1)
{
	return A2U8(tcs, len);
}

/**
 * Convert std::tstring to std::u8string.
 * @param str TCHAR std::string
 * @return std::u8string
 */
static inline std::u8string T2U8(const std::tstring &tcs)
{
	return A2U8(tcs);
}

#endif /* UNICODE */
#endif /* _WIN32 */

#endif /* __ROMPROPERTIES_LIBRPBASE_TEXTFUNCS_WCHAR_HPP__ */
