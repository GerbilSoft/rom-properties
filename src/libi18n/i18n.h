/***************************************************************************
 * ROM Properties Page shell extension. (libi18n)                          *
 * i18n.h: Internationalization support code.                              *
 *                                                                         *
 * Copyright (c) 2017-2018 by David Korth.                                 *
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

#ifndef __ROMPROPERTIES_LIBI18N_H__
#define __ROMPROPERTIES_LIBI18N_H__

#include "libi18n/config.libi18n.h"

#define RP_I18N_DOMAIN "rom-properties"
#define DEFAULT_TEXT_DOMAIN RP_I18N_DOMAIN

/**
 * i18n macros for gettext().
 * All strings should be compile-time constants.
 */
#ifdef HAVE_GETTEXT
# include <locale.h>
# include "gettext.h"
# define _(msgid)				dgettext(RP_I18N_DOMAIN, msgid)
# define C_(msgctxt, msgid)			dpgettext(RP_I18N_DOMAIN, msgctxt, msgid)
# define N_(msgid1, msgid2, n)			dngettext(RP_I18N_DOMAIN, msgid1, msgid2, n)
# define NC_(msgctxt, msgid1, msgid2, n)	dnpgettext(RP_I18N_DOMAIN, msgctxt, msgid1, msgid2, n)
#else
# define _(msgid)				(msgid)
# define C_(msgctxt, msgid)			(msgid)
# define N_(msgid1, msgid2, n)			((n) == 1 ? (msgid1) : (msgid2))
# define NC_(msgctxt, msgid1, msgid2, n)	((n) == 1 ? (msgid1) : (msgid2))
# define dpgettext_expr(domain, msgctxt, msgid)			(msgid)
# define dnpgettext_expr(domain, msgctxt, msgid1, msgid2, n)	((n) == 1 ? (msgid1) : (msgid2))
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
# define printf_p(fmt, ...)			_printf_p((fmt), __VA_ARGS__)
# define fprintf_p(fp, fmt, ...)		_fprintf_p((fp), (fmt), __VA_ARGS__)
# define snprintf_p(buf, sz, fmt, ...)		_sprintf_p((buf), (sz), (fmt), __VA_ARGS__)
#else
# define printf_p(fmt, ...)			printf((fmt), ##__VA_ARGS__)
# define fprintf_p(fp, fmt, ...)		fprintf((fp), (fmt), ##__VA_ARGS__)
# define snprintf_p(buf, sz, fmt, ...)		snprintf((buf), (sz), (fmt), ##__VA_ARGS__)
#endif

#endif /* __ROMPROPERTIES_LIBI18N_H__ */
