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
