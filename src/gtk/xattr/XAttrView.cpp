/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ common)                      *
 * XAttrView.cpp: Extended attribute viewer property page.                 *
 *                                                                         *
 * Copyright (c) 2017-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "XAttrView.hpp"
#include "XAttrView_p.hpp"

// XAttrReader
#include "librpfile/xattr/XAttrReader.hpp"
using LibRpFile::XAttrReader;

// Attribute viewer widgets
#include "Ext2AttrView.h"
#include "XfsAttrView.h"
#include "DosAttrView.h"

static void	rp_xattr_view_set_property(GObject	*object,
					   guint	 prop_id,
					   const GValue	*value,
					   GParamSpec	*pspec);
static void	rp_xattr_view_get_property(GObject	*object,
					   guint	 prop_id,
					   GValue	*value,
					   GParamSpec	*pspec);
static void	rp_xattr_view_finalize    (GObject	*object);

/** Update display **/
static int	rp_xattr_view_load_attributes(RpXAttrView *widget);
static void	rp_xattr_view_clear_display_widgets(RpXAttrView *widget);

static GParamSpec *props[PROP_LAST];

// NOTE: G_DEFINE_TYPE() doesn't work in C++ mode with gcc-6.2
// due to an implicit int to GTypeFlags conversion.
G_DEFINE_TYPE_EXTENDED(RpXAttrView, rp_xattr_view,
	GTK_TYPE_SUPER, (GTypeFlags)0, {});

static void
rp_xattr_view_class_init(RpXAttrViewClass *klass)
{
	GObjectClass *const gobject_class = G_OBJECT_CLASS(klass);
	gobject_class->set_property = rp_xattr_view_set_property;
	gobject_class->get_property = rp_xattr_view_get_property;
	gobject_class->finalize = rp_xattr_view_finalize;

	/** Properties **/

	props[PROP_URI] = g_param_spec_string(
		"uri", "URI", "URI of the file being displayed.",
		nullptr,
		(GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY));

	// Install the properties.
	g_object_class_install_properties(gobject_class, PROP_LAST, props);
}

static void
rp_xattr_view_init(RpXAttrView *widget)
{
#if GTK_CHECK_VERSION(3,0,0)
	// Make this a VBox.
	gtk_orientable_set_orientation(GTK_ORIENTABLE(widget), GTK_ORIENTATION_VERTICAL);
#endif /* GTK_CHECK_VERSION(3,0,0) */

	// Ext2 attributes
	widget->fraExt2Attributes = gtk_frame_new(C_("XAttrView", "Ext2 Attributes"));
	gtk_widget_set_name(widget->fraExt2Attributes, "fraExt2Attributes");
	GtkWidget *const vboxExt2Attributes = rp_gtk_vbox_new(0);
	gtk_widget_set_name(vboxExt2Attributes, "vboxExt2Attributes");
	widget->ext2AttrView = rp_ext2_attr_view_new();
	gtk_widget_set_name(widget->ext2AttrView, "ext2AttrView");

	// XFS attributes
	widget->fraXfsAttributes = gtk_frame_new(C_("XAttrView", "XFS Attributes"));
	gtk_widget_set_name(widget->fraXfsAttributes, "fraXfsAttributes");
	GtkWidget *const vboxXfsAttributes = rp_gtk_vbox_new(0);
	gtk_widget_set_name(vboxXfsAttributes, "vboxXfsAttributes");
	widget->xfsAttrView = rp_xfs_attr_view_new();
	gtk_widget_set_name(widget->xfsAttrView, "xfsAttrView");

	// MS-DOS attributes
	widget->fraDosAttributes = gtk_frame_new(C_("XAttrView", "MS-DOS Attributes"));
	gtk_widget_set_name(widget->fraDosAttributes, "fraDosAttributes");
	GtkWidget *const vboxDosAttributes = rp_gtk_vbox_new(0);
	gtk_widget_set_name(vboxDosAttributes, "vboxDosAttributes");
	widget->dosAttrView = rp_dos_attr_view_new();
	gtk_widget_set_name(widget->dosAttrView, "dosAttrView");

	// Extended attributes
	widget->fraXAttr = gtk_frame_new(C_("XAttrView", "Extended Attributes"));
	gtk_widget_set_name(widget->fraXAttr, "fraXAttr");

	// Scroll area for the GtkTreeView.
#if GTK_CHECK_VERSION(4,0,0)
	GtkWidget *const scrlXAttr = gtk_scrolled_window_new();
	gtk_scrolled_window_set_has_frame(GTK_SCROLLED_WINDOW(scrlXAttr), true);
#else /* !GTK_CHECK_VERSION(4,0,0) */
	GtkWidget *const scrlXAttr = gtk_scrolled_window_new(nullptr, nullptr);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrlXAttr), GTK_SHADOW_IN);
