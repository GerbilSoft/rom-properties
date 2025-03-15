/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ common)                      *
 * AchievementsTab.hpp: Achievements tab for rp-config.                    *
 *                                                                         *
 * Copyright (c) 2017-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "gtk-compat.h"

G_BEGIN_DECLS

#define RP_TYPE_ACHIEVEMENTS_TAB (rp_achievements_tab_get_type())

#if GTK_CHECK_VERSION(3, 0, 0)
#  define _RpAchievementsTab_super	GtkBox
#  define _RpAchievementsTab_superClass	GtkBoxClass
#else /* !GTK_CHECK_VERSION(3, 0, 0) */
#  define _RpAchievementsTab_super	GtkVBox
#  define _RpAchievementsTab_superClass	GtkVBoxClass
#endif /* GTK_CHECK_VERSION(3, 0, 0) */

G_DECLARE_FINAL_TYPE(RpAchievementsTab, rp_achievements_tab, RP, ACHIEVEMENTS_TAB, _RpAchievementsTab_super)

GtkWidget	*rp_achievements_tab_new		(void) G_GNUC_INTERNAL G_GNUC_MALLOC;

G_END_DECLS
