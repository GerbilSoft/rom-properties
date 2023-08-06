/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ common)                      *
 * KeyStoreItem.h: KeyManagerTab item (for GTK4 GtkTreeListModel)          *
 *                                                                         *
 * Copyright (c) 2017-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "KeyStoreItem.h"

/* Property identifiers */
typedef enum {
	PROP_0,

	PROP_NAME,
	PROP_VALUE,
	PROP_STATUS,
	PROP_FLAT_IDX,
	PROP_IS_SECTION,

	PROP_LAST
} RpKeyStorePropID;

static void	rp_key_store_item_set_property(GObject		*object,
					       guint		 prop_id,
					       const GValue	*value,
					       GParamSpec	*pspec);
static void	rp_key_store_item_get_property(GObject		*object,
					       guint		 prop_id,
					       GValue		*value,
					       GParamSpec	*pspec);

static void	rp_key_store_item_finalize    (GObject		*object);

static GParamSpec *props[PROP_LAST];

// KeyStoreItem class
struct _RpKeyStoreItemClass {
	GObjectClass __parent__;
};

// KeyStoreItem instance
struct _RpKeyStoreItem {
	GObject __parent__;

	char		*name;
	char		*value;
	int		flat_idx;

	uint8_t		status;
	bool		is_section;
};

// NOTE: G_DEFINE_TYPE() doesn't work in C++ mode with gcc-6.2
// due to an implicit int to GTypeFlags conversion.
G_DEFINE_TYPE_EXTENDED(RpKeyStoreItem, rp_key_store_item,
	G_TYPE_OBJECT, (GTypeFlags)0, {});