#endif /* GTK_CHECK_VERSION */
	gtk_widget_set_name(scrlXAttr, "scrlXAttr");
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrlXAttr),
		GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
#if GTK_CHECK_VERSION(2,91,1)
	gtk_widget_set_halign(scrlXAttr, GTK_ALIGN_FILL);
	gtk_widget_set_valign(scrlXAttr, GTK_ALIGN_FILL);
	gtk_widget_set_hexpand(scrlXAttr, TRUE);
	gtk_widget_set_vexpand(scrlXAttr, TRUE);
	gtk_widget_set_margin(scrlXAttr, 6);	// TODO: GTK2 version
#endif /* GTK_CHECK_VERSION(2,91,1) */

	// Initialize the GtkTreeView (GTK2/GTK3) or GtkColumnView (GTK4).
	rp_xattr_view_init_posix_xattrs_widgets(widget, GTK_SCROLLED_WINDOW(scrlXAttr));

#if GTK_CHECK_VERSION(4,0,0)
	gtk_box_append(GTK_BOX(vboxExt2Attributes), widget->ext2AttrView);
	gtk_box_append(GTK_BOX(vboxXfsAttributes), widget->xfsAttrView);
	gtk_box_append(GTK_BOX(vboxDosAttributes), widget->dosAttrView);
#else /* !GTK_CHECK_VERSION(4,0,0) */
	gtk_box_pack_start(GTK_BOX(vboxExt2Attributes), widget->ext2AttrView, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vboxXfsAttributes), widget->xfsAttrView, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vboxDosAttributes), widget->dosAttrView, FALSE, FALSE, 0);
#endif /* GTK_CHECK_VERSION(4,0,0) */

#if GTK_CHECK_VERSION(2,91,0)
	gtk_widget_set_margin(widget->fraExt2Attributes, 6);
	gtk_widget_set_margin(widget->fraXfsAttributes, 6);
	gtk_widget_set_margin(widget->fraDosAttributes, 6);
	gtk_widget_set_margin(widget->fraXAttr, 6);
	gtk_widget_set_margin(vboxExt2Attributes, 6);
	gtk_widget_set_margin(vboxXfsAttributes, 6);
	gtk_widget_set_margin(vboxDosAttributes, 6);
	gtk_frame_set_child(GTK_FRAME(widget->fraExt2Attributes), vboxExt2Attributes);
	gtk_frame_set_child(GTK_FRAME(widget->fraXfsAttributes), vboxXfsAttributes);
	gtk_frame_set_child(GTK_FRAME(widget->fraDosAttributes), vboxDosAttributes);
	gtk_frame_set_child(GTK_FRAME(widget->fraXAttr), scrlXAttr);
