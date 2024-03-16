/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ common)                      *
 * XfsAttrView.c: XFS file system attribute viewer widget.                 *
 *                                                                         *
 * Copyright (c) 2017-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "XfsAttrView.h"

// XFS flags
#include "librpfile/xattr/xfs_flags.h"

// XfsAttrData
#include "librpfile/xattr/XfsAttrData.h"

/* Property identifiers */
typedef enum {
	PROP_0,

	PROP_XFLAGS,
	PROP_PROJECT_ID,

	PROP_LAST
} RpXfsAttrViewPropID;

static void	rp_xfs_attr_view_set_property(GObject		*object,
					      guint		 prop_id,
					      const GValue	*value,
					      GParamSpec	*pspec);
static void	rp_xfs_attr_view_get_property(GObject		*object,
					      guint		 prop_id,
					      GValue		*value,
					      GParamSpec	*pspec);

/** Signal handlers **/
static void	checkbox_no_toggle_signal_handler(GtkCheckButton	*checkbutton,
						  RpXfsAttrView		*widget);

/** Update display **/
static void	rp_xfs_attr_view_update_xflags_checkboxes(RpXfsAttrView *widget);
static void	rp_xfs_attr_view_update_project_id(RpXfsAttrView *widget);

static GParamSpec *props[PROP_LAST];

static GQuark XfsAttrView_value_quark;

#if GTK_CHECK_VERSION(3,0,0)
typedef GtkBoxClass superclass;
typedef GtkBox super;
#define GTK_TYPE_SUPER GTK_TYPE_BOX
#define USE_GTK_GRID 1	// Use GtkGrid instead of GtkTable.
#else /* !GTK_CHECK_VERSION(3,0,0) */
typedef GtkVBoxClass superclass;
typedef GtkVBox super;
#define GTK_TYPE_SUPER GTK_TYPE_VBOX
#endif /* GTK_CHECK_VERSION(3,0,0) */

// XfsAttrView class
struct _RpXfsAttrViewClass {
	superclass __parent__;
};

// XfsAttrView instance
struct _RpXfsAttrView {
	super __parent__;

	guint32 xflags;
	guint32 project_id;

	// Inhibit checkbox toggling while updating.
	gboolean inhibit_checkbox_no_toggle;

	// See enum XfsCheckboxId and xfsCheckboxInfo.
	GtkWidget *checkBoxes[XFS_ATTR_CHECKBOX_MAX];

	// Project ID
	GtkWidget *lblProjectId;
};

// NOTE: G_DEFINE_TYPE() doesn't work in C++ mode with gcc-6.2
// due to an implicit int to GTypeFlags conversion.
G_DEFINE_TYPE_EXTENDED(RpXfsAttrView, rp_xfs_attr_view,
	GTK_TYPE_SUPER, (GTypeFlags)0, {});

static void
rp_xfs_attr_view_class_init(RpXfsAttrViewClass *klass)
{
	GObjectClass *const gobject_class = G_OBJECT_CLASS(klass);
	gobject_class->set_property = rp_xfs_attr_view_set_property;
	gobject_class->get_property = rp_xfs_attr_view_get_property;

	/** Quarks **/

	// NOTE: Not using g_quark_from_static_string()
	// because the extension can be unloaded.
	XfsAttrView_value_quark = g_quark_from_string("XfsAttrValue.value");

	/** Properties **/

	props[PROP_XFLAGS] = g_param_spec_uint(
		"xflags", "XFlags", "XFS file system file attributes",
		0, G_MAXUINT32, 0,
		(GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY));

	props[PROP_PROJECT_ID] = g_param_spec_uint(
		"project-id", "Project ID", "Project ID",
		0, G_MAXUINT32, 0,
		(GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY));

	// Install the properties.
	g_object_class_install_properties(gobject_class, PROP_LAST, props);
}

