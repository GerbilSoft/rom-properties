/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ common)                      *
 * AboutTab.hpp: About tab for rp-config.                                  *
 *                                                                         *
 * Copyright (c) 2017-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "gtk-compat.h"

G_BEGIN_DECLS

#define RP_TYPE_ABOUT_TAB (rp_about_tab_get_type())

#if GTK_CHECK_VERSION(3, 0, 0)
#  define _RpAboutTab_super	GtkBox
#  define _RpAboutTab_superClass	GtkBoxClass
#else /* !GTK_CHECK_VERSION(3, 0, 0) */
#  define _RpAboutTab_super	GtkVBox
#  define _RpAboutTab_superClass	GtkVBoxClass
#endif /* GTK_CHECK_VERSION(3, 0, 0) */

G_DECLARE_FINAL_TYPE(RpAboutTab, rp_about_tab, RP, ABOUT_TAB, _RpAboutTab_super)

GtkWidget	*rp_about_tab_new		(void) G_GNUC_INTERNAL G_GNUC_MALLOC;

G_END_DECLS
