/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ common)                      *
 * XAttrView_gtk3.cpp: Extended attribute viewer property page.            *
 * (GTK2/GTK3-specific)                                                    *
 *                                                                         *
 * Copyright (c) 2017-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
//#include "XAttrView.hpp"
#include "XAttrView_p.hpp"

// XAttrReader
#include "librpfile/xattr/XAttrReader.hpp"
using LibRpFile::XAttrReader;

/**
 * Initialize the widgets for POSIX xattrs.
 * @param widget RpXAttrView
 * @param scrlXAttr GtkScrolledWindow parent widget for the POSIX xattrs widget
 */
void
rp_xattr_view_init_posix_xattrs_widgets(struct _RpXAttrView *widget, GtkScrolledWindow *scrlXAttr)
{
	// Column titles
	static const char *const column_titles[XATTR_COL_MAX] = {
		NOP_C_("XAttrView", "Name"),
		NOP_C_("XAttrView", "Value"),
	};

	// TODO: GtkListView version for GTK4

	// Create the GtkListStore and GtkTreeView.
	widget->listStore = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_STRING);
	widget->treeView = gtk_tree_view_new_with_model(GTK_TREE_MODEL(widget->listStore));
	gtk_widget_set_name(widget->treeView, "treeView");
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(widget->treeView), TRUE);
	gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrlXAttr), widget->treeView);

#if !GTK_CHECK_VERSION(3,0,0)
	// GTK+ 2.x: Use the "rules hint" for alternating row colors.
	// Deprecated in GTK+ 3.14 (and removed in GTK4), but it doesn't
	// work with GTK+ 3.x anyway.
	// TODO: GTK4's GtkListView might have a similar function.
	gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(widget->treeView), true);
#endif /* !GTK_CHECK_VERSION(3,0,0) */

	// Create the columns.
	// NOTE: Unlock Time is stored as a string, not as a GDateTime or Unix timestamp.
	for (int i = 0; i < XATTR_COL_MAX; i++) {
		GtkTreeViewColumn *const column = gtk_tree_view_column_new();
		gtk_tree_view_column_set_title(column,
			dpgettext_expr(RP_I18N_DOMAIN, "XAttrView", column_titles[i]));
		gtk_tree_view_column_set_resizable(column, TRUE);

		GtkCellRenderer *const renderer = gtk_cell_renderer_text_new();
		gtk_tree_view_column_pack_start(column, renderer, FALSE);
		gtk_tree_view_column_add_attribute(column, renderer, "text", i);
		gtk_tree_view_append_column(GTK_TREE_VIEW(widget->treeView), column);
	}
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

	gtk_list_store_clear(widget->listStore);
	if (!widget->xattrReader->hasGenericXAttrs()) {
		// No generic attributes.
		return -ENOENT;
	}

	const XAttrReader::XAttrList &xattrList = widget->xattrReader->genericXAttrs();
	for (const auto &xattr : xattrList) {
		GtkTreeIter treeIter;
		gtk_list_store_append(widget->listStore, &treeIter);
		// NOTE: Trimming leading and trailing spaces from the value.
		// TODO: If copy is added, include the spaces.
		gchar *value_str = g_strdup(xattr.second.c_str());
		if (value_str) {
			value_str = g_strstrip(value_str);
			gtk_list_store_set(widget->listStore, &treeIter,
				0, xattr.first.c_str(), 1, value_str, -1);
			g_free(value_str);
		}
	}

	// Resize the columns to fit the contents.
	gtk_tree_view_columns_autosize(GTK_TREE_VIEW(widget->treeView));

	// Extended attributes retrieved.
	gtk_widget_set_visible(widget->fraXAttr, true);
	return 0;
}
