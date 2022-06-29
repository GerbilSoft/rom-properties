/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ common)                      *
 * SystemsTab.hpp: Systems tab for rp-config.                              *
 *                                                                         *
 * Copyright (c) 2017-2022 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_GTK_CONFIG_SYSTEMSTAB_HPP__
#define __ROMPROPERTIES_GTK_CONFIG_SYSTEMSTAB_HPP__

#include <gtk/gtk.h>

G_BEGIN_DECLS

typedef struct _SystemsTabClass		SystemsTabClass;
typedef struct _SystemsTab		SystemsTab;

#define TYPE_SYSTEMS_TAB            (systems_tab_get_type())
#define SYSTEMS_TAB(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), TYPE_SYSTEMS_TAB, SystemsTab))
#define SYSTEMS_TAB_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass),  TYPE_SYSTEMS_TAB, SystemsTabClass))
#define IS_SYSTEMS_TAB(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), TYPE_SYSTEMS_TAB))
#define IS_SYSTEMS_TAB_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass),  TYPE_SYSTEMS_TAB))
#define SYSTEMS_TAB_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj),  TYPE_SYSTEMS_TAB, SystemsTabClass))

/* these two functions are implemented automatically by the G_DEFINE_TYPE macro */
GType		systems_tab_get_type		(void) G_GNUC_CONST G_GNUC_INTERNAL;
void		systems_tab_register_type	(GtkWidget *widget) G_GNUC_INTERNAL;

GtkWidget	*systems_tab_new		(void) G_GNUC_INTERNAL G_GNUC_MALLOC;

G_END_DECLS

#endif /* __ROMPROPERTIES_GTK_CONFIG_SYSTEMSTAB_HPP__ */
