/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ common)                      *
 * KeyStoreQt.hpp: Key store object for GTK.                               *
 *                                                                         *
 * Copyright (c) 2017-2022 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_GTK_CONFIG_KEYSTOREGTK_HPP__
#define __ROMPROPERTIES_GTK_CONFIG_KEYSTOREGTK_HPP__

#include "glib-compat.h"

G_BEGIN_DECLS

#define RP_TYPE_KEY_STORE_GTK (rp_key_store_gtk_get_type())
G_DECLARE_FINAL_TYPE(RpKeyStoreGTK, rp_key_store_gtk, RP, KEY_STORE_GTK, GObject)

RpKeyStoreGTK	*rp_key_store_gtk_new			(void) G_GNUC_INTERNAL G_GNUC_MALLOC;

int		rp_key_store_gtk_get_total_key_count	(RpKeyStoreGTK *keyStore);
gboolean	rp_key_store_gtk_has_changed		(RpKeyStoreGTK *keyStore);

G_END_DECLS

#ifdef __cplusplus
#  include "libromdata/crypto/KeyStoreUI.hpp"
/**
 * Get the underlying KeyStoreUI object.
 * @param keyStore KeyStoreGTK
 * @return KeyStoreUI
 */
LibRomData::KeyStoreUI	*rp_key_store_gtk_get_key_store_ui(RpKeyStoreGTK *keyStore);
#endif /* __cplusplus */

#endif /* __ROMPROPERTIES_GTK_CONFIG_KEYSTOREGTK_HPP__ */
