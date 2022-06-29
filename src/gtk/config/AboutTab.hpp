/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ common)                      *
 * AboutTab.hpp: About tab for rp-config.                                  *
 *                                                                         *
 * Copyright (c) 2017-2022 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_GTK_CONFIG_ABOUTTAB_HPP__
#define __ROMPROPERTIES_GTK_CONFIG_ABOUTTAB_HPP__

#include <gtk/gtk.h>

G_BEGIN_DECLS

typedef struct _AboutTabClass		AboutTabClass;
typedef struct _AboutTab		AboutTab;

#define TYPE_ABOUT_TAB            (about_tab_get_type())
#define ABOUT_TAB(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), TYPE_ABOUT_TAB, AboutTab))
#define ABOUT_TAB_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass),  TYPE_ABOUT_TAB, AboutTabClass))
#define IS_ABOUT_TAB(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), TYPE_ABOUT_TAB))
#define IS_ABOUT_TAB_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass),  TYPE_ABOUT_TAB))
#define ABOUT_TAB_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj),  TYPE_ABOUT_TAB, AboutTabClass))

/* these two functions are implemented automatically by the G_DEFINE_TYPE macro */
GType		about_tab_get_type		(void) G_GNUC_CONST G_GNUC_INTERNAL;
void		about_tab_register_type		(GtkWidget *widget) G_GNUC_INTERNAL;

GtkWidget	*about_tab_new			(void) G_GNUC_INTERNAL G_GNUC_MALLOC;

G_END_DECLS

#endif /* __ROMPROPERTIES_GTK_CONFIG_ABOUTTAB_HPP__ */
