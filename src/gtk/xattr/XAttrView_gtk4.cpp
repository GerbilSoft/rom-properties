/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ common)                      *
 * XAttrView_gtk4.cpp: Extended attribute viewer property page.            *
 * (GTK4-specific)                                                         *
 *                                                                         *
 * Copyright (c) 2017-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "XAttrView_p.hpp"

// XAttrReader
#include "librpfile/xattr/XAttrReader.hpp"
using LibRpFile::XAttrReader;

#include "XAttrViewItem.h"
#include "../sort_funcs_common.h"
#include "../gtk4/sort_funcs.h"

/**
 * Sorting function for COLSORT_NOCASE (case-insensitive).
 * @param a
 * @param b
 * @param userdata Column ID
 * @return -1, 0, or 1 (like strcmp())
 */
static gint
sort_XAttrViewItem_nocase(gconstpointer a, gconstpointer b, gpointer userdata)
{
	RpXAttrViewItem *const xaviA = RP_XATTRVIEW_ITEM((gpointer)a);
	RpXAttrViewItem *const xaviB = RP_XATTRVIEW_ITEM((gpointer)b);

	// Get the text and do a case-insensitive string comparison.
	const int column = GPOINTER_TO_INT(userdata);
	const char *strA, *strB;
	switch (column) {
		default:
			assert(!"Invalid column for XAttrViewItem.");
			return 0;
		case 0:
			strA = rp_xattrview_item_get_name(xaviA);
			strB = rp_xattrview_item_get_name(xaviB);
			break;
		case 1:
			strA = rp_xattrview_item_get_value(xaviA);
			strB = rp_xattrview_item_get_value(xaviB);
			break;
	}

	return rp_sort_string_nocase(strA, strB);
}

static void
setup_listitem_cb(GtkListItemFactory *factory, GtkListItem *list_item, gpointer user_data)
{
	RP_UNUSED(factory);

	// All text columns.
	GtkWidget *const label = gtk_label_new(nullptr);
	gtk_label_set_xalign(GTK_LABEL(label), 0.0f);
	gtk_widget_set_halign(label, GTK_ALIGN_START);
	gtk_list_item_set_child(list_item, label);
}

static void
bind_listitem_cb(GtkListItemFactory *factory, GtkListItem *list_item, gpointer user_data)
{
	RP_UNUSED(factory);

	GtkWidget *const widget = gtk_list_item_get_child(list_item);
	assert(widget != nullptr);
	if (!widget)
		return;

	// user_data is the column number.
	// - Column 0: Name
	// - Column 1: Value
	RpXAttrViewItem *const item = RP_XATTRVIEW_ITEM(gtk_list_item_get_item(list_item));
	assert(item != nullptr);
	if (!item)
		return;

	const int column = GPOINTER_TO_INT(user_data);
	switch (column) {
		default:
			assert(!"Invalid column number!");
			return;
		case 0:
			gtk_label_set_text(GTK_LABEL(widget), rp_xattrview_item_get_name(item));
			break;
		case 1:
			gtk_label_set_text(GTK_LABEL(widget), rp_xattrview_item_get_value(item));
			break;
	}
}

/**
 * Initialize the widgets for POSIX xattrs.
 * @param widget RpXAttrView
 * @param scrlXAttr GtkScrolledWindow parent widget for the POSIX xattrs widget
 */
