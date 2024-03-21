/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ common)                      *
 * ListDataItem.c: RFT_LISTDATA item                                       *
 *                                                                         *
 * Copyright (c) 2017-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "ListDataItem.h"
#include "rp-gtk-enums.h"

/* Property identifiers */
typedef enum {
	PROP_0,

	PROP_COL0_TYPE,
	PROP_ICON,
	PROP_CHECKED,
	PROP_COLUMN_COUNT,
	// NOTE: Column text is not a property.

	PROP_LAST
} RpAchievementPropID;

static void	rp_list_data_item_set_property(GObject	*object,
						 guint		 prop_id,
						 const GValue	*value,
						 GParamSpec	*pspec);
static void	rp_list_data_item_get_property(GObject	*object,
						 guint		 prop_id,
						 GValue		*value,
						 GParamSpec	*pspec);

static void	rp_list_data_item_dispose     (GObject	*object);
static void	rp_list_data_item_finalize    (GObject	*object);

static GParamSpec *props[PROP_LAST];

// ListDataItem class
struct _RpListDataItemClass {
	GObjectClass __parent__;
};

// ListDataItem instance
struct _RpListDataItem {
	GObject __parent__;

	PIMGTYPE	 icon;
	RpListDataItemCol0Type col0_type;
	gboolean	 checked;

	int		 column_count;
	char		**text;
};

// NOTE: G_DEFINE_TYPE() doesn't work in C++ mode with gcc-6.2
// due to an implicit int to GTypeFlags conversion.
G_DEFINE_TYPE_EXTENDED(RpListDataItem, rp_list_data_item,
	G_TYPE_OBJECT, (GTypeFlags)0, {});

static void
rp_list_data_item_class_init(RpListDataItemClass *klass)
{
	GObjectClass *const gobject_class = G_OBJECT_CLASS(klass);
	gobject_class->set_property = rp_list_data_item_set_property;
	gobject_class->get_property = rp_list_data_item_get_property;
	gobject_class->dispose = rp_list_data_item_dispose;
	gobject_class->finalize = rp_list_data_item_finalize;

	/** Properties **/

	props[PROP_COL0_TYPE] = g_param_spec_enum(
		"col0-type", "Column 0 type", "Is column 0 text, checkbox, or icon?",
		RP_TYPE_LIST_DATA_ITEM_COL0_TYPE, RP_LIST_DATA_ITEM_COL0_TYPE_TEXT,
		(GParamFlags)(G_PARAM_READABLE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY));

	props[PROP_ICON] = g_param_spec_object(
		"icon", "Icon", "Icon for this list item",
		PIMGTYPE_GOBJECT_TYPE,
		(GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY));

	props[PROP_CHECKED] = g_param_spec_boolean(
		"checked", "Checked", "Is this list item checked?",
		FALSE,
		(GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY));

	props[PROP_COLUMN_COUNT] = g_param_spec_int(
		"column-count", "Column Count", "Number of text columns",
		1, 16, 1,
		(GParamFlags)(G_PARAM_READABLE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY));

	// Install the properties.
	g_object_class_install_properties(gobject_class, PROP_LAST, props);
}

static void
rp_list_data_item_init(RpListDataItem *item)
{
	RP_UNUSED(item);
}

RpListDataItem*
rp_list_data_item_new(int column_count, RpListDataItemCol0Type col0_type)
{
	g_return_val_if_fail(column_count >= 1, NULL);

	RpListDataItem *const item = g_object_new(RP_TYPE_LIST_DATA_ITEM, NULL);

	// Setting column count manually because set_property() won't set it.
	item->column_count = column_count;
	item->col0_type = col0_type;

	// Allocate the string array.
	item->text = calloc(column_count, sizeof(*item->text));

	return item;
}

/** Properties **/

static void
rp_list_data_item_set_property(GObject		*object,
			       guint		 prop_id,
			       const GValue	*value,
			       GParamSpec	*pspec)
{
	RpListDataItem *const item = RP_LIST_DATA_ITEM(object);

	switch (prop_id) {
		case PROP_ICON:
			rp_list_data_item_set_icon(item, PIMGTYPE_CAST(g_value_get_object(value)));
			break;

		case PROP_CHECKED:
			rp_list_data_item_set_checked(item, g_value_get_boolean(value));
			break;

		// TODO: Handling read-only properties?
		case PROP_COL0_TYPE:
		case PROP_COLUMN_COUNT:
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
			break;
	}

	// TODO: Notification signals for GListModel?
}

