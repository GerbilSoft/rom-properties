/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ common)                      *
 * OptionsTab.hpp: Options tab for rp-config.                              *
 *                                                                         *
 * Copyright (c) 2017-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "gtk-compat.h"

G_BEGIN_DECLS

#define RP_TYPE_OPTIONS_TAB (rp_options_tab_get_type())

#if GTK_CHECK_VERSION(3, 0, 0)
#  define _RpOptionsTab_super		GtkBox
#  define _RpOptionsTab_superClass	GtkBoxClass
#else /* !GTK_CHECK_VERSION(3, 0, 0) */
#  define _RpOptionsTab_super		GtkVBox
#  define _RpOptionsTab_superClass	GtkVBoxClass
#endif /* GTK_CHECK_VERSION(3, 0, 0) */

G_DECLARE_FINAL_TYPE(RpOptionsTab, rp_options_tab, RP, OPTIONS_TAB, _RpOptionsTab_super)

GtkWidget	*rp_options_tab_new		(void) G_GNUC_INTERNAL G_GNUC_MALLOC;

G_END_DECLS
