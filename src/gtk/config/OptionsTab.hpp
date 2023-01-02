/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ common)                      *
 * OptionsTab.hpp: Options tab for rp-config.                              *
 *                                                                         *
 * Copyright (c) 2017-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "gtk-compat.h"

G_BEGIN_DECLS

#define RP_TYPE_OPTIONS_TAB (rp_options_tab_get_type())
#if GTK_CHECK_VERSION(3,0,0)
G_DECLARE_FINAL_TYPE(RpOptionsTab, rp_options_tab, RP, OPTIONS_TAB, GtkBox)
#else /* !GTK_CHECK_VERSION(3,0,0) */
G_DECLARE_FINAL_TYPE(RpOptionsTab, rp_options_tab, RP, OPTIONS_TAB, GtkVBox)
#endif /* GTK_CHECK_VERSION(3,0,0) */

GtkWidget	*rp_options_tab_new		(void) G_GNUC_INTERNAL G_GNUC_MALLOC;

G_END_DECLS