#else /* !GTK_CHECK_VERSION(2,91,0) */
	// NOTE: The extra alignments outside the frame reduce the frame widths.
	// This only affects GTK2, so meh.
	GtkWidget *const alignVBoxExt2Attributes = gtk_alignment_new(0.0f, 0.0f, 0.0f, 0.0f);
	gtk_widget_set_name(alignVBoxExt2Attributes, "alignVBoxExt2Attributes");
	gtk_widget_show(alignVBoxExt2Attributes);
	gtk_alignment_set_padding(GTK_ALIGNMENT(alignVBoxExt2Attributes), 6, 6, 6, 6);
	gtk_container_add(GTK_CONTAINER(alignVBoxExt2Attributes), vboxExt2Attributes);
	gtk_frame_set_child(GTK_FRAME(widget->fraExt2Attributes), alignVBoxExt2Attributes);

	GtkWidget *const alignFraExt2Attributes = gtk_alignment_new(0.0f, 0.0f, 0.0f, 0.0f);
	gtk_widget_set_name(alignFraExt2Attributes, "alignFraExt2Attributes");
	gtk_widget_show(alignFraExt2Attributes);
	gtk_alignment_set_padding(GTK_ALIGNMENT(alignFraExt2Attributes), 6, 6, 6, 6);
	gtk_container_add(GTK_CONTAINER(alignFraExt2Attributes), widget->fraExt2Attributes);

	GtkWidget *const alignVBoxXfsAttributes = gtk_alignment_new(0.0f, 0.0f, 0.0f, 0.0f);
	gtk_widget_set_name(alignVBoxXfsAttributes, "alignVBoxXfsAttributes");
	gtk_widget_show(alignVBoxXfsAttributes);
	gtk_alignment_set_padding(GTK_ALIGNMENT(alignVBoxXfsAttributes), 6, 6, 6, 6);
	gtk_container_add(GTK_CONTAINER(alignVBoxXfsAttributes), vboxXfsAttributes);
	gtk_frame_set_child(GTK_FRAME(widget->fraXfsAttributes), alignVBoxXfsAttributes);

	GtkWidget *const alignFraXfsAttributes = gtk_alignment_new(0.0f, 0.0f, 0.0f, 0.0f);
	gtk_widget_set_name(alignFraXfsAttributes, "alignFraXfsAttributes");
	gtk_widget_show(alignFraXfsAttributes);
	gtk_alignment_set_padding(GTK_ALIGNMENT(alignFraXfsAttributes), 6, 6, 6, 6);
	gtk_container_add(GTK_CONTAINER(alignFraXfsAttributes), widget->fraXfsAttributes);

	GtkWidget *const alignVBoxDosAttributes = gtk_alignment_new(0.0f, 0.0f, 0.0f, 0.0f);
	gtk_widget_set_name(alignVBoxDosAttributes, "alignVBoxDosAttributes");
	gtk_widget_show(alignVBoxDosAttributes);
	gtk_alignment_set_padding(GTK_ALIGNMENT(alignVBoxDosAttributes), 6, 6, 6, 6);
	gtk_container_add(GTK_CONTAINER(alignVBoxDosAttributes), vboxDosAttributes);
	gtk_frame_set_child(GTK_FRAME(widget->fraDosAttributes), alignVBoxDosAttributes);

	GtkWidget *const alignFraDosAttributes = gtk_alignment_new(0.0f, 0.0f, 0.0f, 0.0f);
	gtk_widget_set_name(alignFraDosAttributes, "alignFraDosAttributes");
	gtk_widget_show(alignFraDosAttributes);
	gtk_alignment_set_padding(GTK_ALIGNMENT(alignFraDosAttributes), 6, 6, 6, 6);
	gtk_container_add(GTK_CONTAINER(alignFraDosAttributes), widget->fraDosAttributes);

	GtkWidget *const alignScrlXAttr = gtk_alignment_new(0.0f, 0.0f, 0.0f, 0.0f);
	gtk_widget_set_name(alignScrlXAttr, "alignScrlXAttr");
	gtk_widget_show(alignScrlXAttr);
	gtk_alignment_set_padding(GTK_ALIGNMENT(alignScrlXAttr), 6, 6, 6, 6);
	gtk_container_add(GTK_CONTAINER(alignScrlXAttr), scrlXAttr);
	gtk_frame_set_child(GTK_FRAME(widget->fraXAttr), alignScrlXAttr);

	GtkWidget *const alignFraXAttr = gtk_alignment_new(0.0f, 0.0f, 0.0f, 0.0f);
	gtk_widget_set_name(alignFraXAttr, "alignFraXAttr");
	gtk_widget_show(alignFraXAttr);
	gtk_alignment_set_padding(GTK_ALIGNMENT(alignFraXAttr), 6, 6, 6, 6);
	gtk_container_add(GTK_CONTAINER(alignFraXAttr), widget->fraXAttr);
#endif /* GTK_CHECK_VERSION(2,91,0) */

#if GTK_CHECK_VERSION(4,0,0)
	// Default to hidden. (GTK4 defaults to visible)
	gtk_widget_set_visible(widget->fraExt2Attributes, false);
	gtk_widget_set_visible(widget->fraXfsAttributes, false);
	gtk_widget_set_visible(widget->fraDosAttributes, false);
	gtk_widget_set_visible(widget->fraXAttr, false);

	gtk_box_append(GTK_BOX(widget), widget->fraExt2Attributes);
	gtk_box_append(GTK_BOX(widget), widget->fraXfsAttributes);
	gtk_box_append(GTK_BOX(widget), widget->fraDosAttributes);
	gtk_box_append(GTK_BOX(widget), widget->fraXAttr);	// TODO: Expand?
#else /* !GTK_CHECK_VERSION(4,0,0) */
#  if GTK_CHECK_VERSION(2,91,0)
	gtk_box_pack_start(GTK_BOX(widget), widget->fraExt2Attributes, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(widget), widget->fraXfsAttributes, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(widget), widget->fraDosAttributes, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(widget), widget->fraXAttr, TRUE, TRUE, 0);
#  else /* !GTK_CHECK_VERSION(2,91,0) */
	gtk_box_pack_start(GTK_BOX(widget), alignFraExt2Attributes, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(widget), alignFraXfsAttributes, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(widget), alignFraDosAttributes, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(widget), alignFraXAttr, TRUE, TRUE, 0);	// FIXME: Expand isn't working here.
