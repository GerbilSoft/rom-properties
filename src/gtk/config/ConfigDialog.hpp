/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ common)                      *
 * ConfigDialog.hpp: Configuration dialog.                                 *
 *                                                                         *
 * Copyright (c) 2017-2022 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_GTK_CONFIG_CONFIGDIALOG_HPP__
#define __ROMPROPERTIES_GTK_CONFIG_CONFIGDIALOG_HPP__

#include <gtk/gtk.h>

G_BEGIN_DECLS

typedef struct _ConfigDialogClass	ConfigDialogClass;
typedef struct _ConfigDialog		ConfigDialog;

#define TYPE_CONFIG_DIALOG            (config_dialog_get_type())
#define CONFIG_DIALOG(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), TYPE_CONFIG_DIALOG, ConfigDialog))
#define CONFIG_DIALOG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass),  TYPE_CONFIG_DIALOG, ConfigDialogClass))
#define IS_CONFIG_DIALOG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), TYPE_CONFIG_DIALOG))
#define IS_CONFIG_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass),  TYPE_CONFIG_DIALOG))
#define CONFIG_DIALOG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj),  TYPE_CONFIG_DIALOG, ConfigDialogClass))

/* these two functions are implemented automatically by the G_DEFINE_TYPE macro */
GType		config_dialog_get_type		(void) G_GNUC_CONST G_GNUC_INTERNAL;
void		config_dialog_register_type	(GtkWidget *widget) G_GNUC_INTERNAL;

GtkWidget	*config_dialog_new		(void) G_GNUC_INTERNAL G_GNUC_MALLOC;

G_END_DECLS

#endif /* __ROMPROPERTIES_GTK_CONFIG_CONFIGDIALOG_HPP__ */