static void
rp_xfs_attr_view_init(RpXfsAttrView *widget)
{
#if GTK_CHECK_VERSION(3,0,0)
	// Make this a VBox.
	gtk_orientable_set_orientation(GTK_ORIENTABLE(widget), GTK_ORIENTATION_VERTICAL);
#endif /* GTK_CHECK_VERSION(3,0,0) */

	// Checkboxes
	int col = 0, row = 0;
	static const int col_count = 4;
#ifdef USE_GTK_GRID
	GtkWidget *const gridCheckboxes = gtk_grid_new();
#else /* !USE_GTK_GRID */
	static const int row_count = (XFS_ATTR_CHECKBOX_MAX / col_count) +
				    ((XFS_ATTR_CHECKBOX_MAX % col_count) > 0);
	GtkWidget *const gridCheckboxes = gtk_table_new(row_count, col_count, FALSE);
#endif /* USE_GTK_GRID */
	gtk_widget_set_name(gridCheckboxes, "gridCheckboxes");
	for (size_t i = 0; i < ARRAY_SIZE(widget->checkBoxes); i++) {
		const XfsAttrCheckboxInfo_t *const p = xfsAttrCheckboxInfo((XfsAttrCheckboxID)i);

		GtkWidget *const checkBox = gtk_check_button_new_with_label(
			dpgettext_expr(RP_I18N_DOMAIN, "XfsAttrView", p->label));
		gtk_widget_set_name(checkBox, p->name);
		gtk_widget_set_tooltip_text(checkBox,
			dpgettext_expr(RP_I18N_DOMAIN, "XfsAttrView", p->tooltip));

		widget->checkBoxes[i] = checkBox;

#ifdef USE_GTK_GRID
		gtk_grid_attach(GTK_GRID(gridCheckboxes), checkBox, col, row, 1, 1);
#else /* !USE_GTK_GRID */
		gtk_table_attach(GTK_TABLE(gridCheckboxes), checkBox, col, col+1, row, row+1, GTK_FILL, GTK_FILL, 0, 0);
#endif /* USE_GTK_GRID */

		// Disable user modifications.
		// NOTE: Unlike Qt, both the "clicked" and "toggled" signals are
		// emitted for both user and program modifications, so we have to
		// connect this signal *after* setting the initial value.
		g_signal_connect(checkBox, "toggled", G_CALLBACK(checkbox_no_toggle_signal_handler), widget);

		// Next cell.
		col++;
		if (col == col_count) {
			col = 0;
			row++;
		}
	}

	/** Project ID **/
	// TODO: Best way to display it? Make lblProjectId (and lblLsAttr) selectable?
	GtkWidget *const hboxProjectId = rp_gtk_hbox_new(4);
	gtk_widget_set_name(hboxProjectId, "hboxProjectId");
	GtkWidget *const lblProjectIdDesc = gtk_label_new(C_("XfsAttrView", "Project ID:"));
	gtk_widget_set_name(lblProjectIdDesc, "lblProjectIdDesc");
	widget->lblProjectId = gtk_label_new("0");
	gtk_widget_set_name(widget->lblProjectId, "lblProjectId");

	// Monospace font for lblProjectId
	PangoAttrList *const attr_lst = pango_attr_list_new();
	pango_attr_list_insert(attr_lst, pango_attr_family_new("monospace"));
	gtk_label_set_attributes(GTK_LABEL(widget->lblProjectId), attr_lst);
	pango_attr_list_unref(attr_lst);

#if GTK_CHECK_VERSION(4,0,0)
	gtk_box_append(GTK_BOX(widget), gridCheckboxes);

	gtk_box_append(GTK_BOX(hboxProjectId), lblProjectIdDesc);
	gtk_box_append(GTK_BOX(hboxProjectId), widget->lblProjectId);
	gtk_box_append(GTK_BOX(widget), hboxProjectId);
#else /* !GTK_CHECK_VERSION(4,0,0) */
	gtk_box_pack_start(GTK_BOX(widget), gridCheckboxes, FALSE, FALSE, 0);
	gtk_widget_show_all(gridCheckboxes);

	gtk_box_pack_start(GTK_BOX(hboxProjectId), lblProjectIdDesc, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hboxProjectId), widget->lblProjectId, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(widget), hboxProjectId, FALSE, FALSE, 0);
	gtk_widget_show_all(hboxProjectId);
#endif /* GTK_CHECK_VERSION(4,0,0) */
}

GtkWidget*
rp_xfs_attr_view_new(void)
{
	return (GtkWidget*)g_object_new(RP_TYPE_XFS_ATTR_VIEW, NULL);
}

/** Properties **/

static void
rp_xfs_attr_view_set_property(GObject		*object,
			      guint		 prop_id,
			      const GValue	*value,
			      GParamSpec	*pspec)
{
	RpXfsAttrView *const widget = RP_XFS_ATTR_VIEW(object);

	switch (prop_id) {
		case PROP_XFLAGS:
			rp_xfs_attr_view_set_xflags(widget, g_value_get_uint(value));
			break;

		case PROP_PROJECT_ID:
			rp_xfs_attr_view_set_project_id(widget, g_value_get_uint(value));
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
			break;
	}
}

static void
rp_xfs_attr_view_get_property(GObject		*object,
			      guint		 prop_id,
			      GValue		*value,
			      GParamSpec	*pspec)
{
	RpXfsAttrView *const widget = RP_XFS_ATTR_VIEW(object);

	switch (prop_id) {
		case PROP_XFLAGS:
			g_value_set_uint(value, widget->xflags);
			break;

		case PROP_PROJECT_ID:
			g_value_set_uint(value, widget->project_id);
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
			break;
	}
}

/** Update display **/

/**
 * Update the xflags checkboxes.
 * @param widget XfsAttrView
 */
