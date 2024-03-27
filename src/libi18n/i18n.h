/***************************************************************************
 * ROM Properties Page shell extension. (libi18n)                          *
 * i18n.h: Internationalization support code.                              *
 *                                                                         *
 * Copyright (c) 2017-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "libi18n/config.libi18n.h"
#include "dll-macros.h"	// for RP_LIBROMDATA_PUBLIC

#define RP_I18N_DOMAIN "rom-properties"
#define DEFAULT_TEXT_DOMAIN RP_I18N_DOMAIN

#ifdef _WIN32
#  include <tchar.h>
#  define LIBGNUINTL_DLL _T("libgnuintl-8.dll")
#endif /* _WIN32 */

/**
 * i18n macros for gettext().
 * All strings should be compile-time constants.
 */
#ifdef HAVE_GETTEXT
#  include "gettext.h"
#  define _(msgid)				dgettext(RP_I18N_DOMAIN, msgid)
#  define C_(msgctxt, msgid)			dpgettext(RP_I18N_DOMAIN, msgctxt, msgid)
#  define N_(msgid1, msgid2, n)			dngettext(RP_I18N_DOMAIN, msgid1, msgid2, n)
#  define NC_(msgctxt, msgid1, msgid2, n)	dnpgettext(RP_I18N_DOMAIN, msgctxt, msgid1, msgid2, n)
#else
#  define _(msgid)				(msgid)
#  define C_(msgctxt, msgid)			(msgid)
#  define N_(msgid1, msgid2, n)			((n) == 1 ? (msgid1) : (msgid2))
#  define NC_(msgctxt, msgid1, msgid2, n)	((n) == 1 ? (msgid1) : (msgid2))
#  define pgettext_expr(msgctxt, msgid)				(msgid)
#  define dpgettext_expr(domain, msgctxt, msgid)		(msgid)
#  define npgettext_expr(msgctxt, msgid1, msgid2, n)		((n) == 1 ? (msgid1) : (msgid2))
#  define dnpgettext_expr(domain, msgctxt, msgid1, msgid2, n)	((n) == 1 ? (msgid1) : (msgid2))
#endif

/* No-op formats that are translated later. */
#define NOP_(msgid)		(msgid)
#define NOP_C_(msgctxt, msgid)	(msgid)

#ifdef HAVE_GETTEXT
#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize the internationalization subsystem.
 * @return 0 on success; non-zero on error.
 */
RP_LIBROMDATA_PUBLIC
int rp_i18n_init(void);

#ifdef __cplusplus
}
#endif

#else
/**
 * Dummy macro for rp_i18n_init() that does nothing.
 */
#define rp_i18n_init() do { } while (0)
#endif /* HAVE_GETTEXT */

// Positional printf().
// TODO: cmake verification?
#ifdef _MSC_VER
#  define printf_p(fmt, ...)			_printf_p((fmt), __VA_ARGS__)
#  define fprintf_p(fp, fmt, ...)		_fprintf_p((fp), (fmt), __VA_ARGS__)
#  define snprintf_p(buf, sz, fmt, ...)		_sprintf_p((buf), (sz), (fmt), __VA_ARGS__)
#else
#  define printf_p(fmt, ...)			printf((fmt), ##__VA_ARGS__)
#  define fprintf_p(fp, fmt, ...)		fprintf((fp), (fmt), ##__VA_ARGS__)
#  define snprintf_p(buf, sz, fmt, ...)		snprintf((buf), (sz), (fmt), ##__VA_ARGS__)
#endif
