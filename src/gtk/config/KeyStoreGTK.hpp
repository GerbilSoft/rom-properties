/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ common)                      *
 * KeyStoreQt.hpp: Key store object for GTK.                               *
 *                                                                         *
 * Copyright (c) 2017-2022 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_GTK_CONFIG_KEYSTOREGTK_HPP__
#define __ROMPROPERTIES_GTK_CONFIG_KEYSTOREGTK_HPP__

#include <gtk/gtk.h>

G_BEGIN_DECLS

typedef struct _KeyStoreGTKClass	KeyStoreGTKClass;
typedef struct _KeyStoreGTK		KeyStoreGTK;

#define TYPE_KEY_STORE_GTK            (key_store_gtk_get_type())
#define KEY_STORE_GTK(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), TYPE_KEY_STORE_GTK, KeyStoreGTK))
#define KEY_STORE_GTK_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass),  TYPE_KEY_STORE_GTK, KeyStoreGTKClass))
#define IS_KEY_STORE_GTK(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), TYPE_KEY_STORE_GTK))
#define IS_KEY_STORE_GTK_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass),  TYPE_KEY_STORE_GTK))
#define KEY_STORE_GTK_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj),  TYPE_KEY_STORE_GTK, KeyStoreGTKClass))

/* these two functions are implemented automatically by the G_DEFINE_TYPE macro */
GType		key_store_gtk_get_type			(void) G_GNUC_CONST G_GNUC_INTERNAL;
void		key_store_gtk_register_type		(GtkWidget *widget) G_GNUC_INTERNAL;

KeyStoreGTK	*key_store_gtk_new			(void) G_GNUC_INTERNAL G_GNUC_MALLOC;

int		key_store_gtk_get_total_key_count	(KeyStoreGTK *keyStore);
gboolean	key_store_gtk_has_changed		(KeyStoreGTK *keyStore);

G_END_DECLS

#ifdef __cplusplus
#  include "libromdata/crypto/KeyStoreUI.hpp"
/**
 * Get the underlying KeyStoreUI object.
 * @param keyStore KeyStoreGTK
 * @return KeyStoreUI
 */
LibRomData::KeyStoreUI	*key_store_gtk_get_key_store_ui(KeyStoreGTK *keyStore);
#endif /* __cplusplus */

#endif /* __ROMPROPERTIES_GTK_CONFIG_KEYSTOREGTK_HPP__ */
