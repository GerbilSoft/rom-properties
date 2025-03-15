/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ common)                      *
 * XAttrView_p.hpp: Extended attribute viewer property page.               *
 * (PRIVATE CLASS)                                                         *
 *                                                                         *
 * Copyright (c) 2017-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

// XAttrReader
namespace LibRpFile {
	class XAttrReader;
}

/* Property identifiers */
typedef enum {
	PROP_0,

	PROP_URI,

	PROP_LAST
} RpXAttrViewPropID;

/* Column identifiers */
typedef enum {
	XATTR_COL_NAME,
	XATTR_COL_VALUE,

	XATTR_COL_MAX
} XAttrColumns;

#if GTK_CHECK_VERSION(3, 0, 0)
typedef GtkBoxClass superclass;
typedef GtkBox super;
#define GTK_TYPE_SUPER GTK_TYPE_BOX
#define USE_GTK_GRID 1	// Use GtkGrid instead of GtkTable.
#else /* !GTK_CHECK_VERSION(3, 0, 0) */
typedef GtkVBoxClass superclass;
typedef GtkVBox super;
#define GTK_TYPE_SUPER GTK_TYPE_VBOX
#endif /* GTK_CHECK_VERSION(3, 0, 0) */

// XAttrView class
struct _RpXAttrViewClass {
	superclass __parent__;
};

// XAttrView instance
struct _RpXAttrView {
	super __parent__;

	gchar		*uri;
	LibRpFile::XAttrReader *xattrReader;
	gboolean	has_attributes;

	GtkWidget	*fraExt2Attributes;
	GtkWidget	*ext2AttrView;

	GtkWidget	*fraXfsAttributes;
	GtkWidget	*xfsAttrView;

	GtkWidget	*fraDosAttributes;
	GtkWidget	*dosAttrView;

	GtkWidget	*fraXAttr;
#if GTK_CHECK_VERSION(4, 0, 0)
	GListStore	*listStore;
	GtkWidget	*columnView;
#else /* !GTK_CHECK_VERSION(4, 0, 0) */
	GtkListStore	*listStore;
	GtkWidget	*treeView;
#endif /* GTK_CHECK_VERSION(4, 0, 0) */
};

G_BEGIN_DECLS

/**
 * Initialize the widgets for POSIX xattrs.
 * @param widget RpXAttrView
 * @param scrlXAttr GtkScrolledWindow parent widget for the POSIX xattrs widget
 */
void rp_xattr_view_init_posix_xattrs_widgets(struct _RpXAttrView *widget, GtkScrolledWindow *scrlXAttr);

/**
 * Load POSIX xattrs, if available.
 * @param widget XAttrView
 * @return 0 on success; negative POSIX error code on error.
 */
int rp_xattr_view_load_posix_xattrs(struct _RpXAttrView *widget);

/**
 * Clear POSIX xattrs.
 */
void rp_xattr_view_clear_posix_xattrs(struct _RpXAttrView *widget);

G_END_DECLS