#  endif /* GTK_CHECK_VERSION(2,91,0) */
	gtk_widget_show(vboxExt2Attributes);
	gtk_widget_show(vboxXfsAttributes);
	gtk_widget_show(vboxDosAttributes);
	gtk_widget_show(widget->ext2AttrView);
	gtk_widget_show(widget->xfsAttrView);
	gtk_widget_show(widget->dosAttrView);
	gtk_widget_show(scrlXAttr);
	gtk_widget_show(widget->treeView);
#endif /* GTK_CHECK_VERSION(4,0,0) */
}

/**
 * Create a new XAttrView widget.
 * @param uri URI
 * @return XAttrView
 */
GtkWidget*
rp_xattr_view_new(const gchar *uri)
{
	return (GtkWidget*)g_object_new(RP_TYPE_XATTR_VIEW, "uri", uri, nullptr);
}

/** Properties **/

static void
rp_xattr_view_set_property(GObject	*object,
			   guint	 prop_id,
			   const GValue	*value,
			   GParamSpec	*pspec)
{
	RpXAttrView *const widget = RP_XATTR_VIEW(object);

	switch (prop_id) {
		case PROP_URI:
			rp_xattr_view_set_uri(widget, g_value_get_string(value));
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
			break;
	}
}

static void
rp_xattr_view_get_property(GObject	*object,
			   guint	 prop_id,
			   GValue	*value,
			   GParamSpec	*pspec)
{
	RpXAttrView *const widget = RP_XATTR_VIEW(object);

	switch (prop_id) {
		case PROP_URI:
			g_value_set_string(value, widget->uri);
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
			break;
	}
}

/** Dispose / Finalize **/

static void
rp_xattr_view_finalize(GObject *object)
{
	RpXAttrView *const widget = RP_XATTR_VIEW(object);

	g_free(widget->uri);
	delete widget->xattrReader;

	// Call the superclass finalize() function.
	G_OBJECT_CLASS(rp_xattr_view_parent_class)->finalize(object);
}

/** Update display **/

/**
 * Load Ext2 attributes, if available.
 * @param widget XAttrView
 * @return 0 on success; negative POSIX error code on error.
 */
static int
rp_xattr_view_load_ext2_attrs(RpXAttrView *widget)
{
	// Hide by default.
	// If we do have attributes, we'll show the widgets there.
	gtk_widget_set_visible(widget->fraExt2Attributes, false);

	if (!widget->xattrReader->hasExt2Attributes()) {
		// No Ext2 attributes.
		return -ENOENT;
	}

	// We have Ext2 attributes.
	rp_ext2_attr_view_set_flags(RP_EXT2_ATTR_VIEW(widget->ext2AttrView),
		widget->xattrReader->ext2Attributes());
	gtk_widget_set_visible(widget->fraExt2Attributes, true);
	return 0;
}

/**
 * Load XFS attributes, if available.
 * @param widget XAttrView
 * @return 0 on success; negative POSIX error code on error.
 */
static int
rp_xattr_view_load_xfs_attrs(RpXAttrView *widget)
{
	// Hide by default.
	// If we do have attributes, we'll show the widgets there.
	gtk_widget_set_visible(widget->fraXfsAttributes, false);

	if (!widget->xattrReader->hasXfsAttributes()) {
		// No XFS attributes.
		return -ENOENT;
	}

	// We have XFS attributes.
	// NOTE: If all attributes are 0, don't bother showing it.
	// XFS isn't *nearly* as common as Ext2/Ext3/Ext4.
	// TODO: ...unless this is an XFS file system?
	const uint32_t xflags = widget->xattrReader->xfsXFlags();
	const uint32_t project_id = widget->xattrReader->xfsProjectId();
	if (xflags == 0 && project_id == 0) {
		// All zero.
		return -ENOENT;
	}

	// Show the XFS attributes.
	rp_xfs_attr_view_set_xflags(RP_XFS_ATTR_VIEW(widget->xfsAttrView),
		widget->xattrReader->xfsXFlags());
	rp_xfs_attr_view_set_project_id(RP_XFS_ATTR_VIEW(widget->xfsAttrView),
		widget->xattrReader->xfsProjectId());
	gtk_widget_set_visible(widget->fraXfsAttributes, true);
	return 0;
}

/**
 * Load MS-DOS attributes, if available.
 * @param widget XAttrView
 * @return 0 on success; negative POSIX error code on error.
 */
