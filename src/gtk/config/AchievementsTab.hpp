/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ common)                      *
 * AchievementsTab.hpp: Achievements tab for rp-config.                    *
 *                                                                         *
 * Copyright (c) 2017-2022 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_GTK_CONFIG_ACHIEVEMENTSTAB_HPP__
#define __ROMPROPERTIES_GTK_CONFIG_ACHIEVEMENTSTAB_HPP__

#include <gtk/gtk.h>

G_BEGIN_DECLS

typedef struct _AchievementsTabClass		AchievementsTabClass;
typedef struct _AchievementsTab		AchievementsTab;

#define TYPE_ACHIEVEMENTS_TAB            (achievements_tab_get_type())
#define ACHIEVEMENTS_TAB(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), TYPE_ACHIEVEMENTS_TAB, AchievementsTab))
#define ACHIEVEMENTS_TAB_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass),  TYPE_ACHIEVEMENTS_TAB, AchievementsTabClass))
#define IS_ACHIEVEMENTS_TAB(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), TYPE_ACHIEVEMENTS_TAB))
#define IS_ACHIEVEMENTS_TAB_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass),  TYPE_ACHIEVEMENTS_TAB))
#define ACHIEVEMENTS_TAB_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj),  TYPE_ACHIEVEMENTS_TAB, AchievementsTabClass))

/* these two functions are implemented automatically by the G_DEFINE_TYPE macro */
GType		achievements_tab_get_type		(void) G_GNUC_CONST G_GNUC_INTERNAL;
void		achievements_tab_register_type		(GtkWidget *widget) G_GNUC_INTERNAL;

GtkWidget	*achievements_tab_new			(void) G_GNUC_INTERNAL G_GNUC_MALLOC;

G_END_DECLS

#endif /* __ROMPROPERTIES_GTK_CONFIG_ACHIEVEMENTSTAB_HPP__ */