static void
rp_key_store_item_class_init(RpKeyStoreItemClass *klass)
{
	GObjectClass *const gobject_class = G_OBJECT_CLASS(klass);
	gobject_class->set_property = rp_key_store_item_set_property;
	gobject_class->get_property = rp_key_store_item_get_property;
	gobject_class->finalize = rp_key_store_item_finalize;

	/** Properties **/

	props[PROP_NAME] = g_param_spec_string(
		"name", "Name", "Key (or section) name",
		"",
		(GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	props[PROP_VALUE] = g_param_spec_string(
		"value", "Value", "Key value",
		"",
		(GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	props[PROP_STATUS] = g_param_spec_uint(
		"status", "Status", "Key status (corresponds to KeyStoreUI::Status)",
		0U, 4U, 0U,
		(GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	// NOTE: Flat index *should* be considered unsigned,
	// but everything else uses int for this.
	props[PROP_FLAT_IDX] = g_param_spec_int(
		"flat-idx", "Flat Index", "Flat key index for this item (or section index for headers)",
		0, G_MAXINT, 0,
		(GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	// Is this a section header?
	props[PROP_IS_SECTION] = g_param_spec_boolean(
		"is-section", "Is Section?", "Is this a section header?",
		FALSE,
		(GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	// Install the properties.
	g_object_class_install_properties(gobject_class, PROP_LAST, props);
}

static void
rp_key_store_item_init(RpKeyStoreItem *item)
{
	RP_UNUSED(item);
}

RpKeyStoreItem*
rp_key_store_item_new(const char *name, const char *value, uint8_t status, int flat_idx, gboolean is_section)
{
	return g_object_new(RP_TYPE_KEY_STORE_ITEM,
		"name", name, "value", value, "status", status,
		"flat-idx", flat_idx, "is-section", is_section, NULL);
}

RpKeyStoreItem*
rp_key_store_item_new_key(const char *name, const char *value, uint8_t status, int flat_idx)
{
	return rp_key_store_item_new(name, value, status, flat_idx, FALSE);
}

RpKeyStoreItem*
rp_key_store_item_new_section	(const char *name, const char *value, int sect_idx)
{
	return rp_key_store_item_new(name, value, 0, sect_idx, TRUE);
}

/** Properties **/

static void
rp_key_store_item_set_property(GObject		*object,
			       guint		 prop_id,
			       const GValue	*value,
			       GParamSpec	*pspec)
{
	RpKeyStoreItem *const item = RP_KEY_STORE_ITEM(object);

	switch (prop_id) {
		case PROP_NAME: {
			const char *str = g_value_get_string(value);
			if (g_strcmp0(item->name, str) != 0) {
				g_set_str(&item->name, str);
			}
			break;
		}

		case PROP_VALUE: {
			const char *str = g_value_get_string(value);
			if (g_strcmp0(item->value, str) != 0) {
				g_set_str(&item->value, str);
			}
			break;
		}

		case PROP_STATUS:
			item->status = g_value_get_uint(value);
			break;

		case PROP_FLAT_IDX:
			item->flat_idx = g_value_get_int(value);
			break;

		case PROP_IS_SECTION:
			item->is_section = g_value_get_boolean(value);
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
			break;
	}

	// TODO: Notification signals for GListModel?
}

static void
rp_key_store_item_get_property(GObject		*object,
					guint		 prop_id,
					GValue		*value,
					GParamSpec	*pspec)
{
	RpKeyStoreItem *const item = RP_KEY_STORE_ITEM(object);

	switch (prop_id) {
		case PROP_NAME:
			g_value_set_string(value, item->name);
			break;

		case PROP_VALUE:
			g_value_set_string(value, item->value);
			break;

		case PROP_STATUS:
			g_value_set_uint(value, item->status);
			break;

		case PROP_FLAT_IDX:
			g_value_set_boolean(value, item->flat_idx);
			break;

		case PROP_IS_SECTION:
			g_value_set_boolean(value, item->is_section);
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
			break;
	}
}

/** Dispose / Finalize **/

static void
rp_key_store_item_finalize(GObject *object)
{
	RpKeyStoreItem *const item = RP_KEY_STORE_ITEM(object);

	g_free(item->name);
	g_free(item->value);

	// Call the superclass finalize() function.
	G_OBJECT_CLASS(rp_key_store_item_parent_class)->finalize(object);
}

/** Property accessors / mutators **/

void
rp_key_store_item_set_name(RpKeyStoreItem *item, const char *name)
{
	g_return_if_fail(RP_IS_KEY_STORE_ITEM(item));
	if (g_strcmp0(item->name, name) != 0) {
		g_set_str(&item->name, name);
		g_object_notify_by_pspec(G_OBJECT(item), props[PROP_NAME]);
	}

}

const char*
rp_key_store_item_get_name(RpKeyStoreItem *item)
{
	g_return_val_if_fail(RP_IS_KEY_STORE_ITEM(item), NULL);

	// String is owned by this instance.
	return item->name;
}

void
rp_key_store_item_set_value(RpKeyStoreItem *item, const char *value)
{
	g_return_if_fail(RP_IS_KEY_STORE_ITEM(item));
	if (g_strcmp0(item->value, value) != 0) {
		g_set_str(&item->value, value);
		g_object_notify_by_pspec(G_OBJECT(item), props[PROP_VALUE]);
	}
}

const char*
rp_key_store_item_get_value(RpKeyStoreItem *item)
{
	g_return_val_if_fail(RP_IS_KEY_STORE_ITEM(item), NULL);

	// String is owned by this instance.
	return item->value;
}

void
rp_key_store_item_set_status(RpKeyStoreItem *item, uint8_t status)
{
	g_return_if_fail(RP_IS_KEY_STORE_ITEM(item));
	if (item->status != status) {
		item->status = status;	// TODO: Verify range?
		g_object_notify_by_pspec(G_OBJECT(item), props[PROP_STATUS]);
	}

}

uint8_t
rp_key_store_item_get_status(RpKeyStoreItem *item)
{
	g_return_val_if_fail(RP_IS_KEY_STORE_ITEM(item), 0);
	return item->status;
}

void
rp_key_store_item_set_flat_idx(RpKeyStoreItem *item, int flat_idx)
{
	g_return_if_fail(RP_IS_KEY_STORE_ITEM(item));
	if (item->flat_idx != flat_idx) {
		item->flat_idx = flat_idx;
		g_object_notify_by_pspec(G_OBJECT(item), props[PROP_FLAT_IDX]);
	}
}

int
rp_key_store_item_get_flat_idx(RpKeyStoreItem *item)
{
	g_return_val_if_fail(RP_IS_KEY_STORE_ITEM(item), 0);
	return item->flat_idx;
}

void
rp_key_store_item_set_is_section(RpKeyStoreItem *item, gboolean is_section)
{
	g_return_if_fail(RP_IS_KEY_STORE_ITEM(item));
	if (item->is_section != is_section) {
		item->is_section = is_section;
		g_object_notify_by_pspec(G_OBJECT(item), props[PROP_IS_SECTION]);
	}
}

gboolean
rp_key_store_item_get_is_section(RpKeyStoreItem *item)
{
	g_return_val_if_fail(RP_IS_KEY_STORE_ITEM(item), 0);
	return item->is_section;
}
