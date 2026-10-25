/***************************************************************************
 * ROM Properties Page shell extension. (librptext)                        *
 * wchar.hpp: wchar_t text conversion macros                               *
 * Generally only used on Windows.                                         *
 *                                                                         *
 * Copyright (c) 2009-2024 by David Korth.                                 *
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

// C includes
#include "tcharx.h"

// C++ STL classes
#include "rp-string_view.hpp"

#ifdef _WIN32

// Make sure conversion.hpp was included.
#include "librptext/conversion.hpp"
#include "common.h"

#ifndef RP_WIS16
#  error Cannot use librptext/wchar.hpp if sizeof(wchar_t) != 2
#endif /* RP_WIS16 */

/** wchar_t (Unicode) **/

/**
 * Convert UTF-8 to UTF-16.
 * @param str UTF-8 string
 * @return UTF-16 string
 */
static inline std::u16string U82U16_str(std::string_view str)
{
	return LibRpText::utf8_to_utf16(str.data(), static_cast<int>(str.size()));
}

#define U82W(str) (reinterpret_cast<const wchar_t*>(U82U16_str(str).c_str()))

// FIXME: Better U82W_str().
#define U82W_str(str) U82W(str)

/**
 * Convert UTF-16 to UTF-8.
 * @param wcs UTF-16 string
 * @return UTF-8 string
 */
static inline std::string W2U8_str(std::wstring_view wcs)
{
	return LibRpText::utf16_to_utf8(reinterpret_cast<const char16_t*>(wcs.data()), static_cast<int>(wcs.size()));
}

#define W2U8(wcs) (W2U8_str(wcs).c_str())

/** char (ANSI) **/

/**
 * Convert UTF-8 to ANSI.
 * @param str UTF-8 string
 * @return ANSI string
 */
static inline std::string U82A_str(std::string_view str)
{
	return LibRpText::utf8_to_ansi(str.data(), static_cast<int>(str.size()));
}

#define U82A(str) (U82A_str(str).c_str())

/**
 * Convert ANSI to UTF-8.
 * @param str ANSI string
 * @return UTF-8 string
 */
static inline std::string A2U8_str(std::string_view str)
{
	return LibRpText::ansi_to_utf8(str.data(), static_cast<int>(str.size()));
}

#define A2U8(str) (A2U8_str(str).c_str())

/** UTF-16 to ANSI and vice-versa **/

/**
 * Convert ANSI to UTF-16.
 * @param str ANSI string
 * @return UTF-16 string
 */
static inline std::u16string A2U16_str(std::string_view str)
{
	return LibRpText::cpN_to_utf16(CP_ACP, str.data(), static_cast<int>(str.size()));
}

#define A2W(str) (reinterpret_cast<const wchar_t*>(A2U16_str(str).c_str()))

/**
 * Convert UTF-16 to ANSI.
 * @param wcs UTF-16 string
 * @return ANSI string
 */
static inline std::string W2A_str(std::wstring_view wcs)
{
	return LibRpText::utf16_to_cpN(
		CP_ACP, reinterpret_cast<const char16_t*>(wcs.data()), static_cast<int>(wcs.size()));
}

#define W2A(wcs) (W2A_str(wcs).c_str())

/** TCHAR **/

#ifdef UNICODE

#define U82T_str(str) U82W_str(str)
#define T2U8_str(wcs) W2U8_str(wcs)

#define U82T(str) U82W(str)
#define T2U8(wcs) W2U8(wcs)

#else /* !UNICODE */

#define U82T_str(str) U82A_str(str)
#define T2U8_str(wcs) A2U8_str(wcs)

#define U82T(str) U82A(str)
#define T2U8(wcs) A2U8(wcs)

#endif /* UNICODE */

#else /* !_WIN32 */

// Compatibility functions for U82T() / T2U8() on non-Windows systems.

static inline std::tstring U82T_str(const char *str)
{
	return std::tstring(str);
}
static inline std::tstring U82T_str(const std::string &str)
{
	return str;
}
static inline std::string T2U8_str(const TCHAR *tcs)
{
	return std::string(tcs);
}
static inline std::string T2U8_str(const std::tstring &tcs)
{
	return tcs;
}

static inline const TCHAR *U82T(const char *str)
{
	return str;
}
/*static inline const TCHAR *U82T(const std::string &str)
{
	return str.c_str();
}*/
static inline const char *T2U8(const TCHAR *tcs)
{
	return tcs;
}
/*static inline const char *T2U8(const std::tstring &tcs)
{
	return tcs.c_str();
}*/

#define U82T(str) (U82T_str(str).c_str())
#define T2U8(str) (T2U8_str(str).c_str())

#endif /* _WIN32 */

// Helper macro for using C_() with U82T().
// NOTE: Must include i18n.hpp before using this macro!
#define T_(msgid)				U82T(_(msgid))
#define TC_(msgctxt, msgid)			U82T(C_(msgctxt, msgid))
#define TN_(msgid1, msgid2, n)			U82T(N_(msgid1, msgid2, n))
#define TNC_(msgctxt, msgid1, msgid2, n)	U82T(NC_(msgctxt, msgid1, msgid2, n))
#define tpgettext_expr(msgctxt, msgid)				U82T(pgettext_expr(msgctxt, msgid))
#define tdpgettext_expr(domain, msgctxt, msgid)			U82T(dpgettext_expr(domain, msgctxt, msgid))
#define tnpgettext_expr(msgctxt, msgid1, msgid2, n)		U82T(npgettext_expr(msgctxt, msgid1, msgid2, n))
#define tdnpgettext_expr(domain, msgctxt, msgid1, msgid2, n)	U82T(dnpgettext_expr(domain, msgctxt, msgid1, msgid2, n))