static void
rp_list_data_item_get_property(GObject		*object,
			       guint		 prop_id,
			       GValue		*value,
			       GParamSpec	*pspec)
{
	RpListDataItem *const item = RP_LIST_DATA_ITEM(object);

	switch (prop_id) {
		case PROP_COL0_TYPE:
			g_value_set_enum(value, item->col0_type);
			break;

		case PROP_ICON:
			// Caller must take a reference.
			g_value_set_object(value, item->icon ? item->icon : NULL);
			break;

		case PROP_CHECKED:
			g_value_set_boolean(value, item->checked);
			break;

		case PROP_COLUMN_COUNT:
			g_value_set_int(value, item->column_count);
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
			break;
	}
}

/** Dispose / Finalize **/

static void
rp_list_data_item_dispose(GObject *object)
{
	RpListDataItem *const item = RP_LIST_DATA_ITEM(object);

	if (item->icon) {
		PIMGTYPE_unref(item->icon);
		item->icon = NULL;
	}

	// Call the superclass dispose() function.
	G_OBJECT_CLASS(rp_list_data_item_parent_class)->dispose(object);
}


static void
rp_list_data_item_finalize(GObject *object)
{
	RpListDataItem *const item = RP_LIST_DATA_ITEM(object);

	if (item->text) {
		// Free all of the strings first.
		for (int i = 0; i < item->column_count; i++) {
			g_free(item->text[i]);
		}
		g_free(item->text);
		item->text = NULL;
	}

	// Call the superclass finalize() function.
	G_OBJECT_CLASS(rp_list_data_item_parent_class)->finalize(object);
}

/** Property accessors / mutators **/

RpListDataItemCol0Type
rp_list_data_item_get_col0_type(RpListDataItem *item)
{
	g_return_val_if_fail(RP_IS_LIST_DATA_ITEM(item), FALSE);
	return item->col0_type;
}

void
rp_list_data_item_set_icon(RpListDataItem *item, PIMGTYPE icon)
{
	g_return_if_fail(RP_IS_LIST_DATA_ITEM(item));

	if (icon == item->icon) {
		// Same icon. Nothing to do.
		return;
	}

	if (item->icon) {
		PIMGTYPE_unref(item->icon);
	}
	item->icon = PIMGTYPE_ref(icon);

	g_object_notify_by_pspec(G_OBJECT(item), props[PROP_ICON]);
}

PIMGTYPE
rp_list_data_item_get_icon(RpListDataItem *item)
{
	g_return_val_if_fail(RP_IS_LIST_DATA_ITEM(item), NULL);

	// Caller must take a reference to the icon.
	return item->icon;
}

void
rp_list_data_item_set_checked(RpListDataItem *item, gboolean checked)
{
	g_return_if_fail(RP_IS_LIST_DATA_ITEM(item));

	if (item->checked != checked) {
		item->checked = checked;
		g_object_notify_by_pspec(G_OBJECT(item), props[PROP_CHECKED]);
	}
}

gboolean
rp_list_data_item_get_checked(RpListDataItem *item)
{
	g_return_val_if_fail(RP_IS_LIST_DATA_ITEM(item), FALSE);
	return item->checked;
}

void
rp_list_data_item_set_column_text(RpListDataItem *item, int column, const char *text)
{
	g_return_if_fail(RP_IS_LIST_DATA_ITEM(item));
	g_return_if_fail(column >= 0 && column < item->column_count);

	if (item->text[column]) {
		g_free(item->text[column]);
	}
	item->text[column] = g_strdup(text);
	// TODO: Signal that text has changed?
}

const char*
rp_list_data_item_get_column_text(RpListDataItem *item, int column)
{
	g_return_val_if_fail(RP_IS_LIST_DATA_ITEM(item), NULL);
	g_return_val_if_fail(column >= 0 && column < item->column_count, NULL);

	// String is owned by this object.
	return item->text[column];
}
