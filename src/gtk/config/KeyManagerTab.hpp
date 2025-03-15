/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ common)                      *
 * KeyManagerTab.hpp: Key Manager tab for rp-config.                       *
 *                                                                         *
 * Copyright (c) 2017-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "gtk-compat.h"

G_BEGIN_DECLS

#define RP_TYPE_KEY_MANAGER_TAB (rp_key_manager_tab_get_type())

#if GTK_CHECK_VERSION(3, 0, 0)
#  define _RpKeyManagerTab_super	GtkBox
#  define _RpKeyManagerTab_superClass	GtkBoxClass
#else /* !GTK_CHECK_VERSION(3, 0, 0) */
#  define _RpKeyManagerTab_super	GtkVBox
#  define _RpKeyManagerTab_superClass	GtkVBoxClass
#endif /* GTK_CHECK_VERSION(3, 0, 0) */

G_DECLARE_FINAL_TYPE(RpKeyManagerTab, rp_key_manager_tab, RP, KEY_MANAGER_TAB, _RpKeyManagerTab_super)

GtkWidget	*rp_key_manager_tab_new			(void) G_GNUC_INTERNAL G_GNUC_MALLOC;

G_END_DECLS