void
rp_xattr_view_init_posix_xattrs_widgets(struct _RpXAttrView *widget, GtkScrolledWindow *scrlXAttr)
{
	// Create the GListStore and GtkColumnView.
	widget->listStore = g_list_store_new(RP_TYPE_XATTRVIEW_ITEM);
	widget->columnView = gtk_column_view_new(nullptr);

	// GtkColumnView requires a GtkSelectionModel, so we'll create
	// a GtkSingleSelection to wrap around the GListStore.
	// NOTE: Also setting up the sort model here.
	GtkSorter *const sorter = (GtkSorter*)g_object_ref(gtk_column_view_get_sorter(GTK_COLUMN_VIEW(widget->columnView)));
	GtkSortListModel *const sortListModel = gtk_sort_list_model_new(G_LIST_MODEL(widget->listStore), sorter);
	GtkSingleSelection *const selModel = gtk_single_selection_new(G_LIST_MODEL(sortListModel));
	gtk_column_view_set_model(GTK_COLUMN_VIEW(widget->columnView), GTK_SELECTION_MODEL(selModel));
	g_object_unref(selModel);

	gtk_widget_set_name(widget->columnView, "treeView");
	gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrlXAttr), widget->columnView);

	// NOTE: Regarding object ownership:
	// - GtkColumnViewColumn takes ownership of the GtkListItemFactory
	// - GtkColumnView takes ownership of the GtkColumnViewColumn
	// As such, neither the factory nor the column objects will be unref()'d here.

	// Column titles
	static const char *const column_titles[XATTR_COL_MAX] = {
		NOP_C_("XAttrView", "Name"),
		NOP_C_("XAttrView", "Value"),
	};

	// Create the columns.
	GtkColumnViewColumn *sortingColumn = nullptr;;
	for (int i = 0; i < 2; i++) {
		GtkListItemFactory *const factory = gtk_signal_list_item_factory_new();
		g_signal_connect(factory, "setup", G_CALLBACK(setup_listitem_cb), GINT_TO_POINTER(i));
		g_signal_connect(factory, "bind", G_CALLBACK(bind_listitem_cb), GINT_TO_POINTER(i));

		GtkColumnViewColumn *const column = gtk_column_view_column_new(column_titles[i], factory);
		gtk_column_view_append_column(GTK_COLUMN_VIEW(widget->columnView), column);
		gtk_column_view_column_set_resizable(column, true);
		gtk_column_view_column_set_expand(column, (i == 1));

		// Sort by name (column 0) by default.
		if (i == 0) {
			sortingColumn = column;
		}

		// Use case-insensitive sorting.
		// TODO: Case-sensitive because Linux file systems? (or make it an option)
		GtkCustomSorter *const sorter = gtk_custom_sorter_new(
			sort_XAttrViewItem_nocase, GINT_TO_POINTER(i), nullptr);
		gtk_column_view_column_set_sorter(column, GTK_SORTER(sorter));
		g_object_unref(sorter);
	}

	// Default to sorting by name.
	// FIXME: This might not be working?
	assert(sortingColumn != nullptr);
	gtk_column_view_sort_by_column(GTK_COLUMN_VIEW(widget->columnView), sortingColumn, GTK_SORT_ASCENDING);
}

/**
 * Load POSIX xattrs, if available.
 * @param widget XAttrView
 * @return 0 on success; negative POSIX error code on error.
 */
int
rp_xattr_view_load_posix_xattrs(struct _RpXAttrView *widget)
{
	// Hide by default.
	// If we do have attributes, we'll show the widgets there.
	gtk_widget_set_visible(widget->fraXAttr, false);

	g_list_store_remove_all(widget->listStore);
	if (!widget->xattrReader->hasGenericXAttrs()) {
		// No generic attributes.
		return -ENOENT;
	}

	const XAttrReader::XAttrList &xattrList = widget->xattrReader->genericXAttrs();
	for (const auto &xattr : xattrList) {
		// NOTE: Trimming leading and trailing spaces from the value.
		// TODO: If copy is added, include the spaces.
		gchar *value_str = g_strdup(xattr.second.c_str());
		if (value_str) {
			value_str = g_strstrip(value_str);
		}
		RpXAttrViewItem *const item = rp_xattrview_item_new(xattr.first.c_str(), value_str);
		g_free(value_str);

		g_list_store_append(widget->listStore, item);
	}

	// Resize the columns to fit the contents. [TODO]
	//gtk_tree_view_columns_autosize(GTK_TREE_VIEW(widget->treeView));

	// Extended attributes retrieved.
	gtk_widget_set_visible(widget->fraXAttr, true);
	return 0;
}

/**
 * Clear POSIX xattrs.
 */
void
rp_xattr_view_clear_posix_xattrs(struct _RpXAttrView *widget)
{
	g_list_store_remove_all(widget->listStore);
}
