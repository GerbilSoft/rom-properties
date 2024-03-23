/***************************************************************************
 * ROM Properties Page shell extension. (GTK4)                             *
 * XAttrViewitem.c: XAttrView item for GtkColumnView                       *
 *                                                                         *
 * Copyright (c) 2017-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "XAttrViewItem.h"

/* Property identifiers */
typedef enum {
	PROP_0,

	PROP_NAME,
	PROP_VALUE,

	PROP_LAST
} RpXAttrViewItemPropID;

static void	rp_xattrview_item_set_property(GObject	*object,
						 guint		 prop_id,
						 const GValue	*value,
						 GParamSpec	*pspec);
static void	rp_xattrview_item_get_property(GObject	*object,
						 guint		 prop_id,
						 GValue		*value,
						 GParamSpec	*pspec);

static void	rp_xattrview_item_finalize    (GObject	*object);

static GParamSpec *props[PROP_LAST];

// XAttrViewItem class
struct _RpXAttrViewItemClass {
	GObjectClass __parent__;
};

// XAttrViewItem instance
struct _RpXAttrViewItem {
	GObject __parent__;

	char	*name;
	char	*value;
};

// NOTE: G_DEFINE_TYPE() doesn't work in C++ mode with gcc-6.2
// due to an implicit int to GTypeFlags conversion.
G_DEFINE_TYPE_EXTENDED(RpXAttrViewItem, rp_xattrview_item,
	G_TYPE_OBJECT, (GTypeFlags)0, {});

static void
rp_xattrview_item_class_init(RpXAttrViewItemClass *klass)
{
	GObjectClass *const gobject_class = G_OBJECT_CLASS(klass);
	gobject_class->set_property = rp_xattrview_item_set_property;
	gobject_class->get_property = rp_xattrview_item_get_property;
	gobject_class->finalize = rp_xattrview_item_finalize;

	/** Properties **/

	props[PROP_NAME] = g_param_spec_string(
		"name", "Name", "Extended attribute name",
		NULL,
		(GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY));

	props[PROP_VALUE] = g_param_spec_string(
		"value", "Value", "Extended attribute value",
		NULL,
		(GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY));

	// Install the properties.
	g_object_class_install_properties(gobject_class, PROP_LAST, props);
}

static void
rp_xattrview_item_init(RpXAttrViewItem *item)
{
	RP_UNUSED(item);
}

RpXAttrViewItem*
rp_xattrview_item_new(const char *name, const char *value)
{
	return g_object_new(RP_TYPE_XATTRVIEW_ITEM,
			"name", name,
			"value", value,
			NULL);
}

/** Properties **/

static void
rp_xattrview_item_set_property(GObject		*object,
			       guint		 prop_id,
			       const GValue	*value,
			       GParamSpec	*pspec)
{
	RpXAttrViewItem *const item = RP_XATTRVIEW_ITEM(object);

	switch (prop_id) {
		case PROP_NAME:
			rp_xattrview_item_set_name(item, g_value_get_string(value));
			break;

		case PROP_VALUE:
			rp_xattrview_item_set_value(item, g_value_get_string(value));
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
			break;
	}

	// TODO: Notification signals for GListModel?
}

static void
rp_xattrview_item_get_property(GObject		*object,
			       guint		 prop_id,
			       GValue		*value,
			       GParamSpec	*pspec)
{
	RpXAttrViewItem *const item = RP_XATTRVIEW_ITEM(object);

	switch (prop_id) {
		case PROP_NAME:
			g_value_set_string(value, item->name);
			break;

		case PROP_VALUE:
			g_value_set_string(value, item->value);
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
			break;
	}
}

/** Dispose / Finalize **/

static void
rp_xattrview_item_finalize(GObject *object)
{
	RpXAttrViewItem *const item = RP_XATTRVIEW_ITEM(object);

	g_free(item->name);
	g_free(item->value);

	// Call the superclass finalize() function.
	G_OBJECT_CLASS(rp_xattrview_item_parent_class)->finalize(object);
}

/** Property accessors / mutators **/

void
rp_xattrview_item_set_name(RpXAttrViewItem *item, const char *name)
{
	g_return_if_fail(RP_IS_XATTRVIEW_ITEM(item));

	if (g_strcmp0(item->name, name) != 0) {
		g_set_str(&item->name, name);
		g_object_notify_by_pspec(G_OBJECT(item), props[PROP_NAME]);
	}
}

const char*
rp_xattrview_item_get_name(RpXAttrViewItem *item)
{
	g_return_val_if_fail(RP_IS_XATTRVIEW_ITEM(item), NULL);

	// String is owned by this instance.
	return item->name;
}

void
rp_xattrview_item_set_value(RpXAttrViewItem *item, const char *value)
{
	g_return_if_fail(RP_IS_XATTRVIEW_ITEM(item));

	if (g_strcmp0(item->value, value) != 0) {
		g_set_str(&item->value, value);
		g_object_notify_by_pspec(G_OBJECT(item), props[PROP_VALUE]);
	}
}

const char*
rp_xattrview_item_get_value(RpXAttrViewItem *item)
{
	g_return_val_if_fail(RP_IS_XATTRVIEW_ITEM(item), NULL);

	// String is owned by this instance.
	return item->value;
}