static void
rp_xfs_attr_view_update_xflags_checkboxes(RpXfsAttrView *widget)
{
	static_assert(ARRAY_SIZE(widget->checkBoxes) == XFS_ATTR_CHECKBOX_MAX,
		"checkBoxes and XFS_ATTR_CHECKBOX_MAX are out of sync!");

	widget->inhibit_checkbox_no_toggle = TRUE;

	// NOTE: Bit 2 is skipped, and the last attribute is 0x80000000.
	uint32_t tmp_xflags = widget->xflags;
	for (size_t i = 0; i < ARRAY_SIZE(widget->checkBoxes)-1; i++, tmp_xflags >>= 1) {
		if (i == 2)
			tmp_xflags >>= 1;
		gboolean val = (tmp_xflags & 1);
		gtk_check_button_set_active(GTK_CHECK_BUTTON(widget->checkBoxes[i]), val);
		g_object_set_qdata(G_OBJECT(widget->checkBoxes[i]), XfsAttrView_value_quark, GUINT_TO_POINTER((guint)val));
	}

	// Final attribute (XFS_chkHasAttr / FS_XFLAG_HASATTR)
	gboolean val = !!(widget->xflags & FS_XFLAG_HASATTR);
	gtk_check_button_set_active(GTK_CHECK_BUTTON(widget->checkBoxes[XFS_chkHasAttr]), val);
	g_object_set_qdata(G_OBJECT(widget->checkBoxes[XFS_chkHasAttr]), XfsAttrView_value_quark, GUINT_TO_POINTER((guint)val));

	widget->inhibit_checkbox_no_toggle = FALSE;
}

/**
 * Update the project ID.
 * @param widget XfsAttrView
 */
static void
rp_xfs_attr_view_update_project_id(RpXfsAttrView *widget)
{
	char buf[16];
	snprintf(buf, sizeof(buf), "%u", widget->project_id);
	gtk_label_set_text(GTK_LABEL(widget->lblProjectId), buf);
}

/** Property accessors / mutators **/

/**
 * Set the current XFS xflags.
 * @param widget XfsAttrView
 * @param xflags XFS xflags
 */
void
rp_xfs_attr_view_set_xflags(RpXfsAttrView *widget, guint32 xflags)
{
	g_return_if_fail(RP_IS_XFS_ATTR_VIEW(widget));

	if (widget->xflags != xflags) {
		widget->xflags = xflags;
		rp_xfs_attr_view_update_xflags_checkboxes(widget);
		g_object_notify_by_pspec(G_OBJECT(widget), props[PROP_XFLAGS]);
	}
}

/**
 * Get the current XFS xflags.
 * @param widget XfsAttrView
 * @return XFS xflags
 */
guint32
rp_xfs_attr_view_get_xflags(RpXfsAttrView *widget)
{
	g_return_val_if_fail(RP_IS_XFS_ATTR_VIEW(widget), 0);
	return widget->xflags;
}

/**
 * Clear the current XFS xflags.
 * @param widget XfsAttrView
 */
void
rp_xfs_attr_view_clear_xflags(RpXfsAttrView *widget)
{
	g_return_if_fail(RP_IS_XFS_ATTR_VIEW(widget));
	if (widget->xflags != 0) {
		widget->xflags = 0;
		rp_xfs_attr_view_update_xflags_checkboxes(widget);
		g_object_notify_by_pspec(G_OBJECT(widget), props[PROP_XFLAGS]);
	}
}

/**
 * Set the current XFS project ID.
 * @param widget XfsAttrView
 * @param xflags XFS project ID
 */
void
rp_xfs_attr_view_set_project_id(RpXfsAttrView *widget, guint32 project_id)
{
	g_return_if_fail(RP_IS_XFS_ATTR_VIEW(widget));

	if (widget->xflags != project_id) {
		widget->project_id = project_id;
		rp_xfs_attr_view_update_project_id(widget);
		g_object_notify_by_pspec(G_OBJECT(widget), props[PROP_PROJECT_ID]);
	}
}

/**
 * Get the current XFS project ID.
 * @param widget XfsAttrView
 * @return XFS project ID
 */
guint32
rp_xfs_attr_view_get_project_id(RpXfsAttrView *widget)
{
	g_return_val_if_fail(RP_IS_XFS_ATTR_VIEW(widget), 0);
	return widget->project_id;
}

/**
 * Clear the current XFS project ID.
 * @param widget XfsAttrView
 */
void
rp_xfs_attr_view_clear_project_id(RpXfsAttrView *widget)
{
	g_return_if_fail(RP_IS_XFS_ATTR_VIEW(widget));
	if (widget->project_id != 0) {
		widget->project_id = 0;
		rp_xfs_attr_view_update_project_id(widget);
		g_object_notify_by_pspec(G_OBJECT(widget), props[PROP_PROJECT_ID]);
	}
}

/** Signal handlers **/

/**
 * Prevent bitfield checkboxes from being toggled.
 * @param checkbutton Bitfield checkbox
 * @param page XfsAttrView
 */
static void
checkbox_no_toggle_signal_handler(GtkCheckButton	*checkbutton,
				  RpXfsAttrView		*widget)
{
	if (widget->inhibit_checkbox_no_toggle) {
		// Inhibiting the no-toggle handler.
		return;
	}

	// Get the saved XfsAttrView value.
	const gboolean value = (gboolean)GPOINTER_TO_UINT(
		g_object_get_qdata(G_OBJECT(checkbutton), XfsAttrView_value_quark));
	if (gtk_check_button_get_active(checkbutton) != value) {
		// Toggle this box.
		gtk_check_button_set_active(checkbutton, value);
	}
}
