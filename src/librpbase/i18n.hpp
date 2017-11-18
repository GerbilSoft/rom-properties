#ifndef __ROMPROPERTIES_LIBRPBASE_I18N_HPP__
#define __ROMPROPERTIES_LIBRPBASE_I18N_HPP__

#include "librpbase/config.librpbase.h"

#define RP_I18N_DOMAIN "rom-properties"
#define DEFAULT_TEXT_DOMAIN RP_I18N_DOMAIN

/**
 * i18n macros for gettext().
 * All strings should be compile-time constants.
 */
#ifdef HAVE_GETTEXT
# define ENABLE_NLS 1
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
 * @param dirname Directory name.
 * @return 0 on success; non-zero on error.
 */
int rp_i18n_init(const char *dirname);

#ifdef __cplusplus
}
#endif
#endif /* HAVE_GETTEXT */

// Positional printf().
// TODO: cmake verification?
#ifdef _MSC_VER
# define printf_i18n(fmt, ...)			_printf_p((fmt), __VA_ARGS__)
# define fprintf_i18n(fp, fmt, ...)		_fprintf_p((fp), (fmt), __VA_ARGS__)
# define snprintf_i18n(buf, sz, fmt, ...)	_sprintf_p((buf), (sz), (fmt), __VA_ARGS__)
#else
# define printf_i18n(fmt, ...)			printf((fmt), ##__VA_ARGS__)
# define fprintf_i18n(fp, fmt, ...)		fprintf((fp), (fmt), ##__VA_ARGS__)
# define snprintf_i18n(buf, sz, fmt, ...)	snprintf((buf), (sz), (fmt), ##__VA_ARGS__)
#endif

#endif /* __ROMPROPERTIES_LIBRPBASE_I18N_HPP__ */
