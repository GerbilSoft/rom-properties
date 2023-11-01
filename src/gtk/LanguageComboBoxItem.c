/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ common)                      *
 * LanguageComboBoxItem.cpp: Language ComboBox: GtkComboBox version        *
 *                                                                         *
 * Copyright (c) 2017-2022 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "LanguageComboBoxItem.h"

/* Property identifiers */
typedef enum {
	PROP_0,

	PROP_ICON,
	PROP_NAME,
	PROP_LC,

	PROP_LAST
} RpLanguageComboBoxPropID;

static void	rp_language_combo_box_item_set_property(GObject		*object,
							guint		 prop_id,
							const GValue	*value,
							GParamSpec	*pspec);
static void	rp_language_combo_box_item_get_property(GObject		*object,
							guint		 prop_id,
							GValue		*value,
							GParamSpec	*pspec);

static void	rp_language_combo_box_item_dispose     (GObject		*object);
static void	rp_language_combo_box_item_finalize    (GObject		*object);

static GParamSpec *props[PROP_LAST];

// LanguageComboBoxItem class
struct _RpLanguageComboBoxItemClass {
	GObjectClass __parent__;
};

// LanguageComboBoxItem instance
struct _RpLanguageComboBoxItem {
	GObject __parent__;

	PIMGTYPE	 icon;
	char		*name;
	uint32_t	 lc;
};

// NOTE: G_DEFINE_TYPE() doesn't work in C++ mode with gcc-6.2
// due to an implicit int to GTypeFlags conversion.
G_DEFINE_TYPE_EXTENDED(RpLanguageComboBoxItem, rp_language_combo_box_item,
	G_TYPE_OBJECT, (GTypeFlags)0, {});

static void
rp_language_combo_box_item_class_init(RpLanguageComboBoxItemClass *klass)
{
	GObjectClass *const gobject_class = G_OBJECT_CLASS(klass);
	gobject_class->set_property = rp_language_combo_box_item_set_property;
	gobject_class->get_property = rp_language_combo_box_item_get_property;
	gobject_class->dispose = rp_language_combo_box_item_dispose;
	gobject_class->finalize = rp_language_combo_box_item_finalize;

	/** Properties **/

	props[PROP_ICON] = g_param_spec_object(
		"icon", "Icon", "Icon representing this language code",
		PIMGTYPE_GOBJECT_TYPE,
		(GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	props[PROP_NAME] = g_param_spec_string(
		"name", "Name", "Language name",
		"",
		(GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	props[PROP_LC] = g_param_spec_uint(
		"lc", "Language code", "Language code for this item",
		0U, ~0U, 0U,
		(GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	// Install the properties.
	g_object_class_install_properties(gobject_class, PROP_LAST, props);
}

static void
rp_language_combo_box_item_init(RpLanguageComboBoxItem *item)
{
	RP_UNUSED(item);
}

RpLanguageComboBoxItem*
rp_language_combo_box_item_new(PIMGTYPE icon, const char *name, uint32_t lc)
{
	return g_object_new(RP_TYPE_LANGUAGE_COMBO_BOX_ITEM,
		"icon", icon, "name", name, "lc", lc, NULL);
}

/** Properties **/

static void
rp_language_combo_box_item_set_property(GObject		*object,
					guint		 prop_id,
					const GValue	*value,
					GParamSpec	*pspec)
{
	RpLanguageComboBoxItem *const item = RP_LANGUAGE_COMBO_BOX_ITEM(object);

	switch (prop_id) {
		case PROP_ICON: {
			PIMGTYPE const icon = PIMGTYPE_CAST(g_value_get_object(value));
			if (icon) {
				if (item->icon) {
					PIMGTYPE_unref(item->icon);
					item->icon = NULL;
				}
				item->icon = PIMGTYPE_ref(icon);
			}
			break;
		}

		case PROP_NAME:
			g_set_str(&item->name, g_value_get_string(value));
			break;

		case PROP_LC:
			item->lc = g_value_get_uint(value);
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
			break;
	}

	// TODO: Notification signals for GListModel?
}

static void
rp_language_combo_box_item_get_property(GObject		*object,
					guint		 prop_id,
					GValue		*value,
					GParamSpec	*pspec)
{
	RpLanguageComboBoxItem *const item = RP_LANGUAGE_COMBO_BOX_ITEM(object);

	switch (prop_id) {
		case PROP_ICON:
			if (item->icon) {
				// Caller must take a reference.
				g_value_set_object(value, item->icon);
			}
			break;

		case PROP_NAME:
			g_value_set_string(value, item->name);
			break;

		case PROP_LC:
			g_value_set_uint(value, item->lc);
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
			break;
	}
}

/** Dispose / Finalize **/

static void
rp_language_combo_box_item_dispose(GObject *object)
{
	RpLanguageComboBoxItem *const item = RP_LANGUAGE_COMBO_BOX_ITEM(object);

	if (item->icon) {
		PIMGTYPE_unref(item->icon);
		item->icon = NULL;
	}

	// Call the superclass dispose() function.
	G_OBJECT_CLASS(rp_language_combo_box_item_parent_class)->dispose(object);
}


static void
rp_language_combo_box_item_finalize(GObject *object)
{
	RpLanguageComboBoxItem *const item = RP_LANGUAGE_COMBO_BOX_ITEM(object);

	g_free(item->name);

	// Call the superclass finalize() function.
	G_OBJECT_CLASS(rp_language_combo_box_item_parent_class)->finalize(object);
}

/** Property accessors / mutators **/

void
rp_language_combo_box_item_set_icon(RpLanguageComboBoxItem *item, PIMGTYPE icon)
{
	g_return_if_fail(RP_IS_LANGUAGE_COMBO_BOX_ITEM(item));

	if (item->icon) {
		PIMGTYPE_unref(item->icon);
	}
	item->icon = PIMGTYPE_ref(icon);

	g_object_notify_by_pspec(G_OBJECT(item), props[PROP_ICON]);

}

PIMGTYPE
rp_language_combo_box_item_get_icon(RpLanguageComboBoxItem *item)
{
	g_return_val_if_fail(RP_IS_LANGUAGE_COMBO_BOX_ITEM(item), NULL);

	// Caller must take a reference to the icon.
	return item->icon;
}

void
rp_language_combo_box_item_set_name(RpLanguageComboBoxItem *item, const char *name)
{
	g_return_if_fail(RP_IS_LANGUAGE_COMBO_BOX_ITEM(item));
	g_set_str(&item->name, name);
	g_object_notify_by_pspec(G_OBJECT(item), props[PROP_NAME]);

}

const char*
rp_language_combo_box_item_get_name(RpLanguageComboBoxItem *item)
{
	g_return_val_if_fail(RP_IS_LANGUAGE_COMBO_BOX_ITEM(item), NULL);

	// String is owned by this instance.
	return item->name;
}

void
rp_language_combo_box_item_set_lc(RpLanguageComboBoxItem *item, uint32_t lc)
{
	g_return_if_fail(RP_IS_LANGUAGE_COMBO_BOX_ITEM(item));
	item->lc = lc;
	g_object_notify_by_pspec(G_OBJECT(item), props[PROP_LC]);

}

uint32_t
rp_language_combo_box_item_get_lc(RpLanguageComboBoxItem *item)
{
	g_return_val_if_fail(RP_IS_LANGUAGE_COMBO_BOX_ITEM(item), 0);
	return item->lc;
}
