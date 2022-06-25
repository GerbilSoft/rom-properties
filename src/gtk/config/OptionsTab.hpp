/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ common)                      *
 * OptionsTab.hpp: Options tab for rp-config.                              *
 *                                                                         *
 * Copyright (c) 2017-2022 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_GTK_CONFIG_OPTIONSTAB_HPP__
#define __ROMPROPERTIES_GTK_CONFIG_OPTIONSTAB_HPP__

#include <gtk/gtk.h>

G_BEGIN_DECLS

typedef struct _OptionsTabClass		OptionsTabClass;
typedef struct _OptionsTab		OptionsTab;

#define TYPE_OPTIONS_TAB            (options_tab_get_type())
#define OPTIONS_TAB(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), TYPE_OPTIONS_TAB, OptionsTab))
#define OPTIONS_TAB_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass),  TYPE_OPTIONS_TAB, OptionsTabClass))
#define IS_OPTIONS_TAB(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), TYPE_OPTIONS_TAB))
#define IS_OPTIONS_TAB_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass),  TYPE_OPTIONS_TAB))
#define OPTIONS_TAB_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj),  TYPE_OPTIONS_TAB, OptionsTabClass))

/* these two functions are implemented automatically by the G_DEFINE_TYPE macro */
GType		options_tab_get_type		(void) G_GNUC_CONST G_GNUC_INTERNAL;
void		options_tab_register_type	(GtkWidget *widget) G_GNUC_INTERNAL;

GtkWidget	*options_tab_new		(void) G_GNUC_INTERNAL G_GNUC_MALLOC;

G_END_DECLS

#endif /* __ROMPROPERTIES_GTK_CONFIG_OPTIONSTAB_HPP__ */