static int
rp_xattr_view_load_dos_attrs(RpXAttrView *widget)
{
	// Hide by default.
	// If we do have attributes, we'll show the widgets there.
	gtk_widget_set_visible(widget->fraDosAttributes, false);

	if (!widget->xattrReader->hasDosAttributes()) {
		// No MS-DOS attributes.
		return -ENOENT;
	}

	// We have MS-DOS attributes.
	rp_dos_attr_view_set_attrs(RP_DOS_ATTR_VIEW(widget->dosAttrView),
		widget->xattrReader->dosAttributes());
	gtk_widget_set_visible(widget->fraDosAttributes, true);
	return 0;
}

/**
 * Load the attributes from the specified file.
 * The attributes will be loaded into the display widgets.
 * @param widget XAttrView
 * @return 0 on success; negative POSIX error code on error.
 */
static int
rp_xattr_view_load_attributes(RpXAttrView *widget)
{
	// Attempt to open the file.
	gchar *filename = g_filename_from_uri(widget->uri, nullptr, nullptr);
	if (!filename) {
		// This might be a plain filename and not a URI.
		if (access(widget->uri, R_OK) == 0) {
			// It's a plain filename.
			// TODO: Eliminate g_strdup()?
			filename = g_strdup(widget->uri);
		} else {
			// Not a local file.
			widget->has_attributes = false;
			delete widget->xattrReader;
			widget->xattrReader = nullptr;
			return -EIO;
		}
	}

	// Close the XAttrReader if it's already open.
	delete widget->xattrReader;

	// Open an XAttrReader.
	widget->xattrReader = new XAttrReader(filename);
	g_free(filename);
	int err = widget->xattrReader->lastError();
	if (err != 0) {
		// Error reading attributes.
		delete widget->xattrReader;
		widget->xattrReader = nullptr;
		return err;
	}

	// Load the attributes.
	bool hasAnyAttrs = false;
	int ret = rp_xattr_view_load_ext2_attrs(widget);
	if (ret == 0) {
		hasAnyAttrs = true;
	}
	ret = rp_xattr_view_load_xfs_attrs(widget);
	if (ret == 0) {
		hasAnyAttrs = true;
	}
	ret = rp_xattr_view_load_dos_attrs(widget);
	if (ret == 0) {
		hasAnyAttrs = true;
	}
	ret = rp_xattr_view_load_posix_xattrs(widget);
	if (ret == 0) {
		hasAnyAttrs = true;
	}

	// If we have attributes, great!
	// If not, clear the display widgets.
	if (hasAnyAttrs) {
		widget->has_attributes = true;
	} else {
		widget->has_attributes = false;
		rp_xattr_view_clear_display_widgets(widget);
	}
	return 0;
}

/**
 * Clear the display widgets.
 * @param widget XAttrView
 */
static void
rp_xattr_view_clear_display_widgets(RpXAttrView *widget)
{
	rp_ext2_attr_view_clear_flags(RP_EXT2_ATTR_VIEW(widget->ext2AttrView));
	rp_xfs_attr_view_clear_xflags(RP_XFS_ATTR_VIEW(widget->xfsAttrView));
	rp_xfs_attr_view_clear_project_id(RP_XFS_ATTR_VIEW(widget->xfsAttrView));
	rp_dos_attr_view_clear_attrs(RP_DOS_ATTR_VIEW(widget->dosAttrView));
	gtk_list_store_clear(widget->listStore);
}

/** Property accessors / mutators **/

/**
 * Set the current URI.
 * @param widget XAttrView
 * @param uri URI
 */
void
rp_xattr_view_set_uri(RpXAttrView *widget, const gchar *uri)
{
	g_return_if_fail(RP_IS_XATTR_VIEW(widget));

	if (g_strcmp0(widget->uri, uri) != 0) {
		g_set_str(&widget->uri, uri);
		rp_xattr_view_load_attributes(widget);
		g_object_notify_by_pspec(G_OBJECT(widget), props[PROP_URI]);
	}
}

/**
 * Get the current URI.
 * @param widget XAttrView
 * @return URI
 */
const gchar*
rp_xattr_view_get_uri(RpXAttrView *widget)
{
	g_return_val_if_fail(RP_IS_XATTR_VIEW(widget), 0);
	return widget->uri;
}

/**
 * Are attributes loaded from the current URI?
 * @param widget RpXAttrView
 * @return True if we have attributes; false if not.
 */
gboolean
rp_xattr_view_has_attributes(RpXAttrView *widget)
{
	g_return_val_if_fail(RP_IS_XATTR_VIEW(widget), FALSE);
	return widget->has_attributes;
}
