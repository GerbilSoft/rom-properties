/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ common)                      *
 * KeyManagerTab.hpp: Key Manager tab for rp-config.                          *
 *                                                                         *
 * Copyright (c) 2017-2022 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_GTK_CONFIG_KEY_MANAGERTAB_HPP__
#define __ROMPROPERTIES_GTK_CONFIG_KEY_MANAGERTAB_HPP__

#include <gtk/gtk.h>

G_BEGIN_DECLS

typedef struct _KeyManagerTabClass		KeyManagerTabClass;
typedef struct _KeyManagerTab		KeyManagerTab;

#define TYPE_KEY_MANAGER_TAB            (key_manager_tab_get_type())
#define KEY_MANAGER_TAB(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), TYPE_KEY_MANAGER_TAB, KeyManagerTab))
#define KEY_MANAGER_TAB_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass),  TYPE_KEY_MANAGER_TAB, KeyManagerTabClass))
#define IS_KEY_MANAGER_TAB(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), TYPE_KEY_MANAGER_TAB))
#define IS_KEY_MANAGER_TAB_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass),  TYPE_KEY_MANAGER_TAB))
#define KEY_MANAGER_TAB_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj),  TYPE_KEY_MANAGER_TAB, KeyManagerTabClass))

/* these two functions are implemented automatically by the G_DEFINE_TYPE macro */
GType		key_manager_tab_get_type	(void) G_GNUC_CONST G_GNUC_INTERNAL;
void		key_manager_tab_register_type	(GtkWidget *widget) G_GNUC_INTERNAL;

GtkWidget	*key_manager_tab_new		(void) G_GNUC_INTERNAL G_GNUC_MALLOC;

G_END_DECLS

#endif /* __ROMPROPERTIES_GTK_CONFIG_KEY_MANAGERTAB_HPP__ */
