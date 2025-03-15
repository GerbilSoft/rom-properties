/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ common)                      *
 * KeyStoreItem.h: KeyManagerTab item (for GTK4 GtkTreeListModel)          *
 *                                                                         *
 * Copyright (c) 2017-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "gtk-compat.h"

#if !GTK_CHECK_VERSION(4, 0, 0)
#  error GtkTreeListModel requires GTK4 or later
#endif /* !GTK_CHECK_VERSION(4, 0, 0) */

G_BEGIN_DECLS

#define RP_TYPE_KEY_STORE_ITEM (rp_key_store_item_get_type())
G_DECLARE_FINAL_TYPE(RpKeyStoreItem, rp_key_store_item, RP, KEY_STORE_ITEM, GObject)

RpKeyStoreItem *rp_key_store_item_new		(const char *name, const char *value, uint8_t status, int flat_idx, gboolean is_section) G_GNUC_MALLOC;
RpKeyStoreItem *rp_key_store_item_new_key	(const char *name, const char *value, uint8_t status, int flat_idx) G_GNUC_MALLOC;
RpKeyStoreItem *rp_key_store_item_new_section	(const char *name, const char *value, int sect_idx) G_GNUC_MALLOC;

void		rp_key_store_item_set_name	(RpKeyStoreItem *item, const char *name);
const char*	rp_key_store_item_get_name	(RpKeyStoreItem *item);

void		rp_key_store_item_set_value	(RpKeyStoreItem *item, const char *value);
const char*	rp_key_store_item_get_value	(RpKeyStoreItem *item);

void		rp_key_store_item_set_status	(RpKeyStoreItem *item, uint8_t status);
uint8_t 	rp_key_store_item_get_status	(RpKeyStoreItem *item);

void		rp_key_store_item_set_flat_idx	(RpKeyStoreItem *item, int flat_idx);
int		rp_key_store_item_get_flat_idx	(RpKeyStoreItem *item);

void		rp_key_store_item_set_is_section(RpKeyStoreItem *item, gboolean is_section);
gboolean	rp_key_store_item_get_is_section(RpKeyStoreItem *item);

G_END_DECLS
