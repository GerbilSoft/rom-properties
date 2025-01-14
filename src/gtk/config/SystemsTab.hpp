/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ common)                      *
 * SystemsTab.hpp: Systems tab for rp-config.                              *
 *                                                                         *
 * Copyright (c) 2017-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "gtk-compat.h"

G_BEGIN_DECLS

#define RP_TYPE_SYSTEMS_TAB (rp_systems_tab_get_type())

#if GTK_CHECK_VERSION(3,0,0)
#  define _RpSystemsTab_super	GtkBox
#  define _RpSystemsTab_superClass	GtkBoxClass
#else /* !GTK_CHECK_VERSION(3,0,0) */
#  define _RpSystemsTab_super	GtkVBox
#  define _RpSystemsTab_superClass	GtkVBoxClass
#endif /* GTK_CHECK_VERSION(3,0,0) */

G_DECLARE_FINAL_TYPE(RpSystemsTab, rp_systems_tab, RP, SYSTEMS_TAB, _RpSystemsTab_super)

GtkWidget	*rp_systems_tab_new		(void) G_GNUC_INTERNAL G_GNUC_MALLOC;

G_END_DECLS
