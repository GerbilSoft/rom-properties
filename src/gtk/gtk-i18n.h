/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ common)                      *
 * ConfigDialog.hpp: Configuration dialog.                                 *
 *                                                                         *
 * Copyright (c) 2017-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include <gtk/gtk.h>
#include "libi18n/i18n.h"

#if defined(ENABLE_NLS) && ENABLE_NLS

// i18n is enabled. Use GTK's built-in localization.
#if GTK_CHECK_VERSION(5,0,0)
#  error No GTK5 support
#elif GTK_CHECK_VERSION(4,0,0)
#  define GTK_I18N_DOMAIN "gtk40"
#elif GTK_CHECK_VERSION(3,0,0)
#  define GTK_I18N_DOMAIN "gtk30"
#elif GTK_CHECK_VERSION(2,0,0)
#  define GTK_I18N_DOMAIN "gtk20"
#else
#  error No GTK1 support
#endif

#if GTK_CHECK_VERSION(3,0,0)
// GTK3 and later: No context
#define GTK_I18N_STR_CANCEL	dgettext(GTK_I18N_DOMAIN, "_Cancel")
#define GTK_I18N_STR_APPLY	dgettext(GTK_I18N_DOMAIN, "_Apply")
#define GTK_I18N_STR_OK		dgettext(GTK_I18N_DOMAIN, "_OK")
#define GTK_I18N_STR_SAVE	dgettext(GTK_I18N_DOMAIN, "_Save")
#define GTK_I18N_STR_OPEN	dgettext(GTK_I18N_DOMAIN, "_Open")
#else /* !GTK_CHECK_VERSION(3,0,0) */
// GTK2: Context is "Stock label"
#define GTK_I18N_STR_CANCEL	dpgettext(GTK_I18N_DOMAIN, "Stock label", "_Cancel")
#define GTK_I18N_STR_APPLY	dpgettext(GTK_I18N_DOMAIN, "Stock label", "_Apply")
#define GTK_I18N_STR_OK		dpgettext(GTK_I18N_DOMAIN, "Stock label", "_OK")
#define GTK_I18N_STR_SAVE	dpgettext(GTK_I18N_DOMAIN, "Stock label", "_Save")
#define GTK_I18N_STR_OPEN	dpgettext(GTK_I18N_DOMAIN, "Stock label", "_Open")
#endif /* GTK_CHECK_VERSION(3,0,0) */

#else /* !ENABLE_NLS */

// i18n is disabled. Use English strings.
#define GTK_I18N_STR_CANCEL	"_Cancel"
#define GTK_I18N_STR_APPLY	"_Apply"
#define GTK_I18N_STR_OK		"_OK"
#define GTK_I18N_STR_SAVE	"_Save"
#define GTK_I18N_STR_OPEN	"_Open"

#endif /* #if defined(ENABLE_NLS) && ENABLE_NLS */
